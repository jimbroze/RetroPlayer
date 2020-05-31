
// #include <string>
#include <ArduinoJson.h>
#include "LowPower.h"
#include "PCF8523.h"
#include "SleepyPi2.h"

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

// Abstract base class denoted by virtual = 0
class StateMachine {
    public:
        virtual void process() = 0;
        // void set_state();

    private:
        byte state;
};

class RetroPlayer {
    public:
        void power_control();
        void handshake();
        void wake_on_disp();
        void wake_no_disp();
        void display_on();
        void wakeup_pi();
        void maintenence_mode();
        void normal_mode();
        void shutdown();

        void check_dig_inputs();
        void check_analogues();

        // void wakeup_with_display();
        // void wakeup_no_display();

    private:
        SleepyPiClass *sleepyPi_;
        // byte myState;
        byte handshakeStatus;
        long lastHandshakeTime;
        bool piPower;

        void read_serial_data();
        void process_serial_data(char serialData[]);
        void send_serial(const char *key, byte value);
        void send_serial(const char *key, int value);
        void send_serial(const char *key, const char *value);
        void send_serial(const char *key, byte value[]);
        void send_serial(const char *key, int value[]);
        void send_serial();
        
        void update_value(byte key);
        void update_value(byte key, byte value[]);
        void update_value(byte key, int value[]);

        void power_switch(byte level);
        void door_light(byte level);
        void ignition(byte level);
        void boot_release_but(byte level);
        void spare_in(byte level);
        void air_horns(byte level);

        long ardOnTime;
        enum PlayerState {
            shuttingDown, off, lowPower, dispOff, dispOnAuto, dispOnManual, shutdownTimeout
        } myState;
        enum Handshake {
            none, sent, success, failed
        } handshakeState;

        typedef void (RetroPlayer::*ioFunction)(byte);
        // array of function pointers
        ioFunction inputCallbacks[6] = {
            &RetroPlayer::power_switch,
            &RetroPlayer::door_light,
            &RetroPlayer::ignition,
            &RetroPlayer::boot_release_but,
            &RetroPlayer::spare_in,
            &RetroPlayer::air_horns
        };
};


void wakeup_with_display();
void wakeup_no_display();



const byte DEBOUNCE = 10;  // input debouncer (ms)
 
const byte POWER_SWITCH_PIN = 2;	// Pin B0 - Arduino 8
const byte DOOR_PIN = 3;	// Pin B0 - Arduino 8
const unsigned int POWER_ON_TIMEOUT = 30; // Seconds
const unsigned int HANDSHAKE_TIMEOUT = 30; // Seconds

byte inputs[] = {2, 3, 5, 6, 11, 12};
byte outputs[] = {7, 10, 13};
byte analogues[][2] = {"A0", "A1", "A4", "A5"};
byte ANALOGUE_SINK = 9;
byte ANALOGUE_AVERAGES = 6; // Even no. so that jumps between 2 numbers are centralised


const byte NUM_INPUTS = sizeof(inputs);
byte inputState[NUM_INPUTS];

const byte NUM_OUTPUTS = sizeof(outputs);
byte outputState[NUM_OUTPUTS];

const byte NUM_ANALOGUES = sizeof(analogues);
byte analogueState[NUM_ANALOGUES];


// Serial OUTPUTS
#define DIGITAL_IN 0
#define ANALOG 1
#define DISPLAY 2
#define OFF 3
#define MAINT 4
#define POWER_SWITCH 5
#define INT_LIGHTS 6
#define IGNITION 7

const char serialOutKeys[][7] = {
    "dig",
    "analog",
    "disp",
    "off",
    "maint",
    "volSw",
};

const byte SERIAL_OUT_SIZE = sizeof(serialOutKeys) / sizeof(serialOutKeys)[0];

//Intial byte values
byte playerOut[SERIAL_OUT_SIZE] = {
    0,
    0,
    0,
    0,
    0,
    0
};

const unsigned int SERIAL_OUT_CAPACITY = JSON_OBJECT_SIZE(SERIAL_OUT_SIZE) + JSON_ARRAY_SIZE(NUM_INPUTS) + JSON_ARRAY_SIZE(NUM_ANALOGUES);

// Single values:
// errSer = Serial error (String)

// Serial IN
#define PI_AWAKE 1
#define HANDSHAKE_RECEIVED 1

const char serialInKeys[][7] = {
    "awake",
    "hand"
};
const byte SERIAL_IN_SIZE = sizeof(serialInKeys) / sizeof(serialInKeys)[0];

byte playerIn[SERIAL_IN_SIZE] = {
    0,
    0,
};


const unsigned int SERIAL_IN_CAPACITY = JSON_OBJECT_SIZE(SERIAL_IN_SIZE) + JSON_ARRAY_SIZE(NUM_OUTPUTS);
// If size 3 object includes size 2 array
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

// ************************* SERIAL FUNCS *************************

void RetroPlayer::send_serial(const char *key, byte value) { // Byte
    const int capacity=JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    
    doc[key] = value;
    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}

