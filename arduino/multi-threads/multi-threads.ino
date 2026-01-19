#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include "LightController.h"
#include "AiriqController.h"


/*****************************************************************************************
 ***                                   ESP32S3-N16R8                                   ***
 *****************************************************************************************
 *                                                                                       *
 *                Project Name : Esp32-Assistant                                         *
 *                                                                                       *
 *                   File Name : multi-threads.ino                                       *
 *                                                                                       *
 *                  Programmer : ImSEten                                                 *
 *                                                                                       *
 *                Started Date : 2026/01/01                                              *
 *                                                                                       *
 *                 Last Update : 2026/01/13                                              *
 *                                                                                       *
 *---------------------------------------------------------------------------------------*
 * Functions:                                                                            *
 *   connect_WIFI -- Connect to the WIFI                                                 *
 *   connectHTTP -- Connect to the network via HTTP                                      *
 *   connectHTTPS -- Connect to the network via HTTPS                                    *
 *   connectHTTPandHTTPS -- Automatically choose HTTP/HTTPS to the network               *
 *   getApihzHost -- Get the dynamic IP address of apihz.com                             *
 *   getWeatherFromHost -- Get weather info through the dynamic IP of apihz              *
 *   getWeather -- Get weather info from apihz                                           *
 *   getAIAnswerFromHost -- Get AI answer through the dynamic IP of apihz                *
 *   getAIAnswer -- Get AI answer from apihz                                             *
 *   Connect_WIFI -- multi-thread call connect_WIFI function                             *
 *   GetWeather -- multi-thread call getWeather function                                 *
 *   GetAIAnswer -- multi-thread call getAIAnswer function                               *
 *   ReadFromSerial -- multi-thread read data from Serial input                          *
 *   GetAIQuestions -- multi-thread read data from channel Serial and Other              *
 *   setLedColor -- set led RGB color                                                    *
 *   setLedBrightness -- set led brightness                                              *
 *   lazyOnLed -- light on led slowly                                                    *
 *   lazyOffLed -- light off led slowly                                                  *
 *   SetupLlight -- init L-light, slowly light up and then slowly light off              *
 *   LlightWifi -- light up 3s when wifi connected, blink 2times/1s when wifi lost       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/*
  TODO:
    2、当空气质量为优时，亮绿灯；当空气质量为一般时亮黄灯，当空气质量为差时，亮红灯；当空气质量极差时，红灯闪烁。
    2、读取MQ-4和MQ-135

  Done:
    1、当WIFI连接成功后，L灯亮起3s后熄灭（白灯）
    2、WIFI连接中时，L灯快速闪烁（白灯）
    3、通过apihz获取天气信息
    4、通过apihz得到AI能力
    3、读取PMS9103M空气质量传感器数据
 */


// ================ Serial Settings =================
const uint MAX_SERIAL_INPUT = 1024;  // serial输入最大值
const uint SERIAL_PORT = 115200;     // serial 端口
// ==================================================


// ================= WiFi Settings ==================
const String WIFI_SSID = "OpenWRT_2G";
const String WIFI_PASSWORD = "183492765";
const String HOSTNAME = "esp32s3-n16r8";
// ==================================================


// ================= HTTPS Settings =================
const int HTTPS_PORT = 443;
// ==================================================


/****************************************************
 * see: https://www.apihz.cn/api/tqtqyb.html
 ***************************************************/


// =================== 配置你的信息 ===================
const String HOST_API = "https://cn.apihz.cn/";             // apihz官网，在获取不到动态连接地址的时候使用该地址访问api接口。
const String GET_API = "https://api.apihz.cn/getapi.php";   // api盒子获取动态连接地址。
const String API_ID = "10011341";                           // 从apihz.cn注册获取
const String API_KEY = "967127bed467835653426373aab82828";  // 从apihz.cn注册获取
const String CITY = "成都";                                 // 你想要查询的城市（如beijing, shanghai等）
const uint WEATHER_DELAY_TIME = 600000;                     // 延迟时间
const uint MAX_AI_WORDS = MAX_SERIAL_INPUT;                 // AI提问最大字符长度
// ==================================================


const uint MUTEX_WAIT = 100;  // mutex等待时间，100ms


