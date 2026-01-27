#ifndef APIHZ_CONTROLLER_H
#define APIHZ_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "Common.h"
#include "NetworkController.h"
#include "WeatherController.h"

/****************************************************
 * see: https://www.apihz.cn/api/tqtqyb.html
 ***************************************************/


const String HOST_API = "https://cn.apihz.cn/";             // apihz官网，在获取不到动态连接地址的时候使用该地址访问api接口。
const String GET_API = "https://api.apihz.cn/getapi.php";   // api盒子获取动态连接地址。
const String API_ID = "10011341";                           // 从apihz.cn注册获取
const String API_KEY = "967127bed467835653426373aab82828";  // 从apihz.cn注册获取


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


// 定义天气查询配置
typedef struct {
  String city;              // 查询天气的城市
  uint delay_time;          // 间隔多少ms查询一次天气
  SemaphoreHandle_t mutex;  // 锁，获取本结构体中任何成员变量都需等此锁
} WeatherConfig;


// 定义AI查询配置
typedef struct {
  QueueHandle_t serial_queue;     // 从serial中输入
  QueueHandle_t web_input_queue;  // 从web对话框中输入
  QueueHandle_t words_queue;      // 输出给AI
} AIConfig;

String getApihzHost(String get_api_url, String ca_cert);
void getWeatherFromHost(String host, String ca_cert, String city, WeatherData *weatherData);
void getWeather(String city, WeatherData *weatherData);
String getAIAnswerFromHost(String host, String ca_cert, String words);
String getAIAnswer(String words);

#endif