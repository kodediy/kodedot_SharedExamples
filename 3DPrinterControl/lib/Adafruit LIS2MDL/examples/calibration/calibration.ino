#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS2MDL.h>
#include <float.h>

Adafruit_LIS2MDL mag = Adafruit_LIS2MDL(12345);

float MagMinX = FLT_MAX, MagMaxX = FLT_MIN;
float MagMinY = FLT_MAX, MagMaxY = FLT_MIN;
float MagMinZ = FLT_MAX, MagMaxZ = FLT_MIN;

long lastDisplayTime;

void setup(void)
{
  Serial.begin(115200);
  Serial.println("LIS2MDL Calibration"); Serial.println("");

  /* Initialise the magnetometer */
  if(!mag.begin())
  {
    /* There was a problem detecting the LIS2MDL ... check your connections */
    Serial.println("Ooops, no LIS2MDL detected ... Check your wiring!");
    while(1);
  }
  lastDisplayTime = millis();
}

void loop(void)
{
  /* Get a new sensor event */
  sensors_event_t magEvent;

  mag.getEvent(&magEvent);

  if (magEvent.magnetic.x < MagMinX) MagMinX = magEvent.magnetic.x;
  if (magEvent.magnetic.x > MagMaxX) MagMaxX = magEvent.magnetic.x;

  if (magEvent.magnetic.y < MagMinY) MagMinY = magEvent.magnetic.y;
  if (magEvent.magnetic.y > MagMaxY) MagMaxY = magEvent.magnetic.y;

  if (magEvent.magnetic.z < MagMinZ) MagMinZ = magEvent.magnetic.z;
  if (magEvent.magnetic.z > MagMaxZ) MagMaxZ = magEvent.magnetic.z;

  if ((millis() - lastDisplayTime) > 1000)  // display once/second
  {
    Serial.print("Mag Minimums: "); Serial.print(MagMinX); Serial.print("  ");Serial.print(MagMinY); Serial.print("  "); Serial.print(MagMinZ); Serial.println();
    Serial.print("Mag Maximums: "); Serial.print(MagMaxX); Serial.print("  ");Serial.print(MagMaxY); Serial.print("  "); Serial.print(MagMaxZ); Serial.println(); Serial.println();
    lastDisplayTime = millis();
  }
}
