#include "Config.h"

#include "WF.h"
#include "FileConfig.h"
#include "HTTP.h"

#define DEBUG // Debug control ON
//======================================================================

//=========================== GLOBAL VARIABLES =========================
uint8_t sec_cnt = 0;
char buf[32] = {0}; // buffer for send message
char umsg[30];      // buffer for user message
//======================================================================

//================================ OBJECTs =============================
Audio Amplifier;
MicroDS3231 RTC;

OneWire oneWire1(T1);
OneWire oneWire2(T2);
DallasTemperature ds18b20_1(&oneWire1);
DallasTemperature ds18b20_2(&oneWire2);

TaskHandle_t TaskCore_0;
TaskHandle_t TaskCore_1;
//=======================================================================

//============================== STRUCTURES =============================
DateTime Clock;
GlobalConfig CFG;
HardwareConfig HCONF;
Flag STATE;

color col_carnum;
color col_runtext;
color col_time;
color col_date;
color col_tempin;
color col_tempout;

UserData UserText;
//=======================================================================

//======================    FUNCTION PROTOTYPS     ======================
void HandlerCore0(void *pvParameters);
void HandlerCore1(void *pvParameters);
void GetDSData(void);
void UART_Recieve_Data();
void SayTimeData();
//=======================================================================

//=======================================================================
static uint8_t DS_dim(uint8_t i)
{
    return (i < 7) ? ((i == 1) ? 28 : ((i & 1) ? 30 : 31)) : ((i & 1) ? 31 : 30);
}
//=======================================================================
// Pinned to Core 0. Network Stack Handler
void HandlerCore0(void *pvParameters)
{
    Serial.print("Task0 running on core ");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        // HandleClient();
        Amplifier.loop();
        UART_Recieve_Data();
    }
}
// Pinned to Core 1.
void HandlerCore1(void *pvParameters)
{
    Serial.print("Task1 running on core ");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        sec_cnt++;
        GetDSData();
        DebugInfo();

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
//=======================================================================

//=======================       S E T U P       =========================
void setup()
{
    CFG.fw = "0.0.5";
    CFG.fwdate = "27.04.2024";

    Serial.begin(UARTSpeed);
    // Serial1.begin(115200,SERIAL_8N1,RX1_PIN, TX1_PIN);
    Serial2.begin(RSSpeed);
    SystemInit();
    // SPIFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    // RTC INIT
    RTC.begin();
    // Low battery / RTC battery crash / Set time compilations
    if (RTC.lostPower())
    {
        RTC.setTime(COMPILE_TIME);
    }
    Clock = RTC.getTime();
    Serial.println(F("RTC...Done"));

    ds18b20_1.begin();
    Serial.println(F("Sensor T1...Done"));
    delay(20);
    ds18b20_2.begin();
    Serial.println(F("Sensor T2...Done"));
    delay(20);

    pinMode(WC1, INPUT_PULLUP);
    pinMode(WC2, INPUT_PULLUP);

    // LoadConfig(); // Load configuration from config.json files
    CFG.WiFiMode = 0;
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(CFG.Ssid.c_str(), CFG.Password.c_str());
    while (WiFi.status() != WL_CONNECTED)
        delay(1500);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("");
    // WIFIinit();
    delay(500);

    Amplifier.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    Amplifier.setVolume(10);

    // String buf = "/sound/NM/";
    // buf += "d1.mp3";
    // Amplifier.connecttoFS(SPIFFS, buf.c_str());

    Serial.println(F("DAC PCM Amplifier...Done"));

    // HTTPinit(); // HTTP server initialisation
    // delay(1000);
    SayTimeData();

    xTaskCreatePinnedToCore(
        HandlerCore0,
        "TaskCore_0",
        10000,
        NULL,
        1,
        &TaskCore_0,
        0);
    delay(500);

    xTaskCreatePinnedToCore(
        HandlerCore1,
        "TaskCore_1",
        2048,
        NULL,
        1,
        &TaskCore_1,
        1);
    delay(500);
}
//=======================================================================

//=======================        L O O P        =========================
void loop()
{
    //   ButtonHandler();
}
//=======================================================================

//=========================================================================
// Get Data from DS18B20 Sensor
void GetDSData()
{
    ds18b20_1.requestTemperatures();
    HCONF.dsT1 = ds18b20_1.getTempCByIndex(0);
    HCONF.dsT1 = HCONF.dsT1 + HCONF.T1_offset;
    ds18b20_2.requestTemperatures();
    HCONF.dsT2 = ds18b20_2.getTempCByIndex(0);
    HCONF.dsT2 = HCONF.dsT2 + HCONF.T2_offset;
}
//=========================================================================

//=========================================================================
// Send  data (время, темп). Every 5 seconds
void Task_2s()
{
    if (sec_cnt == 2)
    {
        if (STATE.cnt_Supd < 2 && STATE.StaticUPD)
            STATE.cnt_Supd++;
        else
        {
            STATE.StaticUPD = false;
            STATE.cnt_Supd = 0;
        }
        if (STATE.StaticUPD && STATE.cnt_Supd < 2)
        {
            SendXMLDataS();
        }

        if (!STATE.DUPDBlock)
        {
            SendXMLDataD();
        }
        sec_cnt = 0;
    }
}
//=========================================================================

//=========================================================================
void ButtonHandler()
{
    Serial.println("#### FACTORY RESET ####");
    SystemFactoryReset();
    sprintf(umsg, "Сброшено");
    SendXMLUserData(umsg);
    // SaveConfig();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    Serial.println("#### SAVE DONE ####");
    ESP.restart();
}
//=========================================================================

void UART_Recieve_Data()
{
    if (Serial.available())
    {
        // put streamURL in serial monitor
        // audio.stopSong();
        String r = Serial.readString();
        bool block_st = false;
        r.trim();
        if (r.length() > 3)
        {
            // Amplifier.connecttohost(r.c_str());
            Amplifier.connecttoFS(SPIFFS, r.c_str());
        }
        else
        {
            Amplifier.setVolume(r.toInt());
        }
        log_i("free heap=%i", ESP.getFreeHeap());
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void SayTimeData()
{
    Amplifier.loop();

    String buf = "/sound/S/curtime.mp3";
    Clock = RTC.getTime();
    uint8_t Hour = Clock.hour;

    Amplifier.connecttoFS(SPIFFS, buf.c_str());
    Amplifier.loop();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    buf.clear();

    buf = "/sound/H/";
    buf += "ch";
    buf += Hour;
    buf += ".mp3";
    Amplifier.connecttoFS(SPIFFS, buf.c_str());
    Amplifier.loop();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    buf.clear();

    uint8_t Min = Clock.minute;
    buf = "/sound/M/";
    buf += "min";
    buf += Min;
    buf += ".mp3";
    Amplifier.connecttoFS(SPIFFS, buf.c_str());
    Amplifier.loop();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    buf.clear();
}