# ESP32 Security Lock System

**Platform:** ESP32-WROOM-32 DevKit V1 · ESP-IDF 5.5.0 · PlatformIO · Wokwi

A PIN-based security lock implemented entirely in native ESP-IDF (no Arduino). The system uses a 4×4 matrix keypad for input and an LCD1602 display for output, with full STDIO retargeting so all application logic is written with plain `printf()` / `scanf()`.

## Features

- **First-boot setup wizard** — prompts user to set a 4–8 digit PIN on first power-on; saved to NVS
- **PIN authentication** — masked input (`*`) with last-char reveal (300 ms) during new PIN entry
- **Failed attempt lockout** — 3 wrong PINs trigger a 30 s timed lockout with live countdown
- **PIN change flow** — verify current PIN, set new PIN, confirm; cancellable at every step with 'C'
- **Auto-lock timer** — menu inactivity for 15 s automatically locks the system
- **Backlight auto-off** — any inactivity for 30 s turns backlight off; next keypress restores it
- **Full STDIO retargeting** — `printf()` → LCD, `scanf()` → keypad via ESP-IDF VFS drivers
- **NVS persistence** — PIN hash (SHA-256) survives reboots and power cycles; plaintext PIN is never written to flash
- **LED + buzzer feedback** — green/red LEDs and distinct beep patterns for every event
- **Wokwi simulation** — full hardware simulation via `diagram.json`

## Hardware

| Component | GPIO | Notes |
| --------- | ---- | ----- |
| LCD SDA | 21 | I²C Bus 0 |
| LCD SCL | 22 | I²C Bus 0 |
| LCD Address | 0x27 | PCF8574 backpack (0x3F fallback) |
| Keypad R1–R4 | 13, 12, 14, 27 | Row outputs |
| Keypad C1–C4 | 26, 25, 33, 32 | Column inputs with pullup |
| LED Green | 18 | Active HIGH, 220 Ω |
| LED Red | 19 | Active HIGH, 220 Ω |
| Buzzer | 23 | Active HIGH |

### Keypad Layout

```txt
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  A = Change PIN
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │  C = Cancel
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │  D = Lock
└───┴───┴───┴───┘

* = Backspace  # = Enter
```

## Project Structure

```txt
1_2_lcd_keypad/
├── include/
│   ├── config_pins.h        # GPIO numbers, I²C settings, LCD dimensions
│   ├── config_app.h         # Timeouts, PIN policy, UI timing
│   ├── lock_system.h        # State machine API & state enum
│   ├── lock_storage.h       # NVS persistence API
│   ├── lock_ui.h            # Display helper API
│   └── lock_handlers.h      # State handler declarations
├── lib/
│   ├── lcd_i2c/             # HD44780 driver (4-bit mode via PCF8574)
│   ├── keypad/              # 4×4 matrix scanner (blocking + non-blocking)
│   ├── led/                 # Green/Red LED control
│   ├── buzzer/              # Beep patterns (success, error, reject)
│   ├── stdio_redirect/      # VFS-based STDIO retargeting
│   └── lock_system/
│       ├── lock_system.c    # State machine coordinator + FreeRTOS timers
│       ├── lock_storage.c   # NVS persistence
│       ├── lock_ui.c        # Display helpers
│       └── lock_handlers.c  # One handler function per state
└── src/main.c               # app_main(): peripheral init → lock_system_run()
```

Each library has a single responsibility and no dependency on application code. `lock_system/` is the only module that depends on the other libraries.

## LCD Driver

The LCD1602 uses the HD44780 controller driven in **4-bit mode** over I²C through a PCF8574 I/O expander.

**PCF8574 bit mapping:**

| P7 | P6 | P5 | P4 | P3 | P2 | P1 | P0 |
|----|----|----|----|----|----|----|----|
| D7 | D6 | D5 | D4 | BL | EN | RW | RS |

Every write sends the high nibble then the low nibble, each with an EN pulse. The backlight bit (BL) is kept set in every transaction.

**HD44780 DDRAM layout (16×2):** Row 0 starts at `0x00`, row 1 at `0x40`. The two rows are non-contiguous — writing past column 15 on row 0 advances the cursor to invisible address `0x10`. Auto-wrap is therefore disabled; `lcd_putc()` clamps the cursor to the visible area.

## Keypad Driver

The scanner drives rows LOW one at a time and reads the column lines. A keypress is confirmed after a 50 ms debounce resample. `keypad_getkey_blocking()` waits for key release before returning, ensuring one press yields exactly one character. `keypad_getkey()` is the non-blocking variant used in the menu idle loop.

