#!/usr/bin/env python3

import time
import pigpio

# filter steady time in us
debounceTime = 1000

# pins = [pin, pullup, detect, callback?]

# Set both outs to on (high or low?)
# Set individually on.
# Read 3 outputs

# callback changes state?


class PlayerIO(pigpio.pi):
    def __init__(
        self, loop, pins, callbacks, debounceTime=10,
    ):
        self.loop = loop
        self.debounceTime = debounceTime
        super().__init__()
        # Comment out for testing
        # self.setPins(pins, callbacks)

    def setPins(self, pins, callbacks):
        for pin in pins:
            self.set_mode(pin, pigpio.INPUT)
            self.set_pull_up_down(pin, pigpio.PUD_DOWN)
            # Filter edge detection: Level change must be steady for steady microseconds
            self.set_glitch_filter(pin, self.debounceTime)
            self.callbacks = self.setCallbacks(pins, callbacks)

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
    def __init__(self, playerio, loop, inPins, outPins):
        self.playerio = playerio
        self.loop = loop
        self.inPins = {inPin: 0 for inPin in inPins}
        self.outPins = outPins
        self.noInPins = len(inPins)
        self.noOutPins = len(outPins)
        self.state = 0
        # Setup pins ????????????????????????????//// map pins to single list/tuple then call playerio method
        playerio.setPins(
            inPins, [self.checkInputs for pin in inPins],
        )

    def checkInputs(self, gpio, level, tick):
        """Input callback"""
        print(f"In pin {gpio} changed to to {level}")
        self.inPins[gpio] = level
        if self.state == 0 and level == 1:
            # self.cancelCallbacks(inPins)
            # self.playerio.setCallbacks(inPins)
            self.state = 1
            # threadsafe cyclePins(gpio)??
            output = self.cyclePins(gpio)
            print(output)

    def cyclePins(self, inPin):
        """Cycle through switching off multiplexing outputs until pin goes low"""
        for outPin in self.outPins:
            print(f"Changing pin {outPin} to 0")
            self.playerio.write(outPin, 0)
            # sleep. Try non-async sleep?
            time.sleep(1)
            if self.inPins[inPin] == 0:
                return self.state - 1
