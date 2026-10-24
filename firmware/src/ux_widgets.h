/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's shared page pieces (spec 3.3): header, knob row, glow panel, footer, level meters, step strip. Included by
 * ux_widgets.c after core.h (step_t) and ux_theme.c. */
#ifndef UX_WIDGETS_H
#define UX_WIDGETS_H

/* The 240 x 240 screen, top to bottom (each piece is its own strip, sent through cv_blit_cells: only the 8 x 4 cells
 * that changed go to the panel, as JIANT's HUD strips do; the panel is JIANT's 240 x 150 canvas strip, CV_MAX). The
 * rows between the panel and the footer (UX_Y_GAP) are page margin: the page's full frame clears them to BG. */
#define UX_Y_LINE   0u           /* the 2 px blend line */
#define UX_H_LINE   2u
#define UX_Y_HEAD   2u           /* header text */
#define UX_H_HEAD   14u
#define UX_Y_KNOBS  16u          /* four knob columns */
#define UX_H_KNOBS  40u
#define UX_Y_PANEL  56u          /* the page's picture over the glow */
#define UX_H_PANEL  150u
#define UX_Y_GAP    206u
#define UX_H_GAP    20u
#define UX_Y_FOOT   226u         /* the blend band with dark text */
#define UX_H_FOOT   14u
#define UX_COL_X0   8            /* knob columns: x = UX_COL_X0 + k * UX_COL_PITCH, UX_COL_W wide */
#define UX_COL_PITCH 56
#define UX_COL_W    52
#define UX_STRIP_X  8            /* the step strip: cell i at UX_STRIP_X + i * UX_STRIP_PITCH */
#define UX_STRIP_PITCH 14
#define UX_STRIP_W  13
#define UX_STRIP_H  18

typedef struct { const char *label; char value[8]; uint8_t bar; /* 0..255 */ } ux_knob_t;

static void ux_header(uint32_t trk, const char *sound, const char *right);      /* top line + header, LCD y 0..15 */
static void ux_knobs(const ux_knob_t k[4], int32_t hot);                        /* knob row, LCD y 16..55; hot = 0..3 or -1 */
static void ux_panel_begin(uint32_t glow_level);                                /* canvas 240x150 for LCD y 56: BG + glow */
static void ux_panel_end(void);                                                 /* push the panel (changed cells only) */
/* where (left) is the location, e.g. "HOME · TRACKS"; right the page number and/or the knobs' short names, e.g.
 * "LVL · LVL · LVL · LVL": dark text on the blend band, LCD y 226..239 */
static void ux_footer(const char *where, const char *right);
/* six level columns in the panel's top 100 rows (call between ux_panel_begin and ux_panel_end); sel is outlined; the four from
 * knob_first (0..2) wear the knobs' colours (the four the knobs control), the others MID */
static void ux_meters(const uint8_t lvl[6], const char *const name[6], uint32_t sel, uint32_t knob_first);
/* a strip of 16 cells at canvas y in the panel: the first n16 are the pattern's (steps[0..n16-1]), the rest drawn
 * empty and dim; edit / play: the cell to outline / lighten, or -1 */
static void ux_strip(int32_t y, const step_t *steps, uint32_t n16, int32_t edit, int32_t play);

#endif
