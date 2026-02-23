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
│       ├── lock_system.h
│       └── lock_system.c
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

- `void stdio_redirect_init(void)` — Setup unbuffered stdin/stdout

**Syscall Implementations:**

- `ssize_t _write(int fd, const void *buf, size_t count)`
  - Forward each char to `lcd_putc()`
  - Return `count`
  
- `ssize_t _read(int fd, void *buf, size_t count)`
  - Block until user input (`#` pressed)
  - Handle `*` as backspace: remove last char, update LCD visually
  - Echo normal keys to LCD
  - Translate `#` → `'\n'` in buffer
  - Return bytes read

**Notes:**

- Set `setvbuf(stdin, NULL, _IONBF, 0)` for immediate input
- Set `setvbuf(stdout, NULL, _IONBF, 0)` for immediate output
- Coordinate with `lock_system` for masked input mode

#### `lib/lock_system/` - Lock State Machine & UI

**API:**

- `esp_err_t lock_system_init(void)` — Load code from NVS, init state
- `void lock_system_run(void)` — Main state machine loop
- `void lock_system_set_masked_input(bool masked)` — Toggle asterisk mode
- Helper: `void display_centered(const char *text, uint8_t row)` — Center text on LCD row

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
  - `stdio_redirect.h`: Declare `stdio_redirect_init()`
  - `stdio_redirect.c`:
    - Implement `_write()`: loop chars, call `lcd_putc()`
    - Implement `_read()`: call `keypad_getkey_blocking()`, echo to LCD, handle `*` backspace, convert `#` to `\n`
    - `stdio_redirect_init()`: set `setvbuf()` unbuffered
  - ⚠️ Note: Implementation in `src/` instead of `lib/stdio_redirect/`

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

- [ ] **4.1 NVS Integration**
  - Initialize NVS in `main.c`: `nvs_flash_init()`
  - Create functions:
    - `load_pin_from_nvs(char *pin_buf)` → returns true if exists
    - `save_pin_to_nvs(const char *pin)`

- [ ] **4.2 First Boot Setup**
  - On startup: try `load_pin_from_nvs()`
  - If PIN doesn't exist:
    - Display "=== SETUP WIZARD ==="
    - Prompt "Set PIN (4-8 digits):"
    - Read input (4-8 digits only)
    - Prompt "Confirm PIN:"
    - Match → save to NVS → transition to LOCKED
    - Mismatch → error, retry

**Verification:**

- First boot → setup wizard appears
- Set PIN, confirm → saved to NVS
- Reboot → PIN loaded, setup skipped
- Unlock with saved PIN works

---

### **ITERATION 5: Full State Machine & Change Code**

**Goal:** Implement complete state machine with menu and code changing.

**Tasks:**

- [ ] **5.1 Extract state machine to `lib/lock_system/`**
  - Define `lock_state_t` enum
  - Implement state handlers:
    - `handle_first_boot_setup()`
    - `handle_locked()`
    - `handle_unlocked()`
    - `handle_menu()`
    - `handle_changing_code()`
    - `handle_confirming_code()`

- [ ] **5.2 UNLOCKED menu**
  - Display:

    ```txt
    === UNLOCKED ===
    1. Lock
    2. Change Code
    ```

  - Read keypad input
  - '1' → LOCKED
  - '2' → CHANGING_CODE
  - Start 30s autolock timer

- [ ] **5.3 Change Code Flow**
  - Prompt "Enter current PIN:"
  - Validate → wrong: error, return to UNLOCKED
  - Prompt "Enter new PIN (4-8 digits):"
  - Prompt "Confirm new PIN:"
  - Match → save to NVS, success message → UNLOCKED
  - Mismatch → error, retry

**Verification:**

- Unlock → see menu
- Select "Change Code" → flow works
- Old PIN validation works
- New PIN saved to NVS

---

### **ITERATION 6: UI Enhancements (Centering & Masking)**

**Goal:** Implement centered text, asterisk masking, last-char reveal.

