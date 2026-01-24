#ifndef WEB_DISPLAY_H
#define WEB_DISPLAY_H

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "AirIqController.h"
#include "Common.h"

void setupWebServer(PMData *pmData);
void SetupWebServer(void *pvParameters);

#endif