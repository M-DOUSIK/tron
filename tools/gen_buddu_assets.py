#!/usr/bin/env python3
"""Generate compact firmware sprites from Abirami's original Buddu artwork.

The PNG files in tools/mascot_and_frames_design are the source of truth.  This
script never writes to them.  It validates all 23 files, records their hashes
in the generated output, and emits only the unique runtime artwork needed by
the 800x480 firmware.  Screen-only drawings (intro, home, keyboards and the
registration variants) are represented by firmware drawing code so controls
and entered values stay interactive.

Pillow is required by the generator only; the generated firmware has no image
library dependency.
"""

from __future__ import annotations

import argparse
import hashlib
from collections import deque
from dataclasses import dataclass
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parent
SOURCE_DIR = ROOT / "mascot_and_frames_design"
DEFAULT_OUTPUT = (
    ROOT.parent
    / "sessions/session_13/FSBL/Inc/ui/buddu_assets_data.inc"
)


# Every supplied drawing is deliberately listed.  Generation fails if the set
# changes, preventing a future asset refresh from silently dropping artwork.
ARTWORK_MANIFEST = (
    "State0-idle(1)/mascot - buddu_20260908135550.png",
    "State0-idle(1)/mascot - buddu_20260908135610.png",
    "Intro(2)/mascot - buddu_20260907193120.png",
    "(3)Home screen display/mascot - buddu_20260907195114.png",
    "(4) keyboard /mascot - buddu_20260908113716.png",
    "(4) keyboard /mascot - buddu_20260908113743.png",
    "(5) registration/mascot - buddu_20260908121900.png",
    "(5) registration/mascot - buddu_20260908121910.png",
    "(5) registration/mascot - buddu_20260908121924.png",
    "(5) registration/mascot - buddu_20260908121937.png",
    "(5) registration/mascot - buddu_20260908122959.png",
    "(6) registration confirmed GIF/mascot - buddu_20260908125734.png",
    "(6) registration confirmed GIF/mascot - buddu_20260908125745.png",
    "(7)Dispensing gif/mascot - buddu_20260908134713.png",
    "(7)Dispensing gif/mascot - buddu_20260908134707.png",
    "(7)Dispensing gif/mascot - buddu_20260908134659.png",
    "(7)Dispensing gif/mascot - buddu_20260908134649.png",
    "(8) collect pills/mascot - buddu_20260908141206.png",
    "(9) pill taken gif/mascot - buddu_20260908141459.png",
    "(9) pill taken gif/mascot - buddu_20260908141510.png",
    "(10) error message gif/mascot - buddu_20260908135418.png",
    "(10) error message gif/mascot - buddu_20260908135433.png",
    "(10) error message gif/mascot - buddu_20260908135451.png",
)


@dataclass(frozen=True)
class SpriteSpec:
    name: str
    source: str
    crop: tuple[int, int, int, int]
    fit: tuple[int, int]


# Crops contain artwork only.  Background removal uses an edge flood-fill,
# which preserves enclosed white areas such as Buddu's face and body.
SPRITES = (
    SpriteSpec("buddu_sleep0", ARTWORK_MANIFEST[0], (280, 500, 940, 1260), (245, 270)),
    SpriteSpec("buddu_sleep1", ARTWORK_MANIFEST[1], (280, 500, 940, 1260), (245, 270)),
    SpriteSpec("buddu_register", ARTWORK_MANIFEST[6], (135, 420, 490, 830), (190, 215)),
    SpriteSpec("buddu_registered0", ARTWORK_MANIFEST[11], (215, 410, 1025, 1260), (245, 275)),
    SpriteSpec("buddu_registered1", ARTWORK_MANIFEST[12], (215, 410, 1025, 1260), (245, 275)),
    # All four dispensing drawings have byte-identical cat pixels.  The
    # renderer draws their authored 0/1/2/3-dot progression around this one
    # shared pose, avoiding three duplicate bitmaps in the 511 KB ROM.
    SpriteSpec("buddu_dispense", ARTWORK_MANIFEST[13], (300, 285, 940, 850), (215, 200)),
    SpriteSpec("buddu_collect", ARTWORK_MANIFEST[17], (280, 300, 900, 950), (225, 225)),
    SpriteSpec("buddu_taken0", ARTWORK_MANIFEST[18], (215, 410, 1025, 1260), (245, 275)),
    SpriteSpec("buddu_taken1", ARTWORK_MANIFEST[19], (215, 410, 1025, 1260), (245, 275)),
    SpriteSpec("buddu_error0", ARTWORK_MANIFEST[20], (215, 600, 1025, 1260), (230, 210)),
    SpriteSpec("buddu_error1", ARTWORK_MANIFEST[21], (215, 600, 1025, 1260), (230, 210)),
    SpriteSpec("buddu_error2", ARTWORK_MANIFEST[22], (215, 600, 1025, 1260), (230, 210)),
)


def validate_sources() -> list[tuple[str, str]]:
    expected = set(ARTWORK_MANIFEST)
    actual = {
        p.relative_to(SOURCE_DIR).as_posix()
        for p in SOURCE_DIR.rglob("*.png")
    }
    missing = sorted(expected - actual)
    unexpected = sorted(actual - expected)
    if missing or unexpected:
        raise SystemExit(
            f"Artwork set changed. Missing={missing!r}; unexpected={unexpected!r}"
        )

    hashes = []
    for rel in ARTWORK_MANIFEST:
        digest = hashlib.sha256((SOURCE_DIR / rel).read_bytes()).hexdigest()
        hashes.append((rel, digest))
    return hashes


