#include "SerialComms.h"

SerialComms::SerialComms(byte bufferChars) : numChars{bufferChars}
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
template <class T>
void SerialComms::addOutData(T *outData, byte arrSize, const char *key)
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
    commsOutData[dataOutSize] = new SerialData<T[arrSize]>(outData, key);
    dataOutSize++;

    if (arrSize > maxOutArrSize)
    {
        maxOutArrSize = arrSize;
    }
    numOutArrs++;
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
template <class T>
void SerialComms::addInData(T *inData, byte arrSize, const char *key)
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
    commsInData[dataInSize] = new SerialData<T[arrSize]>(inData, key);
    dataInSize++;

    if (arrSize > maxInArrSize)
    {
        maxInArrSize = arrSize;
    }
    numInArrs++;
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
    int capacity = JSON_OBJECT_SIZE(dataInSize) + numInArrs * JSON_ARRAY_SIZE(maxInArrSize);
    DynamicJsonDocument doc(capacity);
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
    int capacity = JSON_OBJECT_SIZE(dataOutSize) + numOutArrs * JSON_ARRAY_SIZE(maxOutArrSize);
    DynamicJsonDocument fullDoc(capacity);
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
    int capacity = JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(arrSize);
    DynamicJsonDocument doc(capacity);
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