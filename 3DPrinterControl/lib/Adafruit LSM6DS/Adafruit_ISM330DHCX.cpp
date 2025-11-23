
/*!
 *  @file Adafruit_ISM330DHCX.cpp Adafruit ISM330DHCX 6-DoF Accelerometer
 *  and Gyroscope library
 *
 *  Bryan Siepert for Adafruit Industries
 * 	BSD (see license.txt)
 */

#include "Arduino.h"
#include <Wire.h>

#include "Adafruit_ISM330DHCX.h"

/*!
 *    @brief  Instantiates a new ISM330DHCX class
 */
Adafruit_ISM330DHCX::Adafruit_ISM330DHCX(void) {}

bool Adafruit_ISM330DHCX::_init(int32_t sensor_id) {
  // make sure we're talking to the right chip
  if (chipID() != ISM330DHCX_CHIP_ID) {
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
