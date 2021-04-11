#ifndef _SIM5360_Lib_H_
#define _SIM5360_Lib_H_

#include <Arduino.h>
#include <string.h>
#include <stdint.h>

#include "tcpip_adapter.h"

extern "C"
{
#include "netif/ppp/pppos.h"
#include "netif/ppp/pppapi.h"
}

class SIM5360_Lib
{
private:
    bool _inited = false;
    bool startGPRS();
    String trim();
    String parseResponse();
    String parseInfoFromResponse();
    String sendData(String comm);

    /*Lib*/
    String _res = "";
    void parseCoords(String nmea);

    /*PPPoS*/
    struct netif ppp_netif;
    void ppposInit(char *user, char *pass);
    bool ppposConnectionStatus();

public:
    float Lat = 0;
    float Lng = 0;

    SIM5360_Lib();
    void init(int txPin, int rxPin, int baudrate, int uart_number);

    /*Lib*/
    void initGPS();
    void getPos();

    /*PPPoS*/
    bool ppposStart(int timeout);
    bool ppposStatus();
    void ppposStop();
    ~SIM5360_Lib();
};

#endif
