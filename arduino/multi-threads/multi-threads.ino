#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <FastLED.h>

#include "Common.h"
#include "LightController.h"
#include "ApihzController.h"
#include "AirIqController.h"
#include "NetworkController.h"
#include "WeatherController.h"
#include "WebDisplay.h"


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
 *   SetupLlight -- init L-light, slowly light up and then slowly light off              *
 *   LlightWifi -- light up 3s when wifi connected, blink 2times/1s when wifi lost       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/*
  TODO:
    [-] 当空气质量为优时，亮绿灯；当空气质量为一般时亮黄灯，当空气质量为差时，亮红灯；当空气质量极差时，红灯闪烁。
    [-] 读取MQ-4和MQ-135
    [-] PMS9103M支持睡眠模式
    [-] PMS9103M支持切换模式，被动模式使用定时器而不是sleep。
    [-] 创建http server，可通过连接http server获取天气信息/PMS9103M空气质量传感器信息，AI信息。

  Done:
    [x]当WIFI连接成功后，L灯亮起3s后熄灭（白灯）
    [x]WIFI连接中时，L灯快速闪烁（白灯）
    [x]通过apihz获取天气信息
    [x]通过apihz得到AI能力
    [x]读取PMS9103M空气质量传感器数据
    [x]支持PMS9103M设置主动模式和被动模式，并默认为被动模式，10min报告一次结果。
 */


// ================ Serial Settings =================
const uint SERIAL_PORT = 115200;  // serial 端口
// ==================================================


// ================= WiFi Settings ==================
const String WIFI_SSID = "OpenWRT_2G";
const String WIFI_PASSWORD = "183492765";
// const String WIFI_SSID = "realme GT 2 Pro";
// const String WIFI_PASSWORD = "999999999";
const String HOSTNAME = "esp32s3-n16r8";
// ==================================================


// =================== 配置你的信息 ===================
const String CITY = "成都";                             // 你想要查询的城市（如beijing, shanghai等）
const uint WEATHER_DELAY_TIME = DELAY_TIME * 10 * 600;  // 延迟时间
const uint MAX_AI_WORDS = MAX_SERIAL_INPUT;             // AI提问最大字符长度
// ==================================================


const uint MUTEX_WAIT = DELAY_TIME;  // mutex等待时间，100ms

HardwareSerial *pms_serial = &Serial0;

const uint NUM_L_LEDS = 8;

// =============队列句柄初始化=============
const uint Queue_Lenth = 2;
QueueHandle_t Ai_Words_Queue = NULL;
QueueHandle_t Serial_Queue = NULL;
QueueHandle_t Web_Input_Queue = NULL;
QueueHandle_t Web_Output_Queue = NULL;
QueueHandle_t Ai_Answer_Queue = NULL;
// ======================================


// =============init configs=============
Light L_Light;
WIFIConfig Wifi_Config;
WeatherConfig Weather_Config;
AIConfig Ai_Config;
PMSConfig Pms_Config;
// ======================================

// =============init data=============
PMData Pm_Data;
WeatherData Weather_Data;
WebData Web_Data;
//====================================


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *                         多线程函数                            *
 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

