// src/hw/pins.h
#pragma once

#if defined(BOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8)
  #include "../boards/board_waveshare_esp32s3_touch_amoled_1_8.h"
#elif defined(BOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8_V2)
  #include "../boards/board_waveshare_esp32s3_touch_amoled_1_8_v2.h"
#elif defined(BOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_75C)
  #include "../boards/board_waveshare_esp32s3_touch_amoled_1_75c.h"
#elif defined(BOARD_WAVESHARE_ESP32C6_TOUCH_AMOLED_2_16)
  #include "../boards/board_waveshare_esp32c6_touch_amoled_2_16.h"
#elif defined(BOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_2_16)
  #include "../boards/board_waveshare_esp32s3_touch_amoled_2_16.h"
#else
  #error "No board defined. Add -DBOARD_WAVESHARE_ESP32S3_TOUCH_AMOLED_1_8, _1_8_V2, _1_75C, _ESP32C6_TOUCH_AMOLED_2_16, or _ESP32S3_TOUCH_AMOLED_2_16 to build_flags in platformio.ini."
#endif
