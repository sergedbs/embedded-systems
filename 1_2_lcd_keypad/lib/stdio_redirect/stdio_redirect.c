/**
 * @file stdio_redirect.c
 * @brief VFS-based STDIO redirection: stdout → LCD, stdin → GPIO keypad.
 *
 * Registers two ESP-IDF VFS drivers:
 *   /dev/lcd  — character write device (stdout)
 *   /dev/kbd  — line-discipline read device (stdin)
 *
 * After stdio_redirect_init() completes:
 *   - printf() / puts() / putchar() write directly to the LCD.
 *   - scanf() / fgets() block on the GPIO keypad with masking & reveal.
 *   - ESP_LOGI/ESP_LOGE are re-routed to stderr (UART) via
 *     esp_log_set_vprintf() so debug output is never lost.
 */

#include "stdio_redirect.h"
#include "config_pins.h"
#include "config_app.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "esp_vfs.h"
#include "esp_log.h"
#include "lcd_i2c.h"
#include "keypad.h"
#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

static input_mode_t s_input_mode  = INPUT_MODE_NORMAL;
static bool         s_digits_only = false;
static void       (*s_keypress_cb)(void) = NULL;
static bool         s_cancelled   = false;

// Keypad line buffer filled by one keypad session, drained by read() calls.
// Sized for up to 64 chars + '\n' + '\0' = 66 bytes total.
#define KBD_BUF_SIZE 66
static char   s_line_buf[KBD_BUF_SIZE];
static size_t s_line_len = 0;   // valid bytes in buffer (includes '\n')
static size_t s_line_pos = 0;   // next byte to hand to scanf

// Character reveal timer state
static struct {
    TimerHandle_t timer;
    char         *buffer;
    size_t        length;
    uint8_t       row;
    uint8_t       col;
} s_reveal = {0};

// ---------------------------------------------------------------------------
// Reveal timer callback
// ---------------------------------------------------------------------------

static void reveal_timer_callback(TimerHandle_t xTimer)
{
    if (s_reveal.buffer && s_reveal.length > 0) {
        lcd_set_cursor(s_reveal.col + s_reveal.length - 1, s_reveal.row);
        lcd_putc('*');
    }
}

// ---------------------------------------------------------------------------
// LCD display helper — renders buffer with masking, no flicker
// ---------------------------------------------------------------------------

static void render_input(const char *buf, size_t len,
                         uint8_t row, uint8_t col)
{
    lcd_set_cursor(col, row);
    for (size_t i = 0; i < len; i++) {
        if (s_input_mode == INPUT_MODE_NORMAL) {
            lcd_putc(buf[i]);
        } else if (s_input_mode == INPUT_MODE_MASKED) {
            lcd_putc('*');
        } else { // INPUT_MODE_REVEAL_LAST
            lcd_putc((i == len - 1) ? buf[i] : '*');
        }
    }
    // Clear remainder of the input area to avoid stale chars
    size_t available = (size_t)(LCD_COLS - col);
    size_t clear     = (available > len) ? (available - len) : 0;
    for (size_t i = 0; i < clear; i++) {
        lcd_putc(' ');
    }
}

// ---------------------------------------------------------------------------
// UART vprintf — preserves ESP_LOG* output after stdout is redirected
// ---------------------------------------------------------------------------

static int uart_log_vprintf(const char *fmt, va_list ap)
{
    // stderr is never redirected; it stays on the console UART.
    return vfprintf(stderr, fmt, ap);
}

// ---------------------------------------------------------------------------
// LCD VFS driver  (/dev/lcd → stdout)
// ---------------------------------------------------------------------------

static int lcd_vfs_open(const char *path, int flags, int mode)
{
    return 0; // single virtual device — local fd 0
}

static int lcd_vfs_close(int fd)
{
    return 0;
}

static ssize_t lcd_vfs_write(int fd, const void *data, size_t size)
{
    const char *src = (const char *)data;
    for (size_t i = 0; i < size; i++) {
        lcd_putc(src[i]); // lcd_putc handles '\n', '\r', '\b'
    }
    return (ssize_t)size;
}

static int lcd_vfs_fstat(int fd, struct stat *st)
{
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFCHR;
    return 0;
}

static const esp_vfs_t s_lcd_vfs = {
    .flags  = ESP_VFS_FLAG_DEFAULT,
    .open   = lcd_vfs_open,
    .close  = lcd_vfs_close,
    .write  = lcd_vfs_write,
    .fstat  = lcd_vfs_fstat,
};

// ---------------------------------------------------------------------------
// Keypad VFS driver  (/dev/kbd → stdin)
// ---------------------------------------------------------------------------

static int kbd_vfs_open(const char *path, int flags, int mode)
{
    return 0;
}

static int kbd_vfs_close(int fd)
{
    return 0;
}

/**
 * @brief Collect one line of keypad input into s_line_buf.
 *
 * Blocks until '#' (Enter) or 'C' (Cancel) is pressed.
 * On success, s_line_buf contains the typed characters followed by '\n'
 * so that scanf can detect the end of the token.
 *
 * @return true on success, false if the user cancelled with 'C'.
 */
