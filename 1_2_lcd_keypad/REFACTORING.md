# Refactoring Plan

**Goal:** Clean, efficient, standards-compliant code. No new features. No behaviour changes. Build passes after every phase.  
**Order:** Bottom-to-top (drivers → IO layer → application layer → entry point).  
**Principle:** Simplest correct solution. Remove, don't add.

---

## Rules

- One phase at a time, build & test before moving to the next
- A change is only made if it removes a real problem (dead code, duplication, layering violation, wrong practice)
- No "nice to have" restructuring that changes nothing observable
- Log the phase as ✅ when done

---

## Phase 1 — `config_pins.h`

**Problems:**

- `NVS_NAMESPACE` and `NVS_PIN_KEY` don't belong here — they are NVS key names, not pin/hardware config. They are already re-defined locally in `lock_storage.c` (which shadows them), making the ones here dead.
- `PIN_INPUT_START_COL 0` is a trivially zero constant, not used anywhere meaningful. Dead define.
- Backlight timeout is hardcoded as `60 * 1000` in `lock_system.c` instead of a named constant here.

**Changes:**

1. Remove `NVS_NAMESPACE` and `NVS_PIN_KEY`
2. Remove `PIN_INPUT_START_COL`
3. Add `#define BACKLIGHT_TIMEOUT_SEC 60`
4. Minor: group timing constants together, remove outdated inline comments

---

## Phase 2 — `buzzer.c / buzzer.h`

**Problems:**

- `buzzer_on()` and `buzzer_off()` are public API. They are implementation primitives that no caller outside the module should use directly — all callers use `buzzer_beep()`, `buzzer_success()`, or `buzzer_error()`.
- `TAG` + `ESP_LOGD` in `buzzer_beep()` adds noise with no debugging value. `ESP_LOGI` in `buzzer_init()` is fine to keep.

**Changes:**

1. Make `buzzer_on()` and `buzzer_off()` `static` — remove from header
2. Remove `ESP_LOGD` log in `buzzer_beep()` and `buzzer_error()`

---

## Phase 3 — `led.c / led.h`

**Problems:**

- `led_all_off()` calls `gpio_set_level(GPIO_LED_GREEN, 0)` and `gpio_set_level(GPIO_LED_RED, 0)` directly instead of going through the `led_pins[]` array that exists for exactly this purpose. Inconsistent.
- `ESP_LOGD` in `led_set()` adds noise.

**Changes:**

1. Rewrite `led_all_off()` to loop over `led_pins[]` (or call `led_set()` twice) for consistency
2. Remove `ESP_LOGD` log in `led_set()` and `led_all_off()`

---

## Phase 4 — `keypad.c / keypad.h`

**Problems:**

- `keypad_scan()` has the "set all rows HIGH" block duplicated: once inside the key-found early-return path, once at the end of the function. Extract to a `static void keypad_rows_idle(void)` helper.
- `keypad_is_pressed()` in the public API is redundant — `keypad_getkey() != '\0'` is equivalent. It's only used internally in `keypad_getkey_blocking()`.
- `KEYPAD_SCAN_DELAY_MS 10` in `config_pins.h` — this is a fine constant to keep.

**Changes:**

1. Extract `static void keypad_rows_idle(void)` helper, use it in all three exit paths of `keypad_scan()`
2. Make `keypad_is_pressed()` static (used only in `keypad_getkey_blocking()`) — remove from header

---

## Phase 5 — `lcd_i2c.c / lcd_i2c.h`

**Problems:**

- `lcd_home()` is a public function that is never called outside the driver. `lcd_clear()` and `lcd_set_cursor(0,0)` already cover all use cases.
- `lcd_get_cursor_col()` / `lcd_get_cursor_row()` expose internal driver state to the IO layer (`stdio_redirect.c`). Acceptable for now as the alternative (tracking cursor in stdio_redirect) is more code. Leave them but note the coupling.
- `lcd_clear_row()` exists here AND `lock_ui_clear_row()` in `lock_ui.c` does the exact same thing. The driver version is the right one to keep; the UI wrapper is the duplicate (see Phase 8).
- Docstring for `lcd_putc()` says "Automatically wraps at end of line" — this was true in the 20x4 version, now wrapping is disabled. Update the comment.
- `vTaskDelay(pdMS_TO_TICKS(1))` in `lcd_pulse_enable()` — the EN pulse needs >450ns and the command needs >37µs to settle. A 1ms delay via FreeRTOS tick (typically 10ms on ESP-IDF default config) is acceptable but could also use `esp_rom_delay_us()` for precision. Low priority — leave as-is since the display works.

