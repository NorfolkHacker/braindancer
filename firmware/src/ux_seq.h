/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's SEQ pages (3c-1 spec 2; ux_seq.c): STEPS, the roll of the 16 steps shown over the strip of their cells (a DRUM
 * track: the lane grid over it); STEP EXTRAS, the picked step's ratchet, chance, condition, micro; PATTERN. The pieces each SEQ page draws with: the step picked (shared by every SEQ page and MORE), the
 * strip, the roll, the grid. Included by ux_section.h (ux_section_owns: the SEQ page shown is drawn there when ux_seq.c draws
 * it); the code after ux_more.c, before ux_section.c. */
#ifndef UX_SEQ_H
#define UX_SEQ_H

static uint32_t uxq_step(void);                 /* the picked step 0..len-1, shared by every SEQ page and MORE (ui.cursor) */
static void uxq_strip(int y, uint32_t first);   /* the 16-cell strip at canvas y for steps first..first+15 (note / accent /
                                                 * slide / rest; past LEN dark; the picked outlined, the playing lit) */
static void uxq_roll(int y0, int h);            /* the roll of the current 16 (pitch up, time across), rows y0..y0+h-1 */
static void uxq_grid(int y0, int h);            /* DRUM: 8 lanes x the current 16, rows y0..y0+h-1 */
static int uxq_owns(void);                      /* the SEQ page shown is drawn by ux_seq.c: STEPS, (task 3) STEP EXTRAS,
                                                 * PATTERN (its UXQ_*; 0: none) */
static uint32_t uxq_ratch(const step_t *st, uint32_t *shape, int32_t *k);   /* (task 4) the step's ratchet as STEP EXTRAS shows
                                                 * it: its hits (an ECHO component's x2..x8, else the flags' x1..x4), *shape
                                                 * (RS_*), *k the ECHO's slot (-1: the flags') */
static void uxq_cond_turn(step_t *st, int32_t e);   /* STEP EXTRAS' CONDITION a detent on: the step's TRIG put, edited or taken
                                                 * away (ui_input.c; never another component overwritten) */
/* (task 4) SEQ > MORE's views (params.c ID_SQ_*): a view's row shown now (by the step picked, the track, its components), its
 * name and value (*bar its place, 0: none; ux_more.c uxm_row_text); ui_input.c seq_view_turn turns it */
static int uxq_vshown(uint32_t id);
static const char *uxq_vtext(uint32_t id, char *val, uint32_t nv, uint8_t *bar);
/* .. the row picked on SEQ > MORE shown now (0: not there), what its keys do: OCT+ on AUTO LIST's rows adds or turns a record
 * (ui.c act_cols), on MUTATE's KEEP (OCT- FREEZE, ui_input.c); TRACK's rows not while a song plays (uxq_more_busy) */
enum { UXQ_MK_NONE, UXQ_MK_LIST, UXQ_MK_MUTATE, UXQ_MK_AUTO, UXQ_MK_STEP };
static uint32_t uxq_more_kind(void);
static int uxq_more_busy(void);
static int uxq_select(int32_t s);               /* SELECT turned s on STEPS: on the strip's last cell the next 16, on its
                                                 * first the 16 before (1: taken; 0: the chain's, ui_lens.c nav_select) */

#endif
