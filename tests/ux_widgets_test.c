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
static uint32_t test_ms;
#define UX_NOW() test_ms
static uint32_t test_tight;
#define UX_TIGHT() test_tight                          /* (voice.c load_tight) */
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
    ck("UX_H_* sum to 240", UX_H_LINE + UX_H_HEAD + UX_H_KNOBS + UX_H_PANEL + UX_H_STATUS + UX_H_FOOT == 240u);
    ck("UX_Y_* follow each other with no overlap",
       UX_Y_LINE == 0 && UX_Y_HEAD == UX_Y_LINE + UX_H_LINE && UX_Y_KNOBS == UX_Y_HEAD + UX_H_HEAD
       && UX_Y_PANEL == UX_Y_KNOBS + UX_H_KNOBS && UX_Y_STATUS == UX_Y_PANEL + UX_H_PANEL && UX_Y_FOOT == UX_Y_STATUS + UX_H_STATUS
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
        ux_strip(cy, st, 8, 7, 0, UX_EMPTY, 0);
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
        test_ms = 5000;
        ux_knobs(k, -1);
        ck("knobs: no hot column: every column on BG", px(UX_COL_X0 + UX_COL_PITCH + UX_COL_W - 3, 10) == UX_BG);
        ux_knob_touch(2);
        test_ms += UX_HOT_MS - 1u;
        ux_knobs(k, -1);
        ck("knobs: a touched knob's column is hot until UX_HOT_MS", px(UX_COL_X0 + 2 * UX_COL_PITCH + UX_COL_W - 3, 10) == UX_BG2);
        test_ms += 1u;
        ux_knobs(k, -1);
        ck("knobs: ... and cool after it", px(UX_COL_X0 + 2 * UX_COL_PITCH + UX_COL_W - 3, 10) == UX_BG);
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
        {   /* (3b final review M4) the words apart: "NEXT OP ENV" two gaps of at least 3 clear columns (UF_LBL's 1.7 px
             * space read as "NEXT OPENV") and none inside a word */
            int32_t x, y, run = 0, gaps = 0, w;
            ux_footer("NEXT OP ENV", "");
            w = uxf_w("NEXT OP ENV");
            for (x = 8; x <= 8 + w; x++) {
                int ink = 0;
                for (y = 0; y < (int32_t)UX_H_FOOT - 1; y++)
                    ink |= px(x, y) != px(x, (int32_t)UX_H_FOOT - 1);   /* (any ink, its antialiased edge too: the band
                                                         * is one colour down a column) */
                if (!ink) run++;
                else { gaps += run >= 3; run = 0; }
            }
            ck("footer: two-word names apart (NEXT OP ENV: two gaps of 3 px or more)", gaps == 2);
        }
    }
    {   /* status */
        bl_n = 0;
        ux_status("5 \xB7 G2 \xB7 ACC", UX_ACCENT);
        ck("status: one strip 240 x 20 at LCD y 206", bl_n == 1 && bl_y[0] == UX_Y_STATUS && bl_h[0] == UX_H_STATUS && bl_w[0] == 240);
        ck("status: text in the colour given, on BG", ink_n(UX_BG) > 20 && px(239, 19) == UX_BG);
        {
            uint32_t i, hit = 0;
            for (i = 0; i < cv_w * cv_h; i++) hit += swap16(cv_px[i]) == UX_ACCENT;
            ck("status: some pixels are fully the colour", hit > 5);
        }
        ux_status(NULL, UX_MID);
        ck("status: NULL text is a plain BG strip (clears the old line)", ink_n(UX_BG) == 0 && bl_n == 2);
    }
    {   /* the glow's row limit */
        uint32_t i, mx = 0;
        ux_panel_begin(3);
        ck("glow: a strip of UX_GLOW_ROWS rows draws", px(96, 30) != UX_BG);
        cv_begin(240, 150, UX_BG);
        ux_glow(0, 0, 240, (int32_t)UX_GLOW_ROWS, 3);
        ck("glow: h = UX_GLOW_ROWS draws", px(96, 30) != UX_BG);
        cv_begin(240, 150, UX_BG);
        ux_glow(0, 0, 240, 400, 3);
        ck("glow: h past the strip is clamped to 150 (draws, no overrun)", px(96, 45) != UX_BG);
        (void)i; (void)mx;
    }
    {   /* (3a task 7) VISUAL's pace and every page's glow under load (spec 5.3): one pure rule */
        ux_pace_t r;
        r = ux_pace(1000, 1000 - 99, 0, UX_NO_SHED, 0, 0, 3);
        ck("pace: 99 ms after the last draw: not yet (at most every 100 ms)", !r.draw && r.glow == 3);
        r = ux_pace(1000, 1000 - 100, 0, UX_NO_SHED, 0, 0, 3);
        ck("pace: 100 ms after: draw", r.draw && r.glow == 3);
        r = ux_pace(5, 0xFFFFFFF0u, 0, UX_NO_SHED, 0, 0, 2);
        ck("pace: the ms clock wrapping round: 21 ms is still 21 ms (not yet)", !r.draw);
        r = ux_pace(1000, 1000 - 199, 1, UX_NO_SHED, 0, 0, 3);
        ck("pace: load_tight: 199 ms after: not yet (every 200 ms); the glow off", !r.draw && r.glow == 0);
        r = ux_pace(1000, 1000 - 200, 1, UX_NO_SHED, 0, 0, 3);
        ck("pace: load_tight: 200 ms after: draw; the glow off", r.draw && r.glow == 0);
        r = ux_pace(1000, 1000 - 5000, 0, 0, 0, 0, 3);
        ck("pace: the overload guard shed this frame: frozen (however long since the last draw)", !r.draw && r.glow == 3);
        r = ux_pace(1000, 1000 - 5000, 0, 399, 0, 0, 3);
        ck("pace: 399 ms after the last shed: still frozen (held 400 ms)", !r.draw);
        r = ux_pace(1000, 1000 - 5000, 0, 400, 0, 0, 3);
        ck("pace: 400 ms after the last shed: drawn again", r.draw);
        r = ux_pace(1000, 1000 - 5000, 1, 10, 0, 0, 3);
        ck("pace: shedding while tight: frozen, the glow off", !r.draw && r.glow == 0);
        r = ux_pace(1000, 1000 - 5000, 0, UX_NO_SHED, 1, 0, 1);
        ck("pace: gov_lite (the governor's LOAD LITE): frozen while on", !r.draw && r.glow == 1);
        r = ux_pace(1000, 1000 - 199, 0, UX_NO_SHED, 0, 1, 3);
        ck("pace: MENU LOAD LITE (sys_load 1): every 200 ms, not frozen", !r.draw && r.glow == 3);
        r = ux_pace(1000, 1000 - 200, 0, UX_NO_SHED, 0, 1, 3);
        ck("pace: MENU LOAD LITE: 200 ms after: draw", r.draw);
        r = ux_pace(1000, 1000 - 199, 0, UX_NO_SHED, 0, 2, 3);
        ck("pace: MENU LOAD ECO (sys_load 2): every 200 ms", !r.draw);
        r = ux_pace(1000, 1000 - 200, 1, UX_NO_SHED, 0, 2, 3);
        ck("pace: LOAD ECO and tight: 200 ms after: draw, the glow off", r.draw && r.glow == 0);
        r = ux_pace(1000, 1000 - 200, 0, UX_NO_SHED, 0, 0, 0);
        ck("pace: glow 0 asked: 0", r.draw && r.glow == 0);
        r = ux_pace_at(1000, 1000 - 39, 0, UX_NO_SHED, 0, 0, 2, UX_TRK_MS, UX_TRK_SLOW_MS);
        ck("pace_at (TRACKS): 39 ms after: not yet", !r.draw && r.glow == 2);
        r = ux_pace_at(1000, 1000 - 40, 0, UX_NO_SHED, 0, 0, 2, UX_TRK_MS, UX_TRK_SLOW_MS);
        ck("pace_at (TRACKS): 40 ms after: draw", r.draw);
        r = ux_pace_at(1000, 1000 - 199, 1, UX_NO_SHED, 0, 0, 2, UX_TRK_MS, UX_TRK_SLOW_MS);
        ck("pace_at (TRACKS, 3b task 10: half rate under load): tight: 199 ms after: not yet, the glow off", !r.draw && r.glow == 0);
        r = ux_pace_at(1000, 1000 - 200, 1, UX_NO_SHED, 0, 0, 2, UX_TRK_MS, UX_TRK_SLOW_MS);
        ck("pace_at (TRACKS): tight: 200 ms after: draw", r.draw && r.glow == 0);
        r = ux_pace_at(1000, 1000 - 199, 0, UX_NO_SHED, 0, 1, 2, UX_TRK_MS, UX_TRK_SLOW_MS);
        ck("pace_at (TRACKS): LOAD LITE: 199 ms after: not yet", !r.draw);
        r = ux_pace_at(1000, 1000 - 200, 0, UX_NO_SHED, 0, 2, 2, UX_TRK_MS, UX_TRK_SLOW_MS);
        ck("pace_at (TRACKS): LOAD ECO: 200 ms after: draw", r.draw);
        r = ux_pace_at(1000, 1000 - 5000, 0, 399, 0, 0, 2, UX_TRK_MS, UX_TRK_SLOW_MS);
        ck("pace_at (TRACKS): frozen in the shed hold", !r.draw);
        r = ux_pace_at(1000, 1000 - 5000, 0, UX_NO_SHED, 1, 0, 2, UX_TRK_MS, UX_TRK_SLOW_MS);
        ck("pace_at (TRACKS): frozen while gov_lite", !r.draw);
        test_tight = 1;
        ux_panel_begin(3);
        ck("panel: load_tight: no glow (level 0 on every ux page)", ink_n(UX_BG) == 0);
        test_tight = 0;
        ux_panel_begin(3);
        ck("panel: not tight: the glow", ink_n(UX_BG) > 1000);
    }
    ck("text samples: the spec's footer hint fits (cv_text-based widths)", text_w(&UF_LBL, "LVL \xB7 LVL \xB7 LVL \xB7 LVL") < 140);
    return fails ? 1 : 0;
}
