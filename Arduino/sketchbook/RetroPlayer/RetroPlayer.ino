
#include <ArduinoJson.h>
#include "SleepyPi2.h"

#define DEBOUNCE 10  // button debouncer, how many ms to debounce, 5+ ms is usually plenty
 
const byte POWER_SWITCH_PIN = 2;	// Pin B0 - Arduino 8
const byte DOOR_PIN = 3;	// Pin B0 - Arduino 8
const unsigned int POWER_ON_TIMEOUT = 30; // Seconds
const unsigned int HANDSHAKE_TIMEOUT = 30; // Seconds

byte inputs[] = {4, 5, 6, 7, 8, 9};
#define NUMINPUTS sizeof(inputs)
byte inputState[NUMINPUTS];

byte arduinoState = 100;
byte piState = 0;
byte displayState = 0;
byte piAwake = 0;

void doAction0 (byte level) {
    Serial.println (0);
}

void doAction1 (byte level) {
    Serial.println (1);
}
 
void doAction2 (byte level) {
    Serial.println (2);
}

void doAction3 (byte level) {
    Serial.println (3);
}

void doAction4 (byte level) {
    Serial.println (4);
}

typedef void (*ioFunction) (byte level);
// array of function pointers
ioFunction inputCallbacks [] = {
    doAction0,
    doAction1,
    doAction2,
    doAction3,
    doAction4,
};


void read_serial_data() {

    if (Serial.available() <= 0) {
        return;
    }
    // read the incoming byte:
    int inData = Serial.read();

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, inData);
    if (error) {
        Serial.println(error.c_str()); 
        return;
    }
    // Get a reference to the root object
    JsonObject obj = doc.as<JsonObject>();
    // Loop through all the key-value pairs in obj
    for(auto p: obj) {
//        p.key(); // is a JsonString
//        p.value(); // is a JsonVariant
    }

    // TODO Set pi state
}

void check_inputs() {
    static byte newInputState[NUMINPUTS];
    static byte debounce[NUMINPUTS];
    static long lastTime[NUMINPUTS];
    byte i;
  
    for (i = 0; i < NUMINPUTS; i++) {
        if (millis() < lastTime[i]) {
            lastTime[i] = millis(); // Timer has wrapped around
        }
 
        if ((lastTime[i] + DEBOUNCE) > millis()) {
            continue; // not enough time has passed to debounce
        }
        // DEBOUNCE time passed or not started

        newInputState[i] = digitalRead(inputs[i]);   // read the input

        if (newInputState[i] == inputState[i]) {
            if (debounce[i] == 1)  {
                debounce[i] = 0; // Debounce failed, reset
            }
            continue; // Debounced failed or not started
        }
        if (debounce[i] == 1) {
            debounce[i] = 0; // Debouncing finished, stop debounce
            inputState[i] = newInputState[i];
            inputCallbacks [i] (inputState[i]);
        } else {
            debounce[i] = 1; // Debounce not started, start debounce
        }
        // Input changed. Reset timer
        lastTime[i] = millis();
    }
}

void wakeup_no_display()
{
    // Handler for the wakeup interrupt with no display
    arduinoState = 110;
}

void wakeup_with_display()
{
    // Handler for the wakeup interrupt with display
    arduinoState = 120;
}

void pi_controller() {
    static byte handshakeStatus;
    switch(piState) { // State machine to control power up sequence
        case 10: // Shutdown
            // Send shutdown signal
        case 20: // Shutting down
            //TODO Check current. When confirmed off:
                // piState = 50;
                // TODO Clear serial buffer?
                // SleepyPi.enablePiPower(false); 
                // piState = 30;
        case 50: // Pi is off
            // 
        case 110: // Wakeup Pi
            SleepyPi.enablePiPower(true);
            piState = 120;
        case 120: // Wait for Pi wakeup confirmation
            if (piAwake == 1) {
                piState = 130;
                handshakeStatus = 0
                // Send pi initial data
                //TODO Send serial data here
            }
        case 130: // Wait for handshake confirmation
            if (handshakeStatus == 1) {
                lastHandshakeTime = millis();
                piState = 150;
            }
        case 140: // Handshake confirmed. Check time since last message
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                piState = 160;
                // Send pi handshake data
                // TODO Send pi data
            }

    }
}

void arduino_power() {
    static long onTime;

    switch(arduinoState) { // State machine to control arduino power
        case 10: //Request to sleep
            // Ensure pi is off.
            // if (SleepyPi.power????)
                // arduinoState = 50;
        case 50: // Low power state
             // Attach WAKEUP_PIN to wakeup ATMega
            attachInterrupt(digitalPinToInterrupt(DOOR_PIN), wakeup_no_display, RISING);
            attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), wakeup_with_display, RISING);

            // Enter power down state with ADC and BOD module disabled.
            // Wake up when wake up pins are high.
            SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
            
            // ###################### WOKEN UP ######################
            // Disable external pin interrupt on WAKEUP_PIN pin.
            detachInterrupt(DOOR_PIN);
            detachInterrupt(POWER_SWITCH_PIN);

            onTime = millis(); // Start timing power on time

            piState = 110; // Wakeup pi when arduino is awake
        case 110: // Woken up, no display
            // If display is off and timeout has passed, shutdown
            if ((onTime + POWER_ON_TIMEOUT) > millis() ) {
                piState = 10; // Shutdown Pi
                arduinoState = 10; // Shutdown arduino
            }
        case 120: // Woken up, display is on
            // TODO Send display state to pi

    }
}

void setup() {
    // Set the initial Power to be off
    SleepyPi.enablePiPower(false);  
    SleepyPi.enableExtPower(false);

    // Configure "Wakeup" pins as inputs
    pinMode(DOOR_PIN, INPUT);
    pinMode(POWER_SWITCH_PIN, INPUT);
    
    Serial.begin(1152000);

    // Make input & enable pull-up resistors on switch pins
    for (byte i=0; i< NUMINPUTS; i++) {
        pinMode(inputs[i], INPUT);
        inputState[i] = digitalRead(inputs[i]);   // read the inputs
    }
}

void loop() {
    arduino_power() // State machine to handle arduino power
    pi_controller() // State machine to handle pi power and handshaking
    if (arduinoState > 100) { // If arduino is on, read inputs
        check_inputs();
    }
    if (piState > 100) { // If pi is on, read serial
        read_serial_data();
    }
}
