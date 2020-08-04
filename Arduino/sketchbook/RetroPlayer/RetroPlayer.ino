#include "RetroPlayer.h"
#include "SerialComms.h"

const byte NUM_CHARS = 64; //Increase if required for more data.
// Serial buffer is 64 bytes but program should read faster than buffer

const byte DEBOUNCE = 10;  // input debouncer (ms)
 
constexpr byte POWER_SWITCH_PIN = 2; //int 0?
constexpr byte DOOR_PIN = 3; //int 1?
const unsigned int POWER_ON_TIMEOUT = 30 * 1000; // Seconds to millis
const unsigned int HANDSHAKE_TIMEOUT = 30 * 1000; // Seconds to millis
const unsigned int SHUTDOWN_REQ_TIMEOUT = 30 * 1000; // Seconds to millis
const unsigned int SHUTDOWN_TIMEOUT = 30 * 1000; // Seconds to millis

constexpr byte inputs[] = {2, 3, 5, 6, 11, 12};
constexpr byte outputs[] = {7, 10, 13};
constexpr byte analogues[][2] = {"A0", "A1", "A4", "A5"};
constexpr byte ANALOGUE_SINK = 9;
byte ANALOGUE_AVERAGES = 6; // Even no. so that jumps between 2 numbers are centralised

// Single values:
// errSer = Serial error (String)


constexpr byte NUM_DIGITAL = sizeof(inputs) / sizeof(inputs[0]);
constexpr byte NUM_OUTPUTS = sizeof(outputs) / sizeof(outputs[0]);
constexpr byte NUM_ANALOGS = sizeof(analogues) / sizeof(analogues[0]);

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

void wakeup_manual();
void wakeup_auto();

template <byte digitalIns, byte analogIns, byte digitalOuts>
RetroPlayer<digitalIns, analogIns, digitalOuts>::RetroPlayer(SleepyPiClass *sleepyPi, SerialComms *comms) : powerState{off}, handshakeState{none}, sleepyPi_{sleepyPi}, comms_{comms}, numDigIns{digitalIns}, numAnalIns{analogIns}, numDigOuts{digitalOuts}, digitalInStates{}, analogInStates{}, digitalOutStates{}
{

    // Setup serial data references
    comms_->addOutData(digitalInStates, NUM_DIGITAL, "dig");
    comms_->addOutData(analogInStates, NUM_ANALOGS, "analog");
    comms_->addOutData(&display, "disp");
    comms_->addOutData(&piOff, "off");
    comms_->addOutData(&mode, "mode");
    comms_->addOutData(&handshakeData, "hand");

    comms_->addInData(digitalOutStates, NUM_OUTPUTS, "out");
    comms_->addInData(&piAwake, "awake");
    comms_->addInData(&handshakeReceived, "hand");
    comms_->addInData(&keepAlive, "alive");
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
RetroPlayer<digitalIns, analogIns, digitalOuts>::~RetroPlayer()
{
    delete[] digitalInStates;
    delete[] analogInStates;
    delete[] digitalOutStates;
}

// ************************* SERIAL FUNCS *************************

template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::handshake() {
    comms_->read_serial_data();
    if (millis() < lastHandshakeTime) {
        lastHandshakeTime = millis(); // Timer has wrapped around
    }
    switch(handshakeState) {
        case none: // Send handshake
            if (piAwake == 1) {
                // Send pi all data
                handshakeData = 1; // Sending/sent
                comms_->sendData();
                lastHandshakeTime = millis();
                handshakeState = sent;
            }
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) < millis() ) {
                handshakeData = 3; // No Awake signal received.
                comms_->sendData(&handshakeData);
                lastHandshakeTime = millis();
            }
            break;
        case sent: // Sent, wait for receive
            if (handshakeReceived == 1) {
                handshakeData = 2; // Success
                comms_->sendData(&handshakeData);
                lastHandshakeTime = millis();
                handshakeState = success;
            }
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) < millis() ) {
                handshakeData = 4; // No Received signal received.
                comms_->sendData(&handshakeData);
                lastHandshakeTime = millis();
            }
            break;
        case success:
            // Handshake confirmed. Check time since last message
            if ( (lastHandshakeTime + HANDSHAKE_TIMEOUT) < millis() ) {
                handshakeReceived = 0;
                handshakeData = 0; // Require new handshake
                lastHandshakeTime = millis();
                handshakeState = none;
            }
            break;
    }
}

// ************************* HARDWARE IO *************************

template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::setup()
{
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
}

