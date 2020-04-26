#!/usr/bin/env python3

"""
A bluetooth media player with IO and display.

Copyright (c) 2015, Douglas Otwell
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

# Dependencies:
# python3-gi

import time
import dbus
import dbus.service
import dbus.mainloop.glib

import logging
import traceback

import asyncio
import asyncio_glib

from BlueHandler import BlueHandler
from playerio import PlayerIO, MultiplexInput, SleepyPi

# Multiplexer pins
multiInPins = [16, 19, 20]
multiOutPins = [12, 13]

SERVICE_NAME = "org.bluez"
AGENT_IFACE = SERVICE_NAME + ".Agent1"
ADAPTER_IFACE = SERVICE_NAME + ".Adapter1"
DEVICE_IFACE = SERVICE_NAME + ".Device1"
PLAYER_IFACE = SERVICE_NAME + ".MediaPlayer1"
TRANSPORT_IFACE = SERVICE_NAME + ".MediaTransport1"

# LOG_LEVEL = logging.INFO
LOG_LEVEL = logging.DEBUG
LOG_FILE = None
LOG_FORMAT = "%(asctime)s %(levelname)s %(message)s"


class MediaPlayer(dbus.service.Object):
    """a bluetooth mediaplayer using GPIO of host raspberry pi and connected Arduino."""

    mainLoop = None
    bus = None
    blueHandler = None
    playerio = None
    inPins = []
    outPins = []
    multiplexers = []

    lcd = None
    adapter = None
    device = None
    deviceAlias = None
    player = None
    transport = None
    connected = None
    state = None
    status = None
    discoverable = None
    track = None

    def __init__(self, inPins, outPins):
        self.inPins = inPins
        self.outPins = outPins

        # Allows asyncio to use dbus event loop
        asyncio.set_event_loop_policy(asyncio_glib.GLibEventLoopPolicy())
        self.mainLoop = asyncio.get_event_loop()
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        bus = dbus.SystemBus()

        self.blueHandler = BlueHandler(bus, self, "NoInputNoOutput")
        self.playerio = PlayerIO(self, inPins, ["hi"], outPins, [1])
        self.arduino = SleepyPi()

    def addMultiplexer(self, multiInPins, multiOutPins, multiFuncs):
        self.multiplexers.append(
            MultiplexInput(self.playerio, multiInPins, multiOutPins, multiFuncs)
        )

    def get_loop(self):
        return self.mainLoop

    def start(self):
        """Start the player by beginning GPIO and the gobject/asyncio mainloop()"""
        self.playerio.setup()
        for multiplexer in self.multiplexers:
            multiplexer.setup()
        self.mainLoop.create_task(self.arduino.setup())
        self.mainLoop.run_forever()

    def playerHandler(self, interface, changed, invalidated, path):
        """Handle relevant property change signals"""

    def updateDisplay(self):
        """Display the current status of the device on the LCD"""
        logging.debug(
            f"Updating display for connected: [{self.connected}]; "
            f"state: [{self.state}]; "
            f"status: [{self.status}]; "
            f"discoverable [{self.discoverable}]"
        )
        if self.discoverable:
            self.wake()
            self.showDiscoverable()
        else:
            if self.connected:
                if self.state == "idle":
                    self.sleep()
                else:
                    self.wake()
                    if self.status == "paused":
                        self.showPaused()
                    else:
                        self.showTrack()
            else:
                self.sleep()

    def showDevice(self):
        """Display the device connection info on the LCD"""
        self.lcd.clear()
        self.lcd.writeLn("Connected to:", 0)
        self.lcd.writeLn(self.deviceAlias, 1)
        time.sleep(2)

    def showTrack(self):
        """Display track info on the LCD"""
        lines = []
        if "Artist" in self.track:
            lines.append(self.track["Artist"])
            if self.track["Title"]:
                lines.append(self.track["Title"])
        elif "Title" in self.track:
            lines = self.lcd.wrap(self.track["Title"])

        self.lcd.clear()
        for i, line in enumerate(lines):
            if i >= self.lcd.numlines:
                break
            self.lcd.writeLn(lines[i], i)

    def band_but(self):
        """ """

    def seek_but(self, direction):
        """ """

    def num_but(self, number):
        """ """

    def shutdown(self):
        logging.info("Shutting down MediaPlayer")
        # self.lcd.end()
        if self.mainLoop:
            self.mainLoop.stop()

    def getStatus(self):
        return self.status


def nav_handler(buttons):
    logging.debug("Handling navigation for [{}]".format(buttons))
    """Handle the navigation buttons"""
    if buttons == Lcd.BUTTON_SELECT:
        player.startPairing()
    elif buttons == Lcd.BUTTON_LEFT:
        player.previous()
    elif buttons == Lcd.BUTTON_RIGHT:
        player.next()
    elif buttons == Lcd.BUTTON_UP:
        if player.getStatus() == "playing":
            player.pause()
        else:
            player.play()


logging.basicConfig(filename=LOG_FILE, format=LOG_FORMAT, level=LOG_LEVEL)
logging.info("Starting MediaPlayer")

# TODO Add calbacks
inPins = [21]
outPins = [27]
retroPlayer = MediaPlayer(inPins, outPins)

# ######### MULTIPLEXER #########
# Nesting is output pin then input pin. Output "0" is hardwired to 3.3V
multiFuncs = [
    [retroPlayer.band_but, retroPlayer.seek_but("up"), retroPlayer.seek_but("down")],
    [retroPlayer.num_but(1), retroPlayer.num_but(2), retroPlayer.num_but(3)],
    [retroPlayer.num_but(4), retroPlayer.num_but(5), retroPlayer.num_but(6)],
]
retroPlayer.addMultiplexer(multiInPins, multiOutPins, multiFuncs)

try:
    retroPlayer.start()
except KeyboardInterrupt:
    logging.info("MediaPlayer cancelled by user")
except Exception as ex:
    logging.error(f"How embarrassing. The following error occurred: {ex}")
    traceback.print_exc()
finally:
    retroPlayer.shutdown()
