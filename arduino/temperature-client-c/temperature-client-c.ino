#include "WiFi.h"
#include "HTTPClient.h"
#include "NetworkClientSecure.h"
#include "ArduinoJson.h"

/****************************************************
 * see: https://www.apihz.cn/api/tqtqyb.html
 ***************************************************/

// ================= WiFi Setttings =================
const char* WIFI_SSID = "OpenWRT_2G";
const char* WIFI_PASSWORD = "183492765";
const char* HOSTNAME = "esp32s3-n16r8";
// ==================================================


// ================= https settings =================
const int HTTPS_PORT = 443;
// ==================================================


// =================== 配置你的信息 ===================
const char* HOST_API = "https://cn.apihz.cn/";
const char* GET_API = "https://api.apihz.cn/getapi.php";
const char* API_ID = "10011341";
const char* API_KEY = "967127bed467835653426373aab82828"; // 从apihz.cn注册获取
const char* CITY = "成都"; // 你想要查询的城市（如beijing, shanghai等）
// ==================================================

// CA证书（apihz.cn的证书，用于安全验证）
const char CA_CERT_APIHZ[] = R"string_literal(
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

void connect_WIFI(const char* ssid, const char* password, const char* hostname) {
  Serial.printf("\nstart to connect to %s\n", ssid);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {}
  Serial.println(WiFi.localIP());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  connect_WIFI(WIFI_SSID, WIFI_PASSWORD, HOSTNAME);
  // update local time
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

char* connectHTTP(char* url) {
  char* result = NULL;
  if (!url) {
    return result;
  }

  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  //http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
  http.begin(String(url));  //HTTP

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      const char* payload = http.getString().c_str();
      Serial.println(payload);
      result = (char *)malloc((strlen(payload) + 1) * sizeof(payload));
      strcpy(result, payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return result;
}

char* connectHTTPS(char* url, const char* ca_cert) {
  char* result = NULL;
  int httpCode;

  if (!url) {
    return result;
  }
  HTTPClient https;
  // 1. 构建API请求
  NetworkClientSecure *client = new NetworkClientSecure;
  if (!client) {
    return result;
  }
  // 2. set CA证书
  if (!ca_cert) {
    client->setInsecure(); // 临时测试用（不推荐生产环境）
  } else {
    client->setCACert(ca_cert); // 生产环境必须
  }
  // 3. 发送GET请求
  if (https.begin(*client, String(url))) {
    Serial.println("[HTTPS] GET...");
    // start connection and send HTTP header
    httpCode = https.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        const char* payload = https.getString().c_str();
        Serial.println(payload);
        result = (char *)malloc((strlen(payload) + 1) * sizeof(payload));
        strcpy(result, payload);
      } else { // httpCode != 200
        Serial.printf("[HTTPS]请求失败: error: %s\n", https.errorToString(httpCode).c_str());
      }
    } else { // httpCode < 0
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
  } else { // https.begin return error
    Serial.println("[HTTPS]请求失败");
  }
  https.end();
  delete client; // 释放内存
  return result;
}

char* connectHTTPandHTTPS(char* url, const char* ca_cert) {
  if (!url) {
    return NULL;
  }
  if (String(url).startsWith("https://")) {
    Serial.printf("connect to https: %s\n", url);
    return connectHTTPS(url, ca_cert);
  } else {
    Serial.printf("connect to http: %s\n", url);
    return connectHTTP(url);
  }
}

char* getWeatherHost(const char* get_api_url, const char* ca_cert) {
  char* host = NULL;

  // 1. 构建API请求
  if (!get_api_url) {
    get_api_url = GET_API;
  }
  char* url = (char *)malloc((strlen(get_api_url) + 1) * sizeof(get_api_url));
  strcpy(url, get_api_url);
  char* connect_result = connectHTTPandHTTPS(url, ca_cert);
  free(url);
  url = NULL;
  if (!connect_result) {
    return host;
  }
  String payload = String(connect_result);
  free(connect_result);
  connect_result = NULL;
  // 解析JSON
  DynamicJsonDocument doc(16); // 16B内存（足够解析JSON）
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("ERROR: JSON解析失败: ");
    Serial.println(error.c_str());
    Serial.printf("connect return is: %s\n", payload);
    Serial.println("请检查API返回格式或增大内存");
    return host;
  }
  // 提取天气数据
  if (doc["code"] == 200) {
    // 提取精确api
    if (doc["api"]) {
      String api = doc["api"];
      host = (char *)malloc((strlen(doc["api"]) + 1) * sizeof(doc["api"]));
      strcpy(host, api.c_str());
    }
  } else { // doc["code"] != 200
    Serial.printf("[HTTPS]请求失败: return code: %d\n", doc["code"]);
  }
  return host;
}

void getWeatherFromHost(const char* host, const char* ca_cert) {
  // 1. 构建API请求
  if (!host) {
    host = HOST_API;
  }
  String url_string = String(host) + String("api/tianqi/tqyb.php?") + String("id=") + String(API_ID) + String("&key=") + String(API_KEY) + String("&sheng=四川&place=成都&day=1&hourtype=1");
  host = NULL;
  char* url = (char *)malloc((strlen(url_string.c_str()) + 1) * sizeof(url_string.c_str()));
  strcpy(url, url_string.c_str());
  // 3. 发送GET请求
  char* connect_result = connectHTTPandHTTPS(url, ca_cert);
  free(url);
  url = NULL;
  if (!connect_result) {
    return;
  }
  String payload = String(connect_result);
  // 解析JSON
  DynamicJsonDocument doc(1024); // 1KB内存（足够解析大JSON）
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON解析失败: ");
    Serial.println(error.c_str());
    Serial.printf("return is: %s\n", payload);
    Serial.println("请检查API返回格式或增大内存");
    return;
  }
  // 提取天气数据
  if (doc["code"] == 200) {
    // 提取精确温度
    if (doc["nowinfo"] && doc["nowinfo"]["temperature"]) {
      String temperature = doc["nowinfo"]["temperature"];
      Serial.printf("temperature is %s\n", temperature);
    }
  } else {
    Serial.printf("[HTTPS]请求失败: return code: %d\n", doc["code"]);
  }
}

void getWeather() {
  char* host = getWeatherHost(GET_API, NULL);
  if (!host) {
    Serial.println("ERROR: getWeatherHost return host is NULL");
    host = (char *)malloc((strlen(HOST_API) + 1) * sizeof(HOST_API));
    strcpy(host, HOST_API);
  }
  getWeatherFromHost(host, NULL);
  free(host);
  host = NULL;
}

void loop() {
  getWeather();
  delay(600000); // 10分钟
  // 每10分钟获取一次天气
  Serial.println("\n=== 下次天气更新: 10分钟后 ===");
}
