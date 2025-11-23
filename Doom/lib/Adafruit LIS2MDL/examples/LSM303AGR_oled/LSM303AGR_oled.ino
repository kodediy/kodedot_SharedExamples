#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS2MDL.h>
#include <Adafruit_LSM303_Accel.h>

Adafruit_LIS2MDL mag = Adafruit_LIS2MDL(12345);
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

void setup() {
  Serial.begin(115200);
  //while (!Serial);
  Serial.println("LIS2MDL OLED demo");

  if(!mag.begin())
  {
    /* There was a problem detecting the LIS2MDL ... check your connections */
    Serial.println("Ooops, no LIS2MDL detected ... Check your wiring!");
    while(1);
  }
  if (!accel.begin()) {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 Accelerometer detected ... Check your wiring!");
    while (1)
      ;
  }
  
  Serial.println("Found LSM303AGR sensor");

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(500); // Pause for 2 seconds
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setRotation(0);
}


void loop() {
  sensors_event_t a, m;
  accel.getEvent(&a);
  mag.getEvent(&m);

  display.clearDisplay();
  display.setCursor(0,0);
  
  Serial.print("Accelerometer ");
  Serial.print("X: "); Serial.print(a.acceleration.x, 1); Serial.print(" m/s^2, ");
  Serial.print("Y: "); Serial.print(a.acceleration.y, 1); Serial.print(" m/s^2, ");
  Serial.print("Z: "); Serial.print(a.acceleration.z, 1); Serial.println(" m/s^2");

  display.println("Accelerometer - m/s^2");
  display.print(a.acceleration.x, 1); display.print(", ");
  display.print(a.acceleration.y, 1); display.print(", ");
  display.print(a.acceleration.z, 1); display.println("");

  Serial.print("Magnetometer ");
  Serial.print("X: "); Serial.print(m.magnetic.x, 1); Serial.print(" uT, ");
  Serial.print("Y: "); Serial.print(m.magnetic.y, 1); Serial.print(" uT, ");
  Serial.print("Z: "); Serial.print(m.magnetic.z, 1); Serial.println(" uT");

  display.println("Magnetometer - uT");
  display.print(m.magnetic.x, 1); display.print(", ");
  display.print(m.magnetic.y, 1); display.print(", ");
  display.print(m.magnetic.z, 1); display.println("");
  
  display.display();
  delay(100);
}
