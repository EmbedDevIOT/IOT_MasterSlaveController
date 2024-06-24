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
uint8_t old_min = 0;
//======================================================================

//================================ OBJECTs =============================
TaskHandle_t TaskCore_0;
TaskHandle_t TaskCore_1;
TaskHandle_t TaskCore1_500ms;
TaskHandle_t TaskCore0_100ms;

Audio Amplifier;
MicroDS3231 RTC;

OneWire oneWire1(T1);
OneWire oneWire2(T2);
DallasTemperature ds18b20_1(&oneWire1);
DallasTemperature ds18b20_2(&oneWire2);

HardwareSerial RS485(2);

AnalogButtons analogButtons(KBD_PIN, INPUT, 5, 50);
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
void HandlerTaskKeyboard(void *pvParameters);
void ButtonHandler();
void SendtoRS485();
void GetDSData(void);
bool GetWCState(uint8_t num);
void SetColorWC();
void UART_Recieve_Data();
void Tell_me_CurrentTime();
void Tell_me_CurrentData();
void Tell_me_DoorState(bool state);

// - buttons function --
void btn1Click();
void btn2Click();
void btn3Click();
void btn3Hold();
void btn4Click();
void btn5Click();
void configure();

Button b1 = Button(bt1, &btn1Click);
Button b2 = Button(bt2, &btn2Click);
// Button b3 = Button(bt3, &btn3Click, &btn3Hold);
Button b3 = Button(bt3, &btn3Click);
Button b4 = Button(bt4, &btn4Click);
// get it activated (hold function invoked) only every 500ms
// Button b4 = Button(929, &b4Click, &b4Hold, 1000, 500);
Button b5 = Button(bt5, &btn5Click);

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
    CFG.fw = "0.1.6";
    CFG.fwdate = "24.06.2024";

    Serial.begin(UARTSpeed);
    // Serial1.begin(115200,SERIAL_8N1,RX1_PIN, TX1_PIN);
    Serial2.begin(115200, SERIAL_8N1, RX1_PIN, TX1_PIN);
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

    Serial.println("Starting Analog buttons");
    analogButtons.add(b1);
    analogButtons.add(b2);
    analogButtons.add(b3);
    analogButtons.add(b4);
    analogButtons.add(b5);

    // pinMode(KBD_PIN, INPUT);

    ColorSet(&col_speed, WHITE);

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

    xTaskCreatePinnedToCore(
        HandlerTaskKeyboard,
        "TaskCore0_100ms",
        2048,
        NULL,
        1,
        &TaskCore0_100ms,
        0);
    vTaskDelay(500 / portTICK_PERIOD_MS);

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
    HandleClient();
    analogButtons.check();
    // configure();
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
        Amplifier.loop();
        // UART_Recieve_Data();
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

