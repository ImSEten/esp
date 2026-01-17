#include <Arduino.h>

// 定义传感器数据结构
struct PMData {
  uint16_t pm1_0;    // PM1.0浓度（标准颗粒物，单位：μg/m³）
  uint16_t pm2_5;    // PM2.5浓度（标准颗粒物，单位：μg/m³）
  uint16_t pm10_0;   // PM10浓度（标准颗粒物，单位：μg/m³）
  uint16_t pm1_0_atm; // PM1.0浓度（大气环境下，单位：μg/m³）
  uint16_t pm2_5_atm; // PM2.5浓度（大气环境下，单位：μg/m³）
  uint16_t pm10_0_atm; // PM10浓度（大气环境下，单位：μg/m³）
  uint16_t count_0_3; // 0.3μm以上颗粒物个数（每0.1升空气）
  uint16_t count_0_5; // 0.5μm以上颗粒物个数（每0.1升空气）
  uint16_t count_1_0; // 1.0μm以上颗粒物个数（每0.1升空气）
  uint16_t count_2_5; // 2.5μm以上颗粒物个数（每0.1升空气）
  uint16_t count_5_0; // 5.0μm以上颗粒物个数（每0.1升空气）
  uint16_t count_10_0; // 10.0μm以上颗粒物个数（每0.1升空气）
};

void getAirIq(PMData *pmData);
boolean readPMS9103MData(PMData *pmData);