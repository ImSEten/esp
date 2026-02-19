#ifndef WEB_DISPLAY_H
#define WEB_DISPLAY_H

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include "AirIqController.h"
#include "Common.h"
#include "WeatherController.h"

// read only
typedef struct {
  PMData *pmData;
  WeatherData *weatherDataCity;
  WeatherData *weatherDataHome;
  QueueHandle_t *web_input_queue;   // 从web对话框中输入
  QueueHandle_t *web_output_queue;  // 从web对话框中输入
  SemaphoreHandle_t mutex;          // 互斥锁
} WebData;

void setupWebServer(String domain_name, WebData *webData);
void HandleWebRequest(void *pvParameters);

#endif