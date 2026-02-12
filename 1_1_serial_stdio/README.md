# STDIO LED Control

A simple embedded application for ESP32 that controls an LED through serial commands over UART using standard input/output (STDIO).

## Overview

This project demonstrates fundamental embedded systems concepts including serial communication, GPIO control, and command parsing. The application runs on ESP32 DevKit v1 and provides a command-line interface for controlling an LED connected to GPIO2.

## Hardware Requirements

- ESP32 DevKit v1
- LED (red, 5mm standard)
- 220Ω resistor
- Breadboard and jumper wires
- USB cable for programming and serial communication

## Connections

```txt
ESP32 GPIO2 → 220Ω Resistor → LED Anode
LED Cathode → GND
```

The ESP32 DevKit v1 typically has an on-board LED on GPIO2, so the circuit will work even without external components.

## Software Requirements

- PlatformIO (or ESP-IDF directly)
- ESP-IDF framework 5.x
- Serial terminal (built into PlatformIO, or any terminal emulator at 115200 baud)

## Building and Flashing

Using PlatformIO:

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Open serial monitor
pio device monitor
```

Using ESP-IDF:

```bash
idf.py build
idf.py flash monitor
```

## Usage

Once the firmware is running, connect to the serial port at 115200 baud. The system will display a welcome message and available commands:

```bash
STDIO LED Control
Commands:
  led on
  led off
>
```

### Supported Commands

- `led on` - Turns the LED on
- `led off` - Turns the LED off

The system provides feedback for each command:

- `[OK] LED ON` - Command executed successfully
- `[ERROR] Invalid input` - Incorrect command format
- `[ERROR] Unknown command` - Command not recognized

### Examples

```bash
> led on
[OK] LED ON
> led off
[OK] LED OFF
> led blink
[ERROR] Unknown command
```

## Project Structure

```txt
1_1_serial_stdio/
├── src/
│   └── main.c              # Main application with command parser and loop
├── lib/
│   └── Led/
│       ├── Led.h           # LED module interface
│       └── Led.c           # LED module implementation
├── platformio.ini          # PlatformIO configuration
└── README.md
```

## Architecture

The project follows a layered architecture:

**Application Layer** (`main.c`)

- Command parser that validates and interprets user input
- Command loop that continuously reads from STDIN and executes commands
- Uses standard C library functions (`printf`, `getchar`, `sscanf`)

**Hardware Abstraction Layer** (`Led.h/Led.c`)

- Simple API: `Led_Init()`, `Led_On()`, `Led_Off()`
- Encapsulates ESP-IDF GPIO operations
- Makes the code portable to other platforms

**System Layer**

- ESP-IDF GPIO driver for hardware access
- FreeRTOS for task management and delays

### Key Implementation Details

**Command Parsing**: The parser uses `sscanf()` to extract two words from input and validates them against known commands. Buffer overflow protection is implemented by limiting input size.

**Input Handling**: The command loop processes characters one by one, implementing:

- Echo for visual feedback
- Backspace support for editing
- Enter key detection for command submission
- EOF handling with `vTaskDelay()` to prevent busy-waiting

**Hardware Control**: GPIO operations are abstracted through the LED module, which stores the configured pin in a static variable and provides simple on/off control.

## Serial Configuration

- Baud rate: 115200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None
