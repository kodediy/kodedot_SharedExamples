/*!
 *  @file Adafruit_LSM6DSO32.h
 *
 * 	I2C Driver for the Adafruit LSM6DSO32 6-DoF Accelerometer and Gyroscope
 *library
 *
 * 	This is a library for the Adafruit LSM6DSO32 breakout:
 * 	https://www.adafruit.com/products/PID_HERE
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *
 *	BSD license (see license.txt)
 */

#ifndef _ADAFRUIT_LSM6DSO32_H
#define _ADAFRUIT_LSM6DSO32_H

#include "Adafruit_LSM6DS.h"
#include "Adafruit_LSM6DSOX.h"
#define LSM6DSO32_CHIP_ID 0x6C ///< LSM6DSO32 default device id from WHOAMI

/** The accelerometer data range */
typedef enum dso32_accel_range {
  LSM6DSO32_ACCEL_RANGE_4_G,
  LSM6DSO32_ACCEL_RANGE_32_G,
  LSM6DSO32_ACCEL_RANGE_8_G,
  LSM6DSO32_ACCEL_RANGE_16_G
} lsm6dso32_accel_range_t;

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            the LSM6DSO32 I2C Digital Potentiometer
 */
class Adafruit_LSM6DSO32 : public Adafruit_LSM6DSOX {
public:
  Adafruit_LSM6DSO32();

  lsm6dso32_accel_range_t getAccelRange(void);
  void setAccelRange(lsm6dso32_accel_range_t new_range);
  void _read(void);

private:
  bool _init(int32_t sensor_id);
};

#endif
