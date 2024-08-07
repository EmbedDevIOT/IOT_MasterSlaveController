#include "Config.h"

//==========================================================================
void ColorSet(struct color *CS, uint8_t _color)
{
  switch (_color)
  {
  case RED:
    CS->R = 255;
    CS->G = 0;
    CS->B = 0;
    CS->hex = 0xFF0000;
    break;

  case GREEN:
    CS->R = 0;
    CS->G = 255;
    CS->B = 0;
    CS->hex = 0x00FF00;
    break;

  case BLUE:
    CS->R = 0;
    CS->G = 0;
    CS->B = 255;
    CS->hex = 0x0000FF;
    break;

  case BLACK:
    CS->R = 0;
    CS->G = 0;
    CS->B = 0;
    CS->hex = 0x000000;
    break;

  case WHITE:
    CS->R = 255;
    CS->G = 255;
    CS->B = 255;
    CS->hex = 0xFFFFFF;
    break;

  case SEA_WAVE:
    CS->R = 0;
    CS->G = 255;
    CS->B = 255;
    CS->hex = 0x00FFFF;
    break;

  case YELLOW:
    CS->R = 255;
    CS->G = 255;
    CS->B = 0;
    CS->hex = 0xFFFF00;
    break;

  case PURPLE:
    CS->R = 255;
    CS->G = 0;
    CS->B = 255;
    CS->hex = 0xFF00FF;
    break;

  default:
    break;
  }
}
//=======================================================================

int GetColorNum(struct color *C)
{
  int _col = 0;

  if (C->R == 255 && C->G == 0 && C->B == 0)
    _col = RED;
  if (C->R == 0 && C->G == 255 && C->B == 0)
    _col = GREEN;
  if (C->R == 0 && C->G == 0 && C->B == 255)
    _col = BLUE;
  if (C->R == 0 && C->G == 0 && C->B == 0)
    _col = BLACK;
  if (C->R == 255 && C->G == 255 && C->B == 255)
    _col = WHITE;
  if (C->R == 0 && C->G == 255 && C->B == 255)
    _col = SEA_WAVE;
  if (C->R == 255 && C->G == 255 && C->B == 0)
    _col = YELLOW;
  if (C->R == 255 && C->G == 0 && C->B == 255)
    _col = PURPLE;

  return _col;
}
//=======================================================================
// Writing color in RGB to buf
void ColorWrite(char *buf, struct color *C)
{
  strcat(buf, "<R>");
  itoa(C->R, buf + strlen(buf), DEC);
  strcat(buf, "</R>\r\n");
  strcat(buf, "<G>");
  itoa(C->G, buf + strlen(buf), DEC);
  strcat(buf, "</G>\r\n");
  strcat(buf, "<B>");
  itoa(C->B, buf + strlen(buf), DEC);
  strcat(buf, "</B>\r\n");
}
//=======================================================================

//=======================================================================
void UserPresetInit()
{
  ColorSet(&col_carnum, YELLOW);
  ColorSet(&col_wc1, GREEN);
  ColorSet(&col_speed, WHITE);
  ColorSet(&col_time, WHITE);
  ColorSet(&col_date, WHITE);
  ColorSet(&col_tempin, GREEN);
  ColorSet(&col_tempout, BLUE);

  UserText.carnum = 77;

  strcat(UserText.carname, "Вагон ");
}

/************************ System Initialisation **********************/
void SystemInit(void)
{
  STATE.IDLE = true;
  STATE.StaticUPD = true;
  STATE.DynamicUPD = true;
  STATE.DUPDBlock = false;
  STATE.LedRS = false;
  STATE.LedWiFi = false;
  STATE.SaveFlash = false;
  STATE.RS = false;
  STATE.Debug = true;
  STATE.CurDebug = false;
  STATE.WiFiEnable = true;
  STATE.TTS = false;   // Flag Start Time Speech
  STATE.DSTS1 = false; // Flag Start WC_1 Speech
  STATE.DSTS2 = false; // Flag Start WC_2 Speech
  STATE.VolumeUPD = false;

  GetChipID();
}
/*******************************************************************************************************/