## LED & Buzzer Feedback

All feedback is event-driven; no polling or background tasks are used.

| Event | LED | Buzzer |
| ----- | --- | ------ |
| Correct PIN / unlock | Green ON, Red OFF | `buzzer_success()` — 1× 100 ms beep |
| Wrong PIN | Red ON, Green OFF | `buzzer_error()` — 3× (100 ms on / 50 ms off) |
| Lockout (3 failures) | Red ON continuous | `buzzer_error()` on entry |
| Rejected key (digits-only filter) | — | `buzzer_beep(BUZZER_REJECT_MS)` — 30 ms beep |
| Menu / unlocked idle | Green ON | — |
| Locked / lockout | Red OFF, Green OFF | — |

## STDIO Retargeting

stdout and stdin are redirected at the VFS layer using ESP-IDF's `esp_vfs_register()` and `freopen()`:

| Stream | VFS path | Backed by |
| ------ | -------- | --------- |
| `stdout` | `/dev/lcd` | `lcd_putc()` — every `printf()` writes to the LCD |
| `stdin` | `/dev/kbd` | `keypad_getkey_blocking()` — every `scanf()` reads from the keypad |

`esp_log_set_vprintf()` is called **before** `freopen()` so `ESP_LOGI`/`ESP_LOGE` remain on UART throughout. `setvbuf(stdout, NULL, _IONBF, 0)` disables output buffering so characters appear immediately.

The keyboard VFS driver (`kbd_vfs_read`) collects a full line into an internal buffer on the first `read()` call, then drains it across subsequent calls until `scanf()` has consumed the token — matching standard line-discipline behaviour.

### Input Modes

Set before each `scanf()` call via `set_input_mode()`:

| Mode | Behaviour |
| ---- | --------- |
| `INPUT_MODE_NORMAL` | Characters displayed as typed |
| `INPUT_MODE_MASKED` | All characters shown as `*` |
| `INPUT_MODE_REVEAL_LAST` | Last character visible for 300 ms (non-blocking FreeRTOS timer), then masked |

The `digits_only` flag (second parameter of `set_input_mode()`) filters out non-digit keys during PIN entry — rejected keys produce a short beep. Both flags reset automatically after `scanf()` returns.

`set_keypress_cb()` registers a callback fired on every keypress. The lock system uses this to reset the auto-lock and backlight timers without coupling the input layer to application logic.

## State Machine

```txt
FIRST_BOOT_SETUP ──► SETTING_CODE ──► CONFIRMING_CODE ──match──► LOCKED
                           ▲                 │
                           └──── mismatch ───┘

LOCKED ──correct PIN──► UNLOCKED ──► MENU
  ▲                                    │
  │◄─── idle timeout / 'D' key ────────┤
  │◄─── 3 wrong PINs ──► LOCKOUT ──────┘

MENU ──'A'──► CHANGING_CODE ──verify old──► SETTING_CODE ──► CONFIRMING_CODE ──► MENU
```

The coordinator in `lock_system_run()` dispatches to a dedicated handler function for each state (`lock_handlers.c`). Handlers are the only layer that calls `printf()`/`scanf()` and drives state transitions — keeping display logic, storage, and state management cleanly separated.

### Key Decision Points

| State | Input | Outcome |
| ----- | ----- | ------- |
| `FIRST_BOOT_SETUP` | PIN set + confirmed | Save to NVS → `STATE_LOCKED` |
| `FIRST_BOOT_SETUP` | Confirmation mismatch | Error → retry `STATE_SETTING_CODE` |
| `LOCKED` | Correct PIN | Reset failed counter → `STATE_UNLOCKED` |
| `LOCKED` | Wrong PIN (< 3) | `led_error` + `buzzer_error` → re-prompt |
| `LOCKED` | Wrong PIN (= 3) | → `STATE_LOCKOUT` (30 s countdown) |
| `LOCKED` | 'C' key | Clear input, re-prompt — **no** failed attempt counted |
| `MENU` | 'A' key | → `STATE_CHANGING_CODE` |
| `MENU` | 'D' key | → `STATE_LOCKED` |
| `MENU` | 15 s idle | Auto-lock → `STATE_LOCKED` ("AUTO-LOCKED" shown) |
| `CHANGING_CODE` | Old PIN correct | → `STATE_SETTING_CODE` |
| `CHANGING_CODE` | Old PIN wrong | Error → `STATE_MENU` |
| `CONFIRMING_CODE` | PINs match | Save to NVS → `STATE_MENU` |
| `CONFIRMING_CODE` | Mismatch | Error → retry `STATE_SETTING_CODE` |
| `LOCKOUT` | 30 s elapsed | → `STATE_LOCKED` (counter reset) |

