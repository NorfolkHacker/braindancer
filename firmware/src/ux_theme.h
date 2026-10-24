/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's look (spec 3): the palette as native RGB565 constants of its own (independent of gfx.c's T_* tokens, which
 * carry the kept AIR look), the blend ACID -> SLIDE -> VIOLET -> ACCENT -> QUEUED, gradient text, the panel glow, the
 * blend band. Included by ux_theme.c; the unity build reaches gfx.c's RGB(), cv_* and ux_mix(). */
#ifndef UX_THEME_H
#define UX_THEME_H

#define UX_BG        RGB(0x14, 0x15, 0x18)
#define UX_BG2       RGB(0x1A, 0x1C, 0x20)
#define UX_LINE      RGB(0x30, 0x33, 0x38)
#define UX_EMPTY     RGB(0x26, 0x28, 0x2D)
#define UX_TEXT      RGB(0xE8, 0xEA, 0xED)
#define UX_MID       RGB(0x9A, 0xA0, 0xA6)
#define UX_ACID      RGB(0xB8, 0xF5, 0x3A)
#define UX_SLIDE     RGB(0x4C, 0xB8, 0xFF)
#define UX_VIOLET    RGB(0xA6, 0x4B, 0xFF)
#define UX_ACCENT    RGB(0xFF, 0x50, 0x3C)
#define UX_QUEUED    RGB(0xFF, 0xC8, 0x3C)
#define UX_ACID_LT   RGB(0xDC, 0xFA, 0x9D)
#define UX_VIOLET_LT RGB(0xD3, 0xA5, 0xFF)

static const uint16_t UX_BLEND[256];     /* ACID at 0, QUEUED at 255 (flash; tools/gen_ux_blend.py) */
static uint16_t ux_knob_col(uint32_t k);                 /* 0..3: ACID, SLIDE, VIOLET, ACCENT */
static void ux_text_grad(int32_t x, int32_t y, const aafont_t *f, const char *s, uint16_t bg);
static void ux_glow(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t level);   /* level 0..3 (0: none) */
static void ux_band(int32_t y, int32_t h);               /* the blend as a solid band across the canvas */
static void ux_topline(void);                            /* the 2 px blend line at canvas y 0 */

#endif
