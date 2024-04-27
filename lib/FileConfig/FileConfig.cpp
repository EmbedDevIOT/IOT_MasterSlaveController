#include "FileConfig.h"

AT24Cxx eep(0x57, 32);

//////////////////////////////////////////////
// Loading settings from config.json file   //
//////////////////////////////////////////////
void LoadConfig()
{
  // Serial.println("Loading Configuration from config.json");
  String jsonConfig = "";

  File configFile = SPIFFS.open("/config.json", "r"); // Открываем файл для чтения
  jsonConfig = configFile.readString();               // загружаем файл конфигурации из EEPROM в глобальную переменную JsonObject
  configFile.close();

  StaticJsonDocument<2048> doc; //  Резервируем памяь для json обекта
  // Дессиарилизация - преобразование строки в родной объкт JavaScript (или преобразование последовательной формы в параллельную)
  deserializeJson(doc, jsonConfig); //  вызовите парсер JSON через экземпляр jsonBuffer

  char TempBuf[15];

  struct _txt
  {
    char TN[17];     // car_name
    char TI[1024];   // text info
    uint8_t TNU = 0; // car num
    bool SW = true;
    uint8_t SP = 0;
  } T;

  struct _sys
  {
    uint8_t H = 0;
    uint8_t M = 0;
    int D = 0;
    int MO = 0;
    int Y = 0;
  } S;

  HCONF.bright = doc["br"];

  ColorSet(&col_carnum, doc["c_carnum"]);
  ColorSet(&col_date, doc["c_date"]);
  ColorSet(&col_runtext, doc["c_infotext"]);
  ColorSet(&col_tempin, doc["c_tempin"]);
  ColorSet(&col_tempout, doc["c_tempout"]);
  ColorSet(&col_time, doc["c_time"]);

  doc["carname"].as<String>().toCharArray(T.TN, 17);
  memset(UserText.carname, 0, strlen(UserText.carname));
  strcat(UserText.carname, T.TN);

  UserText.carnum = doc["carnum"];

  // doc["date"].as<String>().toCharArray(TempBuf, 15);
  // S.Y = atoi(strtok(TempBuf, "-"));
  // S.MO = atoi(strtok(NULL, "-"));
  // S.D = atoi(strtok(NULL, "-"));

  CFG.fw = doc["firmware"].as<String>();

  UserText.hide_t = doc["hide"];

  doc["infotext"].as<String>().toCharArray(T.TI, 1024);
  memset(UserText.runtext, 0, strlen(UserText.runtext));
  strcat(UserText.runtext, T.TI);

  CFG.IP1 = doc["ip1"];
  CFG.IP2 = doc["ip2"];
  CFG.IP3 = doc["ip3"];
  CFG.IP4 = doc["ip4"];

  CFG.APPAS = doc["pass"].as<String>();
  UserText.run_mode = doc["runtext"];

  // CFG.sn = doc["sn"];

  UserText.speed = doc["speed"];

  CFG.APSSID = doc["ssid"].as<String>();

  HCONF.T1_offset = doc["t1_offset"];
  HCONF.T2_offset = doc["t2_offset"];

  // doc["time"].as<String>().toCharArray(TempBuf, 10);
  // S.H = atoi(strtok(TempBuf, ":"));
  // S.M = atoi(strtok(NULL, ":"));
}

