#include "AirIqController.h"

/**
 * @brief 初始化传感器控制引脚
 * 
 */
void initPMSControl(PMSConfig *pms_config) {
  if (xSemaphoreTake(pms_config->mutex, portMAX_DELAY)) {  // 获取锁
    // 初始化 SET 引脚为输出
    pinMode(pms_config->pms_set_pin, OUTPUT);
    // 上电默认通常是唤醒状态，或者根据需要初始化
    digitalWrite(pms_config->pms_set_pin, HIGH);
    xSemaphoreGive(pms_config->mutex);  // 释放锁
  }
}

/**
 * @brief 通过硬件引脚控制传感器休眠/唤醒
 * 文档说明：SET低电平为休眠，高电平或悬空为正常工作
 * @param pms_config->sleep_status true=休眠, false=唤醒
 */
void setPMSSleepHardware(PMSConfig *pms_config) {
  if (xSemaphoreTake(pms_config->mutex, portMAX_DELAY)) {  // 获取锁
    if (pms_config->sleep_status) {
      digitalWrite(pms_config->pms_set_pin, LOW);
      Serial.println("⚡ INFO: [PMS] 硬件控制：传感器进入休眠模式");
    } else {
      digitalWrite(pms_config->pms_set_pin, HIGH);
      Serial.println("⚡ INFO: [PMS] 硬件控制：传感器唤醒");
      // 文档第6章注意事项：休眠唤醒后需要至少30s稳定时间
      vTaskDelay(pdMS_TO_TICKS(1000 * 30 * 2));
    }
    xSemaphoreGive(pms_config->mutex);  // 释放锁
  }
}

bool writePMS9103MData(HardwareSerial *serial, uint8_t cmd_type, uint8_t value) {
  // 构建切换模式指令帧
  // 格式: 0x42 0x4D cmd_type 0x00 value CHECKSUM_H CHECKSUM_L
  uint8_t cmd[7];
  cmd[0] = 0x42;
  cmd[1] = 0x4D;
  cmd[2] = cmd_type;  // 指令字，设置
  cmd[3] = 0x00;      // 保留位
  cmd[4] = value;     // 数据位：00=被动，01=主动

  // 计算校验和 (参考文档附录B第4点：从特征字节开始所有字节累加和)
  uint16_t checksum = 0;
  for (int i = 0; i < 5; i++) {
    checksum += cmd[i];
  }
  cmd[5] = (checksum >> 8) & 0xFF;  // 校验高八位
  cmd[6] = checksum & 0xFF;         // 校验低八位
  // 发送指令
  serial->write(cmd, 7);
  vTaskDelay(pdMS_TO_TICKS(200));
  PMData pm_data;
  pm_data.mutex = xSemaphoreCreateMutex();
  if (NULL == pm_data.mutex) {
    Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 无法创建锁，程序退出!");
    return false;
  }
  if (readPMS9103MData(serial, &pm_data)) {
    return true;
  } else {
    return false;
  }
}

bool setPMSStandbyMode(HardwareSerial *serial, PMSConfig *pms_config) {
  if (xSemaphoreTake(pms_config->mutex, portMAX_DELAY)) {  // 获取锁
    // 构建切换模式指令帧
    // 格式: 0x42 0x4D CMD 0x00 mode CHECKSUM_H CHECKSUM_L
    uint8_t cmd_type, value;
    cmd_type = CMD_STANDBY_MODE;
    // 设置待机/正常模式
    if (PMS_NORMAL == pms_config->standby_status) {  // 正常模式
      value = STANDBY_OFF;
    } else if (PMS_STANDBY == pms_config->standby_status) {  // 待机模式
      value = STANDBY_ON;
    }
    if (!writePMS9103MData(serial, cmd_type, value)) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 无法设置PMS工作模式，PMS采集退出！");
      xSemaphoreGive(pms_config->mutex);  // 释放锁
      return false;
    }
    if (PMS_NORMAL == pms_config->standby_status) {
      Serial.println("⚡ INFO: [PMS] 串口指令：已切换至正常模式");
    } else if (PMS_STANDBY == pms_config->standby_status) {
      Serial.println("⚡ INFO: [PMS] 串口指令：已切换至待机模式");
    }
    xSemaphoreGive(pms_config->mutex);  // 释放锁
  }
  return true;
}

/**
 * @brief 通过串口发送指令切换传感器工作模式（主动/被动）
 * @param serial 传输用的串口对象指针
 * @param mode MODE_PASSIVE 或 MODE_ACTIVE
 * @return true 成功发送指令
 */
bool setPMSWorkMode(HardwareSerial *serial, PMSConfig *pms_config) {
  if (xSemaphoreTake(pms_config->mutex, portMAX_DELAY)) {  // 获取锁
    // 构建切换模式指令帧
    // 格式: 0x42 0x4D CMD 0x00 mode CHECKSUM_H CHECKSUM_L
    uint8_t cmd_type, value;
    cmd_type = CMD_PASSIVE_ACTIVE_MODE;
    // 设置主动/被动模式
    if (PMS_ACTIVE_MODE == pms_config->mode) {  // 主动模式
      value = ACTIVE_MODE;
    } else if (PMS_PASSIVE_MODE == pms_config->mode) {  // 被动模式
      value = PASSIVE_MODE;
    }
    if (!writePMS9103MData(serial, cmd_type, value)) {
      Serial.println("⚠️⚠️⚠️ ERROR ⚠️⚠️⚠️: 无法设置PMS工作模式，PMS采集退出！");
      xSemaphoreGive(pms_config->mutex);  // 释放锁
      return false;
    }
    if (PMS_ACTIVE_MODE == pms_config->mode) {
      Serial.println("⚡ INFO: [PMS] 串口指令：已切换至主动模式");
    } else if (PMS_PASSIVE_MODE == pms_config->mode) {
      Serial.println("⚡ INFO: [PMS] 串口指令：已切换至被动模式");
    }
    xSemaphoreGive(pms_config->mutex);  // 释放锁
  }
  return true;
}

