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

/* (3b task 7 fix) the face a knob's value is drawn in: UF_VAL, else (a name wider than its column, a MOD MATRIX destination's
 * "CUTOFF": the same name as the EDIT knob's) UF_HDR, else UF_LBL (every knob label fits it), its capitals centred where
 * UF_VAL's are (*dy: rows down from UF_VAL's line) */
static const aafont_t *ux_val_face(const char *v, int32_t *dy)
{
    if (text_w(&UF_VAL, v) <= UX_COL_W - 4) {
        *dy = 0;
        return &UF_VAL;
    }
    if (text_w(&UF_HDR, v) <= UX_COL_W - 4) {
        *dy = UF_VAL_CAP_Y + UF_VAL_CAP_H / 2 - UF_HDR_CAP_Y - UF_HDR_CAP_H / 2;
        return &UF_HDR;
    }
    *dy = UF_VAL_CAP_Y + UF_VAL_CAP_H / 2 - UF_LBL_CAP_Y - UF_LBL_CAP_H / 2;
    return &UF_LBL;
}
static int32_t uxf_w(const char *s);
static void uxf_text(int32_t x, int32_t y, const char *s, uint16_t ink, uint16_t bg);
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
        if (uxf_w(k[i].label) <= UX_COL_W - 4)          /* (3c-1 task 6 fix: a two-word label's space as the footer's, */
            uxf_text(x + 2, 4, k[i].label, on ? UX_TEXT : UX_MID, bg);   /* "DJ FILTER" not "DJFILTER", when it fits) */
        else
            cv_text_on(x + 2, 4, &UF_LBL, k[i].label, on ? UX_TEXT : UX_MID, bg);
        {
            int32_t dy;
            const aafont_t *vf = ux_val_face(k[i].value, &dy);
            cv_text_on(x + 2, 14 + dy, vf, k[i].value, on ? UX_TEXT : k[i].off ? ux_mix(UX_BG, UX_MID, 55) :   /* (off: not in */
                       ux_mix(UX_BG, UX_TEXT, 78), bg);                                            /* use, dim) */
        }
        fw = (int32_t)k[i].bar * (UX_COL_W - 4) / 255;
        cv_rect(x + 2, 32, UX_COL_W - 4, 3, UX_LINE);
        if (fw > 0)
            cv_rect(x + 2, 32, fw, 3, on ? col : UX_DIM(col, 35));
    }
    cv_blit_cells(0, UX_Y_KNOBS);
}

#ifndef UX_TIGHT
#define UX_TIGHT() load_tight                          /* (voice.c; the host tests set their own) */
#endif
static ux_pace_t ux_pace_at(uint32_t now, uint32_t last, uint32_t tight, uint32_t since_shed, uint32_t gov_lite,
                            uint32_t sys_load, uint32_t glow, uint32_t gap, uint32_t slow)
{
    ux_pace_t r;
    if (tight || sys_load)
        gap = slow;
    r.draw = (uint8_t)(since_shed >= UX_SHED_HOLD_MS && !gov_lite && now - last >= gap);
    r.glow = (uint8_t)(tight ? 0u : glow);
    return r;
}
static ux_pace_t ux_pace(uint32_t now, uint32_t last, uint32_t tight, uint32_t since_shed, uint32_t gov_lite, uint32_t sys_load,
                         uint32_t glow)
{
    return ux_pace_at(now, last, tight, since_shed, gov_lite, sys_load, glow, UX_VIS_MS, UX_VIS_SLOW_MS);
}
static uint32_t ux_glow_eff(uint32_t glow_level) { return ux_pace(0, 0, UX_TIGHT(), UX_NO_SHED, 0, 0, glow_level).glow; }

static void ux_panel_begin(uint32_t glow_level)
{
    cv_begin(240, UX_H_PANEL, UX_BG);
    ux_glow(0, 0, 240, (int32_t)UX_H_PANEL, ux_glow_eff(glow_level));
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
    b[0] = 0;
    if (song.g[G_CLOCK])                                /* (3c-1 task 8: following USB or TRS) */
        str_cpy(b, UX_GL_SYNC " ", n);
    str_cpy(b + str_len(b), song.playing ? UX_GL_PLAY " " : UX_GL_STOP " ", n - str_len(b));
    fmt_int(d, song.g[G_BPM]);
    str_cpy(b + str_len(b), d, n - str_len(b));
    str_cpy(b + str_len(b), " BPM", n - str_len(b));
}