**Changes:**

1. Remove `lcd_home()` from header and make it `static` in the .c file (or remove entirely since `lcd_set_cursor(0,0)` replaces it)
2. Update `lcd_putc()` docstring: remove "Automatically wraps" — wrap is disabled
3. Update `lcd_set_cursor()` docstring (col 0-15, row 0-1)

---

## Phase 6 — `stdio_redirect.c / stdio_redirect.h`

**Problems:**

- `lcd_scanf_set_mode()` and `lcd_scanf_set_digits_only()` are deprecated since `lcd_scanf_configure()` was added. Dead public API surface.
- The four reveal-state globals (`reveal_buffer`, `reveal_length`, `reveal_row`, `reveal_col`) are logically one unit. Group into a static struct.
- **Layering violation**: `stdio_redirect.c` includes `lock_system.h` and calls `lock_system_reset_backlight_timer()` on each keypress. `stdio_redirect` is a lower layer than `lock_system`. This creates an upward dependency.  
  Fix: Add a single `void (*keypress_cb)(void)` callback pointer, set to `NULL` by default. `lock_system_init()` calls `lcd_scanf_set_keypress_cb(lock_system_reset_backlight_timer)`. The IO layer fires it on every keypress. No circular include needed.
- `display_input()` uses `PIN_MAX_LENGTH` to calculate how many trailing spaces to write. This works but couples the generic input display to a PIN-specific constant. Replace with: clear from `start_col` to `LCD_COLS` minus current length.

**Changes:**

1. Remove `lcd_scanf_set_mode()` and `lcd_scanf_set_digits_only()` (from .c and .h)
2. Group reveal globals into a `static struct { ... } s_reveal` struct
3. Fix layering: replace `#include "lock_system.h"` with a callback:
   - Add `void lcd_scanf_set_keypress_cb(void (*cb)(void))` to header
   - Call `cb()` (if not NULL) in place of `lock_system_reset_backlight_timer()`
   - In `lock_system_init()`: `lcd_scanf_set_keypress_cb(lock_system_reset_backlight_timer)`
4. Fix `display_input()` trailing-space calculation to not reference `PIN_MAX_LENGTH`

---

## Phase 7 — `lock_storage.c / lock_storage.h`

**Problems:**

- Local `#define NVS_NAMESPACE` and `#define NVS_PIN_KEY` shadow the (now removed) ones from `config_pins.h`. After Phase 1 removes the dead config_pins.h ones, these local defines are correct and clean. No change needed — they belong here as private implementation details.
- `lock_storage_save_pin()` opens NVS, writes, commits, then closes. If `nvs_set_str` fails it returns early before close. The `nvs_close()` is missing in the error path. Fix early-return leak.

**Changes:**

1. Fix `lock_storage_save_pin()`: always call `nvs_close()` before returning (even on error paths) — use goto-cleanup pattern or restructure

---

## Phase 8 — `lock_ui.c / lock_ui.h`

**Problems:**

- `lock_ui_clear_row()` duplicates `lcd_clear_row()` from the driver. It's a one-line wrapper with no added value. Remove it and update all callers to call `lcd_clear_row()` directly.
- `lock_ui_clear_content()` calls `lock_ui_clear_row(1)`. After removing `lock_ui_clear_row()`, this becomes `lcd_clear_row(1)`. Check if `lock_ui_clear_content()` is even called anywhere; if not, remove it too.
- Header docstrings say "max 20 chars" and "rows 0-3" — outdated from the 20x4 era. Update to "max 16 chars" and "rows 0-1".
- `lock_ui_display_title()` is a one-liner wrapper over `lock_ui_display_centered(..., 0)`. It's a useful semantic alias — keep it.

**Changes:**

1. Remove `lock_ui_clear_row()` from `lock_ui.c` and `lock_ui.h`
2. Update all callers (`lock_handlers.c`) to call `lcd_clear_row()` directly
3. Check if `lock_ui_clear_content()` has any callers; remove if unused
4. Fix stale docstrings: "20 chars" → "16 chars", "rows 0-3" → "rows 0-1"

---

## Phase 9 — `lock_handlers.c`

**Problems:**

