"""Convert Figma SVGs rasterized by rsvg-convert -z 8 to LVGL assets."""
from pathlib import Path
from PIL import Image
from generate_concept_assets import write_asset

source = Path(__file__).resolve().parents[1] / "design/quick-settings"
for name in ("volume", "tare", "flush", "standby"):
    with Image.open(source / f"{name}.png") as image:
        size = (round(image.width / 8) * 8, round(image.height / 8) * 8)
        write_asset(f"quick_{name}", image.convert("RGBA").resize(size, Image.Resampling.LANCZOS))
