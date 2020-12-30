#include "uartIDF.h"

#define BUF_SIZE (1024)
char out[BUF_SIZE];

UartIDF::UartIDF() {}

UartIDF::~UartIDF() {}

void UartIDF::uartInitDevice(int txPin, int rxPin, int baudrate, int uartNum, uart_word_length_t dataBits, uart_stop_bits_t stopBits)
{
    this->_baudrate = baudrate;
    this->_uartNum = (uart_port_t) uartNum;
    gpio_set_direction((gpio_num_t) txPin, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t) rxPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t) rxPin, GPIO_PULLUP_ONLY);
    uart_config_t uart_config = {};
    uart_config.baud_rate = baudrate;
    uart_config.data_bits = dataBits;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = stopBits;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    uart_param_config(_uartNum, &uart_config);
    uart_set_pin(_uartNum,(gpio_num_t) txPin,(gpio_num_t) rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(_uartNum, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
}

char *UartIDF::read()
{
  memset(out, 0, BUF_SIZE);
  int len = uart_read_bytes(_uartNum, (uint8_t *)out, BUF_SIZE, 20 / portTICK_RATE_MS);
  if (len > 0)
  {
    return out;
  }
  else
  {
    return NULL;
  }
}

void UartIDF::write(char *cmd)
{
  uart_flush(_uartNum);
  if (cmd != NULL)
  {
    int cmdSize = strlen(cmd);
    uart_write_bytes(_uartNum, (const char *)cmd, cmdSize);
    uart_wait_tx_done(_uartNum, 100 / portTICK_RATE_MS);
  }
}

void UartIDF::uartDelete(){
  uart_driver_delete(_uartNum);
}