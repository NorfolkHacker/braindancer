/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker
 * JIANT (FELUCCA TONIC): JIANT FM's organic line art, drawn by code (docs/TONIC-UI.md). */
/* Everything is drawn into the graph canvas (gfx.c cv_px) with integer maths: no stored artwork. Positions in
 * 1/16 px (Q4) so curves move smoothly between pixels; lines antialiased (two pixels across, blended over what
 * is there). Angles are 16-bit turns (0x10000 = 360 degrees), through dsp.c's sine table.
 *   og_line     an antialiased line (Q4 ends)
 *   og_ring, og_node    a circle; a dot with a soft rim
 *   bg_energy   the output's level now (what FX's lens moves with; ui_being.c's until ACID FM 3b task 5)
 * (ACID FM 3b task 5: the plates, shapes and illustrations (og_path, og_plate, og_ill, og_dash, og_stipple, og_label: the
 * beings' and creatures' line art) gone with the creatures, the user's "get rid of any creatures"; task 7: og_quad_a, the LFO
 * MOD net's curves) */
static int og_neutral(uint16_t c) { return (c >> 11) == (c & 31u) && ((c >> 5) & 63u) >> 1 == (c >> 11); }
/* c over the canvas pixel at (x, y) with coverage a (0..256) */
static inline void og_plot(int32_t x, int32_t y, uint16_t c, int32_t a)
{
    uint16_t *p, o;
    uint32_t r, g, b;
    y += cv_oy;
    if (a <= 0 || (uint32_t)x >= cv_w || (uint32_t)y >= cv_h)
        return;
    p = &cv_px[(uint32_t)y * cv_w + (uint32_t)x];
    if (a >= 256) {
        *p = swap16(c);
        return;
    }
    o = swap16(*p);
    r = (o >> 11) + ((((int32_t)(c >> 11) - (int32_t)(o >> 11)) * a) >> 8);
    if (og_neutral(o) && og_neutral(c)) {                /* grey over grey stays grey */
        *p = swap16((uint16_t)(r << 11 | r << 6 | r));
        return;
    }
    g = ((o >> 5) & 63u) + ((((int32_t)((c >> 5) & 63u) - (int32_t)((o >> 5) & 63u)) * a) >> 8);
    b = (o & 31u) + ((((int32_t)(c & 31u) - (int32_t)(o & 31u)) * a) >> 8);
    *p = swap16((uint16_t)(r << 11 | g << 5 | b));
}
/* an antialiased line between Q4 points, at strength a (0..256) */
static void og_line_a(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c, int32_t a)
{
    int32_t dx = x1 - x0, dy = y1 - y0, adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy, t, g, p, e, f;
    if (adx >= ady) {                                    /* along x: y in Q8, two pixels per column */
        if (x0 > x1) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        g = adx ? ((y1 - y0) << 8) / (x1 - x0) : 0;      /* dy / dx, Q8 */
        e = x1 >> 4;
        for (p = x0 >> 4; p <= e && p - (x0 >> 4) < 480; p++) {
            int32_t y = (y0 << 4) + g * (p - (x0 >> 4)) - 128;   /* (pixel centres) */
            f = y & 255;
            og_plot(p, y >> 8, c, ((256 - f) * a) >> 8);
            og_plot(p, (y >> 8) + 1, c, (f * a) >> 8);
        }
    } else {
        if (y0 > y1) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        g = ((x1 - x0) << 8) / (y1 - y0);
        e = y1 >> 4;
        for (p = y0 >> 4; p <= e && p - (y0 >> 4) < 480; p++) {
            int32_t x = (x0 << 4) + g * (p - (y0 >> 4)) - 128;
            f = x & 255;
            og_plot(x >> 8, p, c, ((256 - f) * a) >> 8);
            og_plot((x >> 8) + 1, p, c, (f * a) >> 8);
        }
    }
}
static void og_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c) { og_line_a(x0, y0, x1, y1, c, 256); }

/* cos / sin of a 16-bit angle, Q15 */
static int32_t og_cos(uint32_t ang) { return sine_i((ang + 0x4000u) << 16); }
static int32_t og_sin(uint32_t ang) { return sine_i(ang << 16); }
/* (JIANT 0.5, ui_being.c's until ACID FM 3b task 5, which took the beings away) a sine of a 16-bit turn, Q15 (FX's lens), and
 * the output's level now, 0..256, slewed (fast up, slow down): what FX's lens moves with */
static int32_t bg_sin(uint32_t ang) { return sine_i(ang << 16); }
static int32_t bg_energy(void)
{
    static int32_t e;
    static uint32_t fr = 0xFFFFFFFFu;
    uint32_t w = scope_w, i;
    int32_t pk = 0, v;
    if (fr == ui.frame)                                 /* (once a frame: a redraw in the same frame draws the same) */
        return e;
    fr = ui.frame;
    for (i = 1; i <= 256u; i += 2u) {
        v = scope_buf[(w - i) & (SCOPE_N - 1u)];
        if (v < 0) v = -v;
        if (v > pk) pk = v;
    }
    v = pk * 256 / 12000;
    if (v > 256) v = 256;
    e += v > e ? (v - e) * 3 / 4 : (v - e) / 6;
    return e;
}

/* a circle of radius r (Q4) in 20 lines */
static void og_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t px = cx + r, py = cy;
    uint32_t i;
    for (i = 1; i <= 20u; i++) {
        uint32_t ang = i * (1u << 16) / 20u;
        int32_t x = cx + ((og_cos(ang) * r) >> 15), y = cy + ((og_sin(ang) * r) >> 15);
        og_line(px, py, x, y, c);
        px = x;
        py = y;
    }
}
/* a filled dot of radius r px (pixel centre (cx, cy) in Q4) with an antialiased rim */
static void og_node(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t x0 = cx >> 4, y0 = cy >> 4, i, j;
    for (j = -r; j <= r; j++)
        for (i = -r; i <= r; i++) {
            int32_t d = i * i + j * j - r * r;          /* inside: d <= 0; the rim: 0 .. 2r */
            if (d <= -r)
                og_plot(x0 + i, y0 + j, c, 256);
            else if (d < r)
                og_plot(x0 + i, y0 + j, c, 128 - d * 128 / r);
        }
}
