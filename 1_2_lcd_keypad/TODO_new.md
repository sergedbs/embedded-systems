# ESP32 Security Lock System - Lab 1.2

## LCD1602 i2c + Keypad 4×4 + LEDs + Buzzer + STDIO Retargeting

**Platform:** ESP32-WROOM-32 DevKit V1 · ESP-IDF 5.5.0 · PlatformIO · Wokwi

---

## 📋 PROJECT OVERVIEW

Lock/unlock security system with:

- First-boot setup wizard (4-8 digit PIN, saved to NVS)
- Lock/unlock via keypad with asterisk masking + last-char reveal (300ms)
- Auto-lock after 30s idle; backlight off after 60s idle
- PIN change with old-PIN verification
- Failed attempt lockout (3 tries → 30s countdown)
- Cancel ('C') for PIN-change operations
- Green/Red LEDs + buzzer feedback
- STDIO retargeting: `lcd_printf()` / `lcd_scanf()` wrappers

---

## 🔌 HARDWARE MAPPING

| Component | GPIO | Notes |
| --------- | ---- | ----- |
| LCD SDA | GPIO21 | i2c Bus 0 |
| LCD SCL | GPIO22 | i2c Bus 0 |
| LCD Addr | 0x27 | PCF8574 backpack |
| Keypad R1–R4 | GPIO13, 12, 14, 27 | Output (row scan) |
| Keypad C1–C4 | GPIO26, 25, 33, 32 | Input + pullup |
| LED Green | GPIO18 | Active HIGH, 220Ω |
| LED Red | GPIO19 | Active HIGH, 220Ω |
| Buzzer | GPIO23 | Active HIGH |

### Keypad Layout

```txt
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  A = Change PIN (menu)
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │  C = Cancel (PIN-change flow only)
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │  * = Backspace  # = Enter  D = Lock (menu)
└───┴───┴───┴───┘
```

---

## 🏗️ FILE STRUCTURE

```txt
1_2_lcd_keypad/
├── include/config_pins.h          # All GPIO, timing & LCD constants
├── lib/
│   ├── lcd_i2c/                   # HD44780 16x2 driver (PCF8574 i2c)
│   ├── lcd_i2c_20x4_archive/      # Archived 20x4 driver (reference only)
│   ├── keypad/                    # 4×4 matrix scanner (blocking + non-blocking)
│   ├── led/                       # Green/Red LED control
│   ├── buzzer/                    # Beep patterns (success, error)
│   ├── stdio_redirect/            # lcd_printf() / lcd_scanf() wrappers
│   └── lock_system/
│       ├── lock_system.c/h        # State machine coordinator + FreeRTOS timers
│       ├── lock_storage.c/h       # NVS persistence
│       ├── lock_ui.c/h            # Display helpers (centered, title, error, success)
│       └── lock_handlers.c/h      # One handler function per state
└── src/main.c                     # app_main(): peripheral init + lock_system_run()
```

### Key Constants (`config_pins.h`)

| Constant | Value | Purpose |
| -------- | ----- | ------- |
| `LCD_COLS` / `LCD_ROWS` | 16 / 2 | Display size |
| `AUTOLOCK_SEC` | 30 | Menu idle → LOCKED |
| `BACKLIGHT_SEC` | 60 | Idle → backlight off |
| `CHAR_REVEAL_MS` | 300 | Last-char reveal window |
| `PIN_MIN_LENGTH` | 4 | Minimum PIN length |
| `PIN_MAX_LENGTH` | 8 | Maximum PIN length |

---

## 🔄 STATE MACHINE

```txt
FIRST_BOOT_SETUP ──► SETTING_CODE ──► CONFIRMING_CODE ──match──► LOCKED
                          ▲                  │
                          └──── mismatch ────┘

LOCKED ──correct PIN──► UNLOCKED ──► MENU
  ▲                                    │
  │◄─── 30s idle / 'D' key ────────────┤
  │◄─── 3 wrong PINs ──► LOCKOUT ──────┘

MENU ──'A'──► CHANGING_CODE ──verify old──► SETTING_CODE ──► CONFIRMING_CODE ──► MENU
```

### Cancel ('C') Behaviour

| State | 'C' pressed | Result |
| ----- | ----------- | ------ |
| `LOCKED` | Clears current input, re-prompts | No failed attempt counted |
| `CHANGING_CODE` | "Cancelled" message | → `STATE_MENU` |
| `SETTING_CODE` (change) | "Cancelled" message | → `STATE_MENU` |
| `SETTING_CODE` (first boot) | Not cancelable | Re-prompts silently |
| `CONFIRMING_CODE` | Re-enter PIN | → `STATE_SETTING_CODE` |

### 16x2 Display Layout

