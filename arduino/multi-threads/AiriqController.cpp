#include "AiriqController.h"

void serialPrintAirIqData(PMData *pmData) {
  if (xSemaphoreTake(pmData->mutex, portMAX_DELAY)) {  // 获取锁
    Serial.printf("PM1.0 (标准): %d μg/m³\nPM2.5 (标准): %d μg/m³\nPM10.0 (标准): %d μg/m³\n", pmData->pm1_0, pmData->pm2_5, pmData->pm10_0);
    Serial.printf("PM1.0 (大气): %d μg/m³\nPM2.5 (大气): %d μg/m³\nPM10.0 (大气): %d μg/m³\n", pmData->pm1_0_atm, pmData->pm2_5_atm, pmData->pm10_0_atm);
    Serial.printf("0.3μm颗粒数: %d 个/0.1L\n0.5μm颗粒数: %d 个/0.1L\n1.0μm颗粒数: %d 个/0.1L\n2.5μm颗粒数: %d 个/0.1L\n5.0μm颗粒数: %d 个/0.1L\n10.0μm颗粒数: %d 个/0.1L\n", pmData->count_0_3, pmData->count_0_5, pmData->count_1_0, pmData->count_2_5, pmData->count_5_0, pmData->count_10_0);
    Serial.println("------------------------");
    xSemaphoreGive(pmData->mutex); // 释放锁
  }
}

// 读取PMS9103M传感器数据的函数
bool readPMS9103MData(PMData *pmData) {
  // PMS9103M数据帧格式：以0x42 0x4D开头，长度32字节
  // 从文档附录A：主动式传输协议
  // 检查是否有32字节数据可读
  int lenth = Serial0.available();
  if (lenth >= 32) {
    // 读取32字节数据帧
    byte buffer[lenth];
    Serial0.readBytes(buffer, lenth);
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
        xSemaphoreGive(pmData->mutex); // 释放锁
        return true;
      }
    }
  }
  return false;  // 数据读取失败
}