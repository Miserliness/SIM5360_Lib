#include "httpClientK.h"
#include <SPIFFS.h>
#include "esp_tls.h"
#include <WiFi.h>
//#include <main.h>
#include <ArduinoJson.h>
using namespace std;

String auth_json;
String token;
String refreshtoken;


String get_body(String buf, size_t beg)
{
    size_t begin = beg;
    String body = "";
    for (int i = begin; i < buf.length(); i++)
    {
        body += buf[i];
    }
    return body;
}

int httpRequest(String method, String host, String url, String header, String data, String *response)
{
    int ret, len, beg = -1;

    int contentLength = data.length();
    char intToStr[7];
    bzero(intToStr, sizeof(intToStr));
    __itoa(contentLength, intToStr, 10);
    String ScontentLength = String(intToStr);
#ifdef DEBUG
    Serial.println(contentLength);
#endif
    String REQUEST = method + " " + url + " HTTP/1.0\r\n" +
                     "Host: " + host + "\r\n" +
                     header +
                     "Connection: close\r\n" +
                     "Content-Type: application/json\r\n" +
                     "Content-Length:" + ScontentLength + "\r\n" +
                     "\r\n" +
                     data;

    char buf[512];
    struct esp_tls *tls = nullptr;
    esp_tls_cfg_t cfg = {};
    // cfg.cacert_pem_buf = (const unsigned char *)server_root_cert_pem_start;
    //cfg.cacert_pem_bytes = server_root_cert_pem_end - server_root_cert_pem_start;
    cfg.timeout_ms = 30000; //таймаут попытки установки соединения
    tls = esp_tls_conn_http_new(url.c_str(), &cfg);
    if (tls != NULL)
    {
#ifdef DEBUG
        Serial.print("Connection established...");
#endif
        size_t written_bytes = 0;
        do
        {
            ret = esp_tls_conn_write(tls,
                                     REQUEST.c_str() + written_bytes,
                                     REQUEST.length() - written_bytes);
            if (ret >= 0)
            {
                //ESP_LOGI(TAG, "%d bytes written", ret);
                written_bytes += ret;
            }
            else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
//ESP_LOGE(TAG, "esp_tls_conn_write  returned 0x%x", ret);
#ifdef DEBUG
                printf("esp_tls_conn_write  returned 0x%x", ret);
#endif
            }
        } while (written_bytes < REQUEST.length());

        //ESP_LOGI(TAG, "Reading HTTP response...");

        String fullResponse;

        do
        {
            len = sizeof(buf) - 1;
            bzero(buf, len);
            ret = esp_tls_conn_read(tls, (char *)buf, len);

            if (ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_WANT_READ)
                continue;

            if (ret < 0)
            {
                //ESP_LOGE(TAG, "esp_tls_conn_read  returned -0x%x", -ret);
                break;
            }

            if (ret == 0)
            {
                //ESP_LOGI(TAG, "connection closed");
                break;
            }

            len = ret;
            //ESP_LOGD(TAG, "%d bytes read", len);
            /* Print response directly to stdout as it is read */
            for (int i = 0; i < len; i++)
            {
                fullResponse += buf[i];
                if (buf[i] == '{')
                {
                    beg = i;
                }
            }
        } while (1);
#ifdef DEBUG
        Serial.println();
#endif
        esp_tls_conn_delete(tls);

        *response = fullResponse;
        String ShttpCode;
        for (int i = 9; i < 13; i++)
        {
            ShttpCode += fullResponse[i];
        }
        int httpCode = atoi(ShttpCode.c_str());
        return httpCode;
    }
    else
    {
        esp_tls_conn_delete(tls);
#ifdef DEBUG
        Serial.println("Connection failed...\n");
#endif
        return 400;
    }
}


int errPostMesures(String str)
{
#ifdef DEBUG
    Serial.println("Cheking err of POST");
#endif
    cJSON *jobj = cJSON_Parse(str.c_str());
    int code = cJSON_GetObjectItem(jobj, "code")->valueint;
    cJSON_Delete(jobj);
    if (jobj)
    {
        jobj = nullptr;
    }

    if (code == 401)
    {
        return 1;
    }
    return 0;
}