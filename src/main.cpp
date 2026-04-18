#include <Arduino.h>
#include "hw/hw.h"

void setup() {
  hwInit();
  Serial.println("idle (quiet)");
}

void loop() {
  hwInputUpdate();
  delay(50);
}