// CA证书（apihz.cn的证书，用于安全验证）
const String CA_CERT_APIHZ = R"string_literal(
-----BEGIN CERTIFICATE-----
MIIGJzCCBQ+gAwIBAgIQfgSnzHPKkAFV5wMJd2vaZDANBgkqhkiG9w0BAQsFADCB
jzELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G
A1UEBxMHU2FsZm9yZDEYMBYGA1UEChMPU2VjdGlnbyBMaW1pdGVkMTcwNQYDVQQD
Ey5TZWN0aWdvIFJTQSBEb21haW4gVmFsaWRhdGlvbiBTZWN1cmUgU2VydmVyIENB
MB4XDTI1MDQyMjAwMDAwMFoXDTI2MDUyMzIzNTk1OVowFTETMBEGA1UEAwwKKi5h
cGloei5jbjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALe9mF0mS8cM
OKM1U/n1cA9X18pKfxLC73liyMJbAqzqU/dLmJprUiQv9pUzitZ/5fDexlzYkAyn
Pfu9mU6PD3q7mT06cLJ448u9CnfZeg0F9TNe6tjb+R4KrazIN/vXCPCYn8b8d/b4
9O06LrlxTMRSy1lxxYvf6wCMpGEgZD5/BpFOGX/zNCuAjsPCmINU4Vrz7lWG/kqM
8fgYrZWQyoW3sSMOQuTIDOaEGQizH5E9duMCj5bXgR9ZBaVJtd/yGuzo7GKHABoa
aqxdYfZF4zGzHArwyOhzIqcE1k8DVBX1nA9jM6Rx7v63xetqRya84/C143zSrfaz
LGLB91prHCcCAwEAAaOCAvYwggLyMB8GA1UdIwQYMBaAFI2MXsRUrYrhd+mb+ZsF
4bgBjWHhMB0GA1UdDgQWBBTcVCVo/XoZXBlczuA2TztsqYlC1zAOBgNVHQ8BAf8E
BAMCBaAwDAYDVR0TAQH/BAIwADAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUH
AwIwSQYDVR0gBEIwQDA0BgsrBgEEAbIxAQICBzAlMCMGCCsGAQUFBwIBFhdodHRw
czovL3NlY3RpZ28uY29tL0NQUzAIBgZngQwBAgEwgYQGCCsGAQUFBwEBBHgwdjBP
BggrBgEFBQcwAoZDaHR0cDovL2NydC5zZWN0aWdvLmNvbS9TZWN0aWdvUlNBRG9t
YWluVmFsaWRhdGlvblNlY3VyZVNlcnZlckNBLmNydDAjBggrBgEFBQcwAYYXaHR0
cDovL29jc3Auc2VjdGlnby5jb20wHwYDVR0RBBgwFoIKKi5hcGloei5jboIIYXBp
aHouY24wggF+BgorBgEEAdZ5AgQCBIIBbgSCAWoBaAB1AJaXZL9VWJet90OHaDcI
Qnfp8DrV9qTzNm5GpD8PyqnGAAABllv1WxQAAAQDAEYwRAIgfZ0jEf90awhMDRw9
X7emVlcE0/FPX/jk0d38xH1onW8CIAt8tpNJ+ovg0Y9mrbRmp6r3S8E/3rM5bZQf
IkobWL86AHYAGYbUxyiqb/66A294Kk0BkarOLXIxD67OXXBBLSVMx9QAAAGWW/Va
8wAABAMARzBFAiB638KE/nmuqV9D86EqNApWmKOYN2NlgVAuMXmOseblLgIhAIAr
cK1sMqPGt/qCpeDJcY/VEiOFSnFpLy56q6N4uWxrAHcADleUvPOuqT4zGyyZB7P3
kN+bwj1xMiXdIaklrGHFTiEAAAGWW/Va9AAABAMASDBGAiEArGXY6k9QZVRgYgNg
SMuW+maRzil7rM0R/TeZDT4viJECIQCNcZS2obce6zbc664BX7EwD5MPCa6RdWbR
2LhFx7F/EzANBgkqhkiG9w0BAQsFAAOCAQEAM6kj1dPPJE4j6N/xyr7u4uOIupoY
5XRy12q3pCUJroEXrREf+5yFLTFMrDkz0tKOs5Xhmk7QRG3uEWGZ62rcsxUfpqyd
71k0+lwxVc+XS5YZ7zQU7KEPS5r6fnQ599TV5yKuqxikxkSM6xHc4pTR826KPDBA
3/iCJkrV7kCpOY8Hn7avxj2OBNCydPbncQ6nTUxgu2c24cQFX+U0oNEPpJ+ilJdG
Nb+lwXktoTBc86moSX6WLoI0d8JX4Yp6WeTpvxizISuE7ajFVTEapKWqGCkxqm1E
2Ex76LF5iYjchN3CYLRterhjIFzxVC1gtrHoq6kNo+c8TlbKY6FBGmW6cA==
-----END CERTIFICATE-----
)string_literal";

