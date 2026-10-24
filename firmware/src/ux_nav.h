/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's HOME navigation (spec 4.1 / 4.3): HOME is two pages, TRACKS and VISUAL; GEN (JIANT's generator, its pages
 * GEN 1..3) is reached by HOME + SELECT. HOME's chain in ui_lens.c (CE_HOME) is TRACKS, VISUAL, then GEN 1..3 from
 * UXH_GEN0. Included by ui.c before ui_lens.c; ux_nav.c after it. */
#ifndef UX_NAV_H
#define UX_NAV_H

enum { UXH_TRACKS, UXH_VISUAL, UXH_N };                 /* HOME's pages */
#define UXH_GEN0 UXH_N                                  /* HOME's chain entry of GEN 1 (GEN 2, GEN 3 after it) */
#define UXH_NGEN 3u

static uint32_t ux_home_page(void);                     /* the HOME page last shown (a stored one that is no more: TRACKS) */
static void ux_home_flip(void);                         /* HOME shown on the other page (TRACKS <-> VISUAL) */
static const char *ux_home_where(void);                 /* the footer's place: "HOME · TRACKS" */
static const char *ux_home_count(void);                 /* .. its page of HOME's: "1 / 2" */
static const char *ux_home_hint(void);                  /* the status line: what HOME does next, "HOME AGAIN: VISUAL" */
static void ux_home_draw(void);                         /* (ux_home.c) HOME's page on the screen */

#endif
