
/*!
 *  @file Adafruit_LSM6DS3.cpp Adafruit LSM6DS3 6-DoF Accelerometer
 *  and Gyroscope library
 *
 *  Bryan Siepert for Adafruit Industries
 * 	BSD (see license.txt)
 */

#include "Arduino.h"
#include <Wire.h>

#include "Adafruit_LSM6DS3.h"

/*!
 *    @brief  Instantiates a new LSM6DS3 class
 */
Adafruit_LSM6DS3::Adafruit_LSM6DS3(void) {}

bool Adafruit_LSM6DS3::_init(int32_t sensor_id) {
  // make sure we're talking to the right chip
  if (chipID() != LSM6DS3_CHIP_ID) {
    return false;
  }
  _sensorid_accel = sensor_id;
  _sensorid_gyro = sensor_id + 1;
  _sensorid_temp = sensor_id + 2;

  temperature_sensitivity = 16.0;

  reset();

  // call base class _init()
  Adafruit_LSM6DS::_init(sensor_id);

  return true;
}

/**************************************************************************/
/*!
    @brief Enables and disables the I2C master bus pulllups.
    @param enable_pullups true to enable the I2C pullups, false to disable.
*/
void Adafruit_LSM6DS3::enableI2CMasterPullups(bool enable_pullups) {
  Adafruit_BusIO_Register master_config = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LSM6DS3_MASTER_CONFIG);
  Adafruit_BusIO_RegisterBits i2c_master_pu_en =
      Adafruit_BusIO_RegisterBits(&master_config, 1, 3);

  i2c_master_pu_en.write(enable_pullups);
}
