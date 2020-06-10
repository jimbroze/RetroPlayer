#ifndef SERIALCOMMS_H
#define SERIALCOMMS_H

#include <ArduinoJson.h>

// Base class required for holding variants of derived classes
class SerialData_Base
{
public:
    virtual ~SerialData_Base(){};
    virtual SerialData_Base *getThis() { return this; }
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
    virtual SerialData<T> *getThis() { return this; }
    T *getData() { return dataStore; }
    const char *getKey() { return m_key; }
    virtual void send();
    virtual void sendAll(JsonObject fullDocObj);
};

// Specialization for arrays. Identical to above but still required.
template <class T, int arrLength>
class SerialData<T[arrLength]> : public SerialData_Base
{
protected:
    T *dataStore;
    const char *m_key;

public:
    SerialData(T *dataIn, const char *key) : dataStore{dataIn}, m_key{key} {}
    virtual SerialData<T[arrLength]> *getThis() { return this; }
    T *getData() { return dataStore; }
    const char *getKey() { return m_key; }
    virtual void send();
    virtual void sendAll(JsonObject fullDocObj);
};

class SerialComms
{
private:
    const int dataSize;
    SerialData_Base **commsData; // Array of pointers

public:
    SerialComms() = default;
    SerialComms(int members);
    ~SerialComms() { delete commsData; }

    SerialData_Base *&operator[](int index) { return commsData[index]; }

    void sendData();
    template <class T> // Templated function for checking
    void sendData(T data_p);
    template <class T, int arrLength>
    void sendData(T (&data_p)[arrLength]);
};

#endif