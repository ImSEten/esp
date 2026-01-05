#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <ArduinoJson.h>

/****************************************************
 * see: https://www.apihz.cn/api/tqtqyb.html
 ***************************************************/

// ================= WiFi Settings =================
const String WIFI_SSID = "OpenWRT_2G";
const String WIFI_PASSWORD = "183492765";
const String HOSTNAME = "esp32s3-n16r8";
// ==================================================


// ================= HTTPS Settings =================
const int HTTPS_PORT = 443;
// ==================================================


// =================== 配置你的信息 ===================
const String HOST_API = "https://cn.apihz.cn/";
const String GET_API = "https://api.apihz.cn/getapi.php";
const String API_ID = "10011341";
const String API_KEY = "967127bed467835653426373aab82828";  // 从apihz.cn注册获取
const String CITY = "成都";                                 // 你想要查询的城市（如beijing, shanghai等）
// ==================================================


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

void connect_WIFI(const String hostname, const String ssid, const String password) {
  Serial.printf("\nstart to connect to %s", ssid.c_str());
  WiFi.setHostname(hostname.c_str());
  WiFi.begin(WIFI_SSID.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("ip address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  connect_WIFI(HOSTNAME, WIFI_SSID, WIFI_PASSWORD);
  // update local time
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

String connectHTTP(String url) {
  String result;
  if (url.isEmpty()) {
    return result;
  }

  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  //http.begin("https://www.howsmyssl.com/a/check", ca); //HTTPS
  http.begin(url);

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      result = http.getString();
      Serial.println(result.c_str());
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return result;
}

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

String connectHTTPandHTTPS(String url, String ca_cert) {
  if (url.isEmpty()) {
    return "";
  }

  if (url.startsWith("https://")) {
    Serial.printf("connect to https: %s\n", url.c_str());
    return connectHTTPS(url, ca_cert);
  } else {
    Serial.printf("connect to http: %s\n", url.c_str());
    return connectHTTP(url);
  }
}

String getApihzHost(String get_api_url, String ca_cert) {
  String host;
  if (get_api_url.isEmpty()) {
    get_api_url = GET_API;
  }
  String connect_result = connectHTTPandHTTPS(get_api_url, ca_cert);

  if (connect_result.isEmpty()) {
    return host;
  }

  DynamicJsonDocument doc(16);  // 16B
  DeserializationError error = deserializeJson(doc, connect_result);

  if (error) {
    Serial.print("ERROR: JSON解析失败: ");
    Serial.println(error.c_str());
    Serial.printf("connect return is: %s\n", connect_result.c_str());
    Serial.println("请检查API返回格式或增大内存");
    return host;
  }

  if (doc["code"] == 200 && !doc["api"].isNull()) {
    host = doc["api"].as<String>();
  } else {
    Serial.printf("[HTTPS]请求失败: return code: %d\n", doc["code"]);
  }

  return host;
}

void getWeatherFromHost(String host, String ca_cert, String city) {
  if (host.isEmpty()) {
    host = HOST_API;
  }

  String url_string = host + "api/tianqi/tqyb.php" + "?id=" + API_ID + "&key=" + API_KEY + "&sheng=四川&place=" + city + "&day=1&hourtype=1";

  String connect_result = connectHTTPandHTTPS(url_string, ca_cert);

  if (connect_result.isEmpty()) {
    return;
  }

  DynamicJsonDocument doc(1024);  // 1KB
  DeserializationError error = deserializeJson(doc, connect_result);

  if (error) {
    Serial.print("JSON解析失败: ");
    Serial.println(error.c_str());
    Serial.printf("return is: %s\n", connect_result.c_str());
    Serial.println("请检查API返回格式或增大内存");
    return;
  }

  if (doc["code"] == 200 && doc["nowinfo"] && !doc["nowinfo"]["temperature"].isNull()) {
    String temperature = doc["nowinfo"]["temperature"].as<String>();
    Serial.printf("temperature is %s\n", temperature.c_str());
  } else {
    Serial.printf("[HTTPS]请求失败: return code: %d\n", doc["code"]);
  }
}

void getWeather(String city) {
  String host = getApihzHost(GET_API, "");

  if (host.isEmpty()) {
    Serial.println("ERROR: getWeatherHost return host is NULL");
    host = HOST_API;
  }

  getWeatherFromHost(host, "", city);
}

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
    Serial.print("JSON解析失败: ");
    Serial.println(error.c_str());
    Serial.printf("return is: %s\n", connect_result.c_str());
    Serial.println("请检查API返回格式或增大内存");
    return "";
  }

  if (doc["code"] == 200 && !doc["msg"].isNull()) {
    String answer = doc["msg"].as<String>();
    Serial.printf("answer is %s\n", answer.c_str());
    return answer;
  } else {
    Serial.printf("[HTTPS]请求失败: return code: %d\n", doc["code"]);
    return "";
  }
}

String getAIAnswer(String words) {
  String host = getApihzHost(GET_API, "");

  if (host.isEmpty()) {
    Serial.println("ERROR: getWeatherHost return host is NULL");
    host = HOST_API;
  }

  String answer = getAIAnswerFromHost(host, "", words);
  return answer;
}

void loop() {
  getWeather(CITY);
  //String answer = getAIAnswer("请讲解一下rust语言中map与map_err的区别");
  //Serial.printf("answer is %s\n", answer.c_str());
  delay(600000);  // 10分钟
  Serial.println("\n=== 下次天气更新: 10分钟后 ===");
}