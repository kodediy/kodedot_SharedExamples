/*!
 * @file Adafruit_LIS2MDL.h
 *
 * This is a library for the LIS2MDL magnentometer/compass
 *
 * Designed specifically to work with the Adafruit LSM303AGR and LIS2MDL
 * Breakouts
 *
 * These displays use I2C to communicate, 2 pins are required to interface.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing products
 * from Adafruit!
 *
 * Written by Bryan Siepert for Adafruit Industries.
 * BSD license, all text above must be included in any redistribution
 */

#ifndef LIS2MDL_MAG_H
#define LIS2MDL_MAG_H

#include <Adafruit_BusIO_Register.h>

#include <Adafruit_I2CDevice.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

/*=========================================================================
    I2C ADDRESS/BITS
    -----------------------------------------------------------------------*/
#define _ADDRESS_MAG 0x1E   //!< Default address
#define _CHIP_ID 0x40       //!< Chip ID from WHO_AM_I register
#define LIS2MDL_MAG_LSB 1.5 //!< Sensitivity
#define LIS2MDL_MILLIGAUSS_TO_MICROTESLA                                       \
  0.1 //!< Conversion rate of Milligauss to Microtesla
/*=========================================================================*/

/*!
 * @brief LIS2MDL I2C register address bits
 */
typedef enum {
  LIS2MDL_OFFSET_X_REG_L = 0x45,
  LIS2MDL_OFFSET_X_REG_H = 0x46,
  LIS2MDL_OFFSET_Y_REG_L = 0x47,
  LIS2MDL_OFFSET_Y_REG_H = 0x48,
  LIS2MDL_OFFSET_Z_REG_L = 0x49,
  LIS2MDL_OFFSET_Z_REG_H = 0x4A,
  LIS2MDL_WHO_AM_I = 0x4F,
  LIS2MDL_CFG_REG_A = 0x60,
  LIS2MDL_CFG_REG_B = 0x61,
  LIS2MDL_CFG_REG_C = 0x62,
  LIS2MDL_INT_CRTL_REG = 0x63,
  LIS2MDL_INT_SOURCE_REG = 0x64,
  LIS2MDL_INT_THS_L_REG = 0x65,
  LIS2MDL_STATUS_REG = 0x67,
  LIS2MDL_OUTX_L_REG = 0x68,
  LIS2MDL_OUTX_H_REG = 0x69,
  LIS2MDL_OUTY_L_REG = 0x6A,
  LIS2MDL_OUTY_H_REG = 0x6B,
  LIS2MDL_OUTZ_L_REG = 0x6C,
  LIS2MDL_OUTZ_H_REG = 0x6D,
} lis2mdl_register_t;
/*=========================================================================*/

/*!
 * @brief Magnetometer update rate settings
 */
typedef enum {
  LIS2MDL_RATE_10_HZ,  //!< 10 Hz
  LIS2MDL_RATE_20_HZ,  //!< 20 Hz
  LIS2MDL_RATE_50_HZ,  //!< 50 Hz
  LIS2MDL_RATE_100_HZ, //!< 100 Hz
} lis2mdl_rate_t;
/*=========================================================================*/

/**************************************************************************/
/*!
    @brief  a data thing
*/
/**************************************************************************/
typedef struct lis2mdl_data {
  int16_t x; ///< x-axis raw data
  int16_t y; ///< y-axis raw data
  int16_t z; ///< z-axis raw data
} lis2mdl_data_t;
/*=========================================================================*/

/**************************************************************************/
/*!
    @brief  Unified sensor driver for the magnetometer
*/
/**************************************************************************/
class Adafruit_LIS2MDL : public Adafruit_Sensor {
public:
  Adafruit_LIS2MDL(int32_t sensorID = -1);

  bool begin(uint8_t i2c_addr = _ADDRESS_MAG, TwoWire *wire = &Wire);
  bool begin_SPI(uint8_t cs_pin, SPIClass *theSPI = &SPI);
  bool begin_SPI(int8_t cs_pin, int8_t sck_pin, int8_t miso_pin,
                 int8_t mosi_pin);

  lis2mdl_rate_t getDataRate();
  void setDataRate(lis2mdl_rate_t rate);
  bool getEvent(sensors_event_t *);
  void getSensor(sensor_t *);
  void reset(void);

  void enableInterrupts(bool);
  void interruptsActiveHigh(bool);
  lis2mdl_data_t raw; ///< struct instance to hold raw data

private:
  bool _init(void);

  int32_t _sensorID;
  Adafruit_BusIO_Register *config_a;

  void read(void);

  Adafruit_I2CDevice *i2c_dev = NULL;
  Adafruit_SPIDevice *spi_dev = NULL;
};

#endif
