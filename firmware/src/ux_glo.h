/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's GLO pages (3c-1 spec 3; ux_glo.c): LIVE, the keys a board of live tricks over the six tracks (ui_live.c, seq_live.c),
 * its picture one bar as a timeline with the tricks on it; GLO > MORE's RAILS rows (the inherited second LIVE page's knobs).
 * Included by ux_section.h (ux_page_owns); the code after ux_seq.c, before ux_section.c. */
#ifndef UX_GLO_H
#define UX_GLO_H

static int uxg_owns(void);                      /* the GLO page shown is drawn by ux_glo.c: LIVE (task 7: MIX, task 8: MIDI) */
static int uxg_more_live(void);                 /* GLO > MORE shown, a LIVE row picked: LIVE's OCT- / OCT+ there (ui_live.c
                                                 * live_page), its tracks the TRACKS row's */
/* GLO > MORE's RAILS views (params.c ID_LV_*): a row's name, its value in words (*bar its place, 0: none; ux_more.c uxm_row_text) */
static const char *uxg_vtext(uint32_t id, char *val, uint32_t nv, uint8_t *bar);

#endif
