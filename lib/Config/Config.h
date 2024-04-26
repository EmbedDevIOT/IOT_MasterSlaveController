#ifndef _Config_H
#define _Config_H

#include <Arduino.h>

// #include <ElegantOTA.h>
#include "HardwareSerial.h"
#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"
#include <microDS3231.h>
#include <ArduinoJson.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define UARTSpeed 115200
#define RSSpeed 115200

#define WiFi_

#define WiFiTimeON 10
#define Client 0
#define AccessPoint 1
#define WorkNET

#define debug(x) Serial.println(x)
// #define DEBUG
// #define I2C_SCAN

#define DISABLE 0
#define ENABLE 1

#define ON 1
#define OFF 0

// GPIO PINs
// Button
#define BTN1 36 // кнопкa 1
#define BTN2 39 // кнопкa 2
#define BTN3 34 // кнопкa 3
#define BTN4 35 // кнопкa 4
#define BTN5 32 // кнопкa 5

// #define CARD 15    // MicroSD set PIN
// #define AIO 26     // Analog input PIN
// #define DIO 27     // Digital input PIN

#define LED_RS 12   // Led State RS Connection
#define LED_WiFi 32 // Led State WiFi Connection
#define LED_ST 14   // User Led

#define T1 4 // Temperature sensor ds18b20 1
#define T2 2 // Temperature sensor ds18b20 2

#define WC1 4 // Pin status control WC 1
#define WC2 2 // Pin status control WC 2

// UART 1 
#define RS_SERIAL1
#ifdef RS_SERIAL1
#define TX1_PIN 33 // UART1_TX
#define RX1_PIN 32 // UART1_RX
#endif
#define DE_RE 35   // DE_MAX13444

// UART 2 
#define GPS_SERIAL2
#ifdef GPS_SERIAL2
#define TX2_PIN 17 // UART2_TX
#define RX2_PIN 16 // UART2_RX
#endif


//=======================================================================
extern MicroDS3231 RTC;
extern DateTime Clock;
//=======================================================================

//========================== ENUMERATION ================================
enum C
{
  RED = 0,
  GREEN,
  BLUE,
  SEA_WAVE,
  BLACK,
  WHITE,
  YELLOW,
  PURPLE
};

enum Clicks
{
  ONE = 1,
  TWO,
  THREE,
};

//=======================================================================

//=========================== GLOBAL CONFIG =============================
struct GlobalConfig
{
  uint16_t sn = 0;
  String fw = ""; // accepts from setup()
  String fwdate = "";
  String chipID = "";
  String MacAdr = "";
  String NTPServer = "pool.ntp.org";
  String APSSID = "0845-0";
  String APPAS = "retra0845zxc";

  String Ssid = "MkT";           // SSID Wifi network
  String Password = "QFCxfXMA3"; // Passwords WiFi network

  int TimeZone = 0;

  char time[7] = "";
  char date[9] = "";

  byte IP1 = 192;
  byte IP2 = 168;
  byte IP3 = 1;
  byte IP4 = 1;
  byte GW1 = 192;
  byte GW2 = 168;
  byte GW3 = 1;
  byte GW4 = 1;
  byte MK1 = 255;
  byte MK2 = 255;
  byte MK3 = 255;
  byte MK4 = 0;

  byte WiFiMode = AccessPoint; // Режим работы WiFi
};
extern GlobalConfig CFG;
//=======================================================================
struct UserData
{
  char carname[17] = "";
  int carnum = 0;
  char runtext[1024] = "";
  bool run_mode = true; // text running
  bool hide_t = false;  // Hidden car number text
  int8_t speed = 20;    // speed running text
};
extern UserData UserText;

struct color
{
  int R;
  int G;
  int B;
};
extern color col_carnum;
extern color col_runtext;
extern color col_time;
extern color col_date;
extern color col_tempin;
extern color col_tempout;
//=======================================================================

//=======================================================================
struct HardwareConfig
{
  int8_t bright = 70; // speed running text
  float dsT1 = 0.0;
  int8_t T1_offset = 0;
  float dsT2 = 0.0;
  int8_t T2_offset = 0;
  uint8_t ERRORcnt = 0;
};
extern HardwareConfig HCONF;
//=======================================================================

//=======================================================================
struct Flag
{
  bool StaticUPD : 1;
  uint8_t cnt_Supd = 0;
  bool DynamicUPD : 1;
  bool DUPDBlock : 1;
  bool IDLE : 1;
  bool LedWiFi : 1;
  bool LedRS : 1;
  bool LedUser : 1;
  bool SaveFlash : 1;
  bool RS : 1;
  bool Debug : 1;
  bool CurDebug : 1;
  bool WiFiEnable : 1;
};
extern Flag STATE;
//============================================================================

//============================================================================
void ColorWrite(char *buf, struct color *C);
void ColorSet(struct color *CS, uint8_t _color);
int GetColorNum(struct color *C);
void UserPresetInit(void);
void SystemInit(void);     //  System Initialisation (variables and structure)
void ShowInfoDevice(void); //  Show information or this Device
void GetChipID(void);
String GetMacAdr();
void DebugInfo(void);
void SystemFactoryReset(void);
void ShowFlashSave(void);
void getTimeChar(char *array);
void getDateChar(char *array);
void SendXMLDataS(void);
void SendXMLDataD(void);
void SendXMLUserData(char *msg);
unsigned int CRC16_mb(char *buf, int len);
//============================================================================
#endif // _Config_H