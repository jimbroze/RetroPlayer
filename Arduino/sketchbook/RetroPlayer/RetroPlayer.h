#ifndef RETROPLAYER_H
#define RETROPLAYER_H

#include "SerialComms.h"
#include "LowPower.h"
#include "PCF8523.h"
#include "SleepyPi2.h"
#include <ArduinoJson.h>

template <byte digitalIns, byte analogIns, byte digitalOuts>
class RetroPlayer {
private:
    SleepyPiClass *sleepyPi_;
    SerialComms *comms_;
    byte powerState;
    long lastHandshakeTime;
    long shutdownTime;
    long ardOnTime;
    bool piPower;
    byte numDigIns;
    byte numAnalIns;
    byte numDigOuts;

    boolean digitalInStates[digitalIns];
    int analogInStates[analogIns];
    
    byte display;
    byte piOff;
    byte mode;
   
    byte handshakeData;

    boolean digitalOutStates[digitalOuts];
    byte piAwake;
    byte handshakeReceived;
    byte keepAlive;

    byte ignition;
    byte intLights;
    boolean volSwitch;

    enum PlayerState {
        shuttingDown, off, lowPower, dispOff, dispOnAuto, dispOnManual, shutdownTimeoutAuto, shutdownTimeoutManual
    } myState;
    enum Handshake {
        none, sent, success, failed
    } handshakeState;

    void power_switch(byte);
    void door_light(byte);
    void ignition_func(byte);
    void boot_release_but(byte);
    void spare_in(byte);
    void air_horns(byte);

    typedef void (RetroPlayer::*ioFunction)(byte);
    // array of function pointers
    ioFunction inputCallbacks[6] = {
        &RetroPlayer::power_switch,
        &RetroPlayer::door_light,
        &RetroPlayer::ignition_func,
        &RetroPlayer::boot_release_but,
        &RetroPlayer::spare_in,
        &RetroPlayer::air_horns
    };

public:
    RetroPlayer(SleepyPiClass *sleepyPi, SerialComms *comms);
    ~RetroPlayer();
    void setup();
    void power_control();
    void handshake();
    void display_on(bool);
    void wakeup();
    void maintenence_mode();
    void normal_mode();
    void shutdown_request(byte);
    void shutdown();

    void check_dig_inputs();
    void check_analogues();
    void set_outputs();
};

#endif