# Signal Acquisition and Conditioning

**Platform:** ESP32 DevKit V1 · ESP-IDF (PlatformIO) · FreeRTOS · Wokwi

This project implements a complete embedded acquisition pipeline with both binary and analog signals, real-time conditioning, alert logic, and live status output.

## Features

- Temperature/humidity acquisition from DHT sensor
- Motion detection from PIR sensor with dedicated LED indicator
- Analog light acquisition from LDR (ADC1) with dedicated LED indicator
- Light conditioning pipeline: clamp -> median(3) -> moving average(3) -> weighted moving average(4)
- Threshold + hysteresis logic for temperature, humidity, and light
- Debounced button with hard runtime reset behavior
- OLED status rendering (SSD1306 over I2C)
- Serial diagnostic output for runtime monitoring

## Hardware

| Component | Role |
| --------- | ---- |
| ESP32 DevKit V1 | Main controller |
| DHT11 (real hardware) / DHT22 (Wokwi) | Temperature + humidity |
| HC-SR501 | Motion input |
| LDR module (AO) | Analog light input |
| Push button | User reset input |
| LED + 220 ohm | Motion indicator |
| LED + 220 ohm | Light indicator |
| SSD1306 OLED (`0x3C`) | Local display |

## Pin Map

- `GPIO4` -> DHT data
- `GPIO18` -> PIR OUT
- `GPIO19` -> Reset button (active-low, internal pull-up)
- `GPIO23` -> Motion LED
- `GPIO25` -> Light LED
- `GPIO34` -> LDR analog output (`ADC1_CH6`)
- `GPIO21` -> I2C SDA (OLED)
- `GPIO22` -> I2C SCL (OLED)

## Runtime Architecture

FreeRTOS tasks:

- `TaskDHT` (1500 ms) - reads DHT and updates environmental values
- `TaskMotion` (50 ms) - motion stability logic + motion LED
- `TaskLightSensor` (50 ms) - ADC read, filtering, light threshold/hysteresis + light LED
- `TaskButton` (20 ms) - debounced reset handling
- `TaskProcessing` (100 ms) - system-level alert/status decisions
- `TaskDisplay` (300 ms) - OLED and serial status output

Shared state is centralized in `system_state` and protected by a mutex.

## Signal Behavior

- **Temperature alert:** hysteresis with ON/OFF thresholds and confirmation samples
- **Humidity alert:** hysteresis ON/OFF thresholds
- **Light alert:** uses filtered ADC value + hysteresis
- **ADC direction:** configured as inverted (`LIGHT_ADC_INVERTED=1`) for setups where brighter light produces lower ADC value
- **Reset behavior:** hard reset clears runtime state, counters, alerts, and filtered analog history, then reacquires sensor values

## Build and Run

```bash
pio run
pio run -t upload
pio device monitor
```

## Wokwi Notes

- Simulation assets: `diagram.json`, `wokwi.toml`
- DHT is simulated with DHT22 due to part availability
- Light percentage shown in logs/display is a mapped ADC brightness estimate (`0-100%`), not lux

## Quick Validation

1. Change PIR state -> motion LED and status update quickly.
2. Change light level -> raw/filtered light values update and light LED toggles by threshold.
3. Press reset button -> counters/alerts clear and values reacquire.
4. Confirm OLED and serial logs remain consistent.

## Project Layout

```txt
3_signal_acq/
├── include/
├── lib/
│   ├── button/
│   ├── dht11/
│   ├── display/
│   ├── filters/
│   ├── ldr/
│   ├── motion/
│   └── system_state/
├── src/
│   ├── tasks/
│   └── main.c
├── diagram.json
├── platformio.ini
└── README.md
```
