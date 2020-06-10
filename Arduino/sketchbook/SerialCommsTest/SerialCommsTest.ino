#include "SerialCommsTest.h"

const unsigned int SERIAL_OUT_CAPACITY {JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(3)};
const unsigned int SERIAL_IN_CAPACITY {JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(3)};
const unsigned int MAX_DATA_ARRAY {3};

SerialComms::SerialComms(byte chars) : dataOutSize{0}, dataInSize{0}, numChars{chars}
{
    commsOutData = new SerialData_Base *[1];
    commsInData = new SerialData_Base *[1];
}

template <class T>
void SerialComms::addOutData(T *outData, const char *key)
{
    if (dataOutSize > 0)
    {
        SerialData_Base **newOutData = new SerialData_Base *[dataOutSize+1];

        for (byte i {0}; i < (dataOutSize); i++)
        {
            newOutData[i] = commsOutData[i];
        }
        delete[] commsOutData;
    
        commsOutData = newOutData;
    }
    commsOutData[dataOutSize] = new SerialData<T>(outData, key);
    dataOutSize++;
}
template <class T, byte arrLength>
void SerialComms::addOutData(T (&outData)[arrLength], byte arrSize, const char *key)
{
    if (dataOutSize > 0)
    {
        SerialData_Base **newOutData = new SerialData_Base *[dataOutSize+1];

        for (byte i {0}; i < (dataOutSize); i++)
        {
            newOutData[i] = commsOutData[i];
        }
        delete[] commsOutData;
    
        commsOutData = newOutData;
    }
    commsOutData[dataOutSize] = new SerialData<T[arrLength]>(outData, key);
    dataOutSize++;
}


template <class T>
void SerialComms::addInData(T *inData, const char *key)
{
    if (dataInSize > 0)
    {
        SerialData_Base **newInData = new SerialData_Base *[dataInSize+1];

        for (byte i {0}; i < (dataInSize); i++)
        {
            newInData[i] = commsInData[i];
        }
        delete[] commsInData;
    
        commsInData = newInData;
    }
    commsInData[dataInSize] = new SerialData<T>(inData, key);
    dataInSize++;
}
template <class T, byte arrLength>
void SerialComms::addInData(T (&inData)[arrLength], byte arrSize, const char *key)
{
    if (dataInSize > 0)
    {
        SerialData_Base **newInData = new SerialData_Base *[dataInSize+1];

        for (byte i {0}; i < (dataInSize); i++)
        {
            newInData[i] = commsInData[i];
        }
        delete[] commsInData;
    
        commsInData = newInData;
    }
    commsInData[dataInSize] = new SerialData<T[arrLength]>(inData, key);
    dataInSize++;
}

