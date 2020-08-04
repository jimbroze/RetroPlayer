#!/usr/bin/env python3

import board
import digitalio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1305
import random

WIDTH = 128
HEIGHT = 32  # Was 64
BORDER = 8
FONTSCALE = 1

DISPLAY_PINS = {
    "cs": digitalio.DigitalInOut(board.D8),
    "dc": digitalio.DigitalInOut(board.D13),
    "rs": digitalio.DigitalInOut(board.D26),
}


class PlayerDisplay(adafruit_ssd1305.SSD1305_SPI):
    """OLED display driver"""

    welcomeMessage = ["Howdy Jim", "Hey handsome", "Hello Jim", "Welcome Jim"]

    def __init__(self, pins=DISPLAY_PINS):
        super().__init__(
            WIDTH, HEIGHT, board.SPI(), pins["dc"], pins["rs"], pins["cs"],
        )
        # Clear display.
        self.fill(0)
        self.show()
        # Create blank image for drawing.
        # Make sure to create image with mode '1' for 1-bit color.
        self.imageVar = Image.new("1", (self.width, self.height))
        # Get drawing object to draw on image.
        self.draw = ImageDraw.Draw(self.imageVar)

        self.clear_display()
        # Show welcome message
        self.println(random.choice(self.welcomeMessage))

    def clear_display(self):
        self.draw.rectangle((0, 0, self.width, self.height), outline=0, fill=0)
        self.show()

    def println(self, text):
        """Standard text display"""

        # Load default font.
        font = ImageFont.load_default()

        # Draw Some Text
        (font_width, font_height) = font.getsize(text)
        self.draw.text(
            (self.width // 2 - font_width // 2, self.height // 2 - font_height // 2),
            text,
            font=font,
            fill=255,
        )
        self.update_display()

    def update_display(self):
        self.image(self.imageVar)
        self.show()


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
