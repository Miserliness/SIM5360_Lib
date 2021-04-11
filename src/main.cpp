#include <Arduino.h>
#include "SIM5360_Lib.h"
#include "httpClientK.h"

extern "C"
{
#include "wifi_test.h"
}

SIM5360_Lib gsm;

void setup()
{
  Serial.begin(115200);
  wifi_init_softap();
  gsm.init(17, 16, 115200, 2);
  Serial.print("Try to start ppp mode...");
  gsm.ppposStart(10000);
}

void loop()
{
  // Serial.print("Try to start ppp mode...");
  // bool b = gsm.ppposStart(5000);
  // Serial.println(b);
  // if (b)
  // {
  //   String response;
  //   int code = httpRequest("GET", "httpbin.org", "https://httpbin.org/ip", "", "", &response);
  //   Serial.print("Code: ");
  //   Serial.println(code);
  //   Serial.println(response);
  //   gsm.ppposStop();
  //   delay(3000);
  // }
}