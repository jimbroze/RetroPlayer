
#include <ArduinoJson.h>

#define DEBOUNCE 10  // button debouncer, how many ms to debounce, 5+ ms is usually plenty
 
// Define standard digital inputs
byte inputs[] = {4, 5, 6, 7, 8, 9};
#define NUMINPUTS sizeof(inputs)
byte inputState[NUMINPUTS];

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

    if (Serial.available() !> 0) {
        return;
    }
    // read the incoming byte:
    int inData = Serial.read();

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.println(error.c_str()); 
        return;
    }
    // Get a reference to the root object
    JsonObject obj = doc.as<JsonObject>();
    // Loop through all the key-value pairs in obj
    for(auto p: obj) {
        p.key() // is a JsonString
        p.value() // is a JsonVariant
    }
}

void check_inputs() {
    static byte newInputState[NUMINPUTS];
    static byte debounce[NUMINPUTS];
    static long lastTime[NUMINPUTS];
    byte i;
  
    for (i = 0; i < NUMINPUTS; i++) {
        changed[i] = 0
        if (millis() < lasttime[i]) {
            lasttime[i] = millis(); // Timer has wrapped around
        }
 
        if ((lasttime[i] + DEBOUNCE) > millis()) {
            continue; // not enough time has passed to debounce
        }
        // DEBOUNCE time passed or not started

        newInputState[i] = digitalRead(inputs[i]);   // read the input

        if (newInputState[i] == inputState[i]) {
            if (debounce[i] == 1)  {
                debounce[i] = 0 // Debounce failed, reset
            }
            continue; // Debounced failed or not started
        }
        if (debounce[i] == 1) {
            debounce[i] = 0 // Debouncing finished, stop debounce
            inputState[i] = newInputState[i];
            inputCallbacks [i] (inputState[i]);
        } else {
            debounce[i] = 1 // Debounce not started, start debounce
        }
        // Input changed. Reset timer
        lasttime[i] = millis();
    }
}

void setup() {
    Serial.begin(1152000);

    byte i;
    
    // Make input & enable pull-up resistors on switch pins
    for (i=0; i< NUMINPUTS; i++) {
        pinMode(buttons[i], INPUT);
        inputState[i] = digitalRead(inputs[i]);   // read the inputs
    }
}

void loop() {
    read_serial_data()
    byte* changedInputs = check_inputs()
}