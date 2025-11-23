/*!
 *  @file Adafruit_LSM6DSOX.h
 *
 * 	I2C Driver for the Adafruit LSM6DSOX 6-DoF Accelerometer and Gyroscope
 *library
 *
 * 	This is a library for the Adafruit LSM6DSOX breakout:
 * 	https://www.adafruit.com/products/PID_HERE
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *
 *	BSD license (see license.txt)
 */

#ifndef _ADAFRUIT_LSM6DSOX_H
#define _ADAFRUIT_LSM6DSOX_H

#include "Adafruit_LSM6DS.h"

#define LSM6DSOX_CHIP_ID 0x6C ///< LSM6DSOX default device id from WHOAMI

#define LSM6DSOX_FUNC_CFG_ACCESS 0x1 ///< Enable embedded functions register
#define LSM6DSOX_PIN_CTRL 0x2        ///< Pin control register

#define LSM6DSOX_INT1_CTRL 0x0D ///< Interrupt enable for data ready
#define LSM6DSOX_CTRL1_XL 0x10  ///< Main accelerometer config register
#define LSM6DSOX_CTRL2_G 0x11   ///< Main gyro config register
#define LSM6DSOX_CTRL3_C 0x12   ///< Main configuration register
#define LSM6DSOX_CTRL9_XL 0x18  ///< Includes i3c disable bit

#define LSM6DSOX_MASTER_CONFIG 0x14
///< I2C Master config; access must be enabled with  bit SHUB_REG_ACCESS
///< is set to '1' in FUNC_CFG_ACCESS (01h).

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            the LSM6DSOX I2C Digital Potentiometer
 */
class Adafruit_LSM6DSOX : public Adafruit_LSM6DS {
public:
  Adafruit_LSM6DSOX();

  void enableI2CMasterPullups(bool enable_pullups);
  void disableSPIMasterPullups(bool disable_pullups);

private:
  bool _init(int32_t sensor_id);
};

#endif
