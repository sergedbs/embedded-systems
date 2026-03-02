# Sequential Tasks (Non-Preemptive Scheduler)

ESP-IDF bare‑metal demo for ESP32 that runs three logical tasks cooperatively on a 10 ms tick: button press measurement, stats/blink feedback, and periodic reporting.

## Overview

Implements “Non-Preemptive (bare-metal)” with a simple tick-based scheduler and task table `{recurrence, offset, function pointer, ctx}`. Only one task runs per tick; timing is driven by `esp_timer_get_time()`.

Behaviors:

- Detect button presses with debounce; classify short (<500 ms) vs long (≥500 ms).
- Short → green LED flash + yellow blink 5 times; long → red flash + yellow blink 10 times.
- Maintain press statistics; every 10 s print totals/averages and reset stats.

No FreeRTOS tasks, queues, or `vTaskDelay` are used.

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
2_1_sequential_tasks/
├── include/                 # shared headers (config, types, stats, scheduler API)
├── lib/
│   ├── button/              # debounced edge detector
│   ├── leds/                # GPIO LED helpers
│   ├── scheduler/           # cooperative scheduler
│   └── stats/               # stats helpers
├── src/
│   ├── tasks/               # task_measure, task_stats, task_report
│   └── main.c               # init, task table, tick loop
├── platformio.ini
└── README.md
```

## Architecture

**Scheduler** (`lib/scheduler/` + `src/main.c`)

- Tick counter in milliseconds (10 ms resolution).
- Task table with recurrence/offset; all due tasks run sequentially per tick.

**Task 1: Measure** (`task_measure.c`)

- Debounce button (≈40 ms), capture press duration via `esp_timer_get_time()`.
- Classify short/long, queue an event, flash green/red briefly.

**Task 2: Stats + Blink** (`task_stats.c`)

- Consume events, update counters, start non-blocking yellow blink state machine (on/off toggles every 60 ms).

**Task 3: Report** (`task_report.c`)

- Every 10 s (offset to avoid collisions) log totals and average, then reset stats.

Timing constants and pins live in `include/app_config.h`.

## Key Implementation Details

- No RTOS tasks: everything runs inside a tight loop, one task per tick.
- Debounce and blink are time-driven (no busy waits).
- Small ring buffer (size 8) passes press events from Task 1 to Task 2.
- Watchdog: main task registers with `esp_task_wdt` and resets after each tick.
- Yellow blink ends with LED off; green/red flashes auto-timeout.

## Serial Output Example

```txt
I (10000) report: 10s window: total=3 short=2 long=1 avg_ms=420
```