- **Row 0:** Title / prompt (e.g. `LOCKED`, `New PIN (C=X)`)
- **Row 1:** Input area / status message

---

## 📝 ITERATION LOG

| # | Goal | Status |
| - | ---- | ------ |
| 1 | Hardware drivers — LCD, keypad, LED, buzzer | ✅ |
| 2 | STDIO retargeting (`lcd_printf` / `lcd_scanf`) | ✅ |
| 3 | Simple lock/unlock (hardcoded PIN) | ✅ |
| 4 | NVS persistence + first-boot setup wizard | ✅ |
| 5 | Full state machine + PIN change flow | ✅ |
| 6 | Centered text, asterisk masking, last-char reveal | ✅ |
| 7 | Auto-lock timer, digits-only filter, UX polish | ✅ |
| 8 | Non-blocking reveal, A/D menu keys, flicker elimination | ✅ |
| 9 | Lockout counter, backlight timer, welcome message | ✅ |
| 10 | Bug fixes, 16x2 migration, cancel key | ✅ (10.5 skipped) |

### Iteration 10 Details

- [x] **10.1 Auto-lock bug** — menu was blocked in `keypad_getkey_blocking()`; replaced with non-blocking poll loop that checks `*ctx->current_state != STATE_MENU`
- [x] **10.2 Backlight timer bug** — was gating `xTimerReset()` on a stale flag; replaced with `xTimerIsTimerActive()` check
- [x] **10.3 Row clearing / lockout flicker** — lockout title drawn once before countdown; only row 1 updated per second; `lock_ui_clear_row(1)` added before prompts where missing
- [x] **10.4 Cancel ('C') key** — `lcd_scanf()` returns `-1` on 'C'; all affected handlers updated (see cancel table above)
- [x] **10.6 LCD 20x4 → 16x2 migration** — old driver archived to `lib/lcd_i2c_20x4_archive/`; new driver uses row offsets `{0x00, 0x40}` with auto-wrap disabled; all UI rewritten for 2-row layout
- [⏭️] **10.5 True STDIO syscalls** — skipped; keeping `lcd_printf`/`lcd_scanf` wrappers

---

## ✅ LAB DEMONSTRATION CHECKLIST

- [x] Wokwi simulation runs, matches `diagram.json`
- [x] First boot → setup wizard → PIN saved to NVS → survives reboot
- [x] Unlock with correct PIN (green LED + beep)
- [x] Wrong PIN → red LED + error beep; 3 failures → 30s lockout countdown
- [x] Auto-lock: 30s idle in menu → LOCKED ("AUTO-LOCKED" shown)
- [x] Backlight: 60s idle → off; any keypress → on + timer reset
- [x] PIN change: verify old → set new → confirm → back to menu
- [x] Cancel: 'C' exits change-PIN flow correctly at each step
- [x] 'C' at lock screen clears input without counting as failed attempt
- [x] Masked input (unlock) + reveal-last (PIN setup) both work
- [x] No text artifacts or flicker on state transitions
- [x] No magic numbers; all constants in `config_pins.h`
- [x] Modular `lib/` structure, single responsibility per module
- [x] `lcd_printf()` / `lcd_scanf()` demonstrate STDIO retargeting

---

## 📚 LAB REPORT OUTLINE

1. **Introduction** — purpose, objectives
2. **Hardware Description** — pin table, PCF8574 bit mapping, Wokwi schematic
3. **Software Architecture** — module diagram, state machine, STDIO retargeting
4. **Implementation Details** — HD44780 DDRAM addressing (16x2 non-contiguous rows), keypad scan algorithm, NVS pattern, FreeRTOS timer usage
5. **Testing & Results** — Wokwi screenshots per state, test case table
6. **Conclusions** — lessons learned, challenges, possible improvements

---

## 📌 NOTES

- **Native ESP-IDF only** — no Arduino libraries
- **HD44780 16x2 DDRAM**: row 0 = `0x00–0x0F`, row 1 = `0x40–0x4F` (non-contiguous). Writing 16 chars to row 0 advances cursor to invisible address `0x10`. Auto-wrap is disabled in `lcd_putc()` to prevent this.
- **FreeRTOS timer callbacks** must be non-blocking (no `vTaskDelay`)
- **NVS**: must call `nvs_flash_init()` before opening a handle; always check return values

### Resources

- [ESP-IDF Docs v5.5](https://docs.espressif.com/projects/esp-idf/en/v5.5/)
- [HD44780 Datasheet](https://www.sparkfun.com/datasheets/LCD/HD44780.pdf)
- [PCF8574 Datasheet](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)

---

**Last Updated:** 2026-02-23 · **Status:** Iteration 10 Complete ✅
