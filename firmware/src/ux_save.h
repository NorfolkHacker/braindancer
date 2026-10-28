/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's SAVE pages (3c-2 spec 2; ux_save.c): SOUND, the 32 user slots as a list and the track's sound over it (task 2); SONG and
 * PROJECT (tasks 3, 4); SAVE > MORE's rows (params.c UXM_SAVE). Included by ux_section.h (ux_page_owns); the code after ux_glo.c,
 * before ux_section.c. */
#ifndef UX_SAVE_H
#define UX_SAVE_H
#include "ux_widgets.h"                                    /* (ux_knob_t: ux_widgets.c comes later) */

static int uxv_owns(void);                               /* the SAVE page shown is drawn by ux_section.c (SOUND, SONG, PROJECT, MORE) */
static int uxv_knobs(const page_t *pg, ux_knob_t k[4]);  /* SOUND: SLOT PRESET NAME SAVE (1: filled) */
static void uxv_picture(const page_t *pg);
static uint32_t uxv_sig(const page_t *pg);               /* what the picture is drawn from */
static int uxv_say(const page_t *pg, char *b, uint32_t nb);   /* the status line ("U07 . DEEP SUB . OCT+ SAVES OVER IT") */
static uint16_t uxv_eng_col(uint32_t e);                 /* an engine's colour: ux_knob_col(engine's place in ENGINE_ORDER % 4) */

#endif
