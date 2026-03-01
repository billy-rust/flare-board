# Flare Board — Serial Protocol Reference

Communication between the quadcopter (host) and the flare board (ESP32) over USB serial at **115200 baud, 8N1**. All messages are newline-terminated (`\n`). Max command length is 32 characters.

---

## Host → Board (Commands)

| Command       | Description                          |
|---------------|--------------------------------------|
| `PING`        | Connection check                     |
| `ARM`         | Arm the system (enables firing)      |
| `DISARM`      | Disarm and kill all active relays    |
| `FIRE <1-4>`  | Fire channel N (1-indexed)           |
| `STATUS`      | Request full system status           |
| `HEARTBEAT`   | Reset the heartbeat watchdog timer   |
| `RESET`       | Clear fired flags (must be disarmed) |

---

## Board → Host (Responses)

### Direct replies (one per command)

| Response                        | Meaning                                   |
|---------------------------------|-------------------------------------------|
| `OK PONG`                       | Reply to `PING`                           |
| `OK ARM`                        | System armed                              |
| `OK ARM ALREADY_ARMED`          | Was already armed                         |
| `OK DISARM`                     | System disarmed, all relays off           |
| `OK FIRE <N>`                   | Channel N firing (relay energized)        |
| `OK HEARTBEAT`                  | Watchdog timer reset                      |
| `OK RESET`                      | Fired flags cleared                       |
| `ERR FIRE NOT_ARMED`            | Must arm before firing                    |
| `ERR FIRE INVALID_CHANNEL`      | Channel number out of range               |
| `ERR FIRE ALREADY_FIRED`        | Channel was already used                  |
| `ERR FIRE COOLDOWN`             | Less than 1000ms since last fire          |
| `ERR FIRE BUSY`                 | Another channel is still firing           |
| `ERR RESET NOT_DISARMED`        | Must disarm before resetting              |
| `ERR UNKNOWN_CMD`               | Unrecognized command                      |
| `ERR CMD_TOO_LONG`              | Command exceeded 32 characters            |
| `STATUS <state> CH1:x CH2:x...` | Full status (see below)                  |

### Async events (board sends unprompted)

| Event                     | Meaning                                      |
|---------------------------|----------------------------------------------|
| `!BOOT READY`             | Board just powered up                        |
| `!HEARTBEAT ARMED`        | Periodic status (every 1s while armed)       |
| `!HEARTBEAT DISARMED`     | Periodic status (every 1s while disarmed)    |
| `!FIRE_COMPLETE <N>`      | Channel N relay released (pulse finished)    |
| `!TIMEOUT DISARMED`       | Heartbeat watchdog expired — auto-disarmed   |

---

## STATUS Response Format

```
STATUS ARMED CH1:READY CH2:FIRING CH3:FIRED CH4:READY
```

- State: `ARMED` or `DISARMED`
- Per-channel: `READY` (unfired), `FIRING` (relay active), `FIRED` (used, relay off)

---

## Timing Constants

| Parameter              | Value   | Description                                  |
|------------------------|---------|----------------------------------------------|
| Fire pulse duration    | 500ms   | How long the relay stays energized           |
| Cooldown between fires | 1000ms  | Minimum gap between any two fire commands    |
| Heartbeat timeout      | 5000ms  | Auto-disarms if no `HEARTBEAT` received      |
| Heartbeat broadcast    | 1000ms  | Board sends `!HEARTBEAT` at this interval    |

---

## Typical Firing Sequence

