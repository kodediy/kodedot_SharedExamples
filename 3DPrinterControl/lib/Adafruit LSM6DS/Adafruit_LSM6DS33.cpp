
/*!
 *  @file Adafruit_LSM6DS33.cpp Adafruit LSM6DS33 6-DoF Accelerometer
 *  and Gyroscope library
 *
 *  Bryan Siepert for Adafruit Industries
 * 	BSD (see license.txt)
 */

#include "Arduino.h"
#include <Wire.h>

#include "Adafruit_LSM6DS33.h"

/*!
 *    @brief  Instantiates a new LSM6DS33 class
 */
Adafruit_LSM6DS33::Adafruit_LSM6DS33(void) {}

bool Adafruit_LSM6DS33::_init(int32_t sensor_id) {
  // make sure we're talking to the right chip
  if (chipID() != LSM6DS33_CHIP_ID) {
    return false;
  }
  _sensorid_accel = sensor_id;
  _sensorid_gyro = sensor_id + 1;
  _sensorid_temp = sensor_id + 2;

  temperature_sensitivity = 16.0;

  reset();
  if (chipID() != LSM6DS33_CHIP_ID) {
    return false;
  }

  // call base class _init()
  Adafruit_LSM6DS::_init(sensor_id);

  return true;
}
