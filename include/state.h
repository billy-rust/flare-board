#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "config.h"

enum SystemState {
  STATE_DISARMED,
  STATE_ARMED
};

static SystemState systemState = STATE_DISARMED;

// Per-channel tracking
static bool channelFired[NUM_CHANNELS];       // Permanent until RESET
static bool channelFiring[NUM_CHANNELS];      // Relay currently energized
static unsigned long fireStartMs[NUM_CHANNELS];

// Global cooldown — tracks last fire command on any channel
static unsigned long lastFireMs = 0;

// Heartbeat tracking
static unsigned long lastHeartbeatMs = 0;

void initPins() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    digitalWrite(CHANNEL_PINS[i], LOW);
    pinMode(CHANNEL_PINS[i], OUTPUT);
    digitalWrite(CHANNEL_PINS[i], LOW);
    channelFired[i] = false;
    channelFiring[i] = false;
    fireStartMs[i] = 0;
  }
}

void allRelaysOff() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    digitalWrite(CHANNEL_PINS[i], LOW);
    channelFiring[i] = false;
  }
}

// Returns NULL on success, or an error reason string on failure.
const char* doFire(int channel) {
  if (systemState != STATE_ARMED) {
    return "NOT_ARMED";
  }
  if (channel < 0 || channel >= NUM_CHANNELS) {
    return "INVALID_CHANNEL";
  }
  if (channelFired[channel]) {
    return "ALREADY_FIRED";
  }

  unsigned long now = millis();

  if (lastFireMs > 0 && (now - lastFireMs) < FIRE_COOLDOWN_MS) {
    return "COOLDOWN";
  }

  // Only one channel can fire at a time
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channelFiring[i]) {
      return "BUSY";
    }
  }

  // Fire the channel
  digitalWrite(CHANNEL_PINS[channel], HIGH);
  channelFiring[channel] = true;
  channelFired[channel] = true;
  fireStartMs[channel] = now;
  lastFireMs = now;
  return NULL;
}

// Called each loop() — auto-releases relays after FIRE_PULSE_MS.
// Returns the 0-indexed channel that just completed, or -1 if none.
int updateFirePulses() {
  unsigned long now = millis();
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channelFiring[i] && (now - fireStartMs[i]) >= FIRE_PULSE_MS) {
      digitalWrite(CHANNEL_PINS[i], LOW);
      channelFiring[i] = false;
      return i;
    }
  }
  return -1;
}

// Returns true if timeout triggered auto-disarm.
bool checkHeartbeatTimeout() {
#if HEARTBEAT_TIMEOUT_MS > 0
  if (systemState == STATE_ARMED) {
    unsigned long now = millis();
    if ((now - lastHeartbeatMs) >= HEARTBEAT_TIMEOUT_MS) {
      allRelaysOff();
      systemState = STATE_DISARMED;
      return true;
    }
  }
#endif
  return false;
}

void resetChannels() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    channelFired[i] = false;
  }
}

#endif
