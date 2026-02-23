# ESP32 Security Lock System - Lab 1.2

## LCD2004 i2c + Keypad 4×4 + LEDs + Buzzer + STDIO Retargeting

---

## 📋 PROJECT OVERVIEW

### Core Functionality

Lock/unlock security system on **ESP32-WROOM-32 DevKit V1** with:

- **Setup wizard** on first boot to configure initial 4-8 digit PIN
- **Lock/Unlock mechanism** with keypad authentication  
- **Auto-lock** after 30 seconds of inactivity
- **Code change** feature with confirmation
- **STDIO retargeting**: `printf()` → LCD, `scanf()`/`getchar()` → Keypad
- **Visual feedback**: Centered text, asterisk masking, last-char reveal (500ms)
- **Hardware signaling**: Green/Red LEDs, buzzer beeps
- **Persistent storage**: Code saved to NVS (survives reboots)

### Hardware Components

- ESP32 DevKit V1 (ESP-WROOM-32)
- LCD2004 i2c (20×4 characters, PCF8574 backpack)
- Keypad 4×4 matrix
- Green & Red LEDs (220Ω resistors)
- Piezo buzzer
- Wokwi simulator ready

---

## 🔌 HARDWARE MAPPING

### Pin Assignments (match `diagram.json`)

| Component | Pin | GPIO | Notes |
| --------- | --- | ---- | ----- |
| **LCD i2c** | SDA | GPIO21 | i2c Bus 0 |
| | SCL | GPIO22 | i2c Bus 0 |
| | Address | 0x27 | PCF8574 default (allow 0x3F) |
| **Keypad Rows** | R1 | GPIO13 | Output, scan row 1 |
| | R2 | GPIO12 | Output, scan row 2 |
| | R3 | GPIO14 | Output, scan row 3 |
| | R4 | GPIO27 | Output, scan row 4 |
| **Keypad Cols** | C1 | GPIO26 | Input with pullup, read col 1 |
| | C2 | GPIO25 | Input with pullup, read col 2 |
| | C3 | GPIO33 | Input with pullup, read col 3 |
| | C4 | GPIO32 | Input with pullup, read col 4 |
| **LEDs** | Green | GPIO18 | Active HIGH, 220Ω resistor |
| | Red | GPIO19 | Active HIGH, 220Ω resistor |
| **Buzzer** | Signal | GPIO23 | Active HIGH, piezo buzzer |

### Keypad Layout

```txt
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │
└───┴───┴───┴───┘

* = BACKSPACE
# = ENTER/CONFIRM
```

---

## 🏗️ ARCHITECTURE & FILE STRUCTURE

### Modular Component Structure

```txt
1_2_lcd_keypad/
├── platformio.ini              # PlatformIO config (ESP-IDF 5.5.0)
├── CMakeLists.txt              # Root CMake (ESP-IDF project)
├── diagram.json                # Wokwi simulation hardware
├── wokwi.toml                  # Wokwi settings
├── TODO.md                     # This file
├── include/
│   └── config_pins.h          # ⭐ All GPIO/i2c pin definitions & constants
├── lib/
│   ├── lcd_i2c/               # ⭐ LCD2004 driver (HD44780 + PCF8574)
│   │   ├── lcd_i2c.h
│   │   └── lcd_i2c.c
│   ├── keypad/                # ⭐ 4×4 matrix keypad scanner
│   │   ├── keypad.h
│   │   └── keypad.c
│   ├── led/                   # ⭐ Generic LED control
│   │   ├── led.h
│   │   └── led.c
│   ├── buzzer/                # ⭐ Buzzer control (beep patterns)
│   │   ├── buzzer.h
│   │   └── buzzer.c
│   ├── stdio_redirect/        # ⭐ STDIO retargeting (_write/_read syscalls)
│   │   ├── stdio_redirect.h
│   │   └── stdio_redirect.c
│   └── lock_system/           # ⭐ Lock state machine & UI logic
│       ├── lock_system.h      # Public API & state enum
│       ├── lock_system.c      # State machine coordinator
│       ├── lock_storage.h     # NVS persistence API
│       ├── lock_storage.c     # NVS operations
│       ├── lock_ui.h          # UI helper functions
│       ├── lock_ui.c          # Display formatting
│       ├── lock_handlers.h    # State handler declarations
│       └── lock_handlers.c    # State handler implementations
├── src/
│   ├── CMakeLists.txt         # Source component registration
│   └── main.c                 # ⭐ app_main() entry point
└── test/
    └── README
```

### Component Responsibilities

#### `include/config_pins.h`

