/*!
 *  @file Adafruit_ISM330DHCX.h
 *
 * 	I2C Driver for the Adafruit ISM330DHCX 6-DoF Accelerometer and Gyroscope
 *library
 *
 * 	This is a library for the Adafruit ISM330DHCX breakout:
 * 	https://www.adafruit.com/products/4480
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *
 *	BSD license (see license.txt)
 */

#ifndef _ADAFRUIT_ISM330DHCX_H
#define _ADAFRUIT_ISM330DHCX_H

#include "Adafruit_LSM6DSOX.h"

#define ISM330DHCX_CHIP_ID 0x6B ///< ISM330DHCX default device id from WHOAMI

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            the ISM330DHCX I2C Digital Potentiometer
 */
class Adafruit_ISM330DHCX : public Adafruit_LSM6DSOX {
public:
  Adafruit_ISM330DHCX();
  ~Adafruit_ISM330DHCX(){};

private:
  bool _init(int32_t sensor_id);
};

#endif
