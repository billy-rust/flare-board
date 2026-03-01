#include <Arduino.h>

// --- Pin Configuration ---
#define NUM_CHANNELS 2
static const uint8_t CHANNEL_PINS[NUM_CHANNELS] = {16, 18};

// --- Timer Configuration ---
// Delay from boot before each channel fires
static const unsigned long FIRE_DELAY_MS[NUM_CHANNELS] = {
  8UL * 60 * 1000,   // CH1 (GPIO 16): 8 minutes
  13UL * 60 * 1000,  // CH2 (GPIO 18): 13 minutes (8 + 5)
};

// How long the relay stays energized (ms)
#define FIRE_PULSE_MS 500

// --- State ---
static bool channelFired[NUM_CHANNELS];
static bool channelFiring[NUM_CHANNELS];
static unsigned long fireStartMs[NUM_CHANNELS];

void setup() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    digitalWrite(CHANNEL_PINS[i], LOW);
    pinMode(CHANNEL_PINS[i], OUTPUT);
    digitalWrite(CHANNEL_PINS[i], LOW);
    channelFired[i] = false;
    channelFiring[i] = false;
    fireStartMs[i] = 0;
  }
}

void loop() {
  unsigned long now = millis();

  // Auto-release relays after pulse duration
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channelFiring[i] && (now - fireStartMs[i]) >= FIRE_PULSE_MS) {
      digitalWrite(CHANNEL_PINS[i], LOW);
      channelFiring[i] = false;
    }
  }

  // Fire channels when their timer elapses
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (!channelFired[i] && !channelFiring[i] && now >= FIRE_DELAY_MS[i]) {
      // Wait for any active channel to finish first
      bool busy = false;
      for (int j = 0; j < NUM_CHANNELS; j++) {
        if (channelFiring[j]) { busy = true; break; }
      }
      if (busy) continue;

      digitalWrite(CHANNEL_PINS[i], HIGH);
      channelFiring[i] = true;
      channelFired[i] = true;
      fireStartMs[i] = now;
    }
  }
}