### States

| State | Description |
| ----- | ----------- |
| `STATE_FIRST_BOOT_SETUP` | No PIN in NVS — shows welcome screen, runs setup wizard |
| `STATE_SETTING_CODE` | User enters a new PIN (4–8 digits, digits-only enforced) |
| `STATE_CONFIRMING_CODE` | User re-enters PIN to confirm |
| `STATE_LOCKED` | Waiting for PIN entry to unlock |
| `STATE_UNLOCKED` | Shows success, transitions to menu |
| `STATE_MENU` | Options: A = Change PIN, D = Lock |
| `STATE_CHANGING_CODE` | Verifies current PIN before allowing change |
| `STATE_LOCKOUT` | 3 failed attempts → 30 s timed lockout with countdown |

### FreeRTOS Timers

| Timer | Period | Action |
| ----- | ------ | ------ |
| Auto-lock | `AUTOLOCK_TIMEOUT_SEC` | `STATE_MENU` inactivity → `STATE_LOCKED` |
| Backlight | `BACKLIGHT_TIMEOUT_SEC` | Inactivity → backlight off; next keypress restores it |
| Char reveal | `CHAR_REVEAL_MS` | Masks the last revealed character after the window expires |

All timer callbacks are non-blocking (no `vTaskDelay`). The `set_keypress_cb` hook resets both activity timers on every keypress.

### Cancel ('C') Key

| Context | Result |
| ------- | ------ |
| `STATE_LOCKED` | Clears current input, re-prompts (no failed attempt counted) |
| `STATE_CHANGING_CODE` | "Cancelled" → `STATE_MENU` |
| `STATE_SETTING_CODE` (change flow) | "Cancelled" → `STATE_MENU` |
| `STATE_SETTING_CODE` (first boot) | Not cancellable — re-prompts |
| `STATE_CONFIRMING_CODE` | Returns to `STATE_SETTING_CODE` |

## Key Constants (`config_app.h` / `config_pins.h`)

| Constant | Value | Purpose |
| -------- | ----- | ------- |
| `PIN_MIN_LENGTH` | 4 | Minimum PIN length |
| `PIN_MAX_LENGTH` | 8 | Maximum PIN length |
| `MAX_FAILED_ATTEMPTS` | 3 | Attempts before lockout |
| `AUTOLOCK_TIMEOUT_SEC` | 15 | Idle in MENU → LOCKED |
| `BACKLIGHT_TIMEOUT_SEC` | 30 | Idle → backlight off |
| `LOCKOUT_DURATION_SEC` | 30 | Lockout duration |
| `CHAR_REVEAL_MS` | 300 | Last-char reveal window |
| `BUZZER_REJECT_MS` | 30 | Short beep for rejected key |
| `LCD_COLS` / `LCD_ROWS` | 16 / 2 | Display dimensions |

## Display Layout (16×2)

```txt
Row 0:  Title / prompt      "LOCKED"  ·  "New PIN (C=X)"
Row 1:  Input / status      "****"    ·  "Wrong PIN!"
```

Titles are centered using `lock_ui_display_centered()`. Input is left-aligned from the cursor position after the prompt, with masked characters (`*`) appended on each keypress. State transitions avoid `lcd_clear()` where possible — only the affected row is rewritten to eliminate visible flicker.

## Building & Running

```bash
# Build
pio run

# Flash & Monitor
pio run --target upload --target monitor

# Wokwi simulation
# Open diagram.json in the Wokwi VS Code extension
```

**Dependencies:** ESP-IDF 5.5.0 (via PlatformIO `espressif32` platform). No Arduino libraries.

## NVS Storage

The raw PIN is **never stored**. Before writing to NVS, `lock_handlers.c` computes a SHA-256 digest of the PIN via `mbedtls_sha256()` and stores the 64-character lower-hex string under namespace `"storage"`, key `"user_pin"`.

All comparisons (unlock, PIN change verification) hash the input first and compare the digest using a constant-time routine (`ct_str_equal`) to prevent timing side-channels.

On first boot (or erased flash), `lock_storage_load_pin()` returns `false` and the setup wizard runs.

## Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/)
- [HD44780 LCD Datasheet](https://www.sparkfun.com/datasheets/LCD/HD44780.pdf)
- [PCF8574 Datasheet](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [ESP32 GPIO Matrix](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
