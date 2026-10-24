/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's HOME navigation (ux_nav.h): HOME's two pages as a table and the page-memory rule. The memory is JIANT's byte,
 * ui.nmem[SEC_HOME] (the chain's entry ^ NAV_DEF, ui_lens.c nav_mem): 0 TRACKS, 1 VISUAL; any other value (0.9.2's
 * ECO MACROS BEING ROUTES GEN ones) reads as TRACKS. GEN never goes in it: out of GEN is HOME's last page. Included by ui.c
 * after ui_lens.c (nav_go / nav_mem / CE_HOME). */
#include "ux_nav.h"

typedef struct { const char *hint; } uxh_page_t;        /* (the name: ui_lens.c CE_HOME's, the chain's) */
static const uxh_page_t UXH_PAGES[UXH_N] = {
    {"HOME AGAIN: VISUAL"},                             /* TRACKS */
    {"HOME AGAIN: TRACKS"},                             /* VISUAL */
};

static uint32_t ux_home_page(void)
{
    uint32_t e = nav_mem(SEC_HOME);
    return e < UXH_N ? e : UXH_TRACKS;
}
static void ux_home_flip(void)
{
    nav_go(SEC_HOME, ux_home_page() == UXH_TRACKS ? UXH_VISUAL : UXH_TRACKS, 0);
}
static const char *ux_home_where(void)
{
    static char b[20];
    str_cpy(b, "HOME \xB7 ", sizeof b);                /* (Latin-1 middle dot, as JIANT's strings) */
    str_cpy(b + str_len(b), CE_HOME[ux_home_page()].nm, sizeof b - str_len(b));
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
static const char *ux_home_hint(void) { return UXH_PAGES[ux_home_page()].hint; }
