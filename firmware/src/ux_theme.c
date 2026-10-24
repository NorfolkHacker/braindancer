/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's blend, gradient text, glow and band (ux_theme.h). Included into gfx.c's unity build after it. */
#include "ux_theme.h"

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

/* the panel glow: UX_GLOW_N filled ellipses, outside in, centred at 40 % / 30 % of the panel, each a little stronger. The
 * mock-up's gradient runs to 70 % of its farthest-corner radius (0.6 w sqrt 2, 0.7 h sqrt 2), so the outer ellipse has
 * half-axes 0.594 w x 0.693 h. A ring's colour, from u = its radius as a percent of the gradient radius (0..70): alpha
 * 35 % at the centre to 12 % at u 45 to 0 at u 70, the hue VIOLET at the centre to SLIDE at u 45; level scales alpha.
 * Integer arithmetic once per ring; spans are cv_rect rows (unhooked: the glow is no lint cell). */
static uint16_t ux_glow_col(uint32_t level, uint32_t ring)
{
    int32_t u = (int32_t)(2u * (UX_GLOW_N - ring) - 1u) * 70 / (int32_t)(2u * UX_GLOW_N);   /* the ring's middle */
    int32_t a = u <= 45 ? 35 - 23 * u / 45 : 12 * (70 - u) / 25;
    int32_t hue = u >= 45 ? 100 : u * 100 / 45;
    if (level > 3u) level = 3u;
    return ux_mix(UX_BG, ux_mix(UX_VIOLET, UX_SLIDE, hue), a * (int32_t)level * 10 / 30);
}

static uint32_t ux_isqrt(uint32_t v)
{
    uint32_t r = 0, b = 1u << 30;
    while (b > v) b >>= 2;
    for (; b; b >>= 2) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
    }
    return r;
}

/* The rings nest (each ellipse inside the one before and the innermost colour wins), so the glow is drawn as the
 * pieces that are left: per row, ring i owns the span between its edge and ring i + 1's, one on each side of the
 * centre, the innermost ring the middle. Every pixel is written once. The edges (half-widths of each ring at each
 * |dy|, the old isqrt rows) are a table of UX_GLOW_N x UX_GLOW_ROWS bytes (2 KiB) built when the panel's size changes. */
#define UX_GLOW_ROWS 105u        /* |dy| 0 .. 104: 0.693 x the strip's 150 rows */
static uint8_t ux_gtab[UX_GLOW_ROWS * UX_GLOW_N] __attribute__((section(".pool")));   /* hw + 1; 0: the ring is not on this row */
static int16_t ux_gw __attribute__((section(".pool"))), ux_gh __attribute__((section(".pool")));
#ifndef UX_HOOK_SPAN
#define UX_HOOK_SPAN(x, y, n) ((void)0)       /* (the host test counts the spans) */
#endif

static void ux_glow_tab(int32_t w, int32_t h)
{
    int32_t ax = w * 594 / 1000, by = h * 693 / 1000, i, dy;
    for (i = 0; i < (int32_t)UX_GLOW_N; i++) {
        int32_t ra = ax * (int32_t)(UX_GLOW_N - (uint32_t)i) / (int32_t)UX_GLOW_N, rb = by * (int32_t)(UX_GLOW_N - (uint32_t)i) / (int32_t)UX_GLOW_N;
        for (dy = 0; dy < (int32_t)UX_GLOW_ROWS; dy++) {
            uint32_t hw = 0;
            if (rb >= 1 && dy <= rb)
                hw = 1u + ux_isqrt((uint32_t)ra * (uint32_t)ra * (uint32_t)(rb * rb - dy * dy) / (uint32_t)(rb * rb));
            ux_gtab[(uint32_t)dy * UX_GLOW_N + (uint32_t)i] = (uint8_t)hw;
        }
    }
    ux_gw = (int16_t)w;
    ux_gh = (int16_t)h;
}

static void ux_gspan(int32_t x0, int32_t x1, int32_t row, int32_t xmin, int32_t xmax, uint16_t sc)   /* [x0, x1] clipped */
{
    int32_t k;
    uint16_t *d;
    if (x0 < xmin) x0 = xmin;
    if (x1 >= xmax) x1 = xmax - 1;
    if (x1 < x0) return;
    UX_HOOK_SPAN(x0, row, x1 - x0 + 1);
    d = &cv_px[(uint32_t)row * cv_w];
    for (k = x0; k <= x1; k++)
        d[k] = sc;
}

static void ux_glow(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t level)
{
    uint16_t sc[UX_GLOW_N];
    uint32_t i;
    int32_t cx = x + w * 40 / 100, cy = y + h * 30 / 100, by = h * 693 / 1000, dy, xmin = x < 0 ? 0 : x, xmax = x + w;
    if (level == 0 || level > 3u || w < 2 || h < 2 || by >= (int32_t)UX_GLOW_ROWS) return;
    if (w != ux_gw || h != ux_gh) ux_glow_tab(w, h);
    for (i = 0; i < UX_GLOW_N; i++)
        sc[i] = swap16(ux_glow_col(level, i));
    if (xmax > (int32_t)cv_w) xmax = (int32_t)cv_w;
    for (dy = -by; dy <= by; dy++) {
        int32_t row = cy + dy, r = row + cv_oy;
        const uint8_t *t = &ux_gtab[(uint32_t)(dy < 0 ? -dy : dy) * UX_GLOW_N];
        if (row < y || row >= y + h || r < 0 || r >= (int32_t)cv_h) continue;
        for (i = 0; i < UX_GLOW_N; i++) {
            int32_t o = (int32_t)t[i] - 1, in = i + 1u < UX_GLOW_N ? (int32_t)t[i + 1u] - 1 : -1;
            if (o < 0) continue;
            if (in < 0)
                ux_gspan(cx - o, cx + o, r, xmin, xmax, sc[i]);          /* the innermost on this row: all of it */
            else {
                ux_gspan(cx - o, cx - in - 1, r, xmin, xmax, sc[i]);
                ux_gspan(cx + in + 1, cx + o, r, xmin, xmax, sc[i]);
            }
        }
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
