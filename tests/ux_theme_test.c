/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ux_theme.c (ACID FM piece 3a): the spec's colours, the 256-entry blend, the gradient text, the glow, the band. */
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
#include "../firmware/src/gfx.c"
#include "../firmware/src/ux_theme.c"

static int fails;
static void ck(const char *what, int ok) { printf("ux_theme: %-70s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }
static uint16_t px(int32_t x, int32_t y) { return swap16(cv_px[(uint32_t)y * cv_w + (uint32_t)x]); }
static int near(uint16_t a, uint16_t b, int tol)
{
    static const uint8_t SH[3] = {11, 5, 0}, MK[3] = {31, 63, 31};
    for (int k = 0; k < 3; k++) {
        int d = (int)((a >> SH[k]) & MK[k]) - (int)((b >> SH[k]) & MK[k]);
        if (d > tol || d < -tol) return 0;
    }
    return 1;
}

static int dist(uint16_t a, uint16_t b)    /* the sum of the channels' distances, in 8-bit steps */
{
    int d = 0;
    for (int k = 0; k < 3; k++) {
        int sh = k == 0 ? 11 : k == 1 ? 5 : 0, mk = k == 1 ? 63 : 31, sc = k == 1 ? 4 : 8;
        int x = (int)((a >> sh) & mk) * sc - (int)((b >> sh) & mk) * sc;
        d += x < 0 ? -x : x;
    }
    return d;
}

int main(void)
{
    palette_set(UI_ACID_INDEX);
    ck("UX_BG #141518", UX_BG == RGB(0x14, 0x15, 0x18));
    ck("UX_ACID #B8F53A, UX_SLIDE #4CB8FF, UX_VIOLET #A64BFF", UX_ACID == RGB(0xB8, 0xF5, 0x3A) && UX_SLIDE == RGB(0x4C, 0xB8, 0xFF) && UX_VIOLET == RGB(0xA6, 0x4B, 0xFF));
    ck("UX_ACCENT #FF503C, UX_QUEUED #FFC83C", UX_ACCENT == RGB(0xFF, 0x50, 0x3C) && UX_QUEUED == RGB(0xFF, 0xC8, 0x3C));
    ck("UX_LINE UX_EMPTY UX_TEXT UX_MID UX_BG2", UX_LINE == RGB(0x30, 0x33, 0x38) && UX_EMPTY == RGB(0x26, 0x28, 0x2D)
       && UX_TEXT == RGB(0xE8, 0xEA, 0xED) && UX_MID == RGB(0x9A, 0xA0, 0xA6) && UX_BG2 == RGB(0x1A, 0x1C, 0x20));
    ck("UX_ACID_LT #DCFA9D, UX_VIOLET_LT #D3A5FF", UX_ACID_LT == RGB(0xDC, 0xFA, 0x9D) && UX_VIOLET_LT == RGB(0xD3, 0xA5, 0xFF));
    ck("blend ends: [0] ACID, [255] QUEUED", UX_BLEND[0] == UX_ACID && UX_BLEND[255] == UX_QUEUED);
    ck("blend hue order: [64] SLIDE, [128] VIOLET, [192] ACCENT (one step)",
       near(UX_BLEND[64], UX_SLIDE, 1) && near(UX_BLEND[128], UX_VIOLET, 1) && near(UX_BLEND[192], UX_ACCENT, 1));
    ck("ux_knob_col: ACID SLIDE VIOLET ACCENT", ux_knob_col(0) == UX_ACID && ux_knob_col(1) == UX_SLIDE
       && ux_knob_col(2) == UX_VIOLET && ux_knob_col(3) == UX_ACCENT);

    {   /* gradient text: ink from ACID (left) to QUEUED (right), the background untouched */
        int32_t x, y, l = -1, r = -1, bad = 0, w;
        uint16_t lc = 0, rc = 0;
        cv_begin(240, 60, UX_BG);
        ux_text_grad(10, 10, &UF_TTL, "ACID 303", UX_BG);
        w = text_w(&UF_TTL, "ACID 303");
        for (x = 0; x < 240; x++)
            for (y = 0; y < 60; y++) {
                uint16_t c = px(x, y);
                if (c != UX_BG) {
                    if (l < 0) { l = x; lc = c; }
                    r = x; rc = c;
                    if (x < 8 || x > 10 + w + 2 || y < 8) bad++;
                }
            }
        ck("text has ink inside its box only", l >= 0 && !bad);
        printf("ux_theme:   leftmost ink x%d %04x, rightmost x%d %04x\n", l, lc, r, rc);
        /* find the strongest (nearest-to-blend) ink on the leftmost and rightmost 6 columns */
        {
            int best_l = 9999, best_r = 9999;
            for (x = l; x < l + 6; x++) for (y = 0; y < 60; y++) { int d = dist(px(x, y), UX_ACID); if (d < best_l) best_l = d; }
            for (x = r - 5; x <= r; x++) for (y = 0; y < 60; y++) { int d = dist(px(x, y), UX_QUEUED); if (d < best_r) best_r = d; }
            ck("left ink within 40 (8-bit channel sum) of ACID", best_l <= 40);
            ck("right ink within 40 (8-bit channel sum) of QUEUED", best_r <= 40);
        }
        ck("pixel (0,0) and (239,59) untouched", px(0, 0) == UX_BG && px(239, 59) == UX_BG);
    }
    {   /* glow: the mock-up's: violet strongest at the centre (40 % / 30 %), fading through slide to nothing */
        uint16_t before[240 * 100];
        int32_t i, same = 1, mono = 1;
        uint32_t lv;
        cv_begin(240, 100, UX_BG);
        memcpy(before, cv_px, sizeof before);
        ux_glow(20, 10, 200, 80, 0);
        for (i = 0; i < 240 * 100; i++) same &= cv_px[i] == before[i];
        ck("ux_glow level 0 leaves the canvas untouched", same);
        for (lv = 1; lv <= 3u; lv++) {
            for (i = 1; i < (int32_t)UX_GLOW_N; i++)   /* each ring as far from BG as the one outside it, give or take RGB565 rounding (12 of 765) */
                mono &= dist(ux_glow_col(lv, (uint32_t)i), UX_BG) + 12 >= dist(ux_glow_col(lv, (uint32_t)i - 1u), UX_BG);
        }
        ck("ring colours get stronger toward the centre (level 1..3)", mono);
        ck("centre ring: more red (violet) than the outermost", (ux_glow_col(3, UX_GLOW_N - 1) >> 11) > (ux_glow_col(3, 0) >> 11));
        ck("centre ring is more violet (higher R) than the SLIDE-coloured middle ring", (ux_glow_col(3, UX_GLOW_N - 1) >> 11) > (ux_glow_col(3, UX_GLOW_N / 3) >> 11));
        ck("level 3 centre is the mock-up's 35 % violet over BG (one step)", near(ux_glow_col(3, UX_GLOW_N - 1), ux_mix(UX_BG, UX_VIOLET, 35), 1));
        ck("level 1 is fainter than level 3", ((ux_glow_col(1, UX_GLOW_N - 1) >> 11)) < (ux_glow_col(3, UX_GLOW_N - 1) >> 11));
        ux_glow(20, 10, 200, 80, 3);
        ck("level 3: centre (40 %, 30 %) is more red than a point near the rim", (px(100, 34) >> 11) > (px(30, 34) >> 11));
        ck("level 3: outside the box untouched", px(5, 50) == UX_BG && px(120, 95) == UX_BG && px(100, 5) == UX_BG);
        ck("level 3: a far corner of the box is untouched (transparent beyond 70 %)", px(21, 89) == UX_BG);
    }
    {   /* band and top line */
        cv_begin(240, 40, UX_BG);
        ux_band(10, 14);
        ux_topline();
        ck("band: left ACID, right QUEUED", px(0, 12) == UX_ACID && px(239, 12) == UX_QUEUED && px(120, 9) == UX_BG && px(120, 24) == UX_BG);
        ck("top line: 2 px at y 0..1, not y 2", px(0, 0) == UX_ACID && px(239, 1) == UX_QUEUED && px(100, 2) == UX_BG);
    }
    printf("ux_theme: %s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
