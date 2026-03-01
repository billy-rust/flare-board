#include <Arduino.h>

// --- Pin Configuration ---
#define NUM_CHANNELS 2
static const uint8_t CHANNEL_PINS[NUM_CHANNELS] = {16, 18};

// --- Timing ---
#define ACTIVATE_MS 2000  // How long each GPIO stays HIGH

// --- Serial ---
#define SERIAL_BAUD 115200

// --- State ---
static int currentChannel = 0;
static bool running = false;
static unsigned long activateStartMs = 0;

void initPins() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    digitalWrite(CHANNEL_PINS[i], LOW);
    pinMode(CHANNEL_PINS[i], OUTPUT);
    digitalWrite(CHANNEL_PINS[i], LOW);
  }
}

void startCycle() {
  if (running) return;
  running = true;
  currentChannel = 0;
  activateStartMs = millis();
  digitalWrite(CHANNEL_PINS[currentChannel], HIGH);
  Serial.print("  GPIO ");
  Serial.print(CHANNEL_PINS[currentChannel]);
  Serial.println(" ON");
}

void stopCycle() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    digitalWrite(CHANNEL_PINS[i], LOW);
  }
  running = false;
  Serial.println("  Stopped — all pins LOW.");
}

void setup() {
  initPins();

  Serial.begin(SERIAL_BAUD);
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 2000) {}

  Serial.println();
  Serial.println("=== FLARE BOARD - GPIO CYCLE TEST ===");
  Serial.println();
  Serial.print("  Pins: ");
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (i > 0) Serial.print(", ");
    Serial.print(CHANNEL_PINS[i]);
  }
  Serial.println();
  Serial.print("  Activate time: ");
  Serial.print(ACTIVATE_MS / 1000.0, 1);
  Serial.println("s per pin");
  Serial.println();
  Serial.println("  x    Stop");
  Serial.println();

  startCycle();
}

void loop() {
  unsigned long now = millis();

  if (running && (now - activateStartMs) >= ACTIVATE_MS) {
    // Turn off current pin
    digitalWrite(CHANNEL_PINS[currentChannel], LOW);
    Serial.print("  GPIO ");
    Serial.print(CHANNEL_PINS[currentChannel]);
    Serial.println(" OFF");

    // Advance to next pin
    currentChannel = (currentChannel + 1) % NUM_CHANNELS;
    activateStartMs = now;

    // Turn on next pin
    digitalWrite(CHANNEL_PINS[currentChannel], HIGH);
    Serial.print("  GPIO ");
    Serial.print(CHANNEL_PINS[currentChannel]);
    Serial.println(" ON");
  }

  // Read keyboard input
  if (Serial.available()) {
    char c = Serial.read();
    if (c < ' ' || c > '~') return;
    if (c == 'g' || c == 'G') {
      startCycle();
    } else if (c == 'x' || c == 'X') {
      stopCycle();
    }
  }
}
