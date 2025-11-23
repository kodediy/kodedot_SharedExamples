/*!
 * @file Adafruit_LSM303_Accel.cpp
 *
 * @mainpage Adafruit LSM303 Accelerometer
 *
 * @section intro_sec Introduction
 *
 * This is a library for the LSM303 Accelerometer and magnentometer/compass
 *
 * Designed specifically to work with the Adafruit LSM303DLHC Breakout
 *
 * These displays use I2C to communicate, 2 pins are required to interface.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit andopen-source hardware by purchasing products
 * from Adafruit!
 *
 * @section author Author
 *
 * Written by Bryan Siepert for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 *
 */

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Adafruit_LSM303_Accel.h"
#include <Wire.h>
#include <limits.h>

/***************************************************************************
 CONSTRUCTOR
 ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Instantiates a new Adafruit_LSM303 class
    @param sensorID an optional identifier for the sensor instance
*/
/**************************************************************************/
Adafruit_LSM303_Accel_Unified::Adafruit_LSM303_Accel_Unified(int32_t sensorID) {
  _sensorID = sensorID;

  // Clear the raw accel data
  raw.x = 0;
  raw.y = 0;
  raw.z = 0;
}

/***************************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************************/

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  i2c_address
 *            The I2C address to be used.
 *    @param  wire
 *            The Wire object to be used for I2C connections.
 *    @return True if initialization was successful, otherwise false.
 */
bool Adafruit_LSM303_Accel_Unified::begin(uint8_t i2c_address, TwoWire *wire) {

  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

  if (!i2c_dev->begin()) {
    return false;
  }
  Adafruit_BusIO_Register ctrl1 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 1);
  // Enable the accelerometer (100Hz)
  ctrl1.write(0x57);

  Adafruit_BusIO_Register chip_id =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_WHO_AM_I, 1);
  if (chip_id.read() != 0x33) {
    // No LSM30 detected ... return false
    return false;
  }

  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the most recent sensor event, Adafruit Unified Sensor format
    @param  event Pointer to an Adafruit Unified sensor_event_t object that
            we'll fill in
    @returns True on successful read
*/
/**************************************************************************/

bool Adafruit_LSM303_Accel_Unified::getEvent(sensors_event_t *event) {
  /* Clear the event */
  memset(event, 0, sizeof(sensors_event_t));

  /* Read new raw data */
  readRawData();
  lsm303_accel_mode_t mode = getMode();

  float lsb = getLSB(mode);
  uint8_t shift = getShift(mode);

  event->version = sizeof(sensors_event_t);
  event->sensor_id = _sensorID;
  event->type = SENSOR_TYPE_ACCELEROMETER;
  event->timestamp = millis();
  event->acceleration.x =
      (float)(raw.x >> shift) * lsb * SENSORS_GRAVITY_STANDARD;
  event->acceleration.y =
      (float)(raw.y >> shift) * lsb * SENSORS_GRAVITY_STANDARD;
  event->acceleration.z =
      (float)(raw.z >> shift) * lsb * SENSORS_GRAVITY_STANDARD;

  return true;
}

/**************************************************************************/
/*!
    @brief  Gets the sensor_t data
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::getSensor(sensor_t *sensor) {
  /* Clear the sensor_t object */
  memset(sensor, 0, sizeof(sensor_t));

  /* Insert the sensor name in the fixed length char array */
  strncpy(sensor->name, "LSM303", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name) - 1] = 0;
  sensor->version = 1;
  sensor->sensor_id = _sensorID;
  sensor->type = SENSOR_TYPE_ACCELEROMETER;
  sensor->min_delay = 0;
  sensor->max_value = 0.0F;  // TBD
  sensor->min_value = 0.0F;  // TBD
  sensor->resolution = 0.0F; // TBD
}

/**************************************************************************/
/*!
    @brief Sets the accelerometer's range
    @param new_range an `lsm303_accel_range_t` representing the range of
    measurements in +/-G. The smaller the range, the more accurate.
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::setRange(lsm303_accel_range_t new_range) {
  Adafruit_BusIO_Register ctrl_4 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 1);

  Adafruit_BusIO_RegisterBits range =
      Adafruit_BusIO_RegisterBits(&ctrl_4, 2, 4);

  range.write(new_range);
}

/**************************************************************************/
/*!
    @brief Gets the accelerometer's range
    @returns The `lsm303_accel_range_t` representing the range of
    measurements in +/-G.
*/
/**************************************************************************/
lsm303_accel_range_t Adafruit_LSM303_Accel_Unified::getRange(void) {
  Adafruit_BusIO_Register ctrl_4 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 1);
  Adafruit_BusIO_RegisterBits range =
      Adafruit_BusIO_RegisterBits(&ctrl_4, 2, 4);

  return (lsm303_accel_range_t)range.read();
}

/**************************************************************************/
/*!
    @brief Sets the accelerometer's power mode
    @param new_mode an `lsm303_accel_mode_t` representing the power mode.
    The mode effects the precision of the sensor's readings
    High resolution is 12-bit
    Normal mode is 10-bit
    Low power is 8-bit
*/
/**************************************************************************/

