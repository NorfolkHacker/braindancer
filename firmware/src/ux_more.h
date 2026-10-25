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
typedef struct { uint16_t id; uint8_t kind; } ux_row_t;
typedef struct ux_more_s { const ux_row_t *rows; uint8_t n; } ux_more_t;

#define UXM_ROWS 8u              /* rows on the panel at once */
#define UXM_PITCH 18             /* .. a row's height (8 x 18 = 144 of the panel's 150) */
#define UXM_SELECT 4u            /* ux_more_turn's knob for SELECT (0..3: KNOB 1..4) */

#endif
