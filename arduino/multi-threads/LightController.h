// LightController.h
#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Light结构体定义
typedef struct {
  uint num_leds;            // LED数量
  CRGB *leds;               // LED颜色数组
  uint brightness;          // LED亮度
  SemaphoreHandle_t mutex;  // 互斥锁
} Light;

// 全局变量声明
extern const CRGB COLOR_WHITE;

// 函数声明
void setLedColor(Light *light, CRGB rgb);
void setLedBrightness(Light *light);
void lazyOnLed(Light *light, uint delay_time);
void lazyOffLed(Light *light, uint delay_time);
void setupLight(Light *light, CRGB rgb);

#endif  // LIGHT_CONTROLLER_H