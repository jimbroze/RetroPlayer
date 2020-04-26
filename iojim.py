#!/usr/bin/env python3

import time
import pigpio
import asyncio

# filter steady time in us
debounceTime = 10000

# pins = [pin, pullup, detect, callback?]

# Set both outs to on (high or low?)
# Set individually on.
# Read 3 outputs

# callback changes state?
# tes


class PlayerIO(pigpio.pi):
    def __init__(
        self, loop, pins, callbacks, debounceTime=10,
    ):
        self.loop = loop
        self.debounceTime = debounceTime
        super().__init__()
        # Comment out for testing
        # self.setPins(pins)
        # self.callbackobjs = self.setCallbacks(pins, callbacks)

    def setInPins(self, pins):
        for pin in pins:
            self.set_mode(pin, pigpio.INPUT)
            self.set_pull_up_down(pin, pigpio.PUD_DOWN)
            # Filter edge detection: Level change must be steady for steady microseconds
            self.set_glitch_filter(pin, self.debounceTime)

    def setOutPins(self, pins, levels):
        for pin, level in zip(pins, levels):
            self.set_mode(pin, pigpio.OUTPUT)
            self.write(pin, level)

    def setCallbacks(self, pins, callbackFuncs):
        callbacks = []
        for pin, callback in zip(pins, callbackFuncs):
            # If no callback given, assign default callback function????????????????????????????????????
            callbacks.append(
                self.callback(
                    pin,
                    pigpio.EITHER_EDGE,
                    callback if callback is not None else self.pinCallback,
                )
            )
        return callbacks

    def pinCallback(self, gpio, level, tick):
        print("Hi")


class MultiplexInput:
    def __init__(self, playerio, loop, inPins, outPins, multiCallback):
        self.playerio = playerio
        self.loop = loop
        self.inPinLevels = {inPin: 0 for inPin in inPins}
        self.inPins = inPins
        self.outPins = outPins
        self.state = 0
        self.multiCallback = multiCallback
        # Setup pins ????????????????????????????//// map pins to single list/tuple then call playerio method
        print("Setting out pins")
        playerio.setOutPins(outPins, [1 for pin in outPins])
        print("Setting in pins")
        playerio.setInPins(inPins)
        self.callbackobjs = playerio.setCallbacks(
            inPins, [self.checkInputs for pin in inPins],
        )

    def checkInputs(self, gpio, level, tick):
        """Input callback"""
        print(f"In pin {gpio} changed to {level}")
        self.inPinLevels[gpio] = level
        if self.state == 0 and level == 1:
            # self.cancelCallbacks(inPins)
            # self.playerio.setCallbacks(inPins)
            self.state = 1
            # threadsafe cyclePins(gpio)??
            asyncio.run_coroutine_threadsafe(self.cyclePins(gpio), self.loop)
            # output = self.cyclePins(gpio)
            # print(output)

    async def cyclePins(self, inPin):
        """Cycle through switching off multiplexing outputs until pin goes low"""
        for idx, outPin in enumerate(self.outPins):
            print(f"Changing pin {outPin} to 0")
            self.playerio.write(outPin, 0)
            await asyncio.sleep((debounceTime + 5) / 1000000)
            if self.inPinLevels[inPin] == 0:
                #     print(self.state - 1)
                self.playerio.setOutPins(self.outPins, [1 for pin in self.outPins])
                self.state = 0
                return self.multiCallback(self.inPins.index(inPin), idx + 1)
        self.playerio.setOutPins(self.outPins, [1 for pin in self.outPins])
        self.state = 0
        return self.multiCallback(inPin, 0)
        # print(self.inPins[inPin])
