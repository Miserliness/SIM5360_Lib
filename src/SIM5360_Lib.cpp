#include "uartIDF.h"
#include "SIM5360_Lib.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_timer.h"
#include "ctype.h

/* UART */
int uart_num = UART_NUM_2;

UartIDF uartDevice;

SIM5360_Lib::SIM5360_Lib(){};

SIM5360_Lib::~SIM5360_Lib(){};

void SIM5360_Lib::init(int txPin, int rxPin, int baudrate, int uart_number)
{
  uartDevice.uartInitDevice(txPin, rxPin, 115200, uart_num, UART_DATA_8_BITS, UART_STOP_BITS_1);
  sendData("AT");
  parseResponse();
  if (_res == "OK")
  {
    Serial.println("Succefull init GSM");
    _inited = true;
  }
}

String SIM5360_Lib::sendData(String comm)
{
  comm += "\r\n";
  uartDevice.write((char *)comm.c_str());
  _res = uartDevice.read();
  return _res;
}

String SIM5360_Lib::trim()
{
  if (_res.length() == 0)
    return "";
  char *buffer = (char *)_res.c_str();
  size_t len = strlen(buffer);
  if (!buffer || len == 0)
    return "";
  char *begin = buffer;
  while (isspace(*begin))
    begin++;
  char *end = buffer + len - 1;
  while (isspace(*end) && end >= begin)
    end--;
  len = end + 1 - begin;
  if (begin > buffer)
    memcpy(buffer, begin, len);
  buffer[len] = 0;
  return buffer;
}

String SIM5360_Lib::parseResponse()
{
  trim();
  if (_res.length() == 0)
    return "";
  char result[1024] = {0};
  const char *res = _res.c_str();
  char *ach = strchr(res, '\n');
  if (ach != NULL)
  {
    int i = ach - res;
    strncpy(result, res + i + 1, strlen(res) - i - 1);
    _res = result;
    return _res;
  }
  return "";
}

String SIM5360_Lib::parseInfoFromResponse()
{
  if (_res.length() == 0)
    return "";
  char result[1024] = {0};
  const char *res = _res.c_str();
  char *ach = strchr(res, '\r');
  if (ach != NULL)
  {
    int i = ach - res;
    strncpy(result, res, i);
    return result;
  }
  return "";
}

bool SIM5360_Lib::startGPRS()
{
  sendData("AT+CGATT=1");
  sendData("AT+CSQ");
  sendData("AT+CREG?");
  sendData("AT+CPSI?");
  sendData("AT+CGREG?");
  sendData("AT+CGAUTH=1,1,\"\",\"\"");
  sendData("AT+CGDCONT=1,\"IP\",\"internet\"");
  sendData("AT+CGSOCKCONT=1,\"IP\",\"internet\"");
  sendData("AT+CSOCKAUTH=1,1,\"\",\"\"");
  sendData("AT+CSOCKSETPN=1");
  sendData("AT+CIPMODE=0");
  sendData("AT+CIPSENDMODE=0");
  sendData("AT+CIPCCFG=10,0,0,0,1,0,75000");
  sendData("AT+CIPTIMEOUT=75000,15000,15000");
  sendData("AT+NETOPEN");
  sendData("AT+IPADDR");
}

//-------------------------------------------------------------------------------------------------
//GSM GPS
//-------------------------------------------------------------------------------------------------

void SIM5360_Lib::getPos()
{
  if (!_inited)
  {
    Serial.println("GSM NOT INITED");
    return;
  }
  sendData("AT+CGPSINFO");
  parseResponse();
  String res = parseInfoFromResponse();
  parseCoords(res);
}

void SIM5360_Lib::initGPS()
{
  if (!_inited)
  {
    Serial.println("GSM NOT INITED");
    return;
  }
  sendData("AT+CGSOCKCONT=1,\"IP\",\"internet.tele2.ru\"");
  sendData("AT+CGPSURL=\"supl.google.com:7276\"");
  sendData("AT+CGPS=1");
  String res = parseResponse();
  if (res == "OK" || res == "ERROR")
  {
    Serial.println("GPS ON");
  }
}

void SIM5360_Lib::parseCoords(String nmea)
{
  nmea = nmea.substring(nmea.indexOf(":") + 1);
  size_t ind = nmea.indexOf(",");
  String sN = nmea.substring(0, ind);
  this->Lat = sN.substring(0, 2).toFloat() + sN.substring(2).toFloat() / 60;
  ind = nmea.indexOf(",", ind + 1) + 1;
  String sE = nmea.substring(ind, nmea.indexOf(",", nmea.indexOf(",", ind)));
  this->Lng = sE.substring(0, 3).toFloat() + sE.substring(3).toFloat() / 60;
}

//-------------------------------------------------------------------------------------------------
//GSM PPPoS
//-------------------------------------------------------------------------------------------------

static const char *TAG = "PPPoS";
/* The PPP IP interface */
bool firststart = false;
bool _ppposConnected = false;
bool ppposStarted = false;
char *PPP_User = "";
char *PPP_Pass = "";

/* The PPP control block */
ppp_pcb *ppp;

