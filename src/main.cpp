#include <Arduino.h>
#include "config.h"
#include "state.h"
#include "protocol.h"
#include "heartbeat.h"

void setup() {
  // Drive all GPIOs LOW immediately — before anything else
  initPins();

  Serial.begin(SERIAL_BAUD);

  // Bounded wait for USB CDC connection (max 2 seconds)
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 2000) {
    // wait
  }

  Serial.println("!BOOT READY");
}

void loop() {
  // 1. Process incoming serial commands
  readSerial();

  // 2. Manage relay pulse durations — detect fire-complete transitions
  int completed = updateFirePulses();
  if (completed >= 0) {
    Serial.print("!FIRE_COMPLETE ");
    Serial.println(completed + 1);
  }

  // 3. Periodic heartbeat broadcast + timeout check
  updateHeartbeat();
}
