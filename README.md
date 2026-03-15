# EMBEDDED SYSTEMS PROJECTS

This repository contains multiple embedded systems projects built on ESP32. Each project explores a different embedded concept, from basic serial I/O and peripheral control to task scheduling, signal acquisition, and real-time processing.

## Projects Overview

### [1. Serial STDIO](1_1_serial_stdio)

A simple ESP32 project that controls an LED through serial commands using standard input and output.

### [2. LCD Keypad Lock System](1_2_lcd_keypad)

A PIN-based security lock system using a keypad, LCD display, persistent storage, and simulated hardware in Wokwi.

### [3. Sequential Tasks](2_1_sequential_tasks)

A cooperative scheduling project that runs multiple logical tasks without FreeRTOS using a simple tick-based design.

### [4. FreeRTOS Tasks](2_2_freertos)

A preemptive multitasking version of the task scheduling project implemented with FreeRTOS synchronization primitives.

### [5. Signal Acquisition and Conditioning](3_signal_acq)

A sensor-based ESP32 application that reads, filters, processes, and displays real-time environmental and motion data.

## Repository Structure

```text
embedded-systems
├── 1_1_serial_stdio
├── 1_2_lcd_keypad
├── 2_1_sequential_tasks
├── 2_2_freertos
├── 3_signal_acq
├── LICENSE
└── README.md
```

## Technologies Used

- ESP32
- PlatformIO
- ESP-IDF
- FreeRTOS
- Wokwi

## License

The source code of this repository is licensed under the MIT License. See the [`LICENSE`](LICENSE) for more details.
