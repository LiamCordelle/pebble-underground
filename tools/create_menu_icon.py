#!/usr/bin/env python3
"""Generate the native 25x25 launcher icon. Requires Pillow; no antialiasing."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]


def create_icon():
    """A transit route with a diagonal connection and three open stations."""
    # Match the watchface's day, steps and UV route colours exactly.
    red, green, purple = '#FF0000', '#00AA55', '#5500AA'
    black, white = '#000000', '#FFFFFF'
    image = Image.new('RGBA', (25, 25), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.line([(3, 2), (3, 9), (10, 16)], fill=red, width=3)
    draw.line([(10, 16), (21, 16), (21, 23)], fill=green, width=3)
    draw.line([(10, 16), (10, 22)], fill=purple, width=3)
    for x, y in ((3, 5), (10, 16), (21, 16)):
        draw.ellipse((x - 3, y - 3, x + 3, y + 3), fill=black)
        draw.ellipse((x - 1, y - 1, x + 1, y + 1), fill=white)
    return image


if __name__ == '__main__':
    output = ROOT / 'resources/images/menu_icon.png'
    output.parent.mkdir(parents=True, exist_ok=True)
    create_icon().save(output, optimize=True)
    print(output)
