#!/usr/bin/env python3

"""Contains classes that handle IO functions for a media player.
- PlayerIO for the primary IO
- MultiplexInput for handling multiple switches through a reduced number of GPIO pins
"""

import logging
import pigpio
import asyncio
import serial_asyncio
import json

# filter steady time in us. (10ms)
debounceTime = 10 * 1000

# pins = [pin, pullup, detect, callback?]

# Set both outs to on (high or low?)
# Set individually on.
# Read 3 outputs

# callback changes state?
# tes


class PlayerIO(pigpio.pi):
    """Handles media player GPIO. Takes asyncio loop, input pins, input pin callbacks,
    output pins and input pin debounce time (us) as arguments"""

    mediaPlayer = None
    loop = None
    debounceTime = None
    inPins = None
    pulls = None
    outPins = None
    levels = None

    def __init__(
        self, mediaPlayer, inPins, callbacks, outPins, levels, debounceTime=10000,
    ):
        self.mediaPlayer = mediaPlayer
        self.loop = mediaPlayer.get_loop()
        self.inPins = inPins
        self.pulls = [None for _ in inPins]
        self.callbackFuncs = callbacks
        self.outPins = outPins
        self.levels = {pin: level for pin, level in zip(outPins, levels)}
        self.debounceTime = debounceTime
        super().__init__()
        # Comment out for testing
        # self.setPins(pins)
        # self.callbackobjs = self.setCallbacks(pins, callbacks)

    def setup(self):
        for pin, pull in zip(self.inPins, self.pulls):
            self.setInPin(pin, pull)
        self.callbackObjs = self.setCallbacks(self.inPins, self.callbackFuncs)

        for pin in self.outPins:
            self.setOutPin(pin, self.levels[pin])

    def setInPin(self, pin, pull=None):
        self.set_mode(pin, pigpio.INPUT)
        if pull == "up":
            self.set_pull_up_down(pin, pigpio.PUD_UP)
        elif pull == "down":
            self.set_pull_up_down(pin, pigpio.PUD_DOWN)
        # Filter edge detection: Level change must be steady for steady microseconds
        self.set_glitch_filter(pin, self.debounceTime)

    def setOutPin(self, pin, level):
        self.set_mode(pin, pigpio.OUTPUT)
        self.write(pin, level)

    def setCallbacks(self, pins, callbackFuncs):
        callbacks = []
        for pin, callback in zip(pins, callbackFuncs):
            # If no callback given, assign default callback function
            callbacks.append(
                self.callback(
                    pin,
                    pigpio.EITHER_EDGE,
                    callback if callback is not None else self.pinCallback,
                )
            )
        return callbacks

    def getLevel(self, pin):
        """ """
        # Is there any need for this func?

    def pinCallback(self, gpio, level, tick):
        print("Hi")

    def getLoop(self):
        return self.loop


class MultiplexInput:
    """A Class to handle multiplexing multiple switches to a reduced number of GPIO pins.
    Takes, mediaplyer, input pins, output pins and a nested list of
    callback functions as inputs."""

    state = 0

    def __init__(self, playerio, inPins, outPins, multiCallback):
        self.playerio = playerio
        self.inPinLevels = {inPin: 0 for inPin in inPins}
        self.inPins = inPins
        self.outPins = outPins
        self.multiCallback = multiCallback

    def setup(self):
        logging.debug("Setting out pins")
        for pin in self.outPins:
            self.playerio.setOutPin(pin, 1)
        logging.debug("Setting in pins")
        for pin in self.inPins:
            self.playerio.setInPin(pin)
        self.callbackobjs = self.playerio.setCallbacks(
            self.inPins, [self.check_inputs for _ in self.inPins],
        )

    def check_inputs(self, gpio, level, tick):
        """Input callback"""
        logging.debug(f"In pin {gpio} changed to {level}")
        self.inPinLevels[gpio] = level
        if self.state == 0 and level == 1:
            self.state = 1
            asyncio.run_coroutine_threadsafe(
                self.cycle_pins(gpio), self.playerio.getLoop()
            )

    async def cycle_pins(self, inPin):
        """Cycle through switching off multiplexing outputs until pin goes low"""
        for idx, outPin in enumerate(self.outPins):
            logging.debug(f"Changing pin {outPin} to 0")
            self.playerio.write(outPin, 0)
            await asyncio.sleep((debounceTime + 5) / 1000000)
            # If input changes, connected output pin was the one that changed
            if self.inPinLevels[inPin] == 0:
                self.playerio.setOutPins(self.outPins, [1 for _ in self.outPins])
                self.state = 0
                return self.multiCallback(self.inPins.index(inPin), idx + 1)
        # If input level does not change, output is hardwired (0)
        self.playerio.setOutPins(self.outPins, [1 for _ in self.outPins])
        self.state = 0
        return self.multiCallback(inPin, 0)


class SleepyPi:
    """ """

    def __init__(self, url="/dev/ttyS0"):
        self.url = url

    async def setup(self):
        self.reader, self.writer = await serial_asyncio.open_serial_connection(
            url=self.url, baudrate=115200
        )

        await self.receive()

    async def send(self, data):
        # Convert to JSON object and encode in ascii
        msg = json.dumps(data)
        self.writer.write(msg.encode("ascii"))
        logging.debug(f"sent: {msg.rstrip()}")
        # Signal end of message
        self.writer.write(b"\n")

    async def receive(self):
        while True:
            # Give control to event loop until data is received
            msg = await self.reader.readuntil(b"\n")
            data = json.loads(msg.decode("utf-8"))
            logging.debug(f"received: {data}")