static void pppos_client_task(void *pvParameters)
{

  char *data = (char *)malloc(1024);
  while (1)
  {
    while (ppposStarted)
    {
      memset(data, 0, 1024);
      int len = uart_read_bytes((uart_port_t)uart_num, (uint8_t *)data, 1024, 10 / portTICK_RATE_MS);
      if (len > 0)
      {
        pppos_input_tcpip(ppp, (u8_t *)data, len);
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

static u32_t ppp_output_callback(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
  return uart_write_bytes((uart_port_t)uart_num, (const char *)data, len);
}

static void ppp_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
  struct netif *pppif = ppp_netif(pcb);
  LWIP_UNUSED_ARG(ctx);

  switch (err_code)
  {
  case PPPERR_NONE:
  {
    ESP_LOGE(TAG, "status_cb: Connected\n");

#if PPP_IPV4_SUPPORT
    ESP_LOGE(TAG, "   our_ipaddr  = %s\n", ipaddr_ntoa(&pppif->ip_addr));

    ESP_LOGE(TAG, "   his_ipaddr  = %s\n", ipaddr_ntoa(&pppif->gw));
    ESP_LOGE(TAG, "   netmask     = %s\n", ipaddr_ntoa(&pppif->netmask));
#endif /* PPP_IPV4_SUPPORT */
#if PPP_IPV6_SUPPORT
    ESP_LOGE(TAG, "   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
    _ppposConnected = true;

    break;
  }
  case PPPERR_PARAM:
  {
    ESP_LOGE(TAG, "status_cb: Invalid parameter\n");
    break;
  }
  case PPPERR_OPEN:
  {
    ESP_LOGE(TAG, "status_cb: Unable to open PPP session\n");
    break;
  }
  case PPPERR_DEVICE:
  {
    ESP_LOGE(TAG, "status_cb: Invalid I/O device for PPP\n");
    break;
  }
  case PPPERR_ALLOC:
  {
    ESP_LOGE(TAG, "status_cb: Unable to allocate resources\n");
    break;
  }
  case PPPERR_USER:
  {
    ESP_LOGE(TAG, "status_cb: User interrupt\n");
    ppposStarted = false;
    _ppposConnected = false;
    break;
  }
  case PPPERR_CONNECT:
  {
    ESP_LOGE(TAG, "status_cb: Connection lost\n");
    ppposStarted = false;
    _ppposConnected = false;
    break;
  }
  case PPPERR_AUTHFAIL:
  {
    ESP_LOGE(TAG, "status_cb: Failed authentication challenge\n");
    break;
  }
  case PPPERR_PROTOCOL:
  {
    ESP_LOGE(TAG, "status_cb: Failed to meet protocol\n");
    break;
  }
  case PPPERR_PEERDEAD:
  {
    ESP_LOGE(TAG, "status_cb: Connection timeout\n");
    break;
  }
  case PPPERR_IDLETIMEOUT:
  {
    ESP_LOGE(TAG, "status_cb: Idle Timeout\n");
    break;
  }
  case PPPERR_CONNECTTIME:
  {
    ESP_LOGE(TAG, "status_cb: Max connect time reached\n");
    break;
  }
  case PPPERR_LOOPBACK:
  {
    ESP_LOGE(TAG, "status_cb: Loopback detected\n");
    break;
  }
  default:
  {
    ESP_LOGE(TAG, "status_cb: Unknown error code %d\n", err_code);
    break;
  }
  }

  if (err_code == PPPERR_NONE)
  {
    return;
  }
  if (err_code == PPPERR_USER)
  {
    return;
  }
}

void SIM5360_Lib::ppposInit(char *user, char *pass)
{
  //tcpip_adapter_init();
  PPP_User = user;
  PPP_Pass = pass;
  xTaskCreate(&pppos_client_task, "pppos_client_task", 10048, NULL, 5, NULL);
}

bool SIM5360_Lib::ppposConnectionStatus()
{
  return _ppposConnected;
}

bool SIM5360_Lib::ppposStart(int timeout)
{
  ppposInit("", "");
  startGPRS();
  sendData("AT+CGDATA=\"PPP\",1");
  delay(1000);
  if (!firststart)
  {
    ppp = pppapi_pppos_create(&ppp_netif, ppp_output_callback, ppp_status_cb, NULL);

    if (ppp == NULL)
    {
      return false;
    }

    pppapi_set_default(ppp);
    pppapi_set_auth(ppp, PPPAUTHTYPE_PAP, PPP_User, PPP_Pass);
    ppp_set_usepeerdns(ppp, 1);
  }
  pppapi_connect(ppp, 0);

  ppposStarted = true;
  firststart = true;
  long time = esp_timer_get_time() / 1000;
  while (esp_timer_get_time() / 1000 - time < timeout)
  {
    if (_ppposConnected)
      return true;
  }
  ppposStop();
  return false;
}

bool SIM5360_Lib::ppposStatus()
{
  return ppposStarted;
}

void SIM5360_Lib::ppposStop()
{
  pppapi_close(ppp, 0);
  delay(2000);
  sendData("AT+NETCLOSE");
}
