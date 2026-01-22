#include "NetworkController.h"

// WIFI连接
void connect_WIFI(WIFIConfig *wifi_config) {
  if (wifi_config == NULL) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: WIFI配置为空，程序退出!");
    return;
  }
  Serial.printf("\nDEBUG: start to connect to %s", wifi_config->ssid.c_str());
  WiFi.setHostname(wifi_config->hostname.c_str());
  WiFi.begin(wifi_config->ssid.c_str(), wifi_config->password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nDEBUG: WiFi connected");
  Serial.print("DEBUG: ip address: ");
  Serial.println(WiFi.localIP());
}

// [HTTP]连接
String connectHTTP(String url) {
  String result;
  if (url.isEmpty()) {
    return result;
  }

  HTTPClient http;
  Serial.print("DEBUG: [HTTP] begin...\n");
  // configure traged server and url
  //http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
  http.begin(url);

  Serial.print("DEBUG: [HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("DEBUG: [HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      result = http.getString();
    }
  } else {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return result;
}

// [HTTPS]连接
String connectHTTPS(String url, String ca_cert) {
  String result;
  int httpCode;

  // check url
  if (url.isEmpty()) {
    return result;
  }
  // create network client
  NetworkClientSecure *client = new NetworkClientSecure;
  if (client == NULL) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 创建network client失败");
    return result;
  }
  // set ca_cert
  if (ca_cert.isEmpty()) {
    client->setInsecure();
  } else {
    client->setCACert(ca_cert.c_str());
  }

  {
    // Add a scoping block for HTTPClient https to make sure it is destroyed before delete client
    HTTPClient https;
    if (https.begin(*client, url)) {
      Serial.println("DEBUG: [HTTPS] GET...");
      httpCode = https.GET();

      if (httpCode > 0) {
        Serial.printf("DEBUG: [HTTPS] GET... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          result = https.getString();
        } else {
          Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [HTTPS]请求失败: error: %s\n", https.errorToString(httpCode).c_str());
        }
      } else {
        Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [HTTPS]请求失败");
    }
  }

  delete client;
  return result;
}

// 自动匹配[HTTP]或[HTTPS]连接
String connectHTTPandHTTPS(String url, String ca_cert) {
  if (url.isEmpty()) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: connect url is nil");
    return "";
  }

  if (url.startsWith("https://")) {
    Serial.printf("DEBUG: connect to https: %s\n", url.c_str());
    return connectHTTPS(url, ca_cert);
  } else {
    Serial.printf("DEBUG: connect to http: %s\n", url.c_str());
    return connectHTTP(url);
  }
}