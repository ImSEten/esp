#ifndef WEB_DISPLAY_H
#define WEB_DISPLAY_H

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include "AirIqController.h"
#include "Common.h"
#include "WeatherController.h"

// read only
typedef struct {
  PMData *pmData;
  WeatherData *weatherDataCity;
  WeatherData *weatherDataHome;
  SemaphoreHandle_t mutex;  // 互斥锁
} WebData;

void setupWebServer(String domain_name, WebData *webData);
void SetupWebServer(void *pvParameters);

#endif