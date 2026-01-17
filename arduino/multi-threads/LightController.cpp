// LightController.cpp
#include "LightController.h"
#include <FastLED.h>

const CRGB COLOR_WHITE = {255, 255, 255};

void setLedColor(Light *light, CRGB rgb) {
  if (NULL == light) {
    return;
  }
  for (int i = 0; i < light->num_leds; i++) {  // 修正：使用 < 而不是 <=
    light->leds[i] = rgb;
  }
}

void setLedBrightness(Light *light) {
  if (NULL == light) {
    return;
  }
  FastLED.setBrightness(light->brightness);
  FastLED.show();
}

void lazyOnLed(Light *light, uint delay_time) {
  if (NULL == light) {
    return;
  }
  for (int i = 0; i <= light->brightness; i++) {
    FastLED.setBrightness(i);
    vTaskDelay(pdMS_TO_TICKS(delay_time));
    FastLED.show();
  }
}

void lazyOffLed(Light *light, uint delay_time) {
  if (NULL == light) {
    return;
  }
  for (int i = light->brightness; i >= 0; i--) {
    FastLED.setBrightness(i);
    vTaskDelay(pdMS_TO_TICKS(delay_time));
    FastLED.show();
  }
  light->brightness = 0;
}

void setupLight(Light *light, CRGB rgb) {
  uint delay_time = 10;
  if (NULL == light) {
    return;
  }

  if (xSemaphoreTake(light->mutex, portMAX_DELAY)) {
    setLedColor(light, rgb);  // 白色
    lazyOnLed(light, delay_time);
    lazyOffLed(light, delay_time);
    xSemaphoreGive(light->mutex);
  } else {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: SetupLlight无法获取light锁");
  }
}