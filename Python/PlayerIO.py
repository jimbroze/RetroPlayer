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
DEBOUNCE_TIME = 10 * 1000
# Buffer time after debounce to allow program to catch up. Maybe not needed?
PAUSE_TIME = 0.5 * 1000

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
        self,
        mediaPlayer,
        inPins,
        callbacks,
        outPins,
        levels,
        debounceTime=DEBOUNCE_TIME,
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
        logging.debug(f"Set {pin} to input")
        if pull == "up":
            self.set_pull_up_down(pin, pigpio.PUD_UP)
            logging.debug(f"{pin} Pulled up")
        elif pull == "down":
            self.set_pull_up_down(pin, pigpio.PUD_DOWN)
            logging.debug(f"{pin} Pulled down")
        # Filter edge detection: Level change must be steady for steady microseconds
        self.set_glitch_filter(pin, self.debounceTime)

    def setOutPin(self, pin, level):
        self.set_mode(pin, pigpio.OUTPUT)
        self.write(pin, level)
        logging.debug(f"Set {pin} to output - {level}")

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
        logging.debug(f"In pin {gpio} changed to {level}")

    def getLoop(self):
        return self.loop


class MultiplexInput:
    """A Class to handle multiplexing multiple switches to a reduced number of GPIO pins.
    Takes, mediaplyer, input pins, output pins and a nested list of
    callback functions as inputs."""

    # Initial state of 1. Setup pins before setting to 0
    state = 1

    def __init__(self, playerio, inPins, outPins, multiCallback):
        self.playerio = playerio
        self.inPinLevels = {inPin: 0 for inPin in inPins}
        self.inPins = inPins
        self.outPins = outPins
        self.multiCallback = multiCallback

    async def setup(self):
        logging.debug("Setting out pins")
        for pin in self.outPins:
            self.playerio.setOutPin(pin, 1)
        logging.debug("Setting in pins")
        for pin in self.inPins:
            self.playerio.setInPin(pin, pull="down")
        self.callbackobjs = self.playerio.setCallbacks(
            self.inPins, [self.check_inputs for _ in self.inPins],
        )
        logging.debug(f"State is {self.state}")
        # FIXME Requires a long wait to ensure pins are stable.
        # Could change this to wait until pins are 0?
        await asyncio.sleep((10 * DEBOUNCE_TIME + PAUSE_TIME) / 1000000)
        self.state = 0
        logging.debug(f"State is {self.state}")

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
        logging.debug("MULTIPLEXER: Start cycling multiplexer pins")
        for idx, outPin in enumerate(self.outPins):
            logging.debug(f"MULTIPLEXER: Changing pin {outPin} to 0")
            self.playerio.write(outPin, 0)
            await asyncio.sleep((DEBOUNCE_TIME + PAUSE_TIME) / 1000000)
            # If input changes, connected output pin was the one that changed
            if self.inPinLevels[inPin] == 0:
                for pin in self.outPins:
                    self.playerio.setOutPin(pin, 1)
                await asyncio.sleep((DEBOUNCE_TIME + PAUSE_TIME) / 1000000)
                self.state = 0
                logging.debug(f"MULTIPLEXER: Inpin was {inPin}, OutPin {outPin}")
                self.multiCallback[idx + 1][self.inPins.index(inPin)]()
                return
        # If input level does not change, output is hardwired (0)
        for pin in self.outPins:
            self.playerio.setOutPin(pin, 1)
        await asyncio.sleep((DEBOUNCE_TIME + (1 * 1000)) / 1000000)
        self.state = 0
        logging.debug(f"MULTIPLEXER: Inpin was {inPin}, OutPin was Hardwired")
        self.multiCallback[0][self.inPins.index(inPin)]()


class SleepyPi:
    """ """

    serialOutData = {}

    # Serial port is swapped when not using onboard bluetooth
    def __init__(self, serialObj, serialMappingIn, serialDataOut, url="/dev/ttyAMA0"):
        self.url = url
        self.serialMappingIn = serialMappingIn
        self.serialObj = serialObj
        self.serialOutData = serialDataOut

    async def setup(self):
        self.reader, self.writer = await serial_asyncio.open_serial_connection(
            url=self.url, baudrate=9600
        )
        logging.info("Setting up Arduino communications.")

        await self.send({"awake": 1})
        await self.receive()

    async def send(self, data):
        # Convert to JSON object and encode in ascii
        msg = json.dumps(data)
        self.writer.write(msg.encode("ascii"))
        logging.debug(f"sent: {msg.rstrip()}")
        # Signal end of message
        self.writer.write(b"\n")

    def send_all(self):
        asyncio.create_task(self.send(self.serialOutdata))

    async def receive(self):
        while True:
            # Give control to event loop until data is received
            msg = await self.reader.readuntil(b"}")
            # TODO Send a re-request if data is not received properly
            # msg = await self.reader.readline()
            data = json.loads(msg.decode("utf-8"))
            logging.debug(f"received: {data}")
            if len(data) > 1:
                # Pass data to RetroPlayer
                self.serialObj = data
            elif len(data) == 1:
                # call a specific function
                self.serialMappingIn.get(next(iter(data)), lambda: "Invalid")(
                    data[next(iter(data))]
                )
            else:
                logging.error(f"Invalid serial data received: {data}")
