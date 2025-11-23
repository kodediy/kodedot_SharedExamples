/*!
 * @file Adafruit_LSM303_Accel.h
 *
 */

#ifndef LSM303_ACCEL_H
#define LSM303_ACCEL_H

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define LSM303_ADDRESS_ACCEL (0x32 >> 1) //!< I2C address/bits, 0011001x

/*!
 * @brief Registers
 */
typedef enum {                               // DEFAULT    TYPE
  LSM303_REGISTER_ACCEL_WHO_AM_I = 0x0F,     // 00000111   rw
  LSM303_REGISTER_ACCEL_CTRL_REG1_A = 0x20,  // 00000111   rw
  LSM303_REGISTER_ACCEL_CTRL_REG2_A = 0x21,  // 00000000   rw
  LSM303_REGISTER_ACCEL_CTRL_REG3_A = 0x22,  // 00000000   rw
  LSM303_REGISTER_ACCEL_CTRL_REG4_A = 0x23,  // 00000000   rw
  LSM303_REGISTER_ACCEL_CTRL_REG5_A = 0x24,  // 00000000   rw
  LSM303_REGISTER_ACCEL_CTRL_REG6_A = 0x25,  // 00000000   rw
  LSM303_REGISTER_ACCEL_REFERENCE_A = 0x26,  // 00000000   r
  LSM303_REGISTER_ACCEL_STATUS_REG_A = 0x27, // 00000000   r
  LSM303_REGISTER_ACCEL_OUT_X_L_A = 0x28,
  LSM303_REGISTER_ACCEL_OUT_X_H_A = 0x29,
  LSM303_REGISTER_ACCEL_OUT_Y_L_A = 0x2A,
  LSM303_REGISTER_ACCEL_OUT_Y_H_A = 0x2B,
  LSM303_REGISTER_ACCEL_OUT_Z_L_A = 0x2C,
  LSM303_REGISTER_ACCEL_OUT_Z_H_A = 0x2D,
  LSM303_REGISTER_ACCEL_FIFO_CTRL_REG_A = 0x2E,
  LSM303_REGISTER_ACCEL_FIFO_SRC_REG_A = 0x2F,
  LSM303_REGISTER_ACCEL_INT1_CFG_A = 0x30,
  LSM303_REGISTER_ACCEL_INT1_SOURCE_A = 0x31,
  LSM303_REGISTER_ACCEL_INT1_THS_A = 0x32,
  LSM303_REGISTER_ACCEL_INT1_DURATION_A = 0x33,
  LSM303_REGISTER_ACCEL_INT2_CFG_A = 0x34,
  LSM303_REGISTER_ACCEL_INT2_SOURCE_A = 0x35,
  LSM303_REGISTER_ACCEL_INT2_THS_A = 0x36,
  LSM303_REGISTER_ACCEL_INT2_DURATION_A = 0x37,
  LSM303_REGISTER_ACCEL_CLICK_CFG_A = 0x38,
  LSM303_REGISTER_ACCEL_CLICK_SRC_A = 0x39,
  LSM303_REGISTER_ACCEL_CLICK_THS_A = 0x3A,
  LSM303_REGISTER_ACCEL_TIME_LIMIT_A = 0x3B,
  LSM303_REGISTER_ACCEL_TIME_LATENCY_A = 0x3C,
  LSM303_REGISTER_ACCEL_TIME_WINDOW_A = 0x3D
} lsm303AccelRegisters_t;

/*=========================================================================*/

/**************************************************************************/
/*!
    @brief  INTERNAL ACCELERATION DATA TYPE
*/
/**************************************************************************/
typedef struct lsm303AccelData_s {
  int16_t x; ///< x-axis data
  int16_t y; ///< y-axis data
  int16_t z; ///< z-axis data
} lsm303AccelData;
/*=========================================================================*/

/*!
 * @brief Set of linear acceleration measurement ranges
 */
typedef enum range {
  LSM303_RANGE_2G,  ///< Measurement range from +2G to -2G (19.61 m/s^2)
  LSM303_RANGE_4G,  ///< Measurement range from +4G to -4G (39.22 m/s^2)
  LSM303_RANGE_8G,  ///< Measurement range from +8G to -8G (78.45 m/s^2)
  LSM303_RANGE_16G, ///< Measurement range from +16G to -16G (156.9 m/s^2)
} lsm303_accel_range_t;

/*!
 * @brief Set of different modes that can be used. Normal, high resolution, and
 * low power
 */
typedef enum mode {
  LSM303_MODE_NORMAL,          ///< Normal measurement mode; 10-bit
  LSM303_MODE_HIGH_RESOLUTION, ///< High resolution mode; 12-bit
  LSM303_MODE_LOW_POWER,       ///< Low power mode; 8-bit
} lsm303_accel_mode_t;

#define LSM303_ID (0b11010100) //!< Chip ID

/*!
  @brief Unified sensor driver for the accelerometer
*/
class Adafruit_LSM303_Accel_Unified : public Adafruit_Sensor {
public:
  Adafruit_LSM303_Accel_Unified(int32_t sensorID = -1);

  bool begin(uint8_t i2c_addr = LSM303_ADDRESS_ACCEL, TwoWire *wire = &Wire);
  bool getEvent(sensors_event_t *);
  void getSensor(sensor_t *);

  void setRange(lsm303_accel_range_t);
  lsm303_accel_range_t getRange(void);

  lsm303_accel_mode_t getMode(void);
  void setMode(lsm303_accel_mode_t);

  void interruptsActiveHigh(bool);

private:
  int32_t _sensorID;

  lsm303AccelData raw; // Last read accelerometer data will be available here
  float getLSB(lsm303_accel_mode_t);
  uint8_t getShift(lsm303_accel_mode_t);

  void readRawData(void);

  Adafruit_I2CDevice *i2c_dev;
};

#endif