- Define all GPIO numbers as named constants (no magic numbers)
- i2c bus configuration (speed, address, SDA/SCL)
- Keypad row/column mappings
- Timing constants (debounce, autolock, char reveal duration)

#### `lib/lcd_i2c/` - LCD Driver

**API:**

- `esp_err_t lcd_init(void)` — Initialize i2c + LCD in 4-bit mode
- `void lcd_clear(void)` — Clear display, home cursor
- `void lcd_set_cursor(uint8_t col, uint8_t row)` — Position cursor (0-19 col, 0-3 row)
- `void lcd_putc(char c)` — Write single char, handle \n, wrap, scrolling
- `void lcd_print(const char *str)` — Print string
- `void lcd_backlight(bool on)` — Control backlight

**Implementation Notes:**

- Use ESP-IDF `driver/i2c.h` APIs
- PCF8574 mapping: P0=RS, P1=RW, P2=EN, P3=BL, P4-P7=D4-D7
- 4-bit mode: send high nibble, then low nibble with EN pulse
- Keep backlight bit set in every write
- Implement proper initialization sequence per HD44780 datasheet
- Handle cursor wrapping: 20 cols × 4 rows

#### `lib/keypad/` - Keypad Scanner

**API:**

- `esp_err_t keypad_init(void)` — Configure GPIO (rows=output, cols=input+pullup)
- `char keypad_getkey(void)` — Non-blocking, return '\0' if no key
- `char keypad_getkey_blocking(void)` — Block until key pressed & released

**Implementation Notes:**

- Scan algorithm: Set row LOW one at a time, read columns
- Debounce: Wait 20-50ms, resample to confirm
- Detect key release before returning (one press = one char)
- Character mapping:

  ```c
  const char KEYMAP[4][4] = {
      {'1','2','3','A'},
      {'4','5','6','B'},
      {'7','8','9','C'},
      {'*','0','#','D'}
  };
  ```

- Use ESP-IDF `driver/gpio.h` APIs

#### `lib/led/` - LED Control

**API:**

- `esp_err_t led_init(void)` — Configure LED GPIOs as output
- `void led_set(led_color_t color, bool state)` — Set specific LED on/off
- `void led_all_off(void)` — Turn off all LEDs

**Types:**

```c
typedef enum {
    LED_GREEN,
    LED_RED
} led_color_t;
```

#### `lib/buzzer/` - Buzzer Control

**API:**

- `esp_err_t buzzer_init(void)` — Configure buzzer GPIO
- `void buzzer_beep(uint32_t duration_ms)` — Single beep
- `void buzzer_success(void)` — Short beep (100ms) for valid code
- `void buzzer_error(void)` — Triple beep pattern (100ms on, 50ms off, repeat 3×)

**Implementation:**

- Use `vTaskDelay()` for timing (FreeRTOS)
- Non-blocking option: Create separate task for patterns

#### `lib/stdio_redirect/` - STDIO Retargeting

**API:**

- `esp_err_t stdio_redirect_init(void)` — Initialize STDIO system
- `int lcd_printf(const char *format, ...)` — Printf-style output to LCD
- `int lcd_scanf(const char *format, ...)` — Scanf-style input from keypad
- `void lcd_scanf_set_mode(input_mode_t mode)` — Set input masking mode
- `void lcd_scanf_set_centered(bool centered)` — Enable/disable centered input

**Input Modes:**

- `INPUT_MODE_NORMAL` — Display characters as-is
- `INPUT_MODE_MASKED` — Display all as asterisks (*)
- `INPUT_MODE_REVEAL_LAST` — Show last character for 500ms, then mask

**Features:**

- Printf-style formatting with vsnprintf
- Backspace (`*`) removes last character with visual update
- Enter (`#`) ends input
- Dynamic centering of input as user types
- Automatic mode reset after each scanf

#### `lib/lock_system/` - Lock State Machine & UI

**Modular Architecture:** (Single Responsibility Principle)

**lock_system.h/.c** - State Machine Coordinator

- `esp_err_t lock_system_init(void)` — Initialize system, load PIN from NVS
- `void lock_system_run(void)` — Main state machine loop

**lock_storage.h/.c** - NVS Persistence

- `esp_err_t lock_storage_init(void)` — Initialize NVS flash
- `bool lock_storage_load_pin(char *pin_buf, size_t buf_size)` — Load PIN
- `esp_err_t lock_storage_save_pin(const char *pin)` — Save PIN with validation

**lock_ui.h/.c** - UI Helper Functions

