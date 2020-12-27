#include <Arduino.h>
#include "SIM5360_Lib.h"

SIM5360_Lib gsm;

void setup()
{
  Serial.begin(115200);
  gsm.init(17, 16, 115200, 2);
  gsm.initGPS();
  delay(1000);
}

void loop()
{
  gsm.getPos();
  Serial.println(gsm.Lat, 6);
  Serial.println(gsm.Lng, 6);
  delay(1000);
}