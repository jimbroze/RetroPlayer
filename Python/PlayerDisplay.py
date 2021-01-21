#!/usr/bin/env python3

import board
import digitalio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1305
import random
import asyncio


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

        self.clear_display()

        # Draw Some Text
        (font_width, font_height) = font.getsize(text)
        self.draw.text(
            (
                self.width // 2 - font_width // 2,
                self.mainWindowHeight // 2 - font_height // 2,
            ),
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

    priority = 0
    iD = 0
    children = []

    def __init__(self, display, x, y, width, height, parents=None):
        self.display = display
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.startPixel = (x, y)
        for parent in parents:
            self.parents.append(parent)
            parent.children.append(self)
            for grandParent in parent.parents:
                self.parents.append(grandParent)
                grandParent.children.append(self)

    def clear_display(self):
        self.priority = 0
        self.iD = None
        for childZone in self.children:
            childZone.priority = 0
            childZone.iD = None
        self.display.draw.rectangle(
            (self.x, self.y, self.width, self.height), outline=0, fill=0
        )
        self.display.show()

    def print_time(self, trackLength):
        # Load default font.
        font = ImageFont.load_default()

        # Draw Some Text
        (font_width, font_height) = font.getsize(trackLength)
        self.draw.text(
            (
                self.width // 2 - font_width // 2,
                self.mainWindowHeight // 2 - font_height // 2,
            ),
            trackLength,
            font=font,
            fill=255,
        )


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
cornerWidth = 25
mainTopHeight = 11
trackTimeBorder = 1
trackTimeNumberWidth = 32


class PlayerDisplay:
    """ """

    welcomeMessage = ["Howdy Jim", "Hey Jim", "Hello Jim", "Welcome Jim"]

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
    mainZone = DisplayZone(
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
        displayHeight - mainTopHeight,
        [mainBottom],
    )
    trackTimeCenter = DisplayZone(
        display,
        trackTimeBorder + trackTimeNumberWidth,
        topBarHeight + mainTopHeight,
        displayWidth - 2 * (trackTimeBorder + trackTimeNumberWidth),
        displayHeight - mainTopHeight,
        [mainBottom],
    )
    trackTimeRight = DisplayZone(
        display,
        displayWidth - (2 * trackTimeBorder + trackTimeNumberWidth),
        topBarHeight + mainTopHeight,
        trackTimeNumberWidth,
        displayHeight - mainTopHeight,
        [mainBottom],
    )

    def __init__(self, loop):
        self.loop = loop

    async def check_priority(self, iD, zone, priority, numAttempts=6):
        """Check if a higher priority function is being shown on the display"""
        attempts = 0
        minPriority = min(
            [
                childZone.priority
                for childZone in zone.children
                if childZone.priority != 0
            ]
            + zone.priority
        )
        while minPriority <= priority:
            asyncio.sleep(0.5)
            attempts += 1
            if attempts >= numAttempts:
                return False
        zone.clear_display()
        zone.priority = priority
        zone.iD = iD
        return True

    async def welcome(self):
        # Show welcome message
        self.flash_message(random.choice(self.welcomeMessage), zone=self.wholeDisplay)

    async def flash_message(self, text, priority=2, time=3, zone=None):
        """Display a message on the main display zone for a limited time"""
        if zone is None:
            zone = self.mainZone
        if self.check_priority(text, zone, priority) is False:
            return
        zone.println(text)
        asyncio.sleep(time)
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
        if self.check_priority("track", self.mainZone, 3, 1):
            return
        # Artist and album on middle line
        middleLine = []
        if "Artist" in track:
            middleLine.append(track["Artist"])
            if "Album" in track:
                middleLine.append(" - " + track["Album"])
        elif "Album" in track:
            middleLine.append("Album: " + track["Album"])
        self.mainTop.println(middleLine)
        if self.track["Title"]:
            # Track name on top line
            self.topCenter.println(track["Title"])

            trackLength = 0
            trackProgress = 0
            self.trackTimeLeft.print_time(trackProgress)
            self.trackTimeRight.print_time(trackLength)
            lineLength = self.trackTimeCenter.width - 2
            progressLength = trackProgress / trackLength * lineLength
            # Left bar end
            self.display.draw.line(
                (
                    self.trackTimeCenter.x + 1,
                    self.trackTimeCenter.y + self.trackTimeCenter.height / 2,
                    1,
                    5,
                ),
                fill=255,
            )
            # Left half of progress bar
            self.display.draw.line(
                (
                    self.trackTimeCenter.x + 1,
                    self.trackTimeCenter.y + self.trackTimeCenter.height / 2,
                    progressLength,
                    3,
                ),
                fill=255,
            )
            # Right half of progress bar
            self.display.draw.line(
                (
                    self.trackTimeCenter.x + 1 + progressLength,
                    self.trackTimeCenter.y + self.trackTimeCenter.height / 2,
                    lineLength - progressLength,
                    1,
                ),
                fill=255,
            )
            # Right bar end
            self.display.draw.line(
                (
                    self.trackTimeCenter.x + lineLength,
                    self.trackTimeCenter.y + self.trackTimeCenter.height / 2,
                    1,
                    5,
                ),
                fill=255,
            )

    def clear_track(self):
        if self.mainZone.iD == "track":
            self.mainZone.clear_display()


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
