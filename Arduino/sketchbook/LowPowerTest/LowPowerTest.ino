#include "LowPower.h"
#include "PCF8523.h"
#include "SleepyPi2.h"

const byte POWER_SWITCH_PIN = 2; // Temp changed to 3
// const byte DOOR_PIN = 3;

void wakeup_no_display() {
    return;
}
void wakeup_manual() {
    return;
}

SleepyPiClass sleepyPi;

void setup() {
    sleepyPi.enableExtPower(false);
    pinMode(9, INPUT);

    sleepyPi.rtcStop_32768_Clkout();
    sleepyPi.rtcClearInterrupts();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);
    // pinMode(POWER_SWITCH_PIN, INPUT);
    // digitalWrite(POWER_SWITCH_PIN, HIGH);

    Serial.begin(9600);
}
void loop() {
    // attachInterrupt(digitalPinToInterrupt(DOOR_PIN), wakeup_no_display, RISING);
    attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), wakeup_manual, FALLING);
    // attachInterrupt(0, wakeup_manual, LOW);

    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pins are high.
    sleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    // delay(5000);
    // bool rtcOn = sleepyPi.rtcInit(true);
    // Serial.println(rtcOn);
    
    // ###################### WOKEN UP ######################
    // Disable external pin interrupts.
    // detachInterrupt(DOOR_PIN);
    detachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN));
    // // detachInterrupt(0);

    // // pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);
    // // digitalWrite(POWER_SWITCH_PIN, HIGH);

    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW); // turn the LED off
    delay(1000);

    digitalWrite(LED_BUILTIN, digitalRead(POWER_SWITCH_PIN));
    delay(5000);

    digitalWrite(LED_BUILTIN, HIGH); // turn the LED on
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW); // turn the LED off
    delay(1000);
}