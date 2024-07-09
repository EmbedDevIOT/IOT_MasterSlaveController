#include "Config.h"

#include "WF.h"
#include "FileConfig.h"
#include "HTTP.h"
#include "keyboard.h"

#define DEBUG // Debug control ON
//======================================================================

//=========================== GLOBAL VARIABLES =========================
uint8_t sec_cnt = 0;

uint8_t menu = 0;     // State menu (levels)
char name_1[25] = ""; // menu message to show TOP zone
char name_2[25] = ""; // menu message to show BOT zone
uint8_t old_bright = 0;
//======================================================================

//================================ OBJECTs =============================
TaskHandle_t TaskCore0_Gen;
TaskHandle_t TaskCore1_Gen;
TaskHandle_t TaskCore0_TSH;
TaskHandle_t TaskCore1_500ms;

Audio Amplifier;
MicroDS3231 RTC;

OneWire oneWire1(T1);
OneWire oneWire2(T2);
DallasTemperature ds18b20_1(&oneWire1);
DallasTemperature ds18b20_2(&oneWire2);

HardwareSerial RS485(2);

PCF8574 I2CBUT(0x20); // object with work I2C ext. (default)
//=======================================================================

//============================== STRUCTURES =============================
DateTime Clock;
GlobalConfig CFG;
HardwareConfig HCONF;
Flag STATE;

color col_carnum;
color col_time;
color col_date;
color col_day;
color col_tempin;
color col_tempout;
color col_wc1;
color col_wc2;
color col_speed;

UserData UserText;
//=======================================================================

//======================    FUNCTION PROTOTYPS     ======================
// - FreeRTOS function --
void HandlerCore0(void *pvParameters);
void HandlerCore1(void *pvParameters);
void HandlerTask500(void *pvParameters);
void HandlerTask1Wire(void *pvParameters);
void I2CExpanderInit();
void ButtonHandler();
void SendtoRS485();
void GetDSData(void);
bool GetWCState(uint8_t num);
void SetColorWC();
void UART_Recieve_Data();
void Tell_me_CurrentTime();
void Tell_me_CurrentData();
void Tell_me_DoorState(bool state);

//=======================================================================
void I2CExpanderInit()
{
    I2CBUT.begin();
    I2CBUT.pinMode(P7, INPUT_PULLUP);
    I2CBUT.pinMode(P6, INPUT_PULLUP);
    I2CBUT.pinMode(P5, INPUT_PULLUP);
    I2CBUT.pinMode(P4, INPUT_PULLUP);
    I2CBUT.pinMode(P3, INPUT_PULLUP);
}
//=======================================================================
static uint8_t DS_dim(uint8_t i)
{
    return (i < 7) ? ((i == 1) ? 28 : ((i & 1) ? 30 : 31)) : ((i & 1) ? 31 : 30);
}
//=======================================================================

