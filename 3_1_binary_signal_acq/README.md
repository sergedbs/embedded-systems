# 3_1_binary_signal_acq

ESP32 signal acquisition project using ESP-IDF, FreeRTOS, PlatformIO, and Wokwi.

Current status: hardware smoke test is implemented in `src/main.c`.

## Hardware

- ESP32 DevKit V1
- DHT sensor in simulation (`dht22` in Wokwi), target hardware: DHT11
- PIR motion sensor
- Push button (reset input)
- Motion LED
- SSD1306 OLED (I2C, `0x3C`)

## Pin Map

- `GPIO4`  -> DHT data
- `GPIO18` -> PIR OUT
- `GPIO19` -> Button input (active-low, internal pull-up)
- `GPIO23` -> Motion LED (through `220Ω`)
- `GPIO21` -> I2C SDA (OLED)
- `GPIO22` -> I2C SCL (OLED)

## Build and Run

```bash
pio run
pio run -t upload
pio device monitor
```

## Simulation

- Wokwi files:
  - `diagram.json`
  - `wokwi.toml`
- Open the project in Wokwi and run the firmware.

## Architecture Direction

- Modular code layout in `include/` and `lib/`
- FreeRTOS runtime split in `src/tasks/`
- `src/main.c` will stay minimal in the final integration phase
