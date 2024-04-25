#include "Config.h"

#include "WF.h"

#define DEBUG // Debug control ON
//======================================================================

//=========================== GLOBAL VARIABLES =========================
uint8_t sec_cnt = 0;
char buf[32] = {0}; // buffer for send message
char umsg[30];      // buffer for user message
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
        vTaskDelay(10 / portTICK_PERIOD_MS);
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
    CFG.fw = "0.0.1";
    CFG.fwdate = "25.04.2024";

    Serial.begin(UARTSpeed);
    Serial2.begin(RSSpeed);

    SystemInit();
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

    // WIFIinit();
    // delay(1000);

    // HTTPinit(); // HTTP server initialisation
    // delay(1000);

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
