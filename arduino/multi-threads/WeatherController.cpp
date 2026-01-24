#include "WeatherController.h"

JsonDocument marshelWeatherData(WeatherData *weatherData) {
  JsonDocument weatherJson;
  if (xSemaphoreTake(weatherData->mutex, portMAX_DELAY)) {
    // 填充JSON数据
    // 填充JSON数据
    weatherJson["temperature"] = weatherData->temperature;
    weatherJson["humidity"] = weatherData->humidity;
    xSemaphoreGive(weatherData->mutex);
  }
  return weatherJson;
}