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

#include <ArduinoJson.h>

// const int IN_PIN_1 = 5;


// int val1 = 0;
int analogPin = A5;
// int OUT_SOURCE = ;
int OUT_SINK = 9;

int val = 0;





void send_serial_value(const char *key, byte value) {
    const int capacity=JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    
    doc[key] = value;
    // doc["key1"] = "value1";

    serializeJson(doc, Serial);
    Serial.println();
}



// the setup function runs once when you press reset or power the board
void setup()
{
    // initialize digital pin LED_BUILTIN as an output.
    // pinMode(LED_BUILTIN, OUTPUT);

    // pinMode(IN_PIN_1, INPUT);

    SleepyPi.enablePiPower(false);
    SleepyPi.enableExtPower(true);

    Serial.begin(9600);

    // pinMode(OUT_SOURCE, OUTPUT);
    pinMode(OUT_SINK, OUTPUT);
    // digitalWrite(OUT_SOURCE, HIGH);
    digitalWrite(OUT_SINK, LOW);
}

// the loop function runs over and over again forever
void loop()
{
    // val1 = digitalRead(IN_PIN_1);
    // val = analogRead(analogPin);
    // char key[] = "key1";
    send_serial_value("key1", 1);
    // Serial.println(val);
    // obj1["value"]=analogRead(A1);
    delay(500);

}
