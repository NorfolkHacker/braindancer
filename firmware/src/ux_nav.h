/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's HOME navigation (spec 4.1 - 4.3): HOME is three pages, TRACKS -> VISUAL -> GEN (GEN: the generator, one page
 * since Braindancer 0.9, JIANT's GEN 1..3 before; HOME + SELECT jumps straight to GEN). HOME's chain in ui_lens.c (CE_HOME)
 * is TRACKS, VISUAL, then GEN at UXH_GEN0. Included by ui.c before ui_lens.c; ux_nav.c after it. */
#ifndef UX_NAV_H
#define UX_NAV_H

enum { UXH_TRACKS, UXH_VISUAL, UXH_GEN, UXH_N };          /* HOME's pages */
#define UXH_GEN0 2u                                     /* HOME's chain entry of GEN */
#define UXH_NGEN 1u                                     /* (0.9: GEN is one page) */

static uint32_t ux_home_page(void);                     /* the HOME page shown / last shown (TRACKS VISUAL GEN) */
static uint32_t ux_home_entry(void);                    /* .. its chain entry (GEN: its page's; one that is no more: TRACKS) */
static void ux_home_flip(void);                         /* HOME's next page, round: TRACKS -> VISUAL -> GEN -> TRACKS */
static const char *ux_home_where(void);                 /* the footer's place: "HOME · TRACKS" */
static const char *ux_home_count(void);                 /* .. its page of HOME's: "1 / 3" */
static const char *ux_home_hint(void);                  /* the status line: what HOME does next, "HOME AGAIN: VISUAL" */
static void ux_home_draw(void);                         /* (ux_home.c) TRACKS / VISUAL on the screen (GEN: JIANT's) */

/* TRACKS' ridgelines (ux_home.c, 3a task 6): each track's level history, UXR_N points a UXR_MS apart (~2 s), drawn over x
 * UXR_X0 .. UXR_X0 + UXR_W - 1 of the panel, row r's baseline at panel y UXR_BASE(r), UXR_ROW apart, a peak UXR_PEAK px high:
 * (3b task 10) inside its own row, the selected line's extra pixel too, never up into the row above (T5's acid line rising
 * into T4's row read as T4 playing); the strip's caption and the strip under them */
#define UXR_N 112u
#define UXR_MS 18u
#define UXR_X0 58
#define UXR_W 148u
#define UXR_ROW 17
#define UXR_PEAK (UXR_ROW - 2)
#define UXR_BASE(r) (26 + UXR_ROW * (int32_t)(r))
#define UXR_CAP_Y 116
#define UXR_STRIP_Y 128
static void ux_levels_tick(void);                       /* every UI frame (ui_draw): the one reader of trk[c].peak */
static void ux_levels_reset(void);                      /* the histories flat */
static void ux_knob_touch(uint32_t k);                  /* (ux_widgets.c) knob k just turned: its column lit UX_HOT_MS */
static uint32_t ux_trk_first(void);                     /* (ux_home.c, 3b task 10) TRACKS' KNOB 1..3: the LEVELs of tracks from this
                                                         * one (the three holding the selected: T1 or T4); KNOB 4 the tempo */
#define UXH_NLVL 3u
static void ux_home_bpm(int32_t steps);                 /* (ux_home.c, 3b task 10) TRACKS' KNOB 4 turned: the tempo */

#endif
