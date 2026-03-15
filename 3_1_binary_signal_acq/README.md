# 3_1_binary_signal_acq

ESP32 signal acquisition and conditioning project built with ESP-IDF, FreeRTOS, PlatformIO, and Wokwi.

## Scope

- Periodic temperature/humidity acquisition
- Motion detection with LED indication
- Button-triggered system reset
- Threshold + hysteresis alert logic
  - Temperature alert ON requires 2 consecutive valid high samples
- OLED status output over I2C

## Hardware

- ESP32 DevKit V1
- DHT11 target on real board
- DHT22 used in Wokwi simulation (component availability)
- HC-SR501 PIR sensor
- Push button
- LED + 220 ohm resistor
- SSD1306 OLED (`0x3C`)

## Pin Map

- `GPIO4`  -> DHT data
- `GPIO18` -> PIR OUT
- `GPIO19` -> Button input (active-low, internal pull-up)
- `GPIO23` -> Motion LED output
- `GPIO21` -> I2C SDA (OLED)
- `GPIO22` -> I2C SCL (OLED)

## Architecture

```txt
TaskDHT (1.5s) -> updates temperature/humidity
TaskMotion (50ms) -> filtered motion + LED + event counter
TaskButton (20ms) -> debounced reset input
TaskProcessing (100ms) -> thresholds/hysteresis -> status
TaskDisplay (300ms) -> OLED + serial snapshot
```

Shared state is protected with a mutex and exchanged through `system_state` APIs.

## Task Priority Rationale

- Priority 4: motion, button (fast reaction path)
- Priority 3: dht, processing
- Priority 2: display

## Build and Run

```bash
pio run
pio run -t upload
pio device monitor
```

## Wokwi

- `diagram.json` and `wokwi.toml` are configured for this project
- DHT defaults in simulation: `24 C`, `52%`

## Real ESP32 Pre-Flash Checklist

1. Confirm board flash size matches configuration (`2MB` vs `4MB`).
2. Replace Wokwi DHT22 with real DHT11 hardware on `GPIO4`.
3. Verify OLED address is `0x3C` (common alternative is `0x3D`).
4. Verify shared GND across ESP32, PIR, DHT, OLED, and button.
5. Run monitor and validate:
   - DHT values update every ~1.5s
   - PIR toggles LED within 100ms
   - Single button press increments reset counter once
   - OLED shows `TEMP`, `HUM`, `MOTION`, `SYS`

## Known Limitations

- DHT read uses timing-sensitive bit-banging and can be affected by electrical noise.
- OLED font is a compact built-in bitmap font, optimized for status text.
