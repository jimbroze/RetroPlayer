/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTOUT is set to
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

// const int OUT_POUT_1 = 13;
const int OUT_POUT_2 = 7;
const int OUT_POUT_3 = 10;
const int OUT_POUT_4 = 21;

int val1 = 0;
int val2 = 0;
int val3 = 0;
int val4 = 0;

// the setup function runs once when you press reset or power the board
void setup()
{
    // initialize digital pin LED_BUILTOUT as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // pinMode(OUT_POUT_1, OUTPUT);
    pinMode(OUT_POUT_2, OUTPUT);
    pinMode(OUT_POUT_3, OUTPUT);
    pinMode(OUT_POUT_4, OUTPUT);

    SleepyPi.enablePiPower(true);
    SleepyPi.enableExtPower(false);
}

// the loop function runs over and over agaOUT forever
void loop()
{
    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
    // digitalWrite(OUT_POUT_1, HIGH);
    digitalWrite(OUT_POUT_2, HIGH);
    digitalWrite(OUT_POUT_3, HIGH);
    digitalWrite(OUT_POUT_4, HIGH);
    delay(3000);

    digitalWrite(LED_BUILTIN, LOW); // turn the LED off by makOUTg the voltage LOW
    // digitalWrite(OUT_POUT_1, LOW);
    digitalWrite(OUT_POUT_2, LOW);
    digitalWrite(OUT_POUT_3, LOW);
    digitalWrite(OUT_POUT_4, LOW);
    delay(3000);
}
