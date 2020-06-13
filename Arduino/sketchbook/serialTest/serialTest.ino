// #include "LowPower.h"
// #include "PCF8523.h"
// #include "SleepyPi2.h"

#include <ArduinoJson.h>

// const int IN_PIN_1 = 5;


// int val1 = 0;
int analogPin = A3;
// int OUT_SOURCE = ;
int OUT_SINK = 9;

int val = 0;





void send_serial_value(const char *key, int value) {
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

    // SleepyPi.enablePiPower(false);
    // SleepyPi.enableExtPower(true);

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
    int val = analogRead(analogPin);
    // char key[] = "key1";
    // send_serial_value("A3", val);
    Serial.println(val);
    // obj1["value"]=analogRead(A1);
    delay(500);

}
