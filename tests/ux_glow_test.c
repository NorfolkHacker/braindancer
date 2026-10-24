/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ux_glow (ACID FM piece 3a): the glow fills each pixel at most once (no overlapping rings) and looks exactly like the
 * overlapped rings it replaced (a reference copy of that drawing below). */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define __attribute__(x)
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
static uint32_t span_calls, span_px;
static uint8_t wr[240 * 150];
static int32_t span_w0;
#define UX_HOOK_SPAN(x, y, n) (span_calls++, span_px += (n), ux_span_note((x), (y), (n)))
static void ux_span_note(int32_t x, int32_t y, int32_t n)
{
    for (int32_t i = 0; i < n; i++) wr[y * 240 + x + i]++;
}
#include "../firmware/src/gfx.c"
#include "../firmware/src/ux_theme.c"

static int fails;
static void ck(const char *what, int ok) { printf("ux_glow: %-76s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }

/* the previous drawing: UX_GLOW_N overlapping ellipses, outside in, a cv_rect row each */
static uint32_t ref_isqrt(uint32_t v)
{
    uint32_t r = 0, b = 1u << 30;
    while (b > v) b >>= 2;
    for (; b; b >>= 2) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
    }
    return r;
}
static uint32_t ref_rects, ref_px;
static void ref_glow(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t level)
{
    uint32_t i;
    int32_t cx = x + w * 40 / 100, cy = y + h * 30 / 100, ax = w * 594 / 1000, by = h * 693 / 1000;
    if (level == 0 || level > 3u || w < 2 || h < 2) return;
    for (i = 0; i < UX_GLOW_N; i++) {
        int32_t ra = ax * (int32_t)(UX_GLOW_N - i) / (int32_t)UX_GLOW_N, rb = by * (int32_t)(UX_GLOW_N - i) / (int32_t)UX_GLOW_N, dy;
        uint16_t c = ux_glow_col(level, i);
        if (rb < 1) continue;
        for (dy = -rb; dy <= rb; dy++) {
            int32_t row = cy + dy, hw, x0, x1;
            if (row < y || row >= y + h) continue;
            hw = (int32_t)ref_isqrt((uint32_t)ra * (uint32_t)ra * (uint32_t)(rb * rb - dy * dy) / (uint32_t)(rb * rb));
            x0 = cx - hw < x ? x : cx - hw;
            x1 = cx + hw + 1 > x + w ? x + w : cx + hw + 1;
            if (x1 > x0) { cv_rect(x0, row, x1 - x0, 1, c); ref_rects++; ref_px += (uint32_t)(x1 - x0); }
        }
    }
}

int main(void)
{
    static uint16_t want[240 * 150];
    int32_t sz[][4] = {{0, 0, 240, 150}, {0, 10, 240, 130}, {20, 10, 200, 80}, {5, 7, 111, 61}, {0, 0, 30, 20}, {0, 0, 2, 2}};
    palette_set(UI_ACID_INDEX);
    for (unsigned s = 0; s < sizeof sz / sizeof sz[0]; s++)
        for (uint32_t lv = 1; lv <= 3; lv++) {
            int same;
            char nm[80];
            cv_begin(240, 150, UX_BG);
            ref_glow(sz[s][0], sz[s][1], sz[s][2], sz[s][3], lv);
            memcpy(want, cv_px, sizeof want);
            cv_begin(240, 150, UX_BG);
            memset(wr, 0, sizeof wr);
            span_calls = span_px = 0;
            ux_glow(sz[s][0], sz[s][1], sz[s][2], sz[s][3], lv);
            same = !memcmp(want, cv_px, sizeof want);
            snprintf(nm, sizeof nm, "glow %dx%d at %d,%d level %u: pixels identical to the overlapped rings", sz[s][2], sz[s][3], sz[s][0], sz[s][1], lv);
            ck(nm, same);
        }
    /* the 240 x 150 level 3 glow: each pixel written at most once */
    ref_rects = ref_px = 0;
    cv_begin(240, 150, UX_BG);
    ref_glow(0, 0, 240, 150, 3);
    cv_begin(240, 150, UX_BG);
    memset(wr, 0, sizeof wr);
    span_calls = span_px = 0;
    ux_glow(0, 0, 240, 150, 3);
    {
        int mx = 0;
        for (int i = 0; i < 240 * 150; i++) if (wr[i] > mx) mx = wr[i];
        printf("ux_glow:   240x150 level 3: before %u rect calls, %u pixel writes; now %u spans, %u pixel writes (max %d per pixel)\n",
               ref_rects, ref_px, span_calls, span_px, mx);
        ck("some pixels written (the span hook is called)", span_px > 0);
        ck("no pixel written twice", mx <= 1);
        ck("writes <= w x h", span_px <= 240u * 150u);
        ck("far fewer writes than the overlapped rings", span_px * 4u < ref_px);
    }
    cv_begin(240, 150, UX_BG);
    memset(wr, 0, sizeof wr);
    span_calls = span_px = 0;
    ux_glow(0, 0, 240, 150, 0);
    ck("level 0 writes nothing", span_px == 0);
    {   /* (3a task 7, tweaks) VISUAL's rings: six track rings round the centre, all inside the 150-row panel with a margin; the
         * edges chosen from a table of radii (UX_RING_R), every pixel of the 240 x 150 panel written exactly once */
        static const uint8_t E[12] = {1, 2, 5, 6, 9, 10, 13, 14, 17, 18, 21, 22};   /* six rings */
        uint8_t es[12];
        uint16_t col[16], scr[16];
        uint32_t i, sent = 0, sym = 1, mx = 0, mn = 255, asc = 1;
        for (i = 0; i < 16u; i++) { col[i] = UX_BLEND[i * 16u]; scr[i] = ux_mix(UX_BG, UX_BLEND[i * 16u], 20); }
        for (i = 1; i < UX_RING_NR; i++) asc &= UX_RING_R[i] > UX_RING_R[i - 1u];
        ck("ring radii ascending; the table <= 2 KiB", asc && sizeof ux_rtab <= 2048u);
        ck("the six rings (swollen too) inside the panel's 75 rows, 5 px or more to spare", UX_RING_R[23] <= 70u);
        for (i = 0; i < 6u; i++)
            asc &= UX_RING_R[4u * i + 2u] - UX_RING_R[4u * i + 1u] == 7u && UX_RING_R[4u * i] + 1u == UX_RING_R[4u * i + 1u] &&
                   UX_RING_R[4u * i + 3u] == UX_RING_R[4u * i + 2u] + 1u && (i == 0u || UX_RING_R[4u * i + 1u] > UX_RING_R[4u * i - 1u]);
        ck("each ring 7 px, its swell a px each side, a gap after it", asc);
        cv_begin(240, 150, UX_BG);
        for (i = 0; i < 240u * 150u; i++) cv_px[i] = 0x1234;
        cv_begin_raw(240, 150, UX_BG);
        for (i = 0; i < 240u * 150u; i++) sent += cv_px[i] == 0x1234;
        ck("cv_begin_raw: the canvas set up, nothing filled", sent == 240u * 150u && cv_w == 240 && cv_h == 150);
        memset(wr, 0, sizeof wr);
        span_calls = span_px = 0;
        ux_rings(E, 12, col, scr, 100, 120);
        for (i = 0; i < 240u * 150u; i++) { if (wr[i] > mx) mx = wr[i]; if (wr[i] < mn) mn = wr[i]; }
        sent = 0;
        for (i = 0; i < 240u * 150u; i++) sent += cv_px[i] == 0x1234;
        printf("ux_rings:  240x150: %u spans, %u pixel writes (min %u max %u per pixel)\n", span_calls, span_px, mn, mx);
        ck("rings: every pixel written exactly once (36000 writes)", mn == 1 && mx == 1 && span_px == 240u * 150u && !sent);
#define RPX(r) swap16(cv_px[75u * 240u + 120u + (r)])
        ck("rings: the segments by radius (centre, ring 0 at 7..13, its gap, ring 5 at 62..68, past it one segment)",
           RPX(3) == col[0] && RPX(7) == col[1] && RPX(13) == col[1] && RPX(14) == col[2] && RPX(62) == col[11] &&
           RPX(68) == col[11] && RPX(69) == col[12] && RPX(90) == col[12] && RPX(119) == col[12]);
        ck("rings: the panel's corners past every ring (the last segment)", swap16(cv_px[0]) == col[12] &&
           swap16(cv_px[149u * 240u + 239u]) == col[12]);
        ck("rings: rows 100..119 the scrim's colours (ring 2 at 125,110; a gap at 125,99 outside)",
           swap16(cv_px[110u * 240u + 125u]) == scr[5] && swap16(cv_px[99u * 240u + 125u]) == col[4]);
        for (i = 0; i < 120u; i++) {
            uint32_t y;
            for (y = 0; y < 150u; y++) sym &= cv_px[y * 240u + 120u + i] == cv_px[y * 240u + 119u - i];
        }
        for (i = 0; i < 25u; i++) sym &= cv_px[(75u + i) * 240u + 200u] == cv_px[(74u - i) * 240u + 200u];
        ck("rings: symmetric about the panel's centre (left / right, up / down)", sym);
        memcpy(es, E, sizeof es);
        es[0] = 0; es[1] = 3;                                       /* ring 0 swollen: 6 .. 14 */
        cv_begin_raw(240, 150, UX_BG);
        memset(wr, 0, sizeof wr);
        ux_rings(es, 12, col, scr, 0, 0);
        mx = 0; mn = 255;
        for (i = 0; i < 240u * 150u; i++) { if (wr[i] > mx) mx = wr[i]; if (wr[i] < mn) mn = wr[i]; }
        ck("rings: a ring swollen a px each side (its edges the table's neighbours), still each pixel once",
           RPX(6) == col[1] && RPX(14) == col[1] && RPX(15) == col[2] && RPX(5) == col[0] && mn == 1 && mx == 1);
    }
    return fails ? 1 : 0;
}
