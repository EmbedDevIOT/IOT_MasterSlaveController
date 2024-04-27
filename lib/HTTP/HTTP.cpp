#include "HTTP.h"

WebServer HTTP(80);

///////////////////////////////
// Initialisation WebServer  //
///////////////////////////////
void HTTPinit()
{
  HTTP.begin();
  // ElegantOTA.begin(&HTTP); // Start ElegantOTA
  HTTP.on("/update.json", UpdateData);
  HTTP.on("/SysUPD", SystemUpdate);
  HTTP.on("/TextUPD", TextUpdate);
  HTTP.on("/ColUPD", ColorUpdate);
  HTTP.on("/SNUPD", SerialNumberUPD);
  HTTP.on("/WiFiUPD", SaveSecurity);
  HTTP.on("/BRBT", Restart); // Restart MCU
  HTTP.on("/FW", ShowSystemInfo);
  HTTP.on("/BFRST", FactoryReset);               // Set default parametrs.
  HTTP.onNotFound([]() {                         // Event "Not Found"
    if (!handleFileRead(HTTP.uri()))             // If function  handleFileRead (discription bellow) returned false in request for file searching in file syste
      HTTP.send(404, "text/plain", "Not Found"); // return message "File isn't found" error state 404 (not found )
  });
}

////////////////////////////////////////
// File System work handler           //
////////////////////////////////////////
bool handleFileRead(String path)
{
  if (path.endsWith("/"))
    path += "index.html";                    // Если устройство вызывается по корневому адресу, то должен вызываться файл index.html (добавляем его в конец адреса)
  String contentType = getContentType(path); // С помощью функции getContentType (описана ниже) определяем по типу файла (в адресе обращения) какой заголовок необходимо возвращать по его вызову
  if (SPIFFS.exists(path))
  {                                                   // Если в файловой системе существует файл по адресу обращения
    File file = SPIFFS.open(path, "r");               //  Открываем файл для чтения
    size_t sent = HTTP.streamFile(file, contentType); //  Выводим содержимое файла по HTTP, указывая заголовок типа содержимого contentType
    file.close();                                     //  Закрываем файл
    return true;                                      //  Завершаем выполнение функции, возвращая результатом ее исполнения true (истина)
  }
  return false; // Завершаем выполнение функции, возвращая результатом ее исполнения false (если не обработалось предыдущее условие)
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Функция, возвращающая необходимый заголовок типа содержимого в зависимости от расширения файла //
////////////////////////////////////////////////////////////////////////////////////////////////////
String getContentType(String filename)
{
  if (filename.endsWith(".html"))
    return "text/html"; // Если файл заканчивается на ".html", то возвращаем заголовок "text/html" и завершаем выполнение функции
  else if (filename.endsWith(".css"))
    return "text/css"; // Если файл заканчивается на ".css", то возвращаем заголовок "text/css" и завершаем выполнение функции
  else if (filename.endsWith(".js"))
    return "application/javascript"; // Если файл заканчивается на ".js", то возвращаем заголовок "application/javascript" и завершаем выполнение функции
  else if (filename.endsWith(".png"))
    return "image/png"; // Если файл заканчивается на ".png", то возвращаем заголовок "image/png" и завершаем выполнение функции
  else if (filename.endsWith(".ttf"))
    return "font/ttf"; // Если файл заканчивается на ".png", то возвращаем заголовок "image/png" и завершаем выполнение функции
  else if (filename.endsWith(".bmp"))
    return "image/bmp";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg"; // Если файл заканчивается на ".jpg", то возвращаем заголовок "image/jpg" и завершаем выполнение функции
  else if (filename.endsWith(".gif"))
    return "image/gif"; // Если файл заканчивается на ".gif", то возвращаем заголовок "image/gif" и завершаем выполнение функции
  else if (filename.endsWith(".svg"))
    return "image/svg+xml"; 
  else if (filename.endsWith(".ico"))
    return "image/x-icon"; // Если файл заканчивается на ".ico", то возвращаем заголовок "image/x-icon" и завершаем выполнение функции
  return "text/plain";     // Если ни один из типов файла не совпал, то считаем что содержимое файла текстовое, отдаем соответствующий заголовок и завершаем выполнение функции
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// Time data dynamic update
void UpdateData()
{
  String buf = "{";

  buf += "\"t\":\"";
  buf += ((Clock.hour < 10) ? "0" : "") + String(Clock.hour) + ":" + ((Clock.minute < 10) ? "0" : "") + String(Clock.minute) + "\",";
  buf += "\"d\":\"";
  buf += String(Clock.year) + "-" + ((Clock.month < 10) ? "0" : "") + String(Clock.month) + "-" + ((Clock.date < 10) ? "0" : "") + String(Clock.date) + "\"";
  buf += "}";

  HTTP.send(200, "text/plain", buf);
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void SystemUpdate()
{
  char TempBuf[15];
  char msg[32] = {0};

  struct _sys
  {
    uint8_t H = 0;
    uint8_t M = 0;
    int8_t D = 0;
    int8_t MO = 0;
    int16_t Y = 0;
  } S;

  HTTP.arg("T").toCharArray(TempBuf, 10);
  S.H = atoi(strtok(TempBuf, ":"));
  S.M = atoi(strtok(NULL, ":"));

  HTTP.arg("D").toCharArray(TempBuf, 15);
  S.Y = atoi(strtok(TempBuf, "-"));
  S.MO = atoi(strtok(NULL, "-"));
  S.D = atoi(strtok(NULL, "-"));

  HCONF.T1_offset = HTTP.arg("T1O").toInt();
  HCONF.T2_offset = HTTP.arg("T2O").toInt();
  HCONF.bright = HTTP.arg("BR").toInt();

  Clock.hour = S.H;
  Clock.minute = S.M;
  Clock.year = S.Y;
  Clock.month = S.MO;
  Clock.date = S.D;

#ifndef DEBUG
  sprintf(msg, "Time: %d : %d", S.H, S.M);
  Serial.println(msg);
  sprintf(msg, "DataIN: %0004d.%02d.%02d", S.Y, S.MO, S.D);
  Serial.println(msg);
  sprintf(msg, "DataRTC: %0004d.%02d.%02d", Clock.year, Clock.month, Clock.date);
  Serial.println(msg);
  Serial.printf("T1_OFFset: %d", HCONF.T1_offset);
  Serial.println();
  Serial.printf("T2_OFFset: %d", HCONF.T2_offset);
  Serial.println();
  Serial.printf("Brigh: %d", HCONF.bright);
  Serial.println();
#endif

  RTC.setTime(Clock);
  SaveConfig();
  STATE.StaticUPD = true;
  STATE.cnt_Supd = 0;

  // ShowLoadJSONConfig();

  // Show Led state (add function)
  Serial.println("System Update");
  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void TextUpdate()
{
  char TempBuf[10];
  char msg[512] = {0};

  struct _txt
  {
    char TN[17];     // car_name
    char TI[1024];   // text info
    uint8_t TNU = 0; // car num
    bool SW = true;
    bool SWH = false;
    uint8_t SP = 0;
  } T;

  HTTP.arg("TN").toCharArray(T.TN, 17);
  T.TNU = HTTP.arg("TNU").toInt();
  HTTP.arg("TI").toCharArray(T.TI, 1024);
  T.SW = HTTP.arg("SW").toInt();
  T.SWH = HTTP.arg("SWH").toInt();
  T.SP = HTTP.arg("SP").toInt();

  memset(UserText.carname, 0, strlen(UserText.carname));
  strcat(UserText.carname, T.TN);

  memset(UserText.runtext, 0, strlen(UserText.runtext));
  strcat(UserText.runtext, T.TI);

  UserText.run_mode = T.SW;
  UserText.speed = T.SP;
  UserText.carnum = T.TNU;

  if (T.SWH != UserText.hide_t)
  {
    UserText.hide_t = T.SWH;
    SendXMLDataD();
  }

  // #ifndef DEBUG
  //   Serial.printf("Name: ");
  //   Serial.printf(T.TN);
  //   Serial.println(msg);

  //   Serial.printf("CarNum: ");
  //   Serial.println(T.TNU);

  //   Serial.printf("Info: ");
  //   Serial.printf(T.TI);
  //   Serial.println(msg);

  //   Serial.printf("RunText: %d", T.SW);
  //   Serial.println();
  //   Serial.printf("Speed: %d", T.SP);
  //   Serial.println();
  // #endif

  // SaveConfig();
  // ShowLoadJSONConfig();

  // Show Led state (add function)

  Serial.println("Text Update");
  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void ColorUpdate()
{
  char TempBuf[10];

  struct _col
  {
    uint8_t CC = HTTP.arg("CC").toInt();
    uint8_t CI = HTTP.arg("CI").toInt();
    uint8_t CT = HTTP.arg("CT").toInt();
    uint8_t CD = HTTP.arg("CD").toInt();
    uint8_t CTI = HTTP.arg("CTI").toInt();
    uint8_t CTO = HTTP.arg("CTO").toInt();
  } C;

  // #ifndef DEBUG
  //   Serial.printf("Color CarNum: %d", C.CC);
  //   Serial.println();
  //   Serial.printf("Color InfoText: %d", C.CI);
  //   Serial.println();
  //   Serial.printf("Color Time: %d", C.CT);
  //   Serial.println();
  //   Serial.printf("Color Date: %d", C.CD);
  //   Serial.println();
  //   Serial.printf("Color TempIN: %d", C.CTI);
  //   Serial.println();
  //   Serial.printf("Color TempOUT: %d", C.CTO);
  //   Serial.println();
  // #endif

  ColorSet(&col_carnum, C.CC);
  ColorSet(&col_runtext, C.CI);
  ColorSet(&col_time, C.CT);
  ColorSet(&col_date, C.CD);
  ColorSet(&col_tempin, C.CTI);
  ColorSet(&col_tempout, C.CTO);

  SaveConfig();
  STATE.StaticUPD = true;
  STATE.cnt_Supd = 0;
  // ShowLoadJSONConfig();

  Serial.println("Сolor Update");
  HTTP.send(200, "text/plain", "OK");
}

/*******************************************************************************************************/
void SerialNumberUPD()
{
  CFG.sn = HTTP.arg("sn").toInt();
  Serial.printf("SN:");
  Serial.println(CFG.sn);
  EEP_Write();
  HTTP.send(200, "text/plain", "Serial Number set");
}
/*******************************************************************************************************/
/*******************************************************************************************************/
void HandleClient()
{
  HTTP.handleClient();
}
/*******************************************************************************************************/
/*******************************************************************************************************/
void SaveSecurity()
{
  char TempBuf[10];

  CFG.APSSID = HTTP.arg("ssid");
  CFG.APPAS = HTTP.arg("pass");

#ifndef DEBUG
  Serial.println("Network name:");
  Serial.print(CFG.APSSID);
  Serial.println();
  Serial.print("Network password:");
  Serial.println(CFG.APPAS);
#endif

  SaveConfig();
  // ShowLoadJSONConfig();

  HTTP.send(200, "text/plain", "OK");
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// ESP Restart
void Restart()
{
  HTTP.send(200, "text/plain", "OK"); // Oтправляем ответ Reset OK
  Serial.println("Restart Core");
  ESP.restart(); // перезагружаем модуль
}
/*******************************************************************************************************/
/*******************************************************************************************************/
void FactoryReset()
{
  HTTP.send(200, "text/plain", "OK"); // Oтправляем ответ Reset OK
  Serial.println("#### FACTORY RESET ####");
  SystemFactoryReset();
  SaveConfig();
  // EEP_Write();
  ShowFlashSave();
  Serial.println("#### SAVE DONE ####");
  ESP.restart(); // перезагружаем модуль
}
/*******************************************************************************************************/
void ShowSystemInfo()
{
  char msg[30];

  Serial.printf("System Information");
  sprintf(msg, "%s.%d", CFG.fw, CFG.sn);
  SendXMLUserData(msg);

  HTTP.send(200, "text/plain", "OK"); // Oтправляем ответ Reset OK
}