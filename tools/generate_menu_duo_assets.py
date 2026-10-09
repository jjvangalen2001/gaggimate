"""Convert the original Figma menu SVG renders to native LVGL assets.

Rasterize each SVG at 8x using librsvg before running this script:
rsvg-convert -z 8 design/menu-duo/brew.svg -o design/menu-duo/brew.png
Repeat for steam, grind, power and down. SVG root dimensions are preserved.
"""
from pathlib import Path
from PIL import Image
from generate_concept_assets import write_asset

source = Path(__file__).resolve().parents[1] / "design/menu-duo"
for name in ("brew", "steam", "grind", "power", "down"):
    with Image.open(source / f"{name}.png") as image:
        write_asset(f"menu_duo_{name}", image.convert("RGBA"))