//=======================       S E T U P       =========================
void setup()
{
    CFG.fw = "0.2.8";
    CFG.fwdate = "9.07.2024";

    Serial.begin(UARTSpeed);
    // Serial1.begin(115200,SERIAL_8N1,RX1_PIN, TX1_PIN);
    Serial2.begin(115200, SERIAL_8N1, RX1_PIN, TX1_PIN);
    SystemInit();


    // SPIFFS INIT
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // RTC INIT
    RTC.begin();
    if (RTC.lostPower())
    {
        RTC.setTime(COMPILE_TIME);
    }
    Clock = RTC.getTime();
    Serial.println(F("RTC...Done"));

    I2CExpanderInit(); // Init i2C expander

    ds18b20_1.begin();
    Serial.println(F("Sensor T1...Done"));
    delay(20);
    ds18b20_2.begin();
    Serial.println(F("Sensor T2...Done"));
    delay(20);

    pinMode(WC1, INPUT_PULLUP);
    pinMode(WC2, INPUT_PULLUP);

    Serial.println("Starting Analog buttons");

    ColorSet(&col_speed, WHITE);

    LoadConfig();         // Load configuration from config.json files
    ShowLoadJSONConfig(); // Show load configuration
    if (STATE.WiFiEnable)
    {
        WIFIinit();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        HTTPinit(); // HTTP server initialisation
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    Amplifier.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    Amplifier.setVolume(HCONF.volume);
    Serial.println(F("DAC PCM Amplifier...Done"));

    xTaskCreatePinnedToCore(
        HandlerCore0,
        "TaskCore0_Gen",
        10000,
        NULL,
        1,
        &TaskCore0_Gen,
        0);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    xTaskCreatePinnedToCore(
        HandlerTask1Wire,
        "TaskCore0_TSH",
        2048,
        NULL,
        1,
        &TaskCore0_TSH,
        0);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    xTaskCreatePinnedToCore(
        HandlerCore1,
        "TaskCore1_Gen",
        2048, // ?
        NULL,
        1,
        &TaskCore1_Gen,
        1);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    xTaskCreatePinnedToCore(
        HandlerTask500,
        "TaskCore1_500ms",
        12000,
        NULL,
        1,
        &TaskCore1_500ms,
        1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
}
//=======================================================================

//=======================        L O O P        =========================
void loop()
{
    if (STATE.WiFiEnable)
    {
        HandleClient();
    }
    // Button Handler 
}
//=======================================================================

//======================    R T O S  T A S K    =========================
// Core 0. Network Stack Handler
void HandlerCore0(void *pvParameters)
{
    Serial.print("Task:0 T:10ms Stack:10000 Core:");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        // UART_Recieve_Data();
        Amplifier.loop();
        // Exit menu (20 sec timer)
        if (STATE.menu_tmr == 20)
        {
            menu = IDLE;
            STATE.menu_tmr = 0;
            // Saving if NEW_BRIGHT != OLD
            if (old_bright != HCONF.bright)
            {
                SaveConfig();
                Serial.printf("Saving Brightness:\r\n");
                memset(name_1, 0, 25);
                memset(name_2, 0, 25);
                // strcat(name_1, "Подождите");
                strcat(name_1, "Ожидайте");
                strcat(name_2, "...");
                Send_BS_UserData(name_1, name_2);

                vTaskDelay(1000 / portTICK_PERIOD_MS);
                STATE.StaticUPD = true;
                vTaskDelay(4000 / portTICK_PERIOD_MS);
            }
            STATE.DUPDBlock = false;
        }

        if (STATE.TTS)
        {
            Tell_me_CurrentTime();
            Tell_me_CurrentData();
            STATE.TTS = false;
        }

        if (STATE.DSTS1)
        {
            Tell_me_DoorState(STATE.StateWC1);
            STATE.DSTS1 = false;
        }

        if (STATE.DSTS2)
        {
            Tell_me_DoorState(STATE.StateWC2);
            STATE.DSTS2 = false;
        }

        if (STATE.VolumeUPD)
        {
            Amplifier.setVolume(HCONF.volume);
            STATE.VolumeUPD = false;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Core 0. Temperature Sensor Handler
void HandlerTask1Wire(void *pvParameters)
{
    Serial.print("Task: TSH T:10s Stack:2048 Core:");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        GetDSData();
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

// Core 1. 1000ms
void HandlerCore1(void *pvParameters)
{
    Serial.print("Task:1 T:1000ms Stack:2048 Core:");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        Clock = RTC.getTime();

        // ButtonHandler();

        DebugInfo();

        if (menu != IDLE)
            STATE.menu_tmr++;

        Serial.printf("Menu | Level: %d | Timer: %d \r\n", menu, STATE.menu_tmr);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Core 1. 500ms
void HandlerTask500(void *pvParameters)
{
    Serial.print("Task:3 T:500ms Stack:12000 Core:");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        sec_cnt++;

        STATE.SensWC1 = GetWCState(WC1);
        STATE.SensWC2 = GetWCState(WC2);
        SetColorWC();
        SendtoRS485();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
//=======================================================================

//=========================================================================
// Get WC Door State
bool GetWCState(uint8_t num)
{
    boolean state = false;
    switch (num)
    {
    case WC1:
        state = digitalRead(WC1);
        break;
    case WC2:
        state = digitalRead(WC2);
        break;

    default:
        break;
    }
    HCONF.WCSS == SENSOR_OPEN ? state : state = !state;
    return state;
}
//=========================================================================
void SetColorWC()
{
    switch (HCONF.WCL)
    {
    case NORMAL:
        if (STATE.SensWC1)
        {
            STATE.StateWC1 = true;
            ColorSet(&col_wc1, RED);
        }
        else
        {
            STATE.StateWC1 = false;
            ColorSet(&col_wc1, GREEN);
        }

        if (STATE.SensWC2)
        {
            STATE.StateWC2 = true;
            ColorSet(&col_wc2, RED);
        }
        else
        {
            STATE.StateWC2 = false;
            ColorSet(&col_wc2, GREEN);
        }
        break;

    case REVERSE:
        if (STATE.SensWC1)
        {
            STATE.StateWC2 = true;
            ColorSet(&col_wc2, RED);
        }
        else
        {
            STATE.StateWC2 = false;
            ColorSet(&col_wc2, GREEN);
        }

        if (STATE.SensWC2)
        {
            STATE.StateWC1 = true;
            ColorSet(&col_wc1, RED);
        }
        else
        {
            STATE.StateWC1 = false;
            ColorSet(&col_wc1, GREEN);
        }
        break;

    case ONE_HALL:
        if (HCONF.WCSS)
        {
            if (STATE.SensWC1 && STATE.SensWC2)
            {
                STATE.StateWC1 = true;
                STATE.StateWC2 = true;
                ColorSet(&col_wc1, RED);
                ColorSet(&col_wc2, RED);
            }
            else
            {
                STATE.StateWC1 = false;
                STATE.StateWC2 = false;
                ColorSet(&col_wc1, GREEN);
                ColorSet(&col_wc2, GREEN);
            }
        }
        else
        {
            if (!STATE.SensWC1 && !STATE.SensWC2)
            {
                STATE.StateWC1 = false;
                STATE.StateWC2 = false;
                ColorSet(&col_wc1, GREEN);
                ColorSet(&col_wc2, GREEN);
            }
            else
            {
                STATE.StateWC1 = true;
                STATE.StateWC2 = true;
                ColorSet(&col_wc1, RED);
                ColorSet(&col_wc2, RED);
            }
        }
        break;

    default:
        break;
    }
}
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
            Send_ITdata(1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            Send_ITdata(2);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (!STATE.DUPDBlock)
        {
            Send_BSdata();
            vTaskDelay(100 / portTICK_PERIOD_MS);
            Send_GPSdata();
        }
        sec_cnt = 0;
    }
}
//=========================================================================

//=========================================================================
void ButtonHandler()
{
    int p7 =! I2CBUT.digitalRead(P7); // read P1
    int p6 =! I2CBUT.digitalRead(P6); // read P1
    int p5 =! I2CBUT.digitalRead(P5); // read P1
    int p4 =! I2CBUT.digitalRead(P4); // read P1
    int p3 =! I2CBUT.digitalRead(P3); // read P1

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
    // String buf = "/sound/S/curtime.mp3";
    String buf = "/sound/S/curtime.wav";
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
    // vTaskDelay(500 / portTICK_PERIOD_MS);

    Clock = RTC.getTime();
    buf = "/sound/H/";

    if ((Clock.hour) != 24)
    {
        buf += "ch";
        buf += Clock.hour;
    }
    else
    {
        buf += "ch0";
    }
    // buf += ".mp3";
    buf += ".wav";
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
    // vTaskDelay(500 / portTICK_PERIOD_MS);

    Clock = RTC.getTime();
    buf = "/sound/Mi/";
    buf += "min";
    buf += Clock.minute;
    // buf += ".mp3";
    buf += ".wav";
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
    if (state)
    {
        // buf = "/sound/S/dclose.mp3";
        buf = "/sound/S/dclose.wav";
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
    else
    {
        // buf = "/sound/S/dopen.mp3";
        buf = "/sound/S/dopen.wav";
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
}

void Tell_me_CurrentData()
{
    String buf;
    // buf = "/sound/S/today.mp3";
    buf = "/sound/S/today.wav";

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
    // vTaskDelay(200 / portTICK_PERIOD_MS);

    buf = "/sound/DW/";
    switch (Clock.day)
    {
    case MON: // Monday
        // buf += "mon.mp3";
        buf += "mon.wav";
        break;
    case TUE: // Tuesday
        // buf += "thu.mp3";
        buf += "thu.wav";
        break;
    case WED: // Wednesday
        // buf += "wed.mp3";
        buf += "wed.wav";
        break;
    case THU: // Thursday
        // buf += "thu.mp3";
        buf += "thu.wav";
        break;
    case FRI: // Friday
        // buf += "fri.mp3";
        buf += "fri.wav";
        break;
    case SAT: // Saturday
        // buf += "sat.mp3";
        buf += "sat.wav";
        break;
    case SUN: // Sunday
        // buf += "sun.mp3";
        buf += "sun.wav";
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
    // vTaskDelay(200 / portTICK_PERIOD_MS);

    buf = "/sound/NM/";
    buf += "d";
    buf += Clock.date;
    // buf += ".mp3";
    buf += ".wav";

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
        // buf += "yan.mp3";
        buf += "yan.wav";
        break;
    case FEB:
        // buf += "feb.mp3";
        buf += "feb.wav";
        break;
    case MAR:
        // buf += "mar.mp3";
        buf += "mar.wav";
        break;
    case APR:
        // buf += "apr.mp3";
        buf += "apr.wav";
        break;
    case MAY:
        // buf += "may.mp3";
        buf += "may.wav";
        break;
    case JUN:
        // buf += "jun.mp3";
        buf += "jun.wav";
        break;
    case JUL:
        // buf += "jul.mp3";
        buf += "jul.wav";
        break;
    case AUG:
        // buf += "aug.mp3";
        buf += "aug.wav";
        break;
    case SEP:
        // buf += "sep.mp3";
        buf += "sep.wav";
        break;
    case OCTB:
        // buf += "oct.mp3";
        buf += "oct.wav";
        break;
    case NOV:
        // buf += "nov.mp3";
        buf += "nov.wav";
        break;
    case DECM:
        // buf += "dec.mp3";
        buf += "dec.wav";
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