- `void lock_ui_display_centered(const char *text, uint8_t row)` — Center text
- `void lock_ui_display_title(const char *title)` — Display bordered title
- `void lock_ui_display_error(const char *message, uint32_t delay_ms)` — Format error
- `void lock_ui_display_success(const char *message, uint32_t delay_ms)` — Format success

**lock_handlers.h/.c** - State Handler Functions

- One handler function per state (first_boot, locked, unlocked, menu, etc.)
- Clean separation of business logic from state machine

**States:**

```c
typedef enum {
    STATE_FIRST_BOOT_SETUP,    // Setup wizard, set initial code
    STATE_LOCKED,              // Wait for PIN entry to unlock
    STATE_UNLOCKED,            // Unlocked, show menu, start autolock timer
    STATE_MENU,                // Show options: Lock, Change Code
    STATE_SETTING_CODE,        // User entering new code
    STATE_CONFIRMING_CODE,     // User confirming new code
    STATE_CHANGING_CODE,       // Enter old code → new code → confirm
    STATE_AUTH_SUCCESS,        // Show success message, transition
    STATE_AUTH_FAILED          // Show error message, retry
} lock_state_t;
```

**UI Behaviors:**

- **Centered text**: Calculate start column = (20 - strlen(text)) / 2
- **Masked input (unlock)**: Show asterisks `*` centered as user types
- **New code entry**: Show last char for 500ms, then replace with `*`
- **Backspace**: Move cursor back, overwrite with space, move back again
- **Auto-lock timer**: FreeRTOS timer, reset on any activity, trigger after 30s

**NVS Storage:**

- Key: `"user_pin"`, Value: 4-8 digit string
- Initialize NVS with `nvs_flash_init()`
- Check if code exists; if not, enter `STATE_FIRST_BOOT_SETUP`

---

## 🔄 STATE MACHINE LOGIC

### State Transitions

```txt
┌─────────────────┐
│ FIRST_BOOT_SETUP│ ← First boot or no code in NVS
└────────┬────────┘
         │ Code set & confirmed
         ↓
    ┌─────────┐
    │ LOCKED  │ ← Default state, show "Enter PIN:"
    └────┬────┘
         │ Correct PIN entered
         ↓
    ┌──────────┐
    │ UNLOCKED │ ← Show menu, start 30s autolock timer
    └────┬─────┘
         │
         ├─→ User selects "Lock" → LOCKED
         ├─→ User selects "Change Code" → CHANGING_CODE
         └─→ 30s timeout → LOCKED (auto-lock)

CHANGING_CODE:
  1. Prompt "Enter current PIN:"
  2. Validate → If wrong, return to UNLOCKED with error
  3. Prompt "Enter new PIN (4-8 digits):"
  4. Prompt "Confirm new PIN:"
  5. If match → Save to NVS → UNLOCKED
  6. If mismatch → Error, retry step 3
```

### Key Decision Points

| State | Input | Action |
| ----- | ----- | ------ |
| FIRST_BOOT_SETUP | User enters PIN | Prompt confirmation → Save to NVS → LOCKED |
| LOCKED | User enters PIN | Validate → match: UNLOCKED ✅ / mismatch: AUTH_FAILED ❌ |
| UNLOCKED | '1' key | Lock manually → LOCKED |
| UNLOCKED | '2' key | Change code → CHANGING_CODE |
| UNLOCKED | 30s idle | Auto-lock → LOCKED |
| CHANGING_CODE | Validated old PIN | Prompt new code |
| CONFIRMING_CODE | Codes match | Save to NVS → Success |
| CONFIRMING_CODE | Mismatch | Error → Retry |

---

## ⚙️ TECHNICAL GUIDELINES

### ESP-IDF Specific Patterns

**1. I2C Initialization:**

```c
i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = GPIO_LCD_SDA,
    .scl_io_num = GPIO_LCD_SCL,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000, // 100kHz standard mode
};
ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
```

**2. GPIO Configuration Pattern:**

```c
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_PIN),
    .mode = GPIO_MODE_OUTPUT,  // or GPIO_MODE_INPUT
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};
ESP_ERROR_CHECK(gpio_config(&io_conf));
```

**3. NVS Storage Pattern:**

```c
#include "nvs_flash.h"
#include "nvs.h"

// Initialize NVS
ESP_ERROR_CHECK(nvs_flash_init());

// Open handle
nvs_handle_t nvs_handle;
ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));

// Write string
ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "user_pin", "1234"));
ESP_ERROR_CHECK(nvs_commit(nvs_handle));

// Read string
size_t required_size;
nvs_get_str(nvs_handle, "user_pin", NULL, &required_size);
char *value = malloc(required_size);
nvs_get_str(nvs_handle, "user_pin", value, &required_size);

// Close
nvs_close(nvs_handle);
```

