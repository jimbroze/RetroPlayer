#ifndef RETROPLAYER_H
#define RETROPLAYER_H

class RetroPlayer {
private:
    SleepyPiClass *sleepyPi_;
    // byte myState;
    long lastHandshakeTime;
    long shutdownTime;
    long ardOnTime;
    bool piPower;

    boolean digitalState[];
    int analogState[];
    byte display;
    byte off; //FIXME
    byte mode;
    boolean volSwitch; //Is needed?
    byte handshake;

    byte ignition;
    byte intLights;

    void read_serial_data();
    void process_serial_data(char serialData[]);

    void power_switch(byte level);
    void door_light(byte level);
    void ignition_func(byte level);
    void boot_release_but(byte level);
    void spare_in(byte level);
    void air_horns(byte level);

    enum PlayerState {
        shuttingDown, off, lowPower, dispOff, dispOnAuto, dispOnManual, shutdownTimeoutAuto, shutdownTimeoutManual
    } myState;
    enum Handshake {
        none, sent, success, failed
    } handshakeState;

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
    RetroPlayer() {
        myState = off;
        handshakeState = none;
    }
};

#endif