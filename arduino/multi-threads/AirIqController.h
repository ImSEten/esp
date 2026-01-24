#ifndef AIRIQ_CONTROLLER_H
#define AIRIQ_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>


// 协议命令定义 (参考文档 附录B)
#define CMD_PASSIVE_ACTIVE_MODE 0xE1  // 主动被动模式切换命令
#define CMD_READ_DATA 0xE2            // 请求读取数据
#define CMD_STANDBY_MODE 0xE4         // 待机控制指令

// 状态定义
#define PASSIVE_MODE 0x00
#define ACTIVE_MODE 0x01
#define STANDBY_ON 0x00   // 待机模式
#define STANDBY_OFF 0x01  // 正常模式

typedef enum {
  PMS_PASSIVE_MODE = 0,  // 被动模式
  PMS_ACTIVE_MODE = 1,   // 主动模式
} PMS_Mode;

typedef enum {
  PMS_STANDBY = 0,  // 待机模式
  PMS_NORMAL = 1,   // 正常模式
} PMS_Standby_Status;

// PMS9103M状态信息
typedef struct {
  uint pms_set_pin;                   // PMS9103M的pin针引脚
  PMS_Mode mode;                      // 0为被动模式，1为主动模式（主动上报值）。
  PMS_Standby_Status standby_status;  // 0为待机模式，1为正常模式
  bool sleep_status;                  // 是否为睡眠状态，SET_PIN低电平为睡眠状态
  SemaphoreHandle_t mutex;            // 锁，获取本结构体中任何成员变量都需等此锁
} PMSConfig;

// 定义传感器数据结构
struct PMData {
  uint16_t pm1_0;           // PM1.0浓度（标准颗粒物，单位：μg/m³）
  uint16_t pm2_5;           // PM2.5浓度（标准颗粒物，单位：μg/m³）
  uint16_t pm10_0;          // PM10浓度（标准颗粒物，单位：μg/m³）
  uint16_t pm1_0_atm;       // PM1.0浓度（大气环境下，单位：μg/m³）
  uint16_t pm2_5_atm;       // PM2.5浓度（大气环境下，单位：μg/m³）
  uint16_t pm10_0_atm;      // PM10浓度（大气环境下，单位：μg/m³）
  uint16_t count_0_3;       // 0.3μm以上颗粒物个数（每0.1升空气）
  uint16_t count_0_5;       // 0.5μm以上颗粒物个数（每0.1升空气）
  uint16_t count_1_0;       // 1.0μm以上颗粒物个数（每0.1升空气）
  uint16_t count_2_5;       // 2.5μm以上颗粒物个数（每0.1升空气）
  uint16_t count_5_0;       // 5.0μm以上颗粒物个数（每0.1升空气）
  uint16_t count_10_0;      // 10.0μm以上颗粒物个数（每0.1升空气）
  SemaphoreHandle_t mutex;  // 锁，获取本结构体中任何成员变量都需等此锁
};

void initPMSControl(PMSConfig *pms_config);
void setPMSSleepHardware(PMSConfig *pms_config);
bool writePMS9103MData(HardwareSerial *serial, uint8_t cmd_type, uint8_t value);
bool setPMSWorkMode(HardwareSerial *serial, PMSConfig *pms_config);
bool setPMSStandbyMode(HardwareSerial *serial, PMSConfig *pms_config);
bool requestPMSDataInPassiveMode(HardwareSerial *serial, PMData *pmData);
bool readPMS9103MData(HardwareSerial *serial, PMData *pmData);
void serialPrintAirIqData(PMData *pmData);
JsonDocument marshelPmData(PMData *pmData);

#endif