/**
 * @brief 在被动模式下请求一组数据
 * 注意：必须先通过 setPMSWorkMode 设置为被动模式，此函数才有效
 * @param serial 传输用的串口对象指针
 * @param pmData 数据存储结构体
 * @return bool true=读取成功
 */
bool requestPMSDataInPassiveMode(HardwareSerial *serial, PMData *pmData) {
  // 1. 发送读取数据命令 (0xE2)
  uint8_t req[7] = { 0x42, 0x4D, CMD_READ_DATA, 0x00, 0x00, 0x00, 0x00 };
  // 计算校验和 (参考文档附录B第4点：从特征字节开始所有字节累加和)
  uint16_t checksum = 0;
  for (int i = 0; i < 5; i++) {
    checksum += req[i];
  }
  req[5] = (checksum >> 8) & 0xFF;  // 校验高八位
  req[6] = checksum & 0xFF;         // 校验低八位
  serial->write(req, 7);

  // 2. 等待传感器返回32字节数据帧
  vTaskDelay(pdMS_TO_TICKS(100));  // 等待传感器处理并返回数据
  return readPMS9103MData(serial, pmData);
}

void serialPrintAirIqData(PMData *pmData) {
  if (xSemaphoreTake(pmData->mutex, portMAX_DELAY)) {  // 获取锁
    Serial.printf("PM1.0 (标准): %d μg/m³\nPM2.5 (标准): %d μg/m³\nPM10.0 (标准): %d μg/m³\n", pmData->pm1_0, pmData->pm2_5, pmData->pm10_0);
    Serial.printf("PM1.0 (大气): %d μg/m³\nPM2.5 (大气): %d μg/m³\nPM10.0 (大气): %d μg/m³\n", pmData->pm1_0_atm, pmData->pm2_5_atm, pmData->pm10_0_atm);
    Serial.printf("0.3μm颗粒数: %d 个/0.1L\n0.5μm颗粒数: %d 个/0.1L\n1.0μm颗粒数: %d 个/0.1L\n2.5μm颗粒数: %d 个/0.1L\n5.0μm颗粒数: %d 个/0.1L\n10.0μm颗粒数: %d 个/0.1L\n", pmData->count_0_3, pmData->count_0_5, pmData->count_1_0, pmData->count_2_5, pmData->count_5_0, pmData->count_10_0);
    Serial.println("------------------------");
    xSemaphoreGive(pmData->mutex);  // 释放锁
  }
}

// 读取PMS9103M传感器数据的函数
bool readPMS9103MData(HardwareSerial *serial, PMData *pmData) {
  // PMS9103M数据帧格式：以0x42 0x4D开头，长度32字节
  // 从文档附录A：主动式传输协议
  // 检查是否有32字节数据可读
  int lenth = serial->available();
  if (lenth >= 32) {
    // 读取32字节数据帧
    byte buffer[lenth];
    serial->readBytes(buffer, lenth);
    // 计算校验值
    uint checksum = 0;
    for (int i = 0; i < 30; i++) {
      checksum += buffer[i];
    }
    // 验证帧头（必须为0x42 0x4D）和验证校验值
    if (buffer[0] == 0x42 && buffer[1] == 0x4D && (buffer[30] << 8 | buffer[31]) == checksum) {
      if (xSemaphoreTake(pmData->mutex, portMAX_DELAY)) {  // 获取锁
        // 从数据帧中提取PM2.5标准数据
        pmData->pm1_0 = (buffer[4] << 8) | buffer[5];   // 数据1
        pmData->pm2_5 = (buffer[6] << 8) | buffer[7];   // 数据2
        pmData->pm10_0 = (buffer[8] << 8) | buffer[9];  // 数据3

        // 从数据帧中提取大气环境数据
        pmData->pm1_0_atm = (buffer[10] << 8) | buffer[11];   // 数据4
        pmData->pm2_5_atm = (buffer[12] << 8) | buffer[13];   // 数据5
        pmData->pm10_0_atm = (buffer[14] << 8) | buffer[15];  // 数据6

        // 从数据帧中提取颗粒物个数
        pmData->count_0_3 = (buffer[16] << 8) | buffer[17];   // 数据7
        pmData->count_0_5 = (buffer[18] << 8) | buffer[19];   // 数据8
        pmData->count_1_0 = (buffer[20] << 8) | buffer[21];   // 数据9
        pmData->count_2_5 = (buffer[22] << 8) | buffer[23];   // 数据10
        pmData->count_5_0 = (buffer[24] << 8) | buffer[25];   // 数据11
        pmData->count_10_0 = (buffer[26] << 8) | buffer[27];  // 数据12
        xSemaphoreGive(pmData->mutex);                        // 释放锁
        return true;
      }
    }
  }
  return false;  // 数据读取失败
}