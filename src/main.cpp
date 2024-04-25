#include "Config.h"

#define DEBUG // Debug control ON
//======================================================================

//=========================== GLOBAL VARIABLES =========================
uint8_t sec_cnt = 0;
char buf[32] = {0}; // buff for send message
//======================================================================

//================================ OBJECTs =============================
MicroDS3231 RTC;

OneWire oneWire1(T1);
OneWire oneWire2(T2);
DallasTemperature ds18b20_1(&oneWire1);
DallasTemperature ds18b20_2(&oneWire2);

TaskHandle_t TaskCore_0;
TaskHandle_t TaskCore_1;
//=======================================================================

//============================== STRUCTURES =============================
