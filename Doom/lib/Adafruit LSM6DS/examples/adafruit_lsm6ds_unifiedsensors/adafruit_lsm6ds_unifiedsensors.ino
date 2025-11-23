// Demo for getting individual unified sensor data from the LSM6DS series

// Can change this to be LSM6DSOX or whatever ya like
#include <Adafruit_LSM6DS33.h>
#include <Adafruit_LSM6DS3TRC.h> 

Adafruit_LSM6DS33 lsm6ds; // can use any LSM6DS/ISM330 family chip!
//Adafruit_LSM6DS3TRC lsm6ds; // uncomment to use LSM6DS3TR-C
Adafruit_Sensor *lsm_temp, *lsm_accel, *lsm_gyro;

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit LSM6DS test!");

  if (!lsm6ds.begin_I2C()) {
    Serial.println("Failed to find LSM6DS chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("LSM6DS Found!");
  lsm_temp = lsm6ds.getTemperatureSensor();
  lsm_temp->printSensorDetails();

  lsm_accel = lsm6ds.getAccelerometerSensor();
  lsm_accel->printSensorDetails();

  lsm_gyro = lsm6ds.getGyroSensor();
  lsm_gyro->printSensorDetails();
}

void loop() {
  //  /* Get a new normalized sensor event */
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  lsm_temp->getEvent(&temp);
  lsm_accel->getEvent(&accel);
  lsm_gyro->getEvent(&gyro);

  Serial.print("\t\tTemperature ");
  Serial.print(temp.temperature);
  Serial.println(" deg C");

  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("\t\tAccel X: ");
  Serial.print(accel.acceleration.x);
  Serial.print(" \tY: ");
  Serial.print(accel.acceleration.y);
  Serial.print(" \tZ: ");
  Serial.print(accel.acceleration.z);
  Serial.println(" m/s^2 ");

  /* Display the results (rotation is measured in rad/s) */
  Serial.print("\t\tGyro X: ");
  Serial.print(gyro.gyro.x);
  Serial.print(" \tY: ");
  Serial.print(gyro.gyro.y);
  Serial.print(" \tZ: ");
  Serial.print(gyro.gyro.z);
  Serial.println(" radians/s ");
  Serial.println();

  delay(100);

  /*   serial plotter friendly format
  Serial.print(temp.temperature);
  Serial.print(",");

  Serial.print(accel.acceleration.x);
  Serial.print(","); Serial.print(accel.acceleration.y);
  Serial.print(","); Serial.print(accel.acceleration.z);
  Serial.print(",");

  Serial.print(gyro.gyro.x);
  Serial.print(","); Serial.print(gyro.gyro.y);
  Serial.print(","); Serial.print(gyro.gyro.z);
  Serial.println();
  delay(10);
  */
}