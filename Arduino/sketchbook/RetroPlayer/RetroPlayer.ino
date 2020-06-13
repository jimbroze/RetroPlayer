#include "RetroPlayer.h"
#include "SerialComms.h"

constexpr byte NUM_DIGITAL {6};
constexpr byte NUM_OUTPUTS {3};
constexpr byte NUM_ANALOGS {4};

// If size 3 object includes size 2 array
const byte NUM_CHARS = 64; //Increase if required for more data.
// Serial buffer is 64 bytes but program should read faster than buffer



#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember)) //FIXME Is this needed?

void wakeup_manual();
void wakeup_auto();

const byte DEBOUNCE = 10;  // input debouncer (ms)
 
const byte POWER_SWITCH_PIN = 2;	// Pin B0 - Arduino 8
const byte DOOR_PIN = 3;	// Pin B0 - Arduino 8
const unsigned int POWER_ON_TIMEOUT = 30 * 1000; // Seconds to millis
const unsigned int HANDSHAKE_TIMEOUT = 30 * 1000; // Seconds
const unsigned int SHUTDOWN_REQ_TIMEOUT = 30 * 1000; // Seconds
const unsigned int SHUTDOWN_TIMEOUT = 30 * 1000; // Seconds

byte inputs[NUM_DIGITAL] = {2, 3, 5, 6, 11, 12};
byte outputs[NUM_OUTPUTS] = {7, 10, 13};
byte analogues[NUM_ANALOGS][2] = {"A0", "A1", "A4", "A5"};
byte ANALOGUE_SINK = 9;
byte ANALOGUE_AVERAGES = 6; // Even no. so that jumps between 2 numbers are centralised

byte digitalInStates[NUM_DIGITAL];
byte outputState[NUM_OUTPUTS];
byte analogueState[NUM_ANALOGS];

// Single values:
// errSer = Serial error (String)


RetroPlayer::RetroPlayer(SleepyPiClass *sleepyPi, SerialComms *comms, byte digitalIns, byte analogIns, byte digitalOuts) : myState{off}, handshakeState{none}, sleepyPi_{sleepyPi}, comms_{comms}, numDigIns{digitalIns}, numAnalIns{analogIns}, numDigOuts{digitalOuts}
{
    digitalInStates = { new boolean[numDigIns] };
    analogInStates = { new int[numAnalIns] };
    digitalOutStates = { new boolean[numDigOuts] };

    // Setup serial data references
    comms_->addOutData(digitalInStates, NUM_DIGITAL, "dig");
    comms_->addOutData(analogInStates, NUM_ANALOGS, "analog");
    comms_->addOutData(&display, "disp");
    comms_->addOutData(&piOff, "off");
    comms_->addOutData(&mode, "mode");
    comms_->addOutData(&volSwitch, "volSw");
    comms_->addOutData(&handshakeData, "hand");

    comms_->addInData(digitalOutStates, NUM_OUTPUTS, "out");
    comms_->addInData(&piAwake, "awake");
    comms_->addInData(&handshakeReceived, "hand");
    comms_->addInData(&keepAlive, "alive");
}
RetroPlayer::~RetroPlayer()
{
    delete[] digitalInStates;
    delete[] analogInStates;
    delete[] digitalOutStates;
}

// ************************* SERIAL FUNCS *************************

void RetroPlayer::handshake() {
    comms_->read_serial_data();
    switch(handshakeState) {
        case none: // Send handshake
            if (piAwake == 1) {
                // Send pi all data
                handshakeData = 1; // Sending/sent
                comms_->sendData();
                lastHandshakeTime = millis();
                handshakeState = sent;
            }
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                handshakeData = 3; // No Awake signal received.
                comms_->sendData(&handshakeData);
                lastHandshakeTime = millis();
            }
        case sent: // Sent, wait for receive
            if (handshakeReceived == 1) {
                handshakeData = 2; // Success
                comms_->sendData(&handshakeData);
                lastHandshakeTime = millis();
                handshakeState = success;
            }
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                handshakeData = 4; // No Received signal received.
                comms_->sendData(&handshakeData);
                lastHandshakeTime = millis();
            }
        case success:
            // Handshake confirmed. Check time since last message
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) > millis() ) {
                handshakeReceived = 0;
                handshakeData = 0; // Require new handshake
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
    static byte newInputState[NUM_DIGITAL];
    static byte debounce[NUM_DIGITAL];
    static long lastTime[NUM_DIGITAL];
  
    for (byte i = 0; i < NUM_DIGITAL; i++) {
        if (millis() < lastTime[i]) {
            lastTime[i] = millis(); // Timer has wrapped around
        }
 
        if ((lastTime[i] + DEBOUNCE) > millis()) {
            continue; // not enough time has passed to debounce
        }
        // DEBOUNCE time passed or not started

        newInputState[i] = digitalRead(inputs[i]);   // read the input

        if (newInputState[i] == digitalInStates[i]) {
            if (debounce[i] == 1)  {
                debounce[i] = 0; // Debounce failed, reset
            }
            continue; // Debounced failed or not started
        }
        if (debounce[i] == 1) {
            debounce[i] = 0; // Debouncing finished, stop debounce
            digitalInStates[i] = newInputState[i];
            CALL_MEMBER_FN(*this, inputCallbacks[i]) (digitalInStates[i]); // Use #define macro to call input functions
            comms_->sendData(digitalInStates); // Send inputs when one changes
        } else {
            debounce[i] = 1; // Debounce not started, start debounce
        }
        // Input changed. Reset timer
        lastTime[i] = millis();
    }
}

