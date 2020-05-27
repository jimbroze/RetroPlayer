
// #include <string>
#include <ArduinoJson.h>
#include "LowPower.h"
#include "PCF8523.h"
#include "SleepyPi2.h"

class SleepyPlayer;

// Abstract base class denoted by virtual = 0
class StateMachine {
    public:
        virtual void process() = 0;
        // void set_state();

    private:
        byte state;
};

class PiController {
    friend class SleepyPlayer;
    public:
        void process();

    private:
        SleepyPlayer *sleepyPi_;
        byte myState;
        byte handshakeStatus;
        long lastHandshakeTime;
        bool piPower;
        
};

class ArduinoController {
    friend class SleepyPlayer;
    public:
        void process();

    private:
        SleepyPlayer *sleepyPi_;
        byte myState;
        long onTime;

        // FIXME NEED CONSTRUCTORS!!!!!!!!!!!!!!
};

class SleepyPlayer: public SleepyPiClass {
    private:
        PiController *piController_;
        ArduinoController *arduinoController_;
    public:
        SleepyPlayer(PiController *piController, ArduinoController *arduinoController) {
            piController_ = piController;
            arduinoController_ = arduinoController;
        }

        inline void set_pi_state(byte state) { piController_->myState = state; }
        inline void set_arduino_state(byte state) { arduinoController_->myState = state; }
        inline byte get_pi_state() { return piController_->myState; }
        inline byte get_arduino_state() { return arduinoController_->myState; }
        inline void process_pi() { piController_->process(); }
        inline void process_arduino() { arduinoController_->process(); }
};



const byte DEBOUNCE = 10;  // button debouncer, how many ms to debounce, 5+ ms is usually plenty
 
const byte POWER_SWITCH_PIN = 2;	// Pin B0 - Arduino 8
const byte DOOR_PIN = 3;	// Pin B0 - Arduino 8
const unsigned int POWER_ON_TIMEOUT = 30; // Seconds
const unsigned int HANDSHAKE_TIMEOUT = 30; // Seconds

byte inputs[] = {2, 3, 5, 6, 8, 11, 12};
byte outputs[] = {7, 10, 13};
byte analogues[][2] = {"A0", "A1", "A4", "A5"};
byte ANALOGUE_SINK = 9;
byte ANALOGUE_AVERAGES = 6; // Even no. so that jumps between 2 numbers are centralised

// Serial OUT
const char serialKeys[][7] = {
    "disp",
    "val2"
};
byte disp = 1;
byte val2 = 1;
byte *handshakeData[] = {
    &disp,
    &val2
};
// Single values:
// errSer = Serial error (String)

// Serial IN
const int SERIAL_DICT_CAPACITY = JSON_OBJECT_SIZE(2); // + JSON_ARRAY_SIZE(2) If size 3 object includes size 2 array
const byte numChars = 64; //Increase if required for more data.
// Serial buffer is 64 bytes but program should read faster than buffer

// TODO Can values change without function?? i.e state machine checks values. Much simpler
void pi_on(byte data) {
    byte awake = data;
}
void funcB(byte) {
    return;
}
struct command {
    char* funcName;
    void (*function)(byte);
};
struct command commands[] = {
    {"piOn", pi_on},
    {"strB", funcB},
    {0,0}
};

byte arduinoState = 50;
byte piState = 50;
byte displayOn = 0;
byte piAwake = 0;

const byte NUM_INPUTS = sizeof(inputs);
byte inputState[NUM_INPUTS];

const byte NUM_OUTPUTS = sizeof(outputs);
byte outputState[NUM_OUTPUTS];

const byte NUM_ANALOGUES = sizeof(analogues);
byte analogueState[NUM_ANALOGUES];

const byte SERIAL_ARRAY_SIZE = sizeof(serialKeys);




// ************************* CALLBACK HANDLERS *************************

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


// ************************* SERIAL FUNCS *************************

void read_serial_data() {
    static byte ndx = 0; // Index
    char endMarker = '\n';
    char dataIn;
    char receivedChars[numChars]; 

    // while (Serial.available() > 0 && newData == false) {
    while (Serial.available() > 0) {
        dataIn = Serial.read();

        if (dataIn != endMarker) {
            receivedChars[ndx] = dataIn;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            // newData = true;
            process_serial_data(receivedChars);
        }
    }

}

void process_serial_data(char serialData[]) {
    StaticJsonDocument<SERIAL_DICT_CAPACITY> doc;
    DeserializationError error = deserializeJson(doc, serialData);
    if (error) {
        send_serial_string("errSer", error.c_str());
        return;
    }
    // Get a reference to the root object
    JsonObject obj = doc.as<JsonObject>();
    // Loop through all the key-value pairs in obj

    for(auto p: obj) {
        struct command *scan;
        for(scan = commands; scan->function; scan++) {
            if(!strcmp(p.key().c_str(), scan->funcName)) {
                scan->function(p.value());
                break;
            }
            // TODO Look for 0?
        }
    }
}

void send_serial_value(const char *key, byte value) {
    const int capacity=JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    
    doc[key] = value;
    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}

void send_serial_string(const char *key, const char *value) {
    const int capacity=JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    
    doc[key] = value;
    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}

void send_serial_dict() {
    const int capacity=JSON_OBJECT_SIZE(SERIAL_ARRAY_SIZE);
    StaticJsonDocument<capacity> doc;

    for (byte i = 0; i < SERIAL_ARRAY_SIZE; i++)
        doc[serialKeys[i]] = *handshakeData[i];
    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}