void ShowLoadJSONConfig()
{
  char msg[32] = {0}; // buff for send message

  Serial.println(F("##############  System Configuration  ###############"));
  Serial.println("---------------------- COLOR ------------------------");
  Serial.printf("####  CARNUM: %d", GetColorNum(&col_carnum));
  Serial.println();
  Serial.printf("####  DATE: %d", GetColorNum(&col_date));
  Serial.println();
  Serial.printf("####  INFO_TEXT: %d", GetColorNum(&col_runtext));
  Serial.println();
  Serial.printf("####  TEMPIN: %d", GetColorNum(&col_tempin));
  Serial.println();
  Serial.printf("####  TEMPOUT: %d", GetColorNum(&col_tempout));
  Serial.println();
  Serial.printf("####  TIME: %d", GetColorNum(&col_time));
  Serial.println();
  Serial.println("-------------------- COLOR END ----------------------");
  Serial.println();
  Serial.println("-------------------- USER DATA -----------------------");
  Serial.printf("####  HideNum: %d", UserText.hide_t);
  Serial.println();
  Serial.printf("####  CarNum: %d", UserText.carnum);
  Serial.println();
  Serial.printf("####  RunText: %d  SPEED: %d", UserText.run_mode, UserText.speed);
  Serial.println();
  Serial.printf("####  Text:");
  Serial.printf(UserText.runtext);
  Serial.println();
  Serial.printf("####  T1_OFFSET: %d", HCONF.T1_offset);
  Serial.println();
  Serial.printf("####  T2_OFFSET: %d", HCONF.T2_offset);
  Serial.println();
  Serial.println("------------------ USER DATA END----------------------");
  Serial.println();
  Serial.println("---------------------- SYSTEM ------------------------");
  sprintf(msg, "####  DATA: %0002d-%02d-%02d", Clock.year, Clock.month, Clock.date);
  Serial.println(F(msg));
  sprintf(msg, "####  TIME: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
  Serial.println(F(msg));
  Serial.printf("####  WiFI NAME:");
  Serial.print(CFG.APSSID);
  Serial.println();
  Serial.printf("####  WiFI PASS:");
  Serial.print(CFG.APPAS);
  Serial.println();
  sprintf(msg, "####  IP: %00d.%00d.%00d.%00d", CFG.IP1, CFG.IP2, CFG.IP3, CFG.IP4);
  Serial.println(F(msg));
  Serial.printf("####  Brigh: %d", HCONF.bright);
  Serial.println();
  Serial.printf("####  SN: %d", CFG.sn);
  Serial.printf(" FW:");
  Serial.print(CFG.fw);
  Serial.println();
  Serial.println("-------------------- SYSTEM END-----------------------");
  Serial.println(F("######################################################"));
}

//////////////////////////////////////////////
// Save configuration in config.json file   //
//////////////////////////////////////////////
void SaveConfig()
{
  String jsonConfig = "";
  StaticJsonDocument<2048> doc;

  doc["br"] = HCONF.bright;
  doc["c_carnum"] = GetColorNum(&col_carnum);
  doc["c_date"] = GetColorNum(&col_date);
  doc["c_infotext"] = GetColorNum(&col_runtext);
  doc["c_tempin"] = GetColorNum(&col_tempin);
  doc["c_tempout"] = GetColorNum(&col_tempout);
  doc["c_time"] = GetColorNum(&col_time);

  doc["carname"] = String(UserText.carname);
  doc["carnum"] = UserText.carnum;
  // doc["date"] = String(Clock.year) + "-" + ((Clock.month < 10) ? "0" : "") + String(Clock.month) + "-" + ((Clock.date < 10) ? "0" : "") + String(Clock.date);
  doc["firmware"] = CFG.fw;
  doc["hide"] = UserText.hide_t;
  doc["infotext"] = String(UserText.runtext);
  doc["ip1"] = CFG.IP1;
  doc["ip2"] = CFG.IP2;
  doc["ip3"] = CFG.IP3;
  doc["ip4"] = CFG.IP4;
  doc["pass"] = CFG.APPAS;
  doc["runtext"] = UserText.run_mode;
  doc["sn"] = CFG.sn;
  doc["speed"] = UserText.speed;
  doc["ssid"] = CFG.APSSID;
  doc["t1_offset"] = HCONF.T1_offset;
  doc["t2_offset"] = HCONF.T2_offset;
  // doc["time"] = ((Clock.hour < 10) ? "0" : "") + String(Clock.hour) + ":" + ((Clock.minute < 10) ? "0" : "") + String(Clock.minute);

  // serializeJson(doc, Serial);
  // Serial.println();

  // serializeJsonPretty(doc, Serial);
  // Serial.println();

  File configFile = SPIFFS.open("/config.json", "w");
  serializeJson(doc, configFile); // Writing json string to file
  configFile.close();

  Serial.println("Configuration SAVE");
}

void TestDeserializJSON()
{
  String jsonConfig = "";

  File configFile = SPIFFS.open("/config.json", "r"); // Открываем файл для чтения
  jsonConfig = configFile.readString();               // загружаем файл конфигурации из EEPROM в глобальную переменную JsonObject
  configFile.close();

  StaticJsonDocument<2048> doc;
  deserializeJson(doc, jsonConfig); //  вызовите парсер JSON через экземпляр jsonBuffer

  serializeJsonPretty(doc, Serial);
  Serial.println();

  Serial.println("JSON testing comleted");
}

void EEP_Write()
{
  eep.write(0, CFG.sn);
}

// Reading data from EEPROM
void EEP_Read()
{
  CFG.sn = eep.read(0);
}