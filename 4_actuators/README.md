# Actuator Control and Power Conversion

**Platform:** ESP32 DevKit V1 · ESP-IDF (PlatformIO) · FreeRTOS · Wokwi

This project implements a complete actuator-control pipeline with a binary relay output, signed PWM motor control, command conditioning, LCD status output, and serial diagnostics.

## Features

- Relay/lamp ON-OFF control with debounce and persistent-state validation
- Signed DC motor command path using ESP32 LEDC PWM and L298-style direction signals
- Serial STDIO command interface: `BIN ON`, `BIN OFF`, `MOTOR -100..100`, `STOP`, `STATUS`, `ALERT`
- Analog command conditioning: signed saturation, median filtering, weighted moving average, and ramping
- Soft-start and soft-stop behavior for PWM output changes
- Mutex-protected shared runtime state between FreeRTOS tasks
- LCD1602 I2C display with main status page and temporary alert page
- Structured serial reporting for validation and debugging

## Hardware

| Component | Role |
| --------- | ---- |
| ESP32 DevKit V1 | Main controller |
| Wokwi relay module | Binary actuator interface |
| Yellow LED + 220 ohm resistor | Relay-switched lamp/load indicator |
| Green LED + 220 ohm resistor | Motor PWM / L298 ENA visualization |
| Blue LED | Motor IN1 forward direction visualization |
| Red LED | Motor IN2 reverse direction visualization |
| LCD1602 with PCF8574 I2C backpack | Local status display |
| Serial monitor | Command input and detailed reporting |

## Pin Map

| ESP32 pin | Connection | Purpose |
| --------- | ---------- | ------- |
| `GPIO23` | Relay module `IN` | Binary relay command |
| `GPIO25` | PWM indicator / L298 `ENA` | Motor speed PWM |
| `GPIO26` | Blue LED / L298 `IN1` | Forward direction |
| `GPIO27` | Red LED / L298 `IN2` | Reverse direction |
| `GPIO21` | LCD `SDA` | I2C data |
| `GPIO22` | LCD `SCL` | I2C clock |
| `VIN` | LCD `VCC`, relay `VCC`, relay `COM` | 5 V style simulation supply |
| `GND` | LCD, relay, LEDs | Common ground |

Relay load wiring in the Wokwi diagram:

```txt
VIN -> relay COM
relay NO -> 220 ohm resistor -> yellow LED -> GND
```

## Runtime Architecture

FreeRTOS tasks:

- `TaskCommand` reads STDIO commands and updates requested actuator targets.
- `TaskBinary` runs every 50 ms and debounces ON/OFF commands.
- `TaskAnalog` runs every 50 ms and clamps, filters, and ramps the signed motor command.
- `TaskOutput` runs every 50 ms and writes conditioned state to GPIO/PWM outputs.
- `TaskDisplay` runs every 500 ms and updates the LCD plus serial diagnostic logs.

Shared state is centralized in `system_state` and protected by a mutex.

## Commands

```txt
BIN ON      # aliases: ON, BINON, RELAYON
BIN OFF     # aliases: OFF, BINOFF, RELAYOFF
MOTOR 75    # aliases: MOTOR75, M75; forward at 75%
MOTOR -75   # aliases: MOTOR-75, M-75; reverse at 75%
MOTOR 150   # clamps to 100% and sets the limit flag
MOTOR -150  # clamps to -100% and sets the limit flag
STOP        # ramps motor command back to 0%
STATUS      # requests a detailed serial state report
ALERT       # temporarily shows LCD alert flags
```

Motor values are signed percentages:

- Positive values drive `IN1` and use PWM duty from the absolute value.
- Negative values drive `IN2` and use PWM duty from the absolute value.
- Values outside `-100..100` are saturated and reported through the limit flag.
- `STOP` requests a soft stop, so the motor output ramps down instead of changing abruptly.

## LCD Legend

Main page:

```txt
BIN:ON  M:  75%
T:  75 D: 767 OK
```

- `BIN` is the applied binary relay state.
- `M` is the signed ramped motor command.
- `T` is the conditioned signed motor target.
- `D` is the 10-bit PWM duty.
- `OK`, `WARN`, or `ALRT` is the derived status.

Alert page, shown after `ALERT`:

```txt
Bpend:0 Limit:1
Ramp:1 Err:0 Ov0
```

- `Bpend` means a binary command is still inside debounce.
- `Limit` means the motor command was saturated.
- `Ramp` means the motor output is still moving toward target.
- `Err` means an invalid command was received recently.
- `Ov` means the simulated overload zone is active.

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

Default build flags in `platformio.ini` target Wokwi and active-high relay/LED inputs:

```ini
-D BINARY_ACTUATOR_ACTIVE_LOW=0
-D MOTOR_DRIVER_USES_DIRECTION_PINS=1
-D MOTOR_DIRECTION_ACTIVE_LOW=0
```

For a real active-low relay module, change `BINARY_ACTUATOR_ACTIVE_LOW` to `1`, then rebuild and upload.

## Simulation and Hardware Notes

The Wokwi simulation uses a relay module for the binary actuator and LEDs as safe visual indicators for motor-driver signals.

- Yellow LED means the relay contact is closed and the lamp/load path is powered.
- Green LED means PWM demand is present.
- Blue LED means forward direction (`IN1`) is active.
- Red LED means reverse direction (`IN2`) is active.

For real hardware:

- Connect `GPIO23` to relay module `IN`.
- Connect `GPIO25` to motor driver `ENA` or PWM input.
- Connect `GPIO26` to motor driver `IN1`.
- Connect `GPIO27` to motor driver `IN2`.
- Remove the L298 `ENA` jumper if PWM speed control is required.
- Share ESP32 `GND` with the relay module, motor driver, and external supply.
- Power the motor from an external supply, not from the ESP32 3.3 V pin.
- Use only low-voltage loads for testing; do not switch mains voltage in this setup.

## Quick Validation

1. Send `BIN ON` and confirm the relay load turns on after debounce.
2. Send `BIN OFF` and confirm the relay load turns off predictably.
3. Send `MOTOR 75` and confirm PWM output ramps forward with `IN1` active.
4. Send `MOTOR -75` and confirm PWM output ramps reverse with `IN2` active.
5. Send `MOTOR 150` and `MOTOR -150` and confirm saturation plus the limit flag.
6. Send `STOP` and confirm the motor output ramps back to zero.
7. Compare the LCD main/alert pages with the serial diagnostic report.

## Project Layout

```txt
4_actuators/
├── include/
├── lib/
│   ├── actuators/
│   ├── display/
│   ├── filters/
│   └── system_state/
├── src/
│   ├── tasks/
│   └── main.c
├── docs/
├── diagram.json
├── platformio.ini
└── README.md
```
