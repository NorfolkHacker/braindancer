/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's SEQ pages (3c-1 spec 2; ux_seq.c): STEPS, the roll of the 16 steps shown over the strip of their cells (a DRUM
 * track: the lane grid over it). The pieces each SEQ page draws with: the step picked (shared by every SEQ page and MORE), the
 * strip, the roll, the grid. Included by ux_section.h (ux_section_owns: the SEQ page shown is drawn there when ux_seq.c draws
 * it); the code after ux_more.c, before ux_section.c. */
#ifndef UX_SEQ_H
#define UX_SEQ_H

static uint32_t uxq_step(void);                 /* the picked step 0..len-1, shared by every SEQ page and MORE (ui.cursor) */
static void uxq_strip(int y, uint32_t first);   /* the 16-cell strip at canvas y for steps first..first+15 (note / accent /
                                                 * slide / rest; past LEN dark; the picked outlined, the playing lit) */
static void uxq_roll(int y0, int h);            /* the roll of the current 16 (pitch up, time across), rows y0..y0+h-1 */
static void uxq_grid(int y0, int h);            /* DRUM: 8 lanes x the current 16, rows y0..y0+h-1 */
static int uxq_owns(void);                      /* the SEQ page shown is drawn by ux_seq.c (task 2: STEPS) */
static int uxq_select(int32_t s);               /* SELECT turned s on STEPS: on the strip's last cell the next 16, on its
                                                 * first the 16 before (1: taken; 0: the chain's, ui_lens.c nav_select) */

#endif
