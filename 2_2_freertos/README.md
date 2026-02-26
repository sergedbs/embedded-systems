# Sequential Tasks (FreeRTOS)

ESP-IDF FreeRTOS demo for ESP32 that runs three preemptive tasks on a 10 ms sampling cadence: button press measurement, stats/blink feedback, and periodic reporting. Uses the same GPIO/button/LED/stats libs as the earlier bare-metal lab; only the scheduling/sync layer is RTOS-based.

## Overview

Implements Lab 2_2 “Preemptive (FreeRTOS)” with three tasks, a binary semaphore, and a mutex for shared data.

Behaviors:

- Detect button presses with debounce; classify short (<500 ms) vs long (≥500 ms).
- Short → green LED flash + yellow blink 5 times; long → red LED flash + yellow blink 10 times.
- Maintain press statistics; every 10 s print totals/averages and reset stats.

## Hardware (Wokwi / ESP32 DevKit v1)

- BTN (active‑low, pull-up): GPIO26  
- LED_GREEN: GPIO19  
- LED_RED: GPIO18  
- LED_YELLOW: GPIO21  
- Resistors: 220 Ω in series with each LED

## Connections

```txt
GPIO26 --- pushbutton --- GND   (internal pull-up enabled)
GPIO19 --- 220Ω --- GREEN LED --- GND
GPIO18 --- 220Ω --- RED LED  --- GND
GPIO21 --- 220Ω --- YELLOW LED --- GND
```

Pins are configurable in `include/app_config.h`.

## Software Requirements

- PlatformIO with ESP-IDF framework (5.x) or ESP-IDF directly
- Serial monitor at 115200 baud

## Building and Flashing

PlatformIO:

```bash
pio run
pio run --target upload
pio device monitor
```

ESP-IDF:

```bash
idf.py build
idf.py flash monitor
```

## Usage

- Press and release the button:
  - Short press (<500 ms): green flashes ~300 ms, yellow blinks 5 times.
  - Long press (≥500 ms): red flashes ~300 ms, yellow blinks 10 times.
- Every 10 s the console prints totals and average press duration, then resets stats.

## Project Structure

```txt
2_2_freertos/
├── include/                 # shared headers (config, types, stats)
├── lib/
│   ├── button/              # debounced edge detector
│   ├── leds/                # GPIO LED helpers
│   ├── scheduler/           # left for reference, not linked here
│   └── stats/               # stats helpers
├── src/
│   ├── tasks/               # task_measure, task_stats, task_report (FreeRTOS)
│   └── main.c               # app_main creates tasks, sem, mutex
├── platformio.ini
└── README.md
```

## Architecture

**Task 1: Measure** (`task_measure.c`)

- Polls button every 10 ms using `vTaskDelayUntil`.
- Debounce (≈40 ms), capture press duration via `esp_timer_get_time()`.
- Classify short/long, flash green/red briefly.
- Write latest press info under mutex; give binary semaphore to Task 2.

**Task 2: Stats + Blink** (`task_stats.c`)

- Waits on semaphore, copies shared press under mutex, updates stats under same mutex.
- Runs yellow blink state machine (non-busy delays), 5× for short or 10× for long.

**Task 3: Report** (`task_report.c`)

- Every 10 s (200 ms start offset) copies + resets stats under mutex, logs totals/average.

Synchronization:

- Binary semaphore: Task 1 → Task 2 “new press measured”.
- Mutex: protects shared press struct and shared stats.

Timing constants and pins live in `include/app_config.h`.

## Key Implementation Details

- Preemptive FreeRTOS tasks; timing driven by `vTaskDelayUntil` and `vTaskDelay`.
- Debounce and blink use timed delays (no busy waits, no ISR required).
- Shared libs are unchanged from the bare-metal lab; only the RTOS task layer differs.
- Cooperative scheduler code remains in `lib/scheduler/` but is not compiled for this target.

## Serial Output Example

```txt
I (10000) report: 10s window: total=3 short=2 long=1 avg_ms=420
```
