#ifndef RETROPLAYER_H
#define RETROPLAYER_H

#include "SerialComms.h"
#include "LowPower.h"
#include "PCF8523.h"
#include "SleepyPi2.h"
#include <ArduinoJson.h>

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

    boolean *digitalInStates; // Array stored as pointer
    int *analogInStates; // Array stored as pointer
    
    byte display;
    byte piOff;
    byte mode;
    boolean volSwitch; //Is needed?
    byte handshakeData;

    boolean *digitalOutStates; // Array stored as pointer
    byte piAwake;
    byte handshakeReceived;
    byte keepAlive;

    byte ignition;
    byte intLights; // FIXME Needed?

    enum PlayerState {
        shuttingDown, off, lowPower, dispOff, dispOnAuto, dispOnManual, shutdownTimeoutAuto, shutdownTimeoutManual
    } myState;
    enum Handshake {
        none, sent, success, failed
    } handshakeState;

    void power_switch(byte level);
    void door_light(byte level);
    void ignition_func(byte level);
    void boot_release_but(byte level);
    void spare_in(byte level);
    void air_horns(byte level);

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
    RetroPlayer(SleepyPiClass *sleepyPi, SerialComms *comms, byte digitalIns, byte analogIns, byte digitalOuts);
    ~RetroPlayer();
    void power_control();
    void handshake();
    void wake_on_disp();
    void wake_no_disp();
    void display_on(bool);
    void wakeup();
    void maintenence_mode();
    void normal_mode();
    void shutdown_request(byte);
    void shutdown();

    void check_dig_inputs();
    void check_analogues();

    // void wakeup_manual();
    // void wakeup_no_display();
    // RetroPlayer() : myState{off}, handshakeState{none} {}
};

#endif