```
Host                          Board
 │                              │
 │  PING\n                      │
 │ ──────────────────────────►  │
 │           OK PONG\n          │
 │  ◄────────────────────────── │
 │                              │
 │  ARM\n                       │
 │ ──────────────────────────►  │
 │            OK ARM\n          │
 │  ◄────────────────────────── │
 │                              │
 │  HEARTBEAT\n    (send every ~2s while armed)
 │ ──────────────────────────►  │
 │       OK HEARTBEAT\n         │
 │  ◄────────────────────────── │
 │                              │
 │  FIRE 1\n                    │
 │ ──────────────────────────►  │
 │        OK FIRE 1\n           │
 │  ◄────────────────────────── │
 │                              │
 │    (500ms later)             │
 │     !FIRE_COMPLETE 1\n       │
 │  ◄────────────────────────── │
 │                              │
 │  (wait ≥1000ms cooldown)     │
 │                              │
 │  FIRE 2\n                    │
 │ ──────────────────────────►  │
 │        OK FIRE 2\n           │
 │  ◄────────────────────────── │
 │                              │
 │  DISARM\n                    │
 │ ──────────────────────────►  │
 │        OK DISARM\n           │
 │  ◄────────────────────────── │
```

---

## Implementation Notes for the Quadcopter Side

1. **Open the serial port** at 115200 baud. Wait for `!BOOT READY` (or send `PING` and wait for `OK PONG`) to confirm the board is alive.

2. **Heartbeat is mandatory while armed.** Send `HEARTBEAT` every 2–3 seconds. If the board doesn't receive one within 5 seconds, it auto-disarms and kills all relays. This is a safety feature — if the host crashes, the board shuts down.

3. **Parse async events.** Lines starting with `!` are unsolicited. Your serial reader should handle them alongside command responses. Key ones:
   - `!FIRE_COMPLETE N` — confirms the relay pulse finished, safe to fire next channel (after cooldown).
   - `!TIMEOUT DISARMED` — your heartbeats stopped arriving; the board disarmed itself.

4. **Respect cooldown.** After `OK FIRE`, wait at least 1000ms before the next `FIRE` command, or you'll get `ERR FIRE COOLDOWN`.

5. **Channels are one-shot.** Once a channel fires, it cannot fire again until the system is disarmed and `RESET` is sent. This prevents accidental double-fires.

6. **Only one channel fires at a time.** If a relay is still energized (within the 500ms pulse), a second `FIRE` returns `ERR FIRE BUSY`.

7. **Message framing.** Every command and response is a single line terminated by `\n`. Read serial data line-by-line. Commands are plain ASCII, no binary framing.

---

## Build Environments

| Environment  | Command                              | Description                        |
|--------------|--------------------------------------|------------------------------------|
| `esp32dev`   | `pio run -e esp32dev -t upload`      | Production firmware (full protocol)|
| `test_fire`  | `pio run -e test_fire -t upload`     | Manual fire test (keyboard input)  |
| `timer`      | `pio run -e timer -t upload`         | Timed auto-fire (2-channel, no serial) |
| `test_cycle` | `pio run -e test_cycle -t upload`    | GPIO cycle test (2-channel)        |

### Manual Fire Test

A stripped-down build for bench-testing the ignition hardware. No arming, heartbeat, or protocol overhead — just direct keyboard-to-relay control.

Flash and open the monitor:

```bash
pio run -e test_fire -t upload && pio device monitor
```

Controls:

| Key   | Action                                      |
|-------|---------------------------------------------|
| `1-4` | Fire that channel (500ms relay pulse)       |
| `s`   | Show status of all channels                 |
| `r`   | Reset all channels (allows re-firing)       |

Channels are one-shot per cycle — once fired, press `r` to reset before firing again. Only one channel can fire at a time (wait for the 500ms pulse to finish).

### Timer Auto-Fire

Standalone 2-channel build (GPIO 16, 18) with no serial I/O. Fires on a fixed schedule from boot:

- **8 minutes** → GPIO 16 (500ms pulse)
- **13 minutes** → GPIO 18 (500ms pulse)

Edit `FIRE_DELAY_MS` in `src/timer.cpp` to change the delays.

### GPIO Cycle Test

2-channel build (GPIO 16, 18) that cycles each pin HIGH for 2 seconds in a loop. Starts automatically on boot. Press `x` to stop, `g` to restart.