/* (3b final review M4) the footer's words apart: UF_LBL's space is 1.7 px, so "NEXT OP ENV" read as "NEXT OPENV"; each
 * space in the footer UXF_SP px wider (the face itself stays: the knob labels fill their columns) */
#define UXF_SP 2
static int32_t uxf_w(const char *s)
{
    int32_t n = 0;
    const char *p;
    for (p = s; *p; p++)
        n += *p == ' ';
    return text_w(&UF_LBL, s) + n * UXF_SP;
}
static void uxf_text(int32_t x, int32_t y, const char *s, uint16_t ink, uint16_t bg)
{
    char b[64];
    uint32_t i = 0, j, n = 0;
    while (s[i] && i < sizeof b - 1u) { b[i] = s[i]; i++; }
    b[i] = 0;
    for (i = 0; b[i];) {
        char c;
        int32_t px;
        if (b[i] == ' ') { i++; n++; continue; }
        for (j = i; b[j] && b[j] != ' '; j++) {}
        c = b[i]; b[i] = 0;
        px = text_w(&UF_LBL, b);                        /* (the words before, as one string: their kerning) */
        b[i] = c;
        c = b[j]; b[j] = 0;
        cv_text_on(x + px + (int32_t)n * UXF_SP, y, &UF_LBL, b + i, ink, bg);
        b[j] = c;
        i = j;
    }
}
static void ux_footer(const char *where, const char *right)
{
    int32_t ty = 2;                                     /* (the 6 px capitals centred in 14 rows) */
    cv_begin(240, UX_H_FOOT, UX_BG);
    ux_band(0, (int32_t)UX_H_FOOT);
    if (where && *where) {
        GFX_HOOK_ALIGN(0, 0, 0, (int32_t)UX_H_FOOT, AL_V, "footer left");
        uxf_text(8, ty, where, UX_BG, UX_BLEND[(8 + uxf_w(where) / 2) * 255 / 239]);
    }
    if (right && *right) {
        int32_t w = uxf_w(right);
        GFX_HOOK_ALIGN(0, 0, 0, (int32_t)UX_H_FOOT, AL_V, "footer right");
        uxf_text(232 - w, ty, right, UX_BG, UX_BLEND[(232 - w / 2) * 255 / 239]);
    }
    cv_blit_cells(0, UX_Y_FOOT);
}

/* (3c-1 task 2 fix 1: drum, a DRUM track's step: its hits, ACCENT with an accent) */
static uint16_t ux_step_col(const step_t *s, uint16_t prev, int drum)
{
    if (s->time == ST_TIE)
        return prev == UX_EMPTY ? UX_EMPTY : UX_DIM(prev, 45);
    if (drum)
        return s->time == ST_NOTE && (s->hit || s->n) ? (s->acc || (s->n && (s->flags & SF_ACCENT)) ? UX_ACCENT : UX_ACID) :
               UX_EMPTY;                                /* (a hit or a GM note on its lane; its accent) */
    if (s->time != ST_NOTE || !s->n)
        return UX_EMPTY;
    return s->flags & SF_ACCENT ? UX_ACCENT : s->flags & SF_SLIDE ? UX_SLIDE : UX_ACID;
}

static void ux_strip(int32_t y, const step_t *steps, uint32_t n16, int32_t edit, int32_t play, uint16_t prev, int drum)
{
    uint32_t i;
    for (i = 0; i < 16u; i++) {
        int32_t x = UX_STRIP_X + (int32_t)i * UX_STRIP_PITCH;
        uint16_t c = i < n16 ? ux_step_col(&steps[i], prev, drum) : UX_BG2;
        if (i < n16 && steps[i].time != ST_TIE)
            prev = c;                                   /* (a held run keeps its note's colour) */
        if ((int32_t)i == play)
            c = c == UX_EMPTY ? UX_LINE : ux_mix(c, UX_TEXT, 45);
        cv_rect(x, y, UX_STRIP_W, UX_STRIP_H, c);
        if ((int32_t)i == edit)
            cv_frame(x, y, UX_STRIP_W, UX_STRIP_H, UX_TEXT);
    }
}
