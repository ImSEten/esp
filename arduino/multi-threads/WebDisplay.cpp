#include "WebDisplay.h"

// Web服务器
WebServer server(80);

// 处理数据请求的AJAX接口
void handleDataRequest(PMData *pmData) {
  // 创建JSON文档
  DynamicJsonDocument pmJson = marshelPmData(pmData);
  pmJson["temperature_home"] = 9999;
  pmJson["humidity_home"] = 9999;

  // 设置缓存控制
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");

  // 发送JSON响应
  String jsonResponse;
  serializeJson(pmJson, jsonResponse);
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
    <div class="sensor-label">温度</div>
    <div class="sensor-value" id="temperature_home">--</div>
    <div class="sensor-label">(°C)</div>
  </div>
  
  <div class="sensor-card">
    <div class="sensor-label">湿度</div>
    <div class="sensor-value" id="humidity_home">--</div>
    <div class="sensor-label">(%)</div>
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
          // 更新页面元素
          document.getElementById('pm1_0_atm').textContent = data.pm1_0_atm + ' μg/m³';
          document.getElementById('pm2_5_atm').textContent = data.pm2_5_atm + ' μg/m³';
          document.getElementById('pm10_0_atm').textContent = data.pm10_0_atm + ' μg/m³';
          document.getElementById('temperature_home').textContent = data.temperature_home.toFixed(1) + ' °C';
          document.getElementById('humidity_home').textContent = data.humidity_home.toFixed(1) + ' %';
          
          // 更新状态信息
          const updateTime = new Date().toLocaleTimeString();
          document.getElementById('last-update').textContent = updateTime;
          document.getElementById('status').textContent = '数据已更新';
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
      setInterval(fetchData, 2000);  // 每2秒轮询一次
    });
  </script>
</body>
</html>
  )";
  server.send(200, "text/html", html);
}

// 设置Web服务器路由
void setupWebServer(PMData *pmData) {
  // 初始化mDNS服务
  if (!MDNS.begin("esp32-n16r8")) {
    Serial.println("mDNS初始化失败");
  }
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, [pmData]() {
    handleDataRequest(pmData);
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
  PMData *pmData = (PMData *)pvParameters;
  while (true) {
    // 处理Web服务器请求
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
  }
  vTaskDelete(NULL);
}