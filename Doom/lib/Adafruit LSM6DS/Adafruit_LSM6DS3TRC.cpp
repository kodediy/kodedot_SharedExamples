
/*!
 *  @file Adafruit_LSM6DS3TRC.cpp Adafruit LSM6DS3TR-C 6-DoF Accelerometer
 *  and Gyroscope library
 *
 *  Written by ladyada for Adafruit Industries
 * 	BSD (see license.txt)
 */

#include "Arduino.h"
#include <Wire.h>

#include "Adafruit_LSM6DS3TRC.h"

/*!
 *    @brief  Instantiates a new LSM6DS3TRC class
 */
Adafruit_LSM6DS3TRC::Adafruit_LSM6DS3TRC(void) {}

bool Adafruit_LSM6DS3TRC::_init(int32_t sensor_id) {
  // make sure we're talking to the right chip
  if (chipID() != LSM6DS3TRC_CHIP_ID) {
    return false;
  }
  _sensorid_accel = sensor_id;
  _sensorid_gyro = sensor_id + 1;
  _sensorid_temp = sensor_id + 2;

  reset();

  // call base class _init()
  Adafruit_LSM6DS::_init(sensor_id);

  // set the Block Data Update bit
  // this prevents MSB/LSB data registers from being updated until both are read
  Adafruit_BusIO_Register ctrl3 = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LSM6DS_CTRL3_C);
  Adafruit_BusIO_RegisterBits bdu = Adafruit_BusIO_RegisterBits(&ctrl3, 1, 6);
  bdu.write(1);

  return true;
}

/**************************************************************************/
/*!
    @brief Enables and disables the pedometer function
    @param enable True to turn on the pedometer function, false to turn off
*/
/**************************************************************************/
void Adafruit_LSM6DS3TRC::enablePedometer(bool enable) {
  // enable or disable functionality
  Adafruit_BusIO_Register ctrl10 = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LSM6DS_CTRL10_C);

  Adafruit_BusIO_RegisterBits ped_en =
      Adafruit_BusIO_RegisterBits(&ctrl10, 1, 4);
  ped_en.write(enable);

  Adafruit_BusIO_RegisterBits func_en =
      Adafruit_BusIO_RegisterBits(&ctrl10, 1, 2);
  func_en.write(enable);

  resetPedometer();
}

/**************************************************************************/
/*!
    @brief Enables and disables the I2C master bus pulllups.
    @param enable_pullups true to enable the I2C pullups, false to disable.
*/
void Adafruit_LSM6DS3TRC::enableI2CMasterPullups(bool enable_pullups) {
  Adafruit_BusIO_Register master_config = Adafruit_BusIO_Register(
      i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, LSM6DS3TRC_MASTER_CONFIG);
  Adafruit_BusIO_RegisterBits i2c_master_pu_en =
      Adafruit_BusIO_RegisterBits(&master_config, 1, 3);

  i2c_master_pu_en.write(enable_pullups);
}
