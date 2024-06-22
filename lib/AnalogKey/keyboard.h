#ifndef _keyboard_H
#define _keyboard_H

#include <Arduino.h>

// #define KBD_PIN 36 // Pin Analog Keyboard

enum KEYBOARD_BTN
{
    BT1 = 1,
    BT2,
    BT3,
    BT4,
    BT5,
};

int KButtonRead();
int GetKeyValue();
int GetButtonNumberByValue(int value);


#endif