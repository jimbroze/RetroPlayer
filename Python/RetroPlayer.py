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
import signal
from functools import partial

import logging
import traceback

import asyncio
import asyncio_glib

from BlueHandler import BlueHandler
from PlayerIO import PlayerIO, MultiplexInput, SleepyPi
from PlayerDisplay import PlayerDisplay

# Multiplexer pins
multiInPins = [24, 25, 7]  # 1,4,band; 2,5,-; 3,6,+
multiOutPins = [17, 27]  # 4,5,6; 1,2,3; (B,-,+ hardwired)

inPins = [18, 23]  # And 23
outPins = [21]

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
    serialData = {}  # Dict/obj which holds all arduino serial data
    serialMappingIn = {
        # "dig": self.digital_change,
        # "analog": self.analog_change,
        # "disp": self.display_change,
        # "off": self.off_change,
        # "mode": self.mode_change,
        # "hand": self.handshake_change,
    }

    display = None
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
        self.serialMappingIn = {
            "dig": self.digital_change,
            "analog": self.analog_change,
            "disp": self.display_change,
            "off": self.off_change,
            "mode": self.mode_change,
            "hand": self.handshake_change,
        }
        self.inPins = inPins
        self.outPins = outPins

        # Allows asyncio to use dbus event loop
        asyncio.set_event_loop_policy(asyncio_glib.GLibEventLoopPolicy())
        self.mainLoop = asyncio.get_event_loop()
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        bus = dbus.SystemBus()

        self.blueHandler = BlueHandler(
            bus, self.mainLoop, self.player_handler, "DisplayYesNo"
        )
        self.playerio = PlayerIO(self, inPins, [None, None], outPins, [0])
        self.arduino = SleepyPi(
            self.serialData,
            self.serialMappingIn,
            serialDataOut={"out": [0, 0, 0], "awake": 1, "hand": 1, "alive": 0},
        )
        self.display = PlayerDisplay(self.mainLoop)
        self.mainLoop.create_task(self.display.welcome())

        # Add signal handler to exit on keyoard press
        for signame in ("SIGINT", "SIGTERM"):
            self.mainLoop.add_signal_handler(
                getattr(signal, signame),
                lambda: asyncio.ensure_future(self.shutdown(signame)),
            )

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
            self.mainLoop.create_task(multiplexer.setup())
        self.mainLoop.create_task(self.arduino.setup())
        self.mainLoop.run_forever()

    def player_handler(self, stateName, value):
        """Handle relevant property change signals"""
        logging.debug(f"{stateName}: {value}")
        if stateName == "Connected":
            prefix = "Connected to " if value[0] is True else "Disconnected from "
            self.mainLoop.create_task(self.display.flash_message(prefix + value[1]))

        if stateName == "State":
            return
        if stateName == "Track":
            self.mainLoop.create_task(self.display.update_track(value))
        if stateName == "Status":
            if value == "paused":
                self.display.clear_track()
        if stateName == "Discoverable":
            suffix = "on: ???s." if value is True else "off."
            self.mainLoop.create_task(
                self.display.flash_message("Pairing mode " + suffix)
            )
            return
        if stateName == "Alias":
            suffix = "True." if value is True else "False"
            self.mainLoop.create_task(self.display.flash_message("Alias " + suffix))

    def update_display(self):
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
                        self.show_paused()
                    else:
                        self.show_track()
            else:
                self.sleep()

    def show_device(self):
        """Display the device connection info on the LCD"""
        self.lcd.clear()
        self.lcd.writeLn("Connected to:", 0)
        self.lcd.writeLn(self.deviceAlias, 1)
        time.sleep(2)

    def show_track(self):
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
        logging.info("Band button pressed")

    def seek_but(self, direction):
        """ """
        logging.info(f"Seek {direction} button pressed")

    def num_but(self, number):
        """ """
        logging.info(f"Number {number} button pressed")
        if number == 6:
            logging.info("6")
            self.blueHandler.set_discoverable()

    async def shutdown(self, sigName):
        logging.info(f"Shutting down MediaPlayer. {sigName} was signalled")
        # self.lcd.end()

        self.mainLoop.stop()
        # self.mainLoop.close()

    async def getStatus(self):
        return self.status

    def digital_change(self, val):
        """ """

    def analog_change(self, val):
        """ """

    def display_change(self, val):
        """ """

    def off_change(self, val):
        """ """

    def mode_change(self, val):
        """ """

    def handshake_change(self, val):
        logging.debug(f"Handshake value of {val} received.")
        if val == 1:  # 1 = sending/sent
            # Send raspi side of full handshake
            # self.arduino.send_all()
            return
        if val == 2:  # 2 = Success/received
            return
        if val == 3:  # 3 = No awake signal received
            # Send awake signal
            asyncio.create_task(
                self.arduino.send({"awake": 1})
            )  # Latest way to create coroutine task (3.7+)
            return
        if val == 4:  # 4 = No received signal received
            # Send raspi side of full handshake
            self.arduino.send_all()
            return

    def send_outputs(self, outputs):
        """ """


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
retroPlayer = MediaPlayer(inPins, outPins)

# ######### MULTIPLEXER #########
# Nesting is output pin then input pin. Output "0" is hardwired to 3.3V
multiFuncs = [
    [
        retroPlayer.band_but,
        partial(retroPlayer.seek_but, "down"),
        partial(retroPlayer.seek_but, "up"),
    ],
    [
        partial(retroPlayer.num_but, 4),
        partial(retroPlayer.num_but, 5),
        partial(retroPlayer.num_but, 6),
    ],
    [
        partial(retroPlayer.num_but, 1),
        partial(retroPlayer.num_but, 2),
        partial(retroPlayer.num_but, 3),
    ],
]
retroPlayer.addMultiplexer(multiInPins, multiOutPins, multiFuncs)

logging.info("Starting RetroPlayer")
retroPlayer.start()
