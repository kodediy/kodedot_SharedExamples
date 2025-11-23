Adafruit_LSM303_Accel [![Build Status](https://github.com/adafruit/Adafruit_LSM303_Accel/workflows/Arduino%20Library%20CI/badge.svg)](https://github.com/adafruit/Adafruit_LSM303_Accel/actions)[![Documentation](https://github.com/adafruit/ci-arduino/blob/master/assets/doxygen_badge.svg)](http://adafruit.github.io/Adafruit_LSM303_Accel/html/index.html)
================

<a href="https://www.adafruit.com/product/1120"><img src="assets/board.jpg?raw=true" width="500px"></a>

# Adafruit LSM303 Accelerometer Library

This library is for the accelerometer used in the LSM303AGR and LSM303DLH

Tested and works great with Adafruit's LSM303AGR and LSM303DLH Breakout Boards
* https://www.adafruit.com/product/1120
* https://www.adafruit.com/product/4413

This chip uses I2C to communicate, 2 pins are required to interface

Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!

## About the LSM303 ##

The LSM303 is a digital (I2C) accelerometer and digital compass (magnetometer) and accelerometer.  The accelerometer allows you to measure acceleration or direction towards the center or the earth or orthogonal axes.

Information on the LSM303AGR can be found in the datasheet: http://www.adafruit.com/datasheets/LSM303AGRC.PDF

Information on the LSM303DLH can be found in the datasheet: http://www.adafruit.com/datasheets/LSM303DLHC.PDF


## What is the Adafruit Unified Sensor Library? ##

The Adafruit Unified Sensor Library (**Adafruit_Sensor**) provides a common interface and data type for any supported sensor.  It defines some basic information about the sensor (sensor limits, etc.), and returns standard SI units of a specific type and scale for each supported sensor type.

It provides a simple abstraction layer between your application and the actual sensor HW, allowing you to drop in any comparable sensor with only one or two lines of code to change in your project (essentially the constructor since the functions to read sensor data and get information about the sensor are defined in the base Adafruit_Sensor class).

This is imporant useful for two reasons:

1.) You can use the data right away because it's already converted to SI units that you understand and can compare, rather than meaningless values like 0..1023.

2.) Because SI units are standardised in the sensor library, you can also do quick sanity checks working with new sensors, or drop in any comparable sensor if you need better sensitivity or if a lower cost unit becomes available, etc. 

Light sensors will always report units in lux, gyroscopes will always report units in rad/s, etc. ... freeing you up to focus on the data, rather than digging through the datasheet to understand what the sensor's raw numbers really mean.

## About this Driver ##

Written by Bryan Siepert for Adafruit Industries.
BSD license, check license.txt for more information
All text above must be included in any redistribution

To install, use the Arduino Library Manager and search for and install the following libraries:
* "Adafruit LSM303 Accel"
* "Adafruit Unified Sensor"
* "Adafruit BusIO"

Additonal details can be found in the guide:

https://learn.adafruit.com/lsm303-accelerometer-slash-compass-breakout/coding