#define NUM_L_LEDS 8


// 定义WIFI配置
typedef struct {
  String hostname;          // 本设备在网络中显示的名称
  String ssid;              // 要连接的wifi名称
  String password;          // 要连接的wifi密码
  bool auto_reconnect;      // 如果为true，当wifi断开时会自动重连
  SemaphoreHandle_t mutex;  // 锁，获取本结构体中任何成员变量都需等此锁
} WIFIConfig;

// 定义天气查询配置
typedef struct {
  String city;              // 查询天气的城市
  uint delay_time;          // 间隔多少ms查询一次天气
  SemaphoreHandle_t mutex;  // 锁，获取本结构体中任何成员变量都需等此锁
} WeatherConfig;


// 定义AI查询配置
typedef struct {
  QueueHandle_t serial_queue;  // 从serial中输入
  QueueHandle_t words_queue;   // 输出给AI
} AIConfig;

// =============Llight初始化==============
Light L_Light;
// ======================================

// =============队列句柄初始化=============
QueueHandle_t Serial_Queue = NULL;
QueueHandle_t Ai_Words_Queue = NULL;
// ======================================

// =============init configs=============
WIFIConfig Wifi_Config;
WeatherConfig Weather_Config;
AIConfig Ai_Config;
PMData PmData;
// ======================================


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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *                         多线程函数                            *
 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

void LlightWifi(void *pvParameters) {
  uint connected_brightness = 255;
  uint connecting_brightness = 120;
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: LlightWifi入参为NULL");
    return;
  }
  Light *light = (Light *)pvParameters;
  wl_status_t pre_wifi_status = WiFi.status();
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT * 5));  // 0.5s
    if (WiFi.status() == WL_CONNECTED) {
      if (WiFi.status() == pre_wifi_status) {  // 状态未发生变化，重新轮询
        pre_wifi_status = WiFi.status();
        continue;
      } else {  // WiFi状态变化，亮灯3s后熄灭
        pre_wifi_status = WiFi.status();
        if (xSemaphoreTake(light->mutex, portMAX_DELAY)) {  // 获取锁
          light->brightness = connected_brightness;
          setLedBrightness(light);                     // light up
          vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT * 30));  // 3s
          light->brightness = 0;
          setLedBrightness(light);       // light off
          xSemaphoreGive(light->mutex);  // 释放锁
        } else {
          Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: LlightWifi无法获取light锁");
        }
      }
    } else {
      pre_wifi_status = WiFi.status();
      if (xSemaphoreTake(light->mutex, portMAX_DELAY)) {  // 获取锁
        light->brightness = 0;
        setLedBrightness(light);                    // light off
        vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT * 5));  // 1s
        light->brightness = connecting_brightness;
        setLedBrightness(light);       // light up
        xSemaphoreGive(light->mutex);  // 释放锁
      } else {
        Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: LlightWifi无法获取light锁");
      }
    }
  }
  free(light->leds);
  light->leds = NULL;
  delete light;
}

void GetAirIq(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAirIq入参为NULL");
    return;
  }
  PMData *pmData = (PMData *)pvParameters;
  while (true) {
    if (NULL == pmData) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAirIq的pmData为NULL");
      vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT));
      continue;
    }
    // 读取PMS9103M传感器数据
    if (readPMS9103MData(pmData)) {
      // 打印读取到的数据
      printAirIqData(pmData);
    }
    vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT * 50)); // 5s读取一次
  }
}

// 在多线程中运行该函数，连接WIFI，并设置自动连接
void Connect_WIFI(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: Connect_WIFI入参为NULL");
    return;
  }
  WIFIConfig *wifi_config = (WIFIConfig *)pvParameters;                                   // 类型转换
  if (wifi_config->mutex != NULL && xSemaphoreTake(wifi_config->mutex, portMAX_DELAY)) {  // portMAX_DELAY表示永久阻塞等锁，占用cpu资源。
    WiFi.setAutoReconnect(wifi_config->auto_reconnect);
    xSemaphoreGive(wifi_config->mutex);  // 释放锁
  }

  connect_WIFI(wifi_config);
}

