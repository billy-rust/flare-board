#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <Arduino.h>
#include "config.h"
#include "state.h"

static unsigned long lastHeartbeatSendMs = 0;

void updateHeartbeat() {
  unsigned long now = millis();

  // Periodic broadcast
  if ((now - lastHeartbeatSendMs) >= HEARTBEAT_SEND_INTERVAL_MS) {
    lastHeartbeatSendMs = now;
    Serial.print("!HEARTBEAT ");
    Serial.println(systemState == STATE_ARMED ? "ARMED" : "DISARMED");
  }

  // Timeout check
  if (checkHeartbeatTimeout()) {
    Serial.println("!TIMEOUT DISARMED");
  }
}

#endif
