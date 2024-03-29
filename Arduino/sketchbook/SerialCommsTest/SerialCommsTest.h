#ifndef SERIALCOMMSTEST_H
#define SERIALCOMMSTEST_H

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

public:
    SerialData(T *dataIn, const char *key) : dataStore{dataIn}, m_key{key} {}
    T *getData() { return dataStore; }
    virtual void setData(JsonVariant dataIn);
    virtual const char *getKey() { return m_key; }
    virtual void send();
    virtual void sendAll(JsonObject fullDocObj);
};

class SerialComms
{
private:
    byte dataOutSize;
    byte dataInSize;
    SerialData_Base **commsOutData; // Array of pointers
    SerialData_Base **commsInData; // Array of pointers
    int numChars;
    void process_serial_data(char serialData[]);

public:
    SerialComms() = default;
    SerialComms(byte bufferChars);
    ~SerialComms() { delete commsOutData; delete commsInData; }

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

#endif