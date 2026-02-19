#include "WebDisplay.h"

// Web服务器
WebServer server(80);
DNSServer dnsServer;

void merge(JsonVariant dst, JsonVariantConst src) {
  if (src.is<JsonObjectConst>()) {
    for (JsonPairConst kvp : src.as<JsonObjectConst>()) {
      if (dst[kvp.key()]) {
        merge(dst[kvp.key()], kvp.value());
      } else {
        dst[kvp.key()] = kvp.value();
      }
    }
  } else {
    dst.set(src);
  }
}

// 处理数据请求的AJAX接口
void handleDataRequest(WebData *webData) {
  if (NULL == webData) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: webData input param is NULL!");
  }
  JsonDocument webJson;
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

// 处理AI请求（独立路由，不影响原有刷新）
void handleAIRequest(WebData *webData) {
  if (NULL == webData) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: webData input param is NULL!");
  }
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No data received");
    return;
  }

  // 获取原始 JSON 字符串
  String json = server.arg("plain");
  Serial.println("[AI] Raw JSON: " + json);  // 保持原始日志用于调试

  // === 关键修复：使用 ArduinoJson 解析 JSON ===
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.printf("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: [AI] JSON 解析错误: %s\n", error.c_str());
    server.send(500, "text/plain", "Invalid JSON");
    return;
  }

  // 从 JSON 中提取 prompt
  if (!doc.containsKey("prompt")) {
    server.send(400, "text/plain", "Missing 'prompt' in JSON");
    return;
  }

  String prompt = doc["prompt"].as<String>();
  Serial.println("Debug: [AI] User: " + prompt);  // 现在打印的是 "你好，你是谁？"
  char buffer[MAX_SERIAL_INPUT];
  prompt.toCharArray(buffer, MAX_SERIAL_INPUT);
  if (NULL != webData && NULL != webData->web_input_queue) {
    // 发送字符数组（安全副本）
    if (xQueueSend(*(webData->web_input_queue), buffer, DELAY_TIME) != pdPASS) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 向web_input_queue队列发送数据失败!");
    } else {
      Serial.printf("DEBUG: 向web_input_queue发送数据: %s\n", buffer);
    }
  }

  char result_char[MAX_SERIAL_INPUT];
  String aiResponse = "";
  if (NULL != webData && NULL != webData->web_output_queue) {
    // 发送字符数组（安全副本）
    if (webData->web_output_queue != NULL && xQueueReceive(*(webData->web_output_queue), result_char, portMAX_DELAY) == pdPASS) {  // MUTEX_WAIT最大等锁时间，超过MUTEX_WAIT则返回NULL
      Serial.printf("DEBUG: 从web_output_queue队列获取输入: %s\n", result_char);
      aiResponse = String(result_char);
    } else {
      Serial.println("⚠️ WARN: 无法从web_output_queue队列中获取值！重新等待队列输入");
      // 模拟 AI 响应
      aiResponse = "你好！我是 ESP32-S3 的 AI 助手，可以回答简单问题。";
    }
  }

  // 构建响应 JSON
  JsonDocument responseJson;
  responseJson["response"] = aiResponse;
  String jsonResponse;
  serializeJson(responseJson, jsonResponse);

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
    <div class="sensor-label">天气预报温度</div>
    <div class="sensor-value" id="temperature_city">--</div>
    <div class="sensor-label">成都(°C)</div>
  </div>
  
  <div class="sensor-card">
    <div class="sensor-label">天气预报湿度</div>
    <div class="sensor-value" id="humidity_city">--</div>
    <div class="sensor-label">成都(%)</div>
  </div>

  <!--div class="sensor-card">
    <div class="sensor-label">PM1.0浓度</div>
    <div class="sensor-value" id="pm1_0_atm">--</div>
    <div class="sensor-label">大气颗粒物 (μg/m³)</div>
  </div-->

  <!--div class="sensor-card">
    <div class="sensor-label">PM2.5颗粒数</div>
    <div class="sensor-value" id="pm2_5_count">--</div>
    <div class="sensor-label">PM2.5颗粒数 (个/0.1L)</div>
  </div-->
  
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

  <!-- === 新增AI对话框（尺寸与前面卡片完全一致） === -->
  <div class="sensor-card">
    <h2 style="text-align: center; margin-bottom: 10px; color: #333;">AI助手</h2>
    <div style="margin-bottom: 10px;">
      <input type="text" id="ai-input" 
             placeholder="输入您的问题..." 
             style="width: 70%; padding: 8px; border: 1px solid #ccc; border-radius: 4px; font-size: 14px; box-sizing: border-box;">
      <button id="ai-send" 
              style="background: #4CAF50; color: white; border: none; padding: 8px 15px; border-radius: 4px; cursor: pointer; margin-left: 5px; box-sizing: border-box;">
        发送
      </button>
    </div>
    <div id="ai-response" style="padding: 12px; background: #f0f0f0; border-radius: 4px; min-height: 60px; font-size: 14px; line-height: 1.5; box-sizing: border-box;">
      AI助手已就绪，您可以开始提问...
    </div>
    <div id="ai-status" style="text-align: center; margin-top: 10px; font-size: 12px; color: #666; box-sizing: border-box;">
      状态: 就绪
    </div>
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
          // document.getElementById('pm1_0_atm').textContent = data.pm1_0_atm + ' μg/m³';
          // document.getElementById('pm2_5_count').textContent = data.pm2_5_count + ' 个/0.1L';
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

    // === AI助手功能（独立于原有刷新逻辑）===
    document.getElementById('ai-send').addEventListener('click', sendAIRequest);
    document.getElementById('ai-input').addEventListener('keypress', function(e) {
      if (e.key === 'Enter') sendAIRequest();
    });

    function sendAIRequest() {
      const input = document.getElementById('ai-input').value.trim();
      if (!input) return;

      // 仅更新AI对话框状态，不影响原有刷新
      document.getElementById('ai-status').textContent = '思考中...';
      document.getElementById('ai-response').textContent = '思考中...';
      document.getElementById('ai-input').value = '';

      fetch('/ai', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ prompt: input })
      })
      .then(response => response.json())
      .then(data => {
        document.getElementById('ai-response').textContent = data.response;
        document.getElementById('ai-status').textContent = '已回复';
      })
      .catch(error => {
        console.error('AI请求失败:', error);
        document.getElementById('ai-response').textContent = '请求失败';
        document.getElementById('ai-status').textContent = '错误';
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
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: mDNS初始化失败");
  }
  if (!dnsServer.start(53, domain_name, WiFi.localIP())) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: DNS初始化失败");
  }
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, [webData]() {
    handleDataRequest(webData);
  });
  server.on("/ai", HTTP_POST, [webData]() {
    handleAIRequest(webData);
  });
  server.begin();
}

// 初始化SPIFFS文件系统
bool initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: SPIFFS initialization failed: Failed to mount SPIFFS");
    return false;
  }
  Serial.println("DEBUG: SPIFFS mounted successfully");
  return true;
}

void HandleWebRequest(void *pvParameters) {
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