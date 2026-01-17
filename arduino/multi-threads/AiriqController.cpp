#include "AiriqController.h"

void getAirIq(PMData *pmData) {
  // 读取PMS9103M传感器数据
  if (readPMS9103MData(pmData)) {
    // 打印读取到的数据
    Serial.println("PMS9103M数据读取成功:");
    Serial.print("PM1.0 (标准): "); Serial.print(pmData->pm1_0); Serial.println(" μg/m³");
    Serial.print("PM2.5 (标准): "); Serial.print(pmData->pm2_5); Serial.println(" μg/m³");
    Serial.print("PM10.0 (标准): "); Serial.print(pmData->pm10_0); Serial.println(" μg/m³");
    Serial.print("PM1.0 (大气): "); Serial.print(pmData->pm1_0_atm); Serial.println(" μg/m³");
    Serial.print("PM2.5 (大气): "); Serial.print(pmData->pm2_5_atm); Serial.println(" μg/m³");
    Serial.print("PM10.0 (大气): "); Serial.print(pmData->pm10_0_atm); Serial.println(" μg/m³");
    Serial.print("0.3μm颗粒数: "); Serial.print(pmData->count_0_3); Serial.println(" 个/0.1L");
    Serial.print("0.5μm颗粒数: "); Serial.print(pmData->count_0_5); Serial.println(" 个/0.1L");
    Serial.print("1.0μm颗粒数: "); Serial.print(pmData->count_1_0); Serial.println(" 个/0.1L");
    Serial.print("2.5μm颗粒数: "); Serial.print(pmData->count_2_5); Serial.println(" 个/0.1L");
    Serial.print("5.0μm颗粒数: "); Serial.print(pmData->count_5_0); Serial.println(" 个/0.1L");
    Serial.print("10.0μm颗粒数: "); Serial.print(pmData->count_10_0); Serial.println(" 个/0.1L");
    Serial.println("------------------------");
  }
}

// 读取PMS9103M传感器数据的函数
boolean readPMS9103MData(PMData *pmData) {
  // PMS9103M数据帧格式：以0x42 0x4D开头，长度32字节
  // 从文档附录A：主动式传输协议
  
  // 检查是否有32字节数据可读
  int lenth = Serial0.available();
  if (lenth >= 32) {
    Serial.printf("readed data: lenth is %d\n", lenth);
    // 读取32字节数据帧
    byte buffer[lenth];
    // for (int i = 0; i < lenth; i++) {
    //   buffer[i] = Serial0.read();
    // }
    Serial0.readBytes(buffer, lenth);
    Serial.printf("buffer[0] is %d\n", buffer[0]);
    // 验证帧头（必须为0x42 0x4D）
    if (buffer[0] == 0x42 && buffer[1] == 0x4D) {
      Serial.println("buffer[0] is OK");
      // 从数据帧中提取PM2.5标准数据
      pmData->pm1_0 = (buffer[4] << 8) | buffer[5];     // 数据1
      pmData->pm2_5 = (buffer[6] << 8) | buffer[7];     // 数据2
      pmData->pm10_0 = (buffer[8] << 8) | buffer[9];    // 数据3
      
      // 从数据帧中提取大气环境数据
      pmData->pm1_0_atm = (buffer[10] << 8) | buffer[11]; // 数据4
      pmData->pm2_5_atm = (buffer[12] << 8) | buffer[13]; // 数据5
      pmData->pm10_0_atm = (buffer[14] << 8) | buffer[15]; // 数据6
      
      // 从数据帧中提取颗粒物个数
      pmData->count_0_3 = (buffer[16] << 8) | buffer[17]; // 数据7
      pmData->count_0_5 = (buffer[18] << 8) | buffer[19]; // 数据8
      pmData->count_1_0 = (buffer[20] << 8) | buffer[21]; // 数据9
      pmData->count_2_5 = (buffer[22] << 8) | buffer[23]; // 数据10
      pmData->count_5_0 = (buffer[24] << 8) | buffer[25]; // 数据11
      pmData->count_10_0 = (buffer[26] << 8) | buffer[27]; // 数据12
      
      // 验证校验和
      uint checksum = 0;
      for (int i = 0; i < 30; i++) {
        checksum += buffer[i];
      }
      if (checksum == (buffer[30] << 8 | buffer[31])) {
        Serial.println("checksum is OK");
        return true; // 校验和正确，数据有效
      }
    }
  }
  return false; // 数据读取失败
}