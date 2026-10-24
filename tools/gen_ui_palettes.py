#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
# Portions Copyright (C) 2026 NorfolkHacker
"""The UI palette (run by tools/build.py generate()).

  gen_ui_palettes.py OUT.h [--report]

ACID FM 0.3: one palette, ACID (docs/superpowers/specs/2026-10-24-acid-fm-ui-design.md 3.1), in place of JIANT's eleven
(GREY GREEN AMBER ICE VIOLET ROSE PAPER HI-CON NIGHT MONO JIANT): every saved palette id, of any earlier firmware, loads
as ACID (src/gfx.c palette_from_stored). A palette is five colours: BG (background), SURF (surface: the spec's BG2;
the one skin, AIR's look, draws no boxes, so SURF on the screen is BG and the stored SURF only feeds RAISE), TEXT,
THEME (identity colour: values, curves, gauges, selection; the spec's ACID) and ACCENT (the one active thing: hot
knob, cursor, playhead, a sounding step). The firmware derives the rest with fixed blends at palette_set() time
(src/gfx.c, the same integer maths as mix() here), the percentages chosen so the derived tokens are the spec's
(within one RGB565 step a channel; tests/theme_test.c checks it):
  MID  = mix(BG, TEXT, 66 %)    labels, units, secondary text (the spec's MID #9AA0A6)
  DIM  = mix(BG, TEXT, 42 %)    inactive, empty steps, disabled, the dividers (T_RULE)
  LINE = mix(BG, TEXT, 14 %)    1 px dividers, the faint gauge track (the spec's LINE #303338)
  SEL  = mix(BG, THEME, 80 %)   selection fill and gauge fill (the "mid" of the theme)
  TINT = mix(BG, THEME, 12 %)   a faint area (the selected drum lane)
  RAISE = mix(SURF, TEXT, 5 %)  a raised area: empty step stubs, guides, fader slots, button wells, chips (the spec's
                                EMPTY #26282D)
  KEY  = mix(BG, TEXT, 78 %)    a keycap's fill (a mid-light cushion; its label INK, unavailable: a DIM fill)
  INK  = BG: text on a selection or THEME fill
KNOB 1..4: the spec's knob colours, ACID SLIDE VIOLET ACCENT. Outside the palette: REC, a fixed red; the QR code's
fixed black and white; the crash screen's fixed red.
Saved settings name palettes by a tagged id (PAL_TAG + index, JIANT's 0..10); older firmware saved the index into
its 20 palettes. All of them read as ACID.
--report prints the WCAG contrast of every pairing. The tool fails (exit 1) on any miss.
"""
import argparse
import sys
from pathlib import Path

# name, bg, surf, text, theme, accent (8-bit RGB; stored as RGB565): the spec's BG, BG2, TEXT, ACID, ACCENT
PALETTES = [
    ("ACID",   (0x14, 0x15, 0x18), (0x1A, 0x1C, 0x20), (0xE8, 0xEA, 0xED), (0xB8, 0xF5, 0x3A), (0xFF, 0x50, 0x3C)),
]
# KNOB 1..4's colours (the cards' values and gauges, the EDIT chips, the knob row): the spec's ACID SLIDE VIOLET ACCENT
KNOBS = {
    "ACID":   [(0xB8, 0xF5, 0x3A), (0x4C, 0xB8, 0xFF), (0xA6, 0x4B, 0xFF), (0xFF, 0x50, 0x3C)],
}
PAL_TAG = 64                       # stored id = PAL_TAG + index; below 20: an old id (every one reads as ACID)
REC = (255, 72, 72)
QR_LIGHT, QR_DARK, CRASH_BG, CRASH_INK = (255, 255, 255), (0, 0, 0), (160, 0, 0), (255, 255, 255)
PCT = {"MID": 66, "DIM": 42, "LINE": 14, "SEL": 80, "TINT": 12, "KEY": 78}
RAISE_PCT = 5                      # SURF -> TEXT


def to565(c):
    r, g, b = c
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def from565(v):
    return ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31)


def cdiv(a, b):
    return a // b if a >= 0 else -((-a) // b)


def mix(a, b, pct):
    """src/gfx.c ux_mix: a + (b - a) * pct / 100 per channel, rounded"""
    out = 0
    for sh, mask in ((11, 31), (5, 63), (0, 31)):
        x, y = (a >> sh) & mask, (b >> sh) & mask
        d = (y - x) * pct
        out |= (x + cdiv(d + (50 if d >= 0 else -50), 100)) << sh
    return out


def lum(c565):
    def lin(v):
        v /= 255.0
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
    r, g, b = from565(c565)
    return 0.2126 * lin(r) + 0.7152 * lin(g) + 0.0722 * lin(b)


def contrast(a, b):
    la, lb = lum(a), lum(b)
    return (max(la, lb) + 0.05) / (min(la, lb) + 0.05)


def derive(p):
    """the tokens as src/gfx.c palette_set makes them (the one skin: SURF on the screen is BG)"""
    name, *cols = p
    bg, surf, text, theme, accent = (to565(c) for c in cols)
    d = dict(bg=bg, surf=bg, text=text, theme=theme, accent=accent,
             mid=mix(bg, text, PCT["MID"]), dim=mix(bg, text, PCT["DIM"]),
             line=mix(bg, text, PCT["LINE"]), sel=mix(bg, theme, PCT["SEL"]),
             tint=mix(bg, theme, PCT["TINT"]), raise_=mix(surf, text, RAISE_PCT),
             key=mix(bg, text, PCT["KEY"]), ink=bg, rec=to565(REC))
    for i, k in enumerate(KNOBS[name]):
        d[f"k{i}"] = to565(k)
    return name, d