def is_canvas(rgb: tuple[int, int, int]) -> bool:
    return min(rgb) >= 244 and max(rgb) - min(rgb) <= 8


def remove_canvas(image: Image.Image) -> Image.Image:
    """Flood transparent canvas from crop edges while retaining white fill."""
    rgb = image.convert("RGB")
    w, h = rgb.size
    pixels = rgb.load()
    outside = bytearray(w * h)
    queue: deque[tuple[int, int]] = deque()

    def seed(x: int, y: int) -> None:
        i = y * w + x
        if not outside[i] and is_canvas(pixels[x, y]):
            outside[i] = 1
            queue.append((x, y))

    for x in range(w):
        seed(x, 0)
        seed(x, h - 1)
    for y in range(h):
        seed(0, y)
        seed(w - 1, y)

    while queue:
        x, y = queue.popleft()
        for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
            if 0 <= nx < w and 0 <= ny < h:
                i = ny * w + nx
                if not outside[i] and is_canvas(pixels[nx, ny]):
                    outside[i] = 1
                    queue.append((nx, ny))

    rgba = rgb.convert("RGBA")
    alpha = Image.new("L", (w, h), 255)
    alpha.putdata([0 if value else 255 for value in outside])
    rgba.putalpha(alpha)
    bbox = alpha.getbbox()
    if bbox is None:
        raise ValueError("crop contains no visible artwork")

    pad = 4
    left = max(0, bbox[0] - pad)
    top = max(0, bbox[1] - pad)
    right = min(w, bbox[2] + pad)
    bottom = min(h, bbox[3] + pad)
    return rgba.crop((left, top, right, bottom))


def fit_image(image: Image.Image, box: tuple[int, int]) -> Image.Image:
    scale = min(box[0] / image.width, box[1] / image.height, 1.0)
    size = (max(1, round(image.width * scale)), max(1, round(image.height * scale)))
    return image.resize(size, Image.Resampling.LANCZOS)


def rgb565(rgb: tuple[int, int, int]) -> int:
    r, g, b = rgb
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def quantize(image: Image.Image) -> tuple[list[int], bytes]:
    """Return a 16-entry RGB565 palette and one 0..15 index per pixel."""
    alpha = image.getchannel("A")
    white = Image.new("RGB", image.size, "white")
    white.paste(image.convert("RGB"), mask=alpha)
    q = white.quantize(colors=15, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    raw_palette = q.getpalette()[: 15 * 3]
    palette = [0]
    for i in range(15):
        offset = i * 3
        palette.append(rgb565(tuple(raw_palette[offset : offset + 3])))

    indices = bytearray(q.size[0] * q.size[1])
    qdata = bytes(q.getdata())
    adata = bytes(alpha.getdata())
    for i, (color, opacity) in enumerate(zip(qdata, adata)):
        indices[i] = 0 if opacity < 128 else color + 1
    return palette, bytes(indices)


def rle_encode(indices: bytes) -> bytes:
    """Encode index runs as [length, palette-index] pairs."""
    if not indices:
        return b""
    output = bytearray()
    value = indices[0]
    length = 1
    for current in indices[1:]:
        if current == value and length < 255:
            length += 1
        else:
            output.extend((length, value))
            value = current
            length = 1
    output.extend((length, value))
    return bytes(output)


def c_array(values: list[int] | bytes, width: int = 12, hex_width: int = 2) -> str:
    rendered = [f"0x{value:0{hex_width}X}" for value in values]
    return "\n".join(
        "    " + ", ".join(rendered[i : i + width]) + ","
        for i in range(0, len(rendered), width)
    )


def generate(output: Path, preview_dir: Path | None) -> None:
    hashes = validate_sources()
    sections = [
        "/* buddu_assets_data.inc - GENERATED by tools/gen_buddu_assets.py.",
        " * Do not edit. Abirami's PNG files are the source of truth.",
        " * All supplied drawings are validated below, including screen-only references.",
    ]
    sections.extend(f" * SHA256 {digest}  {rel}" for rel, digest in hashes)
    sections.extend((" */", ""))

    total_bytes = 0
    for spec in SPRITES:
        source = Image.open(SOURCE_DIR / spec.source).convert("RGBA")
        artwork = remove_canvas(source.crop(spec.crop))
        artwork = fit_image(artwork, spec.fit)
        palette, indices = quantize(artwork)
        encoded = rle_encode(indices)
        total_bytes += len(encoded) + len(palette) * 2

        if preview_dir is not None:
            preview_dir.mkdir(parents=True, exist_ok=True)
            artwork.save(preview_dir / f"{spec.name}.png")

        sections.extend(
            (
                f"/* {spec.source} */",
                f"static const uint16_t pal_{spec.name}[16] BUDDU_ASSET = {{",
                c_array(palette, width=8, hex_width=4),
                "};",
                f"static const uint8_t rle_{spec.name}[] BUDDU_ASSET = {{",
                c_array(encoded),
                "};",
                f"const ui_rle_sprite_t ui_sprite_{spec.name} = {{",
                f"    {artwork.width}u, {artwork.height}u, pal_{spec.name},",
                f"    rle_{spec.name}, sizeof(rle_{spec.name})",
                "};",
                "",
            )
        )

    sections.append(f"/* Runtime Buddu sprite bytes: {total_bytes}. */")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(sections) + "\n", encoding="utf-8")
    print(f"Generated {output}")
    print(f"Runtime Buddu sprite bytes: {total_bytes}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--preview-dir", type=Path)
    args = parser.parse_args()
    generate(args.output.resolve(), args.preview_dir.resolve() if args.preview_dir else None)


if __name__ == "__main__":
    main()