// 在多线程中运行该函数，获取天气信息
void GetWeather(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetWeather入参为NULL");
    return;
  }
  WeatherConfig *weather_config = (WeatherConfig *)pvParameters;  // 类型转换
  uint delay_time;
  while (true) {
    if (weather_config == NULL) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetWeather的weather_config为NULL");
      vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT));
      continue;
    }
    if (weather_config->mutex != NULL && xSemaphoreTake(weather_config->mutex, MUTEX_WAIT)) {  // MUTEX_WAIT最大等锁时间，超过MUTEX_WAIT则返回NULL
      delay_time = weather_config->delay_time;
      getWeather(weather_config->city);
      xSemaphoreGive(weather_config->mutex);  // 释放锁
      vTaskDelay(pdMS_TO_TICKS(delay_time));
    } else {
      Serial.printf("⚠️ WARN: 获取天气锁超时！下次获取天气信息将在%ds后\n", delay_time / 10 / 1000);
      vTaskDelay(pdMS_TO_TICKS(delay_time / 10));
    }
  }
  vTaskDelete(NULL);
}

// 在多线程中运行该函数，获取AI问答信息。
void GetAIAnswer(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAIAnswer入参为NULL");
    return;
  }
  AIConfig *ai_config = (AIConfig *)pvParameters;  // 类型转换
  char words_char[MAX_AI_WORDS];
  while (true) {
    if (ai_config == NULL) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAIAnswer的ai_config为NULL");
      vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT));
      continue;
    }
    if (ai_config->words_queue != NULL && xQueueReceive(ai_config->words_queue, words_char, portMAX_DELAY) == pdPASS) {  // MUTEX_WAIT最大等锁时间，超过MUTEX_WAIT则返回NULL
      String words = String(words_char);
      getAIAnswer(words);
    } else {  // 未获取到锁
      Serial.println("⚠️ WARN: 无法从队列中获取值！重新等待队列输入");
    }
    vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT));
  }
  vTaskDelete(NULL);
}

// 在多线程中运行该函数，从serial中获取输入。
void ReadFromSerial(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: ReadFromSerial入参为NULL");
    return;
  }
  char buffer[MAX_SERIAL_INPUT];
  QueueHandle_t *serial_queue = (QueueHandle_t *)pvParameters;
  while (true) {
    if (serial_queue == NULL) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: ReadFromSerial的serial_queue为NULL");
      vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT));
      continue;
    }
    if (Serial.available() > 0) {
      String message = Serial.readStringUntil('\n');
      message.toCharArray(buffer, MAX_SERIAL_INPUT);
      // 发送字符数组（安全副本）
      if (xQueueSend(*serial_queue, buffer, portMAX_DELAY) != pdPASS) {
        Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 向serial_queue队列发送数据失败!");
      } else {
        Serial.printf("DEBUG: 向serial_queue发送数据: %s\n", buffer);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT));
  }
  vTaskDelete(NULL);
}

// 在多线程中运行该函数，获取对AI进行提问的问题。即获取输入给AI的input。
void GetAIQuestions(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAIAnswer入参为NULL");
    return;
  }
  AIConfig *ai_config = (AIConfig *)pvParameters;  // 类型转换
  char serial_char[MAX_SERIAL_INPUT];
  char words_char[MAX_AI_WORDS];
  while (true) {
    if (ai_config == NULL) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAIQuestions的ai_config为NULL");
      vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT));
      continue;
    }
    Serial.println("DEBUG: waiting for serail_queue input");
    if (ai_config->serial_queue != NULL && xQueueReceive(ai_config->serial_queue, serial_char, portMAX_DELAY) == pdPASS) {  // MUTEX_WAIT最大等锁时间，超过MUTEX_WAIT则返回NULL
      String words = String(serial_char);
      words.toCharArray(words_char, MAX_AI_WORDS);
      if (ai_config->words_queue != NULL && xQueueSend(ai_config->words_queue, words_char, portMAX_DELAY) != pdPASS) {
        Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 向words_queue队列发送数据失败!");
      } else {
        Serial.printf("DEBUG: 向words_queue发送数据: %s\n", words_char);
      }
    } else {  // 未从serial_queue中获取到数据
      Serial.println("⚠️ WARN: 无法从serial_queue队列中获取值！重新等待队列输入");
    }
    vTaskDelay(pdMS_TO_TICKS(MUTEX_WAIT));
  }
  vTaskDelete(NULL);
}