**4. FreeRTOS Delays:**

- Use `vTaskDelay(pdMS_TO_TICKS(ms))` instead of blocking loops
- For timers: `xTimerCreate()` and  `xTimerStart()` for autolock

**5. STDIO Retargeting:**

- Implement in separate file: `stdio_redirect.c`
- Newlib syscalls: `_write()`, `_read()`
- Link automatically during build

### Coding Standards (per requirements)

- **No magic numbers**: Define all constants in `config_pins.h`
- **CamelCase naming**: `keypadInit()`, `lcdSetCursor()`, or use snake_case for C consistency
- **Error handling**: Check return values, use `ESP_ERROR_CHECK()` macro
- **Comments**: Document all public APIs in headers
- **Modular design**: Each peripheral = separate library for reuse

### LCD PCF8574 Bit Mapping

```txt
PCF8574 Pin:  P7  P6  P5  P4  P3  P2  P1  P0
HD44780 Pin:  D7  D6  D5  D4  BL  EN  RW  RS

BL = Backlight (keep HIGH)
EN = Enable (pulse HIGH→LOW to latch data)
RW = Read/Write (keep LOW for write)
RS = Register Select (0=command, 1=data)
```

### HD44780 4-bit Initialization Sequence

1. Wait 50ms after power-on
2. Send 0x03 (function set 8-bit), wait 5ms
3. Send 0x03, wait 1ms
4. Send 0x03, wait 1ms
5. Send 0x02 (function set 4-bit)
6. Send 0x28 (4-bit, 2 lines, 5×8 font)
7. Send 0x0C (display ON, cursor OFF)
8. Send 0x06 (entry mode: increment cursor)
9. Send 0x01 (clear display)

---

## 📝 IMPLEMENTATION PLAN (Iterative Development)

### **ITERATION 1: Foundation - Hardware Drivers**

**Goal:** Build and test all peripheral drivers independently.

**Tasks:**

- [x] **1.1 Create `include/config_pins.h`**
  - Define all GPIO constants (LCD_SDA, LCD_SCL, KEYPAD_R1, etc.)
  - Define i2c address, bus number
  - Define timing constants (DEBOUNCE_MS=50, AUTOLOCK_SEC=30, CHAR_REVEAL_MS=500)

- [x] **1.2 Implement `lib/lcd_i2c/`**
  - `lcd_i2c.h`: Declare API functions
  - `lcd_i2c.c`:
    - Initialize i2c bus
    - Implement PCF8574 write function (send byte via i2c)
    - Implement 4-bit send (high nibble, EN pulse, low nibble, EN pulse)
    - HD44780 init sequence
    - `lcd_clear()`, `lcd_set_cursor()`, `lcd_putc()` with line wrapping
  - Test: Print "Hello World" on row 0, "ESP32 LCD Test" on row 1

- [x] **1.3 Implement `lib/keypad/`**
  - `keypad.h`: Declare API
  - `keypad.c`:
    - Configure rows as output, columns as input+pullup
    - Scan function: iterate rows, read columns, map to char
    - Debounce logic (wait, resample)
    - Detect key release
  - Test: Read key, print to LCD using `lcd_print()`

- [x] **1.4 Implement `lib/led/`**
  - `led.h`: Define `led_color_t` enum, API functions
  - `led.c`:
    - Initialize GPIOs
    - `led_set()` implementation
  - Test: Blink green/red alternately

- [x] **1.5 Implement `lib/buzzer/`**
  - `buzzer.h`: API functions
  - `buzzer.c`:
    - Initialize GPIO
    - `buzzer_beep()` with `vTaskDelay()`
    - `buzzer_success()`, `buzzer_error()` patterns
  - Test: Play beep on keypress

- [x] **1.6 Basic `src/main.c`**
  - Initialize all peripherals
  - Simple test loop: read keypad → display on LCD → beep

**Verification:**

- [x] Run Wokwi simulation
- [x] Press keys on keypad → see characters on LCD
- [x] LEDs toggle correctly
- [x] Buzzer beeps

---

### **ITERATION 2: STDIO Retargeting**

**Goal:** Implement `printf()` and `scanf()` redirection to LCD/Keypad.

**Tasks:**