void SerialComms::read_serial_data() {
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

void SerialComms::process_serial_data(char serialData[]) {
    StaticJsonDocument<SERIAL_IN_CAPACITY> doc;
    DeserializationError error = deserializeJson(doc, serialData);
    if (error) {
        // send_serial("errSer", error.c_str());
        Serial.println(error.c_str());
        return;
    }
    // Get a reference to the root object
    JsonObject obj = doc.as<JsonObject>();
    // Loop through all the key-value pairs in obj

    for(JsonPair p: obj) {
        for(byte i=0; i < dataInSize; i++) {
            if(strcmp(p.key().c_str(), commsInData[i]->getKey()) == 0) {
                JsonVariant dataValue  = p.value();
                commsInData[i]->setData(dataValue);
                break;
            }
            // TODO Look for 0?
        }
    }
}

// Templated function for checking
template <class T>
void SerialComms::sendData(T data_p)
{
    for (byte i = 0; i < dataOutSize; i++)
    {
        //Store type to be checked in dynamic typedef
        auto data = *data_p;
        using inputType = decltype(data);
        // Use dynamic typedef to downcast Base pointer
        SerialData<inputType> *p = static_cast<SerialData<inputType> *>(commsOutData[i]);
        if (p)
        {
            if (p->getData() == data_p)
            {
                p->send();
            }
        }
    }
}

template <class T, byte arrLength>
void SerialComms::sendData(T (&data_p)[arrLength])
{
    for (byte i = 0; i < dataOutSize; i++)
    {
        // Store type to be checked in dynamic typedef.
        // Array is dereferenced by getting singular member type and creating a temporary array.
        auto data = *data_p;
        using singularType = decltype(data);
        singularType arr[arrLength];
        using arrayType = decltype(arr);

        // Use dynamic typedef to downcast Base pointer
        SerialData<arrayType> *p = static_cast<SerialData<arrayType> *>(commsOutData[i]);
        // Check if dynamic cast worked i.e same data type.
        if (p)
        {
            // See if input pointer matches array member pointer
            if (p->getData() == data_p)
            {
                p->send();
            }
        }
    }
}

void SerialComms::sendData() // Dict
{
    StaticJsonDocument<SERIAL_OUT_CAPACITY> fullDoc;
    JsonObject fullDocObj = fullDoc.to<JsonObject>();

    for (byte i{0}; i < dataOutSize; i++)
    {
        commsOutData[i]->sendAll(fullDocObj);
    }

    // Produce a minified JSON document
    serializeJson(fullDoc, Serial);
    Serial.println();
}

template <class T>
void SerialData<T>::setData(JsonVariant dataIn)
{
    *dataStore = dataIn.as<T>();
}

template <class T, byte arrLength>
void SerialData<T[arrLength]>::setData(JsonVariant dataIn)
{
    auto dataArray = dataIn.as<JsonArray>();
    byte i {0};
    for (JsonVariant dataMember : dataArray)
    {
        dataStore[i] = dataMember.as<T>();
        i++;
    }
    
}

template <class T>
void SerialData<T>::send()
{
    constexpr int capacity = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<capacity> doc;
    JsonObject docObj = doc.to<JsonObject>();

    docObj[m_key] = *dataStore;

    // Send JSON to serial
    serializeJson(doc, Serial);
    Serial.println();
}    

template <class T, byte arrLength>
void SerialData<T[arrLength]>::send()
{
    constexpr int capacity = JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(MAX_DATA_ARRAY);
    StaticJsonDocument<capacity> doc;
    JsonObject docObj = doc.to<JsonObject>();
    for (byte i {0}; i < arrLength; i++)
    {
        docObj[m_key][i] = dataStore[i];
    }

    serializeJson(doc, Serial);
    Serial.println();
    
}

template <class T>
void SerialData<T>::sendAll(JsonObject fullDocObj)
{
    fullDocObj[m_key] = *dataStore;
}

template <class T, byte arrLength>
void SerialData<T[arrLength]>::sendAll(JsonObject fullDocObj)
{
    for (byte i {0}; i < arrLength; i++)
    {
        fullDocObj[m_key][i] = dataStore[i];
    }
}

long data1 = 1;
int data2 = 2;
int data3[] = {1, 2, 3};

long data4 = 1;
int data5 = 2;
int data6[] = {1, 2, 3};

SerialComms testComms(64);

void setup()
{
    Serial.begin(9600);

    pinMode(LED_BUILTIN, OUTPUT);

    testComms.addOutData(&data1, "key1");
    testComms.addOutData(&data2, "key2");
    testComms.addOutData(data3, 3, "key3");


    testComms.addInData(&data4, "key4");
    testComms.addInData(&data5, "key5");
    testComms.addInData(data6, 3, "key6");
}

void loop()
{
    // testComms.sendData(&data1);
    // delay(1000);
    // testComms.sendData(&data2);
    // delay(1000);
    // testComms.sendData(data3);
    // delay(1000);
    // testComms.sendData();
    // delay(8000);

    // Receive data and send it back.
    testComms.read_serial_data();

    if (data6[1] == 7)
    {
        digitalWrite(LED_BUILTIN, HIGH); 
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW); 
    }
    delay(1000);
}
