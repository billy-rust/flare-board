#include <Arduino.h>
#include "config.h"

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

  Serial.begin(SERIAL_BAUD);
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 2000) {}

  Serial.println();
  Serial.println("=== FLARE BOARD - MANUAL FIRE TEST ===");
  Serial.println();
  Serial.println("  1-4  Fire channel");
  Serial.println("  s    Show status");
  Serial.println("  r    Reset all channels");
  Serial.println();
}

void printStatus() {
  Serial.println();
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print("  CH");
    Serial.print(i + 1);
    Serial.print(" (GPIO ");
    Serial.print(CHANNEL_PINS[i]);
    Serial.print("): ");
    if (channelFiring[i]) {
      Serial.println("FIRING");
    } else if (channelFired[i]) {
      Serial.println("FIRED");
    } else {
      Serial.println("READY");
    }
  }
  Serial.println();
}

void fire(int ch) {
  if (ch < 0 || ch >= NUM_CHANNELS) return;

  if (channelFired[ch]) {
    Serial.print("  CH");
    Serial.print(ch + 1);
    Serial.println(" already fired. Press 'r' to reset.");
    return;
  }

  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channelFiring[i]) {
      Serial.println("  Busy — wait for current channel to finish.");
      return;
    }
  }

  digitalWrite(CHANNEL_PINS[ch], HIGH);
  channelFiring[ch] = true;
  channelFired[ch] = true;
  fireStartMs[ch] = millis();

  Serial.print("  CH");
  Serial.print(ch + 1);
  Serial.println(" FIRED");
}

void resetAll() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    digitalWrite(CHANNEL_PINS[i], LOW);
    channelFiring[i] = false;
    channelFired[i] = false;
    fireStartMs[i] = 0;
  }
  Serial.println("  All channels reset.");
}

void loop() {
  // Auto-release relays after pulse duration
  unsigned long now = millis();
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (channelFiring[i] && (now - fireStartMs[i]) >= FIRE_PULSE_MS) {
      digitalWrite(CHANNEL_PINS[i], LOW);
      channelFiring[i] = false;
      Serial.print("  CH");
      Serial.print(i + 1);
      Serial.println(" relay released.");
    }
  }

  // Read keyboard input — ignore non-printable bytes (serial noise)
  if (Serial.available()) {
    char c = Serial.read();
    if (c < ' ' || c > '~') return;
    if (c >= '1' && c <= '4') {
      fire(c - '1');
    } else if (c == 's' || c == 'S') {
      printStatus();
    } else if (c == 'r' || c == 'R') {
      resetAll();
    }
  }
}