void LlightWifi(void *pvParameters) {
  uint connected_brightness = 30;
  uint connecting_brightness = 10;
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: LlightWifi入参为NULL");
    return;
  }
  Light *light = (Light *)pvParameters;
  wl_status_t pre_wifi_status = WiFi.status();
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME * 5));  // 0.5s
    if (WiFi.status() == WL_CONNECTED) {
      if (WiFi.status() == pre_wifi_status) {  // 状态未发生变化，重新轮询
        pre_wifi_status = WiFi.status();
        continue;
      } else {  // WiFi状态变化，亮灯3s后熄灭
        pre_wifi_status = WiFi.status();
        if (xSemaphoreTake(light->mutex, portMAX_DELAY)) {  // 获取锁
          light->brightness = connected_brightness;
          setLedBrightness(light);                     // light up
          vTaskDelay(pdMS_TO_TICKS(DELAY_TIME * 30));  // 3s
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
        vTaskDelay(pdMS_TO_TICKS(DELAY_TIME * 5));  // 0.5s
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
  vTaskDelete(NULL);
}

void GetAirIq(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAirIq入参为NULL");
    return;
  }
  PMSConfig *pms_config = (PMSConfig *)pvParameters;

  while (true) {
    if (NULL == pms_config) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAirIq的pms_config为NULL");
      vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
      continue;
    }
    if (NULL != pms_config->mutex && xSemaphoreTake(pms_config->mutex, portMAX_DELAY)) {  // 获取锁
      PMS_Mode mode = pms_config->mode;                                                   // 获取模式
      xSemaphoreGive(pms_config->mutex);                                                  // 释放锁
      if (PMS_ACTIVE_MODE == mode) {                                                      // 主动模式，无需定时器，300ms轮询一次
        // 读取PMS9103M传感器数据
        if (readPMS9103MData(pms_serial, &Pm_Data)) {
          // 打印读取到的数据
          serialPrintAirIqData(&Pm_Data);
        }
      } else if (PMS_PASSIVE_MODE == mode) {  // 被动模式，需要定时器，300ms轮询一次对于被动模式太快，需要使用定时器检查是否需要读取数据。
        // 读取PMS9103M传感器数据
        if (requestPMSDataInPassiveMode(pms_serial, &Pm_Data)) {
          // // 打印读取到的数据
          // serialPrintAirIqData(&Pm_Data);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME * 10 * 2));  // 2s读取一次
  }
  vTaskDelete(NULL);
}

// 在多线程中运行该函数，连接WIFI，并设置自动连接
void Connect_WIFI(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: Connect_WIFI入参为NULL");
    return;
  }
  WIFIConfig *wifi_config = (WIFIConfig *)pvParameters;  // 类型转换
  // if (wifi_config->mutex != NULL && xSemaphoreTake(wifi_config->mutex, portMAX_DELAY)) {  // portMAX_DELAY表示永久阻塞等锁，占用cpu资源。
  //   WiFi.setAutoReconnect(wifi_config->auto_reconnect);
  //   xSemaphoreGive(wifi_config->mutex);  // 释放锁
  // }
  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      bool _ = connect_WIFI(wifi_config);
    }
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME * 10 * 5));  // 5s检查一次wifi状态，否则重连。
  }
  vTaskDelete(NULL);
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
      vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
      continue;
    }
    if (weather_config->mutex != NULL && xSemaphoreTake(weather_config->mutex, MUTEX_WAIT)) {  // MUTEX_WAIT最大等锁时间，超过MUTEX_WAIT则返回NULL
      delay_time = weather_config->delay_time;
      getWeather(weather_config->city, &Weather_Data);
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
      vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
      continue;
    }
    if (ai_config->words_queue != NULL && xQueueReceive(ai_config->words_queue, words_char, portMAX_DELAY) == pdPASS) {  // MUTEX_WAIT最大等锁时间，超过MUTEX_WAIT则返回NULL
      String words = String(words_char);
      String result = getAIAnswer(words);
      // 将ai result输出给answer_queue
      char result_char[MAX_AI_WORDS * 2];
      result.toCharArray(result_char, MAX_AI_WORDS * 2);
      if (xQueueSend(ai_config->answer_queue, result_char, DELAY_TIME) != pdPASS) {
        Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 向answer_queue队列发送数据失败!");
      } else {
        Serial.printf("DEBUG: 向answer_queue发送数据: %s\n", result_char);
      }
    } else {  // 未获取到锁
      Serial.println("⚠️ WARN: 无法从words_queue队列中获取值！重新等待队列输入");
    }
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
  }
  vTaskDelete(NULL);
}

void DistributeAIAnswers(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: DistributeAIAnswers入参为NULL");
    return;
  }
  AIConfig *ai_config = (AIConfig *)pvParameters;  // 类型转换
  char result_char[MAX_AI_WORDS];
  while (true) {
    if (ai_config == NULL) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: DistributeAIAnswers入参为NULL");
      vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
      continue;
    }
    if (ai_config->answer_queue != NULL && xQueueReceive(ai_config->answer_queue, result_char, portMAX_DELAY) == pdPASS) {  // MUTEX_WAIT最大等锁时间，超过MUTEX_WAIT则返回NULL
      String result = String(result_char);
      // 将ai result分发出去
      if (ai_config->answer_queue != NULL && xQueueSend(ai_config->web_output_queue, result_char, DELAY_TIME) == pdPASS) {
      } else {
        Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 向web_output_queue队列发送数据失败!");
      }
    } else {  // 未获取到锁
      Serial.println("⚠️ WARN: 无法从answer_queue队列中获取值！重新等待队列输入");
    }
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
  }
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
      vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
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
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
  }
  vTaskDelete(NULL);
}