# (foreground, background, minimum contrast, why). KNOB 3 (VIOLET #A64BFF) is 4.3:1 on BG, 3.5:1 on RAISE: the knobs' values and gauges
# at 4.0, on a RAISE chip 3.5 (they sit beside their label in TEXT / MID; the spec's colours, as they are)
CHECKS = [("text", "bg", 12.0, "body text"), ("text", "surf", 9.0, "body text on a surface"),
          ("mid", "bg", 5.0, "labels"), ("mid", "surf", 4.0, "labels on a surface"),
          ("dim", "bg", 2.2, "inactive (not read)"), ("line", "bg", 1.25, "1 px dividers"),
          ("theme", "bg", 4.5, "values, curves"), ("theme", "surf", 4.5, "values on a surface"),
          ("accent", "bg", 4.5, "hot value, cursor, playhead"), ("accent", "surf", 4.5, "hot on a surface"),
          ("ink", "sel", 4.5, "text on a selection"), ("ink", "theme", 4.5, "text on a THEME fill"),
          ("sel", "bg", 1.8, "gauge fill vs track"), ("rec", "bg", 3.0, "REC (graphic)"),
          ("text", "raise_", 7.0, "text on a RAISE chip or button"), ("ink", "rec", 4.5, "text on the REC chip"),
          ("ink", "key", 4.5, "a keycap's label"), ("key", "bg", 3.0, "a keycap on the background"),
          ("key", "surf", 2.5, "a keycap on a surface"), ("ink", "dim", 1.8, "an unavailable keycap's label"),
          ("ink", "accent", 4.5, "text on the ARM badge")] + \
         [(f"k{i}", b, 4.0 if b != "raise_" else 3.5, f"KNOB {i + 1}'s value") for i in range(4) for b in ("bg", "surf", "raise_")]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out", nargs="?")
    ap.add_argument("--report", action="store_true")
    a = ap.parse_args()
    fails = []
    assert len(PALETTES) == 1 and PALETTES[0][0] == "ACID" and set(KNOBS) == {"ACID"}
    rows = [derive(p) for p in PALETTES]
    for name, d in rows:
        for f, b, lim, why in CHECKS:
            c = contrast(d[f], d[b])
            if c < lim:
                fails.append(f"{name}: {f}/{b} {c:.2f} < {lim} ({why})")
        if contrast(to565(CRASH_INK), to565(CRASH_BG)) < 4.5 or contrast(to565(QR_LIGHT), to565(QR_DARK)) < 15.0:
            fails.append("the crash screen's or the QR code's colours")
    if a.report:
        print(f"{'palette':8s} " + " ".join(f"{f}/{b}".ljust(12) for f, b, _, _ in CHECKS))
        for name, d in rows:
            print(f"{name:8s} " + " ".join(f"{contrast(d[f], d[b]):5.1f}".ljust(12) for f, b, _, _ in CHECKS))
        print("minimum  " + " ".join(f"{lim:<12.2f}" for _, _, lim, _ in CHECKS))
    if a.out:
        out = ["/* generated by tools/gen_ui_palettes.py: the UI palette (ACID FM 0.3: one, ACID; 5 colours, the rest derived) */",
               "#pragma once", "#include <stdint.h>", "",
               "/* ui_pal_t {name, bg, surf, text, theme, accent} (src/gfx.c) */",
               "static const ui_pal_t UI_PALETTES[] = {"]
        for p in PALETTES:
            out.append(f'    {{"{p[0]}", ' + ", ".join(f"0x{to565(c):04x}" for c in p[1:]) + "},")
        out += ["};", "/* KNOB 1..4's colours: ACID SLIDE VIOLET ACCENT */", "static const uint16_t UI_KNOBS[][4] = {"]
        out += [f'    {{' + ", ".join(f"0x{to565(c):04x}" for c in KNOBS[p[0]]) + f'}},   /* {p[0]} */' for p in PALETTES]
        out += ["};", f"#define UI_NPALETTES {len(PALETTES)}u",
                "#define UI_ACID_INDEX 0u          /* ACID: the only palette */",
                f"#define UI_PAL_TAG {PAL_TAG}u          /* stored id = UI_PAL_TAG + index; below 20: an old id (all ACID) */"]
        out += [f"#define UI_{k}_PCT {v}" for k, v in PCT.items()] + [f"#define UI_RAISE_PCT {RAISE_PCT}   /* SURF -> TEXT */"]
        out += [f"#define UI_REC_DARK 0x{to565(REC):04x}u",
                f"#define UI_QR_LIGHT 0x{to565(QR_LIGHT):04x}u", f"#define UI_QR_DARK 0x{to565(QR_DARK):04x}u",
                f"#define UI_CRASH_BG 0x{to565(CRASH_BG):04x}u", f"#define UI_CRASH_INK 0x{to565(CRASH_INK):04x}u", ""]
        Path(a.out).write_text("\n".join(out))
    for f in fails:
        print("FAIL", f)
    if fails:
        sys.exit(1)


if __name__ == "__main__":
    main()
