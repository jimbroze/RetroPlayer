#ifndef SERIALCOMMS_H
#define SERIALCOMMS_H

#include <ArduinoJson.h>

// Base class required for holding variants of derived classes
class SerialData_Base
{
public:
    virtual ~SerialData_Base(){};
    virtual void setData(JsonVariant dataIn);
    virtual const char *getKey() { return "N/A"; }
    virtual void send();
    virtual void sendAll(JsonObject fullDocObj);
};

// Templated derived class
template <class T>
class SerialData : public SerialData_Base
{
protected:
    T *dataStore;
    const char *m_key;

public:
    SerialData(T *dataIn, const char *key) : dataStore{dataIn}, m_key{key} {}
    T *getData() { return dataStore; }
    virtual void setData(JsonVariant dataIn);
    virtual const char *getKey() { return m_key; }
    virtual void send();
    virtual void sendAll(JsonObject fullDocObj);
};

// Specialization for arrays. Identical to above but still required.
template <class T, byte arrLength>
class SerialData<T[arrLength]> : public SerialData_Base
{
protected:
    T *dataStore;
    const char *m_key;
    byte arrSize;

public:
    SerialData(T *dataIn, const char *key) : dataStore{dataIn}, m_key{key}, arrSize{arrLength} {}
    T *getData() { return dataStore; }
    virtual void setData(JsonVariant dataIn);
    virtual const char *getKey() { return m_key; }
    virtual void send();
    virtual void sendAll(JsonObject fullDocObj);
};

class SerialComms
{
private:
    byte dataOutSize{0};
    byte dataInSize{0};
    SerialData_Base **commsOutData; // Array of pointers
    SerialData_Base **commsInData; // Array of pointers
    byte maxOutArrSize = 0;
    byte maxInArrSize = 0;
    byte numOutArrs = 0;
    byte numInArrs = 0;
    int numChars;
    void process_serial_data(char serialData[]);

public:
    SerialComms() = default;
    SerialComms(byte bufferChars);
    ~SerialComms() {}

    SerialData_Base *&operator[](byte index) { return commsOutData[index]; }
    template <class T>
    void addOutData(T *outData, const char *key);
    template <class T, byte arrLength>
    void addOutData(T (&outData)[arrLength], byte arrSize, const char *key);
    template <class T>
    void addInData(T *inData, const char *key);
    template <class T, byte arrLength>
    void addInData(T (&inData)[arrLength], byte arrSize, const char *key);

    void sendData();
    template <class T> // Templated function for checking
    void sendData(T data_p);
    template <class T, byte arrLength>
    void sendData(T (&data_p)[arrLength]);
    void read_serial_data();
};



// *********************** SerialComms.cpp ***********************


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
        // Serial.println(error.c_str());
        // Serial.flush();
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
    Serial.flush();
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
    Serial.flush();
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
    Serial.flush();
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

#endif