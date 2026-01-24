#include "WebDisplay.h"

// Web服务器
WebServer server(80);

void merge(JsonVariant dst, JsonVariantConst src)
{
  if (src.is<JsonObjectConst>()) {
    for (JsonPairConst kvp : src.as<JsonObjectConst>()) {
      if (dst[kvp.key()]) {
        merge(dst[kvp.key()], kvp.value());
      }
      else {
        dst[kvp.key()] = kvp.value();
      }
    }
  } else {
    dst.set(src);
  }
}

// 处理数据请求的AJAX接口
void handleDataRequest(WebData *webData) {
  JsonDocument webJson;
  if (NULL == webData) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: webData input param is NULL!");
  }
  if (NULL != webData->pmData) {
    // 创建JSON文档
    JsonDocument pmJson = marshelPmData(webData->pmData);
    merge(webJson.as<JsonVariant>(), pmJson.as<JsonVariant>());
  } else {
    Serial.println("⚠️ WARN: webData->pmData is NULL!");
  }
  if (NULL != webData->weatherDataCity) {
    // 创建JSON文档
    JsonDocument weatherCityJson = marshelWeatherData(webData->weatherDataCity);
    for (JsonPairConst kvp : weatherCityJson.as<JsonObjectConst>()) {
      String key = String(kvp.key().c_str()) + "_city";
      webJson.as<JsonVariant>()[key] = kvp.value();
    }
  } else {
    Serial.println("⚠️ WARN: webData->weatherDataCity is NULL!");
    webJson["temperature_city"] = NULL;
    webJson["humidity_city"] = NULL;
  }
  if (NULL != webData->weatherDataHome) {
    // 创建JSON文档
    JsonDocument weatherHomeJson = marshelWeatherData(webData->weatherDataHome);
    for (JsonPairConst kvp : weatherHomeJson.as<JsonObjectConst>()) {
      String key = String(kvp.key().c_str()) + "_home";
      webJson.as<JsonVariant>()[key] = kvp.value();
    }
  } else {
    Serial.println("⚠ WARN: webData->weatherDataHome is NULL!");
    // webJson["temperature_home"] = NULL;
    // webJson["humidity_home"] = NULL;
  }
  // 设置缓存控制
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");

  // 发送JSON响应
  String jsonResponse;
  serializeJson(webJson, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

// 处理根路径请求（返回HTML文件）
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>传感器数据监控</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { 
      font-family: 'Arial', sans-serif; 
      max-width: 800px; 
      margin: 0 auto; 
      padding: 20px; 
      background-color: #f8f9fa;
    }
    .header {
      text-align: center;
      margin-bottom: 30px;
    }
    .sensor-card {
      background: #ffffff;
      border-radius: 8px;
      padding: 15px;
      margin-bottom: 15px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      transition: all 0.3s ease;
    }
    .sensor-card:hover {
      transform: translateY(-3px);
      box-shadow: 0 4px 8px rgba(0,0,0,0.15);
    }
    .sensor-value {
      font-size: 24px;
      font-weight: bold;
      color: #333;
      text-align: center;
      margin: 10px 0;
    }
    .sensor-label {
      color: #666;
      text-align: center;
      font-weight: 500;
    }
    .status-bar {
      display: flex;
      justify-content: space-between;
      padding: 10px;
      background: #e9ecef;
      border-radius: 4px;
      margin-top: 20px;
    }
    .loading {
      text-align: center;
      color: #666;
      font-style: italic;
    }
    .error {
      color: #dc3545;
      text-align: center;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <div class="header">
    <h1>实时传感器数据监控</h1>
    <p>无需刷新页面，数据自动更新</p>
  </div>

  <div class="sensor-card">
    <div class="sensor-label">城市温度</div>
    <div class="sensor-value" id="temperature_city">--</div>
    <div class="sensor-label">成都(°C)</div>
  </div>
  
  <div class="sensor-card">
    <div class="sensor-label">城市湿度</div>
    <div class="sensor-value" id="humidity_city">--</div>
    <div class="sensor-label">成都(%)</div>
  </div>

  <div class="sensor-card">
    <div class="sensor-label">PM1.0浓度</div>
    <div class="sensor-value" id="pm1_0_atm">--</div>
    <div class="sensor-label">大气颗粒物 (μg/m³)</div>
  </div>
  
  <div class="sensor-card">
    <div class="sensor-label">PM2.5浓度</div>
    <div class="sensor-value" id="pm2_5_atm">--</div>
    <div class="sensor-label">大气颗粒物 (μg/m³)</div>
  </div>
  
  <div class="sensor-card">
    <div class="sensor-label">PM10浓度</div>
    <div class="sensor-value" id="pm10_0_atm">--</div>
    <div class="sensor-label">大气颗粒物 (μg/m³)</div>
  </div>
  
  <div class="sensor-card">
    <div class="sensor-label">室内温度</div>
    <div class="sensor-value" id="temperature_home">--</div>
    <div class="sensor-label">室内(°C)</div>
  </div>
  
  <div class="sensor-card">
    <div class="sensor-label">室内湿度</div>
    <div class="sensor-value" id="humidity_home">--</div>
    <div class="sensor-label">室内(%)</div>
  </div>
  
  <div class="status-bar">
    <div>最后更新: <span id="last-update">--</span></div>
    <div>状态: <span id="status">连接中...</span></div>
  </div>
  
  <script>
    // 定义获取数据的函数
    function fetchData() {
      const startTime = Date.now();
      document.getElementById('status').textContent = '获取数据中...';
      
      fetch('/data')
        .then(response => {
          if (!response.ok) {
            document.getElementById('status').textContent = '获取失败';
            throw new Error('网络错误');
          }
          return response.json();
        })
        .then(data => {
          // 更新状态信息
          const updateTime = new Date().toLocaleTimeString();
          document.getElementById('last-update').textContent = updateTime;
          document.getElementById('status').textContent = '数据已更新';
          // 更新页面元素
          document.getElementById('temperature_city').textContent = data.temperature_city.toFixed(1) + ' °C';
          document.getElementById('humidity_city').textContent = data.humidity_city.toFixed(1) + ' %';
          document.getElementById('pm1_0_atm').textContent = data.pm1_0_atm + ' μg/m³';
          document.getElementById('pm2_5_atm').textContent = data.pm2_5_atm + ' μg/m³';
          document.getElementById('pm10_0_atm').textContent = data.pm10_0_atm + ' μg/m³';
          document.getElementById('temperature_home').textContent = data.temperature_home.toFixed(1) + ' °C';
          document.getElementById('humidity_home').textContent = data.humidity_home.toFixed(1) + ' %';
        })
        .catch(error => {
          console.error('获取数据失败:', error);
          document.getElementById('status').textContent = '连接错误';
        })
        .finally(() => {
          // 确保至少等待1秒再显示状态
          const elapsed = Date.now() - startTime;
          if (elapsed < 1000) {
            setTimeout(() => {
              document.getElementById('status').textContent = '等待下一次更新';
            }, 1000 - elapsed);
          } else {
            document.getElementById('status').textContent = '等待下一次更新';
          }
        });
    }

    // 页面加载完成后开始轮询
    document.addEventListener('DOMContentLoaded', function() {
      fetchData();  // 首次加载
      setInterval(fetchData, 5000);  // 每5秒轮询一次
    });
  </script>
</body>
</html>
  )";
  server.send(200, "text/html", html);
}

// 设置Web服务器路由
void setupWebServer(String domain_name, WebData *webData) {
  // 初始化mDNS服务
  if (!MDNS.begin(domain_name)) {
    Serial.println("mDNS初始化失败");
  }
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, [webData]() {
    handleDataRequest(webData);
  });
  server.begin();
}

// 初始化SPIFFS文件系统
bool initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed: Failed to mount SPIFFS");
    return false;
  }
  Serial.println("SPIFFS mounted successfully");
  return true;
}

void SetupWebServer(void *pvParameters) {
  if (NULL == pvParameters) {
    return;
  }
  WebData *webData = (WebData *)pvParameters;
  while (true) {
    // 处理Web服务器请求
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
  }
  vTaskDelete(NULL);
}