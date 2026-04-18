#pragma once
#include <Adafruit_XCA9554.h>

// Shared TCA9554 instance. Initialised by hwExpanderInit() before any
// driver that needs LCD/TP reset or AXP IRQ access.
extern Adafruit_XCA9554 g_expander;

bool hwExpanderInit();          // returns false on I2C failure
void hwExpanderResetSequence(); // pull EXIO 0/1/2 low → 20ms → high
bool hwExpanderAxpIrqLow();     // true while AXP IRQ pin is asserted
