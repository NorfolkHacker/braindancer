/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's shared page pieces (spec 3.3): header, knob row, glow panel, status line, footer, step strip. Included by
 * ux_widgets.c after core.h (step_t) and ux_theme.c. */
#ifndef UX_WIDGETS_H
#define UX_WIDGETS_H

/* The 240 x 240 screen, top to bottom (each piece is its own strip, sent through cv_blit_cells: only the 8 x 4 cells
 * that changed go to the panel, as JIANT's HUD strips do; the panel is JIANT's 240 x 150 canvas strip, CV_MAX). The
 * rows between the panel and the footer are the status line (ux_status), drawn every frame so they are never stale. */
#define UX_Y_LINE   0u           /* the 2 px blend line */
#define UX_H_LINE   2u
#define UX_Y_HEAD   2u           /* header text */
#define UX_H_HEAD   14u
#define UX_Y_KNOBS  16u          /* four knob columns */
#define UX_H_KNOBS  40u
#define UX_Y_PANEL  56u          /* the page's picture over the glow */
#define UX_H_PANEL  150u
#define UX_Y_STATUS 206u         /* the status line: hint / readout / refusal */
#define UX_H_STATUS 20u
#define UX_Y_FOOT   226u         /* the blend band with dark text */
#define UX_H_FOOT   14u
#define UX_COL_X0   8            /* knob columns: x = UX_COL_X0 + k * UX_COL_PITCH, UX_COL_W wide */
#define UX_COL_PITCH 56
#define UX_COL_W    52
#define UX_STRIP_X  8            /* the step strip: cell i at UX_STRIP_X + i * UX_STRIP_PITCH */
#define UX_STRIP_PITCH 14
#define UX_STRIP_W  13
#define UX_STRIP_H  18

typedef struct { const char *label; char value[12]; uint8_t bar; /* 0..255 */ uint8_t off; /* not in use: its value dim */
                 uint8_t mark; /* (3c-1 task 10) the setting moves on its own: automation (PLAY ON) or a lock on the step picked */
} ux_knob_t;   /* (3b task 7 fix: value 12, a name: "CHO DEPTH") */
/* (3c-1 task 10) the mark: the column's rule's right end, UX_MARK_W px of it and a row under it, in UX_TEXT (hot or not) */
#define UX_MARK_W 10
#define UX_MARK_H 3

/* top line + header, LCD y 0..15: "T<trk> · sound" (trk 0: the sound alone), right in MID */
static void ux_header(uint32_t trk, const char *sound, const char *right);
/* knob row, LCD y 16..55. hot: 0..3 forces that column bright; -1 uses ux_knob_touch: the touched column is bright for
 * UX_HOT_MS after the touch, then normal. */
static void ux_knobs(const ux_knob_t k[4], int32_t hot);
static void ux_knob_touch(uint32_t k);                                          /* knob k (0..3) just moved: stamp the time */
static int32_t ux_knob_hot(void);                                               /* the knob touched less than UX_HOT_MS ago, or -1 */
#define UX_HOT_MS 600u
/* The load rules (spec 5.3; 3a task 7, fix 1), one pure rule: HOME VISUAL's panel drawn at most every UX_VIS_MS, every
 * UX_VIS_SLOW_MS while load_tight (voice.c: the audio over its budget) or under MENU's LOAD LITE / ECO (sys_load 1 / 2);
 * frozen (nothing drawn, the last picture kept) for UX_SHED_HOLD_MS after the overload guard last shed (load_diag.shed rose;
 * since_shed: ms since then, UX_NO_SHED none seen) and while the governor's gov_lite is on; the glow, on every page, level 0
 * while load_tight. now / last: ms */
#define UX_VIS_MS 100u
#define UX_VIS_SLOW_MS 200u
#define UX_SHED_HOLD_MS 400u
#define UX_NO_SHED 0xFFFFFFFFu
typedef struct { uint8_t draw, glow; } ux_pace_t;
static ux_pace_t ux_pace(uint32_t now, uint32_t last, uint32_t tight, uint32_t since_shed, uint32_t gov_lite, uint32_t sys_load,
                         uint32_t glow);
/* the same rule at other intervals: gap normally, slow while load_tight or LOAD LITE / ECO (ux_pace: UX_VIS_MS,
 * UX_VIS_SLOW_MS). HOME TRACKS' ridges (fix wave I2): UX_TRK_MS (25 frames a second at most), UX_TRK_SLOW_MS (3b task 10, spec
 * 5: half rate under load, was 100 ms; LFO's wave and FX MASTER's meter pace as the ridges) */
#define UX_TRK_MS 40u
#define UX_TRK_SLOW_MS 200u
static ux_pace_t ux_pace_at(uint32_t now, uint32_t last, uint32_t tight, uint32_t since_shed, uint32_t gov_lite,
                            uint32_t sys_load, uint32_t glow, uint32_t gap, uint32_t slow);
static uint32_t ux_glow_eff(uint32_t glow_level);                               /* the glow as the load allows (ux_pace) */
static void ux_panel_begin(uint32_t glow_level);                                /* canvas 240x150 for LCD y 56: BG + glow (ux_glow_eff) */
static void ux_panel_end(void);                                                 /* push the panel (changed cells only) */
/* the status line, LCD y 206..225 (whole strip BG every call, so nothing stays on it); text (NULL: none) in col, left at
 * the page margin: UX_MID a hint, UX_TEXT a readout, UX_ACCENT a refusal / warning, UX_QUEUED pending */
static void ux_status(const char *text, uint16_t col);
/* where (left) is the location, e.g. "HOME · TRACKS"; right the page number and/or the knobs' short names, e.g.
 * "LVL · LVL · LVL · LVL": dark text on the blend band, LCD y 226..239 */
static void ux_footer(const char *where, const char *right);
/* the transport for a header's right: "\x87 124 BPM" playing, "\x88 124 BPM" stopped (UF_HDR's play / stop glyphs,
 * tools/gen_aa_font.py CODE: U+25B6 at 0x87, U+25A0 at 0x88), into b (n bytes); (3c-1 task 8) led by the diamond (UX_GL_SYNC) while
 * the tempo is an outside clock's (CLOCK USB / TRS), on every ux page */
#define UX_GL_PLAY "\x87"
#define UX_GL_STOP "\x88"
/* (3c-1 task 8) GLO > MIDI's dots, on (U+25CF at 0x89) and off (U+25CB at 0x8A), and the outside clock's diamond (U+25C6 at 0x8B;
 * UF_LBL's, the diamond UF_HDR's too: the header marks an outside clock, ux_transport) */
#define UX_GL_ON "\x89"
#define UX_GL_OFF "\x8A"
#define UX_GL_SYNC "\x8B"
static void ux_transport(char *b, uint32_t n);
/* a strip of 16 cells at canvas y in the panel: the first n16 are the pattern's (steps[0..n16-1]), the rest drawn
 * dim (BG2); a tie after a rest or an empty step stays EMPTY (nothing to hold); edit / play: the cell to outline / lighten, or -1;
 * (3c-1 task 2 fix 1) prev: the colour a TIE in the first cell carries (UX_EMPTY: none), drum: the steps a DRUM track's (hits) */
static void ux_strip(int32_t y, const step_t *steps, uint32_t n16, int32_t edit, int32_t play, uint16_t prev, int drum);

#endif
