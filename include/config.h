#ifndef CONFIG_H
#define CONFIG_H

// --- Channel Configuration ---
#define NUM_CHANNELS 4

// Safe GPIO pins — avoid strapping pins (0, 2, 5, 12, 15) which can glitch HIGH during boot
static const uint8_t CHANNEL_PINS[NUM_CHANNELS] = {16, 17, 18, 19};

// --- Timing ---
#define FIRE_PULSE_MS        500    // Relay hold duration (ms)
#define FIRE_COOLDOWN_MS     1000   // Minimum time between fire commands on any channel (ms)
#define HEARTBEAT_TIMEOUT_MS 5000   // Auto-disarm if no heartbeat received (0 to disable)
#define HEARTBEAT_SEND_INTERVAL_MS 1000  // Periodic status broadcast interval (ms)

// --- Serial ---
#define SERIAL_BAUD    115200
#define MAX_CMD_LENGTH 32

#endif