// Core 0. Keyboard Handler
void HandlerTaskKeyboard(void *pvParameters)
{
    Serial.print("Task: KBD T:100ms Stack:2048 Core:");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        // ButtonHandler();
        vTaskDelay(10 / portTICK_PERIOD_MS);
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

        GetDSData();
        DebugInfo();
        Serial.printf("Menu Level: %d \r\n", menu);
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
        // if (STATE.DUPDBlock && menu == L2_MIN)
        // {
        //     if (Clock.second == 0)
        //     {
        //         Serial.print("Minute Update \r\n");
        //         memset(name_1, 0, 25);
        //         memset(name_2, 0, 25);
        //         strcat(name_1, "Минута:");
        //         sprintf(name_2, "%02d", Clock.minute);
        //         Send_BS_UserData(name_1, name_2);
        //     }
        // }
        vTaskDelay(500 / portTICK_PERIOD_MS);
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

//=================== Keyboard buttons Handler =============================
// Button 1 Handling (+)
void btn1Click()
{
    Serial.print("button 1 clicked\r\n");
    if (menu == IDLE)
    {
        int hour = Clock.hour;
        hour++;
        if (hour > 23)
            hour = 0;
        Clock.hour = hour;
        RTC.setTime(Clock);
        Serial.printf("Hour: %d \n\r", Clock.hour);
        Send_GPSdata();
    }
}

// Button 2 Handling (-)
void btn2Click()
{
    Serial.print("button 2 clicked\r\n");
    int min, hour, month;
    switch (menu)
    {
    case IDLE:
        // CarNUM (--) to IDLE state (no change TOP zone)
        if (!UserText.hide_t)
        {
            UserText.carnum > 0 ? UserText.carnum-- : UserText.carnum = 99;
            Serial.printf("CARNUM: %d \r\n", UserText.carnum);
            SaveConfig();
            Send_BSdata();
        }
        break;
    // CarNUM (--) to General MENU  (change TOP zone)
    case _CAR_NUM:
        UserText.carnum > 0 ? UserText.carnum-- : UserText.carnum = 99;
        Serial.printf("CARNUM: %d \r\n", UserText.carnum);
        memset(name_2, 0, 15);
        sprintf(name_2, "%d", UserText.carnum);
        Send_BS_UserData(name_1, name_2);
        SaveConfig();
        break;
    // GMT --
    case _GMT:
        break;
    // MIN --
    case _MIN:
        min = Clock.minute;
        min--;
        if (min < 0)
            min = 59;
        Clock.minute = min;
        Serial.printf("MIN: %d \r\n", min);
        RTC.setTime(Clock);
        memset(name_2, 0, 15);
        sprintf(name_2, "%d", min);
        Send_BS_UserData(name_1, name_2);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Send_GPSdata();
        break;
    // Hour --
    case _HOUR:
        hour = Clock.hour;
        hour--;
        if (hour < 0)
            hour = 23;
        Clock.hour = hour;
        RTC.setTime(Clock);
        Serial.printf("Hour: %d \n\r", Clock.hour);
        memset(name_2, 0, 15);
        sprintf(name_2, "%d", hour);
        Send_BS_UserData(name_1, name_2);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Send_GPSdata();
        break;
    // Day --
    case _DAY:
        break;
    // MONTH --
    case _MONTH:
        month = Clock.month;
        month--;
        if (month < 1)
        {
            month = 12;
        }
        Clock.month = month;
        RTC.setTime(Clock);
        Serial.printf("DATA: %d \t MONTH: %d \r\n ", Clock.date, Clock.month);
        memset(name_2, 0, 15);
        sprintf(name_2, "%d", month);
        Send_BS_UserData(name_1, name_2);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Send_GPSdata();
        break;
    // Year --
    case _YEAR:
        break;
    // Brightness --
    case _BRIGHT:
        break;
    // WC Signal State Logiq
    case _WCL:
        break;
    // WC Signal sensor state Preset
    case _WCSS:
        break;
    // WiFI ON / OFF
    case _WiFi:
        /* code */
        break;
    default:
        break;
    }
}

// Button 3 Handling (Check and Select level menu)
void btn3Click()
{
    Serial.print("button 3 clicked\r\n");
    menu += 1;

    if (menu <= 11)
    {
        STATE.DUPDBlock = true;
    }
    else
    {
        menu = IDLE;
        STATE.DUPDBlock = false;
    }

    switch (menu)
    {
    // CarNUM (--) to General MENU  (change TOP zone)
    case _CAR_NUM:
        Serial.printf("CARNUM:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "Вагон:");
        sprintf(name_2, "%d", UserText.carnum);
        Send_BS_UserData(name_1, name_2);
        break;
        // Min --
    case _GMT:
        Serial.printf("GMT:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "Час.пояс:");
        if (CFG.gmt < 0)
        {
            sprintf(name_2, "-%2d", CFG.gmt);
        }
        else if (CFG.gmt == 0)
        {
            sprintf(name_2, "%d", CFG.gmt);
        }
        else
        {
            sprintf(name_2, "+%d", CFG.gmt);
        }
        Send_BS_UserData(name_1, name_2);
        break;
    case _MIN:
        Serial.printf("Minute:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "Минута:");
        sprintf(name_2, "%02d", Clock.minute);
        Send_BS_UserData(name_1, name_2);
        break;
    // Hour --
    case _HOUR:
        Serial.printf("Hour:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "Час:");
        sprintf(name_2, "%02d", Clock.hour);
        Send_BS_UserData(name_1, name_2);
        break;
    // Day --
    case _DAY:
        Serial.printf("Day:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "День:");
        sprintf(name_2, "%02d", Clock.date);
        Send_BS_UserData(name_1, name_2);
        break;
    // MONTH --
    case _MONTH:
        Serial.printf("Month:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "Месяц:");
        sprintf(name_2, "%02d", Clock.month);
        Send_BS_UserData(name_1, name_2);
        break;
    // Year --
    case _YEAR:
        Serial.printf("Year:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "Год:");
        sprintf(name_2, "%02d", Clock.year);
        Send_BS_UserData(name_1, name_2);
        break;
    // Brightness --
    case _BRIGHT:
        Serial.printf("Brightness:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "Яркость:");
        sprintf(name_2, "%02d", HCONF.bright);
        Send_BS_UserData(name_1, name_2);
        break;
    // WC Signal State Logiq
    case _WCL:
        Serial.printf("WC Signal Logiq:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "ЛогикаWC");
        if (HCONF.WCL == NORMAL)
        {
            sprintf(name_2, "Нормальн");
        }
        else if (HCONF.WCL == REVERSE)
        {
            sprintf(name_2, "Реверс");
        }
        else if (HCONF.WCL == ONE_HALL)
        {
            sprintf(name_2, "Один Тамбур");
        }
        Send_BS_UserData(name_1, name_2);
        break;
    // WC Signal sensor state Preset
    case _WCSS:
        Serial.printf("WC Signal sensor preset:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "СигналWC");
        if (HCONF.WCSS == SENSOR_OPEN)
        {
            sprintf(name_2, "Разомкнут");
        }
        else
        {
            sprintf(name_2, "Замкнут");
        }

        Send_BS_UserData(name_1, name_2);
        break;
    // WiFI ON / OFF
    case _WiFi:
        Serial.printf("WiFI Conrol:\r\n");
        memset(name_1, 0, 25);
        memset(name_2, 0, 25);
        strcat(name_1, "WiFi:");
        sprintf(name_2, "ВКЛ");
        Send_BS_UserData(name_1, name_2);
        break;
    default:
        break;
    }
}

// Button 3 Hold Handling (Enter the menu)
void btn3Hold()
{
    Serial.print("button 3 hold");

    if (menu == IDLE)
    {
        menu = _CAR_NUM;
        STATE.DUPDBlock = true;
        strcat(name_1, "Вагон");
        sprintf(name_2, "%d", UserText.carnum);
        Send_BS_UserData(name_1, name_2);
    }
}

// Button 4 Handling (+)
void btn4Click()
{
    Serial.print("button 4 clicked\r\n");
    int min, hour, month;
    switch (menu)
    {
    case IDLE:
        // CarNUM (++) to IDLE state (no change TOP zone)
        if (!UserText.hide_t)
        {
            UserText.carnum < 99 ? UserText.carnum++ : UserText.carnum = 0;
            Serial.printf("CARNUM: %d \r\n", UserText.carnum);
            SaveConfig();
            Send_BSdata();
        }
        break;
    // CarNUM (++) to General MENU  (change TOP zone)
    case _CAR_NUM:
        UserText.carnum < 99 ? UserText.carnum++ : UserText.carnum = 0;
        Serial.printf("CARNUM: %d \r\n", UserText.carnum);
        memset(name_2, 0, 25);
        sprintf(name_2, "%d", UserText.carnum);
        Send_BS_UserData(name_1, name_2);
        SaveConfig();
        break;
    // GMT ++
    case _GMT:
        break;
    // MIN ++
    case _MIN:
        min = Clock.minute;
        min++;
        if (min > 59)
            min = 0;
        Clock.minute = min;
        Serial.printf("MIN: %d \r\n", min);
        RTC.setTime(Clock);
        memset(name_2, 0, 15);
        sprintf(name_2, "%d", min);
        Send_BS_UserData(name_1, name_2);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Send_GPSdata();
        break;
    // Hour ++
    case _HOUR:
        hour = Clock.hour;
        hour++;
        if (hour > 23)
            hour = 0;
        Clock.hour = hour;
        RTC.setTime(Clock);
        memset(name_2, 0, 15);
        sprintf(name_2, "%d", hour);
        Send_BS_UserData(name_1, name_2);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Serial.printf("Hour: %d \n\r", Clock.hour);
        Send_GPSdata();
        break;
    // Day ++
    case _DAY:
        break;
    // MONTH ++
    case _MONTH:
        month = Clock.month;
        month++;
        if (month > 12)
        {
            month = 1;
        }
        Clock.month = month;
        RTC.setTime(Clock);
        Serial.printf("DATA: %d \t MONTH: %d \r\n ", Clock.date, Clock.month);
        memset(name_2, 0, 15);
        sprintf(name_2, "%d", month);
        Send_BS_UserData(name_1, name_2);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        Send_GPSdata();
        break;
    // Year ++
    case _YEAR:
        break;
    // Brightness ++
    case _BRIGHT:
        break;
    // WC Signal State Logiq
    case _WCL:
        break;
    // WC Signal sensor state Preset
    case _WCSS:
        break;
    // WiFI ON / OFF
    case _WiFi:
        /* code */
        break;
    default:
        break;
    }
}

// Button 5 Handling (-)
void btn5Click()
{
    Serial.print("button 5 clicked\r\n");
    // Hour --
    if (menu == IDLE)
    {
        int hour = Clock.hour;
        hour--;
        if (hour < 0)
            hour = 23;
        Clock.hour = hour;
        RTC.setTime(Clock);
        Serial.printf("Hour: %d \n\r", Clock.hour);
        Send_GPSdata();
    }
}

void configure()
{
    unsigned int value = analogRead(KBD_PIN);
    Serial.println(value);
    vTaskDelay(250 / portTICK_PERIOD_MS);
}
//=========================================================================
void ButtonHandler()
{

    // Serial.println("#### FACTORY RESET ####");
    // SystemFactoryReset();
    // sprintf(umsg, "Сброшено");
    // SendXMLUserData(umsg);
    // // SaveConfig();
    // vTaskDelay(2000 / portTICK_PERIOD_MS);
    // Serial.println("#### SAVE DONE ####");
    // ESP.restart();
    // Serial.printf("ADC %0004d \r\n", analogRead(KBD_PIN));
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
    // vTaskDelay(500 / portTICK_PERIOD_MS);

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
    // vTaskDelay(500 / portTICK_PERIOD_MS);

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
    // vTaskDelay(200 / portTICK_PERIOD_MS);

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
    // vTaskDelay(200 / portTICK_PERIOD_MS);

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
