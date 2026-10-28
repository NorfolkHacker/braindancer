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
 * FM6 or DRUM-X track (ui_lens.c edit_kind 0..2: 3b tasks 4 and 5; DIGITAL, FELUCCA_FM4's, keeps JIANT's pages); (task 6) ENV (5),
 * SCL (7), ARP (8); (task 7) LFO (6); (task 8) FX (3); (3c-1 task 2) SEQ (2), (task 6) GLO (4): their pages drawn here are
 * ux_page_owns' (below) */
#include "ux_seq.h"
#include "ux_glo.h"
#include "ux_save.h"
#define UXS_SEC_EDIT 1u
#define UXS_SEC_SEQ 2u
#define UXS_SEC_FX 3u
#define UXS_SEC_GLO 4u
#define UXS_SEC_ENV 5u
#define UXS_SEC_LFO 6u
#define UXS_SEC_SCL 7u
#define UXS_SEC_ARP 8u
#define UXS_SEC_SAVE 9u          /* (3c-2 task 2) SAVE: its SOUND and MORE (uxv_owns; the inherited SECTIONS ARRANGE PROJECTS TOOLS
                                  * until tasks 3, 4) */
#define UXS_OWN (1u << UXS_SEC_EDIT | 1u << UXS_SEC_SEQ | 1u << UXS_SEC_FX | 1u << UXS_SEC_ENV | 1u << UXS_SEC_LFO | \
                 1u << UXS_SEC_SCL | 1u << UXS_SEC_ARP | 1u << UXS_SEC_GLO | 1u << UXS_SEC_SAVE)
static uint32_t edit_kind(void);  /* (ui_lens.c) 0 a synth, 1 FM6, 2 DRUM, 3 DIGITAL */
/* section sec is on the ux engine (ui.c page_jiant: its MORE page shown, from any of its pages) */
static int ux_section_owns(uint32_t sec)
{
    return sec < 16u && ((UXS_OWN >> sec) & 1u) && (sec != UXS_SEC_EDIT || edit_kind() <= 2u);
}

/* (3c-1 task 2 fix 1) the page shown is drawn by ux_section_draw (ui_draw.c routes it, ui_input.c lights its knob column): its
 * section's on the ux engine, and on SEQ one of the pages ux_seq.c draws (uxq_owns: STEPS, task 3: STEP EXTRAS, PATTERN) or
 * (task 4) its MORE; PHRASES (PRESETS' pattern loader) JIANT's; (task 6) on GLO LIVE (uxg_owns) and its MORE, (task 7) MIX: every
 * GLO page (the inherited MIXER, SENDS, GLOBAL, SYSTEM gone); (3c-2 task 2) on SAVE its SOUND and MORE (uxv_owns: MORE counted
 * there), the inherited SECTIONS, ARRANGE, PROJECTS, TOOLS JIANT's until tasks 3, 4 */
static uint32_t nav_sec(void);
static int ux_page_owns(void)
{
    uint32_t sec = nav_sec();
    int more = !ui.home && ui.page < NPAGES && PAGES[ui.page].graph == GR_MORE;
    return ux_section_owns(sec) && (sec == UXS_SEC_SEQ ? uxq_owns() || more : sec == UXS_SEC_GLO ? uxg_owns() || more :
                                    sec == UXS_SEC_SAVE ? uxv_owns() : 1);
}
static void ux_section_draw(void);   /* cur_page(): header, knobs from its PAGES row, its picture, status, footer */
/* the panel's list (between ux_panel_begin / ux_panel_end): each shown row's name and value, the picked one (sel, a
 * row index) lit, from row top */
static void ux_more_draw(const ux_more_t *m, uint32_t sel, uint32_t top);
/* KNOB knob (0..3, UXM_SELECT: SELECT) turned steps on list m: KNOB 1 picks a row, KNOB 2 sets it, SELECT scrolls (the
 * pick kept in sight). 1: taken; 0: not (SELECT before the first row: the page before, ui_input.c nav_select) */
static int ux_more_turn(const ux_more_t *m, uint32_t *sel, uint32_t *top, uint32_t knob, int32_t steps);
static int ux_more_knob(uint32_t knob, int32_t steps);   /* .. on the MORE page shown (its section's sel / top) */
/* (3b task 9) a MORE page pg as the step locks' page (ui.plk_src: ui.c lock_id, ui_input.c lock_turn, ui_draw.c's cards while a
 * step is held on STEP): the row its section's pick names now (0: none); KNOB 1 with a step held, that pick moved steps rows;
 * row r takes a lock (UXM_LOCKS) or why not (UXM_NL_*: UXM_WHY says it, nothing changes); card c of the step held's four (KNOB 1 the pick,
 * KNOB 2 the row: its lock's value, *on, else the sound's), its label (0: an empty card), *ratio 0..1000 (-1: none) */
static const ux_row_t *uxm_lock_row(const page_t *pg);
static void uxm_lock_pick(const page_t *pg, int32_t steps);
static uint32_t uxm_lock_why(const ux_row_t *r);
static void uxm_lock_say(const ux_row_t *r);   /* .. why not, said (ui_message) */
static const char *uxm_lock_card(const page_t *pg, uint32_t c, uint32_t step, char *val, uint32_t nv, int *on, int32_t *ratio);
/* setting id (a G_* when glo, else a P_* or a view) as section sec shows it: its plain name (ux_section.c UXS_NAME by section,
 * engine and id, else UXS_LABEL by JIANT's label l: "RES": "RESO"), else l */
static const char *uxs_label(uint32_t sec, uint32_t glo, uint32_t id, const char *l);
/* value v of setting d as the knob row, MORE and the status line show it: the number and its unit, compact ("1.9K", "1.0MS") */
static void uxs_val(const param_desc_t *d, int32_t v, char *b, uint32_t nb);
/* .. as setting id (a G_* when glo) shows it: uxs_val's, or its own words (ARP's REPT, SNAP's KEYS, ALL TRACKS' OFF / +2,
 * FM6's P 5) */
static void uxs_vtext(const param_desc_t *d, int32_t v, uint32_t glo, uint32_t id, char *b, uint32_t nb);

#endif