template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::power_switch(byte level) {
    volSwitch = !level; // Switch is inverted
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::door_light(byte level) {
    intLights = level;
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::ignition_func(byte level) {
    ignition = level;
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::boot_release_but(byte level) {
    return;
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::spare_in(byte level) {
    return;
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::air_horns(byte level) {
    return;
}

template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::check_dig_inputs() {
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

template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::check_analogues()
{
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
        int diff = newAnalogueState[i] - analogInStates[i];
        if (abs(diff) > thresholds[i]) {
            analogInStates[i] = newAnalogueState[i];
        }
    }
}

template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::set_outputs()
{
    for (byte i = 0; i < NUM_OUTPUTS; i++)
    {
        digitalWrite(outputs[i], digitalOutStates[i]);
    }
}

// **************** STATE FUNCTIONS AND MACHINE ****************

template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::display_on(bool wakeup = false) {
    display = 1;
    if (wakeup == true) { return; }
    comms_->sendData(&display);
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::wakeup() {
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
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::maintenence_mode() { // Cancel analog power setup so i2c can be used
    sleepyPi_->enableExtPower(false);
    pinMode(ANALOGUE_SINK, INPUT);
    mode = 1;
    comms_->sendData(&mode);
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::normal_mode() { // Setup analogues to read
    sleepyPi_->enableExtPower(true);
    pinMode(ANALOGUE_SINK, OUTPUT);
    digitalWrite(ANALOGUE_SINK, LOW);
    mode = 0;
    comms_->sendData(&mode);
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::shutdown_request(byte shutdownType = 2) {
    shutdownTime = millis();
    piOff = shutdownType; //Shutdown request code (2=auto, 3=manual)
    comms_->sendData(&piOff);
    if (shutdownType == 3) {
        powerState = shutdownTimeoutManual;
    } else {
        powerState = shutdownTimeoutAuto;
    }
}
template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::shutdown() {
    // Send shutdown signal
    piOff = 1;
    comms_->sendData(&piOff);
    shutdownTime = millis();
    powerState = shuttingDown; // Shutdown arduino
}

template <byte digitalIns, byte analogIns, byte digitalOuts>
void RetroPlayer<digitalIns, analogIns, digitalOuts>::power_control() {
    switch(powerState) { // State machine to control power up sequence
        case shuttingDown: // Shutting down
            //TODO Add timeout involving piAwake
            piPower = sleepyPi_->checkPiStatus(90, false);  // 90mA threshold. Don't Cut Power automatically
            if (piPower == false) {
                // TODO Clear serial buffer?
                sleepyPi_->enablePiPower(false);
                powerState = off;
            }
            if ( (shutdownTime + SHUTDOWN_TIMEOUT) < millis() ) {
                piOff = 4; // Shutdown not completed
                comms_->sendData(&piOff);
                shutdownTime = millis();
            }
            break;
        case off: // Low power state
            handshakeData = 7; // Temp DELETEME
            comms_->sendData(&handshakeData);
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
            volSwitch = !digitalRead(POWER_SWITCH_PIN); // Switch is inverted
            if (volSwitch == 0) { // If switch is off, go back to sleep.
                display = 0;
                break;
            }

            ardOnTime = millis(); // Record power on time
            lastHandshakeTime = millis();

            wakeup(); // Wakeup pi and Set next state based on inputs
            break;
        case lowPower: // Arduino on but pi off. Currently unused
            break;
        case dispOff: // Woken up, no display
            // If inputs tripped, turn display on
            if (ignition == 1 || keepAlive == 1) {
                display_on();
                break;
            }
            // If display is off and timeout has passed, shutdown
            if ((ardOnTime + POWER_ON_TIMEOUT) < millis() ) {
                shutdown_request(2); // Shutdown Pi
            }
            break;
        // *********** Regular auto on/off (Not using vol switch) ***********
        case dispOnAuto: // Woken up with ignition, display should be on
            if (ignition == 0 && intLights == 1) {
                shutdown_request(2); // Auto
            }
            if (volSwitch == 0) { 
                shutdown_request(3); // Manual
            }
            break;
        case shutdownTimeoutAuto: 
            if ((shutdownTime + SHUTDOWN_REQ_TIMEOUT) < millis() ) {
                shutdown();
            }
            if (ignition == 1  || keepAlive == 1) {
                wakeup(); //Cancel shutdown 
            }
            break;
        // *********** Manual on/off (Using vol switch) ***********
        case dispOnManual: // Woken up but ignition is off (Using vol switch)
            if (volSwitch == 0) {
                shutdown();
            }
            if (ignition == 1) {
                powerState = dispOnAuto;
            }
            break;
        case shutdownTimeoutManual: // Power switch turned off
            if ((shutdownTime + SHUTDOWN_REQ_TIMEOUT) < millis() ) {
                shutdown();
            }
            if (volSwitch == 1 || keepAlive == 1) {
                wakeup(); // Cancel shutdown 
            }
            break;
    }
}


// ************************* MAIN PROGRAM *************************

SleepyPiClass sleepyPi;
SerialComms retroComms(NUM_CHARS);
RetroPlayer<NUM_DIGITAL, NUM_ANALOGS, NUM_OUTPUTS> retroPlayer(&sleepyPi, &retroComms);

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

    retroPlayer.setup(); //Setup hardware pins

    // Stop 32khz clock output on interrupt line (Power switch)
    sleepyPi.rtcStop_32768_Clkout();
    sleepyPi.rtcClearInterrupts();
}

void loop() {
    retroPlayer.power_control(); // State machine to handle arduino + Pi power
    // Controllers check state during following functions
    retroPlayer.check_dig_inputs();
    retroPlayer.check_analogues();
    retroPlayer.set_outputs();
    retroPlayer.handshake();
}