void RetroPlayer::send_serial(const char *key, int value) { // Byte
    const int capacity=JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    
    doc[key] = value;
    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}

void RetroPlayer::send_serial(const char *key, const char *value) { // C_string
    const int capacity=JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    
    doc[key] = value;
    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}

void RetroPlayer::send_serial(const char *key, byte value[]) { // byte Array
    const byte arraySize = sizeof(value);
    const int capacity=JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(arraySize);
    StaticJsonDocument<capacity> doc;

    for (byte i = 0; i < arraySize; i++) {
        doc[key][i] = value[i];
    }
    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}

void RetroPlayer::send_serial(const char *key, int value[]) { // int Array
    const byte arraySize = sizeof(value);
    const int capacity=JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(arraySize);
    StaticJsonDocument<capacity> doc;

    for (byte i = 0; i < arraySize; i++) {
        doc[key][i] = value[i];
    }
    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}

void RetroPlayer::send_serial() { // Dict
    const int capacity=JSON_OBJECT_SIZE(SERIAL_IN_SIZE);
    StaticJsonDocument<capacity> doc;

    doc[serialOutKeys[DIGITAL_IN]] = inputState;
    doc[serialOutKeys[ANALOG]] = analogueState;
    doc[serialOutKeys[DISPLAY]] = playerOut[DISPLAY];
    doc[serialOutKeys[OFF]] = playerOut[OFF];

    // Produce a minified JSON document
    serializeJson(doc, Serial);
    Serial.println();
}


void RetroPlayer::process_serial_data(char serialData[]) {
    StaticJsonDocument<SERIAL_IN_CAPACITY> doc;
    DeserializationError error = deserializeJson(doc, serialData);
    if (error) {
        send_serial("errSer", error.c_str());
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

void RetroPlayer::read_serial_data() {
    if (myState != dispOn && myState != dispOff) {
        return;
    }
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
            this->process_serial_data(receivedChars);
        }
    }
}

void RetroPlayer::update_value(byte key) {
    if (handshakeState == none) {
        return;
    }
    this->send_serial(serialOutKeys[key], playerOut[key]);
}
void RetroPlayer::update_value(byte key, byte value[]) { // Byte array
    if (handshakeState == none) {
        return;
    }
    this->send_serial(serialInKeys[key], value);
}
void RetroPlayer::update_value(byte key, int value[]) { // int array
    if (handshakeState == none) {
        return;
    }
    this->send_serial(serialInKeys[key], value);
}

void RetroPlayer::handshake() {
    this->read_serial_data();
    switch(handshakeState) {
        case none: //Send handshake
            if (playerIn[PI_AWAKE] == 1) {
                // Send pi all data
                send_serial();
                handshakeState = 1;
                //TODO Add timeout
            }
        case sent: // Sent, wait for receive
            if (playerIn[HANDSHAKE_RECEIVED] == 1) {
                lastHandshakeTime = millis();
                handshakeState = 2;
            }
        case success:
            // Handshake confirmed. Check time since last message
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                handshakeState = 0;
                playerIn[HANDSHAKE_RECEIVED] = 0;
            }
    }
}



// ************************* HARDWARE IO *************************

void RetroPlayer::power_switch(byte level) {
    volSwitch = level;
}
void RetroPlayer::door_light(byte level) {
    intLights = level;
}
void RetroPlayer::ignition(byte level) {
    ignition = level;
}
void RetroPlayer::boot_release_but(byte level) {
    return;
}
void RetroPlayer::spare_in(byte level) {
    return;
}
void RetroPlayer::air_horns(byte level) {
    return;
}

void RetroPlayer::check_dig_inputs() {
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
            CALL_MEMBER_FN(*this, inputCallbacks[i]) (inputState[i]); // Use #define macro to call input functions
            update_value(DIGITAL_IN, inputState); // Send inputs when one changes
        } else {
            debounce[i] = 1; // Debounce not started, start debounce
        }
        // Input changed. Reset timer
        lastTime[i] = millis();
    }
}

void RetroPlayer::check_analogues() {
    int throwaway;
    int newAnalogueState[NUM_ANALOGUES];
    int thresholds[NUM_ANALOGUES];
    // static byte debounce;
    for (byte i=0; i< NUM_ANALOGUES; i++) {
        // Ignore first reading to allow capacitor to charge. Especially high impedance pot
        throwaway = analogRead(analogues[i]);
        byte analogueValue = analogRead(analogues[i]);
        for(byte j=0; j < ANALOGUE_AVERAGES; j++) { 
            delay(10);
            analogueValue = analogRead(analogues[i]);
        }
        newAnalogueState[i] = analogueValue / ANALOGUE_AVERAGES;
        int diff = newAnalogueState[i] - analogueState[i];
        if (abs(diff) > thresholds[i]) {
            analogueState[i] = newAnalogueState[i];
        }
    }
}

// ************************* STATE FUNCTIONS AND MACHINE *************************