// ************************* HARDWARE IO *************************

void check_inputs() {
    static byte newInputState[NUM_INPUTS];
    static byte debounce[NUM_INPUTS];
    static long lastTime[NUM_INPUTS];
  
    for (byte i = 0; i < NUM_INPUTS; i++) {
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
            inputCallbacks[i] (inputState[i]);
        } else {
            debounce[i] = 1; // Debounce not started, start debounce
        }
        // Input changed. Reset timer
        lastTime[i] = millis();
    }
}

void check_analogues() {
    static int throwaway;
    // static byte debounce;
    for (byte i=0; i< NUM_ANALOGUES; i++) {
        // Ignore first reading to allow capacitor to charge. Especially high impedance pot
        throwaway = analogRead(analogues[i]);
        byte analogueValue = analogRead(analogues[i]);
        for(byte j=0; j < ANALOGUE_AVERAGES; j++) { 
            delay(10);
            analogueValue = analogRead(analogues[i]);
        }
        analogueState[i] = analogueValue / ANALOGUE_AVERAGES;
        // TODO Filter??
    }
}


// ************************* STATE MACHINES *************************


void PiController::process() {
    switch(myState) { // State machine to control power up sequence
        case 10: // Shutdown
            // Send shutdown signal
        case 20: // Shutting down
            //TODO Check current. When confirmed off:
            piPower = sleepyPi_->checkPiStatus(false);  // Don't Cut Power automatically
            if (piPower == false) {
                myState = 30;
                // TODO Clear serial buffer?
                sleepyPi_->enablePiPower(false); 
                myState = 50;
            }
        case 50: // Pi is off
            // 
        case 110: // Wakeup Pi
            sleepyPi_->enablePiPower(true);
            myState = 120;
        case 120: // Wait for Pi wakeup confirmation
            if (piAwake == 1) {
                myState = 130;
                handshakeStatus = 1;
                // Send pi initial data
                send_serial_dict();
                //TODO Add timeout
            }
        case 130: // Wait for handshake confirmation
            if (handshakeStatus == 2) {
                lastHandshakeTime = millis();
                myState = 150;
                // FIXME two handshake states?
            }
        case 140: // Handshake confirmed. Check time since last message
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                myState = 160;
                // Send pi handshake data
                // TODO Send pi data as struct?
            }
        case 200: // Maintenence mode
            break;
        default:
            break;
    }
}

void ArduinoController::process() {
    switch(myState) { // State machine to control arduino power
        case 10: //Request to sleep
            // Ensure pi is off.
            // if (sleepyPi.power????)
                // arduinoState = 50;
        case 50: // Low power state
             // Attach WAKEUP_PIN to wakeup ATMega
            attachInterrupt(digitalPinToInterrupt(DOOR_PIN), wakeup_no_display, RISING);
            attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), wakeup_with_display, RISING);

            // Enter power down state with ADC and BOD module disabled.
            // Wake up when wake up pins are high.
            sleepyPi_->powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
            
            // ###################### WOKEN UP ######################
            // Disable external pin interrupt on WAKEUP_PIN pin.
            detachInterrupt(DOOR_PIN);
            detachInterrupt(POWER_SWITCH_PIN);

            onTime = millis(); // Start timing power on time

            sleepyPi_->set_pi_state(110); // Wakeup pi when arduino is awake
        case 110: // Woken up, no display
            // If display is off and timeout has passed, shutdown
            if ((onTime + POWER_ON_TIMEOUT) > millis() ) {
                sleepyPi_->set_pi_state(10); // Shutdown Pi
                myState = 10; // Shutdown arduino
            }
            // TODO If inputs tripped, turn display on
        case 120: // Woken up, display should be on
            displayOn = 1;
            if (sleepyPi_->get_pi_state() > 120) { // Initial data already sent. Send display on seperate
                send_serial_value("disp", 1);
            }
            myState = 130;
        case 150: // Standard awake state

        case 200: // Maintenence mode request
            // Turn off analogue source and sink
            sleepyPi_->enableExtPower(false);
            pinMode(ANALOGUE_SINK, INPUT);
            send_serial_value("state", myState);
            myState = 202;

        case 202:  // In maintenence mode
            break;
    }
}



// ************************* MAIN PROGRAM *************************

SleepyPlayer sleepyPi(new PiController, new ArduinoController);

void setup() {
    Serial.begin(9600);

    // Set the initial Power to be off
    sleepyPi.enablePiPower(false);  
    sleepyPi.enableExtPower(false);

    // Configure "Wakeup" pins as inputs
    pinMode(DOOR_PIN, INPUT);
    pinMode(POWER_SWITCH_PIN, INPUT);
    
    // Setup inputs. Off is high, on is low
    for (byte i=0; i< NUM_INPUTS; i++) {
        pinMode(inputs[i], INPUT);
        inputState[i] = digitalRead(inputs[i]);   // read the inputs
    }

    // Setup outputs
    for (byte i=0; i< NUM_OUTPUTS; i++) {
        pinMode(outputs[i], OUTPUT);
        digitalWrite(outputs[i], LOW);  // Set to off
    }
}

void loop() {
    sleepyPi.process_arduino(); // State machine to handle arduino power
    sleepyPi.process_pi(); // State machine to handle pi power and handshaking
    if (sleepyPi.get_arduino_state() > 100) { // If arduino is on, read inputs
        check_inputs();
        check_analogues();
    }
    if (sleepyPi.get_pi_state() > 100) { // If pi is on, read serial
        read_serial_data();
    }
}