// 在多线程中运行该函数，获取对AI进行提问的问题。即获取输入给AI的input。
void GetAIQuestions(void *pvParameters) {
  if (NULL == pvParameters) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAIQuestions入参为NULL");
    return;
  }
  AIConfig *ai_config = (AIConfig *)pvParameters;  // 类型转换
  char input_char[MAX_AI_WORDS];
  // 创建队列集（容量需≥监听的队列数量）
  QueueSetHandle_t queueSet = xQueueCreateSet(2);  // 监听2个队列
  if (NULL != ai_config && NULL != ai_config->serial_queue) {
    xQueueAddToSet(ai_config->serial_queue, queueSet);
  }
  if (NULL != ai_config && NULL != ai_config->web_input_queue) {
    xQueueAddToSet(ai_config->web_input_queue, queueSet);
  }
  while (true) {
    if (ai_config == NULL) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: GetAIQuestions的ai_config为NULL");
      vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
      continue;
    }
    QueueSetMemberHandle_t activatedQueue = xQueueSelectFromSet(
      queueSet,
      portMAX_DELAY);
    if (activatedQueue != NULL && xQueueReceive(activatedQueue, input_char, portMAX_DELAY) == pdPASS) {  // MUTEX_WAIT最大等锁时间，超过MUTEX_WAIT则返回NULL
      if (ai_config->words_queue != NULL && xQueueSend(ai_config->words_queue, input_char, portMAX_DELAY) != pdPASS) {
        Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 向words_queue队列发送数据失败!");
      } else {
        Serial.printf("DEBUG: 向words_queue发送数据: %s\n", input_char);
      }
    } else {  // 未从serial_queue中获取到数据
      Serial.println("⚠️ WARN: 无法从activatedQueue队列中获取值！重新等待队列输入");
    }
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
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
  SemaphoreHandle_t pms_config_mutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t pm_data_mutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t weather_data_mutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t web_data_mutex = xSemaphoreCreateMutex();
  // check mutex created
  if (NULL == wifi_mutex || NULL == weather_mutex || NULL == ai_mutex || NULL == l_light_mutex || NULL == pms_config_mutex || NULL == pm_data_mutex || NULL == weather_data_mutex || NULL == web_data_mutex) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 无法创建锁，程序退出!");
    return;
  }

  // create queue
  Ai_Words_Queue = xQueueCreate(Queue_Lenth * 2, sizeof(char) * MAX_AI_WORDS);
  Serial_Queue = xQueueCreate(Queue_Lenth, sizeof(char) * MAX_AI_WORDS);
  Web_Input_Queue = xQueueCreate(Queue_Lenth, sizeof(char) * MAX_AI_WORDS);
  Web_Output_Queue = xQueueCreate(Queue_Lenth, sizeof(char) * MAX_AI_WORDS);
  Ai_Answer_Queue = xQueueCreate(Queue_Lenth * 2, sizeof(char) * MAX_AI_WORDS);
  // check queue created
  if (NULL == Ai_Words_Queue || NULL == Serial_Queue || NULL == Web_Input_Queue || NULL == Web_Output_Queue || NULL == Ai_Answer_Queue) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 无法创建队列，程序退出!");
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

  // PMS9103M config
  Pms_Config = {
    pms_set_pin: 0,              // PMS9103M的pin针引脚
    mode: PMS_PASSIVE_MODE,      // PMS_PASSIVE_MODE为被动模式，PMS_ACTIVE_MODE为主动模式（主动上报值）。
    standby_status: PMS_NORMAL,  // PMS_STANDBY为待机模式，PMS_NORMAL为正常模式
    sleep_status: false,         // 是否为睡眠状态，SET_PIN低电平为睡眠状态
    mutex: pms_config_mutex,     // 锁，获取本结构体中任何成员变量都需等此锁
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
    web_input_queue: Web_Input_Queue,
    web_output_queue: Web_Output_Queue,
    words_queue: Ai_Words_Queue,
    answer_queue: Ai_Answer_Queue,
  };

  // pm data
  Pm_Data = {
    mutex: pm_data_mutex,
  };

  // weather data
  Weather_Data = {
    mutex: weather_data_mutex,
  };

  // data
  Web_Data = {
    pmData: &Pm_Data,
    weatherDataCity: &Weather_Data,
    weatherDataHome: NULL,
    web_input_queue: &Web_Input_Queue,
    web_output_queue: &Web_Output_Queue,
    mutex: web_data_mutex,
  };

  // 任务句柄（可选）
  TaskHandle_t taskLlightWifiHandle = NULL;
  TaskHandle_t taskGetAirIqHandle = NULL;
  TaskHandle_t taskConnect_WIFIHandle = NULL;
  TaskHandle_t taskReadFromSerialHandle = NULL;
  TaskHandle_t taskGetWeatherHandle = NULL;
  TaskHandle_t taskGetAIQuestionsHandle = NULL;
  TaskHandle_t taskGetAIAnswerHandle = NULL;
  TaskHandle_t taskHandleWebRequestHandle = NULL;
  TaskHandle_t taskDistributeAIAnswersHandle = NULL;

  // L-light wifi
  // ----------------------L灯WIFI控制----------------------
  FastLED.addLeds<WS2812, PIN_RGB_LED, RGB>(L_Light.leds, L_Light.num_leds);
  setupLight(&L_Light, COLOR_WHITE);
  // // 设置PMS9103M为被动模式
  // if (setPMSWorkMode(pms_serial, &Pms_Config)) {
  //   Serial.println("set PMSWorkMode successfully");
  // }
  // ----------------------开机获取PMS9103M空气质量数据----------------------
  if (requestPMSDataInPassiveMode(pms_serial, &Pm_Data)) {
    // // 打印读取到的数据
    // serialPrintAirIqData(&Pm_Data);
  } else {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 开机获取PMS9103M空气质量数据失败！");
  }
  // -----------------------灯光控制-----------------------
  xTaskCreatePinnedToCore(
    LlightWifi, "TaskLlightWifi", 8192, (void *)&L_Light, 5, &taskLlightWifiHandle, 0);  // tskNO_AFFINITY表示不限制core
  // -----------------------传感器-----------------------
  xTaskCreatePinnedToCore(
    GetAirIq, "TaskGetAirIq", 8192, (void *)&Pms_Config, 5, &taskGetAirIqHandle, 0);  // tskNO_AFFINITY表示不限制core
  // -----------------------Network-----------------------
  while (!connect_WIFI(&Wifi_Config))
    ;
  // ----------------------WiFi控制----------------------
  xTaskCreatePinnedToCore(
    Connect_WIFI, "TaskConnect_WIFI", 8192, (void *)&Wifi_Config, 5, &taskConnect_WIFIHandle, 0);  // tskNO_AFFINITY表示不限制core
  // update local time
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  // ----------------------设置web服务端----------------------
  setupWebServer(HOSTNAME, &Web_Data);
  // // ----------------------从serial中读取数据----------------------
  // xTaskCreatePinnedToCore(
  //   ReadFromSerial, "TaskReadFromSerial", 8192, (void *)&Serial_Queue, 5, &taskReadFromSerialHandle, 1);  // tskNO_AFFINITY表示不限制core
  // ----------------------从apihz中获取天气信息----------------------
  xTaskCreatePinnedToCore(
    GetWeather, "TaskGetWeather", 8192, (void *)&Weather_Config, 4, &taskGetWeatherHandle, 1);  // tskNO_AFFINITY表示不限制core
  // ----------------------获取询问AI的问题----------------------
  xTaskCreatePinnedToCore(
    GetAIQuestions, "TaskGetAIQuestions", 8192, (void *)&Ai_Config, 3, &taskGetAIQuestionsHandle, 1);  // tskNO_AFFINITY表示不限制core
  // ----------------------获取AI结果----------------------
  xTaskCreatePinnedToCore(
    GetAIAnswer, "TaskGetAIAnswer", 8192, (void *)&Ai_Config, 3, &taskGetAIAnswerHandle, 1);  // tskNO_AFFINITY表示不限制core
  // ----------------------分发AI结果----------------------
  xTaskCreatePinnedToCore(
    DistributeAIAnswers, "TaskDistributeAIAnswers", 8192, (void *)&Ai_Config, 3, &taskDistributeAIAnswersHandle, 1);  // tskNO_AFFINITY表示不限制core
  // SetWebServer
  xTaskCreatePinnedToCore(
    HandleWebRequest, "TaskHandleWebRequest", 8192, (void *)&Web_Data, 2, &taskHandleWebRequestHandle, 1);  // tskNO_AFFINITY表示不限制core
}

void loop() {
  // This loop is not executed by FreeRTOS
  // Instead, tasks are scheduled by the FreeRTOS scheduler
}