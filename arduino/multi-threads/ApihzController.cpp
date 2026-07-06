#include "ApihzController.h"

// 获取apihz动态ip
String getApihzHost(String get_api_url, String ca_cert) {
  String host;
  if (get_api_url.isEmpty()) {
    get_api_url = GET_API;
  }
  String connect_result = connectHTTPandHTTPS(get_api_url, ca_cert);
  Serial.printf("DEBUG: getApihzHost connect_result is %s\n", connect_result.c_str());

  if (connect_result.isEmpty()) {
    Serial.println("⚠️ WARN: connect apihzHost return is NULL!");
    return host;
  }

  DynamicJsonDocument doc(16);  // 16B
  DeserializationError error = deserializeJson(doc, connect_result);

  if (error) {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: JSON解析失败: %s\n", error.c_str());
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: connect return is: %s\n", connect_result.c_str());
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 请检查API返回格式或增大内存");
    return host;
  }

  if (doc["code"] == 200 && !doc["api"].isNull()) {
    host = doc["api"].as<String>();
  } else {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [HTTPS]请求失败: code: %d", doc["code"]);
    if (!doc["msg"].isNull()) {
      Serial.printf("; msg: %s\n", doc["msg"]);
    } else {
      Serial.printf("; msg: NULL\n");
    }
  }

  return host;
}

// 从apihz中获取天气信息
void getWeatherFromHost(String host, String ca_cert, String city, WeatherData *weatherData) {
  if (host.isEmpty()) {
    host = HOST_API;
  }

  String url_string = host + "api/tianqi/tqyb.php" + "?id=" + API_ID + "&key=" + API_KEY + "&sheng=四川&place=" + city + "&day=1&hourtype=1";

  String connect_result = connectHTTPandHTTPS(url_string, ca_cert);
  Serial.printf("DEBUG: getWeather connect_result is %s\n", connect_result.c_str());

  if (connect_result.isEmpty()) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: connect return is NULL!");
    return;
  }

  DynamicJsonDocument doc(1024);  // 1KB
  DeserializationError error = deserializeJson(doc, connect_result);

  if (error) {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: JSON解析失败: %s\n", error.c_str());
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: connect return is: %s\n", connect_result.c_str());
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 请检查API返回格式或增大内存");
    return;
  }

  if (doc["code"] == 200 && doc["nowinfo"]) {
    if (!doc["nowinfo"]["temperature"].isNull()) {
      if (xSemaphoreTake(weatherData->mutex, portMAX_DELAY)) {
        weatherData->temperature = doc["nowinfo"]["temperature"].as<float>();
        Serial.printf("⚡ INFO: temperature is %.2f\n", weatherData->temperature);
        xSemaphoreGive(weatherData->mutex);
      }
    } else {
      Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 天气信息temperature为NULL，无法获取温度信息\n");
    }
    if (!doc["nowinfo"]["humidity"].isNull()) {
      if (xSemaphoreTake(weatherData->mutex, portMAX_DELAY)) {
        weatherData->humidity = doc["nowinfo"]["humidity"].as<int>();
        Serial.printf("⚡ INFO: humidity is %d\n", weatherData->humidity);
        xSemaphoreGive(weatherData->mutex);
      }
    } else {
      Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 天气信息humidity为NULL，无法获取湿度信息\n");
    }
  } else {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [HTTPS]请求失败: return code: %d\n", doc["code"]);
  }
}

// 获取天气信息
void getWeather(String city, WeatherData *weatherData) {
  String host = getApihzHost(GET_API, "");

  if (host.isEmpty()) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: getWeatherHost return host is NULL");
    host = HOST_API;
  }

  getWeatherFromHost(host, "", city, weatherData);
}

// 从apihz中获取AI问答结果。
String getAIAnswerFromHost(String host, String ca_cert, String words) {
  if (host.isEmpty()) {
    host = HOST_API;
  }
  // type: 1=Qwen3.5-4B，2=Qwen3-8B，3=DeepSeek-R1-0528-Qwen3-8B，4=GLM-Z1-9B-0414，5=Qwen/Qwen2.5-7B-Instruct，6=THUDM/GLM-4-9B-0414。例：type=1
  String url_string = host + "api/ai/litejhwb.php" + "?id=" + API_ID + "&key=" + API_KEY + "&type=1" + "&words=" + words;

  String connect_result = connectHTTPandHTTPS(url_string, ca_cert);

  if (connect_result.isEmpty()) {
    return "";
  }
  DynamicJsonDocument doc(40960);  // 40KB
  DeserializationError error = deserializeJson(doc, connect_result);

  if (error) {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: JSON解析失败: %s\n", error.c_str());
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: return is: %s\n", connect_result.c_str());
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 请检查API返回格式或增大内存");
    return "";
  }
  String code = doc["code"].as<String>();
  if (doc["msg"].isNull()) {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: AI Answer return msg is NULL\n");
    return "";
  }
  String answer = doc["msg"].as<String>();
  if (code == "400") {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: AI Answer return code: %s, error is: %s\n", code.c_str(), answer.c_str());
  }
  return answer;
}

// 获取AI问答结果。
String getAIAnswer(String words) {
  if (words.isEmpty()) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 向AI提问的内容为空");
    return "";
  }
  String host = getApihzHost(GET_API, "");

  if (host.isEmpty()) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: getWeatherHost return host is NULL");
    host = HOST_API;
  }

  String answer = getAIAnswerFromHost(host, "", words);
  return answer;
}
