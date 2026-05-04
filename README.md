# EMBEDDED SYSTEMS PROJECTS

This repository contains multiple embedded systems projects built on ESP32. Each project explores a different embedded concept, from basic serial I/O and peripheral control to task scheduling, signal acquisition, actuator control, and automatic control.

## Projects Overview

### [1.1 Serial STDIO](1_1_serial_stdio)

A simple ESP32 project that controls an LED through serial commands using standard input and output.

### [1.2 LCD Keypad Lock System](1_2_lcd_keypad)

A PIN-based security lock system using a keypad, LCD display, persistent storage, and simulated hardware in Wokwi.

### [2.1 Sequential Tasks](2_1_sequential_tasks)

A cooperative scheduling project that runs multiple logical tasks without FreeRTOS using a simple tick-based design.

### [2.2 FreeRTOS Tasks](2_2_freertos)

A preemptive multitasking version of the task scheduling project implemented with FreeRTOS synchronization primitives.

### [3. Signal Acquisition and Conditioning](3_signal_acq)

A sensor-based ESP32 application that reads, filters, processes, and displays real-time environmental and motion data.

### [4. Actuator Control and Power Conversion](4_actuators)

An actuator-control project with a relay output, signed PWM motor control, command conditioning, LCD status output, and serial diagnostics.

### [5. Automatic Temperature Control](5_auto_control)

A closed-loop temperature-control project using DHT22 acquisition, ON-OFF relay control with hysteresis, PID PWM control, LCD status output, and serial commands.

## Repository Structure

```text
embedded-systems
├── 1_1_serial_stdio
├── 1_2_lcd_keypad
├── 2_1_sequential_tasks
├── 2_2_freertos
├── 3_signal_acq
├── 4_actuators
├── 5_auto_control
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
