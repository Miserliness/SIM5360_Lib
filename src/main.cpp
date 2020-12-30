#include <Arduino.h>
#include "SIM5360_Lib.h"
#include "httpClientK.h"

SIM5360_Lib gsm;

void setup()
{
  Serial.begin(115200);
  gsm.init(17, 16, 115200, 2);
  delay(10000);
  gsm.ppposStart();
  delay(2000);
}

void loop()
{
  String response;
  int code = httpRequest("GET", "httpbin.org", "https://httpbin.org/ip", "", "", &response);
  Serial.print("Code: ");
  Serial.println(code);
  Serial.println(response);

  delay(3000);
}