void RetroPlayer::check_analogues() {
    int throwaway;
    int newAnalogueState[NUM_ANALOGS];
    int thresholds[NUM_ANALOGS];
    // static byte debounce;
    for (byte i=0; i< NUM_ANALOGS; i++) {
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
    comms_->sendData(&display);
}
void RetroPlayer::wakeup() {
    sleepyPi_->enablePiPower(true);
    if (display == 1) {
        if (ignition == 1) {
            powerState = dispOnAuto;
        }
        else
        {
            powerState = dispOnManual;
        }
    }
    else
    {
        powerState = dispOff;
    }
}
void RetroPlayer::maintenence_mode() { // Cancel analog power setup so i2c can be used
    sleepyPi_->enableExtPower(false);
    pinMode(ANALOGUE_SINK, INPUT);
    mode = 1;
    comms_->sendData(&mode);
}
void RetroPlayer::normal_mode() { // Setup analogues to read
    sleepyPi_->enableExtPower(true);
    pinMode(ANALOGUE_SINK, OUTPUT);
    digitalWrite(ANALOGUE_SINK, LOW);
    mode = 0;
    comms_->sendData(&display);
}
void RetroPlayer::shutdown_request(byte shutdownType = 2) {
    shutdownTime = millis();
    piOff = shutdownType; //Shutdown request code (2=auto, 3=manual)
    comms_->sendData(&piOff);
    if (shutdownType == 3) {
        powerState = shutdownTimeoutManual;
    } else {
        powerState = shutdownTimeoutAuto;
    }
}
void RetroPlayer::shutdown() {
    // Send shutdown signal
    piOff = 1;
    comms_->sendData(&piOff);
    shutdownTime = millis();
    powerState = shuttingDown; // Shutdown arduino
}


void RetroPlayer::power_control() {
    switch(powerState) { // State machine to control power up sequence
        case shuttingDown: // Shutting down
            //TODO Add timeout involving piAwake
            piPower = sleepyPi_->checkPiStatus(false);  // Don't Cut Power automatically
            if (piPower == false) {
                // TODO Clear serial buffer?
                sleepyPi_->enablePiPower(false);
                powerState = off;
            }
            if ( (shutdownTime + SHUTDOWN_TIMEOUT) > millis() ) {
                piOff = 4; // Shutdown not completed
                comms_->sendData(&piOff);
            }
        case off: // Low power state
            // Attach WAKEUP_PIN to wakeup ATMega
            attachInterrupt(digitalPinToInterrupt(DOOR_PIN), wakeup_auto, FALLING);
            attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), wakeup_manual, FALLING);

            // Enter power down state with ADC and BOD module disabled.
            // Wake up when wake up pins are high.
            sleepyPi_->powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
            
            // ###################### WOKEN UP ######################
            // Disable external pin interrupts.
            detachInterrupt(digitalPinToInterrupt(DOOR_PIN));
            detachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN));
            
            delay(DEBOUNCE); // Basic blocking debounce
            if (volSwitch == 0) { // If switch is off, go back to sleep.
                display = 0;
                break;
            }

            ardOnTime = millis(); // Record power on time

            wakeup(); // Wakeup pi and Set next state based on inputs
        case lowPower: // Arduino on but pi off. Currently unused
            break;
        case dispOff: // Woken up, no display
            // If inputs tripped, turn display on
            if (ignition == 1 || keepAlive == 1) {
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
            if (ignition == 1  || keepAlive == 1) {
                wakeup(); //Cancel shutdown 
            }
        // *********** Manual on/off (Using vol switch) ***********
        case dispOnManual: // Woken up but ignition is off (Using vol switch)
            if (volSwitch == 0) {
                shutdown_request();
            }
            if (ignition == 1) {
                powerState = dispOnAuto;
            }
        case shutdownTimeoutManual: // Power switch turned off
            if ((shutdownTime + SHUTDOWN_REQ_TIMEOUT) > millis() ) {
                shutdown();
            }
            if (volSwitch == 1 || keepAlive == 1) {
                wakeup(); // Cancel shutdown 
            }
    }
}


// ************************* MAIN PROGRAM *************************

SleepyPiClass sleepyPi;
SerialComms retroComms(NUM_CHARS);
RetroPlayer retroPlayer(&sleepyPi, &retroComms, NUM_DIGITAL, NUM_ANALOGS, NUM_OUTPUTS);

// isr interrupt callbacks
void wakeup_auto()
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

void setup() {
    Serial.begin(9600);

    // Set the initial Power to be off
    sleepyPi.enablePiPower(false);  
    sleepyPi.enableExtPower(false);
    
    // Setup inputs. Off is high, on is low
    pinMode(inputs[0], INPUT_PULLUP);
    for (byte i=1; i< NUM_DIGITAL; i++) { //Start at 1 to miss first
        pinMode(inputs[i], INPUT);
        digitalInStates[i] = digitalRead(inputs[i]);   // read the inputs
    }

    // Setup outputs
    for (byte i=0; i< NUM_OUTPUTS; i++) {
        pinMode(outputs[i], OUTPUT);
        digitalWrite(outputs[i], LOW);  // Set to off
    }

    // Stop 32khz clock output on interrupt line (Power switch)
    sleepyPi.rtcStop_32768_Clkout();
    sleepyPi.rtcClearInterrupts();
}

void loop() {
    retroPlayer.power_control(); // State machine to handle arduino + Pi power
    // Controllers check state during following functions
    retroPlayer.check_dig_inputs();
    retroPlayer.check_analogues();
    retroPlayer.handshake();
}