- [x] **2.1 Implement `lib/stdio_redirect/`**
  - `stdio_redirect.h`: Declare `stdio_redirect_init()`, `lcd_printf()`, `lcd_scanf()`
  - `stdio_redirect.c`:
    - Implement `lcd_printf()`: wrapper using vsnprintf + lcd_print
    - Implement `lcd_scanf()`: read from keypad, echo to LCD, handle `*` backspace, `#` to end input
    - `stdio_redirect_init()`: minimal setup (wrapper-based approach)
  - ✅ Now properly located in `lib/stdio_redirect/` (refactored from `src/`)

- [x] **2.2 Update `src/main.c`**
  - Call `stdio_redirect_init()` after peripheral init
  - Test with:

    ```c
    char input[16];
    printf("Type something:\n");
    scanf("%15s", input);
    printf("You typed: %s\n", input);
    ```

**Verification:**

- [x] Type on keypad → see characters appear on LCD via `printf()`
- [x] Press `#` → input captured by `scanf()`
- [x] Backspace (`*`) removes last char visually

---

### **ITERATION 3: Simple Lock/Unlock (Hardcoded PIN)**

**Goal:** Basic authentication with hardcoded PIN "1234".

**Tasks:**

- [x] **3.1 Simple state machine in `src/main.c`**
  - States: LOCKED, UNLOCKED
  - LOCKED: prompt "Enter PIN:", read 4 chars, validate
  - Match: green LED, `printf("✓ Unlocked\n")`, buzzer success → UNLOCKED
  - Mismatch: red LED, `printf("✗ Wrong PIN\n")`, buzzer error → stay LOCKED
  - UNLOCKED: show "Press # to lock", wait for `#` → LOCKED

- [x] **3.2 LED and buzzer feedback**
  - Success: green ON, red OFF, 100ms beep
  - Failure: red ON, green OFF, triple beep pattern

**Verification:**

- [x] Enter "1234" → unlocks with green LED
- [x] Enter wrong PIN → red LED, error beep
- [x] Press '#' when unlocked → locks again

---

### **ITERATION 4: NVS Storage & Initial Setup**

**Goal:** Persist PIN to NVS, implement first-boot setup wizard.

**Tasks:**

- [x] **4.1 NVS Integration**
  - Initialize NVS in `lock_system_init()`: `nvs_flash_init()`
  - Create functions in `lib/lock_system/`:
    - `load_pin_from_nvs(char *pin_buf, size_t buf_size)` → returns true if exists
    - `save_pin_to_nvs(const char *pin)` → saves to NVS with validation

- [x] **4.2 First Boot Setup**
  - On startup: try `load_pin_from_nvs()`
  - If PIN doesn't exist:
    - Display "=== SETUP WIZARD ==="
    - STATE_FIRST_BOOT_SETUP → STATE_SETTING_CODE
    - Prompt "Set PIN (4-8 digits):"
    - Validate: length (4-8), digits only (0-9)
    - Prompt "Confirm PIN:"
    - Match → save to NVS → transition to LOCKED
    - Mismatch → error, retry from STATE_SETTING_CODE

- [x] **4.3 Code Refactoring**
  - Extracted state machine to `lib/lock_system/lock_system.c`
  - Moved NVS functions to lock_system module
  - Simplified `main.c` to just peripheral init + lock_system_run()
  - Each state has dedicated handler function
  - Clean separation of concerns

**Verification:**

- [x] First boot → setup wizard appears
- [x] Set PIN, confirm → saved to NVS
- [x] Reboot → PIN loaded, setup skipped
- [x] Unlock with saved PIN works
- [x] PIN validation (length, digits only) works
- [x] Code properly modularized across lib/ modules

---

### **ITERATION 5: Full State Machine & Change Code**

**Goal:** Implement complete state machine with menu and code changing.

**Tasks:**

- [x] **5.1 State machine architecture**
  - Added STATE_MENU and STATE_CHANGING_CODE to enum
  - Implemented dedicated handler functions:
    - `handle_menu()` - Display menu and handle selection
    - `handle_changing_code()` - Validate old PIN before allowing change
    - Updated `handle_unlocked()` - Show success then transition to menu
  - Added `return_state` variable to track where to return after PIN change

- [x] **5.2 UNLOCKED menu**
  - Display:

    ```txt
    === MENU ===
    
    1. Lock System
    2. Change PIN
    ```

  - Read keypad input with validation
  - '1' → LOCKED (with lock confirmation)
  - '2' → STATE_CHANGING_CODE
  - Invalid input → error beep
  - Green LED stays on during menu

- [x] **5.3 Change Code Flow**
  - Prompt "Enter current PIN:" in STATE_CHANGING_CODE
  - Validate old PIN:
    - Correct → Set return_state=MENU, go to STATE_SETTING_CODE
    - Wrong → Error message, return to MENU
  - Reuse existing STATE_SETTING_CODE and STATE_CONFIRMING_CODE
  - After successful change, return to MENU (not LOCKED)
  - Full validation maintained (4-8 digits, numbers only)

