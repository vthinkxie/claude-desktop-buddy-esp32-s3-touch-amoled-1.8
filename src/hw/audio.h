#pragma once
#include <stdint.h>

bool hwAudioInit();
void hwBeep(uint16_t freqHz, uint16_t durMs);
