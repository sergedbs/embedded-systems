# Automatic Temperature Control

**Platform:** ESP32 DevKit V1 · ESP-IDF (PlatformIO) · FreeRTOS · Wokwi

This project implements a temperature-control system with a DHT22 sensor, ON-OFF relay control with hysteresis, PID-based PWM control, LCD status output, serial commands, and optional Serial Plotter telemetry.

## Features

- DHT22 temperature and humidity acquisition
- ON-OFF heater control with relay output and hysteresis
- PID heater control with ESP32 LEDC PWM output
- Runtime command interface over serial STDIO
- Configurable set point, hysteresis, PID gains, and telemetry mode
- LCD1602 I2C display with mode, measured value, set point, output, relay state, and status
- Serial status reports and optional plotter lines
- Mutex-protected shared state between FreeRTOS tasks
- Wokwi simulation with relay module and safe LED heater indicators

## Hardware

| Component | Role |
| --------- | ---- |
| ESP32 DevKit V1 | Main controller |
| DHT22 | Temperature and humidity sensor |
| Wokwi relay module | ON-OFF heater actuator interface |
| Yellow LED + 220 ohm resistor | Relay-switched heater/load indicator |
| Red LED + 220 ohm resistor | PID PWM heater indicator |
| LCD1602 with PCF8574 I2C backpack | Local status display |
| Serial monitor | Command input, diagnostics, and plotter output |

## Pin Map

| ESP32 pin | Connection | Purpose |
| --------- | ---------- | ------- |
| `GPIO18` | DHT22 `SDA` / data | Temperature and humidity input |
| `GPIO23` | Relay module `IN` | ON-OFF heater command |
| `GPIO25` | PWM heater LED / MOSFET driver input | PID PWM heater output |
| `GPIO21` | LCD `SDA` | I2C data |
| `GPIO22` | LCD `SCL` | I2C clock |
| `3V3` | DHT22 `VCC` | Sensor power |
| `VIN` | LCD `VCC`, relay `VCC`, relay `COM` | 5 V style simulation supply |
| `GND` | DHT22, LCD, relay, LEDs | Common ground |

Relay load wiring in the Wokwi diagram:

```txt
VIN -> relay COM
relay NO -> 220 ohm resistor -> yellow LED -> GND
```

## Runtime Architecture

FreeRTOS tasks:

- `TaskCommand` reads serial commands and updates runtime settings.
- `TaskAcquire` samples the DHT22 every 2 seconds.
- `TaskControl` computes ON-OFF or PID output every 200 ms.
- `TaskOutput` applies relay/PWM output every 100 ms.
- `TaskDisplay` updates LCD and serial diagnostics every 500 ms.
- `TaskPlotter` emits plotter-compatible telemetry when enabled.

Shared state is centralized in `system_state` and protected by a mutex.

## Commands

```txt
MODE ONOFF       # relay control with hysteresis
MODE PID         # PID control with PWM output
SP 25.5          # set temperature set point in degrees Celsius
HYST 1.0         # set ON-OFF hysteresis margin in degrees Celsius
PID 18 0.25 6    # set Kp, Ki, and Kd together
KP 20            # update only Kp
KI 0.3           # update only Ki
KD 5             # update only Kd
PLOT ON          # enable Serial Plotter telemetry
PLOT OFF         # disable Serial Plotter telemetry
STATUS           # print current state
HELP             # print command summary
```

Default startup settings:

- Mode: `MODE ONOFF`
- Set point: `25.0 °C`
- Hysteresis: `1.0 °C`
- PID gains: configured in `include/app_config.h`
- Plotter output: disabled

## LCD Legend

Main status page:

```txt
ONF T:24.1/25.0
Out:100% R:1 OK
```

- `ONF` means ON-OFF mode.
- `PID` means PID mode.
- `T` shows filtered temperature and set point.
- `Out` shows actuator output percentage.
- `R:1` means relay ON, `R:0` means relay OFF.
- `OK`, `WARN`, or `ALRT` is the derived status.

When the sensor is not ready, the LCD shows:

```txt
ONF T:--.-/25.0
DHT wait OK
```

## Control Behavior

The controlled value is temperature from the DHT22. The set point is the desired temperature. The output is the heater command.

In `MODE ONOFF`, the relay heater uses hysteresis:

- If temperature is below `setpoint - hysteresis`, the relay turns ON.
- If temperature is above `setpoint + hysteresis`, the relay turns OFF.
- If temperature is inside the band, the previous relay state is kept to avoid rapid switching.
- The PWM output is forced to `0%` while ON-OFF relay control is active.

In `MODE PID`, the relay is kept OFF and the PWM heater indicator is controlled by the PID result:

- `error = setpoint - measured_temperature`
- `Kp`, `Ki`, and `Kd` generate proportional, integral, and derivative terms.
- The final output is clamped to `0..100%` and converted to PWM duty.
- The red LED brightness represents the PID heater demand.

Serial Plotter output, when enabled, uses labelled values:

```txt
SetPoint:25.00 Value:24.40 Output:100.00
```

## Build and Run

```bash
pio run
pio run -t upload
pio device monitor
```

If `pio` is not on PATH, use the PlatformIO virtualenv binary:

```bash
/Users/sergiu/.platformio/penv/bin/pio run
```

Default build flags in `platformio.ini` target Wokwi and active-high relay/PWM indicators:

```ini
-D RELAY_HEATER_ACTIVE_LOW=0
-D PWM_HEATER_ACTIVE_LOW=0
```

For a real active-low relay module, change `RELAY_HEATER_ACTIVE_LOW` to `1`, then rebuild and upload.

## Simulation and Hardware Notes

The Wokwi simulation uses a relay module for ON-OFF control and LEDs as safe visual substitutes for heater loads.

- Yellow LED means the relay contact is closed and the ON-OFF heater load is powered.
- Red LED brightness follows the PID PWM output.
- DHT22 temperature can be changed by clicking the sensor in Wokwi.
- Plotter output is disabled by default to keep the serial terminal readable.

For real hardware:

- Connect `GPIO18` to DHT22 data and add a 4.7 kOhm to 10 kOhm pull-up if the module does not include one.
- Connect `GPIO23` to relay module `IN`.
- Connect relay `COM/NO` to the external low-voltage load circuit.
- Connect `GPIO25` to a MOSFET/transistor driver input for PWM heater control.
- Share ESP32 `GND` with the relay module, MOSFET driver, and external supply.
- Do not power a heater or relay coil directly from an ESP32 GPIO pin.
- Use only low-voltage loads for testing; do not switch mains voltage in this setup.

## Quick Validation

1. Build with `pio run`.
2. Start the Wokwi simulation and wait for the LCD to leave `DHT wait`.
3. Send `MODE ONOFF`, set `SP 25`, and change DHT22 temperature below and above the hysteresis band.
4. Confirm the relay load turns ON below `SP - HYST` and OFF above `SP + HYST`.
5. Send `MODE PID` and confirm the relay turns OFF while the red PWM LED follows PID output.
6. Send `PID 18 0.25 6`, `KP`, `KI`, or `KD` commands and confirm `STATUS` reports updated gains.
7. Send `PLOT ON` to verify plotter lines, then `PLOT OFF` to stop continuous telemetry.

## Project Layout

```txt
5_auto_control/
├── include/
├── lib/
│   ├── dht/
│   ├── display/
│   ├── heater/
│   └── system_state/
├── src/
│   ├── tasks/
│   └── main.c
├── docs/
├── diagram.json
├── platformio.ini
└── README.md
```
