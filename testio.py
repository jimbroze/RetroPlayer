#!/usr/bin/env python3

import time
import asyncio

from playerio import PlayerIO, MultiplexInput

inPins = [17, 27]
outPins = [23, 24]

multiInPins = [16, 19, 20]
multiOutPins = [12, 13]
# Nesting is output pin then input pin. Output "0" is hardwired to 3.3V
multiFuncs = [
    [self.band(), self.seek_up(), self.seek_down(),],
    [self.butt_1(), self.butt_2(), self.butt_3(),],
    [self.butt_4(), self.butt_5(), self.butt_6(),],
]


def testCallback(inPin, outPin):
    print(f"In is {inPin} and Out is {outPin}")


loop = asyncio.get_event_loop()

playerio = PlayerIO(None, loop, None)
multiplexer = MultiplexInput(playerio, loop, inPins, outPins, testCallback)

try:
    # while True:
    #     time.sleep(1)
    loop.run_forever()
except KeyboardInterrupt:
    print("bye")
