#!/usr/bin/env python3
"""Generate a static C++ theme header from a design-time theme source.

Usage:
    python tools/generate_theme_header.py [input.json] [output.h]

Defaults:
    input  = src/themes/Default.json
    output = src/generated/DefaultTheme.h

This is a BUILD-TIME / DESIGN-TIME tool. The plugin never runs it and never
reads the JSON source at runtime — it only ever includes the generated header.

The generator:
  * reads the JSON source (any JSON parsing happens here, in tooling, never in
    the plugin binary),
  * maps source keys onto a fixed set of NEUTRAL semantic tokens,
  * normalizes colors authored as "#RRGGBB", "#RRGGBBAA", [r,g,b], [r,g,b,a]
    (0-255 int arrays) or normalized float arrays (0..1) into float RGBA,
  * emits a safe fallback (with a comment) for any token missing from the
    source.

No vendor / product / editor identity is read into or written from the public
token API: only the neutral semantic token names below are emitted.
"""

from __future__ import annotations

import json
import os
import sys

# --- Neutral semantic color tokens: name -> fallback (float RGBA) -----------
COLOR_TOKENS = [
    ("Background",     (0.070, 0.075, 0.085, 1.0)),
    ("Panel",          (0.095, 0.102, 0.118, 1.0)),
    ("PanelElevated",  (0.120, 0.130, 0.150, 1.0)),
    ("Border",         (0.210, 0.225, 0.250, 1.0)),
    ("BorderMuted",    (0.150, 0.160, 0.180, 1.0)),
    ("TextPrimary",    (0.880, 0.900, 0.930, 1.0)),
    ("TextSecondary",  (0.600, 0.640, 0.700, 1.0)),
    ("TextMuted",      (0.420, 0.450, 0.500, 1.0)),
    ("Accent",         (0.480, 0.620, 1.000, 1.0)),
    ("AccentMuted",    (0.230, 0.300, 0.460, 1.0)),
    ("Warning",        (1.000, 0.640, 0.250, 1.0)),
    ("Danger",         (1.000, 0.360, 0.360, 1.0)),
    ("Success",        (0.400, 0.840, 0.600, 1.0)),
    ("MeterInput",     (0.480, 0.620, 1.000, 1.0)),
    ("MeterOutput",    (0.400, 0.840, 0.600, 1.0)),
    ("MeterReduction", (1.000, 0.640, 0.250, 1.0)),
]

# --- Neutral semantic scalar tokens: name -> fallback (float) ----------------
FLOAT_TOKENS = [
    ("RadiusSmall",     5.0),
    ("RadiusMedium",    8.0),
    ("RadiusLarge",    12.0),
    ("SpacingXSmall",   4.0),
    ("SpacingSmall",    8.0),
    ("SpacingMedium",  12.0),
    ("SpacingLarge",   16.0),
    ("FontSizeTiny",   10.0),
    ("FontSizeSmall",  11.0),
    ("FontSizeBody",   12.0),
    ("FontSizeTitle",  14.0),
    ("FontSizeDisplay", 28.0),
]

# Optional aliases: alternative source key paths (dotted) that should be mapped
# onto a neutral semantic token. This lets a richer/foreign source be remapped
# WITHOUT preserving any of its key names in the generated API. Purely generic
# UI vocabulary — no vendor/product names.
ALIASES = {
    "Background":     ["colors.Background", "surface.base", "surface.window"],
    "Panel":          ["colors.Panel", "surface.panel"],
    "PanelElevated":  ["colors.PanelElevated", "surface.raised", "surface.elevated"],
    "Border":         ["colors.Border", "border.strong", "border.normal"],
    "BorderMuted":    ["colors.BorderMuted", "border.subtle", "border.muted"],
    "TextPrimary":    ["colors.TextPrimary", "text.primary"],
    "TextSecondary":  ["colors.TextSecondary", "text.secondary"],
    "TextMuted":      ["colors.TextMuted", "text.muted"],
    "Accent":         ["colors.Accent", "accent.primary"],
    "AccentMuted":    ["colors.AccentMuted", "accent.muted"],
    "Warning":        ["colors.Warning", "status.warning"],
    "Danger":         ["colors.Danger", "status.error", "status.danger"],
    "Success":        ["colors.Success", "status.success"],
    "MeterInput":     ["colors.MeterInput", "meter.input"],
    "MeterOutput":    ["colors.MeterOutput", "meter.output"],
    "MeterReduction": ["colors.MeterReduction", "meter.reduction"],
    "RadiusSmall":     ["metrics.RadiusSmall"],
    "RadiusMedium":    ["metrics.RadiusMedium"],
    "RadiusLarge":     ["metrics.RadiusLarge"],
    "SpacingXSmall":   ["metrics.SpacingXSmall"],
    "SpacingSmall":    ["metrics.SpacingSmall"],
    "SpacingMedium":   ["metrics.SpacingMedium"],
    "SpacingLarge":    ["metrics.SpacingLarge"],
    "FontSizeTiny":    ["typography.FontSizeTiny"],
    "FontSizeSmall":   ["typography.FontSizeSmall"],
    "FontSizeBody":    ["typography.FontSizeBody"],
    "FontSizeTitle":   ["typography.FontSizeTitle"],
    "FontSizeDisplay": ["typography.FontSizeDisplay"],
}


