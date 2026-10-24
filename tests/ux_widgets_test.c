/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ux_widgets.c (ACID FM piece 3a): the screen's rows sum to 240, each piece lands on its rows, the step strip's colours. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define __attribute__(x)
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }
static void lcd_wake_now(void) {}
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
static uint32_t bl_n, bl_y[64], bl_h[64], bl_w[64];
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)p; if (bl_n < 64) { bl_y[bl_n] = y; bl_h[bl_n] = h; bl_w[bl_n] = w; bl_n++; } }
#include "../firmware/src/gfx.c"
#include "../firmware/src/ux_theme.c"
#include "../firmware/src/core.h"
#include "../firmware/src/ux_widgets.c"

static int fails;
static void ck(const char *what, int ok) { printf("ux_widgets: %-78s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }
static uint16_t px(int32_t x, int32_t y) { return swap16(cv_px[(uint32_t)y * cv_w + (uint32_t)x]); }
static uint32_t ink_n(uint16_t bg)       /* pixels that are not bg */
{
    uint32_t n = 0, i;
    for (i = 0; i < cv_w * cv_h; i++) n += swap16(cv_px[i]) != bg;
    return n;
}
static int blitted(uint32_t y, uint32_t h)
{
    for (uint32_t i = 0; i < bl_n; i++) if (bl_y[i] == y && bl_h[i] >= 1 && bl_y[i] + bl_h[i] <= y + h) return 1;
    return 0;
}

int main(void)
{
    palette_set(UI_ACID_INDEX);
    ck("UX_H_* sum to 240", UX_H_LINE + UX_H_HEAD + UX_H_KNOBS + UX_H_PANEL + UX_H_GAP + UX_H_FOOT == 240u);
    ck("UX_Y_* follow each other with no overlap",
       UX_Y_LINE == 0 && UX_Y_HEAD == UX_Y_LINE + UX_H_LINE && UX_Y_KNOBS == UX_Y_HEAD + UX_H_HEAD
       && UX_Y_PANEL == UX_Y_KNOBS + UX_H_KNOBS && UX_Y_GAP == UX_Y_PANEL + UX_H_PANEL && UX_Y_FOOT == UX_Y_GAP + UX_H_GAP
       && UX_Y_FOOT + UX_H_FOOT == 240u);
    ck("the panel is the canvas strip (240 x 150)", UX_H_PANEL * 240u == CV_MAX);

    {   /* the step strip: note, accent, slide, rest, tie (after the slide), note ; edit 5, play 0 */
        step_t st[16];
        int32_t cy = 20, mid = UX_STRIP_H / 2;
        memset(st, 0, sizeof st);
        st[0].n = 1; st[0].time = ST_NOTE;
        st[1].n = 1; st[1].time = ST_NOTE; st[1].flags = SF_ACCENT;
        st[2].n = 1; st[2].time = ST_NOTE; st[2].flags = SF_SLIDE;
        st[3].time = ST_REST;
        st[4].n = 1; st[4].time = ST_NOTE;
        st[5].time = ST_TIE;
        st[6].time = ST_TIE;
        st[7].n = 1; st[7].time = ST_NOTE;
        cv_begin(240, 150, UX_BG);
        ux_strip(cy, st, 8, 7, 0);
#define CELL(i) px(UX_STRIP_X + (i) * UX_STRIP_PITCH + UX_STRIP_W / 2, cy + mid)
        ck("strip: a play cell is its note's colour lightened (not the plain ACID)", CELL(0) != UX_ACID && CELL(0) != UX_EMPTY);
        ck("strip: accent is ACCENT", CELL(1) == UX_ACCENT);
        ck("strip: slide is SLIDE", CELL(2) == UX_SLIDE);
        ck("strip: a rest is EMPTY", CELL(3) == UX_EMPTY);
        ck("strip: a note is ACID", CELL(4) == UX_ACID);
        ck("strip: a tie is the note's colour dimmed", CELL(5) == ux_mix(UX_ACID, UX_BG, 45) && CELL(5) != UX_ACID);
        ck("strip: a held run keeps one colour", CELL(6) == CELL(5));
        ck("strip: the edit cell is outlined TEXT, its middle is not",
           px(UX_STRIP_X + 7 * UX_STRIP_PITCH, cy) == UX_TEXT && px(UX_STRIP_X + 7 * UX_STRIP_PITCH + UX_STRIP_W - 1, cy + UX_STRIP_H - 1) == UX_TEXT
           && CELL(7) == UX_ACID);
        ck("strip: cells past the pattern are dim (BG2)", CELL(9) == UX_BG2);
        ck("strip: 14 px pitch leaves a gap of BG between cells", px(UX_STRIP_X + UX_STRIP_W, cy + mid) == UX_BG);
        ck("strip: nothing above or below it", px(UX_STRIP_X + 3, cy - 1) == UX_BG && px(UX_STRIP_X + 3, cy + UX_STRIP_H) == UX_BG);
    }

    dc_frame(1);
    {   /* header */
        bl_n = 0;
        ux_header(3, "ACID 303", "128 BPM");
        ck("header: one strip 240 x 16 at LCD y 0", bl_n == 1 && bl_y[0] == 0 && bl_h[0] == UX_H_LINE + UX_H_HEAD && bl_w[0] == 240);
        ck("header: the blend line on rows 0 and 1", px(0, 0) == UX_ACID && px(239, 1) == UX_QUEUED && px(100, 2) == UX_BG);
        ck("header: text drawn", ink_n(UX_BG) > 240u * 2u + 150u);
    }
    {   /* knobs */
        ux_knob_t k[4] = {{"CUTOFF", "42", 42}, {"RESO", "110", 200}, {"ENV MOD", "88", 128}, {"DECAY", "31", 255}};
        bl_n = 0;
        ux_knobs(k, 1);
        ck("knobs: one strip 240 x 40 at LCD y 16", bl_n == 1 && bl_y[0] == UX_Y_KNOBS && bl_h[0] == UX_H_KNOBS);
        ck("knobs: the hot column sits on BG2, the others on BG",
           px(UX_COL_X0 + UX_COL_PITCH + UX_COL_W - 3, 10) == UX_BG2 && px(UX_COL_X0 + UX_COL_W - 3, 10) == UX_BG && px(UX_COL_X0 + 2 * UX_COL_PITCH + UX_COL_W - 3, 10) == UX_BG);
        ck("knobs: the hot column's bar is its knob's colour (SLIDE), the end of an unfilled bar LINE",
           px(UX_COL_X0 + UX_COL_PITCH + 4, 33) == UX_SLIDE && px(UX_COL_X0 + UX_COL_W - 4, 33) == UX_LINE);
        ck("knobs: a full bar reaches the column's end", px(UX_COL_X0 + 3 * UX_COL_PITCH + UX_COL_W - 4, 33) != UX_LINE);
        ck("knobs: a cool column's bar is its colour dimmed", px(UX_COL_X0 + 4, 33) == ux_mix(UX_ACID, UX_BG, 35));
        ux_knobs(k, -1);
        ck("knobs: no hot column: every column on BG", px(UX_COL_X0 + UX_COL_PITCH + UX_COL_W - 3, 10) == UX_BG);
    }
    {   /* panel */
        bl_n = 0;
        ux_panel_begin(3);
        ck("panel: 240 x 150 canvas, BG in the corner, glow in the middle", cv_w == 240 && cv_h == 150 && px(239, 149) == UX_BG && px(96, 45) != UX_BG);
        ux_panel_end();
        ck("panel: sent at LCD y 56", bl_n == 1 && bl_y[0] == UX_Y_PANEL && bl_h[0] == UX_H_PANEL);
        ux_panel_begin(0);
        ck("panel: glow level 0 is plain BG", ink_n(UX_BG) == 0);
    }
    {   /* footer */
        bl_n = 0;
        ux_footer("HOME \xB7 TRACKS", "LVL \xB7 LVL \xB7 LVL \xB7 LVL");
        ck("footer: one strip 240 x 14 at LCD y 226", bl_n == 1 && bl_y[0] == UX_Y_FOOT && bl_h[0] == UX_H_FOOT);
        ck("footer: the band, ACID at the left edge, QUEUED at the right", px(0, 12) == UX_ACID && px(239, 12) == UX_QUEUED);
        {   /* dark ink on both sides */
            uint32_t l = 0, r = 0, x, y;
            for (y = 0; y < UX_H_FOOT; y++)
                for (x = 0; x < 240; x++)
                    if ((px((int32_t)x, (int32_t)y) >> 11) < 6) { if (x < 100) l++; else if (x > 140) r++; }
            ck("footer: dark text on the left (the location) and on the right (the knobs' names)", l > 15 && r > 15);
        }
    }
    {   /* meters */
        uint8_t lv[6] = {255, 128, 0, 64, 200, 30};
        const char *nm[6] = {"T1", "T2", "T3", "T4", "T5", "T6"};
        ux_panel_begin(3);
        ux_meters(lv, nm, 1, 0);
        ck("meters: a full column reaches 70 px in the knob 1 colour (ACID)", px(10 + 16, 12) == UX_ACID && px(10 + 16, 81) == UX_ACID);
        ck("meters: knob columns wear knob colours, the others MID",
           px(10 + 37 + 16, 82 - 30) == UX_SLIDE && px(10 + 4 * 37 + 16, 82 - 50) == UX_MID);
        ck("meters: an empty column shows only its track", px(10 + 2 * 37 + 16, 81) == UX_EMPTY);
        ck("meters: the selected column is outlined TEXT", px(10 + 37 - 2, 10) == UX_TEXT);
    }
    ck("text samples: the spec's footer hint fits (cv_text-based widths)", text_w(&UF_LBL, "LVL \xB7 LVL \xB7 LVL \xB7 LVL") < 140);
    return fails ? 1 : 0;
}
