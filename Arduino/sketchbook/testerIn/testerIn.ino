/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/

#include "LowPower.h"
#include "PCF8523.h"
#include "SleepyPi2.h"

const int IN_PIN_1 = 5;
const int IN_PIN_2 = 3;
const int IN_PIN_3 = 6;
const int IN_PIN_4 = 12;
const int IN_PIN_5 = 11;
const int IN_PIN_6 = 8;
int val1 = 0;
int val2 = 0;
int val3 = 0;
int val4 = 0;
int val5 = 0;
int val6 = 0;

// the setup function runs once when you press reset or power the board
void setup()
{
    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(IN_PIN_1, INPUT);
    pinMode(IN_PIN_2, INPUT);
    pinMode(IN_PIN_3, INPUT);
    pinMode(IN_PIN_4, INPUT);
    pinMode(IN_PIN_5, INPUT);
    pinMode(IN_PIN_6, INPUT);

    SleepyPi.enablePiPower(true);
    SleepyPi.enableExtPower(false);
}

// the loop function runs over and over again forever
void loop()
{
    val1 = digitalRead(IN_PIN_1);
    val2 = digitalRead(IN_PIN_2);
    val3 = digitalRead(IN_PIN_3);
    val4 = digitalRead(IN_PIN_4);
    val5 = digitalRead(IN_PIN_5);
    val6 = digitalRead(IN_PIN_6);
    if (val1 == LOW || val2 == LOW || val3 == LOW || val4 == LOW || val5 == LOW || val6 == LOW)
    {
        digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
    }
}