def _get_path(data: dict, dotted: str):
    """Return the value at a dotted path, or None if absent."""
    node = data
    for part in dotted.split("."):
        if isinstance(node, dict) and part in node:
            node = node[part]
        else:
            return None
    return node


def _lookup(data: dict, token: str):
    """Find a raw value for a semantic token: try its own name (top-level and
    inside common groups), then any configured aliases."""
    candidates = [token, f"colors.{token}", f"metrics.{token}", f"typography.{token}"]
    candidates += ALIASES.get(token, [])
    for path in candidates:
        val = _get_path(data, path)
        if val is not None:
            return val
    return None


def _hex_to_rgba(s: str):
    s = s.strip().lstrip("#")
    if len(s) == 6:
        r, g, b = (int(s[i:i + 2], 16) for i in (0, 2, 4))
        a = 255
    elif len(s) == 8:
        r, g, b, a = (int(s[i:i + 2], 16) for i in (0, 2, 4, 6))
    else:
        raise ValueError(f"Unsupported hex color '#{s}'")
    return (r / 255.0, g / 255.0, b / 255.0, a / 255.0)


def parse_color(value):
    """Normalize any supported color spec into a float RGBA tuple in 0..1."""
    if isinstance(value, str):
        return _hex_to_rgba(value)

    if isinstance(value, (list, tuple)) and len(value) in (3, 4):
        nums = [float(x) for x in value]
        # Heuristic: if anything is > 1, treat the array as 0-255 ints.
        if max(nums) > 1.0:
            nums = [n / 255.0 for n in nums]
        if len(nums) == 3:
            nums.append(1.0)
        return tuple(max(0.0, min(1.0, n)) for n in nums)

    raise ValueError(f"Unsupported color value: {value!r}")


def fmt_float(v: float) -> str:
    s = f"{float(v):.6f}".rstrip("0")
    if s.endswith("."):
        s += "0"
    return s + "f"


def generate(src_path: str, out_path: str) -> None:
    with open(src_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    lines = []
    lines.append("// =============================================================================")
    lines.append("//  DefaultTheme.h  -- GENERATED FILE, DO NOT EDIT BY HAND.")
    lines.append("//")
    lines.append("//  Produced by tools/generate_theme_header.py from a design-time theme source.")
    lines.append("//  This is the ONLY theme source the plugin uses at runtime. The plugin never")
    lines.append("//  opens, reads or parses any JSON at runtime. Tokens are neutral semantic")
    lines.append("//  names only.")
    lines.append("// =============================================================================")
    lines.append("#pragma once")
    lines.append("")
    lines.append("namespace odf { namespace Theme {")
    lines.append("")
    lines.append("struct Color {")
    lines.append("  float r;")
    lines.append("  float g;")
    lines.append("  float b;")
    lines.append("  float a;")
    lines.append("};")
    lines.append("")

    # Colors
    for name, fallback in COLOR_TOKENS:
        raw = _lookup(data, name)
        if raw is None:
            rgba = fallback
            comment = "  // fallback (token missing from source)"
        else:
            try:
                rgba = parse_color(raw)
                comment = ""
            except ValueError:
                rgba = fallback
                comment = "  // fallback (unparseable source value)"
        vals = ", ".join(fmt_float(c) for c in rgba)
        lines.append(f"inline constexpr Color {name:<15}{{{vals}}};{comment}")

    lines.append("")

    # Scalars
    for name, fallback in FLOAT_TOKENS:
        raw = _lookup(data, name)
        if raw is None or not isinstance(raw, (int, float)):
            val = fallback
            comment = "  // fallback (token missing from source)"
        else:
            val = float(raw)
            comment = ""
        lines.append(f"inline constexpr float {name:<16}= {fmt_float(val)};{comment}")

    lines.append("")
    lines.append("} } // namespace odf::Theme")
    lines.append("")

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))

    print(f"[generate_theme_header] wrote {out_path} from {src_path}")


def main(argv):
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.dirname(here)
    src = argv[1] if len(argv) > 1 else os.path.join(repo, "src", "themes", "Default.json")
    out = argv[2] if len(argv) > 2 else os.path.join(repo, "src", "generated", "DefaultTheme.h")
    generate(src, out)


if __name__ == "__main__":
    main(sys.argv)
