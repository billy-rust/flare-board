#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>
#include "config.h"
#include "state.h"

static char cmdBuffer[MAX_CMD_LENGTH + 1];
static uint8_t cmdPos = 0;

void sendStatus() {
  Serial.print("STATUS ");
  Serial.print(systemState == STATE_ARMED ? "ARMED" : "DISARMED");
  for (int i = 0; i < NUM_CHANNELS; i++) {
    Serial.print(" CH");
    Serial.print(i + 1);
    Serial.print(":");
    if (channelFiring[i]) {
      Serial.print("FIRING");
    } else if (channelFired[i]) {
      Serial.print("FIRED");
    } else {
      Serial.print("READY");
    }
  }
  Serial.println();
}

void processCommand(const char* cmd) {
  // PING
  if (strcmp(cmd, "PING") == 0) {
    Serial.println("OK PONG");
    return;
  }

  // ARM
  if (strcmp(cmd, "ARM") == 0) {
    if (systemState == STATE_ARMED) {
      Serial.println("OK ARM ALREADY_ARMED");
    } else {
      systemState = STATE_ARMED;
      lastHeartbeatMs = millis();
      Serial.println("OK ARM");
    }
    return;
  }

  // DISARM
  if (strcmp(cmd, "DISARM") == 0) {
    allRelaysOff();
    systemState = STATE_DISARMED;
    Serial.println("OK DISARM");
    return;
  }

  // FIRE N
  if (strncmp(cmd, "FIRE ", 5) == 0) {
    int n = atoi(cmd + 5);
    if (n < 1 || n > NUM_CHANNELS) {
      Serial.println("ERR FIRE INVALID_CHANNEL");
      return;
    }
    const char* err = doFire(n - 1);
    if (err) {
      Serial.print("ERR FIRE ");
      Serial.println(err);
    } else {
      Serial.print("OK FIRE ");
      Serial.println(n);
    }
    return;
  }

  // STATUS
  if (strcmp(cmd, "STATUS") == 0) {
    sendStatus();
    return;
  }

  // HEARTBEAT
  if (strcmp(cmd, "HEARTBEAT") == 0) {
    lastHeartbeatMs = millis();
    Serial.println("OK HEARTBEAT");
    return;
  }

  // RESET
  if (strcmp(cmd, "RESET") == 0) {
    if (systemState != STATE_DISARMED) {
      Serial.println("ERR RESET NOT_DISARMED");
    } else {
      resetChannels();
      Serial.println("OK RESET");
    }
    return;
  }

  Serial.println("ERR UNKNOWN_CMD");
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmdPos > 0) {
        cmdBuffer[cmdPos] = '\0';
        processCommand(cmdBuffer);
        cmdPos = 0;
      }
    } else {
      if (cmdPos < MAX_CMD_LENGTH) {
        cmdBuffer[cmdPos++] = c;
      } else {
        // Overflow — reject and reset
        Serial.println("ERR CMD_TOO_LONG");
        cmdPos = 0;
        // Drain remaining chars until newline
        while (Serial.available()) {
          char d = Serial.read();
          if (d == '\n' || d == '\r') break;
        }
      }
    }
  }
}

#endif
