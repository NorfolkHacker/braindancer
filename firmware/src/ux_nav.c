/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's HOME navigation (ux_nav.h): HOME's three pages as a table and the page-memory rule. The memory is JIANT's byte,
 * ui.nmem[SEC_HOME] (the chain's entry ^ NAV_DEF, ui_lens.c nav_mem): 0 TRACKS, 1 VISUAL, 2 GEN (one page since 0.9; spec
 * rule 1: a section opens on the page last used there, GEN too); a value past the chain (0.8's 3, 4: GEN 2, GEN 3) reads
 * as TRACKS. While GEN is shown the page is GEN whatever the byte says; a GEN closed by other means (its own OK) shows TRACKS. Included by ui.c after
 * ui_lens.c (nav_go / nav_mem / CE_HOME). */
#include "ux_nav.h"

typedef struct { const char *name, *hint; } uxh_page_t;
static const uxh_page_t UXH_PAGES[UXH_N] = {
    {"TRACKS", "HOME AGAIN: VISUAL"},
    {"VISUAL", "HOME AGAIN: GENERATE"},
    {"GEN", "HOME AGAIN: TRACKS"},
};

static uint32_t ux_home_entry(void)
{
    uint32_t e = nav_mem(SEC_HOME);
    return e < NELEM(CE_HOME) ? e : UXH_TRACKS;
}
static uint32_t ux_home_page(void)
{
    uint32_t e = ux_home_entry();
    if (gen_on())
        return UXH_GEN;
    return e < UXH_GEN0 ? e : ui.home ? UXH_TRACKS : UXH_GEN;   /* (on HOME with GEN closed by its OK: TRACKS) */
}
static void ux_home_flip(void)
{
    uint32_t p = ux_home_page();
    nav_go(SEC_HOME, p == UXH_TRACKS ? UXH_VISUAL : p == UXH_VISUAL ? UXH_GEN0 : UXH_TRACKS, 0);
    if (gen_on())                                       /* (GEN is JIANT's screen, no status line: its hint as a message) */
        ui_message(ux_home_hint());
}
static const char *ux_home_where(void)
{
    static char b[20];
    str_cpy(b, "HOME \xB7 ", sizeof b);                /* (Latin-1 middle dot, as JIANT's strings) */
    str_cpy(b + str_len(b), UXH_PAGES[ux_home_page()].name, sizeof b - str_len(b));
    return b;
}
static const char *ux_home_count(void)
{
    static char b[8];
    b[0] = (char)('1' + ux_home_page());
    str_cpy(b + 1, " / ", 4);
    b[4] = (char)('0' + UXH_N);
    b[5] = 0;
    return b;
}
/* what HOME does next; VISUAL while a song plays (GEN refused then, ui_lens.c nav_go: STOP TO EDIT) says so first */
#define UXH_HINT_BUSY "STOP, THEN HOME: GENERATE"
static const char *ux_home_hint(void)
{
    uint32_t p = ux_home_page();
    return p == UXH_VISUAL && chain_busy() ? UXH_HINT_BUSY : UXH_PAGES[p].hint;
}