void Adafruit_LSM303_Accel_Unified::setMode(lsm303_accel_mode_t new_mode) {

  Adafruit_BusIO_Register ctrl_1 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 1);

  Adafruit_BusIO_Register ctrl_4 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 1);

  Adafruit_BusIO_RegisterBits low_power =
      Adafruit_BusIO_RegisterBits(&ctrl_1, 1, 3);

  Adafruit_BusIO_RegisterBits hi_res =
      Adafruit_BusIO_RegisterBits(&ctrl_4, 1, 3);

  hi_res.write(new_mode & 0b01);
  delay(20);
  low_power.write((new_mode & 0b10) >> 1);
  delay(20);
}
/**************************************************************************/
/*!
    @brief Get the accelerometer's power mode
    @returns The `lsm303_accel_mode_t` representing the power mode.
*/
/**************************************************************************/
lsm303_accel_mode_t Adafruit_LSM303_Accel_Unified::getMode(void) {

  Adafruit_BusIO_Register ctrl_1 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 1);
  Adafruit_BusIO_Register ctrl_4 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG4_A, 1);

  Adafruit_BusIO_RegisterBits low_power =
      Adafruit_BusIO_RegisterBits(&ctrl_1, 1, 3);
  Adafruit_BusIO_RegisterBits hi_res =
      Adafruit_BusIO_RegisterBits(&ctrl_4, 1, 3);

  uint8_t low_power_bit = low_power.read();
  uint8_t hi_res_bit = hi_res.read();
  return (lsm303_accel_mode_t)(low_power_bit << 1 | hi_res_bit);
}
// /***************************************************************************
//  PRIVATE FUNCTIONS
//  ***************************************************************************/

/**************************************************************************/
/*!
    @brief  Reads the raw data from the sensor
*/
/**************************************************************************/
void Adafruit_LSM303_Accel_Unified::readRawData() {
  // this sucks but using one register with a 6 byte read to buffer doesn't
  // work.
  Adafruit_BusIO_Register data_reg0 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_X_L_A, 1);
  Adafruit_BusIO_Register data_reg1 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_X_H_A, 1);
  Adafruit_BusIO_Register data_reg2 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_Y_L_A, 1);
  Adafruit_BusIO_Register data_reg3 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_Y_H_A, 1);
  Adafruit_BusIO_Register data_reg4 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_Z_L_A, 1);
  Adafruit_BusIO_Register data_reg5 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_OUT_Z_H_A, 1);

  uint8_t xlo = data_reg0.read();
  uint8_t xhi = data_reg1.read();
  uint8_t ylo = data_reg2.read();
  uint8_t yhi = data_reg3.read();
  uint8_t zlo = data_reg4.read();
  uint8_t zhi = data_reg5.read();

  raw.x = (int16_t)(xlo | (xhi << 8));
  raw.y = (int16_t)(ylo | (yhi << 8));
  raw.z = (int16_t)(zlo | (zhi << 8));
}
/**************************************************************************/
/*!
    @brief  Gets the Least Significant Bit value for the current mode
    @param mode the current mode, used to determind the appropriate lsb value
    in concert with the current range setting.
*/
/**************************************************************************/
float Adafruit_LSM303_Accel_Unified::getLSB(lsm303_accel_mode_t mode) {
  float lsb = 0;
  lsm303_accel_range_t range = getRange();
  if (mode == LSM303_MODE_NORMAL) {
    switch (range) {
    case LSM303_RANGE_2G:
      lsb = 0.0039;
      break;
    case LSM303_RANGE_4G:
      lsb = 0.00782;
      break;
    case LSM303_RANGE_8G:
      lsb = 0.01563;
      break;
    case LSM303_RANGE_16G:
      lsb = 0.0469;
      break;
    }
  }

  else if (mode == LSM303_MODE_HIGH_RESOLUTION) {
    switch (range) {
    case LSM303_RANGE_2G:
      lsb = 0.00098;
      break;
    case LSM303_RANGE_4G:
      lsb = 0.00195;
      break;
    case LSM303_RANGE_8G:
      lsb = 0.0039;
      break;
    case LSM303_RANGE_16G:
      lsb = 0.01172;
      break;
    }
  } else if (mode == LSM303_MODE_LOW_POWER) {
    switch (range) {
    case LSM303_RANGE_2G:
      lsb = 0.01563;
      break;
    case LSM303_RANGE_4G:
      lsb = 0.03126;
      break;
    case LSM303_RANGE_8G:
      lsb = 0.06252;
      break;
    case LSM303_RANGE_16G:
      lsb = 0.18758;
      break;
    }
  }

  return lsb;
}
/**************************************************************************/
/*!
    @brief  Gets the bit shift amount for the current mode
    @param mode the current mode, used to determind the appropriate shift
    amount based on the bitdepth of the mode
*/
/**************************************************************************/
uint8_t Adafruit_LSM303_Accel_Unified::getShift(lsm303_accel_mode_t mode) {
  uint8_t shift = 0;
  switch (mode) {
  case LSM303_MODE_HIGH_RESOLUTION:
    shift = 4;
    break;
  case LSM303_MODE_NORMAL:
    shift = 6;
    break;
  case LSM303_MODE_LOW_POWER:
    shift = 8;
    break;
  }

  return shift;
}

/*************************************************************************/
/*!
    @brief Sets the polarity of the interrupt pins
    @param active_high Set to true for the INT pints to be active high,
        false for active low
*/
void Adafruit_LSM303_Accel_Unified::interruptsActiveHigh(bool active_high) {
  Adafruit_BusIO_Register ctrl_6 =
      Adafruit_BusIO_Register(i2c_dev, LSM303_REGISTER_ACCEL_CTRL_REG6_A);
  Adafruit_BusIO_RegisterBits active_low_bit =
      Adafruit_BusIO_RegisterBits(&ctrl_6, 1, 1);

  active_low_bit.write(active_high);
}