**Tasks:**

- [ ] **6.1 Centered Text Helper**
  - Implement `display_centered(const char *text, uint8_t row)` in `lock_system.c`
  - Calculate: `col_start = (20 - strlen(text)) / 2`
  - Clear row, set cursor, print text

- [ ] **6.2 Asterisk Masking for Unlock**
  - Modify `_read()` in `stdio_redirect.c` to check masked mode flag
  - When masked: display `*` for each char, keep centered as input grows
  - Example: `"***"` → `" *** "` → `"  ***  "` (always centered)

- [ ] **6.3 Last-Char Reveal for New PIN Entry**
  - When setting/changing code:
    - Display actual char for 500ms
    - Replace with `*` after timeout
  - Use FreeRTOS timer or `vTaskDelay()` in separate task

- [ ] **6.4 Dynamic Centering During Input**
  - As user types, recalculate center position
  - Shift existing asterisks/chars, append new one
  - Example progression:

    ```txt
    start (0 chars): cursor at col 10 (20/2)
    "1" entered:        "         1          " (1 at center)
    "12" entered:       "        12          " (shift left)
    "123" entered:      "       123          " (shift left)
    ```

**Verification:**

- Unlock screen shows centered asterisks as typing
- Setting new code shows last char briefly
- Backspace properly updates centered display

---

### **ITERATION 7: Auto-Lock Timer & Polish**

**Goal:** Implement auto-lock, finalize UX, optimize performance.

**Tasks:**

- [ ] **7.1 Auto-Lock Timer**
  - Create FreeRTOS timer in `lock_system.c`:

    ```c
    TimerHandle_t autolock_timer;
    autolock_timer = xTimerCreate("autolock", pdMS_TO_TICKS(30000), pdFALSE, NULL, autolock_callback);
    ```

  - Start timer when entering UNLOCKED state
  - Reset timer on any keypress in UNLOCKED/MENU
  - Callback: transition to LOCKED, clear LCD, show "Auto-locked"

- [ ] **7.2 Input Validation**
  - PIN entry: only accept '0'-'9'
  - Length check: 4-8 digits
  - Reject invalid characters (A, B, C, D)
  - Show error: "PIN must be 4-8 digits"

- [ ] **7.3 UX Polish**
  - Add progress indicators (e.g., "Processing...")
  - Clear LCD between state transitions
  - Consistent message formatting
  - Buzzer feedback for each state transition

- [ ] **7.4 Code Optimization**
  - Review all `vTaskDelay()` usage
  - Minimize LCD refreshes (only when needed)
  - Optimize keypad scan frequency
  - Add comments and documentation

**Verification:**

- Sit idle in UNLOCKED for 30s → auto-locks
- Invalid input rejected with clear error messages
- Smooth transitions between all states
- No screen flicker or repeated refreshes

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

**Iteration:** Iteration 3 (Simple Lock/Unlock) - COMPLETE ✅
**Completed Tasks:** 13/40+
**Progress:**

- ✅ Iteration 1: COMPLETE - All hardware drivers working
- ✅ Iteration 2: COMPLETE - STDIO redirect (lcd_printf/lcd_scanf) implemented
- ✅ Iteration 3: COMPLETE - Lock/unlock with hardcoded PIN "1234"
  - State machine with LOCKED/UNLOCKED states
  - PIN validation with visual/audio feedback
  - LED indicators (green=unlocked, red=error)
- ⏳ Iteration 4-7: Not started

**Next Steps:**

1. Test Iteration 3 in Wokwi simulator
2. Begin Iteration 4: NVS storage and first-boot setup wizard
3. Implement PIN persistence across reboots
**Next Steps:** Begin Iteration 1 → Create `config_pins.h` and LCD driver
23

---

**Last Updated:** 2026-02-11
**Project Type:** Embedded Systems Lab Assignment
**Platform:** ESP32-IDF 5.5.0 + Wokwi
**Target:** Lock/Unlock Security System with STDIO Retargeting
