# Flare Board — PlatformIO Project

ESP32 firmware that controls 4 relay channels over USB serial for a quadcopter-mounted flare deployment system.

## Project Structure

```
platformio.ini        # Build config: esp32dev, Arduino framework, 115200 baud
src/main.cpp          # Entry point (setup/loop)
include/
  config.h            # Pin assignments, timing constants, serial config
  state.h             # System state, relay logic, safety checks
  protocol.h          # Serial command parser and responses
  heartbeat.h         # Periodic broadcast and watchdog timeout
```

## Build & Flash

```bash
pio run                # compile
pio run -t upload      # compile + flash (builds automatically)
pio device monitor     # serial monitor at 115200
pio device list        # find connected board port
```

## Architecture

- Single-file compilation: all headers are `#include`d into `main.cpp`. State is shared via `static` globals in headers — this matches the original Arduino IDE structure.
- Arduino framework on ESP32 — uses `Serial`, `millis()`, `digitalWrite()`, `pinMode()`.
- No external libraries. No RTOS tasks. Single-threaded `loop()`.

## Serial Protocol

See `README.md` for the full protocol reference. Key points:

- 115200 baud, newline-terminated ASCII commands
- Host sends: `PING`, `ARM`, `DISARM`, `FIRE <1-4>`, `STATUS`, `HEARTBEAT`, `RESET`
- Board replies with `OK ...`, `ERR ...`, or `STATUS ...`
- Board sends async events prefixed with `!` (`!BOOT READY`, `!FIRE_COMPLETE`, `!HEARTBEAT`, `!TIMEOUT DISARMED`)

## Safety Constraints

These are intentional and should not be relaxed:

- **Heartbeat watchdog**: auto-disarms after 5s without a `HEARTBEAT` command
- **One-shot channels**: each channel fires once per arm cycle (prevents double-fire)
- **Single channel at a time**: no concurrent relay activation
- **1s cooldown** between fire commands on any channel
- **500ms relay pulse**: hardware-timed, auto-releases
- **Safe boot pins**: GPIOs 16–19 avoid ESP32 strapping pins that glitch HIGH on boot
- **Boot sequence**: pins driven LOW before `Serial.begin()`

## Conventions

- Channel numbers are **1-indexed** in the serial protocol, **0-indexed** internally
- Error strings are constants (e.g., `NOT_ARMED`, `COOLDOWN`, `BUSY`) — don't change these; the host parses them
- Async events start with `!`, command responses start with `OK` or `ERR`