// 主函数
void setup() {
  // start usb serial
  Serial.begin(SERIAL_PORT);
  Serial0.begin(9600);
  // create mutex;
  SemaphoreHandle_t wifi_mutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t weather_mutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t ai_mutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t l_light_mutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t airiq_mutex = xSemaphoreCreateMutex();
  // create queue
  Ai_Words_Queue = xQueueCreate(10, sizeof(char) * MAX_AI_WORDS);
  Serial_Queue = xQueueCreate(10, sizeof(char) * MAX_AI_WORDS);
  // check mutex and queue created
  if (NULL == wifi_mutex || NULL == weather_mutex || NULL == ai_mutex || NULL == l_light_mutex || NULL == airiq_mutex) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 无法创建锁，程序退出!");
    return;
  }

  CRGB *l_leds = (CRGB *)malloc(sizeof(CRGB) * NUM_L_LEDS);

  // L-light config
  L_Light = {
    num_leds: NUM_L_LEDS,
    leds: l_leds,
    brightness: 255,
    mutex: l_light_mutex,
  };

  // connect wifi config
  Wifi_Config = {
    hostname: HOSTNAME,
    ssid: WIFI_SSID,
    password: WIFI_PASSWORD,
    auto_reconnect: true,
    mutex: wifi_mutex,
  };

  // get weather config
  Weather_Config = {
    city: CITY,
    delay_time: WEATHER_DELAY_TIME,
    mutex: weather_mutex,
  };

  // Ai answer config
  Ai_Config = {
    serial_queue: Serial_Queue,
    words_queue: Ai_Words_Queue,
  };

  // AirIq data
  PmData.mutex = airiq_mutex;

  // 任务句柄（可选）
  TaskHandle_t taskLlightWifiHandle = NULL;
  TaskHandle_t taskGetAirIqHandle = NULL;
  TaskHandle_t taskReadFromSerialHandle = NULL;
  TaskHandle_t taskGetWeatherHandle = NULL;
  TaskHandle_t taskGetAIQuestionsHandle = NULL;
  TaskHandle_t taskGetAIAnswerHandle = NULL;

  // L-light wifi
  FastLED.addLeds<WS2812, PIN_RGB_LED, RGB>(L_Light.leds, L_Light.num_leds);
  setupLight(&L_Light, COLOR_WHITE);
  // -----------------------灯光控制-----------------------
  xTaskCreatePinnedToCore(
    LlightWifi, "TaskLlightWifi", 8192, (void *)&L_Light, 5, &taskLlightWifiHandle, 0);  // tskNO_AFFINITY表示不限制core
  //-----------------------传感器-----------------------
  xTaskCreatePinnedToCore(
    GetAirIq, "TaskGetAirIq", 8192, (void *)&PmData, 5, &taskGetAirIqHandle, 0);  // tskNO_AFFINITY表示不限制core
  // -----------------------Network-----------------------
  // Connect wifi
  Connect_WIFI((void *)&Wifi_Config);
  // update local time
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  // read from serial
  xTaskCreatePinnedToCore(
    ReadFromSerial, "TaskReadFromSerial", 8192, (void *)&Serial_Queue, 5, &taskReadFromSerialHandle, 1);  // tskNO_AFFINITY表示不限制core
  // get weather
  xTaskCreatePinnedToCore(
    GetWeather, "TaskGetWeather", 8192, (void *)&Weather_Config, 4, &taskGetWeatherHandle, 1);  // tskNO_AFFINITY表示不限制core
  // get AIQuestions
  xTaskCreatePinnedToCore(
    GetAIQuestions, "TaskGetAIQuestions", 8192, (void *)&Ai_Config, 3, &taskGetAIQuestionsHandle, 1);  // tskNO_AFFINITY表示不限制core
  // get AIAnswer
  xTaskCreatePinnedToCore(
    GetAIAnswer, "TaskGetAIAnswer", 8192, (void *)&Ai_Config, 3, &taskGetAIAnswerHandle, 1);  // tskNO_AFFINITY表示不限制core
}

void loop() {
  // This loop is not executed by FreeRTOS
  // Instead, tasks are scheduled by the FreeRTOS scheduler
}