#ifndef _UARTIDF_H_
#define _UARTIDF_H_

#include <string.h>
#include <stdint.h>
#include "driver/uart.h"
#include "driver/gpio.h"

class UartIDF {
    private:
        int _baudrate;
        uart_port_t _uartNum;
    public:
        UartIDF();
        void uartInitDevice(int txPin, int rxPin, int baudrate, int uartNum, uart_word_length_t dataBits, uart_stop_bits_t stopBits);
        void write(char *com);
        char *read();
        void uartDelete();
        ~UartIDF();
};

#endif