/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's section pages (3b): any page of EDIT, ENV, LFO, FX, ARP, SCL drawn with the 3a widgets (ux_section.c), its MORE
 * list (ux_more.c). JIANT's page model stays the backend: the page is cur_page() (a PAGES row), the chain ui_lens.c's, a
 * knob's write ui_input.c's edit_param / param_knob. Sections move one at a time: ux_section_owns says which are drawn here
 * (ui_draw.c ui_draw_body routes them, ui.c page_visible shows their MORE page). Included by ui.c (page_visible); the code
 * after ux_home.c: ux_more.c, then ux_section.c. */
#ifndef UX_SECTION_H
#define UX_SECTION_H

/* the sections drawn by ux_section_draw, a bit per SEC_* (ui_lens.c; ux_section.c checks the numbers): EDIT (1) on a synth,
 * FM6 or DRUM-X track (ui_lens.c edit_kind 0..2: 3b tasks 4 and 5; DIGITAL, FELUCCA_FM4's, keeps JIANT's pages). Tasks 6..8 of
 * 3b add theirs */
#define UXS_OWN (1u << 1)
#define UXS_SEC_EDIT 1u
static uint32_t edit_kind(void);  /* (ui_lens.c) 0 a synth, 1 FM6, 2 DRUM, 3 DIGITAL */
static int ux_section_owns(uint32_t sec)
{
    return sec < 16u && ((UXS_OWN >> sec) & 1u) && (sec != UXS_SEC_EDIT || edit_kind() <= 2u);
}

static void ux_section_draw(void);   /* cur_page(): header, knobs from its PAGES row, its picture, status, footer */
/* the panel's list (between ux_panel_begin / ux_panel_end): each shown row's name and value, the picked one (sel, a
 * row index) lit, from row top */
static void ux_more_draw(const ux_more_t *m, uint32_t sel, uint32_t top);
/* KNOB knob (0..3, UXM_SELECT: SELECT) turned steps on list m: KNOB 1 picks a row, KNOB 2 sets it, SELECT scrolls (the
 * pick kept in sight). 1: taken; 0: not (SELECT before the first row: the page before, ui_input.c nav_select) */
static int ux_more_turn(const ux_more_t *m, uint32_t *sel, uint32_t *top, uint32_t knob, int32_t steps);
static int ux_more_knob(uint32_t knob, int32_t steps);   /* .. on the MORE page shown (its section's sel / top) */
/* setting id (a G_* when glo, else a P_* or a view) as section sec shows it: its plain name (ux_section.c UXS_NAME by section,
 * engine and id, else UXS_LABEL by JIANT's label l: "RES": "RESO"), else l */
static const char *uxs_label(uint32_t sec, uint32_t glo, uint32_t id, const char *l);
/* value v of setting d as the knob row, MORE and the status line show it: the number and its unit, compact ("1.9K", "1.0MS") */
static void uxs_val(const param_desc_t *d, int32_t v, char *b, uint32_t nb);

#endif
