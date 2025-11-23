
/*!
 *  @file Adafruit_LSM6DSL.cpp Adafruit LSM6DSL 6-DoF Accelerometer
 *  and Gyroscope library
 *
 *  Adapted by Eugene Anikin for Adafruit Industries
 * 	BSD (see license.txt)
 */

#include "Arduino.h"
#include <Wire.h>

#include "Adafruit_LSM6DSL.h"

/*!
 *    @brief  Instantiates a new LSM6DSL class
 */
Adafruit_LSM6DSL::Adafruit_LSM6DSL(void) {}

bool Adafruit_LSM6DSL::_init(int32_t sensor_id) {
  // make sure we're talking to the right chip
  if (chipID() != LSM6DSL_CHIP_ID) {
    return false;
  }
  _sensorid_accel = sensor_id;
  _sensorid_gyro = sensor_id + 1;
  _sensorid_temp = sensor_id + 2;

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
void Adafruit_LSM6DSL::enableI2CMasterPullups(bool enable_pullups) {
  Adafruit_BusIO_Register master_config = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LSM6DSL_MASTER_CONFIG);
  Adafruit_BusIO_RegisterBits i2c_master_pu_en =
      Adafruit_BusIO_RegisterBits(&master_config, 1, 3);

  i2c_master_pu_en.write(enable_pullups);
}
