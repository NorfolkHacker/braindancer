/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's MORE list (3b spec 2, parent spec 4.1 rule 5): a section's last page, a list of everything else in it, one setting
 * a row. SELECT scrolls, KNOB 1 picks a row, KNOB 2 sets it (through ui_input.c param_knob, the write a page's knob does:
 * undo, locks, automation and MIDI LEARN as there). A MORE page is a PAGES row (params.c) whose graph is GR_MORE and whose
 * .more points at its list. Types only here (params.c includes it for page_t); the code is ux_more.c, included after
 * ux_home.c. */
#ifndef UX_MORE_H
#define UX_MORE_H

enum { UXR_TRACK, UXR_GLOBAL, UXR_ACTION, UXR_HEAD };   /* a row: a track setting (P_*), a global (G_*), an action (an
                                                         * index of ux_more.c UXM_ACT), a heading (an index of UXM_HEAD) */
/* eng: shown only on that engine's tracks (its ENGINES index + 1; 0: any), where one stored slot means two things (P_LRTRG is
 * LOFI's RETRIG and VOICE's WORDS): each row says only one of them */
typedef struct { uint16_t id; uint8_t kind, eng; } ux_row_t;
typedef struct ux_more_s { const ux_row_t *rows; uint8_t n; } ux_more_t;
#define UXR_ON(e) ((uint8_t)((e) + 1u))   /* (a row's eng: on engine e only) */

/* the headings (UXR_HEAD rows' ids; ux_more.c UXM_HEAD their words) and the actions (UXR_ACTION rows' ids; UXM_ACT) */
enum { UXM_H_ARP, UXM_H_VOICE, UXM_H_FILTER, UXM_H_TONE, UXM_H_MOD, UXM_H_MSEQ, UXM_H_DELAY, UXM_H_REVERB, UXM_H_CHORUS,
       UXM_H_SLICER, UXM_H_WORDS, UXM_H_KIT, UXM_H_OP, UXM_H_DRUM, UXM_H_MLFO, UXM_H_PATCH, UXM_H_LFO, UXM_H_N };   /* (3b task 5: OP and DRUM name the
                                                         * operator and the drum picked, "OP 3", "DRUM \xB7 SD": ux_more.c;
                                                         * task 7: LFO's MORE, LFO and MSEQ: "STEPS") */
enum { UXM_A_LYRIC, UXM_A_N };

#define UXM_ROWS 8u              /* rows on the panel at once */
#define UXM_PITCH 18             /* .. a row's height (8 x 18 = 144 of the panel's 150) */
#define UXM_SELECT 4u            /* ux_more_turn's knob for SELECT (0..3: KNOB 1..4) */
#define UXM_K34 "KNOB 1 PICKS \xB7 KNOB 2 SETS"   /* KNOB 3 or 4 turned on MORE: what the knobs do (ui_input.c) */

#endif