static bool kbd_collect_line(void)
{
    size_t  idx       = 0;
    uint8_t start_col = lcd_get_cursor_col();
    uint8_t start_row = lcd_get_cursor_row();

    // Clear input area (preserve prompt to the left of the cursor)
    for (uint8_t i = start_col; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    lcd_set_cursor(start_col, start_row);

    while (idx < KBD_BUF_SIZE - 2) { // reserve room for '\n' and '\0'
        char key = keypad_getkey_blocking();

        if (s_keypress_cb) s_keypress_cb();

        if (key == 'C') {
            // Cancel — stop reveal timer and reset state
            if (s_reveal.timer && xTimerIsTimerActive(s_reveal.timer)) {
                xTimerStop(s_reveal.timer, 0);
            }
            s_input_mode  = INPUT_MODE_NORMAL;
            s_digits_only = false;
            return false;
        }

        if (s_digits_only) {
            if (key != '*' && key != '#' && (key < '0' || key > '9')) {
                buzzer_beep(BUZZER_REJECT_MS);
                continue;
            }
        }

        if (key == '#') {
            // Enter / Confirm
            if (s_reveal.timer && xTimerIsTimerActive(s_reveal.timer)) {
                xTimerStop(s_reveal.timer, 0);
            }
            // Mask the last revealed character before confirming
            if (s_input_mode == INPUT_MODE_REVEAL_LAST && idx > 0) {
                lcd_set_cursor(start_col + idx - 1, start_row);
                lcd_putc('*');
            }
            break;

        } else if (key == '*') {
            // Backspace
            if (idx > 0) {
                if (s_reveal.timer && xTimerIsTimerActive(s_reveal.timer)) {
                    xTimerStop(s_reveal.timer, 0);
                }
                idx--;
                render_input(s_line_buf, idx, start_row, start_col);
            }

        } else {
            // Regular character
            s_line_buf[idx++] = key;
            render_input(s_line_buf, idx, start_row, start_col);

            if (s_input_mode == INPUT_MODE_REVEAL_LAST) {
                if (s_reveal.timer && xTimerIsTimerActive(s_reveal.timer)) {
                    xTimerStop(s_reveal.timer, 0);
                }
                s_reveal.buffer = s_line_buf;
                s_reveal.length = idx;
                s_reveal.row    = start_row;
                s_reveal.col    = start_col;
                xTimerStart(s_reveal.timer, 0);
            }
        }
    }

    // Terminate with '\n' so scanf sees a complete token
    s_line_buf[idx]     = '\n';
    s_line_buf[idx + 1] = '\0';
    s_line_len          = idx + 1; // includes '\n'
    s_line_pos          = 0;

    s_input_mode        = INPUT_MODE_NORMAL;
    s_digits_only       = false;
    s_reveal.buffer     = NULL;
    s_reveal.length     = 0;
    return true;
}

static ssize_t kbd_vfs_read(int fd, void *dst, size_t size)
{
    // If the line buffer is exhausted, collect a fresh line
    if (s_line_pos >= s_line_len) {
        bool ok = kbd_collect_line();
        if (!ok) {
            // Cancel: signal EOF so scanf returns EOF (-1)
            s_cancelled = true;
            return 0;
        }
    }

    size_t avail   = s_line_len - s_line_pos;
    size_t to_copy = (avail < size) ? avail : size;
    memcpy(dst, s_line_buf + s_line_pos, to_copy);
    s_line_pos += to_copy;
    return (ssize_t)to_copy;
}

static int kbd_vfs_fstat(int fd, struct stat *st)
{
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFCHR;
    return 0;
}

static const esp_vfs_t s_kbd_vfs = {
    .flags  = ESP_VFS_FLAG_DEFAULT,
    .open   = kbd_vfs_open,
    .close  = kbd_vfs_close,
    .read   = kbd_vfs_read,
    .fstat  = kbd_vfs_fstat,
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

esp_err_t stdio_redirect_init(void)
{
    // Create the one-shot reveal timer
    s_reveal.timer = xTimerCreate(
        "reveal",
        pdMS_TO_TICKS(CHAR_REVEAL_MS),
        pdFALSE,
        NULL,
        reveal_timer_callback
    );
    if (s_reveal.timer == NULL) {
        return ESP_FAIL;
    }

    // Register LCD VFS path
    esp_err_t ret = esp_vfs_register("/dev/lcd", &s_lcd_vfs, NULL);
    if (ret != ESP_OK) return ret;

    // Register Keypad VFS path
    ret = esp_vfs_register("/dev/kbd", &s_kbd_vfs, NULL);
    if (ret != ESP_OK) return ret;

    // Preserve ESP_LOG* on UART *before* we redirect stdout away from it
    esp_log_set_vprintf(uart_log_vprintf);

    // Redirect stdout → LCD; disable buffering so every printf is immediate
    if (freopen("/dev/lcd", "w", stdout) == NULL) return ESP_FAIL;
    setvbuf(stdout, NULL, _IONBF, 0);

    // Redirect stdin → keypad
    if (freopen("/dev/kbd", "r", stdin) == NULL) return ESP_FAIL;

    return ESP_OK;
}

void set_input_mode(input_mode_t mode, bool digits_only)
{
    s_input_mode  = mode;
    s_digits_only = digits_only;
    s_cancelled   = false;
    // Discard any leftover buffer from a previous (possibly cancelled) read
    s_line_len    = 0;
    s_line_pos    = 0;
    // Clear EOF/error set by a previous cancel so the next scanf can proceed
    clearerr(stdin);
}

void set_keypress_cb(void (*cb)(void))
{
    s_keypress_cb = cb;
}

bool stdin_was_cancelled(void)
{
    return s_cancelled;
}
