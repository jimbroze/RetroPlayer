#!/usr/bin/env python3

import board
import digitalio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1305

WIDTH = 128
HEIGHT = 32  # Was 64
BORDER = 8
FONTSCALE = 1

pins = {
    "cs": digitalio.DigitalInOut(board.D8),
    "dc": digitalio.DigitalInOut(board.D13),
    "rs": digitalio.DigitalInOut(board.D26),
}


class Display(adafruit_ssd1305.SSD1305_SPI):
    """OLED display driver"""

    def __init__(self, pins):
        super().__init__(
            WIDTH, HEIGHT, board.SPI(), pins["dc"], pins["rs"], pins["cs"],
        )

        # Clear display.
        self.fill(0)
        self.show()

    def displayText(self, text):
        """Standard text display"""

        # Load default font.
        font = ImageFont.load_default()

        # Draw Some Text
        (font_width, font_height) = font.getsize(text)
        draw.text(
            (self.width // 2 - font_width // 2, self.height // 2 - font_height // 2),
            text,
            font=font,
            fill=255,
        )
        self.show()


oled = Display(pins)


# Create blank image for drawing.
# Make sure to create image with mode '1' for 1-bit color.
image = Image.new("1", (oled.width, oled.height))

# Get drawing object to draw on image.
draw = ImageDraw.Draw(image)

# Draw a white background
draw.rectangle((0, 0, oled.width, oled.height), outline=255, fill=255)

# Draw a smaller inner rectangle
draw.rectangle(
    (BORDER, BORDER, oled.width - BORDER - 1, oled.height - BORDER - 1),
    outline=0,
    fill=0,
)

oled.image(image)

oled.displayText("Hello world")

# Display image

oled.show()