void RetroPlayer::display_on(bool wakeup = false) {
    playerOut[DISPLAY]= 1;
    if (wakeup == true) { return; }
    this->update_value(DISPLAY, 1);
    
}
void RetroPlayer::wakeup() {
    if (playerOut[DISPLAY] == 1) {
        if (ignition == 1) {
            myState = dispOnAuto;
        }
        else
        {
            myState = dispOnManual;
        }
    }
    else
    {
        myState = dispOff;
    }
}
void RetroPlayer::wakeup_pi() {
    sleepyPi_->enablePiPower(true);
}
void RetroPlayer::maintenence_mode() { // Cancel analog power setup so i2c can be used
    sleepyPi_->enableExtPower(false);
    pinMode(ANALOGUE_SINK, INPUT);
    playerOut[MAINT]= 1;
    this->update_value(MAINT);
}
void RetroPlayer::normal_mode() { // Setup analogues to read
    sleepyPi_->enableExtPower(true);
    pinMode(ANALOGUE_SINK, OUTPUT);
    digitalWrite(ANALOGUE_SINK, LOW);
    playerOut[MAINT]= 0;
    this->update_value(MAINT);
}
void RetroPlayer::shutdown_request(byte shutdownType = 2) {
    shutdownTimer = millis();
    playerOut[OFF]= shutdownType; //Shutdown request code (2=auto, 3=manual)
    this->update_value(OFF);
    if (shutdownType == 3) {
        myState = shutdownTimeoutManual;
    } else {
        myState = shutdownTimeoutAuto;
    }
}
void RetroPlayer::shutdown() {
    // Send shutdown signal
    playerOut[OFF]= 1;
    this->update_value(OFF);
    myState = shuttingDown; // Shutdown arduino
}


void RetroPlayer::power_control() {
    switch(myState) { // State machine to control power up sequence
        case shuttingDown: // Shutting down
            //TODO Add timeout involving piAwake
            piPower = sleepyPi_->checkPiStatus(false);  // Don't Cut Power automatically
            if (piPower == false) {
                // TODO Clear serial buffer?
                sleepyPi_->enablePiPower(false); 
                myState = off;
            }
        case off: // Low power state
             // Attach WAKEUP_PIN to wakeup ATMega
            attachInterrupt(digitalPinToInterrupt(DOOR_PIN), wakeup_no_display, RISING);
            attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), wakeup_with_display, RISING);

            // Enter power down state with ADC and BOD module disabled.
            // Wake up when wake up pins are high.
            sleepyPi_->powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
            
            // ###################### WOKEN UP ######################
            // Disable external pin interrupts.
            detachInterrupt(DOOR_PIN);
            detachInterrupt(POWER_SWITCH_PIN);

            if (volSwitch == 1) { // If switch is off, go back to sleep.
                playerOut[DISPLAY] == 0;
                break;
            }

            ardOnTime = millis(); // Start timing power on time

            wakeup(); // Set next state based on inputs
            wakeup_pi(); // Wakeup pi when arduino is awake
        case lowPower: // Arduino on but pi off. Currently unused
            break;
        case dispOff: // Woken up, no display
            // If display is off and timeout has passed, shutdown
            // TODO If inputs tripped, turn display on
            if (ignition == 1) {
                display_on();
                break;
            }
            if (volSwitch == 1) { //FIXME
                display_on();
                break;
            }
            if ((ardOnTime + POWER_ON_TIMEOUT) > millis() ) {
                shutdown(); // Shutdown Pi
            }
        // *********** Regular auto on/off (Not using vol switch) ***********
        case dispOnAuto: // Woken up with ignition, display should be on
            if (ignition == 0 && intLights == 1) {
                shutdown_request(2); // Auto
            }
            if (volSwitch == 0) { 
                shutdown_request(3); // Manual
            }
        case shutdownTimeoutAuto: 
            if ((shutdownTimer + SHUTDOWN_TIMEOUT) > millis() ) {
                shutdown();
            }
            if (ignition == 1  || keepAlive == 1) {
                wakeup(); //Cancel shutdown 
            }
        // *********** Manual on/off (Using vol switch) ***********
        case dispOnManual: // Woken up but ignition is off (Using vol switch)
            if (volSwitch == 0) {
                shutdown_request();
            }
            if (ignition == 1) {
                myState = dispOnAuto;
            }
        case shutdownTimeoutManual: // Power switch turned off
            if ((shutdownTimer + SHUTDOWN_TIMEOUT) > millis() ) {
                shutdown();
            }
            if (volSwitch == 1 || keepAlive == 1) {
                wakeup(); //Cancel shutdown 
            }
    }
}


// ************************* MAIN PROGRAM *************************

SleepyPiClass sleepyPi;
RetroPlayer retroPlayer;

void wakeup_no_display()
{
    // Handler for the wakeup interrupt with no display
    // retroPlayer.wake_on_disp();
    return;
}

void wakeup_with_display()
{
    // Handler for the wakeup interrupt with display
    retroPlayer.display_on(wakeup=true);
}

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
    retroPlayer.power_control(); // State machine to handle arduino + Pi power
    // Controllers check state during following functions
    retroPlayer.check_dig_inputs();
    retroPlayer.check_analogues();
    retroPlayer.handshake();
}
