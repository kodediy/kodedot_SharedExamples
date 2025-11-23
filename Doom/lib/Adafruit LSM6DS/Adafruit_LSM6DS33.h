/*!
 *  @file Adafruit_LSM6DS33.h
 *
 * 	I2C Driver for the Adafruit LSM6DS33 6-DoF Accelerometer and Gyroscope
 *library
 *
 * 	This is a library for the Adafruit LSM6DS33 breakout:
 * 	https://www.adafruit.com/products/4480
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *
 *	BSD license (see license.txt)
 */

#ifndef _ADAFRUIT_LSM6DS33_H
#define _ADAFRUIT_LSM6DS33_H

#include "Adafruit_LSM6DS.h"

#define LSM6DS33_CHIP_ID 0x69 ///< LSM6DS33 default device id from WHOAMI

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            the LSM6DS33 I2C Digital Potentiometer
 */
class Adafruit_LSM6DS33 : public Adafruit_LSM6DS {
public:
  Adafruit_LSM6DS33();
  ~Adafruit_LSM6DS33(){};

private:
  bool _init(int32_t sensor_id);
};

#endif
