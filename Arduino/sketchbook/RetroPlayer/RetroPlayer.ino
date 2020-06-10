
// #include <string>
// #include <ArduinoJson.h>
#include "LowPower.h"
#include "PCF8523.h"
#include "SleepyPi2.h"

// Serial constants. MUST BE BEFORE SerialComms INCLUDE.
byte NUM_INPUTS {6};
byte NUM_OUTPUTS {3};
byte NUM_ANALOGUES {4};
constexpr int DATA_MEMBERS {3}; //FIXME Remove and make dynamic json document
constexpr int MAX_DATA_ARRAY = max(NUM_INPUTS, NUM_ANALOGUES);
const unsigned int SERIAL_OUT_CAPACITY {JSON_OBJECT_SIZE(DATA_MEMBERS) + JSON_ARRAY_SIZE(NUM_INPUTS) + JSON_ARRAY_SIZE(NUM_ANALOGUES)};
const unsigned int SERIAL_IN_CAPACITY = JSON_OBJECT_SIZE(SERIAL_IN_SIZE) + JSON_ARRAY_SIZE(NUM_OUTPUTS);
// If size 3 object includes size 2 array
const byte numChars = 64; //Increase if required for more data.
// Serial buffer is 64 bytes but program should read faster than buffer


#include "SerialComms.h"

    retroComms.sendData(&data1);
    delay(1000);
    retroComms.sendData(&data2);
    delay(1000);
    retroComms.sendData(data3);
    delay(1000);
    retroComms.sendData();
    delay(8000);

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))




// Abstract base class denoted by virtual = 0
class StateMachine {
    public:
        virtual void process() = 0;
        // void set_state();

    private:
        byte state;
};




void wakeup_manual();
void wakeup_no_display();



const byte DEBOUNCE = 10;  // input debouncer (ms)
 
const byte POWER_SWITCH_PIN = 2;	// Pin B0 - Arduino 8
const byte DOOR_PIN = 3;	// Pin B0 - Arduino 8
const unsigned int POWER_ON_TIMEOUT = 30 * 1000; // Seconds to millis
const unsigned int HANDSHAKE_TIMEOUT = 30 * 1000; // Seconds
const unsigned int SHUTDOWN_REQ_TIMEOUT = 30 * 1000; // Seconds
const unsigned int SHUTDOWN_TIMEOUT = 30 * 1000; // Seconds

byte inputs[NUM_INPUTS] = {2, 3, 5, 6, 11, 12};
byte outputs[NUM_OUTPUTS] = {7, 10, 13};
byte analogues[NUM_ANALOGUES][2] = {"A0", "A1", "A4", "A5"};
byte ANALOGUE_SINK = 9;
byte ANALOGUE_AVERAGES = 6; // Even no. so that jumps between 2 numbers are centralised

byte digitalState[NUM_INPUTS];
byte outputState[NUM_OUTPUTS];
byte analogueState[NUM_ANALOGUES];

// Single values:
// errSer = Serial error (String)

// Serial IN
#define PI_AWAKE 0
#define HANDSHAKE_RECEIVED 1
#define KEEP_ALIVE 2

const char serialInKeys[][7] = {
    "awake",
    "hand",
    "alive"
};
const byte SERIAL_IN_SIZE = sizeof(serialInKeys) / sizeof(serialInKeys)[0];


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

