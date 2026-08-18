#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Extract glyph outlines from a font into square monochrome SVG icons.

The glyph ink is centered on a 24x24 grid (matching common icon sets),
so the resulting icons are optically centered regardless of the font's
line metrics.

Usage: noto_glyph_to_svg.py <font.ttf> <outdir> <hexcodepoint>:<name> [...]

Example: noto_glyph_to_svg.py NotoSansSymbols2-Regular.ttf out \\
    25C0:triangle-left 25B6:triangle-right 23FB:power
"""

import argparse
from pathlib import Path

from fontTools.misc.transform import Transform
from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.ttLib import TTFont

GRID_SIZE = 24
# Empty margin around the ink: icon sets keep the ink inside a "live
# area" (e.g. 20x20 on a 24x24 grid in Material Design) instead of
# letting it touch the canvas edges
PADDING = 2


def glyph_to_svg(font: TTFont, codepoint: int) -> str:
    glyph_set = font.getGlyphSet()
    glyph_name = font.getBestCmap()[codepoint]

    bounds_pen = BoundsPen(glyph_set)
    glyph_set[glyph_name].draw(bounds_pen)
    if bounds_pen.bounds is None:
        raise ValueError(f"glyph U+{codepoint:04X} has no outline")
    x_min, y_min, x_max, y_max = bounds_pen.bounds
    width, height = x_max - x_min, y_max - y_min

    # Uniform scale (largest side fits the live area), y flipped from
    # font coordinates (up) to SVG coordinates (down), ink centered
    live_area = GRID_SIZE - 2 * PADDING
    scale = live_area / max(width, height)
    dx = (GRID_SIZE - width * scale) / 2 - scale * x_min
    dy = scale * y_max + (GRID_SIZE - height * scale) / 2

    svg_pen = SVGPathPen(glyph_set)
    glyph_set[glyph_name].draw(TransformPen(svg_pen, Transform(scale, 0, 0, -scale, dx, dy)))

    return (
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">'
        f'<path d="{svg_pen.getCommands()}"/></svg>\n'
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("font", type=Path)
    parser.add_argument("outdir", type=Path)
    parser.add_argument("glyphs", nargs="+", metavar="HEXCP:NAME")
    args = parser.parse_args()

    font = TTFont(args.font, lazy=True)
    args.outdir.mkdir(parents=True, exist_ok=True)
    for spec in args.glyphs:
        codepoint_str, name = spec.split(":", maxsplit=1)
        svg = glyph_to_svg(font, int(codepoint_str, 16))
        out_path = args.outdir / f"{name}.svg"
        out_path.write_text(svg)
        print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
