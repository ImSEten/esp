#ifndef NETWORK_CONTROLLER_H
#define NETWORK_CONTROLLER_H

// ================= HTTPS Settings =================
const int HTTPS_PORT = 443;
// ==================================================

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// 定义WIFI配置
typedef struct {
  String hostname;          // 本设备在网络中显示的名称
  String ssid;              // 要连接的wifi名称
  String password;          // 要连接的wifi密码
  bool auto_reconnect;      // 如果为true，当wifi断开时会自动重连
  SemaphoreHandle_t mutex;  // 锁，获取本结构体中任何成员变量都需等此锁
} WIFIConfig;

void connect_WIFI(WIFIConfig *wifi_config);
String connectHTTP(String url);
String connectHTTPS(String url, String ca_cert);
String connectHTTPandHTTPS(String url, String ca_cert);

#endif