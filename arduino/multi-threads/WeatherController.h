#ifndef WEATHER_CONTROLLER_H
#define WEATHER_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

typedef struct {
  String address;           // 天气信息地址
  float temperature;        // 温度
  int humidity;             // 湿度
  SemaphoreHandle_t mutex;  // 互斥锁
} WeatherData;

JsonDocument marshelWeatherData(WeatherData *weatherData);

#endif