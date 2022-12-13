# RetroPlayer

RetroPlayer is a modern car radio built with a raspberry pi, Arduino, LCD display and retro radio face plate.
Built using Python on a Raspberry Pi and Arduino C++ on an Arduino. The two communicate over an asynchronous serial connection.

It features:
* Bluetooth
* LCD Display showing track information
* Automated on/off using car ignition
* Control and recognition of car lights, horn, boot release
* Volume, balance, fader, tone 
* Volume adjustment based on engine speed

## Components & Functions
### Raspberry Pi
* UI - Control of SSD1305 LCD display.
* Media player - Control and play media through the Raspberry Pi.
* Bluetooth - Communicate with phone or other media player.
* Communicate with with the Arduino to access physical inputs and outputs

### Arduino - C++
* Power control - Power on and shutdown the Raspberry PI based on a car's ignition. Also control power to audio equipment
* I/O - Interface with:
  * Analogue radio inputs (volume, balance, fader, tone)
  * Digital inputs (ignition, lights, boot release, horn, car speed)
  * Digital outputs (lights, horn, amplifier power)
* Communicate the analogue and dipital I/O with the Pi

## Notable Code Features
### Python - Raspberry Pi
* [Custom UI](https://github.com/jimbroze/RetroPlayer/blob/master/Python/PlayerDisplay.py) for LCD display
* [Custom multiplexer](https://github.com/jimbroze/RetroPlayer/blob/master/Python/PlayerIO.py#L112) for switch inputs using a reduced number of GPIO pins. (9 switch inputs from 5 GPIO pins - 2 out, 3 in)
* [Bluetooth integration](https://github.com/jimbroze/RetroPlayer/blob/master/Python/BlueHandler.py) using DBUS
* [State machine](https://github.com/jimbroze/RetroPlayer/blob/master/Python/RetroPlayer.py#L142) for controlling media player
* [Handshake communication](https://github.com/jimbroze/RetroPlayer/blob/master/Python/RetroPlayer.py#L281) between Raspberry Pi and Arduino

### C++ - Arduino
* [State machine](https://github.com/jimbroze/RetroPlayer/blob/master/Arduino/sketchbook/RetroPlayer/RetroPlayer.ino#L325) for Raspberry Pi power control
* [Handshake communication](https://github.com/jimbroze/RetroPlayer/blob/master/Arduino/sketchbook/RetroPlayer/RetroPlayer.ino#L62) between Arduino and Raspberry PI