- `lock_handler_changing_code()` displays `"\xE2\x9C\x93"` (UTF-8 check mark ✓). HD44780 character ROM doesn't support multi-byte UTF-8 — this will display garbage or partial characters on the hardware. Replace with an ASCII string.
- `lock_handler_unlocked()` is a 2-line function: `lock_system_start_autolock(); return STATE_MENU;`. It exists only because every state has a handler, but it adds an indirection with no logic. Could be absorbed into the state machine switch in `lock_system.c`. Low priority — leave if the consistency is preferred.
- Several `ESP_LOGI` calls log things like "PIN entered" or "Current PIN entered for validation" that have zero debugging value. Reduce to keep only the meaningful ones (wrong PIN, correct PIN, state changes).
- `lock_handler_locked()` comment says "Turn off all LEDs" before the while loop, but was originally outside the loop — now LED off should only happen once before the loop, which it does. Fine as-is.

**Changes:**

1. Replace `"\xE2\x9C\x93"` (UTF-8 ✓) with `"Verified!"` or just remove that display line
2. Remove low-value `ESP_LOGI` calls (keep: wrong PIN, correct PIN, cancel, lockout)

---

## Phase 10 — `lock_system.c`

**Problems:**

- `backlight_timer_enabled` bool flag is now stale. After the Phase 10.2 bug fix, the reset function correctly uses `xTimerIsTimerActive()`. The flag is still written to in `start_backlight_timer()` and the callback, but never read outside of the now-fixed reset function. Remove it.
- `autolock_enabled` bool flag is used as a guard in `reset_autolock()` and `stop_autolock()`. This is acceptable — it prevents attempting to reset a timer that was never started. Keep it.
- Backlight timer creation uses `pdMS_TO_TICKS(60 * 1000)` — hardcoded. After Phase 1 adds `BACKLIGHT_TIMEOUT_SEC`, use `pdMS_TO_TICKS(BACKLIGHT_TIMEOUT_SEC * 1000)`.
- Both timers are lazily created (allocated on first call). This is fine but consider creating both eagerly in `lock_system_init()` for predictable memory allocation at startup. Low priority.

**Changes:**

1. Remove `backlight_timer_enabled` variable and all assignments to it
2. Replace `60 * 1000` with `BACKLIGHT_TIMEOUT_SEC * 1000`

---

## Phase 11 — `main.c`

**Problems:**

- Startup display uses three `\n` characters to navigate the 16x2 display:  
  `lcd_printf("=== LOCK SYSTEM ===\n")` → row 0, then `\n` moves to row 1, next `\n` wraps back to row 0, etc. This is fragile and produces the wrong visual ('Starting...' may land on row 1, or row 0 depending on tick timing).  
  Use `lcd_set_cursor()` explicitly.
- `"=== LOCK SYSTEM ==="` uses `===` borders that were removed from all other UI in Iteration 8. Inconsistent. Replace with `lock_ui_display_centered()` calls to match the rest of the UI.
- `lock_handler_first_boot_setup()` also shows a welcome screen (`"SECURITY LOCK v1.0"`). The `main.c` startup message and the handler welcome are shown back-to-back — redundant. Remove the startup display from `main.c` and let the first handler handle it. Keep the `buzzer_success()` init beep.

**Changes:**

1. Remove the startup `lcd_printf` block (the 4 lcd_printf calls + delay)
2. Keep `buzzer_success()` and a short `vTaskDelay` as the "boot complete" signal before `lock_system_init()`

---

## Summary Table

| Phase | File(s) | Key Changes | Status |
|-------|---------|-------------|--------|
| 1 | `config_pins.h` + new `config_app.h` | Split hardware vs app constants, remove dead defines | ✅ |
| 2 | `buzzer.c/h` | Make `buzzer_on/off` static | ✅ |
| 3 | `led.c/h` | Fix `led_all_off()` inconsistency, remove debug logs | ✅ |
| 4 | `keypad.c/h` | Extract helper, make `is_pressed` static | ✅ |
| 5 | `lcd_i2c.c/h` | Remove `lcd_home()`, fix stale docstrings | ✅ |
| 6 | `stdio_redirect.c/h` | Remove deprecated API, fix layering with callback, group reveal struct | ✅ |
| 7 | `lock_storage.c` | Fix NVS handle leak on error paths | ⬜ |
| 8 | `lock_ui.c/h` | Remove `clear_row` duplicate, fix stale docs | ⬜ |
| 9 | `lock_handlers.c` | Fix UTF-8 garbage char, trim noise logs | ⬜ |
| 10 | `lock_system.c` | Remove stale flag, use `BACKLIGHT_TIMEOUT_SEC` | ⬜ |
| 11 | `main.c` | Remove redundant startup display | ⬜ |
