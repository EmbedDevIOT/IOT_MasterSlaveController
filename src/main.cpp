#include "Config.h"

#include "WF.h"
#include "FileConfig.h"
#include "HTTP.h"
#include "keyboard.h"

#define DEBUG // Debug control ON
//======================================================================

//=========================== GLOBAL VARIABLES =========================
bool WiFiVD = true;

uint8_t sec = 0;

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

Button btn1(7, INPUT, LOW);
Button btn2(6, INPUT, LOW);
Button btn3(5, INPUT, LOW);
Button btn4(4, INPUT, LOW);
Button btn5(3, INPUT, LOW);

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
    // I2CBUT.begin();
    // I2CBUT.pinMode(P7, INPUT_PULLUP);
    // I2CBUT.pinMode(P6, INPUT_PULLUP);
    // I2CBUT.pinMode(P5, INPUT_PULLUP);
    // I2CBUT.pinMode(P4, INPUT_PULLUP);
    // I2CBUT.pinMode(P3, INPUT_PULLUP);
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
    CFG.fw = "0.3.5";
    CFG.fwdate = "20.07.2024";

    Serial.begin(UARTSpeed);
    Serial2.begin(115200, SERIAL_8N1, RX1_PIN, TX1_PIN);
    SystemInit();

    // GPIO init
    pinMode(LED_ST, OUTPUT);
    digitalWrite(LED_ST, LOW);
    pinMode(LED_WiFi, OUTPUT);
    digitalWrite(LED_WiFi, LOW);

    pinMode(WC1, INPUT_PULLUP);
    pinMode(WC2, INPUT_PULLUP);

    pinMode(SW1, INPUT_PULLUP);
    pinMode(SW2, INPUT_PULLUP);

    // RS 485 ADR SET
    if (digitalRead(SW1) && digitalRead(SW2))
    {
        HCONF.ADR = 0;
    }
    else if (!digitalRead(SW1) && digitalRead(SW2))
    {
        HCONF.ADR = 1;
    }
    else if (digitalRead(SW1) && !digitalRead(SW2))
    {
        HCONF.ADR = 2;
    }
    else if (!digitalRead(SW1) && !digitalRead(SW2))
    {
        HCONF.ADR = 3;
    }
    Serial.printf("RS485 ADR: %d \r\n", HCONF.ADR);

    vTaskDelay(500 / portTICK_PERIOD_MS);
    for (uint8_t i = 1; i <= HCONF.ADR; i++)
    {
        digitalWrite(LED_ST, HIGH);
        vTaskDelay(150 / portTICK_PERIOD_MS);
        digitalWrite(LED_ST, LOW);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }

    // SPIFFS INIT
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    // ADR = 1
    if (HCONF.ADR == 1)
    {
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
    }

    // I2C_Scanning();
    // vTaskDelay(2000 / portTICK_PERIOD_MS);

    ColorSet(&col_speed, WHITE);

    LoadConfig();         // Load configuration from config.json files
    ShowLoadJSONConfig(); // Show load configuration

    if ((HCONF.ADR == 1) && (STATE.WiFiEnable))
    {
        Serial.println("Wifi Enable");
        WIFIinit();
        vTaskDelay(500 / portTICK_PERIOD_MS);

        HTTPinit(); // HTTP server initialisation
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    Amplifier.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    Amplifier.setVolume(HCONF.volume);
    Serial.println(F("DAC PCM Amplifier...Done"));

    // Task: I2S Audio Handler | MenuTimer Exit
    xTaskCreatePinnedToCore(
        HandlerCore0,
        "TaskCore0_Gen",
        10000,
        NULL,
        1,
        &TaskCore0_Gen,
        0);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // Temperature Sensor Task Handler
    if (HCONF.ADR == 1)
    {
        xTaskCreatePinnedToCore(
            HandlerTask1Wire,
            "TaskCore0_TSH",
            2048,
            NULL,
            1,
            &TaskCore0_TSH,
            0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    // Core 1. 1000ms (Read RTC / Debug Info / menu timer)
    if (HCONF.ADR == 1)
    {
        xTaskCreatePinnedToCore(
            HandlerCore1,
            "TaskCore1_Gen",
            2048,
            NULL,
            1,
            &TaskCore1_Gen,
            1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    // Sending RS485 data
    xTaskCreatePinnedToCore(
        HandlerTask500,
        "TaskCore1_500ms",
        16384, // 12000
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
    // HTTP Handling (RSADR = 1 and WiFi = ON)
    if ((HCONF.ADR == 1) && (STATE.WiFiEnable))
    {
        HandleClient();
    }
    ButtonHandler();
}
//=======================================================================

//======================    R T O S  T A S K    =========================
// Core 0. 10ms (I2S Audio Handler | MenuTimer_Exit
void HandlerCore0(void *pvParameters)
{
    Serial.print("Task: I2S Handler | MenuTimer Exit \r\n");
    Serial.print("T:10ms Stack:10000 Core:");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        // UART_Recieve_Data();
        Amplifier.loop();
        // Exit menu (20 sec timer)
        if ((HCONF.ADR == 1) && (STATE.menu_tmr == 20))
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
    Serial.print("Task: Temperature Sensor Handler \r\n");
    Serial.print("T:10s Stack:2048 Core:");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        // Serial.printf("sec: %d \r\n", sec);

        if (HCONF.ADR == 1)
        {
            if (sec < 10)
            {
                sec++;

                if ((STATE.WiFiEnable) && (sec % 2 == 0))
                {
                    // Serial.printf("LED ON sec: %d \r\n", sec);

                    digitalWrite(LED_WiFi, HIGH);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    digitalWrite(LED_WiFi, LOW);
                }
            }
            else
            {
                GetDSData();
                sec = 1;
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

// Core 1. 1000ms (Read RTC / Debug Info / menu timer)
void HandlerCore1(void *pvParameters)
{
    Serial.print("Task: Read RTC / Debug Info / menu timer \r\n");
    Serial.print("T:1000ms Stack:2048 Core:");
    Serial.println(xPortGetCoreID());
    for (;;)
    {
        if (HCONF.ADR == 1)
        {
            Clock = RTC.getTime();
            // DebugInfo();
            if (menu != IDLE)
                STATE.menu_tmr++;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Core 1. 1000ms
void HandlerTask500(void *pvParameters)
{
    Serial2.print("\r\n\r\n"); // Костыль инита RS485

    Serial.print("Task: Sending RS485 data \r\n");
    Serial.print("T:1000ms Stack:12000 Core:");
    Serial.println(xPortGetCoreID());

    for (;;)
    {

        if (HCONF.ADR == 1)
        {
            sec_cnt++;
            STATE.SensWC1 = GetWCState(WC1);
            STATE.SensWC2 = GetWCState(WC2);
            SetColorWC();

            SendtoRS485();
        }
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
            Serial.println("Send IT");

            digitalWrite(LED_ST, HIGH);
            Send_ITdata(1);
            digitalWrite(LED_ST, LOW);
        }

        if (!STATE.DUPDBlock)
        {
            digitalWrite(LED_ST, HIGH);
            Send_BSdata();
            vTaskDelay(100 / portTICK_PERIOD_MS);
            digitalWrite(LED_ST, LOW);
            Send_GPSdata();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        sec_cnt = 0;
    }
}
//=========================================================================
//=========================================================================
//=================== Keyboard buttons Handler =============================
void ButtonHandler()
{
    switch (HCONF.ADR)
    {
    case 1:
        btn1.tick();
        btn2.tick();
        btn3.tick();
        btn4.tick();
        btn5.tick();
        // Click Button 1 Handling (+)
        if (btn1.click())
        {
            Serial.printf(" BTN 1 Click \r\n");

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

        // Click Button 2 Handling (-)
        if (btn2.click())
        {
            Serial.printf(" BTN 2 Click \r\n");
            int min, hour, data, month, year;

            STATE.menu_tmr = 0; // timer menu reset
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
                if (CFG.gmt > -12 && CFG.gmt <= 12)
                {
                    CFG.gmt--;
                    memset(name_2, 0, 15);
                    if (CFG.gmt > 0)
                    {
                        sprintf(name_2, "+%d", CFG.gmt);
                    }
                    else
                        sprintf(name_2, "%d", CFG.gmt);
                    Send_BS_UserData(name_1, name_2);
                }
                SaveConfig();
                Serial.printf("GMT: %d \r\n", CFG.gmt);
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
                data = Clock.date;
                month = Clock.month;
                data--;
                data = constrain(data, 0, DS_dim(month - 1));

                if (data < 1)
                    data = DS_dim(month - 1);

                Clock.date = data;
                RTC.setTime(Clock);
                Serial.printf("DATA: %d \t MONTH: %d \r\n ", Clock.date, Clock.month);
                memset(name_2, 0, 15);
                sprintf(name_2, "%d", Clock.date);
                Send_BS_UserData(name_1, name_2);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                Send_GPSdata();
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
                year = Clock.year;
                if (year > 2000 && year <= 2099)
                    year--;
                Clock.year = year;
                RTC.setTime(Clock);
                Serial.printf("Year: %d\r\n ", Clock.year);
                memset(name_2, 0, 15);
                sprintf(name_2, "%d", Clock.year);
                Send_BS_UserData(name_1, name_2);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                Send_GPSdata();
                break;
            // Brightness --
            case _BRIGHT:

                if (HCONF.bright > 10 && HCONF.bright <= 100)
                    HCONF.bright -= 10;

                Serial.printf("Bright: %d\r\n ", HCONF.bright);
                memset(name_2, 0, 15);
                sprintf(name_2, "%d", HCONF.bright);
                Send_BS_UserData(name_1, name_2);
                break;
            // WC Signal State Logiq
            case _WCL:
                (HCONF.WCL > 0 && HCONF.WCL <= 2) ? HCONF.WCL -= 1 : HCONF.WCL = 2;
                Serial.printf("WCL: %d\r\n ", HCONF.WCL);

                memset(name_2, 0, 15);

                switch (HCONF.WCL)
                {
                case NORMAL:
                    sprintf(name_2, "Нормальн");
                    break;
                case REVERSE:
                    sprintf(name_2, "Реверс");
                    break;
                case ONE_HALL:
                    sprintf(name_2, "1 Тамбур");
                    break;
                default:
                    break;
                }
                SaveConfig();
                Send_BS_UserData(name_1, name_2);
                break;

            // WC Signal sensor state Preset
            case _WCSS:
                memset(name_2, 0, 25);
                if (HCONF.WCSS == SENSOR_OPEN)
                {
                    HCONF.WCSS = SENSOR_CLOSE;
                    sprintf(name_2, "Замкнут");
                }
                else
                {
                    HCONF.WCSS = SENSOR_OPEN;
                    sprintf(name_2, "Разомкнут");
                }
                SaveConfig();
                Send_BS_UserData(name_1, name_2);
                break;
            // WiFI ON / OFF
            case _WiFi:
                if (STATE.WiFiEnable)
                {
                    STATE.WiFiEnable = false;
                    Serial.println("WiFi_Disable");
                    memset(name_2, 0, 15);
                    sprintf(name_2, "ОТКЛ");
                    Send_BS_UserData(name_1, name_2);
                    WiFi.disconnect(true);
                    WiFi.mode(WIFI_OFF);
                }
                else
                {
                    STATE.WiFiEnable = true;
                    Serial.println("WiFi_Enable");
                    memset(name_2, 0, 15);
                    sprintf(name_2, "ВКЛ");
                    Send_BS_UserData(name_1, name_2);
                    WIFIinit(AccessPoint);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    HTTPinit(); // HTTP server initialisation
                }
                SaveConfig();
                break;
            default:
                break;
            }
        }

        // Click Button 3 Handling (Check and Select level menu)
        if (btn3.click())
        {
            Serial.printf(" BTN 3 Click \r\n");
            STATE.menu_tmr = 0; // timer menu reset
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
                if (CFG.gmt > 0)
                {
                    sprintf(name_2, "+%d", CFG.gmt);
                }
                else
                {
                    sprintf(name_2, "%d", CFG.gmt);
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
                sprintf(name_2, "%d", Clock.date);
                Send_BS_UserData(name_1, name_2);
                break;
            // MONTH --
            case _MONTH:
                Serial.printf("Month:\r\n");
                memset(name_1, 0, 25);
                memset(name_2, 0, 25);
                strcat(name_1, "Месяц:");
                sprintf(name_2, "%d", Clock.month);
                Send_BS_UserData(name_1, name_2);
                break;
            // Year --
            case _YEAR:
                Serial.printf("Year:\r\n");
                memset(name_1, 0, 25);
                memset(name_2, 0, 25);
                strcat(name_1, "Год:");
                sprintf(name_2, "%d", Clock.year);
                Send_BS_UserData(name_1, name_2);
                break;
            // Brightness --
            case _BRIGHT:
                old_bright = HCONF.bright;
                Serial.printf("Brightness:\r\n");
                memset(name_1, 0, 25);
                memset(name_2, 0, 25);
                strcat(name_1, "Яркость:");
                sprintf(name_2, "%02d", HCONF.bright);
                Send_BS_UserData(name_1, name_2);
                break;
            // WC Signal State Logiq
            case _WCL:
                // Saving if NEW_BRIGHT != OLD
                if (old_bright != HCONF.bright)
                {
                    SaveConfig();
                    Serial.printf("Saving Brightness:\r\n");
                    memset(name_1, 0, 25);
                    memset(name_2, 0, 25);
                    strcat(name_1, "Ожидайте");
                    strcat(name_2, "...");
                    Send_BS_UserData(name_1, name_2);

                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    STATE.StaticUPD = true;
                    vTaskDelay(4000 / portTICK_PERIOD_MS);
                }

                Serial.printf("WC Signal Logiq:\r\n");
                memset(name_1, 0, 25);
                memset(name_2, 0, 25);
                strcat(name_1, "РасполWC");
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
                    sprintf(name_2, "1 Тамбур");
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
                if (STATE.WiFiEnable)
                {
                    sprintf(name_2, "ВКЛ");
                }
                else
                    sprintf(name_2, "ОТКЛ");
                Send_BS_UserData(name_1, name_2);
                break;
            default:
                break;
            }
        }

        // Holding Button 3 Handling (+)
        if (btn3.getSteps() == 50)
        {
            Serial.printf(" BTN 3 STEP 50 \r\n");
            // if (menu == IDLE)
            // {
            uint32_t now;
            uint8_t cnt = 3;

            memset(name_1, 0, 25);
            memset(name_2, 0, 25);

            STATE.DUPDBlock = true;
            strcat(name_1, "Cброс");
            itoa(cnt, name_2 + strlen(name_2), DEC);
            Send_BS_UserData(name_1, name_2);

            now = millis();
            while (millis() - now < 3500)
            {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                if (cnt != 0)
                {
                    cnt--;
                    memset(name_2, 0, 25);
                    itoa(cnt, name_2 + strlen(name_2), DEC);
                    Send_BS_UserData(name_1, name_2);
                }
            }
            Serial.println("#### FACTORY RESET ####");
            memset(name_1, 0, 25);
            memset(name_2, 0, 25);
            strcat(name_1, "Сброшено");
            Send_BS_UserData(name_1, name_2);
            SystemFactoryReset();
            SaveConfig();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            Serial.println("#### SAVE DONE ####");
            ESP.restart();
            // }
            btn3.clear();
        }

        // Click Button 4 Handling (+)
        if (btn4.click())
        {
            Serial.printf(" BTN 4 Click \r\n");
            int min, hour, data, month, year;

            STATE.menu_tmr = 0; // timer menu reset

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
                if (CFG.gmt >= -12 && CFG.gmt < 12)
                {
                    CFG.gmt++;
                    memset(name_2, 0, 15);
                    if (CFG.gmt > 0)
                    {
                        sprintf(name_2, "+%d", CFG.gmt);
                    }
                    else
                        sprintf(name_2, "%d", CFG.gmt);
                    Send_BS_UserData(name_1, name_2);
                }
                Serial.printf("GMT: %d \r\n", CFG.gmt);
                SaveConfig();
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
                data = Clock.date;
                month = Clock.month;

                data++;

                if (data > constrain(data, 0, DS_dim(month - 1)))
                    data = 1;

                Clock.date = data;
                RTC.setTime(Clock);
                Serial.printf("DATA: %d \t MONTH: %d \r\n ", Clock.date, Clock.month);
                memset(name_2, 0, 15);
                sprintf(name_2, "%d", Clock.date);
                Send_BS_UserData(name_1, name_2);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                Send_GPSdata();
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
                year = Clock.year;
                if (year >= 2000 && year < 2099)
                {
                    year++;
                }
                Clock.year = year;
                RTC.setTime(Clock);
                Serial.printf("Year: %d\r\n", Clock.year);
                memset(name_2, 0, 15);
                sprintf(name_2, "%d", Clock.year);
                Send_BS_UserData(name_1, name_2);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                Send_GPSdata();
                break;

            // Brightness ++
            case _BRIGHT:
                if (HCONF.bright >= 10 && HCONF.bright < 100)
                {
                    HCONF.bright += 10;
                }
                Serial.printf("Bright: %d\r\n ", HCONF.bright);
                memset(name_2, 0, 15);
                sprintf(name_2, "%d", HCONF.bright);
                Send_BS_UserData(name_1, name_2);
                break;

            // WC Signal State Logiq
            case _WCL:
                (HCONF.WCL >= 0 && HCONF.WCL < 2) ? HCONF.WCL += 1 : HCONF.WCL = 0;
                Serial.printf("WCL: %d\r\n ", HCONF.WCL);
                memset(name_2, 0, 15);
                switch (HCONF.WCL)
                {
                case NORMAL:
                    sprintf(name_2, "Нормальн");
                    break;
                case REVERSE:
                    sprintf(name_2, "Реверс");
                    break;
                case ONE_HALL:
                    sprintf(name_2, "1 Тамбур");
                    break;
                default:
                    break;
                }
                SaveConfig();
                Send_BS_UserData(name_1, name_2);
                break;

            // WC Signal sensor state Preset
            case _WCSS:
                memset(name_2, 0, 25);
                if (HCONF.WCSS == SENSOR_OPEN)
                {
                    HCONF.WCSS = SENSOR_CLOSE;
                    sprintf(name_2, "Замкнут");
                }
                else
                {
                    HCONF.WCSS = SENSOR_OPEN;
                    sprintf(name_2, "Разомкнут");
                }
                SaveConfig();
                Send_BS_UserData(name_1, name_2);
                break;

            // WiFI ON / OFF
            case _WiFi:

                if (STATE.WiFiEnable)
                {
                    STATE.WiFiEnable = false;
                    Serial.println("WiFi_Disable");
                    memset(name_2, 0, 15);
                    sprintf(name_2, "ОТКЛ");
                    Send_BS_UserData(name_1, name_2);
                    WiFi.disconnect(true);
                    WiFi.mode(WIFI_OFF);
                }
                else
                {
                    STATE.WiFiEnable = true;
                    Serial.println("WiFi_Enable");
                    memset(name_2, 0, 15);
                    sprintf(name_2, "ВКЛ");
                    Send_BS_UserData(name_1, name_2);
                    WIFIinit(AccessPoint);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    HTTPinit(); // HTTP server initialisation
                }
                SaveConfig();
                break;
            default:
                break;
            }
        }
        // Click Button 5 Handling (-)
        if (btn5.click())
        {
            Serial.printf(" BTN 5 Click \r\n");
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
        break;
    case 2:
        btn2.tick();
        btn3.tick();
        btn4.tick();
        // Click Button 2 Handling (TTS)
        if (btn2.click())
        {
            STATE.TTS = true;
        }
        // Click Button 3 Handling (DTS) WC1
        if (btn3.click())
        {
            STATE.DSTS1 = true;
        }
        // Click Button 4 Handling (DTS) WC2
        if (btn4.click())
        {
            STATE.DSTS2 = true;
        }
        break;
    case 3:
        break;
    default:
        break;
    }
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
