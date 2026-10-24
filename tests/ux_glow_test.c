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
    return fails ? 1 : 0;
}
