#include "hw/imu.h"
#include "hw/pins.h"
#include <Wire.h>
#include <SensorQMI8658.hpp>

static SensorQMI8658 s_qmi;

bool hwImuInit() {
  if (!s_qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL)) {
    Serial.println("hwImu: QMI8658 begin failed");
    return false;
  }
  s_qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_2G,
                            SensorQMI8658::ACC_ODR_125Hz);
  s_qmi.enableAccelerometer();
  return true;
}

void hwImuAccel(float* ax, float* ay, float* az) {
  IMUdata d;
  s_qmi.getAccelerometer(d.x, d.y, d.z);
  *ax = d.x;
  *ay = d.y;
  // Z is inverted vs M5StickC convention. Smoke 3 verified: screen-up
  // gives raw d.z ≈ -0.94 on this board, but the original face-down
  // detector wants az < -0.7 for face-DOWN. Flip the sign.
  *az = -d.z;
}