/***************************** Function Show information or Device *************************************/
void ShowInfoDevice(void)
{
  Serial.println(F("Starting..."));
  Serial.println(F("TableController_0845"));
  Serial.print(F("SN:"));
  Serial.println(CFG.sn);
  Serial.print(F("fw_date:"));
  Serial.println(CFG.fwdate);
  Serial.println(CFG.fw);
  Serial.println(CFG.chipID);
  Serial.println(F("by EmbedDev"));
  Serial.println();
}
/*******************************************************************************************************/
/*******************************************************************************************************/
void GetChipID()
{
  uint32_t chipId = 0;

  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  CFG.chipID = chipId;
}
/*******************************************************************************************************/
/*******************************************************************************************************/
String GetMacAdr()
{
  CFG.MacAdr = WiFi.macAddress(); //
  Serial.print(F("MAC:"));        // временно
  Serial.println(CFG.MacAdr);     // временно
  return WiFi.macAddress();
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// Debug information
void DebugInfo()
{
#ifndef DEBUG
  if (STATE.Debug)
  {
    char message[37];

    Serial.println(F("!!!!!!!!!!!!!!  DEBUG INFO  !!!!!!!!!!!!!!!!!!"));

    // sprintf(message, "StaticUPD: %0d CNT: %0d", STATE.StaticUPD, STATE.cnt_Supd);
    // Serial.println(message);
    // sprintf(message, "CAR_NUM: %d HideMode: %d", UserText.carnum, UserText.hide_t);
    // Serial.println(message);
    // Serial.printf("Brightness:");
    // Serial.println(HCONF.bright);
    sprintf(message, "GMT: %d", CFG.gmt);
    Serial.println(message);
    sprintf(message, "RTC Time: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
    Serial.println(message);
    sprintf(message, "RTC Date: %4d.%02d.%02d", Clock.year, Clock.month, Clock.date);
    Serial.println(message);
    sprintf(message, "T1: %0.1f T2: %0.1f", HCONF.dsT1, HCONF.dsT2);
    Serial.println(message);
    sprintf(message, "T1_OFS: %d T2_OFS: %d", HCONF.T1_offset, HCONF.T2_offset);
    Serial.println(message);
    sprintf(message, "WC1 | Sensor: %d State %d", STATE.SensWC1, STATE.StateWC1);
    Serial.println(message);
    sprintf(message, "WC2 | Sensor: %d State %d", STATE.SensWC2, STATE.StateWC2);
    Serial.println(message);
    // sprintf(message, "WC1 | Color: %d", GetColorNum(&col_wc1));
    // Serial.println(message);
    // sprintf(message, "WC2 | Color: %d", GetColorNum(&col_wc2));
    // Serial.println(message);

    // Serial.printf("SN:");
    // Serial.println(CFG.sn);

    Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
    Serial.println();
  }
#endif
}

//=======================================================================

/*******************************************************************************************************/
void ShowFlashSave()
{
  bool state = false;

  for (uint8_t i = 0; i <= 1; i++)
  {
    digitalWrite(LED_ST, ON);
    vTaskDelay(400 / portTICK_PERIOD_MS);
    digitalWrite(LED_ST, OFF);
    vTaskDelay(400 / portTICK_PERIOD_MS);
  }
}
/***************************************************** ****************************/
void SystemFactoryReset()
{
  CFG.gmt = 3;
  CFG.WiFiMode = AccessPoint;
  CFG.APSSID = "0845";
  CFG.APPAS = "retra0845zxc";
  CFG.IP1 = 192;
  CFG.IP2 = 168;
  CFG.IP3 = 1;
  CFG.IP4 = 1;
  CFG.GW1 = 192;
  CFG.GW2 = 168;
  CFG.GW3 = 1;
  CFG.GW4 = 1;
  CFG.MK1 = 255;
  CFG.MK2 = 255;
  CFG.MK3 = 255;
  CFG.MK4 = 0;

  HCONF.bright = 90;
  HCONF.volume = 100;
  HCONF.T1_offset = 0;
  HCONF.T2_offset = 0;
  HCONF.WCL = NORMAL;
  HCONF.WCSS = SENSOR_CLOSE;

  STATE.WiFiEnable = true;

  ColorSet(&col_carnum, WHITE);
  ColorSet(&col_wc1, GREEN);
  ColorSet(&col_wc2, GREEN);
  ColorSet(&col_time, WHITE);
  ColorSet(&col_date, WHITE);
  ColorSet(&col_tempin, GREEN);
  ColorSet(&col_tempout, BLUE);

  UserText.hide_t = false;
  UserText.carnum = 7;

  memset(UserText.carname, 0, strlen(UserText.carname));
  strcat(UserText.carname, "Вагон ");
}

/***************************************************************************************/
void getTimeChar(char *array)
{
  DateTime now = RTC.getTime();

  uint8_t TimGMT = Clock.hour - CFG.gmt;
  
  if (TimGMT >= 24)
  {
    TimGMT = TimGMT - 24;
  }
  
  // array[0] = now.hour / 10 + '0';
  // array[1] = now.hour % 10 + '0';
  array[0] = TimGMT / 10 + '0';
  array[1] = TimGMT % 10 + '0';
  array[2] = now.minute / 10 + '0';
  array[3] = now.minute % 10 + '0';
  array[4] = now.second / 10 + '0';
  array[5] = now.second % 10 + '0';
  array[6] = '\0';
}
/**********************************************************************************/

void getDateChar(char *array)
{
  DateTime now = RTC.getTime();

  int8_t bytes[4];
  byte amount;
  uint16_t year;
  year = now.year;

  for (byte i = 0; i < 4; i++)
  {                       //>
    bytes[i] = year % 10; // записываем остаток в буфер
    year /= 10;           // "сдвигаем" число
    if (year == 0)
    {             // если число закончилось
      amount = i; // запомнили, сколько знаков
      break;
    }
  } // массив bytes хранит цифры числа data в обратном порядке!

  array[0] = now.date / 10 + '0';
  array[1] = now.date % 10 + '0';
  array[2] = now.month / 10 + '0';
  array[3] = now.month % 10 + '0';
  array[4] = (char)bytes[1] + '0';
  array[5] = (char)bytes[0] + '0';
  array[6] = '\0';
}
/*****************************************************************************************/

//========================== Sending GPS Protocol  =========================
void Send_GPSdata()
{
  getTimeChar(CFG.time);
  getDateChar(CFG.date);

  unsigned int crc = 0;

  char buf_crc[256] = "";
  char xml[256] = "";

  memset(buf_crc, 0, strlen(buf_crc));
  memset(xml, 0, strlen(xml));

  if (CFG.gmt <= 0)
  {
    strcat(buf_crc, "<gmt>");
  }
  else
    strcat(buf_crc, "<gmt>+");

  itoa(CFG.gmt, buf_crc + strlen(buf_crc), DEC);
  strcat(buf_crc, "</gmt>\r\n");
  strcat(buf_crc, "<time>");
  strcat(buf_crc, CFG.time);
  strcat(buf_crc, "</time>\r\n");
  strcat(buf_crc, "<date>");
  strcat(buf_crc, CFG.date);
  strcat(buf_crc, "</date>\r\n");
  strcat(buf_crc, "<lat></lat>\r\n");
  strcat(buf_crc, "<lon></lon>\r\n");
  strcat(buf_crc, "<speed></speed>\r\n");

  strcat(buf_crc, "<temp1>");
  if (HCONF.dsT1 <= -100 or HCONF.dsT1 == 85)
  {
    strcat(buf_crc, "N/A");
    strcat(buf_crc, "</temp1>\r\n");
  }
  else
  {
    itoa(HCONF.dsT1, buf_crc + strlen(buf_crc), DEC);
    strcat(buf_crc, "</temp1>\r\n");
  }

  strcat(buf_crc, "<temp2>");
  if (HCONF.dsT2 <= -100 or HCONF.dsT2 == 85)
  {
    strcat(buf_crc, "N/A");
    strcat(buf_crc, "</temp2>\r\n");
  }
  else
  {
    itoa(HCONF.dsT2, buf_crc + strlen(buf_crc), DEC);
    strcat(buf_crc, "</temp2>\r\n");
  }

  crc = CRC16_mb(buf_crc, strlen(buf_crc));

  strcat(xml, "<gps_data>\r\n");
  strcat(xml, buf_crc);
  strcat(xml, "<gps_crc>");
  char crc_temp[5] = {0};
  sprintf(crc_temp, "%04X", crc);
  strcat(xml, crc_temp);
  // itoa(crc, xml + strlen(xml), HEX);
  strcat(xml, "</gps_crc>\r\n");
  strcat(xml, "</gps_data>");

  Serial2.println(xml);
  Serial2.println();
}
//=========================================================================

//========================== Sending IT Protocol  =========================
// Extended Board
void Send_ITdata(uint8_t adr)
{
  unsigned int crc;
  char buf_crc[4096] = "";
  memset(buf_crc, 0, strlen(buf_crc));
  char xml[4096] = "";
  memset(xml, 0, strlen(xml));

  switch (adr)
  {
  case 1:
    strcat(buf_crc, "<adr id=\"1\">\r\n");
    break;
  case 2:
    strcat(buf_crc, "<adr id=\"2\">\r\n");
    break;
  case 3:
    strcat(buf_crc, "<adr id=\"3\">\r\n");
    break;
  default:
    break;
  }

  strcat(buf_crc, "<cabin_mode>1</cabin_mode>\r\n");
  strcat(buf_crc, "<TsizeDx>\r\n");
  strcat(buf_crc, "<X>128</X>\r\n");
  strcat(buf_crc, "<Y>64</Y>\r\n");
  strcat(buf_crc, "</TsizeDx>\r\n");
  strcat(buf_crc, "<TsizeMx>\r\n");
  strcat(buf_crc, "<X>2</X>\r\n");
  strcat(buf_crc, "<Y>2</Y>\r\n");
  strcat(buf_crc, "</TsizeMx>\r\n");
  strcat(buf_crc, "<TsizeLx>\r\n");
  strcat(buf_crc, "<X>256</X>\r\n");
  strcat(buf_crc, "<Y>128</Y>\r\n");
  strcat(buf_crc, "</TsizeLx>\r\n");
  strcat(buf_crc, "<rgb>1</rgb>\r\n");
  strcat(buf_crc, "<rgb_balance>\r\n");
  strcat(buf_crc, "<R>0</R>\r\n");
  strcat(buf_crc, "<G>0</G>\r\n");
  strcat(buf_crc, "<B>0</B>\r\n");
  strcat(buf_crc, "</rgb_balance>\r\n");
  strcat(buf_crc, "<min_bright>");
  itoa(HCONF.bright, buf_crc + strlen(buf_crc), DEC);
  strcat(buf_crc, "</min_bright>\r\n");
  strcat(buf_crc, "<max_bright>100</max_bright>\r\n");
  strcat(buf_crc, "<auto_bright>0</auto_bright>\r\n");
  strcat(buf_crc, "<zones>6</zones>\r\n");
  strcat(buf_crc, "\r\n");

  //=============== ZONE 1 ================
  strcat(buf_crc, "<zone id=\"1\">\r\n");
  strcat(buf_crc, "<startX>1</startX>\r\n");
  strcat(buf_crc, "<startY>1</startY>\r\n");
  strcat(buf_crc, "<size>\r\n");
  strcat(buf_crc, "<X>74</X>\r\n");
  strcat(buf_crc, "<Y>16</Y>\r\n");
  strcat(buf_crc, "</size>\r\n");
  strcat(buf_crc, "<color>\r\n");
  // Carnum Color
  // ColorWrite(buf_crc, &col_carnum);
  // Carnum color END
  strcat(buf_crc, "</color>\r\n");
  strcat(buf_crc, "<bgcolor>\r\n");
  strcat(buf_crc, "<R>0</R>\r\n");
  strcat(buf_crc, "<G>0</G>\r\n");
  strcat(buf_crc, "<B>0</B>\r\n");
  strcat(buf_crc, "</bgcolor>\r\n");
  strcat(buf_crc, "<mode>0</mode>\r\n");
  strcat(buf_crc, "<align>center</align>\r\n");
  strcat(buf_crc, "<text>");
  strcat(buf_crc, "$textbs");
  strcat(buf_crc, "</text>\r\n");
  strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
  strcat(buf_crc, "</zone>\r\n");
  strcat(buf_crc, "\r\n");
  //=======================================

  //=============== ZONE 2 ================
  strcat(buf_crc, "<zone id=\"2\">\r\n");
  strcat(buf_crc, "<startX>80</startX>\r\n");
  strcat(buf_crc, "<startY>1</startY>\r\n");
  strcat(buf_crc, "<size>\r\n");
  strcat(buf_crc, "<X>49</X>\r\n");
  strcat(buf_crc, "<Y>16</Y>\r\n");
  strcat(buf_crc, "</size>\r\n");
  strcat(buf_crc, "<color>\r\n");
  // Time Color
  ColorWrite(buf_crc, &col_time);
  // Time Color END
  strcat(buf_crc, "</color>\r\n");
  strcat(buf_crc, "<bgcolor>\r\n");
  strcat(buf_crc, "<R>0</R>\r\n");
  strcat(buf_crc, "<G>0</G>\r\n");
  strcat(buf_crc, "<B>0</B>\r\n");
  strcat(buf_crc, "</bgcolor>\r\n");
  strcat(buf_crc, "<mode>0</mode>\r\n");
  strcat(buf_crc, "<align>center</align>\r\n");
  strcat(buf_crc, "<text>$time</text>\r\n");
  strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
  strcat(buf_crc, "</zone>\r\n");
  strcat(buf_crc, "\r\n");
  //=======================================

  //=============== ZONE 3 ================
  strcat(buf_crc, "<zone id=\"3\">\r\n");
  strcat(buf_crc, "<startX>1</startX>\r\n");
  strcat(buf_crc, "<startY>17</startY>\r\n");
  strcat(buf_crc, "<size>\r\n");
  strcat(buf_crc, "<X>74</X>\r\n");
  strcat(buf_crc, "<Y>16</Y>\r\n");
  strcat(buf_crc, "</size>\r\n");
  strcat(buf_crc, "<color>\r\n");
  ColorWrite(buf_crc, &col_carnum);
  strcat(buf_crc, "</color>\r\n");
  strcat(buf_crc, "<bgcolor>\r\n");
  strcat(buf_crc, "<R>0</R>\r\n");
  strcat(buf_crc, "<G>0</G>\r\n");
  strcat(buf_crc, "<B>0</B>\r\n");
  strcat(buf_crc, "</bgcolor>\r\n");
  strcat(buf_crc, "<mode>0");
  strcat(buf_crc, "</mode>\r\n");
  strcat(buf_crc, "<align>center</align>\r\n");
  strcat(buf_crc, "<direction>RTL</direction>\r\n");
  strcat(buf_crc, "<speed>10");
  strcat(buf_crc, "</speed>\r\n");
  strcat(buf_crc, "<text>");
  strcat(buf_crc, "$route");
  strcat(buf_crc, "</text>\r\n");
  strcat(buf_crc, "<font>[16=0+14+2]2+medium+condensed+regular.font</font>\r\n");
  strcat(buf_crc, "</zone>\r\n");
  strcat(buf_crc, "\r\n");
  //=======================================

  //=============== ZONE 4 ================
  strcat(buf_crc, "<zone id=\"4\">\r\n");
  strcat(buf_crc, "<startX>80</startX>\r\n");
  strcat(buf_crc, "<startY>17</startY>\r\n");
  strcat(buf_crc, "<size>\r\n");
  strcat(buf_crc, "<X>49</X>\r\n");
  strcat(buf_crc, "<Y>8</Y>\r\n");
  strcat(buf_crc, "</size>\r\n");
  strcat(buf_crc, "<color>\r\n");
  // Date Color Start
  ColorWrite(buf_crc, &col_date);
  // Date Color END
  strcat(buf_crc, "</color>\r\n");
  strcat(buf_crc, "<bgcolor>\r\n");
  strcat(buf_crc, "<R>0</R>\r\n");
  strcat(buf_crc, "<G>0</G>\r\n");
  strcat(buf_crc, "<B>0</B>\r\n");
  strcat(buf_crc, "</bgcolor>\r\n");
  strcat(buf_crc, "<mode>0</mode>\r\n");
  strcat(buf_crc, "<align>center</align>\r\n");
  strcat(buf_crc, "<text>$dd $mm1</text>\r\n");
  strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
  strcat(buf_crc, "</zone>\r\n");
  strcat(buf_crc, "\r\n");
  //=======================================

  //=============== ZONE 5 ================
  strcat(buf_crc, "<zone id=\"5\">\r\n");
  strcat(buf_crc, "<startX>80</startX>\r\n");
  strcat(buf_crc, "<startY>26</startY>\r\n");
  strcat(buf_crc, "<size>\r\n");
  strcat(buf_crc, "<X>24</X>\r\n");
  strcat(buf_crc, "<Y>8</Y>\r\n");
  strcat(buf_crc, "</size>\r\n");
  strcat(buf_crc, "<color>\r\n");
  // TempIN Color Start
  ColorWrite(buf_crc, &col_tempin);
  // TempIN Color END
  strcat(buf_crc, "</color>\r\n");
  strcat(buf_crc, "<bgcolor>\r\n");
  strcat(buf_crc, "<R>0</R>\r\n");
  strcat(buf_crc, "<G>0</G>\r\n");
  strcat(buf_crc, "<B>0</B>\r\n");
  strcat(buf_crc, "</bgcolor>\r\n");
  strcat(buf_crc, "<mode>0</mode>\r\n");
  strcat(buf_crc, "<align>center</align>\r\n");
  strcat(buf_crc, "<text>$temp1°C</text>\r\n");
  strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
  strcat(buf_crc, "</zone>\r\n");
  strcat(buf_crc, "\r\n");
  //=======================================

  //=============== ZONE 5 ================
  strcat(buf_crc, "<zone id=\"6\">\r\n");
  strcat(buf_crc, "<startX>105</startX>\r\n");
  strcat(buf_crc, "<startY>26</startY>\r\n");
  strcat(buf_crc, "<size>\r\n");
  strcat(buf_crc, "<X>24</X>\r\n");
  strcat(buf_crc, "<Y>8</Y>\r\n");
  strcat(buf_crc, "</size>\r\n");
  strcat(buf_crc, "<color>\r\n");
  // TempOUT Color Start
  ColorWrite(buf_crc, &col_tempout);
  // TempOUT Color END
  strcat(buf_crc, "</color>\r\n");
  strcat(buf_crc, "<bgcolor>\r\n");
  strcat(buf_crc, "<R>0</R>\r\n");
  strcat(buf_crc, "<G>0</G>\r\n");
  strcat(buf_crc, "<B>0</B>\r\n");
  strcat(buf_crc, "</bgcolor>\r\n");
  strcat(buf_crc, "<mode>0</mode>\r\n");
  strcat(buf_crc, "<align>center</align>\r\n");
  strcat(buf_crc, "<text>$temp2°C</text>\r\n");
  strcat(buf_crc, "<font>[8=0+7+1]1+thin+condensed+regular.font</font>\r\n");
  strcat(buf_crc, "</zone>\r\n");
  strcat(buf_crc, "\r\n");
  //=======================================

  strcat(buf_crc, "</adr>\r\n");
  crc = CRC16_mb(buf_crc, strlen(buf_crc));

  strcat(xml, "<extboard_data>\r\n");
  strcat(xml, buf_crc);
  strcat(xml, "<extboard_crc>");

  char crc_temp[5] = {0};
  sprintf(crc_temp, "%04X", crc);
  strcat(xml, crc_temp);
  // itoa(crc, xml + strlen(xml), HEX);
  strcat(xml, "</extboard_crc>\r\n");
  strcat(xml, "</extboard_data>");
  Serial2.println(xml);
  Serial2.println();
}
//=========================================================================

//========================== Sending BS Protocol  =========================
// Internal Board
void Send_BSdata()
{
  // memset(UserText.carname, 0, strlen(UserText.carname));
  unsigned int crc = 0;

  char buf[1024] = "";
  char xml[1024] = "";

  strcat(buf, "<adr id=\"1\">\r\n");

  strcat(buf, "<color_text>");
  char buf_col[9] = {0};

  sprintf(buf_col, "%00006X", col_wc1.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_text>\r\n");
  strcat(buf, "<color_date>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_date.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_date>\r\n");
  strcat(buf, "<color_day>FF0000</color_day>\r\n");
  // memset(buf_col, 0, 7);
  // sprintf(buf_col, "%00006X", col_day.hex);
  // strcat(buf, buf_col);
  // strcat(buf, "</color_day>\r\n");
  strcat(buf, "<color_time>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_time.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_time>\r\n");
  strcat(buf, "<color_temp_in>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_tempin.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_temp_in>\r\n");
  strcat(buf, "<color_temp_out>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_tempout.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_temp_out>\r\n");
  strcat(buf, "<color_route>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_carnum.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_route>\r\n");
  strcat(buf, "<color_speed>FF0000");
  strcat(buf, "</color_speed>\r\n");
  strcat(buf, "<route_name>");
  if (UserText.hide_t == false)
  {
    strcat(buf, UserText.carname);
    strcat(buf, " ");
    itoa(UserText.carnum, buf + strlen(buf), DEC);
  }
  else
  {
    strcat(buf, UserText.carname);
  }
  strcat(buf, "</route_name>\r\n");
  strcat(buf, "<text_bs>Туалет");
  strcat(buf, "</text_bs>\r\n");
  strcat(buf, "</adr>\r\n");

  strcat(buf, "<adr id=\"2\">\r\n");
  strcat(buf, "<color_text>");

  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_wc2.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_text>\r\n");
  strcat(buf, "<color_date>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_date.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_date>\r\n");
  strcat(buf, "<color_day>FF0000</color_day>\r\n");
  // memset(buf_col, 0, 7);
  // sprintf(buf_col, "%00006X", col_day.hex);
  // strcat(buf, buf_col);
  // strcat(buf, "</color_day>\r\n");
  strcat(buf, "<color_time>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_time.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_time>\r\n");
  strcat(buf, "<color_temp_in>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_tempin.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_temp_in>\r\n");
  strcat(buf, "<color_temp_out>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_tempout.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_temp_out>\r\n");
  strcat(buf, "<color_route>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_carnum.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_route>\r\n");
  strcat(buf, "<color_speed>FF0000");
  strcat(buf, "</color_speed>\r\n");
  strcat(buf, "<route_name>");
  if (UserText.hide_t == false)
  {
    strcat(buf, UserText.carname);
    strcat(buf, " ");
    itoa(UserText.carnum, buf + strlen(buf), DEC);
  }
  else
  {
    strcat(buf, UserText.carname);
  }
  strcat(buf, "</route_name>\r\n");
  strcat(buf, "<text_bs>Туалет");
  strcat(buf, "</text_bs>\r\n");
  strcat(buf, "</adr>\r\n");

  // CRC
  crc = CRC16_mb(buf, strlen(buf));
  strcat(xml, "<intboard_data>\r\n");
  strcat(xml, buf);
  strcat(xml, "<intboard_crc>");
  char crc_temp[5] = {0};
  sprintf(crc_temp, "%04X", crc);
  strcat(xml, crc_temp);
  strcat(xml, "</intboard_crc>\r\n");
  strcat(xml, "</intboard_data>");
  strcat(xml, "\r\n");

  Serial2.print(xml);
  Serial2.println();
}
//=========================================================================

/*****************************************************************************************/
// msg1 - TOP zone  (YELLOW)
// msg2 - BOT zone  (WHITE)
void Send_BS_UserData(char *msg1, char *msg2)
{
  unsigned int crc = 0;

  char buf[1024] = "";
  char xml[1024] = "";

  strcat(buf, "<adr id=\"1\">\r\n");

  strcat(buf, "<color_text>");
  char buf_col[9] = {0};

  sprintf(buf_col, "%00006X", 0xFFFF00);
  strcat(buf, buf_col);
  strcat(buf, "</color_text>\r\n");
  strcat(buf, "<color_date>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_date.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_date>\r\n");
  strcat(buf, "<color_day>FF0000</color_day>\r\n");
  // memset(buf_col, 0, 7);
  // sprintf(buf_col, "%00006X", col_day.hex);
  // strcat(buf, buf_col);
  // strcat(buf, "</color_day>\r\n");
  strcat(buf, "<color_time>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_time.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_time>\r\n");
  strcat(buf, "<color_temp_in>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_tempin.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_temp_in>\r\n");
  strcat(buf, "<color_temp_out>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_tempout.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_temp_out>\r\n");
  strcat(buf, "<color_route>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", 0xFFFFFF);
  strcat(buf, buf_col);
  strcat(buf, "</color_route>\r\n");
  strcat(buf, "<color_speed>FF0000");
  strcat(buf, "</color_speed>\r\n");

  strcat(buf, "<route_name>");
  strcat(buf, msg2);
  strcat(buf, "</route_name>\r\n");

  strcat(buf, "<text_bs>");
  strcat(buf, msg1);
  strcat(buf, "</text_bs>\r\n");

  strcat(buf, "</adr>\r\n");

  strcat(buf, "<adr id=\"2\">\r\n");
  strcat(buf, "<color_text>");

  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", 0xFFFF00);
  strcat(buf, buf_col);
  strcat(buf, "</color_text>\r\n");
  strcat(buf, "<color_date>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_date.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_date>\r\n");
  strcat(buf, "<color_day>FF0000</color_day>\r\n");
  // memset(buf_col, 0, 7);
  // sprintf(buf_col, "%00006X", col_day.hex);
  // strcat(buf, buf_col);
  // strcat(buf, "</color_day>\r\n");
  strcat(buf, "<color_time>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_time.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_time>\r\n");
  strcat(buf, "<color_temp_in>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_tempin.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_temp_in>\r\n");
  strcat(buf, "<color_temp_out>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", col_tempout.hex);
  strcat(buf, buf_col);
  strcat(buf, "</color_temp_out>\r\n");
  strcat(buf, "<color_route>");
  memset(buf_col, 0, 9);
  sprintf(buf_col, "%00006X", 0xFFFFFF);
  strcat(buf, buf_col);
  strcat(buf, "</color_route>\r\n");
  strcat(buf, "<color_speed>FF0000");
  strcat(buf, "</color_speed>\r\n");
  strcat(buf, "<route_name>");
  strcat(buf, msg2);
  strcat(buf, "</route_name>\r\n");
  strcat(buf, "<text_bs>");
  strcat(buf, msg1);
  strcat(buf, "</text_bs>\r\n");
  strcat(buf, "</adr>\r\n");

  // CRC
  crc = CRC16_mb(buf, strlen(buf));
  strcat(xml, "<intboard_data>\r\n");
  strcat(xml, buf);
  strcat(xml, "<intboard_crc>");
  char crc_temp[5] = {0};
  sprintf(crc_temp, "%04X", crc);
  strcat(xml, crc_temp);
  strcat(xml, "</intboard_crc>\r\n");
  strcat(xml, "</intboard_data>");
  strcat(xml, "\r\n");

  Serial2.print(xml);
  Serial2.println();
}
/*****************************************************************************************/

//=========================================================================

//======================= CRC Check summ calculators  =====================
unsigned int CRC16_mb(char *buf, int len)
{
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++)
  {
    crc ^= (unsigned int)buf[pos]; // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--)
    { // Loop over each bit
      if ((crc & 0x0001) != 0)
      {            // If the LSB is set
        crc >>= 1; // Shift right and XOR 0xA001
        crc ^= 0xA001;
        // crc ^= 0x8005;
      }
      else         // Else LSB is not set
        crc >>= 1; // Just shift right
    }
  }
  return crc;
}
//=========================================================================