**Verification:**

- [x] Unlock → see menu with options
- [x] Select "1" → locks system
- [x] Select "2" → prompts for current PIN
- [x] Wrong old PIN → error, return to menu
- [x] Correct old PIN → proceeds to new PIN entry
- [x] New PIN saved to NVS successfully
- [x] After change → returns to menu (not locked)

---

### **ITERATION 6: UI Enhancements (Centering & Masking)**

**Goal:** Implement centered text, asterisk masking, last-char reveal.

**Tasks:**

- [x] **6.1 Centered Text Helper**
  - Implemented `lock_ui_display_centered()` in `lock_ui.c`
  - Calculate: `col_start = (20 - strlen(text)) / 2`
  - Clear row, set cursor, print text
  - Used throughout all state handlers

- [x] **6.2 Asterisk Masking for Unlock**
  - Implemented three input modes in `stdio_redirect.c`:
    - `INPUT_MODE_NORMAL` - display as-is
    - `INPUT_MODE_MASKED` - full asterisk masking (unlock)
    - `INPUT_MODE_REVEAL_LAST` - show last char, then mask (PIN setup)
  - When masked: display `*` for each char, keep centered as input grows

- [x] **6.3 Last-Char Reveal for New PIN Entry**
  - `INPUT_MODE_REVEAL_LAST` implemented
  - Display actual char for CHAR_REVEAL_MS (500ms)
  - Replace with `*` after timeout using `vTaskDelay()`
  - Applied to PIN setting and confirmation states

- [x] **6.4 Dynamic Centering During Input**
  - `display_input()` helper function recalculates center on each keystroke
  - Clears entire row, repositions centered content
  - Handles backspace with proper re-centering
  - Smooth visual feedback as user types

**Verification:**

- [x] Unlock screen shows centered asterisks as typing
- [x] Setting new code shows last char briefly (500ms)
- [x] Backspace properly updates centered display
- [x] All handlers use centered text helpers
- [x] Tested in Wokwi simulator - all features working

---

### **ITERATION 7: Auto-Lock Timer & Polish**

**Goal:** Implement auto-lock, finalize UX, optimize performance.

**Tasks:**

- [x] **7.1 Auto-Lock Timer**
  - Created FreeRTOS timer in `lock_system.c`
  - Timer implementation:
    - `lock_system_start_autolock()` - Start 30-second timer
    - `lock_system_reset_autolock()` - Reset on keypress
    - `lock_system_stop_autolock()` - Stop when leaving menu
    - `autolock_timer_callback()` - Transitions to LOCKED on timeout
  - Started when entering UNLOCKED/MENU states
  - Reset on any keypress in MENU
  - Displays "AUTO-LOCKED" message on timeout
  - Properly stops timer when user manually locks or changes PIN

- [x] **7.2 Input Validation**
  - Implemented `lcd_scanf_set_digits_only(bool)` function
  - PIN entry now only accepts '0'-'9' digits
  - Rejects A, B, C, D with short error beep (30ms)
  - Applied to all PIN entry states:
    - LOCKED (unlock screen)
    - SETTING_CODE (new PIN)
    - CONFIRMING_CODE (confirm PIN)
    - CHANGING_CODE (validate old PIN)
  - Automatic filter reset after each scanf

