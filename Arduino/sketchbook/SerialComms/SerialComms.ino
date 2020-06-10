#include "SerialComms.h"

constexpr int DATA_MEMBERS {3};
constexpr int MAX_DATA_ARRAY {3};
constexpr int CAPACITY{JSON_OBJECT_SIZE(DATA_MEMBERS) + JSON_ARRAY_SIZE(3)};

SerialComms::SerialComms(int members) : dataSize{members}
{
    if (members > 0)
        commsData = new SerialData_Base *[members];
}

// Templated function for checking
template <class T>
void SerialComms::sendData(T data_p)
{
    for (int i = 0; i < dataSize; i++)
    {
        //Store type to be checked in dynamic typedef
        auto data = *data_p;
        using inputType = decltype(data);
        // Use dynamic typedef to downcast Base pointer
        SerialData<inputType> *p = static_cast<SerialData<inputType> *>(commsData[i]);
        if (p)
        {
            if (p->getData() == data_p)
            {
                p->send();
            }
        }
    }
}

template <class T, int arrLength>
void SerialComms::sendData(T (&data_p)[arrLength])
{
    for (int i = 0; i < dataSize; i++)
    {
        // Store type to be checked in dynamic typedef.
        // Array is dereferenced by getting singular member type and creating a temporary array.
        auto data = *data_p;
        using singularType = decltype(data);
        singularType arr[arrLength];
        using arrayType = decltype(arr);

        // Use dynamic typedef to downcast Base pointer
        SerialData<arrayType> *p = static_cast<SerialData<arrayType> *>(commsData[i]);
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
    StaticJsonDocument<CAPACITY> fullDoc;
    JsonObject fullDocObj = fullDoc.to<JsonObject>();

    for (int i{0}; i < dataSize; i++)
    {
        commsData[i]->sendAll(fullDocObj);
    }

    // Produce a minified JSON document
    serializeJson(fullDoc, Serial);
    Serial.println();
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

template <class T, int arrLength>
void SerialData<T[arrLength]>::send()
{
    constexpr int capacity = JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(MAX_DATA_ARRAY);
    StaticJsonDocument<capacity> doc;
    JsonObject docObj = doc.to<JsonObject>();
    for (int i {0}; i < arrLength; i++)
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

template <class T, int arrLength>
void SerialData<T[arrLength]>::sendAll(JsonObject fullDocObj)
{
    for (int i {0}; i < arrLength; i++)
    {
        fullDocObj[m_key][i] = dataStore[i];
    }
}



long data1 = 1;
SerialData<long> serial1(&data1, "key1");

int data2 = 2;
SerialData<int> serial2(&data2, "key2");

int data3[] = {1, 2, 3};
SerialData<int[3]> serial3(data3, "key3");

SerialComms testComms(DATA_MEMBERS);

void setup()
{
    Serial.begin(9600);

    testComms[0] = &serial1;
    testComms[1] = &serial2;
    testComms[2] = &serial3;
}

void loop()
{
    testComms.sendData(&data1);
    delay(1000);
    testComms.sendData(&data2);
    delay(1000);
    testComms.sendData(data3);
    delay(1000);
    testComms.sendData();
    delay(8000);
}
