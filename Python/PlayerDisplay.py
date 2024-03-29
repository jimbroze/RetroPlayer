#!/usr/bin/env python3

import board
import digitalio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1305
import random
import asyncio

import logging

logging.basicConfig(level=logging.INFO)


class LCDDisplay(adafruit_ssd1305.SSD1305_SPI):
    """OLED display driver"""

    def __init__(self, pins, width, height):
        super().__init__(
            width, height, board.SPI(), pins["dc"], pins["rs"], pins["cs"],
        )
        self.width = width
        self.height = height
        # Clear display.
        self.fill(0)
        self.show()
        # Create blank image for drawing.
        # Make sure to create image with mode '1' for 1-bit color.
        self.displayImg = Image.new("1", (self.width, self.height))
        # Get drawing object to draw on image.
        self.draw = ImageDraw.Draw(self.displayImg)

        self.clear_display()

    def clear_display(self):
        self.draw.rectangle((0, 0, self.width, self.height), outline=0, fill=0)
        self.show()

    def println(self, text, textWidth=None, textHeight=None):
        """Standard text display. Local function?????"""

        # Load default font.
        font = ImageFont.load_default()
        # font = ImageFont.truetype("Fonts/Retro Gaming.ttf", 8)

        self.clear_display()

        # Draw Some Text
        (font_width, font_height) = font.getsize(text)
        self.draw.text(
            (self.width // 2 - font_width // 2, self.height // 2 - font_height // 2,),
            text,
            font=font,
            fill=255,
        )
        self.update_display()

    def update_display(self):
        self.image(self.displayImg)
        self.show()


class DisplayZone(LCDDisplay):
    """An area of the display which can be independently updated"""

    def __init__(self, display, x, y, width, height, parents=None):
        self.display = display
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.startPixel = (x, y)
        self.priority = 0
        self.iD = ""
        self.task = None
        self.children = []
        self.parents = []
        if parents is not None:
            for parent in parents:
                self.parents.append(parent)
                logging.debug(1)
                parent.add_child(self)

    def add_child(self, child):
        self.children.append(child)
        if self.parents:
            for parent in self.parents:
                # logging.debug(2)
                parent.add_child(child)

    def clear_display(self, update=True, wipeID=True):
        if wipeID:
            self.priority = 0
            self.iD = None
            for childZone in self.children:
                childZone.priority = 0
                childZone.iD = None
            self.cancel_task()
        self.display.draw.rectangle(
            (self.x, self.y, self.x + self.width, self.y + self.height),
            outline=0,
            fill=0,
        )
        if update:
            self.update_display()

    def update_display(self):
        self.display.update_display()

    def println(self, text, update=True, font=ImageFont.load_default(), offset=0):
        """Standard text display. Local function?????"""

        # Load default font.
        # font = ImageFont.truetype("Fonts/Retro Gaming.ttf", 8)

        self.clear_display(update=False, wipeID=False)

        # Draw Some Text
        (font_width, font_height) = font.getsize(text)
        self.display.draw.text(
            (
                self.x + (self.width // 2 - font_width // 2) + offset,
                # self.x + 5,
                self.y + (self.height // 2 - font_height // 2),
            ),
            text,
            font=font,
            fill=255,
            anchor="ms",
        )

        if update:
            self.update_display()

    async def print_scroll(self, text, font=ImageFont.load_default(), scrollSpeed=4):
        """Scrolling text display."""
        buffer = 5

        # print text here
        # font = ImageFont.truetype("Fonts/Retro Gaming.ttf", 8)
        (font_width, font_height) = font.getsize(text)
        scrollText = text
        while font_width >= self.width:
            scrollText = scrollText[:-1]
            (font_width, font_height) = font.getsize(scrollText)
        scrollSize = len(scrollText)
        noOfScrolls = len(text) - scrollSize + 1
        doubleText = text + (" " * buffer) + text

        while True:
            for i in range(noOfScrolls + scrollSize + buffer):
                scrollText = doubleText[i : (scrollSize + i)]
                self.println(scrollText, update=False)
                self.update_display()
                await asyncio.sleep(1 / scrollSpeed)

    def print_time(self, timeMS, update=True, hours=False):
        s, ms = divmod(timeMS, 1000)
        m, s = divmod(s, 60)
        h, m = divmod(m, 60)
        if hours:
            timeText = f"{h:d}:{m:02d}:{s:02d}"
        else:
            timeText = f"{m:02d}:{s:02d}"

        self.println(timeText, update=update)

    def print_text(self, text, font=ImageFont.load_default()):
        self.cancel_task()
        # font = ImageFont.truetype("Fonts/Retro Gaming.ttf", 8)
        (font_width, font_height) = font.getsize(text)
        if font_width > self.width:
            self.task = asyncio.create_task(self.print_scroll(text, font=font))
        else:
            self.println(text, font=font)

    def cancel_task(self):
        for zone in [self] + self.children:
            try:
                zone.task.cancel()
            except asyncio.CancelledError:
                logging.debug(f"Cancel not worked")
            except:
                logging.debug(f"Cancel not worked")


displayPins = {
    "cs": digitalio.DigitalInOut(board.D8),
    "dc": digitalio.DigitalInOut(board.D13),
    "rs": digitalio.DigitalInOut(board.D26),
}
displayWidth = 128
displayHeight = 32  # Was 64
Border = 8
fontScale = 1

topBarHeight = 9
bluetoothWidth = 6
cornerWidth = 20
mainTopHeight = 11
trackTimeBorder = 1
trackTimeNumberWidth = 38


class PlayerDisplay:
    """ """

    driver = "Driver"
    welcomeMessage = [
        "Howdy " + driver,
        "Hey " + driver,
        "Hello " + driver,
        "Welcome " + driver,
    ]

    display = LCDDisplay(displayPins, displayWidth, displayHeight)
    img = display.displayImg

    # Display Zones
    wholeDisplay = DisplayZone(display, 0, 0, displayWidth, displayHeight)
    topLeft = DisplayZone(display, 0, 0, cornerWidth, topBarHeight, [wholeDisplay])
    bluetoothZone = DisplayZone(display, 0, 0, bluetoothWidth, topBarHeight, [topLeft])
    topCenter = DisplayZone(
        display,
        cornerWidth,
        0,
        displayWidth - 2 * cornerWidth,
        topBarHeight,
        [wholeDisplay],
    )
    topRight = DisplayZone(
        display, displayWidth - 25, 0, cornerWidth, topBarHeight, [wholeDisplay]
    )
    mainZone = DisplayZone(  # Everything except top bar
        display,
        0,
        topBarHeight,
        displayWidth,
        displayHeight - topBarHeight,
        [wholeDisplay],
    )
    mainTop = DisplayZone(
        display, 0, topBarHeight, displayWidth, mainTopHeight, [mainZone]
    )
    mainBottom = DisplayZone(
        display,
        0,
        topBarHeight + mainTopHeight,
        displayWidth,
        displayHeight - mainTopHeight,
        [mainZone],
    )
    trackTimeLeft = DisplayZone(
        display,
        trackTimeBorder,
        topBarHeight + mainTopHeight,
        trackTimeNumberWidth,
        displayHeight - mainTopHeight - topBarHeight,
        [mainBottom],
    )
    trackTimeCenter = DisplayZone(
        display,
        trackTimeBorder + trackTimeNumberWidth,
        trackTimeLeft.y,
        displayWidth - 2 * (trackTimeBorder + trackTimeNumberWidth),
        trackTimeLeft.height,
        [mainBottom],
    )
    trackTimeRight = DisplayZone(
        display,
        displayWidth - (2 * trackTimeBorder + trackTimeNumberWidth),
        trackTimeLeft.y,
        trackTimeNumberWidth,
        trackTimeLeft.height,
        [mainBottom],
    )

    def __init__(self, loop):
        self.loop = loop

    async def check_priority(self, iD, zone, priority, timeToWait=3):
        """Check if a higher priority function is being shown on the display"""
        logging.info(f"{zone.iD}, {iD}")

        # TODO Need to check higher zones?
        if zone.iD != iD:
            attempts = 0
            minPriority = min(
                [
                    childZone.priority
                    for childZone in zone.children
                    if childZone.priority != 0
                ]
                + [zone.priority]
            )
            logging.info(f"{minPriority}")
            if minPriority != 0:
                while minPriority < priority:
                    await asyncio.sleep(0.5)
                    logging.info(f"priority attempt: {attempts}")
                    attempts += 1
                    if attempts > timeToWait / 0.5:
                        logging.info(f"Returning true")
                        return True
            zone.iD = iD
        # zone.clear_display()  # TODO is this needed?
        zone.priority = priority
        logging.info(f"Priority given to {zone.iD}")
        return False

    async def welcome(self):
        # Show welcome message
        await self.flash_message(
            random.choice(self.welcomeMessage), zone=self.wholeDisplay
        )

    async def flash_message(self, text, priority=2, time=3, zone=None):
        """Display a message on the main display zone for a limited time"""
        if zone is None:
            zone = self.mainZone
            # FIXME
        # if await self.check_priority(text, zone, priority) is False:
        #     return
        zone.println(text)
        await asyncio.sleep(time)
        zone.clear_display()

    def set_bluetooth(self, status):
        """Show or hide the bluetooth icon when a phone is connected/disconnected"""
        if status is True:
            blueImage = Image.open("Images/Bluetooth.png")
            self.display.img.paste(blueImage, self.bluetoothZone.startPixel)
            self.display.show()
        else:
            self.bluetoothZone.clear_display()

    async def update_track(self, track):
        """Show or update song information when playing"""
        logging.info(f"updating track info")
        task1 = asyncio.create_task(self.check_priority("track", self.topCenter, 3, 3))
        task2 = asyncio.create_task(self.check_priority("track", self.mainZone, 3, 3))
        if await task1 or await task2:
            return
        # Artist and album on middle line
        middleLine = []
        if "Artist" in track:
            middleLine.append(track["Artist"])
            if "Album" in track:
                middleLine.append(" - " + track["Album"])
        elif "Album" in track:
            middleLine.append("Album: " + track["Album"])
        self.mainTop.print_text("".join(middleLine))
        if "Title" in track:
            # Track name on top line
            self.topCenter.print_text(track["Title"])

    async def update_position(self, trackProgress, trackLength):
        """Show or update song information when playing"""
        if self.mainZone.iD != "track" or await self.check_priority(
            "track", self.mainZone, 3, 0
        ):
            return
        self.trackTimeLeft.print_time(trackProgress, update=False)
        self.trackTimeRight.print_time(trackLength, update=False)
        lineLength = self.trackTimeCenter.width - 2
        progressLength = trackProgress / trackLength * lineLength
        self.trackTimeCenter.clear_display(update=False, wipeID=False)
        # Left bar end
        self.display.draw.line(
            (
                self.trackTimeCenter.x,
                self.trackTimeCenter.y + self.trackTimeCenter.height // 2 - 3,
                self.trackTimeCenter.x,
                self.trackTimeCenter.y + self.trackTimeCenter.height // 2 + 3,
            ),
            fill=255,
        )
        # Left half of progress bar
        self.display.draw.line(
            (
                self.trackTimeCenter.x,
                self.trackTimeCenter.y + self.trackTimeCenter.height // 2,
                self.trackTimeCenter.x + progressLength,
                self.trackTimeCenter.y + self.trackTimeCenter.height // 2,
            ),
            fill=255,
            width=3,
        )
        # Right half of progress bar
        self.display.draw.line(
            (
                self.trackTimeCenter.x + progressLength,
                self.trackTimeCenter.y + self.trackTimeCenter.height // 2,
                self.trackTimeCenter.x + lineLength,
                self.trackTimeCenter.y + self.trackTimeCenter.height // 2,
            ),
            fill=255,
        )
        # Right bar end
        self.display.draw.line(
            (
                self.trackTimeCenter.x + lineLength,
                self.trackTimeCenter.y + self.trackTimeCenter.height // 2 - 3,
                self.trackTimeCenter.x + lineLength,
                self.trackTimeCenter.y + self.trackTimeCenter.height // 2 + 3,
            ),
            fill=255,
        )
        self.display.update_display()

    def clear_track(self):
        if self.mainZone.iD == "track":
            logging.info("Clearing track")
            self.mainZone.clear_display()
            self.topCenter.clear_display()


# oled = PlayerDisplay()

# # Draw a white background
# oled.draw.rectangle((0, 0, oled.width, oled.height), outline=255, fill=255)

# # Draw a smaller inner rectangle
# oled.draw.rectangle(
#     (BORDER, BORDER, oled.width - BORDER - 1, oled.height - BORDER - 1),
#     outline=0,
#     fill=0,
# )

# oled.display_text("Hello world")

# oled.update_display()
