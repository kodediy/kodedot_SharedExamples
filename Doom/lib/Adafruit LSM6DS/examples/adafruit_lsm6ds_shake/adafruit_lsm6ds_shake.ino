// Basic demo for accelerometer/gyro readings from Adafruit LSM6DS33

#include <Adafruit_LSM6DS33.h>
#include <Adafruit_LSM6DS3TRC.h> 

// For SPI mode, we need a CS pin
#define LSM_CS 10
// For software-SPI mode we need SCK/MOSI/MISO pins
#define LSM_SCK 13
#define LSM_MISO 12
#define LSM_MOSI 11


Adafruit_LSM6DS33 lsm6ds; // uncomment to use LSM6DS33
//Adafruit_LSM6DS3TRC lsm6ds; // uncomment to use LSM6DS3TR-C

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit LSM6DS shake test!");

  if (!lsm6ds.begin_I2C()) {
    // if (!lsm6ds.begin_SPI(LSM_CS)) {
    // if (!lsm6ds.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)) {
    Serial.println("Failed to find LSM6DS chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("LSM6DS Found!");

  // enable shake detection
  lsm6ds.enableWakeup(true);
}

void loop() {
  // check for shake
  if (lsm6ds.shake()) {
    Serial.println("SHAKE!");
  }
}