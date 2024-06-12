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
TaskHandle_t Task1000ms;

HardwareSerial RS485(2);
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
void HandlerTask1000(void *pvParameters);
void SendtoRS485();
void GetDSData(void);
void UART_Recieve_Data();
void Tell_me_CurrentTime();
void Tell_me_CurrentData();
void Tell_me_DoorState(bool state);
//=======================================================================

//=======================================================================
static uint8_t DS_dim(uint8_t i)
{
    return (i < 7) ? ((i == 1) ? 28 : ((i & 1) ? 30 : 31)) : ((i & 1) ? 31 : 30);
}
//=======================================================================

//=======================       S E T U P       =========================
void setup()
{
    CFG.fw = "0.0.8";
    CFG.fwdate = "12.06.2024";

    Serial.begin(UARTSpeed);
    // Serial1.begin(115200,SERIAL_8N1,RX1_PIN, TX1_PIN);
    Serial2.begin(115200,SERIAL_8N1,RX1_PIN, TX1_PIN);
    // Serial2.begin(RSSpeed);
    SystemInit();
    // SPIFFS INIT
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // RTC INIT
    RTC.begin();
    // RTC.setTime(COMPILE_TIME);
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

    LoadConfig();         // Load configuration from config.json files
    ShowLoadJSONConfig(); // Show load configuration

    WIFIinit();
    delay(500);

    Amplifier.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    Amplifier.setVolume(HCONF.volume);

    Serial.println(F("DAC PCM Amplifier...Done"));

    HTTPinit(); // HTTP server initialisation
    delay(1000);

    xTaskCreatePinnedToCore(
        HandlerCore0,
        "TaskCore_0",
        10000,
        NULL,
        1,
        &TaskCore_0,
        0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    // delay(500);

    xTaskCreatePinnedToCore(
        HandlerCore1,
        "TaskCore_1",
        2048,
        NULL,
        1,
        &TaskCore_1,
        1);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    xTaskCreatePinnedToCore(
        HandlerTask1000,
        "Task1000ms",
        10000,
        NULL,
        1,
        &Task1000ms,
        1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    // delay(500);
}
//=======================================================================

//=======================        L O O P        =========================
void loop()
{
    HandleClient();
    //   ButtonHandler();
}
//=======================================================================

//======================    R T O S  T A S K    =========================
// Core 0. Network Stack Handler
void HandlerCore0(void *pvParameters)
{
    Serial.print("Task0 running on core ");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        Amplifier.loop();
        // UART_Recieve_Data();
        if (STATE.TTS)
        {
            Tell_me_CurrentTime();
            Tell_me_CurrentData();
            STATE.TTS = false;
        }

        if (STATE.DSTS)
        {
            Tell_me_DoorState(1);
            STATE.DSTS = false;
        }

        if (STATE.VolumeUPD)
        {
            Amplifier.setVolume(HCONF.volume);
            STATE.VolumeUPD = false;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
// Core 1.
void HandlerCore1(void *pvParameters)
{
    Serial.print("Task1 running on core ");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        Clock = RTC.getTime();
        GetDSData();
        DebugInfo();
        Serial.printf("AMP: %d \r\n", Amplifier.isRunning());

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Core 1. 1000ms
void HandlerTask1000(void *pvParameters)
{
    Serial.print("Task1 running on core ");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        sec_cnt++;
        SendtoRS485();

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
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
void SendtoRS485()
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
            if (r == "time")
            {
                Serial.println("Current Time");
                Tell_me_CurrentTime();
            }
            if (r == "door1")
            {
                Serial.println("DoorState");
                Tell_me_DoorState(true);
            }
            if (r == "door0")
            {
                Serial.println("DoorState");
                Tell_me_DoorState(false);
            }
            if (r == "date")
            {
                Serial.println("Current Date");
                Tell_me_CurrentData();
            }
            if (r == "tellmetime")
            {
                Serial.println("Current Time and Date");
                Tell_me_CurrentTime();
                Tell_me_CurrentData();
            }
        }
        else
        {
            Amplifier.setVolume(r.toInt());
        }
        log_i("free heap=%i", ESP.getFreeHeap());
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void Tell_me_CurrentTime()
{
    String buf = "/sound/S/curtime.mp3";
    if (!Amplifier.isRunning())
    {
        Amplifier.connecttoFS(SPIFFS, buf.c_str());
        while (Amplifier.isRunning())
        {
            Amplifier.loop();
        }
        Amplifier.loop();
    }
    buf.clear();
    vTaskDelay(500 / portTICK_PERIOD_MS);

    Clock = RTC.getTime();
    buf = "/sound/H/";
    buf += "ch";
    buf += Clock.hour;
    buf += ".mp3";
    Serial.printf(buf.c_str());
    Serial.println();

    if (!Amplifier.isRunning())
    {
        Amplifier.connecttoFS(SPIFFS, buf.c_str());
        while (Amplifier.isRunning())
        {
            Amplifier.loop();
        }
        Amplifier.loop();
    }
    buf.clear();
    vTaskDelay(500 / portTICK_PERIOD_MS);

    Clock = RTC.getTime();
    buf = "/sound/Mi/";
    buf += "min";
    buf += Clock.minute;
    buf += ".mp3";
    Serial.printf(buf.c_str());
    Serial.println();

    if (!Amplifier.isRunning())
    {
        Amplifier.connecttoFS(SPIFFS, buf.c_str());
        while (Amplifier.isRunning())
        {
            Amplifier.loop();
        }
        Amplifier.loop();
    }
    buf.clear();
}

void Tell_me_DoorState(bool state)
{
    String buf;
    switch (state)
    {
    case 1:
        buf = "/sound/S/dclose.mp3";
        if (!Amplifier.isRunning())
        {
            Amplifier.connecttoFS(SPIFFS, buf.c_str());
            while (Amplifier.isRunning())
            {
                Amplifier.loop();
            }
            Amplifier.loop();
        }
        buf.clear();
        break;

    case 0:
        buf = "/sound/S/dopen.mp3";
        if (!Amplifier.isRunning())
        {
            Amplifier.connecttoFS(SPIFFS, buf.c_str());
            while (Amplifier.isRunning())
            {
                Amplifier.loop();
            }
            Amplifier.loop();
        }
        buf.clear();
        break;
    default:
        break;
    }
}

void Tell_me_CurrentData()
{
    String buf;
    buf = "/sound/S/today.mp3";

    if (!Amplifier.isRunning())
    {
        Amplifier.connecttoFS(SPIFFS, buf.c_str());
        while (Amplifier.isRunning())
        {
            Amplifier.loop();
        }
        Amplifier.loop();
    }
    buf.clear();
    vTaskDelay(200 / portTICK_PERIOD_MS);

    buf = "/sound/DW/";
    switch (Clock.day)
    {
    case MON: // Monday
        buf += "mon.mp3";
        break;
    case TUE: // Tuesday
        buf += "thu.mp3";
        break;
    case WED: // Wednesday
        buf += "wed.mp3";
        break;
    case THU: // Thursday
        buf += "thu.mp3";
        break;
    case FRI: // Friday
        buf += "fri.mp3";
        break;
    case SAT: // Saturday
        buf += "sat.mp3";
        break;
    case SUN: // Sunday
        buf += "sun.mp3";
        break;
    default:
        break;
    }
    Serial.printf(buf.c_str());
    Serial.println();

    if (!Amplifier.isRunning())
    {
        Amplifier.connecttoFS(SPIFFS, buf.c_str());
        while (Amplifier.isRunning())
        {
            Amplifier.loop();
        }
        Amplifier.loop();
    }
    buf.clear();
    vTaskDelay(200 / portTICK_PERIOD_MS);

    buf = "/sound/NM/";
    buf += "d";
    buf += Clock.date;
    buf += ".mp3";

    if (!Amplifier.isRunning())
    {
        Amplifier.connecttoFS(SPIFFS, buf.c_str());
        while (Amplifier.isRunning())
        {
            Amplifier.loop();
        }
        Amplifier.loop();
    }
    buf.clear();

    buf = "/sound/Mo/";
    switch (Clock.month)
    {
    case YAN:
        buf += "yan.mp3";
        break;
    case FEB:
        buf += "feb.mp3";
        break;
    case MAR:
        buf += "mar.mp3";
        break;
    case APR:
        buf += "apr.mp3";
        break;
    case MAY:
        buf += "may.mp3";
        break;
    case JUN:
        buf += "jun.mp3";
        break;
    case JUL:
        buf += "jul.mp3";
        break;
    case AUG:
        buf += "aug.mp3";
        break;
    case SEP:
        buf += "sep.mp3";
        break;
    case OCTB:
        buf += "oct.mp3";
        break;
    case NOV:
        buf += "nov.mp3";
        break;
    case DECM:
        buf += "dec.mp3";
        break;
    default:
        break;
    }
    Serial.printf(buf.c_str());
    Serial.println();

    if (!Amplifier.isRunning())
    {
        Amplifier.connecttoFS(SPIFFS, buf.c_str());
        while (Amplifier.isRunning())
        {
            Amplifier.loop();
        }
        Amplifier.loop();
    }
    buf.clear();
}
