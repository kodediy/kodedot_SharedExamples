/*!
 *  @file Adafruit_LSM6DS3.h
 *
 * 	I2C Driver for the Adafruit LSM6DS3 6-DoF Accelerometer and Gyroscope
 *library
 *
 * 	This is a library for the Adafruit LSM6DS3 breakout:
 * 	https://www.adafruit.com/
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *
 *	BSD license (see license.txt)
 */

#ifndef _ADAFRUIT_LSM6DS3_H
#define _ADAFRUIT_LSM6DS3_H

#include "Adafruit_LSM6DS.h"

#define LSM6DS3_CHIP_ID 0x69 ///< LSM6DS3 default device id from WHOAMI

#define LSM6DS3_MASTER_CONFIG 0x1A ///< I2C Master config

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            the LSM6DS3
 */
class Adafruit_LSM6DS3 : public Adafruit_LSM6DS {
public:
  Adafruit_LSM6DS3();
  ~Adafruit_LSM6DS3(){};

  void enableI2CMasterPullups(bool enable_pullups);

private:
  bool _init(int32_t sensor_id);
};

#endif