void RetroPlayer::handshake() {
    this->read_serial_data();
    switch(handshakeState) {
        case none: // Send handshake
            if (playerIn[PI_AWAKE] == 1) {
                // Send pi all data
                handshake = 1; // Sending/sent
                comms.sendData();
                lastHandshakeTime = millis();
                handshakeState = sent;
            }
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                handshake = 3; // No Awake signal received.
                comms.sendData(&handshake);
                lastHandshakeTime = millis();
            }
        case sent: // Sent, wait for receive
            if (playerIn[HANDSHAKE_RECEIVED] == 1) {
                handshake = 2; // Success
                comms.sendData(&handshake);
                lastHandshakeTime = millis();
                handshakeState = success;
            }
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                handshake = 4; // No Received signal received.
                comms.sendData(&handshake);
                lastHandshakeTime = millis();
            }
        case success:
            // Handshake confirmed. Check time since last message
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                playerIn[HANDSHAKE_RECEIVED] = 0; //TODO Don't change playerIn directly
                handshake = 0; // Require new handshake
                lastHandshakeTime = millis();
                handshakeState = none;
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
void RetroPlayer::ignition_func(byte level) {
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

        if (newInputState[i] == digitalState[i]) {
            if (debounce[i] == 1)  {
                debounce[i] = 0; // Debounce failed, reset
            }
            continue; // Debounced failed or not started
        }
        if (debounce[i] == 1) {
            debounce[i] = 0; // Debouncing finished, stop debounce
            digitalState[i] = newInputState[i];
            CALL_MEMBER_FN(*this, inputCallbacks[i]) (digitalState[i]); // Use #define macro to call input functions
            comms.sendData(digitalState); // Send inputs when one changes
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

// **************** STATE FUNCTIONS AND MACHINE ****************

void RetroPlayer::display_on(bool wakeup = false) {
    display = 1;
    if (wakeup == true) { return; }
    comms.sendData(&display);
}
void RetroPlayer::wakeup() {
    sleepyPi_->enablePiPower(true);
    if (display == 1) {
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
void RetroPlayer::maintenence_mode() { // Cancel analog power setup so i2c can be used
    sleepyPi_->enableExtPower(false);
    pinMode(ANALOGUE_SINK, INPUT);
    mode = 1;
    comms.sendData(&mode);
}
void RetroPlayer::normal_mode() { // Setup analogues to read
    sleepyPi_->enableExtPower(true);
    pinMode(ANALOGUE_SINK, OUTPUT);
    digitalWrite(ANALOGUE_SINK, LOW);
    mode = 0;
    comms.sendData(&display);
}
void RetroPlayer::shutdown_request(byte shutdownType = 2) {
    shutdownTime = millis();
    off = shutdownType; //Shutdown request code (2=auto, 3=manual)
    comms.sendData(&off);
    if (shutdownType == 3) {
        myState = shutdownTimeoutManual;
    } else {
        myState = shutdownTimeoutAuto;
    }
}
void RetroPlayer::shutdown() {
    // Send shutdown signal
    off = 1;
    comms.sendData(&off);
    shutdownTime = millis();
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
            if ( (shutdownTime + SHUTDOWN_TIMEOUT) > millis() ) {
                off = 4; // Shutdown not completed
                comms.sendData(&off);
            }
        case off: // Low power state
             // Attach WAKEUP_PIN to wakeup ATMega
            attachInterrupt(digitalPinToInterrupt(DOOR_PIN), wakeup_no_display, RISING);
            attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), wakeup_manual, RISING);
            digitalWrite(LED_BUILTIN, LOW); // turn the LED on (HIGH is the voltage level)

            // Enter power down state with ADC and BOD module disabled.
            // Wake up when wake up pins are high.
            sleepyPi_->powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
            
            // ###################### WOKEN UP ######################
            // Disable external pin interrupts.
            detachInterrupt(DOOR_PIN);
            detachInterrupt(POWER_SWITCH_PIN);

            digitalWrite(LED_BUILTIN, HIGH); // turn the LED on

            if (volSwitch == 0) { // If switch is off, go back to sleep.
                display = 0;
                break;
            }

            ardOnTime = millis(); // Start timing power on time

            wakeup(); // Wakeup pi and Set next state based on inputs
        case lowPower: // Arduino on but pi off. Currently unused
            break;
        case dispOff: // Woken up, no display
            // If inputs tripped, turn display on
            if (ignition == 1 || playerIn[KEEP_ALIVE] == 1) {
                display_on();
                break;
            }
            // If display is off and timeout has passed, shutdown
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
            if ((shutdownTime + SHUTDOWN_REQ_TIMEOUT) > millis() ) {
                shutdown();
            }
            if (ignition == 1  || playerIn[KEEP_ALIVE] == 1) {
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
            if ((shutdownTime + SHUTDOWN_REQ_TIMEOUT) > millis() ) {
                shutdown();
            }
            if (volSwitch == 1 || playerIn[KEEP_ALIVE] == 1) {
                wakeup(); // Cancel shutdown 
            }
    }
}

// isr interrupt callbacks

void wakeup_no_display()
{
    // Handler for the wakeup interrupt with no display
    // retroPlayer.wake_on_disp();
    return;
}

void wakeup_manual()
{
    // Handler for the wakeup interrupt with display
    retroPlayer.display_on(true);
}



// ************************* MAIN PROGRAM *************************

SleepyPiClass sleepyPi;
RetroPlayer retroPlayer;
SerialComms retroComms(DATA_MEMBERS);

void setup() {
    Serial.begin(9600);

    // Set the initial Power to be off
    sleepyPi.enablePiPower(false);  
    sleepyPi.enableExtPower(false);

    // Setup serial data references

    SerialData<byte[NUM_DIGITAL]> serial_digital(digitalState, "dig");
    retroComms[0] = &serial_digital;
    SerialData<int[NUM_ANALOG]> serial_analog(analogState, "analog");
    retroComms[1] = &serial_analog;
    SerialData<byte> serial_display(&display, "disp");
    retroComms[2] = &serial_display;
    SerialData<byte> serial_off(&data1, "off");
    retroComms[3] = &serial_off;
    SerialData<byte> serial_mode(&mode, "mode");
    retroComms[4] = &serial_mode;
    SerialData<byte> serial_volSwitch(&volSwitch, "volSw");
    retroComms[5] = &serial_volSwitch;
    SerialData<byte> serial_handshake(&handshake, "hand");
    retroComms[6] = &serial_handshake;

    
    // Setup inputs. Off is high, on is low
    pinMode(inputs[0], INPUT_PULLUP);
    digitalWrite(inputs[0], HIGH);  // Pullup for on/off switch
    for (byte i=1; i< NUM_INPUTS; i++) { //Start at 1 to miss first
        pinMode(inputs[i], INPUT);
        digitalState[i] = digitalRead(inputs[i]);   // read the inputs
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
