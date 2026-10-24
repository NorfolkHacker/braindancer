/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's shared page pieces (ux_widgets.h). Each piece is a one-shot strip like JIANT's head and foot: cv_begin, drawn,
 * cv_blit_cells (only the cells that changed are sent). Text goes through cv_text* so the host lint sees it; each
 * column is a GFX_HOOK_CELL its texts must lie in; header and footer texts declare they sit centred up and
 * down in their rows (GFX_HOOK_ALIGN). */
#include "ux_widgets.h"

#ifndef UX_NOW
#define UX_NOW() fm1_ms                                /* the UI's ms clock (tests set their own) */
#endif
static int32_t ux_hot_k = -1;
static uint32_t ux_hot_t;

#define UX_DIM(c, pct) ux_mix((c), UX_BG, (pct))        /* c, pct % of the way to BG */

static void ux_header(uint32_t trk, const char *sound, const char *right)
{
    char pre[8];
    int32_t x = 8, ty = 3;                              /* (the capitals' band centred in rows 2..15) */
    pre[0] = 'T'; pre[1] = (char)('0' + (trk % 10u)); pre[2] = ' '; pre[3] = (char)0xB7; pre[4] = ' '; pre[5] = 0;      /* (Latin-1 middle dot, as JIANT's strings) */
    cv_begin(240, UX_H_LINE + UX_H_HEAD, UX_BG);
    ux_topline();
    GFX_HOOK_ALIGN(0, UX_H_LINE, 0, UX_H_LINE + UX_H_HEAD, AL_V, "header left");
    if (trk)                                            /* (0: no track, the name alone: HOME's ACID FM) */
        x = cv_text_on(x, ty, &UF_HDR, pre, UX_MID, UX_BG);
    if (sound && *sound)
        ux_text_grad(x, ty, &UF_HDR, sound, UX_BG);
    if (right && *right) {
        GFX_HOOK_ALIGN(0, UX_H_LINE, 0, UX_H_LINE + UX_H_HEAD, AL_V, "header right");
        cv_text_r(232, ty, &UF_HDR, right, UX_MID, UX_BG);
    }
    cv_blit_cells(0, UX_Y_LINE);
}

static void ux_knob_touch(uint32_t k)
{
    ux_hot_k = (int32_t)(k & 3u);
    ux_hot_t = UX_NOW();
}

static int32_t ux_knob_hot(void)
{
    return ux_hot_k >= 0 && UX_NOW() - ux_hot_t < UX_HOT_MS ? ux_hot_k : -1;
}

static void ux_knobs(const ux_knob_t k[4], int32_t hot)
{
    int32_t i;
    if (hot < 0)
        hot = ux_knob_hot();
    cv_begin(240, UX_H_KNOBS, UX_BG);
    for (i = 0; i < 4; i++) {
        int32_t x = UX_COL_X0 + i * UX_COL_PITCH, on = i == hot, fw;
        uint16_t col = ux_knob_col((uint32_t)i), bg = on ? UX_BG2 : UX_BG;
        if (on)
            cv_rect(x, 0, UX_COL_W, 38, UX_BG2);
        GFX_HOOK_CELL(x, 0, x + UX_COL_W, 38);
        cv_rect(x, 0, UX_COL_W, 2, on ? col : UX_LINE);                    /* the column's rule: its knob's colour while hot */
        cv_text_on(x + 2, 4, &UF_LBL, k[i].label, on ? UX_TEXT : UX_MID, bg);
        cv_text_on(x + 2, 14, &UF_VAL, k[i].value, on ? UX_TEXT : ux_mix(UX_BG, UX_TEXT, 78), bg);
        fw = (int32_t)k[i].bar * (UX_COL_W - 4) / 255;
        cv_rect(x + 2, 32, UX_COL_W - 4, 3, UX_LINE);
        if (fw > 0)
            cv_rect(x + 2, 32, fw, 3, on ? col : UX_DIM(col, 35));
    }
    cv_blit_cells(0, UX_Y_KNOBS);
}

static void ux_panel_begin(uint32_t glow_level)
{
    cv_begin(240, UX_H_PANEL, UX_BG);
    ux_glow(0, 0, 240, (int32_t)UX_H_PANEL, glow_level);
}

static void ux_panel_end(void)
{
    cv_blit_cells(0, UX_Y_PANEL);
}

static void ux_status(const char *text, uint16_t col)
{
    cv_begin(240, UX_H_STATUS, UX_BG);
    if (text && *text) {
        GFX_HOOK_ALIGN(0, 0, 0, (int32_t)UX_H_STATUS, AL_V, "status line");
        cv_text_on(8, 4, &UF_HDR, text, col, UX_BG);       /* (the 8 px capitals centred in 20 rows) */
    }
    cv_blit_cells(0, UX_Y_STATUS);
}

static void ux_transport(char *b, uint32_t n)
{
    char d[8];
    if (n < 2u)
        return;
    str_cpy(b, song.playing ? UX_GL_PLAY " " : UX_GL_STOP " ", n);
    fmt_int(d, song.g[G_BPM]);
    str_cpy(b + str_len(b), d, n - str_len(b));
    str_cpy(b + str_len(b), " BPM", n - str_len(b));
}

static void ux_footer(const char *where, const char *right)
{
    int32_t ty = 2;                                     /* (the 6 px capitals centred in 14 rows) */
    cv_begin(240, UX_H_FOOT, UX_BG);
    ux_band(0, (int32_t)UX_H_FOOT);
    if (where && *where) {
        GFX_HOOK_ALIGN(0, 0, 0, (int32_t)UX_H_FOOT, AL_V, "footer left");
        cv_text_on(8, ty, &UF_LBL, where, UX_BG, UX_BLEND[(8 + text_w(&UF_LBL, where) / 2) * 255 / 239]);
    }
    if (right && *right) {
        int32_t w = text_w(&UF_LBL, right);
        GFX_HOOK_ALIGN(0, 0, 0, (int32_t)UX_H_FOOT, AL_V, "footer right");
        cv_text_on(232 - w, ty, &UF_LBL, right, UX_BG, UX_BLEND[(232 - w / 2) * 255 / 239]);
    }
    cv_blit_cells(0, UX_Y_FOOT);
}

static uint16_t ux_step_col(const step_t *s, uint16_t prev)
{
    if (s->time == ST_TIE)
        return prev == UX_EMPTY ? UX_EMPTY : UX_DIM(prev, 45);
    if (s->time != ST_NOTE || !s->n)
        return UX_EMPTY;
    return s->flags & SF_ACCENT ? UX_ACCENT : s->flags & SF_SLIDE ? UX_SLIDE : UX_ACID;
}

static void ux_strip(int32_t y, const step_t *steps, uint32_t n16, int32_t edit, int32_t play)
{
    uint32_t i;
    uint16_t prev = UX_EMPTY;
    for (i = 0; i < 16u; i++) {
        int32_t x = UX_STRIP_X + (int32_t)i * UX_STRIP_PITCH;
        uint16_t c = i < n16 ? ux_step_col(&steps[i], prev) : UX_BG2;
        if (i < n16 && steps[i].time != ST_TIE)
            prev = c;                                   /* (a held run keeps its note's colour) */
        if ((int32_t)i == play)
            c = c == UX_EMPTY ? UX_LINE : ux_mix(c, UX_TEXT, 45);
        cv_rect(x, y, UX_STRIP_W, UX_STRIP_H, c);
        if ((int32_t)i == edit)
            cv_frame(x, y, UX_STRIP_W, UX_STRIP_H, UX_TEXT);
    }
}
