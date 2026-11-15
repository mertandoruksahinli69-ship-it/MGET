#!/usr/bin/env python3
"""Generate transparent MGET icons: exactly one black M."""
from pathlib import Path
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parent.parent
assets = root / "assets"
assets.mkdir(exist_ok=True)
scale = 4
image = Image.new("RGBA", (256 * scale, 256 * scale), (0, 0, 0, 0))
draw = ImageDraw.Draw(image)
outline = [(45, 210), (45, 46), (87, 46), (128, 116),
           (169, 46), (211, 46), (211, 210), (169, 210),
           (169, 108), (128, 178), (87, 108), (87, 210)]
draw.polygon([(x * scale, y * scale) for x, y in outline], fill=(0, 0, 0, 255))
image = image.resize((256, 256), Image.Resampling.LANCZOS)
image.save(assets / "MGET.png")
image.save(assets / "MGET.ico", sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])
print("Generated MGET.png and MGET.ico")
