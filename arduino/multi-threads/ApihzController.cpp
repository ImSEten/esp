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
void getWeatherFromHost(String host, String ca_cert, String city) {
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

  if (doc["code"] == 200 && doc["nowinfo"] && !doc["nowinfo"]["temperature"].isNull()) {
    String temperature = doc["nowinfo"]["temperature"].as<String>();
    Serial.printf("⚡ INFO: temperature is %s\n", temperature.c_str());
  } else {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [HTTPS]请求失败: return code: %d\n", doc["code"]);
  }
}

// 获取天气信息
void getWeather(String city) {
  String host = getApihzHost(GET_API, "");

  if (host.isEmpty()) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: getWeatherHost return host is NULL");
    host = HOST_API;
  }

  getWeatherFromHost(host, "", city);
}

// 从apihz中获取AI问答结果。
String getAIAnswerFromHost(String host, String ca_cert, String words) {
  if (host.isEmpty()) {
    host = HOST_API;
  }

  String url_string = host + "api/ai/wxtiny.php" + "?id=" + API_ID + "&key=" + API_KEY + "&words=" + words;

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

  if (doc["code"] == 200 && !doc["msg"].isNull()) {
    String answer = doc["msg"].as<String>();
    Serial.printf("INFO: answer is %s\n", answer.c_str());
    return answer;
  } else {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [HTTPS]请求失败: return code: %d\n", doc["code"]);
    return "";
  }
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