- [x] **7.3 UX Polish**
  - Clean LCD transitions between all states
  - Centered messages throughout
  - Consistent visual feedback with checkmarks (✓)
  - Auto-lock timer properly restarted after PIN change
  - Smart state returns (menu restarts auto-lock, locked doesn't)
  - Buzzer feedback for invalid input
  - All messages formatted consistently

- [x] **7.4 Code Optimization**
  - Modular timer functions (start/stop/reset)
  - Auto-reset of all input modes after scanf
  - Efficient state transitions
  - Proper resource cleanup (LEDs, timers)
  - Minimal LCD refreshes (only when state changes)
  - Optimized keypad polling in menu
  - Clean separation of concerns across modules

**Verification:**

- [x] Sit idle in UNLOCKED for 30s → auto-locks with message
- [x] Invalid input (A,B,C,D) rejected with beep during PIN entry
- [x] Smooth transitions between all states
- [x] No screen flicker or repeated refreshes
- [x] Timer resets on menu keypress
- [x] Timer stops when locking manually
- [x] Auto-lock restarts after PIN change

---

## ✅ TESTING & VERIFICATION

### Unit Tests (per iteration)

- **LCD**: Print test patterns, verify cursor positioning
- **Keypad**: Press each key, verify correct character returned
- **LEDs**: Toggle each LED, verify state
- **Buzzer**: Test single beep and error pattern
- **NVS**: Save/load PIN, verify persistence across reboots

### Integration Tests

1. **Full Flow Test:**
   - First boot → setup wizard → set PIN → confirm → lock
   - Unlock with correct PIN → green LED + beep
   - Try wrong PIN → red LED + error beep
   - Change code → validate → confirm → success
   - Wait 30s → auto-lock triggers

2. **Edge Cases:**
   - Enter PIN <4 digits → error
   - Enter PIN >8 digits → truncate or error
   - Rapid key presses → debounce works
   - Backspace on empty input → no crash
   - Mismatch on confirmation → retry works

3. **Visual Tests (Wokwi):**
   - Centered text appears correctly on LCD
   - Asterisks display properly while typing
   - Last char reveals for 500ms then masks
   - Backspace updates display smoothly
   - LCD doesn't flicker or show garbage

### Lab Demonstration Checklist

- [ ] Wokwi simulation runs without errors
- [ ] Hardware connections match `diagram.json`
- [ ] Code structured in separate modules (reusable)
- [ ] STDIO `printf()`/`scanf()` working (demonstrate)
- [ ] Lock/unlock with keypad works
- [ ] LED feedback correct (green=success, red=error)
- [ ] Buzzer patterns play
- [ ] Code persists across reboots (NVS)
- [ ] Auto-lock after 30s works
- [ ] Code follows CamelCase standard
- [ ] No magic numbers (all in `config_pins.h`)

---

## 📚 LAB REPORT REQUIREMENTS

### Sections to Include

1. **Introduction**
   - Purpose: User interaction with Keypad + LCD using STDIO
   - Objectives: Understand peripherals, STDIO retargeting, modular design

2. **Hardware Description**
   - Block diagram showing ESP32 connections
   - Electrical schematic (Wokwi diagram export or hand-drawn)
   - Component list with specifications

3. **Software Architecture**
   - Module structure diagram
   - State machine diagram (all states and transitions)
   - STDIO retargeting explanation:
     - How `_write()` redirects to LCD
     - How `_read()` gets input from keypad
     - Character echo and backspace handling

4. **Implementation Details**
   - Key code snippets for each module (not full code dump)
   - PCF8574 bit mapping explanation
   - Keypad matrix scanning algorithm
   - NVS storage mechanism

5. **Testing & Results**
   - Screenshots from Wokwi showing:
     - Setup wizard
     - Unlock screen with masked input
     - Menu display
     - Success/error states
   - Test case table with expected vs actual results

6. **Conclusions**
   - What was learned
   - Challenges faced and solutions
   - Possible improvements (e.g., password strength indicator, multiple users)

### Submission Format

- Follow UTM report standards (cover page, pagination, etc.)
- Include code as appendix or link to repository
- Deadline: Check course schedule (-10% per week late)

---

## 🎯 BONUS OPPORTUNITIES (+10%)

- **Pre-lab validation**: Test components before lab session
- **Additional features**:
  - Multiple user codes (admin + user roles)
  - Failed attempt counter (lock after 3 tries)
  - Custom welcome message stored in NVS
  - LCD backlight auto-dim after idle
  - Morse code mode for buzzer output
- Contact instructor to confirm bonus eligibility

---

## 📌 NOTES & REMINDERS

### Critical Points

- **Native ESP-IDF**: Do NOT use Arduino libraries (incompatible)
- **Wokwi Config**: `diagram.json` is already set up, don't modify pins
- **STDIO**: Must be unbuffered (`_IONBF`) for immediate I/O
- **Debouncing**: Essential for keypad reliability
- **Memory**: Ensure buffers sized correctly (max PIN=8 digits + null terminator)

### Common Pitfalls

- Forgetting to pulse EN when writing to LCD (no display update)
- Wrong PCF8574 bit positions (display shows garbage)
- Blocking I/O without FreeRTOS delays (watchdog timer triggers)
- Not checking NVS return values (crashes if NVS not initialized)
- Hardcoded column positions (breaks when text length changes)

### Useful Resources

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/)
- [HD44780 LCD Datasheet](https://www.sparkfun.com/datasheets/LCD/HD44780.pdf)
- [PCF8574 Datasheet](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [ESP32 GPIO Matrix](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)

---

## 🚀 CURRENT STATUS

**Iteration:** Iteration 7 (Auto-Lock & Polish) - COMPLETE ✅
**Completed Tasks:** 40+/40+
**Progress:** 100% Complete! 🎉

- ✅ Iteration 1: COMPLETE - All hardware drivers working
  - LCD i2c, Keypad, LED, Buzzer modules fully functional
  - All in separate `lib/` modules with clean APIs
  
- ✅ Iteration 2: COMPLETE - STDIO redirect (lcd_printf/lcd_scanf) implemented
  - Wrapper functions in `lib/stdio_redirect/`
  - Printf-style formatting to LCD
  - Scanf-style input from keypad with backspace support
  
- ✅ Iteration 3: COMPLETE - Lock/unlock functionality
  - State machine with LOCKED/UNLOCKED states
  - PIN validation with visual/audio feedback
  - LED indicators (green=unlocked, red=error)
  
- ✅ Iteration 4: COMPLETE - NVS Storage & First-Boot Setup
  - NVS integration for PIN persistence
  - First-boot setup wizard (STATE_FIRST_BOOT_SETUP)
  - PIN setting + confirmation flow (STATE_SETTING_CODE, STATE_CONFIRMING_CODE)
  - Input validation (4-8 digits, numbers only)
  - **Code refactored:** State machine extracted to `lib/lock_system/`
  - **Clean architecture:** main.c now only 93 lines (init + run)
  - PIN persists across reboots
  
- ✅ Iteration 5: COMPLETE - Menu System & PIN Change
  - STATE_MENU with "Lock" and "Change PIN" options
  - STATE_CHANGING_CODE validates old PIN before allowing change
  - Reuses existing PIN entry/confirmation states
  - Smart return_state handling (LOCKED for setup, MENU for changes)
  - Full user flow: Unlock → Menu → Change PIN → Validate Old → Set New → Confirm → Back to Menu
  - All validation rules enforced throughout
  
- ✅ **REFACTORING: Single Responsibility Principle Applied**
  - Split `lock_system` into 4 focused modules:
    - `lock_system.c` (93 lines) - State machine coordinator only
    - `lock_storage.c` (114 lines) - NVS operations only
    - `lock_ui.c` (89 lines) - UI/display helpers only
    - `lock_handlers.c` (287 lines) - State handlers only
  - Each module has clear, single responsibility
  - Improved maintainability, testability, reusability
  
- ✅ Iteration 6: COMPLETE - UI Enhancements
  - **Centered text:** All titles and messages use `lock_ui_display_centered()`
  - **Input masking modes:**
    - `INPUT_MODE_MASKED` - Full asterisks for unlock screen
    - `INPUT_MODE_REVEAL_LAST` - Last char reveal (500ms) for PIN setup
    - `INPUT_MODE_NORMAL` - Plain text display
  - **Dynamic centering:** Input stays centered as user types
  - **Smart backspace:** Properly updates centered masked input
  - All handlers updated to use new UI features
  - Tested and verified in Wokwi simulator
  
- ✅ Iteration 7: COMPLETE - Auto-Lock Timer & Final Polish
  - **Auto-lock timer (30 seconds):**
    - FreeRTOS timer with start/stop/reset functions
    - Automatically locks after 30s of inactivity in menu
    - Resets on any keypress
    - Shows "AUTO-LOCKED" message
  - **Enhanced input validation:**
    - Digits-only filter (`lcd_scanf_set_digits_only()`)
    - Rejects A,B,C,D during PIN entry with error beep
    - Applied to all PIN input states
  - **UX polish:**
    - Smooth transitions between all states
    - Consistent message formatting
    - Proper timer restart after PIN changes
    - Clean resource management
  - **Code optimization:**
    - Modular timer management
    - Efficient state handling
    - Minimal LCD refreshes
    - Auto-reset of input modes

**Project Complete!** ✅

All core requirements implemented:

- ✓ Setup wizard on first boot
- ✓ Lock/Unlock with keypad authentication
- ✓ Auto-lock after 30 seconds
- ✓ PIN change with confirmation
- ✓ STDIO retargeting (lcd_printf/lcd_scanf)
- ✓ Centered text, masked input, last-char reveal
- ✓ LED & buzzer feedback
- ✓ NVS persistent storage
- ✓ Modular, maintainable architecture

**Next Steps:**

1. ✓ Test all features in Wokwi simulator
2. Prepare lab demonstration
3. Write lab report
4. (Optional) Implement bonus features

---

**Last Updated:** 2026-02-23
**Project Type:** Embedded Systems Lab Assignment
**Platform:** ESP32-IDF 5.5.0 + Wokwi
**Target:** Lock/Unlock Security System with STDIO Retargeting
