/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's blend, gradient text, glow and band (ux_theme.h). Included into gfx.c's unity build after it. */
#include "ux_theme.h"

static const uint16_t UX_BLEND[256] = {
#include "ux_blend.h"
};

static uint16_t ux_knob_col(uint32_t k)
{
    static const uint16_t C[4] = {UX_ACID, UX_SLIDE, UX_VIOLET, UX_ACCENT};
    return C[k & 3u];
}

/* The text is drawn as JIANT draws it (anti-aliased, cv_text_on) in a reference colour that contrasts with bg; each
 * pixel that matches one of that ramp's 15 ink steps is then recoloured: bg mixed toward UX_BLEND[column] by the
 * same weight. Chosen over a per-column colour in the text path (text_run's ramp is one table per run, and glyph
 * code is shared by every screen): this touches only this call's box and leaves cv_text alone. bg is the colour under
 * the text (a glow's local colour), so edge pixels blend with it. */
static void ux_text_grad(int32_t x, int32_t y, const aafont_t *f, const char *s, uint16_t bg)
{
    const uint16_t ref = bg == UX_TEXT ? UX_BG : UX_TEXT;
    int32_t w = text_w(f, s), x0 = x - 2, x1 = x + w + 2, y0 = y, y1 = y + f->h, px, py, a;
    uint16_t rv[16];
    const uint16_t *r;
    cv_text_on(x, y, f, s, ref, bg);
    r = ramp(ref, bg);
    for (a = 0; a < 16; a++)
        rv[a] = r[a];                                        /* (the cache may be reused below) */
    if (x0 < 0) x0 = 0;
    if (x1 > (int32_t)cv_w) x1 = (int32_t)cv_w;
    for (py = y0; py < y1; py++) {
        int32_t cy = py + cv_oy;
        if (cy < cv_cy0 || cy >= cv_cy1) continue;
        for (px = x0; px < x1; px++) {
            uint16_t *d = &cv_px[(uint32_t)cy * cv_w + (uint32_t)px];
            for (a = 1; a < 16; a++)
                if (*d == rv[a]) {
                    int32_t xi = w > 1 ? (px - x) * 255 / (w - 1) : 0;
                    if (xi < 0) xi = 0;
                    if (xi > 255) xi = 255;
                    *d = swap16(ux_mix(bg, UX_BLEND[xi], (int32_t)CURVE_DARK[a] * 100 / 256));
                    break;
                }
        }
    }
}

/* the panel glow: concentric filled rounded rects, outside in, each a little brighter; the colour runs VIOLET (outer) to
 * SLIDE (centre) over BG. level -> rings, and per ring the percent of colour over BG. No per-pixel maths. */
#define UX_GLOW_RINGS 8u
#define UX_GLOW_STEP 4           /* px between rings at most: the innermost ring (the lint's "cell") stays big enough for a title */
static const struct { uint8_t n, pct[UX_GLOW_RINGS]; } UX_GLOW[3] = {
    {4, {3, 6, 9, 12}},
    {6, {3, 6, 10, 15, 20, 25}},
    {8, {3, 6, 10, 15, 21, 28, 36, 45}},
};
static void ux_glow(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t level)
{
    uint32_t i;
    int32_t half = (w < h ? w : h) / 2;
    uint16_t under = UX_BG;
    if (level == 0 || level > 3u) return;
    for (i = 0; i < UX_GLOW[level - 1u].n; i++) {
        int32_t n = UX_GLOW[level - 1u].n, in = (int32_t)i * (half / n < UX_GLOW_STEP ? half / n : UX_GLOW_STEP);
        uint16_t c = ux_mix(UX_BG, ux_mix(UX_VIOLET, UX_SLIDE, (int32_t)i * 100 / (n - 1)), UX_GLOW[level - 1u].pct[i]);
        cv_rrect(x + in, y + in, w - 2 * in, h - 2 * in, 8, c, under);
        under = c;
    }
}

static void ux_band(int32_t y, int32_t h)
{
    int32_t x, n = (int32_t)cv_w;
    for (x = 0; x < n; x++)
        cv_rect(x, y, 1, h, UX_BLEND[n > 1 ? x * 255 / (n - 1) : 0]);
}

static void ux_topline(void)
{
    ux_band(0, 2);
}
