/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's held layers (3b spec 2.6): EDIT, LFO's MACRO, FX's performance layer and SCL drawn in the 3a look (ux_layer.c): the
 * header, the knob row (KNOB 1..4 in their colours: the layer's own four, EDIT's the engine's first page, SCL's ROOT SCALE ROTATE
 * SEQ PITCH), the panel (MAP: the keys, what each does; STATE: what is on, ALGORITHM between them), the status line (the layer
 * and what its keys and OCT do; a knob's readout; a message), the footer. What the keys and knobs do is ui_layer.c's, as before;
 * GLO, SEQ TOOLS, REC and SAVE keep JIANT's overlay until 3c. Included by ui.c (ui_draw.c routes the layers drawn here); the code
 * after ux_section.c. */
#ifndef UX_LAYER_H
#define UX_LAYER_H

static int ux_layer_owns(uint32_t l);   /* layer l (ui.c LAYER_*) is drawn here */
static void ux_layer_draw(void);        /* ui.layer's screen (ui_draw.c ui_draw_body; the MACRO latch's too) */
static void uxl_chip(uint32_t n, const char *s, int on);   /* STATE's n-th chip (ui_layer.c layer_state, lst_chip) */

#endif
