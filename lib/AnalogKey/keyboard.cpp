#include "keyboard.h"
#include "Config.h"

// const uint16_t btn1 = 4095;
// const uint16_t btn2 = 1920;
// const uint16_t btn3 = 1230;
// const uint16_t btn4 = 870;
// const uint16_t btn5 = 670;
int keyValue = 0;
uint8_t BTN_AMOUNT = 5;

enum BTN_VAL
{
    btn1 = 4095,
    btn2 = 1920,
    btn3 = 1230,
    btn4 = 870,
    btn5 = 670
};

int KButtonRead()
{
    int newKeyValue = GetKeyValue();
    if (keyValue != newKeyValue)
    {                           // Если новое значение не совпадает со старым - реагируем на него
        keyValue = newKeyValue; // Актуализируем переменную хранения состояния
        if (keyValue > 0)
        { // Если значение больше 0, значит кнопка нажата
            // Serial.println("Key pressed: " + String(keyValue));

            return keyValue;
        }
        else if (keyValue < 0)
        { // Если -1 - неизвестное состояние, незапрограммированное нажатие
            // Serial.println("unknown pressed");
            return -1;
        }
        else
        { // Если 0, то состояние покоя
            // Serial.println("all keys are not pressed");
            return 0;
        }
    }
}

int GetKeyValue()
{ // Функция устраняющая дребезг
    static int count;
    static int oldKeyValue; // Переменная для хранения предыдущего значения состояния кнопок
    static int innerKeyValue;

    // Здесь уже не можем использовать значение АЦП, так как оно постоянно меняется в силу погрешности
    int actualKeyValue = GetButtonNumberByValue(analogRead(KBD_PIN)); // Преобразовываем его в номер кнопки, тем самым убирая погрешность

    if (innerKeyValue != actualKeyValue)
    {                                   // Пришло значение отличное от предыдущего
        count = 0;                      // Все обнуляем и начинаем считать заново
        innerKeyValue = actualKeyValue; // Запоминаем новое значение
    }
    else
    {
        count += 1; // Увеличиваем счетчик
    }

    if ((count >= 10) && (actualKeyValue != oldKeyValue))
    {
        oldKeyValue = actualKeyValue; // Запоминаем новое значение
    }
    return oldKeyValue;
}

int GetButtonNumberByValue(int value)
{ // Новая функция по преобразованию кода нажатой кнопки в её номер
    int values[BTN_AMOUNT + 1] = {0, btn5, btn4, btn3, btn2, btn1};
    int error = 15; // Величина отклонения от значений - погрешность
    for (int i = 0; i <= BTN_AMOUNT; i++)
    {
        // Если значение в заданном диапазоне values[i]+/-error - считаем, что кнопка определена
        if (value <= values[i] + error && value >= values[i] - error)
            return i;
    }
    return -1; // Значение не принадлежит заданному диапазону
}