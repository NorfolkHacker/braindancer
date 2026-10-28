/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* Renders of the firmware UI (firmware/src/ui*.c on tests/ui_test.c's stubs), the layout lint and the host draw cost.
 * (ACID FM 0.3: one palette, ACID, and one skin, AIR's look; until then every screen in each of JIANT's 11 palettes,
 * in STYLE FLAT and LINE and in SKIN COLORS, HUD and AIR, with GREY's gray and MONO's neutral checks.)
 *   ui_render OUTDIR [SLOTDIR]   (run by tests/run_tests.sh; tests/ui_render.py makes the PNGs;
 *                                 SLOTDIR: filmstrips of the rolling digits, ACID)
 * Layout lint, over every screen below in every palette, every page of every engine and every value of every
 * column: the ink box of every text and icon drawn (gfx.c GFX_HOOK_TEXT) as it lands on the screen. A finding:
 * a box off the screen, cut by its canvas, partly covered by a later strip or fill, overlapping another box, or
 * ellipsised when it is not free text (labels and values must fit; names and messages may be ellipsised), or
 * spilling out of its cell: touching a rounded rectangle of its canvas (gfx.c cv_rrect: a cell, card, row, button,
 * GFX_HOOK_CELL) without lying inside it, though still on the screen (the FX map's REVERSE), or drawn in the
 * colour it is drawn on (gfx.c GFX_HOOK_INK: a text, icon or keycap label that cannot be seen; MONO's tokens share
 * values, DIM and RAISE its one grey).
 * A text or icon on a divider (GFX_HOOK_RULE: a rule between the strips; cv_rule's are cells) is a finding too.
 * Rolling digits: every frame of a header BPM roll and a card roll, up and down (lint).
 * FM6's algorithm charts (0.9.6; 3b task 5: EDIT > OPERATOR's, ux_section.c uxs_fm6_draw, each operator through
 * UXS_FM6_HOOK), every algorithm: no two operators' dots overlapping or touching, every one inside the panel.
 * Output: OUTDIR/ppm/ACID_<screen>.ppm, OUTDIR/report.txt (findings, ellipsised
 * free text, the draw cost), OUTDIR/text_audit.tsv (per screen: texts, icons and keycaps with their ink-box px,
 * the ellipsised ones, the texts closer than 2 px to their cell's edge, the words).
 * Alignment (gfx.c GFX_HOOK_ALIGN): every place where a text, icon or keycap is meant to sit centred (in a cell, a
 * chip, a button, a row, a column, a knob, a strip) or on a line shared with its neighbours (an icon on its label's
 * line, a word on its keycap's, a unit on its value's baseline) declares its box; the ink drawn there (a text: its
 * glyphs' boxes across, its capitals' band up and down; an icon: its cell's nibbles; a keycap: its pill; also the
 * keycaps' labels in their pills, from the data, and rows of keys or bars against their panel) is measured against
 * it, on every screen above, the page / value sweep, and align_sweeps (every value
 * each place can show). 0.5 px is the odd pixel left over (it goes left / up); 1 px or more is a finding.
 * OUTDIR/align.txt: per place, how often measured, the worst offsets and where.
 * Exit 1 on a finding or an alignment 1 px or more off. */
#define KC_REF 1                 /* (0.9.0) ui_keycaps.h: the pills as nibbles too (al_keycaps; KC_DATA is Huffman-coded) */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

typedef struct { int16_t x0, y0, x1, y1; uint8_t flags; char s[28]; } rbox_t;
static rbox_t pend[256], scr[1200];
static int16_t cells[64][4];                    /* GFX_HOOK_CELL: the cells and cards of the canvas being drawn */
static uint32_t ncells, nspill;
static FILE *aud;                               /* OUTDIR/text_audit.tsv: GREY, the text of each screen */
static char tight[400];                         /* .. the texts of this screen closer than 2 px to their cell's edge */
static uint32_t ntight;
static uint32_t npend, nscr, nfind, nfree;
static uint64_t px_visited, n_text, blit_px;
static FILE *rep;
static const char *cur_name = "";
static char free_seen[64][40];
static uint32_t nfree_seen;
/* the alignment check (gfx.c GFX_HOOK_ALIGN): a place declares the box its next item(s) are meant to be centred in,
 * across and / or up and down; the items' ink as drawn (a text: its glyphs' boxes across, its capitals' band up and
 * down, a text of no letter or figure: across only; an icon: the ink of its cell; a keycap: its pill; a cushion: the
 * cushion) against the box: the offset of the ink's centre from the box's, in half pixels. 0.5 is the odd pixel
 * left over (it goes left / up: a +0.5 is counted apart); 1 px or more is a finding. Per place (its tag): how
 * often it was measured, the worst offsets and where, in OUTDIR/align.txt */
typedef struct { int32_t x0, y0, x1, y1, ix0, iy0, ix1, iy1; uint32_t mode, left, nv; const char *tag; char s[24]; } al_t;
typedef struct { const char *tag; uint32_t n, nfail, half_lo, half_hi; int32_t wh2, wv2; char ex_h[112], ex_v[112]; } alstat_t;
static al_t al_stk[8];
static uint32_t al_n, al_fail, al_lost, al_count, al_off;
static int32_t al_cy0, al_cy1;                  /* GFX_HOOK_PEN: the capitals' band of the text being hooked */
static alstat_t al_st[160];
static uint32_t al_nst;
static FILE *al_rep;
static alstat_t *al_stat(const char *tag)
{
    uint32_t i;
    for (i = 0; i < al_nst && strcmp(al_st[i].tag, tag); i++) ;
    if (i == al_nst && al_nst < 160u) { memset(&al_st[i], 0, sizeof al_st[i]); al_st[i].tag = tag; al_nst++; }
    return &al_st[i < 160u ? i : 159u];
}
/* one measurement: offsets in half pixels (h2, v2; 99: not measured) */
static void al_record(const char *tag, int32_t h2, int32_t v2, const char *s, int32_t bx0, int32_t by0, int32_t bx1,
                      int32_t by1, int32_t ix0, int32_t iy0, int32_t ix1, int32_t iy1)
{
    alstat_t *a = al_stat(tag);
    int32_t ah = h2 == 99 ? 0 : h2 < 0 ? -h2 : h2, av = v2 == 99 ? 0 : v2 < 0 ? -v2 : v2;
    a->n++;
    al_count++;
    if (h2 == 1 || v2 == 1) a->half_hi++;
    if (h2 == -1 || v2 == -1) a->half_lo++;
    if (ah > a->wh2 || (!a->ex_h[0] && h2 != 99)) {
        if (ah > a->wh2) a->wh2 = ah;
        snprintf(a->ex_h, sizeof a->ex_h, "%+.1f '%s' %s ink x %d..%d box %d..%d", h2 / 2.0, s, cur_name, ix0, ix1, bx0, bx1);
    }
    if (av > a->wv2 || (!a->ex_v[0] && v2 != 99)) {
        if (av > a->wv2) a->wv2 = av;
        snprintf(a->ex_v, sizeof a->ex_v, "%+.1f '%s' %s ink y %d..%d box %d..%d", v2 / 2.0, s, cur_name, iy0, iy1, by0, by1);
    }
    if (ah >= 2 || av >= 2) {
        a->nfail++;
        al_fail++;
        if (al_rep && a->nfail <= 3u)
            fprintf(al_rep, "OFF  %-28s %-26s '%s' across %+.1f, up/down %+.1f  (ink %d,%d-%d,%d box %d,%d-%d,%d)\n", tag, cur_name,
                    s, h2 == 99 ? 0.0 : h2 / 2.0, v2 == 99 ? 0.0 : v2 / 2.0, ix0, iy0, ix1, iy1, bx0, by0, bx1, by1);
    }
}
static void hk_align(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t mode, const char *tag)
{
    al_t *a;
    if (al_off) return;                         /* (the draw cost: timed without the check)  */
    if (al_n >= 8u) { al_lost++; return; }
    a = &al_stk[al_n++];
    memset(a, 0, sizeof *a);
    a->x0 = x0; a->y0 = y0; a->x1 = x1; a->y1 = y1; a->mode = mode; a->tag = tag;
    a->left = mode >> 8 ? mode >> 8 : 1u;
    a->ix0 = a->iy0 = 1 << 20; a->ix1 = a->iy1 = -(1 << 20);
}
/* an item drawn (cell: a cv_rrect, else ink): its extent across (x0..x1), up and down (y0..y1; vy 0: none), to the
 * place declared last (AL_PASS, its item done: to the one before it too) */
static void al_item_k(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int vy, const char *s, int cell)
{
    al_t *a;
    int32_t h2 = 99, v2 = 99;
    uint32_t pass;
    if (!al_n) return;
    a = &al_stk[al_n - 1u];
    if (!(a->mode & 32u) != !cell) return;          /* (AL_CELLS: its cells only; else ink only) */
    if (x0 < a->ix0) a->ix0 = x0;
    if (x1 > a->ix1) a->ix1 = x1;
    if (vy) {
        if (y0 < a->iy0) a->iy0 = y0;
        if (y1 > a->iy1) a->iy1 = y1;
        a->nv++;
    }
    if (!a->s[0] || strlen(a->s) + strlen(s) + 2u < sizeof a->s)
        snprintf(a->s + strlen(a->s), sizeof a->s - strlen(a->s), "%s%s", a->s[0] ? "+" : "", s);
    if (--a->left) return;
    al_n--;
    pass = a->mode & 16u;
    if (a->mode & 1u) h2 = (a->ix0 + a->ix1) - (a->x0 + a->x1);       /* AL_H: the centres */
    if (a->mode & 8u) h2 = 2 * (a->ix1 - a->x1);                      /* AL_R: the right edges */
    if (a->nv && (a->mode & 2u)) v2 = (a->iy0 + a->iy1) - (a->y0 + a->y1);
    if (a->nv && (a->mode & 4u)) v2 = 2 * (a->iy1 - a->y1);           /* AL_B: the baselines (capitals' bottoms) */
    al_record(a->tag, h2, v2, a->s, a->x0, a->y0, a->x1, a->y1, a->ix0, a->iy0, a->ix1, a->iy1);
    if (pass) al_item_k(x0, y0, x1, y1, vy, s, cell);
}
static void al_item(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int vy, const char *s) { al_item_k(x0, y0, x1, y1, vy, s, 0); }
static int al_alnum(const char *s)
{
    for (; *s; s++)
        if ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9')) return 1;
    return 0;
}
static void al_text(int32_t x0, int32_t y0, int32_t x1, int32_t y1, const char *s, uint32_t flags)
{
    if (!al_n || ((flags & 4u) && !strcmp(s, "icon"))) return;      /* (an icon: GFX_HOOK_ICON) */
    if (flags & 4u) al_item(x0, y0, x1, y1, 1, s);                    /* a keycap: its pill */
    else al_item(x0, al_cy0, x1, al_cy1, al_alnum(s), s);
}
static void al_flush(void)                      /* a canvas blitted or begun: declared places left without their items */
{
    uint32_t i;
    for (i = 0; i < al_n; i++)
        if (al_rep && al_lost + i < 12u) fprintf(al_rep, "LOST %-28s %s (declared, nothing drawn after it)\n", al_stk[i].tag, cur_name);
    al_lost += al_n;
    al_n = 0;
}
static int32_t icon_cell(uint32_t *size, uint32_t id);
static void hk_icon(int32_t x, int32_t y, uint32_t size, uint32_t id);   /* (after the icons: their cells' nibbles) */
#define GFX_HOOK_ALIGN(x0, y0, x1, y1, mode, tag) hk_align(x0, (y0) + cv_oy, x1, (y1) + cv_oy, mode, tag)
#define GFX_HOOK_PEN(y, f) (al_n ? (al_cy0 = (y) + cv_oy + ag((f), glyph((f), 'H')).by, al_cy1 = al_cy0 + ag((f), glyph((f), 'H')).bh) : 0)
#define GFX_HOOK_ITEM(x0, y0, x1, y1) al_item(x0, (y0) + cv_oy, x1, (y1) + cv_oy, 1, "cushion")
#define GFX_HOOK_ICON(x, y, size, id) hk_icon(x, (y) + cv_oy, size, id)
static char splash_seen[96];                    /* the splash scene: its texts as drawn, '|' between (draw) */
static uint8_t in_splash;
static void hk_text(int32_t x0, int32_t y0, int32_t x1, int32_t y1, const char *s, uint32_t flags)
{
    al_text(x0, y0, x1, y1, s, flags);
    if (in_splash && strlen(splash_seen) + strlen(s) + 2u < sizeof splash_seen)
        snprintf(splash_seen + strlen(splash_seen), sizeof splash_seen - strlen(splash_seen), "%s%s", splash_seen[0] ? "|" : "", s);
    n_text++;
    if (npend >= 256u) return;
    pend[npend] = (rbox_t){(int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1, (uint8_t)flags, {0}};
    strncpy(pend[npend].s, s, 27);
    npend++;
}
static void hk_cell(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    al_item_k(x0, y0, x1, y1, 1, "cell", 1);
    if (ncells < 64u) {
        cells[ncells][0] = (int16_t)x0; cells[ncells][1] = (int16_t)y0;
        cells[ncells][2] = (int16_t)x1; cells[ncells][3] = (int16_t)y1;
        ncells++;
    }
}
static void hk_blit(uint32_t x, uint32_t y, uint32_t r0, uint32_t w, uint32_t h);
static void hk_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
/* STYLE LINE: the dividers drawn on the screen (ui_draw.c lcd_rule) while no later blit or fill covers them */
static int16_t rules[32][4];
static uint32_t nrules;
static void hk_rule(uint32_t x, uint32_t y, uint32_t w, uint32_t h)   /* (3c-2 task 1b: a NOTICE's holes gone, all of it) */
{
    if (nrules < 32u) {
        rules[nrules][0] = (int16_t)x; rules[nrules][1] = (int16_t)y;
        rules[nrules][2] = (int16_t)(x + w); rules[nrules][3] = (int16_t)(y + h);
        nrules++;
    }
}
static void rules_cover(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    uint32_t i, k = 0;
    for (i = 0; i < nrules; i++)
        if (!(rules[i][0] >= x0 && rules[i][2] <= x1 && rules[i][1] >= y0 && rules[i][3] <= y1))
            memcpy(rules[k++], rules[i], sizeof rules[i]);
    nrules = k;
}
#define GFX_HOOK_TEXT(x0, y0, x1, y1, s, flags) hk_text(x0, y0, x1, y1, s, flags)
#define GFX_HOOK_BLIT(x, y, r0) hk_blit(x, y, r0, cv_w, cv_h)
#define GFX_HOOK_BEGIN() (npend = ncells = 0, al_flush())
#define GFX_HOOK_CELL(x0, y0, x1, y1) hk_cell(x0, y0, x1, y1)
#define GFX_HOOK_RULE(x, y, w, h) hk_rule(x, y, w, h)
static uint32_t nink;                           /* GFX_HOOK_INK: inks drawn on their own colour */
static void hk_ink(uint16_t ink, uint16_t bg);
#define GFX_HOOK_INK(ink, bg) hk_ink(ink, bg)
#define GFX_HOOK_PIXELS(n) (px_visited += (n))
#define UI_FILL_HOOK(x, y, w, h) hk_fill(x, y, w, h)
static void hk_overlay(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
#define GFX_HOOK_COVER(x0, y0, x1, y1) hk_overlay(x0, y0, x1, y1)
static void fm6_hook(int32_t k, int32_t x, int32_t y, int32_t r);
#define UXS_FM6_HOOK(k, x, y, r) fm6_hook(k, x, y, r)
/* MENU > LARGE: the tall cards' values by the face they got (ui_draw.c LARGE_HOOK), the characters L lacked */
static uint64_t lg_n[4];
static char lg_miss[96], lg_wide[16][12], lg_small[16][12];
static uint32_t lg_nwide, lg_nsmall;
static void lg_hook(const char *s, int why);
#define LARGE_HOOK(s, why) lg_hook(s, why)
#define UI_TEST_NO_MAIN 1
#define PF_MS 900u                                       /* (0.6.61) the page name drawn (ui_test.c: off) */
#include "ui_test.c"

static uint8_t large_on;                         /* the LARGE pass: every scene with MENU > LARGE ON */
static void lg_hook(const char *s, int why)
{
    uint32_t i;
    lg_n[why & 3]++;
    if (why == 1)
        for (; *s; s++) {
            uint32_t c = fold(&AF_L, (uint8_t)*s);
            if (glyph_at(&AF_L, c) < 0 && !strchr(lg_miss, (int)c) && strlen(lg_miss) + 1u < sizeof lg_miss)
                lg_miss[strlen(lg_miss)] = (char)c;
        }
    if (why == 2 || why == 3) {                     /* a few of each, distinct */
        char (*l)[12] = why == 2 ? lg_wide : lg_small;
        uint32_t *n = why == 2 ? &lg_nwide : &lg_nsmall;
        for (i = 0; i < *n && strcmp(l[i], s); i++) ;
        if (i == *n && *n < 16u) snprintf(l[(*n)++], 12, "%s", s);
    }
}

/* an icon drawn: its ink, read from its cell's nibbles here (not icons.c icon_ink's table: measured, not trusted) */
/* n alpha values of a cell from its bit stream (the canonical Huffman code AI_HC, as gen_aa_font.py huff_unpack:
 * decoded here bit by bit, not by gfx.c's table) */
static void icon_unpack(const uint8_t *d, uint8_t *out, uint32_t n)
{
    uint32_t k = 0, pos = 0;
    while (k < n) {
        uint32_t code = 0, first = 0, idx = 0, len, s, r;
        for (len = 0; len < 15u; len++) {
            code |= (d[pos >> 3] >> (7u - (pos & 7u))) & 1u;
            pos++;
            if (code - first < AI_HC[len]) break;
            idx += AI_HC[len];
            first = (first + AI_HC[len]) << 1;
            code <<= 1;
        }
        s = AI_HC[15u + idx + code - first];
        for (r = 0; r <= s >> 4 && k < n; r++) out[k++] = (uint8_t)(s & 15u);
    }
}
static void hk_icon(int32_t x, int32_t y, uint32_t size, uint32_t id)
{
    int32_t k, x0 = 99, y0 = 99, x1 = -1, y1 = -1;
    uint32_t i;
    const uint8_t *c;
    uint8_t cell[24 * 24];
    if (!al_n || (k = icon_cell(&size, id)) < 0) return;
    c = size == 24u ? AI24_DATA + AI24_OFF[k] : size == 16u ? AI16_DATA + AI16_OFF[k] : AI12_DATA + AI12_OFF[k];
    icon_unpack(c, cell, size * size);
    for (i = 0; i < size * size; i++)
        if (cell[i]) {
            int32_t px = (int32_t)(i % size), py = (int32_t)(i / size);
            if (px < x0) x0 = px;
            if (px + 1 > x1) x1 = px + 1;
            if (py < y0) y0 = py;
            if (py + 1 > y1) y1 = py + 1;
        }
    if (cv_flip) { int32_t t = x0; x0 = (int32_t)size - x1; x1 = (int32_t)size - t; }   /* (mirrored: the metronome) */
    al_item(x + x0, y + y0, x + x1, y + y1, 1, "icon");
}

/* (0.9.6; 3b task 5: the algorithm on EDIT's FM6 pages, ux_section.c uxs_fm6_draw) the FM6 chart lint: each operator's
 * dot (its centre, its radius, Q4 px) as drawn; at the sixth the six checked: apart by a pixel at least, and inside the
 * panel */
static int32_t fmp_x[6], fmp_y[6], fmp_r[6];
static uint32_t fmp_charts;
static void finding(const char *what, const rbox_t *b, const rbox_t *o);
static void fm6_hook(int32_t k, int32_t x, int32_t y, int32_t r)
{
    int32_t i, j;
    fmp_x[k] = x; fmp_y[k] = y; fmp_r[k] = r;
    if (k < 5)
        return;
    for (i = 0; i < 6; i++) {
        rbox_t b = {(int16_t)((fmp_x[i] - fmp_r[i]) >> 4), (int16_t)((fmp_y[i] - fmp_r[i]) >> 4), (int16_t)((fmp_x[i] + fmp_r[i]) >> 4),
                    (int16_t)((fmp_y[i] + fmp_r[i]) >> 4), 0, {0}}, o = b;
        snprintf(b.s, sizeof b.s, "FM6 OP%d", (int)i + 1);
        if (b.x0 < 4 || b.x1 > 236 || b.y0 < 0 || b.y1 >= (int32_t)cv_h) {
            snprintf(o.s, sizeof o.s, "panel");
            finding("FM6 operator off the panel", &b, &o);
        }
        for (j = 0; j < i; j++) {
            int64_t dx = fmp_x[i] - fmp_x[j], dy = fmp_y[i] - fmp_y[j], d = fmp_r[i] + fmp_r[j] + 16;
            if (dx * dx + dy * dy < d * d) {
                snprintf(o.s, sizeof o.s, "FM6 OP%d", (int)j + 1);
                finding("FM6 operators overlap", &b, &o);
            }
        }
    }
    fmp_charts++;
}

static void finding(const char *what, const rbox_t *b, const rbox_t *o)
{
    static char seen[600][64];
    static uint32_t nseen;
    char key[64];
    uint32_t i;
    nfind++;
    snprintf(key, sizeof key, "%.20s|%.14s|%.24s", cur_name, what, b->s);
    for (i = 0; i < nseen && strcmp(seen[i], key); i++) ;
    if (i < nseen) return;                       /* each finding once per screen */
    if (nseen < 600u) strcpy(seen[nseen++], key);
    else return;
    fprintf(rep, "LINT %-22s %-32s '%s' (%d,%d)-(%d,%d)", cur_name, what, b->s, b->x0, b->y0, b->x1, b->y1);
    if (o) fprintf(rep, " / '%s' (%d,%d)-(%d,%d)", o->s, o->x0, o->y0, o->x1, o->y1);
    fputc('\n', rep);
}
/* a rectangle of the screen is drawn again: boxes inside it are gone, boxes it covers in part are hidden */
static void cover(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    uint32_t i, k = 0;
    for (i = 0; i < nscr; i++) {
        rbox_t *b = &scr[i];
        int in = b->x0 >= x0 && b->x1 <= x1 && b->y0 >= y0 && b->y1 <= y1;
        int touch = b->x0 < x1 && x0 < b->x1 && b->y0 < y1 && y0 < b->y1;
        if (in) continue;
        if (touch && !(b->flags & 48u)) finding("hidden in part by a later draw", b, 0);
        if (touch && (b->flags & 48u)) scr[k++] = *b;   /* split across bands / scrolled: drawn on purpose */
        else scr[k++] = *b;
    }
    nscr = k;
}
static void hk_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    cover((int32_t)x, (int32_t)y, (int32_t)(x + w), (int32_t)(y + h));
    rules_cover((int32_t)x, (int32_t)y, (int32_t)(x + w), (int32_t)(y + h));
}
/* a text across two bands (the menu and the document draw 124 + 85 rows): each band draws its part, the same
 * screen box twice, cut at the band edge; together they are whole (flag 16) */
static int split_pair(rbox_t *b)
{
    uint32_t j;
    if (!(b->flags & 2u)) return 0;
    for (j = 0; j < nscr; j++) {
        rbox_t *o = &scr[j];
        if ((o->flags & 2u) && o->x0 == b->x0 && o->x1 == b->x1 && o->y0 == b->y0 && o->y1 == b->y1 && !strcmp(o->s, b->s)) {
            o->flags = (uint8_t)((o->flags & ~2u) | 16u);
            return 1;
        }
    }
    return 0;
}
/* a text or icon box that touches a cell or card of its canvas (GFX_HOOK_CELL) but spills past its edge; for the
 * audit, a text inside its smallest cell closer than 2 px to an edge */
static void contain(int32_t x, int32_t y)
{
    uint32_t i, c;
    for (i = 0; i < npend; i++) {
        const rbox_t *b = &pend[i];
        int32_t best = 1 << 30, gap = 99;
        for (c = 0; c < ncells && !(b->flags & 4u); c++) {   /* (texts only: an icon's box is its whole cell) */
            const int16_t *r = cells[c];
            int32_t a = (r[2] - r[0]) * (r[3] - r[1]), g;
            if (b->x0 < r[0] || b->x1 > r[2] || b->y0 < r[1] || b->y1 > r[3] || a >= best) continue;
            best = a;
            g = b->x0 - r[0];
            if (r[2] - b->x1 < g) g = r[2] - b->x1;
            if (b->y0 - r[1] < g) g = b->y0 - r[1];
            if (r[3] - b->y1 < g) g = r[3] - b->y1;
            gap = g;
        }
        if (gap < 2 && aud) {
            ntight++;
            if (strlen(tight) + strlen(b->s) + 8u < sizeof tight)
                snprintf(tight + strlen(tight), sizeof tight - strlen(tight), "%s'%s' %dpx", tight[0] ? ", " : "", b->s, (int)gap);
        }
        for (c = 0; c < ncells; c++) {
            const int16_t *r = cells[c];
            int touch = b->x0 < r[2] && r[0] < b->x1 && b->y0 < r[3] && r[1] < b->y1;
            int in = b->x0 >= r[0] && b->x1 <= r[2] && b->y0 >= r[1] && b->y1 <= r[3];
            int around = r[0] >= b->x0 && r[2] <= b->x1 && r[1] >= b->y0 && r[3] <= b->y1;   /* (a mark inside the box) */
            if (touch && !in && !around) {
                rbox_t t = *b, o = {(int16_t)(r[0] + x), (int16_t)(r[1] + y), (int16_t)(r[2] + x), (int16_t)(r[3] + y), 0, "cell"};
                t.x0 += (int16_t)x; t.x1 += (int16_t)x; t.y0 += (int16_t)y; t.y1 += (int16_t)y;
                nspill++;
                finding("spills out of its cell", &t, &o);
                break;
            }
        }
    }
}
/* (0.9.5) an overlay drawn over its canvas (ui_graph.c toast_pill): the cells under it are hidden by it, on purpose (the
 * page's graph goes on under a toast); a text of the page under it is a finding (the pill keeps clear of the page's
 * words: gfx.c cv_txt, MUTATE's lines, AUTO LIST's rows, MOTION's, PUNCH's; none clear: no pill) */
static void hk_overlay(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    uint32_t i, k = 0;
    al_flush();
    for (i = 0; i < npend; i++)
        if (!(pend[i].x0 < x1 && x0 < pend[i].x1 && pend[i].y0 < y1 && y0 < pend[i].y1))
            pend[k++] = pend[i];
        else {
            rbox_t o = {(int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1, 0, "toast pill"};
            finding("page text under the toast pill", &pend[i], &o);
        }
    npend = k;
    for (i = k = 0; i < ncells; i++)
        if (!(cells[i][0] < x1 && x0 < cells[i][2] && cells[i][1] < y1 && y0 < cells[i][3]))
            memmove(cells[k++], cells[i], sizeof cells[i]);
    ncells = k;
}
static void hk_blit(uint32_t x, uint32_t y, uint32_t r0, uint32_t w, uint32_t h)
{
    uint32_t i, k = 0;
    al_flush();
    contain((int32_t)x, (int32_t)y);
    ncells = 0;
    blit_px += (uint64_t)w * (h > r0 ? h - r0 : 0u);
    for (i = 0; i < npend; i++) {
        rbox_t b = pend[i];
        if (b.y1 <= (int32_t)r0) continue;           /* rows not blitted */
        if (b.y0 < (int32_t)r0) b.flags |= 2u;
        b.x0 += (int16_t)x; b.x1 += (int16_t)x; b.y0 += (int16_t)y; b.y1 += (int16_t)y;
        if (!split_pair(&b)) pend[k++] = b;
    }
    cover((int32_t)x, (int32_t)(y + r0), (int32_t)(x + w), (int32_t)(y + h));
    rules_cover((int32_t)x, (int32_t)(y + r0), (int32_t)(x + w), (int32_t)(y + h));
    for (i = 0; i < k && nscr < 1200u; i++)
        scr[nscr++] = pend[i];
    npend = 0;
}

static void lint(void)
{
    uint32_t i, j;
    for (i = 0; i < nscr; i++) {
        const rbox_t *b = &scr[i];
        if (b->x0 < 0 || b->y0 < 0 || b->x1 > 240 || b->y1 > 240) finding("off the screen", b, 0);
        if ((b->flags & 2u) && !(b->flags & 32u)) finding("cut by its canvas", b, 0);
        if ((b->flags & 1u) && !(b->flags & 8u)) finding("ellipsised (not free text)", b, 0);
        if ((b->flags & 9u) == 9u) {
            for (j = 0; j < nfree_seen && strcmp(free_seen[j], b->s); j++) ;
            if (j == nfree_seen && nfree_seen < 64u) {
                snprintf(free_seen[nfree_seen++], 40, "%s", b->s);
                fprintf(rep, "free   %-22s ellipsised '%s'\n", cur_name, b->s);
            }
            nfree++;
        }
        for (j = i + 1u; j < nscr; j++) {
            const rbox_t *o = &scr[j];
            if (b->x0 < o->x1 && o->x0 < b->x1 && b->y0 < o->y1 && o->y0 < b->y1) finding("overlaps", b, o);
        }
        for (j = 0; j < nrules; j++) {               /* a divider is a cell edge: nothing on it */
            const int16_t *r = rules[j];
            rbox_t o = {r[0], r[1], r[2], r[3], 0, "divider"};
            if (b->x0 < r[2] && r[0] < b->x1 && b->y0 < r[3] && r[1] < b->y1) finding("touches a divider", b, &o);
        }
    }
}

/* the audit of one screen (GREY): its texts (count, ink-box px), icons, keycaps, the ellipsised and tight texts,
 * and the words themselves */
static FILE *audf;
static void audit_scene(const char *name)
{
    uint32_t i, nt = 0, ni = 0, nk = 0, ne = 0, at = 0, ai = 0, ak = 0;
    char words[1200] = "";
    if (!audf) return;
    for (i = 0; i < nscr; i++) {
        const rbox_t *b = &scr[i];
        uint32_t a = (uint32_t)((b->x1 - b->x0) * (b->y1 - b->y0));
        if ((b->flags & 4u) && !strcmp(b->s, "icon")) { ni++; ai += a; continue; }
        if (b->flags & 4u) { nk++; ak += a; continue; }
        nt++; at += a;
        if (b->flags & 1u) ne++;
        if (strlen(words) + strlen(b->s) + 4u < sizeof words)
            snprintf(words + strlen(words), sizeof words - strlen(words), "%s%s", words[0] ? " | " : "", b->s);
    }
    fprintf(audf, "%s\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%s\t%s\n", name, nt, at, ni, ai, nk, ak, ne, ntight, words, tight);
}
static void hk_ink(uint16_t ink, uint16_t bg)
{
    if (ink != bg) return;
    if (nink < 20u) fprintf(rep, "%s: ink %04x drawn on its own colour (%s)\n", cur_name, ink, UI_PALETTES[settings.palette % NPALETTES].name);
    nink++;
}
static void write_ppm(const char *dir, const char *pal, const char *name)
{
    char path[512];
    uint32_t i;
    FILE *f;
    snprintf(path, sizeof path, "%s/ppm/%s_%s.ppm", dir, pal, name);
    f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", path); return; }
    fprintf(f, "P6\n240 240\n255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = swap16(host_screen[i]);
        uint8_t b[3] = {(uint8_t)((c >> 11) * 255u / 31u), (uint8_t)(((c >> 5) & 63u) * 255u / 63u), (uint8_t)((c & 31u) * 255u / 31u)};
        fwrite(b, 1, 3, f);
    }
    fclose(f);
}

/* ------------------------------------------------------------------ the screens --- */
static void pal(uint32_t p) { settings.palette = p; palette_set(p); }
static void fx_load_back(void);                 /* (3b task 8: an FX scene's LOAD, put back) */
static void state(void)                          /* a playing song with steps on every track */
{
    uint32_t i;
    fx_load_back();
    ui_power_on();
    dx_mute_set(0);
    memset(macro_v, 0, sizeof macro_v);
    if (large_on) ui_prefs |= PREF_LARGE;
    ui_prefs &= (uint8_t)~PREF_BPM_LOCK;          /* (S_HOME_REC sets it: not on the scenes after) */
    song.playing = 1;
    song.g[G_BPM] = 124;
    song.g[G_STRN] = 0;                           /* (lente_bar_long: ALL TR set) */
    clk_pos = 0; clk_step = 8; cin_total = cin_left = 0;   /* the bar's 3rd beat: the header's metronome at rest */
    for (i = 0; i < NTRK; i++) trk[i].seq_idx = 5;
    my_steps(&trk[1]);
    trk[0].step[2].flags |= SF_ACCENT;
    trk[0].step[6].flags |= SF_SLIDE;
    for (i = 0; i < SCOPE_N; i++)                 /* a stand-in signal for the scope */
        scope_buf[i] = (int16_t)((int32_t)((i * 37u) % 128u) * 200 - 12800 + (int32_t)((i % 32u) < 16u ? 3000 : -3000));
    scope_w = 0;
}
static void drum(uint32_t kit)
{
    uint32_t i;
    song.sel = 3;
    trk[3].p[P_E0] = (int16_t)kit;
    for (i = 0; i < 16u; i++) {
        step_t *s = &trk[3].step[i];
        memset(s, 0, sizeof *s);
        s->time = ST_NOTE; s->vel = 100;
        if (i % 4u == 0u) s->hit |= 1u;
        if (i == 4u || i == 12u) s->hit |= 2u, s->acc |= 2u;
        if (i == 10u) s->hit |= 4u;
        if (i % 2u == 0u) s->hit |= 8u;
        if (i == 14u) s->hit |= 16u;
        if (i == 7u || i == 15u) s->hit |= 32u;
        if (i == 3u) s->hit |= 64u;
        if (i == 0u) s->hit |= 128u, s->acc |= 128u;
    }
    trk[3].p[P_SLEN] = 32;
}
static void eng(uint32_t e) { set_engine_of(TSEL, e); }
#define ENGI_VOICE_UI 5u                         /* (0.8.5) VOICE (engines.c ENGINES[5]) */
/* the FM engine of the FM screens: DIGITAL with FELUCCA_FM4, else FM6 (DIGITAL retired: its own screens, the OP ENV /
 * OP LEVEL pages and the algorithm charts, exist only there: fm4_screen) */
#define E_FM (FELUCCA_FM4 ? ENGI_DIGITAL : ENGI_FM6)

enum { S_HOME, S_HOME_IDLE, S_HOME_VISUAL, S_HOME_VISUAL_IDLE, S_HOME_PLAY, S_HOME_DEMO, S_HOME_T5, S_HOME_TURN, S_HOME_REC,
       S_HOME_BATT0, S_MESSAGE, S_MESSAGE_KEY, S_MESSAGE_NOFILE, S_PHRASES, S_SONG_EMPTY, S_SONG, S_STEP,
       S_PATTERN, S_COMP, S_COMP_TRIG, S_COMP_ECHO, S_MUTATE, S_MORPH, S_DXIMG, S_DXSEND, S_PFLASH, S_FUGUE, S_MOTION, S_DRUM,
       S_DRUM_HAND, S_DRUM_CYM, S_MIXER, S_MIXER_PAN, S_ENV, S_ENVDEST, S_LFO, S_MOD, S_FX, S_SLICER, S_SLBUS, S_SLPITCH,
       S_DLY, S_SCL, S_SHIFT, S_ARP, S_VOICE, S_GLOBAL, S_SYSTEM, S_EDIT_ANALOG, S_EDIT_DIGITAL, S_OP_ENV, S_EDIT_SAMPLE,
       S_EDIT_GRAIN, S_ALG1, S_ALG2, S_ALG3, S_ALG4, S_ALG5, S_ALG6, S_ALG7, S_ALG8, S_OP_LEVEL, S_FM6_ALG1, S_FM6_ALG5,
       S_FM6_ALG22, S_FM6_ALG32, S_DRUMX, S_DRUMX_HIT, S_DX_SOUND, S_DX_SOUND2, S_DLG_CLEAR_SEQ, S_DLG_CLEAR_TRK,
       S_DLG_CLEAR_ALL, S_DLG_CLEAR_MOTION, S_DLG_CLEAR_PFX, S_DLG_CLEAR_SONG, S_DLG_DEL_ROW, S_DLG_INIT_SOUND,
       S_DLG_INIT_SOUND_DRUM, S_DLG_INIT_SYS, S_DLG_CPU_TEST, S_DLG_LIVE_COMMIT, S_DLG_LOAD_PAT, S_DLG_OVR_PROJ,
       S_DLG_OVR_USER, S_DLG_ERASE_USER, S_DLG_RECALL_SOUND, S_GEN, S_GEN_PARTS, S_MENU, S_MENU_SPEAKER, S_ABOUT, S_ABOUT_REC,
       S_ABOUT_CREDITS, S_ABOUT_END, S_ABOUT_PROFILE, S_ABOUT_PROFILE2, S_UBOOT, S_CALIBRATION, S_SPLASH, S_BATT0, S_BATT1,
       S_BATT2, S_BATT3, S_BATT_USB, S_MOTION_REC, S_MOTION_OFF, S_MOTION_CARD, S_SONG_HOME, S_FX_PEEK, S_FX_HELD, S_FX_WAIT,
       S_FX_HARM, S_FX_GLT, S_FX_SCR, S_FX_SHUF, S_FX_FRZ, S_MENU_HOLD, S_MENU_LEDS, S_MENU_END, S_MENU_SYSTEM, S_MENU_SLIDE,
       S_REVERB, S_REVERB_RESO, S_GLO_PEEK, S_GLO_ACTIVE, S_GLO_EXT, S_SCL_PEEK, S_SCL_ACTIVE, S_EDIT_PEEK, S_EDIT_ACTIVE,
       S_EDIT_USER, S_EDIT_DRUM, S_LAYER_HINT, S_LAYER_LOCK, S_LAYER_LOCK_FX, S_LAYER_SONG, S_LAYER_SONG_CHAIN, S_SEQ_TOOLS,
       S_SEQ_TOOLS_DRUM, S_SEQ_TOOLS_SONG, S_LENTE_PAT, S_LENTE_PAT_DRUM, S_LENTE_STRETCH, S_LENTE_BAR, S_LENTE_BAR_DRUM,
       S_LENTE_STEP, S_LENTE_STEP_COMP, S_LENTE_STEP_COMP2, S_LENTE_STEP_TIE, S_LENTE_STEP_REST, S_LENTE_STEP_DRUM,
       S_LENTE_BAR_LONG, S_LENTE_STEP_LONG, S_NAME_USER, S_NAME_TYPING, S_NAME_123, S_NAME_EMPTY, S_NAME_FULL, S_NAME_PLAYING,
       S_PROJECT_NAMED, S_SONG_NAMED, S_WORDS, S_WORDS_EN, S_WORDS_JA, S_WORDS_EDIT, S_WORDS_OFF, S_NAME_LYRIC,
       S_NAME_LYRIC_LONG, S_ROLL_EMPTY, S_ROLL_ACID, S_ROLL_CHORDS, S_ROLL_TIES, S_ROLL_LEN32, S_ROLL_HIGH, S_ROLL_LOW,
       S_ROLL_WIDE, S_ROLL_PLAYING, S_ROLL_LOCKS, S_ROLL_LOCK_HELD, S_DRUM_LOCKS, S_DRUM_LOCK_HELD, S_DRUM_LOCK_PAGE,
       S_MOTION_LOCKS, S_AUTO_LIST, S_AUTO_LIST_ADD, S_AUTO_LIST_DRUM, S_REC_LAYER, S_REC_LAYER_SET, S_REC_LAYER_SONG,
       S_MACRO_LAYER, S_MSEQ, S_DIST_PAGE, S_DLY2, S_REVERB2, S_CHORUS, S_MASTER, S_LFO_DEST, S_LFO2, S_LFO2_FREE, S_COUNTIN,
       S_COUNTIN2, S_MENU_AUDIO, S_HEAD_PLAY_REC, S_HEAD_REC_OTHER, S_HEAD_BPM_TURN, S_HEAD_BPM_LOCK, S_HEAD_BPM_LOCKED,
       S_HEAD_GLO_TURN, S_HEAD_DOWNBEAT, S_HEAD_BEAT_2, S_SAVE_PROJECT, S_SAVE_MORE_END, S_SAVE_SOUND,
       S_FXL_SENDS, S_FXL_DIST, S_FXL_PAN, S_FXL_BUSES, S_FXL_DELAY, S_FXL_DELAY2, S_FXL_REVERB, S_FXL_SLICER, S_FXL_MASTER,
       S_GEN_LEAD, S_ED_FILTER, S_ED_FILTER_BP, S_ED_VOICE2, S_ED_OPERATOR, S_ENV_TRACKS, S_ENV_TRACKS_DEST, S_ENV_STAGE,
       S_ENV_STAGE_SUS, S_MSEQ2, S_KEY_CHORD, S_KEY_CHORD_OFF, S_KEY_ARP, S_KEY_ARP2, S_SCL_MORE, S_SCL_WHITE, S_ARP_CHORD,
       S_GLO_SENDS, S_MENU_ROW, S_MENU_ROW_LEDS, S_LY_FX_KNOBS, S_LY_FX_STATE, S_LY_GLO_STATE, S_DLG_DICE_SOUND,
       S_DLG_DICE_SEQ, S_DLG_DICE_FX, S_DLG_DICE_KIT, S_DLG_DICE_LFO, S_DLG_MUT_BACK, S_DRUM_X4, S_DRUM_GHOST, S_DRUM_OCT_UP,
       S_DRUM_OCT_DN, S_COMP_TOUCH, S_COMP_EVEN, S_COMP_SEL, S_COMP_SEL_DRUM, S_COMP_JUMP, S_GEN_EXPLORE, S_GLO_LIVE,
       S_GLO_LIVE_TRICKS, S_GLO_LIVE_LATCH, S_GLO_MORE, S_MIXER_T4, S_MOCK_HOME, S_MOCK_SEQ, S_MOCK_DRUM, S_MOCK_MIXER,
       S_MOCK_DIALOG, S_MOCK_MENU, S_TOAST_ENV, S_TOAST_MUTATE, S_TOAST_MUTATE_HINT, S_TOAST_MOTION, S_TOAST_MOTION_LOCKS,
       S_TOAST_PUNCH, S_TOAST_AUTO_LIST, S_TOAST_AUTO_LIST_DRUM, S_TOAST_PATTERN, S_TOAST_FM6_OP, S_TOAST_ANALOG, S_MENU_MIDI,
       S_MENU_SCREEN2, S_UX_THEME, S_UX_DEMO, S_UX_DEMO_WARN, S_SPLASH_MID, S_GEN_BEAT, S_GEN_DNB, S_GEN_RETRO,
       S_EDIT_ANALOG_1, S_EDIT_ANALOG_2, S_EDIT_ANALOG_3, S_EDIT_ANALOG_MORE, S_EDIT_PHASE_1, S_EDIT_PHASE_2, S_EDIT_PHASE_3,
       S_EDIT_PHASE_MORE, S_EDIT_LOFI_1, S_EDIT_LOFI_2, S_EDIT_LOFI_3, S_EDIT_LOFI_MORE, S_EDIT_NOISE_1, S_EDIT_NOISE_2,
       S_EDIT_NOISE_3, S_EDIT_NOISE_MORE, S_EDIT_VOICE_1, S_EDIT_VOICE_2, S_EDIT_VOICE_3, S_EDIT_VOICE_MORE,
       S_EDIT_LOFI_BYTE_1, S_EDIT_LOFI_BYTE_MORE, S_EDIT_NOISE_LFSR_2, S_EDIT_VOICE_WORDS_1, S_EDIT_VOICE_MORE_END,
       S_EDIT_PHASE_MORE_K3, S_EDIT_ANALOG_TURN, S_EDIT_FM6_1, S_EDIT_FM6_2, S_EDIT_FM6_3, S_EDIT_FM6_MORE, S_EDIT_FM6_ALG4,
       S_EDIT_FM6_TURN, S_EDIT_DRUM_1, S_EDIT_DRUM_2, S_EDIT_DRUM_3, S_EDIT_DRUM_MORE, S_EDIT_DRUM_MUTED, S_EDIT_DRUM_MORE_END,
       S_LAYER_EDIT, S_LAYER_MACRO, S_LAYER_FX, S_LAYER_SCL, S_LAYER_FX_STATE, S_ROLL_LOCK_MORE, S_HOME_BPM, S_SEQ_STEPS,
       S_SEQ_STEPS_64, S_SEQ_STEPS_DRUM, S_SEQ_EXTRAS, S_SEQ_EXTRAS_DRUM, S_SEQ_PATTERN, S_SEQ_PATTERN_32, S_SEQ_MORE,
       S_SEQ_MORE_END, S_SEQ_MORE_DRUM, S_SEQ_LOCK, S_SEQ_EXTRAS_ECHO, S_GLO_MIX, S_GLO_MORE_END, S_GLO_MIDI, S_GLO_MORE_MIDI,
       S_SAVE_SOUND_EMPTY, S_SAVE_MORE, S_SAVE_SONG, S_SAVE_SONG_EMPTY, S_DLG_LOAD_PROJ, S_LAYER_SAVE, S_COUNT };
static const char *const S_NAME[S_COUNT] = {"home", "home_idle", "home_visual", "home_visual_idle", "home_play", "home_demo",
    "home_t5", "home_turn", "home_rec", "home_batt0", "message", "message_key", "message_nofile", "phrases",
    "song_empty", "song", "step", "pattern", "comp", "comp_trig", "comp_echo", "mutate", "morph", "dx_image",
    "dx_send", "page_flash", "fugue", "motion", "drum", "drum_hand", "drum_cym", "mixer", "mixer_pan", "env_1", "env_2",
    "lfo_1", "lfo_3", "fx_1_eco", "fx_more_eco", "fx_3_quiet", "fx_more_end", "fx_2_free", "scl_1", "shift", "arp_off",
    "voice", "global", "system", "edit_analog", FELUCCA_FM4 ? "edit_digital" : "edit_fm6", "op_env", "edit_voice",
    "edit_noise", "alg_1", "alg_2", "alg_3", "alg_4", "alg_5", "alg_6", "alg_7", "alg_8", "op_level", "fm6_alg_01",
    "fm6_alg_05", "fm6_alg_22", "fm6_alg_32", "drumx", "drumx_hit", "dx_sound", "dx_sound2", "dlg_clear_seq", "dlg_clear_trk",
    "dlg_clear_all", "dlg_clear_motion", "dlg_clear_pfx", "dlg_clear_song", "dlg_del_row", "dlg_init_sound",
    "dlg_init_sound_drum", "dlg_init_sys", "dlg_cpu_test", "dlg_live_commit", "dlg_load_pat", "dlg_ovr_proj", "dlg_ovr_user",
    "dlg_erase_user", "dlg_recall_sound", "gen", "gen_parts", "menu", "menu_speaker", "about", "about_rec", "about_credits",
    "about_end", "about_profile", "about_profile2", "uboot", "calibration", "splash", "batt_0", "batt_1", "batt_2", "batt_3",
    "batt_usb", "motion_rec", "motion_off", "motion_card", "song_home", "perform_peek", "perform_held", "perform_wait",
    "perform_harm", "perform_glitch", "perform_scratch", "perform_shuffle", "perform_freeze", "menu_hold", "menu_leds",
    "menu_end", "menu_system", "menu_slide", "fx_2_lite", "fx_2_reso", "layer_glo_peek", "layer_glo_active", "layer_glo_ext",
    "layer_scl_peek", "layer_scl_active", "layer_edit_peek", "layer_edit_active", "layer_edit_user", "layer_edit_drum",
    "layer_hint", "layer_lock", "layer_lock_fx", "layer_song", "layer_song_chain", "layer_seq_tools", "layer_seq_tools_drum",
    "layer_seq_tools_song", "lente_pattern", "lente_pattern_drum", "lente_stretch", "lente_bar", "lente_bar_drum",
    "lente_step_note", "lente_step_comp", "lente_step_comp2", "lente_step_tie", "lente_step_rest", "lente_step_drum",
    "lente_bar_long", "lente_step_long", "name_user", "name_typing", "name_123", "name_empty", "name_full", "name_playing",
    "project_named", "song_named", "words", "words_en", "words_ja", "words_edit", "words_off", "name_lyric", "name_lyric_long",
    "roll_empty", "roll_acid", "roll_chords", "roll_ties", "roll_len32_p2", "roll_high", "roll_low", "roll_wide",
    "roll_playing", "roll_locks", "roll_lock_held", "drum_locks", "drum_lock_held", "drum_lock_page", "motion_locks",
    "auto_list", "auto_list_add", "auto_list_drum", "layer_rec", "layer_rec_set", "layer_rec_song", "layer_macro", "lfo_more",
    "fx_1_turn", "fx_2_hold", "fx_2_cloud", "fx_more_chorus", "fx_3_hot", "lfo_2", "lfo_sh", "lfo_free", "countin",
    "countin_2bars", "menu_audio", "head_play_rec", "head_rec_other", "head_bpm_turn", "head_bpm_lock", "head_bpm_locked",
    "head_glo_turn", "head_downbeat", "head_beat_2", "save_project", "save_more_end",
    "save_sound", "fx_1", "fx_more", "fx_more_lane", "fx_2", "fx_2_ping", "fx_more_delay", "fx_more_reso", "fx_more_slicer",
    "fx_3", "gen_lead", "ed_filter", "ed_filter_bp", "ed_voice2", "ed_operator", "env_loop", "env_turn", "env_2_loop",
    "env_first", "lfo_more_end", "scl_2", "scl_2_off", "arp_1", "arp_2", "scl_more", "scl_white", "arp_chord", "glo_sends",
    "menu_row", "menu_row_leds", "ly_fx_knobs", "ly_fx_state", "ly_glo_state", "dlg_dice_sound", "dlg_dice_seq", "dlg_dice_fx",
    "dlg_dice_kit", "dlg_dice_lfo", "dlg_mut_back", "drum_hit_x4", "drum_hit_ghost", "drum_oct_x8", "drum_oct_flam",
    "comp_touch", "comp_echo_even", "comp_sel", "comp_sel_drum", "comp_jump", "gen_explore", "glo_live", "glo_live_tricks",
    "glo_live_latch", "glo_more", "mixer_tracks4", "mock_home", "mock_seq", "mock_drum", "mock_mixer", "mock_dialog",
    "mock_menu", "toast_env", "toast_mutate", "toast_mutate_hint", "toast_motion", "toast_motion_locks", "toast_punch",
    "toast_auto_list", "toast_auto_list_drum", "toast_pattern", "toast_fm6_op", "toast_analog", "menu_midi", "menu_screen2",
    "ux_theme", "ux_demo", "ux_demo_warn", "splash_mid", "gen_broken_beat", "gen_drill_n_bass", "gen_retroboc",
    "edit_analog_1", "edit_analog_2", "edit_analog_3", "edit_analog_more", "edit_phase_1", "edit_phase_2", "edit_phase_3",
    "edit_phase_more", "edit_lofi_1", "edit_lofi_2", "edit_lofi_3", "edit_lofi_more", "edit_noise_1", "edit_noise_2",
    "edit_noise_3", "edit_noise_more", "edit_voice_1", "edit_voice_2", "edit_voice_3", "edit_voice_more", "edit_lofi_byte_1",
    "edit_lofi_byte_more", "edit_noise_lfsr_2", "edit_voice_words_1", "edit_voice_more_end", "edit_phase_more_k3",
    "edit_analog_turn", "edit_fm6_1", "edit_fm6_2", "edit_fm6_3", "edit_fm6_more", "edit_fm6_alg4", "edit_fm6_turn",
    "edit_drum_1", "edit_drum_2", "edit_drum_3", "edit_drum_more", "edit_drum_muted", "edit_drum_more_end", "layer_edit",
    "layer_lfo", "layer_fx", "layer_scl", "layer_fx_state", "roll_lock_more", "home_bpm", "seq_steps", "seq_steps_64",
    "seq_steps_drum", "seq_extras", "seq_extras_drum", "seq_pattern", "seq_pattern_32", "seq_more", "seq_more_end",
    "seq_more_drum", "seq_lock", "seq_extras_echo", "glo_mix", "glo_more_end", "glo_midi", "glo_more_midi", "save_sound_empty",
    "save_more", "save_song", "save_song_empty", "dlg_load_proj", "layer_save"};

/* (0.9.11) GLO > LIVE: six sequences of their own lengths (16 32 7 16 . 1/8 20 4 . 1/4, as the mock-up's); (ACID FM 3c-1 task 6)
 * the ux pages: LIVE idle (stopped, nothing live); with tricks (T2 THIN 70 %, T3 x2 RATCH x3, T4 LOOP 8, T5 2/3; black key 3 held:
 * T3 picked, KNOB 1 just turned), played two seconds from PLAY so the bar's line is where it would be; latched (LATCH on, A4
 * PING latched on T5, C4 held on T3); GLO > MORE's RAILS (TRACKS T3) */
static void live_scene(int s)
{
    static const uint8_t NN[5] = {60, 67, 63, 72, 58};
    uint32_t k;
    trk[2].p[P_SLEN] = 7; trk[3].p[P_SDIV] = 1; trk[4].p[P_SLEN] = 20; trk[5].p[P_SLEN] = 4; trk[5].p[P_SDIV] = 0;
    for (k = 0; k < 20u; k++) {
        step_t *st = &trk[4].step[k];
        if (k % 3u) continue;
        st->time = ST_NOTE; st->n = 1; st->note[0] = NN[k % 5u]; st->vel = 100;
    }
    for (k = 0; k < 4u; k++) { trk[5].step[k].time = ST_NOTE; trk[5].step[k].n = 1; trk[5].step[k].note[0] = NN[k]; trk[5].step[k].vel = 90; }
    for (k = 0; k < 16u; k++) {                         /* T1 an acid line, T3 seven steps, T4 a beat of 1/8 */
        static const uint8_t A[16] = {36, 0, 48, 36, 0, 39, 0, 36, 43, 0, 36, 48, 0, 41, 39, 0};
        step_t *st = &trk[0].step[k], *d = &trk[3].step[k];
        if (A[k]) { st->time = ST_NOTE; st->n = 1; st->note[0] = A[k]; st->vel = 100; st->flags = k % 4u ? 0 : SF_ACCENT; }
        memset(d, 0, sizeof *d);
        d->time = ST_NOTE; d->vel = 100;
        d->hit = (uint8_t)((k % 4u == 0u ? 1u : 0u) | (k % 8u == 4u ? 2u : 0u) | (k % 2u ? 0u : 8u));
        if (k < 7u && k != 4u) { trk[2].step[k].time = ST_NOTE; trk[2].step[k].n = 1; trk[2].step[k].note[0] = NN[k % 5u] - 12u; trk[2].step[k].vel = 100; }
    }
    memset(&lv, 0, sizeof lv);
    song.sel = 2;
    if (s == S_GLO_MORE) {                              /* (fix round 1: the RAILS' own line, OCT- / OCT+ there, not the section's
                                                         * first-time hint) */
        uxs_seen |= (uint16_t)(1u << SEC_GLO);
        uxs_hint_sec = 0xFF;
    }
    nav_put(SEC_GLO, s == S_GLO_MORE ? "MORE" : "LIVE");
    if (s == S_GLO_LIVE)
        return;
    lv.dn[1] = -3; lv.rate[2] = 2; lv.dn[2] = 2; lv.cut[3] = 5; lv.here = 1u << 3; lv.rate[4] = -1;
    if (s == S_GLO_MORE) lv.tgt = 3;
    if (s == S_GLO_LIVE_TRICKS) lv.kb = 1u << key_at(1, 2);
    if (s == S_GLO_LIVE_LATCH) {
        memset(&lv, 0, sizeof lv);
        lv.latch = 1; lv.lt[9] = 1u << 4; lv.tk[5] = 1u << 2;
        lv.kb = 1u << key_at(1, 2) | 1u << key_at(0, 5);
    }
    lv.kprev = lv.kb;
    seq_start();
    live_publish();
    for (k = 0; k < 2800u; k++) { events_block(CTL); if (!(k % 64u)) live_publish(); }
    if (s == S_GLO_LIVE_TRICKS) ux_knob_touch(0);
}
/* the scenes of the UI design screens: the state the UI-redesign
 * prototype drew them from (its setup(): two pattern tracks, the drum pattern on track 4, a synthetic scope),
 * plus the hot knob each mock shows; tests/ui_mockcmp.py compares these renders with the mock PNGs */
static void mock_state(int s)
{
    uint32_t i;
    ui_power_on();
    if (large_on) ui_prefs |= PREF_LARGE;
    usb.config = 0;
    song.batt_raw = 600;
    for (i = 0; i < SCOPE_N; i++) {                  /* a saw with a little second harmonic */
        double ph = fmod(i / 64.0, 1.0);
        scope_buf[i] = (int16_t)((ph * 2.0 - 1.0) * 9000.0 + sin(i * 0.4908) * 2500.0);
    }
    scope_w = 0;
    song.playing = 1;
    song.g[G_BPM] = 124;
    clk_pos = 0; clk_step = 8; cin_total = cin_left = 0;   /* (as state()) */
    for (i = 0; i < NTRK; i++) trk[i].seq_idx = 5;
    load_pat16(&trk[0], PATTERNS[0].note, PATTERNS[0].flags);
    load_pat16(&trk[1], PATTERNS[4].note, PATTERNS[4].flags);
    for (i = 0; i < 16u; i++) {
        step_t *st = &trk[3].step[i];
        memset(st, 0, sizeof *st);
        st->time = ST_NOTE; st->vel = 100;
        if (i % 4u == 0u) st->hit |= 1u;
        if (i == 4u || i == 12u) st->hit |= 2u, st->acc |= 2u;
        if (i == 10u) st->hit |= 4u;
        if (i % 2u == 0u) st->hit |= 8u;
        if (i == 14u) st->hit |= 16u;
        if (i == 7u || i == 15u) st->hit |= 32u;
        if (i == 3u) st->hit |= 64u;
        if (i == 0u) st->hit |= 128u, st->acc |= 128u;
    }
    trk[3].p[P_SLEN] = 32;
    switch (s) {
    case S_MOCK_HOME: ui.home = 1; ui.hot_col = 1; ui.hot_t = 30; break;
    case S_MOCK_SEQ: song.sel = 0; song.rec = 1; go_page(GR_ROLL); ui.cursor = 6; ui.hot_col = 0; ui.hot_t = 30; break;
    case S_MOCK_DRUM: song.sel = 3; go_page(GR_ROLL); ui.cursor = 4; ui.lane = 1; trk[3].seq_idx = 9; ui.hot_col = 0; ui.hot_t = 30; break;
    case S_MOCK_MIXER:
        go_page(GR_MIX);                                /* (3c-1 task 7: GLO > MIX, the MIXER gone) */
        trk[0].p[P_LEVEL] = 100; trk[1].p[P_LEVEL] = 84; trk[2].p[P_LEVEL] = 64; trk[3].p[P_LEVEL] = 110;
        trk[0].p[P_PAN] = -20; trk[1].p[P_PAN] = 24; trk[2].p[P_PAN] = 0; trk[3].p[P_PAN] = -4;
        trk[0].p[P_REV] = 30; trk[1].p[P_REV] = 80; trk[2].p[P_REV] = 10; trk[3].p[P_REV] = 0;
        trk[2].p[P_MUTE] = 1;
        song.rec = 2u;
        trk[0].peak = 11000; trk[1].peak = 3300; trk[3].peak = 20000;     /* (the mock's stand-in meters) */
        ui.hot_col = 1; ui.hot_t = 30;
        break;
    case S_MOCK_DIALOG: ui.confirm = CF_OVR_PROJ; ui.confirm_trk = 0; go_page(GR_PATS); break;
    case S_MOCK_MENU: ui.menu = 1; ui.menu_sel = 0; break;
    default: break;
    }
}

/* the STEP page's piano roll (ui_graph.c graph_roll) on track 1, stopped unless said: an empty pattern; ACID in
 * A minor; POLY chords (Am7 F C G, tied); a line with ties, slides and accents in C major; LEN 32 on its second
 * page, playing; high (A6..) and low (C-1..) notes; a page wider than the view (edge marks); ACID playing with a
 * key held (A3, its row lit) */
static void step_put(step_t *st, uint32_t time, uint32_t flags, uint32_t n, const uint8_t *notes)
{
    uint32_t j;
    memset(st, 0, sizeof *st);
    st->time = (uint8_t)time; st->flags = (uint8_t)flags; st->n = (uint8_t)n; st->vel = 100;
    for (j = 0; j < n; j++) st->note[j] = notes[j];
}
static void roll_scene(int s)
{
    static const uint8_t AM7[] = {57, 60, 64, 67}, FM7[] = {53, 57, 60, 64}, CMA[] = {60, 64, 67}, GMA[] = {55, 59, 62, 67};
    static const uint8_t LINE[16] = {48, 0, 55, 60, 0, 59, 0, 0, 62, 64, 0, 67, 0, 65, 64, 0};
    static const uint8_t LINE_T[16] = {0, 1, 0, 0, 1, 0, 2, 2, 0, 0, 1, 0, 1, 0, 0, 2};   /* 0 note, 1 tie, 2 rest */
    track_t *t;
    uint32_t i;
    song.sel = 0; t = TSEL;
    song.playing = 0;
    track_defaults_steps(t);
    t->p[P_SLEN] = 16;
    go_page(GR_ROLL);
    ui.cursor = 0;
    switch (s) {
    case S_ROLL_EMPTY: break;
    case S_ROLL_ACID:
    case S_ROLL_PLAYING:
        load_pat16(t, PATTERNS[0].note, PATTERNS[0].flags);
        t->p[P_ROOT] = 9; t->p[P_SCALE] = 2; ui.cursor = 6;
        if (s == S_ROLL_PLAYING) {
            song.playing = 1; t->seq_idx = 9;
            kb_trk[3] = 0; kb_on[3] = 1; kb_snd[3][0] = 57;           /* A3 held */
        }
        break;
    case S_ROLL_CHORDS:
        t->p[P_VOICE] = V_POLY; t->p[P_ROOT] = 9; t->p[P_SCALE] = 2;
        for (i = 0; i < 16u; i++) {
            const uint8_t *c = i / 4u == 0u ? AM7 : i / 4u == 1u ? FM7 : i / 4u == 2u ? CMA : GMA;
            uint32_t n = i / 4u == 2u ? 3u : 4u;
            if (i % 4u == 0u) step_put(&t->step[i], ST_NOTE, i == 8u ? SF_ACCENT : 0u, n, c);
            else if (i % 4u == 1u) step_put(&t->step[i], ST_TIE, 0, 0, c);
            else if (i % 4u == 2u) step_put(&t->step[i], ST_NOTE, 0, n, c);
            else step_put(&t->step[i], ST_REST, 0, 0, c);
        }
        ui.cursor = 4;
        break;
    case S_ROLL_TIES:
        t->p[P_ROOT] = 0; t->p[P_SCALE] = 1;
        for (i = 0; i < 16u; i++) {
            uint8_t n = LINE[i];
            uint32_t f = (i == 3u || i == 9u ? SF_SLIDE : 0u) | (i == 0u || i == 8u || i == 13u ? SF_ACCENT : 0u);
            if (LINE_T[i] == 0u) step_put(&t->step[i], ST_NOTE, f, 1, &n);
            else step_put(&t->step[i], LINE_T[i] == 1u ? ST_TIE : ST_REST, 0, 0, &n);
        }
        t->step[2].flags |= SF_SLIDE;                  /* (slides into a note, from before a tie) */
        ui.cursor = 9;
        break;
    case S_ROLL_LEN32:
        load_pat16(t, PATTERNS[0].note, PATTERNS[0].flags);
        t->p[P_SLEN] = 32;
        for (i = 16; i < 32u; i++) {
            uint8_t n = (uint8_t)(60 + (i * 5u) % 12u);
            if (i % 3u != 2u) step_put(&t->step[i], ST_NOTE, i % 8u == 0u ? SF_ACCENT : 0u, 1, &n);
        }
        ui.cursor = 20; song.playing = 1; t->seq_idx = 22;
        break;
    case S_ROLL_HIGH:
    case S_ROLL_LOW:
        for (i = 0; i < 16u; i += 2u) {
            uint8_t n = (uint8_t)(s == S_ROLL_HIGH ? 112 + (i * 7u) % 15u : (i * 7u) % 15u);
            step_put(&t->step[i], ST_NOTE, 0, 1, &n);
        }
        ui.cursor = 2;
        break;
    case S_ROLL_WIDE:
        t->p[P_VOICE] = V_POLY;
        for (i = 0; i < 16u; i += 2u) {
            uint8_t n[2] = {(uint8_t)(36 + i), (uint8_t)(72 + i)};
            step_put(&t->step[i], ST_NOTE, 0, 2, n);
        }
        ui.cursor = 4;
        break;
    default: break;
    }
    ui.bank = (uint8_t)(ui.cursor / 16u);
}

/* (0.9.2) a section's page by its name in the chain (ui_lens.c nav_put), as a scene: no page flash over the footer */
static void nav_scene(uint32_t sec, const char *nm)
{
    if (!nav_put(sec, nm)) {
        fprintf(stderr, "nav_scene: no page \"%s\"\n", nm);
        exit(2);
    }
    pf_on = 0;
}
/* (0.9.4 .. 3b task 4) the creatures' scenes (ui_engvis.c's kit and colony, their being): gone with them in 3b task 5 */
enum { ENGI_PHASE_UI = 2, ENGI_LOFI_UI = 3, ENGI_NOISE_UI = 11 };   /* (engines.c ENGINES[]) */
/* (0.9.17) a short line on the selected track for the SEQ pages that drew their own panel before (TRACK, MUTATE, AUTOMATION,
 * AUTO LIST): every page shows the roll now, its marks over these notes */
static void seq_line(void)
{
    static const uint8_t NN[8] = {60, 63, 67, 70, 67, 65, 63, 58};
    uint32_t j;
    for (j = 0; j < 16u; j += 2u) step_put(&TSEL->step[j], ST_NOTE, 0, 1, &NN[j / 2u]);
}
/* (3b task 8) FX's scenes: T1's sends DISTORT 30 CHORUS 60 DELAY 40 REVERB 50, SOFT, AUTO PAN 40; DLY 1/8 FDBK 70 MIX 90, REVERB
 * ROOM SIZE 70, CHORUS DEPTH 64, SLICER GATE PAT 3 DEPTH 90, CLIP 30 PUNCH 40 DUCK 20 REL 60; no first-time hint, no knob hot */
static void fx_scene(void)
{
    uxs_seen = 0x3FFu; ux_hot_k = -1;
    TSEL->p[P_DIST] = 30; TSEL->p[P_CHOR] = 60; TSEL->p[P_DLY] = 40; TSEL->p[P_REV] = 50;
    TSEL->p[P_DTYPE] = 0; TSEL->p[P_APDEP] = 40; TSEL->p[P_APWAVE] = 0;
    song.g[G_DTIME] = 1; song.g[G_DFDBK] = 70; song.g[G_DMIX] = 90; song.g[G_RTYPE] = 0; song.g[G_RSIZE] = 70;
    song.g[G_CDEPTH] = 64; song.g[G_SLMODE] = SL_GATE; song.g[G_SLPAT] = 3; song.g[G_SLDEP] = 90;
    song.g[G_CLIP] = 30; song.g[G_PUNCH] = 40; song.g[G_DUCK] = 20; song.g[G_DREL] = 60;
    ui.msg_t = 0;
}
/* (3b task 8) a scene at LOAD l (FX's ECO and LITE ones): the LOAD settings as they were put back by the next scene's state(),
 * so the scenes after keep theirs (GLO > SYSTEM's TRACKS and LOAD) */
static struct { uint8_t on, ul, sl, tl, tu; int16_t lu; } fx_ld;
static void fx_load(uint8_t l)
{
    if (!fx_ld.on) {
        fx_ld.on = 1; fx_ld.ul = ui_load; fx_ld.sl = sys_load; fx_ld.tl = (uint8_t)trk_live; fx_ld.tu = (uint8_t)trks_ui;
        fx_ld.lu = load_ui;
    }
    ui_load = l;
    rp_apply();
}
static void fx_load_back(void)
{
    if (!fx_ld.on)
        return;
    fx_ld.on = 0;
    ui_load = fx_ld.ul; sys_load = fx_ld.sl; trk_live = fx_ld.tl; trks_ui = fx_ld.tu; load_ui = fx_ld.lu;
}
static void fx_scope(int32_t a)                         /* (MASTER's meter: the output a sine of peak a) */
{
    uint32_t i;
    for (i = 0; i < SCOPE_N; i++)
        scope_buf[i] = (int16_t)(a * sin(i * 0.37));
}
static void fx_more_at(uint32_t id, uint32_t kind)      /* FX > MORE, the row (kind, id) picked */
{
    uint32_t r;
    fx_scene();
    memset(uxm_at, 0, sizeof uxm_at);
    nav_scene(SEC_FX, "MORE");
    for (r = 0; r < UXM_FX.n && !(UXM_FX.rows[r].kind == kind && UXM_FX.rows[r].id == id); r++)
        ;
    uxm_at[uxm_key()].sel = (uint8_t)r;
    uxm_at[uxm_key()].top = 0;
    {   /* (the pick in sight) */
        uint32_t s = r, t = 0;
        uxm_fix(&UXM_FX, uxm_mask(&UXM_FX), &s, &t);
        uxm_at[uxm_key()].sel = (uint8_t)s;
        uxm_at[uxm_key()].top = (uint8_t)t;
    }
}
/* (ACID FM 3c-1 task 4) SEQ > MORE, the row (kind, id) picked and in sight: JIANT's SEQ pages' scenes (COMP, PLAYHEADS, MUTATE,
 * AUTOMATION, AUTO LIST, TRACK, STRETCH, NOTE) are its rows now; the step picked kept */
/* (ACID FM 3c-1 task 7) GLO > MORE, row (kind, id) picked, in sight (the first-time hint seen) */
static void glo_more_at(uint32_t kind, uint32_t id)
{
    uint32_t r, sl, t;
    memset(uxm_at, 0, sizeof uxm_at);
    uxs_seen |= (uint16_t)(1u << SEC_GLO);
    uxs_hint_sec = 0xFF;
    nav_scene(SEC_GLO, "MORE");
    for (r = 0; r < UXM_GLO.n && !(UXM_GLO.rows[r].kind == kind && UXM_GLO.rows[r].id == id); r++)
        ;
    sl = r; t = r && r < UXM_GLO.n && UXM_GLO.rows[r - 1u].kind == UXR_HEAD ? r - 1u : r;   /* (its heading on top) */
    uxg_cpu_on = 0;                                     /* (the CPU readout taken now: song.cpu_q8 as the scene sets it) */
    uxm_fix(&UXM_GLO, uxm_mask(&UXM_GLO), &sl, &t);
    uxm_at[uxm_key()].sel = (uint8_t)sl;
    uxm_at[uxm_key()].top = (uint8_t)t;
}
/* (ACID FM 3c-1 task 7) GLO > MIX: six tracks' levels (OFF .. +7.5 dB), pans, T3 muted, T2 selected; KNOB 1 just turned */
static void glo_mix_scene(void)
{
    static const int16_t LV[NTRK] = {112, 100, 88, 127, 64, 0}, PN[NTRK] = {-20, 24, 0, -64, 63, 10};
    uint32_t c;
    for (c = 0; c < NTRK; c++) { trk[c].p[P_LEVEL] = LV[c]; trk[c].p[P_PAN] = PN[c]; }
    trk[2].p[P_MUTE] = 1;
    song.sel = 1; song.g[G_SWING] = 12; song.g[G_TUNE] = -3;
    uxs_seen |= (uint16_t)(1u << SEC_GLO);
    uxs_hint_sec = 0xFF;
    nav_scene(SEC_GLO, "MIX");
    ux_knob_touch(0);
}
/* (ACID FM 3c-1 task 8) GLO > MIDI: mixed settings (T1 OUT off, T2 on channel 10, T3 IN off, T4 selected on channel 16: its hint,
 * T5 OUT off on channel 5), the clock USB with the diamond lit (a beat just received) */
static void glo_midi_scene(void)
{
    trk_midi_defaults();
    song.g[G_MROUT1] = 0; song.g[G_MRCH1 + 1] = 10; song.g[G_MRIN1 + 2] = 0; song.g[G_MRCH1 + 3] = 16; song.g[G_MROUT1 + 4] = 0;
    song.g[G_CLOCK] = 1;
    song.sel = 3;
    uxs_seen |= (uint16_t)(1u << SEC_GLO);
    uxs_hint_sec = 0xFF;
    ux_hot_k = -1;
    nav_scene(SEC_GLO, "MIDI");
    midi_clock.npulse += 24u;                           /* (a beat: the diamond lit) */
}
static void seq_more_at(uint32_t kind, uint32_t id)
{
    uint32_t r, c = ui.cursor;
    memset(uxm_at, 0, sizeof uxm_at);
    nav_scene(SEC_SEQ, "MORE");
    cursor_set((int32_t)c);
    for (r = 0; r < UXM_SEQ.n && !(UXM_SEQ.rows[r].kind == kind && UXM_SEQ.rows[r].id == id); r++)
        ;
    uxm_at[uxm_key()].sel = (uint8_t)r;
    uxm_at[uxm_key()].top = 0;
    {
        uint32_t sl = r, t = r;
        uxm_fix(&UXM_SEQ, uxm_mask(&UXM_SEQ), &sl, &t);
        uxm_at[uxm_key()].top = (uint8_t)t;
    }
}
static void setup(int s)
{
    memset(kb_on, 0, sizeof kb_on);               /* no key held (roll_playing holds one) */
    live.req = live.cur = -1; live.mode = live.srec = live.dirty = 0; lys.nqc = 0;   /* (the song layer: off) */
    trk_live = NTRK;                              /* (the six awake but on *_tracks4) */
    if (s >= S_TOAST_ENV && s <= S_TOAST_ANALOG) {   /* (0.9.5) a toast over a page with words to its foot: the pill keeps clear
                                                   * of them (or is not drawn: the header says it), never over one (hk_overlay) */
        static const uint16_t BASE[] = {S_GLOBAL, S_MUTATE, S_MUTATE, S_MOTION, S_MOTION_LOCKS, S_COUNT, S_AUTO_LIST,
                                       S_AUTO_LIST_DRUM, S_PATTERN, S_EDIT_FM6_2, S_EDIT_ANALOG_1};   /* (0.9.6: over the creatures
                                                   * too; 3b: ANALOG's EDIT the ux page, FM6's OPERATOR; task 6: toast_env
                                                   * over GLOBAL, ENV the ux pages', their message the status line) */
        int b = BASE[s - S_TOAST_ENV];
        if (b == S_COUNT) { setup(S_FXL_PAN); return; }   /* (3b task 8: PUNCH's hint, FX > MORE's PUNCH LANE status line) */
        setup(b);
        if (s == S_TOAST_MUTATE_HINT) {           /* (MUTATE's old hint text as a toast: the toast's look; since */
                                                  /* 3c-1 task 4 the hint is SEQ > MORE's status line) */
            ui_message("OCT+ KEEP  GLO+A4 FREEZE");
            str_cpy(ui.msg2, "SEQ+D5 REVERT  GLO+B4 ALL", sizeof ui.msg2);
        } else
            ui_message("SAVED A LONG MESSAGE");
        return;
    }
    if (s >= S_MOCK_HOME && s <= S_MOCK_MENU) {   /* (0.9.18: the scenes appended after the toasts are their own: until
                                                   * then PRESETS TYPE and MENU > MIDI drew a toast over a stray page) */
        mock_state(s);
        return;
    }
    state();
    switch (s) {
    case S_HOME: {                                /* (0.9.9) the device's power-on parts (T1 DRUM-X, T2 VOICE, T3 PHASE,
                                                   * T4 LOFI, ...: ui.c power_sound_of) with state()'s steps: README's HOME */
        uint32_t k;
        load_depth++;
        for (k = 0; k < NTRK; k++) { power_sound_of(&trk[k], k); trk[k].engine = trk[k].eng_req; }
        load_depth--;
        song.octave = 2; song.rec = 1; usb.config = 1;
        song.playing = 0; break;                  /* (ACID FM 3a task 6: stopped, the ridgelines flat) */
    }
    case S_HOME_DEMO: {                           /* (3b) the power-on demo (power_on_parts, new settings: TRACKS 6) PLAYING for real: the
                                                   * sequencer and the engines run ~2.3 s (mix_block), the real peaks feed the ridgelines
                                                   * at the UI's cadence (T1, T5, T6 active; T2..T4 flat), the DEMO hint on the status line */
        int32_t o[2 * CTL];
        uint32_t b, us = 0;
        load_depth++; power_on_parts(); load_depth--;
        memset(&favorites, 0, sizeof favorites);
        settings_fresh();
        rp_apply();
        song.master_q12 = 4096;
        ux_levels_reset();
        transport_req = 1;
        for (b = 0; b < 3200u; b++) {
            events_block(CTL);
            mix_block(o, CTL);
            us += CTL * 1000000u / 44100u;
            fm1_ms += us / 1000u; us %= 1000u;
            if (b % 5u == 4u) ux_levels_tick();   /* (ui_draw's tick: every 16 ms frame, ~22 blocks; finer here, the ring's own cadence) */
        }
        break;
    }
    case S_HOME_PLAY:                             /* (ACID FM 3a task 6) TRACKS playing: ~2 s of levels behind each line, */
    case S_HOME_T5:                               /* T1's acid line on its 5th step (an accent, a slide); T5: T3..T6 lit */
    case S_HOME_TURN:                             /* .. T5's LEVEL just turned (3b task 10: KNOB 2 of T4..T6): its column, line, readout */
    case S_HOME_BPM: {                            /* (3b task 10) .. KNOB 4, the tempo, just turned to 128: TEMPO 128 BPM */
        uint32_t i, c;
        static const uint8_t AN[16] = {36, 0, 48, 39, 43, 0, 36, 1, 46, 41, 0, 48, 36, 0, 43, 39};   /* (0 rest, 1 tie) */
        for (i = 0; i < NTRK; i++) trk[i].seq_idx = 4;
        for (i = 0; i < 16u; i++) {               /* an acid line: accents on 5 and 12, slides into 4 and 10 */
            step_t *st = &trk[0].step[i];
            memset(st, 0, sizeof *st);
            st->time = AN[i] == 0u ? ST_REST : AN[i] == 1u ? ST_TIE : ST_NOTE;
            st->n = AN[i] > 1u; st->note[0] = AN[i]; st->vel = 100;
            st->flags = (uint8_t)((i == 4u || i == 11u ? SF_ACCENT : 0u) | (i == 3u || i == 9u ? SF_SLIDE : 0u));
            trk[4].step[i] = *st;
        }
        trk[0].p[P_SLEN] = trk[4].p[P_SLEN] = 16;
        ux_levels_reset();
        for (i = 0; i < UXR_N; i++) {             /* each track its own rhythm (T6 silent: flat) */
            static const uint8_t EV[NTRK] = {14, 9, 3, 28, 20, 0}, AT[NTRK] = {0, 2, 1, 5, 0, 0};
            for (c = 0; c < NTRK; c++)
                trk[c].peak = EV[c] && i % EV[c] == AT[c] ? (int32_t)(7600 - c * 900 - (i * 37u % 1500u)) :
                              c == 4u && i % 20u < 4u ? 2600 : 0;
            fm1_ms += UXR_MS;
            ux_levels_tick();
        }
        if (s != S_HOME_PLAY) track_select(4);
        if (s == S_HOME_TURN) { trk[4].p[P_LEVEL] = 92; ux_knob_touch(1); }
        if (s == S_HOME_BPM) { song.g[G_BPM] = 128; ux_knob_touch(3); }
        break;
    }
    case S_HOME_REC:                              /* (fix wave I3) the header's marks at their longest: REC armed, OCT+1, BPM LOCK,
                                                   * the battery low (a third: UX_ACCENT), a song at A 12/16, 180 BPM, playing */
        song.rec = 1u << song.sel; song.octave = 1; ui_prefs |= PREF_BPM_LOCK; usb.config = 0; song.batt_raw = 540;
        song.g[G_BPM] = 180; song.playing = 1;
        chain.running = 1; chain.row = 0; chain.config.count = 1;
        chain.config.row[0].slot = 0; chain.config.row[0].bars = 16; chain.remaining = 5;
        break;
    case S_HOME_BATT0:                            /* (fix wave) the battery nearly flat: its one segment in UX_ACCENT (the blink's on half) */
        usb.config = 0; song.batt_raw = 500; fm1_ms = (fm1_ms / 1000u + 1u) * 1000u; break;
    case S_HOME_IDLE: song.playing = 0; song.batt_raw = 570; ui.hot_col = 1; ui.hot_t = 30; break;
    case S_HOME_VISUAL:                          /* (ACID FM 3a) HOME's second page (HOME again), M1..M4, M2 just turned; */
    case S_HOME_VISUAL_IDLE: {                   /* (task 7) playing mid-bar, a song chain at B bar 3/8, a named project,
                                                  * the tracks sounding; _IDLE: stopped, no name (ACID FM), silent */
        uint32_t i;
        macro_v[0] = 40; macro_v[1] = 90; macro_v[2] = 20; macro_v[3] = 64;
        nav_go(SEC_HOME, UXH_VISUAL, 0);
        ux_levels_reset();
        if (s == S_HOME_VISUAL) {
            ux_knob_touch(1);
            str_cpy(proj_name, "VELVET PYLON", sizeof proj_name);
            chain.running = 1; chain.row = 0; chain.config.count = 2;
            chain.config.row[0].slot = 1; chain.config.row[0].bars = 8; chain.remaining = 6;
            chain.bar_pos = chain_bar() * 5u / 8u;
            for (i = 0; i < 12u; i++) {              /* (the last ~200 ms: T1 T3 loud, T2 T5 on their beats, T4 soft, T6 silent:
                                                      * its ring, the innermost, dim) */
                trk[0].peak = 7000; trk[1].peak = i % 2u ? 0 : 4200; trk[2].peak = 5600;
                trk[3].peak = 1800; trk[4].peak = i % 3u ? 0 : 6000; trk[5].peak = 0;
                fm1_ms += UXR_MS;
                ux_levels_tick();
            }
        } else {
            song.playing = 0;
            ux_hot_k = -1;                           /* (no knob turned, the ring legend shown before: the hint) */
            uxv.intro = 1; uxv.intro_ms = fm1_ms - UXV_INTRO_MS;
        }
        break;
    }
    case S_MESSAGE: ui_say("LOADED ", "07 A VERY LONG PATTERN NAME"); break;
    case S_MESSAGE_KEY: ui_message("[SAVE] HOLD TO UNDO"); break;            /* a message with a keycap */
    case S_MESSAGE_NOFILE: ui_message(MSG_NOFILE "NO FILE"); break;   /* a message led by the no-file icon */
    /* (ACID FM 3c-2 task 2) PRESETS (presets, presets_nofav, presets_pluck) and USER (user, user_foot) gone: SAVE > SOUND and MORE
     * (save_sound, save_sound_empty, save_more) */
    case S_MENU_MIDI:                                   /* (0.9.12) MENU > MIDI; (ACID FM 3c-1 task 8) its pointer, MIDI IS ON GLO */
        ui.menu = 1; ui.menu_sel = MI_GLOMIDI; break;
    case S_MENU_SCREEN2:                                /* (0.9.18) MENU > SCREEN 2: SCREEN SLEEP 15 MIN just turned */
        scr_put(2u); ui.menu = 1; ui.menu_sel = MI_SCROFF; ui.hot_col = 2u; ui.hot_t = 30; break;
    case S_PHRASES: go_page(GR_PATS); break;
    case S_SONG_EMPTY: song.playing = 0; go_page(GR_SVSONG); break;
    case S_SONG:
        song.playing = 0; project_save(0); project_save(1);
        chain_config.count = 3;
        chain_config.row[0] = (chain_row_t){0, 2}; chain_config.row[1] = (chain_row_t){1, 4}; chain_config.row[2] = (chain_row_t){2, 1};
        ui.song_row = 1; go_page(GR_SVSONG); chain_prepare(); events_block(32);
        break;
    case S_STEP: song.rec = 1; go_page(GR_ROLL); ui.cursor = 6; break;
    case S_PATTERN: go_title("PATTERN"); ui.cursor = 3; break;
    case S_FUGUE:                                       /* (0.6.52) heads 2 x2 +2, 3 1/2 REV -3, playing */
        for (uint32_t j = 0; j < 8u; j++) { uint8_t nn = (uint8_t)(60 + (j * 5u) % 12u); step_put(&TSEL->step[j], ST_NOTE, 0, 1, &nn); }
        TSEL->p[P_SLEN] = 8;
        fg_set[song.sel][0] = (fg_set_t){7, 0, 2}; fg_set[song.sel][1] = (fg_set_t){3, 1, -3};
        fg_head[song.sel][0].idx = 5; fg_head[song.sel][1].idx = 2; song.playing = 1; fg_ui = 1;
        seq_more_at(UXR_TRACK, ID_SQ_HEAD);
        break;
    case S_PFLASH:                                      /* (0.6.51) a page changed by hand: its name big, held */
        go_title("LFO"); page_flash(); pf_ms = fm1_ms - 200u;
        break;
    case S_DXIMG:                                       /* (0.6.48) DRUM IMAGE 40 (3b task 5: MORE's STEREO, picked) */
        set_engine_of(TSEL, ENGI_DRUM); TSEL->engine = TSEL->eng_req; TSEL->p[P_DXIMG] = 40;
        memset(uxm_at, 0, sizeof uxm_at); nav_scene(SEC_EDIT, "MORE"); (void)ux_more_knob(0, 7);
        break;
    case S_DXSEND:                                      /* (0.6.48) SEND: the snare to the REVERB, 75 % (3b task 5: MORE's) */
        set_engine_of(TSEL, ENGI_DRUM); TSEL->engine = TSEL->eng_req; dx_ui_lane = 1;
        dx_snd[1] = 3u << 6 | 47u; dx_lay[1] = 0x26;
        memset(uxm_at, 0, sizeof uxm_at); nav_scene(SEC_EDIT, "MORE"); (void)ux_more_knob(0, 15);
        break;
    case S_MORPH:                                       /* (0.6.45) ANALOG SAW, MORPH 50 */
        set_engine_of(TSEL, ENGI_ANALOG); TSEL->p[P_E0] = 0; TSEL->p[P_MORPH] = 50;   /* (3b task 4: EDIT's MORE, MORPH picked) */
        nav_scene(SEC_EDIT, "MORE"); uxm_at[SEC_EDIT].sel = 9;
        break;
    case S_MUTATE:                                      /* (0.6.43) three tracks mutating, bar 5 */
        trk[0].p[P_MSND] = 2; trk[0].p[P_MPAT] = 4; trk[1].p[P_MSND] = 3; trk[1].p[P_MMODE] = 1; trk[3].p[P_MPAT] = 1;
        seq_line(); bar_n = 5; seq_more_at(UXR_TRACK, P_MSND);
        break;
    case S_COMP:                                        /* (0.6.39) an ARP NOTES step: UP-DN x6 over two octaves */
    case S_COMP_TRIG:                                   /* .. a TRIG 3:4 */
    case S_COMP_ECHO:                                   /* (0.6.42) .. an ECHO x4, a JUMP, a PULSE */
        TSEL->step[0] = (step_t){{60, 63, 67}, 3, ST_NOTE, 0, 100};
        TSEL->step[0].comp = s == S_COMP ? CP_ARPN : s == S_COMP_ECHO ? CP_ECHO : CP_TRIG;
        TSEL->step[0].carg = s == S_COMP ? CP_ARG(2u, 3u, 1u) : s == S_COMP_ECHO ? 2u : 7u;
        if (s == S_COMP_ECHO) { TSEL->step[10].comp = CP_JUMP; TSEL->step[10].carg = 2; TSEL->step[12].comp = CP_PULSE; }
        TSEL->step[4].comp = CP_TRIG; step_set_chance(&TSEL->step[6], 50); step_set_ratchet(&TSEL->step[8], 3);
        ui.cursor = 0; seq_more_at(UXR_TRACK, ID_SQ_TYPE);
        break;
    case S_MOTION: seq_line(); motion_set_event(TSEL, 4, P_REV, 40); seq_more_at(UXR_TRACK, ID_SQ_PLAY); break;
    /* 1.1.5, the REC layer (REC held): over HOME, the settings as they come (CLICK OFF, COUNT-IN OFF, LEVEL MID);
     * over STEP armed, CLICK REC, COUNT-IN 2 BARS, LEVEL HIGH (KNOB 2 just turned); a song playing on an empty
     * track (CLEAR dimmed: STOP TO EDIT) */
    case S_REC_LAYER: go_home(); ui.layer = LAYER_REC; break;
    case S_MSEQ: case S_MSEQ2: {                     /* (3b task 7) LFO's MORE: the STEPS, a rising, falling line over LENGTH 12;
                                                         * STEP 5 picked (its rows in sight), then the list's end */
        static const uint8_t L[16] = {10, 40, 70, 100, 127, 90, 60, 30, 0, 50, 110, 20, 80, 80, 80, 80};
        uint32_t k;
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        for (k = 0; k < 16u; k++) TSEL->p[P_MS0 + k] = L[k];
        TSEL->p[P_MSLEN] = 12; TSEL->p[P_MSSLW] = 30;
        memset(uxm_at, 0, sizeof uxm_at);
        nav_scene(SEC_LFO, "MORE");
        (void)ux_more_knob(0, s == S_MSEQ ? 6 : 30);
        break;
    }
    /* (3b task 8) FX on the ux pages (JIANT's DIST, DLY 2, REVERB 2, CHORUS, MASTER pages gone): their scenes the new pages' */
    case S_DIST_PAGE:                                   /* SENDS, DISTORT just turned (FOLD): the status line */
        fx_scene(); TSEL->p[P_DIST] = 70; TSEL->p[P_DTYPE] = 2; nav_scene(SEC_FX, "SENDS"); edit_param(0, 1); ux_knob_touch(0);
        break;
    case S_DLY2:                                        /* SPACE: SHIMMER with MOD (its tail holds), 1/16T */
        fx_scene(); song.g[G_RTYPE] = 2; song.g[G_RMOD] = 60; song.g[G_DTIME] = 5; nav_scene(SEC_FX, "SPACE"); break;
    case S_REVERB2:                                     /* SPACE: CLOUD, a long free echo */
        fx_scene(); song.g[G_RTYPE] = 4; song.g[G_RSIZE] = 100; song.g[G_DTIME] = DT_FREE + 44; nav_scene(SEC_FX, "SPACE"); break;
    case S_CHORUS: fx_more_at(G_CRATE, UXR_GLOBAL); break;   /* MORE: the CHORUS rows */
    case S_MASTER:                                      /* MASTER: CLIP, PUNCH, DUCK all up, the output past the clip point */
        fx_scene(); song.g[G_CLIP] = 100; song.g[G_PUNCH] = 100; song.g[G_DUCK] = 100; song.g[G_DREL] = 20;
        fx_scope(30000); nav_scene(SEC_FX, "MASTER"); break;
    case S_LFO2: case S_LFO2_FREE:                      /* (3b task 7) LFO: RANDOM in time (1/8), TRIG at PHASE 32; a square
                                                         * at 1/16, FREE (the 1 BEAT window) */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_LRATE] = s == S_LFO2 ? -8 : -10; TSEL->p[P_LWAVE] = s == S_LFO2 ? 4 : 3;
        TSEL->p[P_LPHASE] = s == S_LFO2 ? 32 : -1; TSEL->p[P_LFADE] = 0;
        TSEL->lfo_ph = 0x60000000u; TSEL->lfo_rnd = 0x9E3779B9u; TSEL->lfo_fade = 32767;
        TSEL->lfo_val = lfo_wave(TSEL, TSEL->lfo_ph);
        nav_scene(SEC_LFO, "LFO");
        break;
    case S_LFO_DEST:                                    /* (3b task 7) DEST: PITCH a little vibrato, FILTER down, AMP a tremolo */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_LRATE] = 70; TSEL->p[P_LD_PIT] = 20; TSEL->p[P_LD_FLT] = -40; TSEL->p[P_LD_AMP] = 60;
        nav_scene(SEC_LFO, "DEST");
        break;
    case S_MACRO_LAYER:                              /* M1: T1 CUT and T4's MRPH; M2: CLIP; M3 at 0, M4 none */
        go_home(); ui.layer = LAYER_MACRO;
        macro_v[0] = 90; macro_v[1] = 40; macro_v[3] = 127;
        trk[0].p[P_M1SRC] = MS_M1; trk[0].p[P_M1DST] = MD_CUT; trk[0].p[P_M1AMT] = 40;
        trk[3].p[P_M2SRC] = MS_M1; trk[3].p[P_M2DST] = MD_E1; trk[3].p[P_M2AMT] = -20;
        trk[1].p[P_M1SRC] = MS_M1 + 1; trk[1].p[P_M1DST] = MD_CLIP; trk[1].p[P_M1AMT] = 63;
        trk[2].p[P_M1SRC] = MS_M1 + 2; trk[2].p[P_M1DST] = MD_REV; trk[2].p[P_M1AMT] = 30;
        break;
    case S_REC_LAYER_SET: song.rec = 1; go_page(GR_ROLL); ui.cursor = 6; rp_put(RP_CLICK, 1u); rp_put(RP_COUNTIN, 2u);
        rp_put(RP_LEVEL, 2u); ui.layer = LAYER_REC; ui.hot_col = 1; ui.hot_t = 30; break;
    case S_REC_LAYER_SONG: drum(1); track_defaults_steps(TSEL); go_page(GR_SVSONG); chain.running = 1; ui.layer = LAYER_REC;
        break;
    /* 1.1 (Discussion #131): the count-in over the page: 1 BAR on its second beat (3 to go); 2 BARS, the first bar's
     * first beat (STEP under it); the MENU's AUDIO tab with CLICK REC selected, COUNT-IN 1 BAR */
    case S_COUNTIN: song.playing = 0; song.rec = 1; cin_total = 4; cin_left = 3; go_home(); break;
    case S_COUNTIN2: song.playing = 0; song.rec = 1; cin_total = 8; cin_left = 8; go_page(GR_ROLL); break;
    case S_MENU_AUDIO: ui.menu = 1; ui.menu_sel = MI_CLICK; rp_put(RP_CLICK, 1u); rp_put(RP_COUNTIN, 1u); break;
    /* (3c-1 task 7: on SAVE's TOOLS, a JIANT page and its header; GLO > GLOBAL gone; 3c-2 task 4: TOOLS gone, on SAVE > MORE) 1.2: the header's three slots (ui_draw.c draw_head): playing with the selected track armed; another track armed;
     * the BPM being turned (the accent's); BPM LOCK ON (the lock right of the BPM), and its message after a turn;
     * GLO with the tempo turned and BPM LOCK ON (the label right of the lock); the metronome on the bar's 1st beat
     * (the accent's) and its 2nd (mirrored) */
    case S_HEAD_PLAY_REC: song.rec = 1u << song.sel; go_title("SAVE MORE"); break;
    case S_HEAD_REC_OTHER: song.rec = 1u << ((song.sel + 2u) % NTRK); go_title("SAVE MORE"); break;
    case S_HEAD_BPM_TURN: song.g[G_BPM] = 128; ui.roll_bpm = 128; ui.bpm_t = 40; go_title("SAVE MORE"); break;
    case S_HEAD_BPM_LOCK: ui_prefs |= PREF_BPM_LOCK; go_title("SAVE MORE"); break;
    case S_HEAD_BPM_LOCKED: ui_prefs |= PREF_BPM_LOCK; go_title("SAVE MORE"); ui_message("BPM LOCKED"); break;
    case S_HEAD_GLO_TURN: ui_prefs |= PREF_BPM_LOCK; go_home(); ui.layer = LAYER_GLO; ui.bpm_t = 40; break;
    case S_HEAD_DOWNBEAT: clk_step = 1; song.rec = 1u << song.sel; go_title("SAVE MORE"); break;
    case S_HEAD_BEAT_2: clk_step = 5; song.rec = 1u << song.sel; go_title("SAVE MORE"); break;
    case S_DRUM: drum(0); go_page(GR_ROLL); ui.cursor = 4; ui.lane = 1; trk[3].seq_idx = 9; break;
    case S_DRUM_HAND: drum(1); go_page(GR_ROLL); ui.lane = 5; ui.cursor = 7; break;
    case S_DRUM_CYM: drum(2); go_page(GR_ROLL); ui.lane = 7; break;
    case S_MIXER:                                       /* (3c-1 task 7: GLO > MIX, the MIXER gone; its DJ FILTER LIVE's) */
        go_page(GR_MIX);
        trk[0].p[P_LEVEL] = 100; trk[1].p[P_LEVEL] = 84; trk[2].p[P_LEVEL] = 64; trk[3].p[P_LEVEL] = 110;
        trk[0].p[P_PAN] = -20; trk[1].p[P_PAN] = 24; trk[3].p[P_PAN] = -4; trk[1].p[P_REV] = 80;
        trk[2].p[P_MUTE] = 1; trk[3].p[P_MUTE] = 1; song.rec = 2u | 8u; trk[0].peak = 9000;
        song.g[G_DJF] = -24;                     /* (0.9.1) KNOB 4: the DJ FILTER, a low-pass */
        break;
    case S_MIXER_T4:                             /* (0.6.22) TRACKS 4: T5, T6 asleep, grey, OFF */
        go_page(GR_MIX); trk_live = 4; break;
    case S_ED_FILTER: case S_ED_FILTER_BP:             /* (0.8.5) EDIT's lens: TONE, FILTER (3b task 5: FM6's MORE, its */
        eng(ENGI_FM6); TSEL->engine = TSEL->eng_req; TSEL->p[P_FRES] = 90;   /* filter rows; a synth's is EDIT's page 3) */
        if (s == S_ED_FILTER_BP) TSEL->p[P_FTYPE] = 2, TSEL->p[P_DIST] = 60;
        memset(uxm_at, 0, sizeof uxm_at); nav_scene(SEC_EDIT, "MORE"); (void)ux_more_knob(0, s == S_ED_FILTER_BP ? 0 : 1);
        ux_knob_touch(1);
        break;
    case S_ED_VOICE2:                                   /* (3b task 5: FM6's MORE, SPREAD picked; VOICE UNISON) */
        eng(ENGI_FM6); TSEL->engine = TSEL->eng_req; TSEL->p[P_VOICE] = 3;
        memset(uxm_at, 0, sizeof uxm_at); nav_scene(SEC_EDIT, "MORE"); (void)ux_more_knob(0, 20);
        break;
    /* (3b task 6) ENV, SCL and ARP on the ux pages: AMP with LOOP on; RELEASE just turned (its readout); ENV TO with LOOP on;
     * AMP as first opened (its hint); SCL's MORE, SCL's KEY in WHITE (C minor), the ARP's CHORD */
    case S_ENV_TRACKS: case S_ENV_TRACKS_DEST: case S_ENV_STAGE: case S_ENV_STAGE_SUS:
        TSEL->p[P_ATK] = 30; TSEL->p[P_DEC] = 60; TSEL->p[P_SUS] = 70; TSEL->p[P_REL] = 70;
        TSEL->p[P_ED_FLT] = 48; TSEL->p[P_ED_PIT] = -12; TSEL->p[P_ED_SHP] = 24;
        if (s != S_ENV_TRACKS_DEST && s != S_ENV_STAGE_SUS) TSEL->p[P_ELOOP] = 1;
        uxs_seen = s == S_ENV_STAGE_SUS ? 0u : 0x3FFu;
        ux_hot_k = -1;
        nav_scene(SEC_ENV, s == S_ENV_STAGE ? "ENV TO" : "AMP");
        if (s == S_ENV_TRACKS_DEST) { edit_param(3, 4); ux_knob_touch(3); }   /* (RELEASE turned: its column lit, the readout) */
        break;
    case S_DLG_MUT_BACK:                                /* (0.9.3) MUTATE turned OFF on T3 after mutating: the NOTICE */
        song.sel = 2; trk[2].p[P_MSND] = 2; trk[2].p[P_MPAT] = 4; mut_keep();
        trk[2].p[P_MSND] = trk[2].p[P_MPAT] = 0;
        seq_more_at(UXR_TRACK, P_MSND);
        ui.confirm = CF_MUT_BACK; ui.confirm_trk = 2;
        break;
    case S_DLG_DICE_SOUND: case S_DLG_DICE_SEQ: case S_DLG_DICE_FX: case S_DLG_DICE_KIT: case S_DLG_DICE_LFO: {
        /* (0.8.5) the DICE dialog: BEFORE / NOW; (3c-2 task 1b) full screen (ux_dialog.c), a DRUM track's KIT, the LFO's */
        uint32_t k = s == S_DLG_DICE_SOUND || s == S_DLG_DICE_KIT ? CF_DICE_SOUND : s == S_DLG_DICE_SEQ ? CF_DICE_SEQ :
                     s == S_DLG_DICE_LFO ? CF_DICE_LFO : CF_DICE_FX;
        if (s == S_DLG_DICE_KIT) { song.sel = 0; set_engine_of(TSEL, ENGI_DRUM); }
        go_title("EDIT 1"); song.playing = 0;
        dk_keep(k); dk_vis_take();
        if (k == CF_DICE_SOUND) { TSEL->p[trk_engp(TSEL)->knob[0] % P_COUNT] = 100; TSEL->p[trk_engp(TSEL)->knob[2] % P_COUNT] = 5; }
        else if (k == CF_DICE_SEQ) for (uint32_t i = 0; i < 16u; i += 3u) { TSEL->step[i].time = ST_NOTE; TSEL->step[i].n = 1; TSEL->step[i].note[0] = (uint8_t)(48 + i); }
        else if (k == CF_DICE_LFO) { TSEL->p[P_LRATE] = 90; TSEL->p[P_LD_FLT] = 40; }
        else { TSEL->p[P_DIST] = 90; TSEL->p[P_CHOR] = 60; }
        ui.confirm = k; ui.confirm_trk = song.sel;
        break;
    }
    case S_LY_FX_KNOBS: case S_LY_FX_STATE:            /* (0.8.5) a layer's lens: FX's KNOBS, STATE */
        go_home(); ui.layer = LAYER_FX; ui.uxlv = 1u << LAYER_FX;   /* (3b task 9: the ACID FX has MAP and STATE, its own) */
        perf_held = perf_act = PF_BIT(PF_R16) | PF_BIT(PF_GLT) | PF_BIT(PF_SCRU) | PF_BIT(PF_M1 + 1);
        perf_m[0] = 6; perf_m[1] = 7; perf_m[2] = 40; perf_m[3] = 30; pf.sw[0] = 1u << 23;   /* (0.9.14: the macros) */
        break;
    case S_LY_GLO_STATE: go_home(); ui.layer = LAYER_GLO; ui.uxlv = 1u << LAYER_GLO; break;   /* (3c-1 task 9: its own bit) */
    case S_MENU_ROW: case S_MENU_ROW_LEDS:             /* (0.9.18) the knob just turned: COLOR's card lit (SCREEN), LED GLOW's
                                                         * (LEDS, KNOB 1) */
        ui.menu = 1; ui.menu_sel = s == S_MENU_ROW ? MI_COLOR : MI_LEDS; ui.hot_col = 0u; ui.hot_t = 30;
        break;
    case S_GLO_SENDS:                                  /* (0.8.5) GLO's lens: MIX > SENDS; (3c-1 task 7) FX > SENDS, the sends' one
                                                         * place */
        trk[0].p[P_DIST] = 30; trk[1].p[P_CHOR] = 90; trk[2].p[P_DLY] = 60; trk[3].p[P_REV] = 127; trk[4].p[P_MUTE] = 1;
        song.sel = 3; nav_scene(SEC_FX, "SENDS"); ux_knob_touch(3);
        break;
    case S_KEY_CHORD: case S_KEY_CHORD_OFF:             /* (3b task 6) SCL's CHORD: A minor, 7TH LEAD strummed, a line rotated 3 */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_ROOT] = 9; TSEL->p[P_SCALE] = 2; TSEL->p[P_VOICE] = V_POLY;
        if (s == S_KEY_CHORD) { TSEL->p[P_CHRD] = 2; TSEL->p[P_CVOI] = 1; TSEL->p[P_CSTR] = 70; TSEL->p[P_SOFS] = 3; seq_line(); }
        nav_scene(SEC_SCL, "CHORD");
        break;
    case S_KEY_ARP: case S_KEY_ARP2: case S_ARP_CHORD:  /* (3b task 6) ARP: UPDN over 2 octaves, SWING 40, a few left to CHANCE;
                                                         * ARP 2; CHORD */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_AMODE] = s == S_ARP_CHORD ? 14 : 3; TSEL->p[P_AOCT] = 2; TSEL->p[P_ASWING] = 40; TSEL->p[P_APROB] = 100;
        nav_scene(SEC_ARP, s == S_KEY_ARP2 ? "ARP 2" : "ARP");
        break;
    case S_SCL_MORE:                                    /* (3b task 6) SCL's MORE: SEQ PITCH picked, +7 */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_POFS] = 7;
        memset(uxm_at, 0, sizeof uxm_at);
        nav_scene(SEC_SCL, "MORE");
        break;
    case S_SCL_WHITE:                                   /* .. KEY: C minor, WHITE (the white keys play it), ALL TRACKS +2 */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_ROOT] = 0; TSEL->p[P_SCALE] = 2; TSEL->p[P_QUANT] = 2; song.g[G_STRN] = 2;
        nav_scene(SEC_SCL, "KEY");
        break;
    case S_ED_OPERATOR: eng(ENGI_FM6); TSEL->engine = TSEL->eng_req; nav_scene(SEC_EDIT, "OPERATOR"); break;   /* (3b task 5) */
    case S_GEN_EXPLORE:                                 /* (0.8.6) GEN 1/3 on EXPLORE, where it opens, after a piece */
        ui.confirm = CF_DICE_ALL; gp.page = 0; gp.hot = 0; gp.genre = GEN_EXPLORE; gp.seed = 421;
        str_cpy(gp.last, "EXPLORE 0421", sizeof gp.last);
        break;
    case S_GEN_BEAT: case S_GEN_DNB: case S_GEN_RETRO:  /* (3b) the long genre names: the card's short one, the full one across the panel */
        ui.confirm = CF_DICE_ALL; gp.page = 0; gp.hot = 0; gp.seed = 421;
        gp.genre = (uint8_t)(1u + (s == S_GEN_BEAT ? 42u : s == S_GEN_DNB ? 31u : 26u));
        str_cpy(gp.last, "HOUSE 0421", sizeof gp.last);
        break;
    case S_GEN_LEAD: ui.confirm = CF_DICE_ALL; gp.page = 2; gp.hot = 1; gp.form = 2; break;   /* (0.8.5) GEN 3/3, SONG */
    case S_MIXER_PAN:                            /* PAN just turned, the ends of each knob, OFF, armed while stopped */
        go_page(GR_MIX); song.playing = 0; song.sel = 1;
        trk[0].p[P_LEVEL] = 127; trk[1].p[P_LEVEL] = 0; trk[2].p[P_LEVEL] = 1; trk[3].p[P_LEVEL] = 104;
        trk[0].p[P_PAN] = -64; trk[1].p[P_PAN] = 63; trk[2].p[P_PAN] = 0; trk[3].p[P_PAN] = 1;
        trk[0].p[P_REV] = 127; trk[2].p[P_REV] = 1; trk[3].p[P_MUTE] = 1;
        song.rec = 1u | 8u; ui.hot_col = 3; ui.hot_t = 30;
        song.g[G_DJF] = 63;                      /* (0.9.1) the DJ FILTER just turned: the high-pass's end */
        break;
    case S_ENV: case S_ENVDEST:                         /* (3b task 6) ENV's AMP, ENV TO (the envelope moving the filter up, the
                                                         * pitch a little down, the shape) */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_ATK] = 20; TSEL->p[P_ED_FLT] = 48; TSEL->p[P_ED_PIT] = -12; TSEL->p[P_ED_SHP] = 24;
        nav_scene(SEC_ENV, s == S_ENV ? "AMP" : "ENV TO");
        break;
    case S_LFO:                                         /* (3b task 7) LFO: a sine at 1.18 Hz (1 BAR), FADE 0.5 s, TRIG */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_LRATE] = 60; TSEL->p[P_LWAVE] = 0; TSEL->p[P_LFADE] = 70; TSEL->p[P_LPHASE] = 0;
        TSEL->lfo_ph = 0x30000000u; TSEL->lfo_fade = 32767; TSEL->lfo_val = lfo_wave(TSEL, TSEL->lfo_ph);
        nav_scene(SEC_LFO, "LFO");
        break;
    case S_MOD:                                         /* (3b task 7) MOD MATRIX: the LFO to the FILTER, the velocity down on
                                                         * E5, ANALOG's CUTOFF (slot 2 picked: its names wider than UF_VAL, in
                                                         * UF_HDR: 3b task 7 fix), slot 3 the mod wheel to the delay's feedback;
                                                         * slot 4 off */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_M1SRC] = 1; TSEL->p[P_M1DST] = 2; TSEL->p[P_M1AMT] = 40; TSEL->p[P_M2SRC] = 3;
        TSEL->p[P_M2DST] = 16; TSEL->p[P_M2AMT] = -64; TSEL->p[P_M3SRC] = 6; TSEL->p[P_M3DST] = 23; TSEL->p[P_M3AMT] = 20;
        mod_ui_slot = 1;
        nav_scene(SEC_LFO, "MOD MATRIX");
        break;
    case S_FX:                                          /* (3b task 8) SENDS under LOAD ECO: no chorus (its bar dim, ECO: OFF) */
        fx_scene(); fx_load(LOAD_ECO); nav_scene(SEC_FX, "SENDS"); break;
    case S_SLICER:                                      /* .. MORE under LOAD ECO: no CHORUS, no SLICER rows */
        fx_load(LOAD_ECO); fx_more_at(G_RWIDE, UXR_GLOBAL); break;
    case S_SLBUS:                                       /* .. MASTER silent, every setting OFF */
        fx_scene(); song.g[G_CLIP] = song.g[G_PUNCH] = song.g[G_DUCK] = 0; fx_scope(0); nav_scene(SEC_FX, "MASTER"); break;
    case S_SLPITCH: {                                   /* .. MORE at its end: the SLICER's pitches */
        static const int8_t PS[16] = {0, 7, 12, -5, 0, 3, 24, -12, 5, 0, -24, 7, 12, 0, 0, 0};
        for (uint32_t k = 0; k < 16u; k++) song.g[G_SLP0 + k] = PS[k];
        song.g[G_SLMODE] = 2; song.g[G_SLPLEN] = 12;
        fx_more_at(G_SLP15, UXR_GLOBAL);
        break;
    }
    case S_DLY:                                         /* .. SPACE: a free time, the track sending no delay */
        fx_scene(); TSEL->p[P_DLY] = 0; song.g[G_DTIME] = DT_FREE + 30; nav_scene(SEC_FX, "SPACE"); break;
    case S_SCL:                                         /* (3b task 6) SCL's KEY: A minor, SNAP (each black key the scale's note) */
        uxs_seen = 0x3FFu; ux_hot_k = -1;
        TSEL->p[P_ROOT] = 9; TSEL->p[P_SCALE] = 2; TSEL->p[P_QUANT] = 1;
        nav_scene(SEC_SCL, "KEY");
        break;
    case S_SHIFT: seq_line(); TSEL->p[P_SOFS] = -3; TSEL->p[P_POFS] = 7; TSEL->p[P_HVEL] = 80; seq_more_at(UXR_TRACK, P_POFS); break;   /* (0.6.92: TRACK;
                                                         * 3c-1 task 4: SEQ > MORE's) */
    case S_ARP: uxs_seen = 0x3FFu; ux_hot_k = -1; nav_scene(SEC_ARP, "ARP"); break;   /* (3b task 6: OFF, the chord as played) */
    case S_VOICE:                                       /* (3b task 4: FM6's: a synth's VOICE is EDIT's ux page 3; task 5: FM6's */
        eng(ENGI_FM6); TSEL->engine = TSEL->eng_req;    /* MORE, its VOICE rows) */
        memset(uxm_at, 0, sizeof uxm_at); nav_scene(SEC_EDIT, "MORE"); (void)ux_more_knob(0, 15);
        break;
    case S_GLOBAL: glo_more_at(UXR_GLOBAL, G_BPM); break;   /* (3c-1 task 7: GLO > MORE's TEMPO, GLOBAL gone) */
    case S_SYSTEM: song.cpu_q8 = 87; glo_more_at(UXR_GLOBAL, G_TRKS); break;   /* (.. its SYSTEM, the page gone) */
    case S_GLO_MIX: glo_mix_scene(); break;
    case S_GLO_MORE_END: song.cpu_q8 = 87; glo_more_at(UXR_ACTION, UXM_A_UNDO); break;   /* (scrolled to its end) */
    case S_GLO_MIDI: glo_midi_scene(); break;
    case S_GLO_MORE_MIDI: mout_put(MOUT_SEQ, 1); glo_more_at(UXR_GLOBAL, ID_MO_CLOCK); break;   /* (3c-1 task 8: its MIDI rows,
                                                         * CLOCK OUT picked: its hint) */
    case S_EDIT_ANALOG: go_title("EDIT 1"); break;
    case S_EDIT_DIGITAL: eng(E_FM); go_title("EDIT 1"); break;
    case S_OP_ENV: eng(1); go_title("OP1 ENV"); break;
    case S_EDIT_SAMPLE: eng(5); go_title("EDIT 1"); break;   /* (JIANT: SAMPLE retired; VOICE in its place) */
    case S_EDIT_GRAIN: eng(11); go_title("EDIT 2"); break;   /* (GRAIN retired: NOISE's second page) */
    case S_ALG1: case S_ALG2: case S_ALG3: case S_ALG4: case S_ALG5: case S_ALG6: case S_ALG7: case S_ALG8:
        eng(1); TSEL->p[P_E0] = (int16_t)(s - S_ALG1);      /* the 8 DIGITAL charts; FB on the odd ones, IDX high .. 0 */
        TSEL->p[P_E6] = (s - S_ALG1) & 1 ? 40 : 0; TSEL->p[P_E4] = (int16_t)((S_ALG8 - s) * 18);
        go_title("EDIT 2"); break;
    case S_OP_LEVEL: eng(1); TSEL->p[P_E0] = 1; TSEL->p[P_FM4_LEVEL] = 0; go_title("OP LEVEL");   /* op 4 silent, op 3 hot */
        ui.hot_col = 2; ui.hot_t = 30; break;
    /* FM6's algorithm charts: 1 as it is; 5 with FB +3 just turned; 22 on EDIT 2, DTUN just turned (the carriers);
     * 32 with operator 6 at output level 0, MLVL just turned (no routes: nothing in ACCENT but nothing either) */
    case S_FM6_ALG1: eng(ENGI_FM6); TSEL->p[P_E0] = 1; go_title("EDIT 1"); break;
    /* DRUM-X: KIT, MORPH 40; then MORPH 100 and the hi-hat just struck (a key: its drum picked) (3b task 5: the ux page,
     * the drum's hit and the eight drums, no specimen) */
    case S_DRUMX: eng(ENGI_DRUM); TSEL->engine = TSEL->eng_req; TSEL->p[P_E0] = 40; go_title("EDIT 1"); break;
    case S_DRUMX_HIT:
        eng(ENGI_DRUM); TSEL->engine = TSEL->eng_req; TSEL->p[P_E0] = 100; go_title("EDIT 1");
        trk_note_on(TSEL, 36, 120); trk_note_on(TSEL, 42, 100); last_note = 42; dx_ui_lane = (uint8_t)drum_lane(42); events_block(256);
        break;
    /* EDIT > SOUND / SOUND 2: the snare's patch B, PITCH just turned; the kick's SOUND 2 */
    case S_DX_SOUND:
        eng(ENGI_DRUM); TSEL->engine = TSEL->eng_req; dx_ui_lane = 1; dx_ui_side = 1;
        nav_scene(SEC_EDIT, "SOUND"); ux_knob_touch(2);
        break;
    case S_DX_SOUND2:
        eng(ENGI_DRUM); TSEL->engine = TSEL->eng_req; dx_ui_lane = 0; dx_ui_side = 0;
        nav_scene(SEC_EDIT, "SOUND 2");
        break;
    case S_FM6_ALG5: eng(ENGI_FM6); TSEL->p[P_E0] = 5; TSEL->p[P_E1] = 3; go_title("EDIT 1"); ui.hot_col = 1; ui.hot_t = 30; break;
    case S_FM6_ALG22: eng(ENGI_FM6); TSEL->p[P_E0] = 22; TSEL->p[P_E6] = 40; go_title("EDIT 1"); ui.hot_col = 2; ui.hot_t = 30; break;   /* (3b
                                                         * task 5: EDIT 2 MORE's: on SOUND) */
    case S_FM6_ALG32:
        eng(ENGI_FM6); TSEL->p[P_E0] = 32; fm6_patch[song.sel][FP_OL] = 0; fm6_pgen[song.sel]++;   /* (op 6: the patch's first) */
        go_title("EDIT 1"); ui.hot_col = 2; ui.hot_t = 30;
        break;
    /* (ACID FM 3c-2 task 1) the dialogs full screen (ux_dialog.c), over the page each opens from; the longest names */
    case S_DLG_CLEAR_SEQ: case S_DLG_CLEAR_TRK:
        song.sel = 2; go_title("SAVE MORE");
        for (uint32_t i = 0; i < 14u; i++) { trk[2].step[i].time = ST_NOTE; trk[2].step[i].n = 1; trk[2].step[i].note[0] = (uint8_t)(48 + i); }
        (void)motion_set_lock(&trk[2], 1, P_FCUT, 40); (void)motion_set_lock(&trk[2], 5, P_FCUT, 90);
        (void)motion_set_lock(&trk[2], 9, P_FCUT, 70);
        ui.confirm = s == S_DLG_CLEAR_SEQ ? CF_CLEAR_SEQ : CF_CLEAR_TRK; ui.confirm_trk = 2;
        break;
    case S_DLG_CLEAR_ALL: song.sel = 2; nav_scene(SEC_SEQ, "STEPS"); ui.confirm = CF_CLEAR_ALL; ui.confirm_trk = 2; break;
    case S_DLG_CLEAR_MOTION:
        song.sel = 3; seq_more_at(UXR_ACTION, 0);
        (void)motion_set_lock(&trk[3], 2, P_FCUT, 40);
        ui.confirm = CF_CLEAR_MOTION; ui.confirm_trk = 3;
        break;
    case S_DLG_CLEAR_PFX: fx_more_at(0, UXR_ACTION); ui.confirm = CF_CLEAR_PFX; ui.confirm_trk = song.sel; break;
    case S_DLG_CLEAR_SONG: case S_DLG_DEL_ROW: {
        uint32_t i;
        song.playing = 0; go_title(s == S_DLG_DEL_ROW ? "SAVE SONG" : "SAVE MORE");
        chain_config.count = 6;
        for (i = 0; i < 6u; i++) chain_config.row[i] = (chain_row_t){(uint8_t)(i % 3u), (uint8_t)(16u + 4u * (i & 1u))};
        ui.confirm = s == S_DLG_DEL_ROW ? CF_DEL_ROW : CF_CLEAR_SONG; ui.confirm_trk = s == S_DLG_DEL_ROW ? 2 : song.sel;
        break;
    }
    case S_DLG_INIT_SOUND: case S_DLG_INIT_SOUND_DRUM:
        song.sel = s == S_DLG_INIT_SOUND ? 2 : 0; go_title("SAVE MORE");
        if (s == S_DLG_INIT_SOUND_DRUM) eng(ENGI_DRUM);
        ui.confirm = CF_INIT_SOUND; ui.confirm_trk = song.sel;
        break;
    case S_DLG_INIT_SYS: go_title("SAVE MORE"); ui.confirm = CF_INIT_SYS; ui.confirm_trk = song.sel; break;
    case S_DLG_CPU_TEST: go_title("SAVE MORE"); ui.confirm = CF_CPU_TEST; ui.confirm_trk = song.sel; break;
    case S_DLG_LOAD_PROJ:                        /* (3c-2 task 4) LOAD over unsaved changes, section B */
        song.playing = 0; project_save_as(1, "DAWN"); go_title("SAVE PROJECT"); song.g[G_SLOT] = 2;
        ui.confirm = CF_LOAD_PROJ; ui.confirm_trk = 1;
        break;
    case S_DLG_LIVE_COMMIT: go_title("LIVE"); ui.confirm = CF_LIVE_COMMIT; ui.confirm_trk = 0x3C; break;
    case S_DLG_LOAD_PAT: song.sel = 0; go_title("PATTERN"); ui.confirm = CF_LOAD_PAT; ui.confirm_trk = 0; ui.ppick = 4; break;
    case S_DLG_OVR_PROJ:
        song.playing = 0; project_save_as(1, "MWMWMWMWMWMW"); go_title("SAVE PROJECT");
        ui.confirm = CF_OVR_PROJ; ui.confirm_trk = 1;
        break;
    case S_DLG_OVR_USER: case S_DLG_ERASE_USER:
        song.playing = 0; up_store(6, "A VERY LONG SOUND NAME"); go_title("SAVE SOUND");
        ui.confirm = s == S_DLG_OVR_USER ? CF_OVR_USER : CF_ERASE_USER; ui.confirm_trk = 6;
        break;
    case S_DLG_RECALL_SOUND: song.sel = 0; go_title("EDIT 1"); ui.confirm = CF_RECALL_SOUND; ui.confirm_trk = 0; break;
    case S_GEN: ui.confirm = CF_DICE_ALL; gp.page = 0; gp.hot = 0; gp.genre = (uint8_t)(1u + GEN_SHOWN[4]); gp.seed = 421; str_cpy(gp.last, "ACID 0421", sizeof gp.last); break;   /* (0.6.91) GEN (3b: a shown genre, ACID) */
    case S_GEN_PARTS: ui.confirm = CF_DICE_ALL; gp.page = 1; gp.hot = 2; gp.amt[GP_CHORDS] = 70; gp.amt[GP_ARP] = 0; break;
    case S_MENU: ui.menu = 1; ui.menu_sel = 0; song.rec = 1; break;
    case S_MENU_SPEAKER: ui.menu = 1; ui.menu_sel = MI_LOWCUT; settings.lowcut = 2; break;
    case S_ABOUT: ui.menu = 2; ui.menu_scroll = 0; break;
    case S_ABOUT_REC: ui.menu = 2; ui.menu_scroll = 0; song.rec = 1; break;          /* the REC mark beside OCT- BACK */
    case S_ABOUT_CREDITS: ui.menu = 2; ui.menu_scroll = 360; break;
    case S_ABOUT_END: ui.menu = 2; ui.menu_scroll = (uint16_t)menu_scroll_max(); break;
    case S_ABOUT_PROFILE: case S_ABOUT_PROFILE2: {   /* (0.9.14) MENU > ABOUT > PROFILE: a second playing and one stopped,
                                                   * numbers made up for the picture (the host has no timer: prof.c) */
        static const uint16_t P[2][PRF_N][2] = {   /* % x10, avg and max (the half's as its own) */
            {{12, 20}, {8, 15}, {6, 9}, {6, 9}, {5, 8}, {7, 11}, {9, 13}, {3, 210}, {2, 160}, {4, 9}, {14, 25}, {38, 50}, {6, 9},
             {50, 220}, {14, 30}, {160, 240}},
            {{31, 95}, {78, 140}, {95, 160}, {62, 110}, {50, 90}, {84, 150}, {120, 190}, {210, 260}, {90, 110}, {60, 220},
             {20, 40}, {40, 52}, {15, 30}, {460, 600}, {25, 60}, {580, 810}}};
        uint32_t w, k;
        ui.menu = 3; ui.menu_scroll = s == S_ABOUT_PROFILE2 ? (uint16_t)menu_scroll_max() : 0;
        for (w = 0; w < 2u; w++) {
            memset(&prf_win[w], 0, sizeof prf_win[w]);
            prf_win[w].blocks = 1376u;
            for (k = 0; k < PRF_N; k++) {
                prf_win[w].sum[k] = P[w][k][0] * (1376u * (PRF_BLK / 8u) / 125u);
                prf_win[w].max[k] = P[w][k][1] * PRF_BLK / (k >= PRF_NB ? 250u : 1000u);
            }
            for (k = 0; k < NPART; k++) prf_win[w].c1[k] = k == 2u || k == 3u || k == 5u ? 1376u : 0u;
            prf_win[w].t5 = (w ? 32u : 30u) * (1376u * (PRF_BLK / 8u) / 125u);
            prf_win[w].lat = (w ? 190u : 140u) * 24u;
            prf_win[w].late = 0;
            prf_win[w].fxrest = w ? 0u : 1300u;
            prf_win[w].frames = w ? 52u : 60u;
        }
        prf_have = 3;
        for (k = 0; k < NPART; k++) { set_engine_of(&trk[k], TRK_DEF[k][0]); trk[k].engine = trk[k].eng_req; }
        break;
    }
    case S_UBOOT: ui.uboot = 3; break;
    /* the header's battery at each stock level (raw ADC: under 531, 531.., 561.., 591..) and on USB power */
    case S_BATT0: usb.config = 0; song.batt_raw = 500; go_title("SAVE MORE"); break;
    case S_BATT1: usb.config = 0; song.batt_raw = 540; go_title("SAVE MORE"); break;
    case S_BATT2: usb.config = 0; song.batt_raw = 570; go_title("SAVE MORE"); break;
    case S_BATT3: usb.config = 0; song.batt_raw = 600; go_title("SAVE MORE"); break;
    case S_BATT_USB: usb.config = 1; usb.suspended = 0; song.batt_raw = 500; go_title("SAVE MORE"); break;
    case S_MOTION_REC: song.rec = 1u << song.sel; seq_more_at(UXR_TRACK, ID_SQ_PLAY); break;    /* recording into the motion */
    case S_MOTION_OFF: song.playing = 0; seq_more_at(UXR_ACTION, UXM_A_CLRAUTO); break;  /* stopped, CLEAR picked */
    case S_MOTION_CARD: {                                          /* #63: EDIT 1 (ACID FM 3a: was HOME), MOTION drives KNOB 1
                                                                    * and 3 (3 just turned) */
        const page_t *pg;
        go_title("EDIT 1");
        pg = cur_page();
        motion_set_event(TSEL, 2, pg->id[0], TSEL->p[pg->id[0]]);
        motion_set_event(TSEL, 6, pg->id[2], TSEL->p[pg->id[2]]);
        ui.hot_col = 2; ui.hot_t = 30;
        break;
    }
    case S_SONG_HOME:                            /* the song playing: the disc and its row in the header */
        song.playing = 0; project_save(0); project_save(1);
        chain_config.count = 2;
        chain_config.row[0] = (chain_row_t){0, 2}; chain_config.row[1] = (chain_row_t){1, 4};
        go_page(GR_SVSONG); chain_prepare(); events_block(32); go_title("SAVE MORE"); ui.msg_t = 0;   /* (3a: a lens page's header) */
        break;
    /* the FX layer's map: held alone; REPEAT 1/16 + GLITCH + the CRUSH sweep + a mute playing with the macros turned;
     * at 72 BPM: a REPEAT 1/16 waiting for its 1/16 (shown THEME) beside the FILTER sweep playing, and REPEAT 1/8
     * held, too long at that tempo (dimmed) */
    case S_FX_PEEK: go_title("ENV"); ui.layer = LAYER_FX; break;
    case S_FX_HELD:
        go_home(); ui.layer = LAYER_FX;
        perf_held = perf_act = PF_BIT(PF_R16) | PF_BIT(PF_GLT) | PF_BIT(PF_SCRU) | PF_BIT(PF_M1 + 1);
        perf_m[0] = 6; perf_m[1] = 7; perf_m[2] = 40; perf_m[3] = 30; ui.hot_col = 0; ui.hot_t = 30;
        break;
    case S_FX_WAIT:
        song.playing = 1; song.g[G_BPM] = 72; ui.layer = LAYER_FX;
        perf_held = PF_BIT(PF_R16) | PF_BIT(PF_R8) | PF_BIT(PF_SHUF); perf_act = PF_BIT(PF_SHUF);
        perf_m[3] = 100;
        break;
    case S_FX_HARM:                                 /* (0.9.14) TAPE STOP and the THROW sweep, TRANSPOSE at -OCT */
        song.playing = 1; ui.layer = LAYER_FX;
        perf_held = perf_act = PF_BIT(PF_TAPE) | PF_BIT(PF_STHR);
        perf_m[1] = 21; ui.hot_col = 1; ui.hot_t = 30;
        break;
    /* (0.9.17) the glitcher's key down: its four knobs on the cards (MAP), as gauges (KNOBS: SHUFFLE, FREEZE); TAPE
     * stopped: SCRATCH TURN */
    case S_FX_GLT:
        song.playing = 1; ui.layer = LAYER_FX;
        perf_held = perf_act = perf_down = PF_BIT(PF_GLT); perf_gp[GX_GLT][1] = -5; perf_gp[GX_GLT][2] = 3;
        perf_gp[GX_GLT][3] = 16; ui.hot_col = 1; ui.hot_t = 30;
        break;
    case S_FX_SCR:
        song.playing = 1; ui.layer = LAYER_FX;
        perf_held = perf_act = perf_down = PF_BIT(PF_TAPE); pf.mode = BM_TAPE; pf.state = 1; perf_gp[GX_TAPE][2] = -100;
        break;
    case S_FX_SHUF:
        song.playing = 1; ui.layer = LAYER_FX; ui.uxlv = 1u << LAYER_FX;
        perf_held = perf_act = perf_down = PF_BIT(PF_SHUF) | PF_BIT(PF_R8); perf_gp[GX_SHUF][0] = 100; perf_gp[GX_SHUF][2] = 100;
        break;
    case S_FX_FRZ:
        song.playing = 1; ui.layer = LAYER_FX; ui.uxlv = 1u << LAYER_FX;
        perf_held = perf_act = perf_down = PF_BIT(PF_FRZ); perf_gp[GX_FRZ][1] = 12; perf_gp[GX_FRZ][3] = 0;
        break;
    case S_REVERB:                                      /* (3b task 8) SPACE: SHIMMER at LOAD LITE (plays as ROOM: said), REV TYPE
                                                         * touched */
        fx_scene(); fx_load(LOAD_LITE); song.g[G_RTYPE] = 2; nav_scene(SEC_FX, "SPACE"); ux_knob_touch(2); break;
    case S_REVERB_RESO:                                 /* .. SPACE: RESO (PITCH 4, FOLLOW T2) */
        fx_scene(); song.g[G_RTYPE] = 3; song.g[G_RDAMP] = 2; song.g[G_RPRE] = 4; nav_scene(SEC_FX, "SPACE"); break;
    case S_MENU_HOLD: ui.menu = 1; ui.menu_sel = MI_HOLD; settings_hold = 2; break;
    case S_MENU_LEDS: ui.menu = 1; ui.menu_sel = MI_LEDS; settings_leds = LEDS_INV; break;
    case S_MENU_END: ui.menu = 1; ui.menu_sel = MI_COUNT - 1u; ui_prefs = 0xFF; ui_rec_prefs = 0x2A; break;   /* the last row,
                                                     * every flag set (CLICK ON, LEVEL HIGH, COUNT-IN 2 BARS) */
    /* the MENU's tabs (1.0.5): menu DISPLAY (COLOR), menu_hold CONTROL, menu_speaker AUDIO, menu_system SYSTEM (USB
     * SERIAL); menu_slide: from DISPLAY to CONTROL with ANIM ON, the frame half way (draw) */
    case S_MENU_SYSTEM: ui.menu = 1; ui.menu_sel = MI_SERIAL; break;
    case S_MENU_SLIDE: ui.menu = 1; ui.menu_sel = MI_ACCEL; ui_prefs |= PREF_LATCH; break;
    /* the GLO SCL EDIT layers (ui_layer.c): just opened (a peek), and in use: GLO with T2 muted, T3 soloed (its key
     * held) and KNOB 1 turned; with CLK EXT (TAP dimmed); SCL at D# minor, KNOB 2 turned; EDIT on DIGITAL preset 3,
     * a favourite; a user preset; the hint after a tap */
    case S_GLO_PEEK: go_title("ENV"); ui.layer = LAYER_GLO; break;
    case S_GLO_ACTIVE:
        go_home(); ui.layer = LAYER_GLO; trk[1].p[P_MUTE] = 1; perf_solo = 4; trk[0].p[P_LEVEL] = 90;
        ui.hot_col = 0; ui.hot_t = 30;
        break;
    case S_GLO_EXT: go_home(); ui.layer = LAYER_GLO; song.g[G_CLOCK] = 1; trk[0].p[P_MUTE] = trk[3].p[P_MUTE] = 1; break;
    case S_SCL_PEEK: go_title("ENV"); ui.layer = LAYER_SCL; break;
    case S_SCL_ACTIVE: go_home(); ui.layer = LAYER_SCL; TSEL->p[P_ROOT] = 3; TSEL->p[P_SCALE] = 2; ui.hot_col = 1; ui.hot_t = 30; break;
    case S_EDIT_PEEK: go_title("EDIT 1"); ui.layer = LAYER_EDIT; break;
    case S_EDIT_ACTIVE: eng(E_FM); apply_preset_to(TSEL, 2); favorite_set(E_FM, 2, 1); go_home(); ui.layer = LAYER_EDIT;
        ui.hot_col = 0; ui.hot_t = 30; break;
    case S_EDIT_DRUM: eng(ENGI_DRUM); go_home(); dx_mute_set(DXM_LANE(2) | DXM_LANE(6)); ui.layer = LAYER_EDIT; break;
    case S_EDIT_USER: song.playing = 0; eng(2); up_store(6, "MY LONG PHASE NAME");   /* (ACID FM 0.2: PHASE, WHEEL retired) */
        up_load(6); favorite_set(NENGINES, 6, 1);
        go_home(); ui.layer = LAYER_EDIT; break;
    case S_LAYER_HINT: go_page(GR_MIX); ui.msg_t = 0; layer_tap(LAYER_GLO); break;
    /* #83: locked open by a double tap (the lock after the header's name): EDIT and FX */
    case S_LAYER_LOCK: go_title("ENV"); ui.layer = LAYER_EDIT; break;   /* (TONIC: no lock; the map as held) */
    case S_LAYER_LOCK_FX: go_title("ENV"); ui.layer = LAYER_FX; break;
    /* JIANT: the song layer, section B playing, SONG mode, SONG REC on; then a quick chain being tapped */
    case S_LAYER_SONG: go_home(); ui.layer = LAYER_SAVE; live.cur = 1; live.mode = 1; live.srec = 2; live.dirty = 4; break;
    case S_LAYER_SAVE:                           /* (3c-2 task 4) the SAVE layer in the ACID look: section B playing, SONG REC armed */
        song.playing = 0; project_save_as(0, "NIGHT RUN"); project_save_as(1, "DAWN");
        go_home(); ui.layer = LAYER_SAVE; live.cur = 1; live.srec = 1; live.dirty = 1; ui.msg_t = 0;
        break;
    case S_LAYER_SONG_CHAIN:
        go_home(); ui.layer = LAYER_SAVE; live.cur = 0; live.req = 2;
        lys.nqc = 4; lys.qc[0] = 0; lys.qc[1] = 1; lys.qc[2] = 1; lys.qc[3] = 2;
        break;
    /* 1.2 SEQ TOOLS (SEQ held on STEP): a melodic track (the sequence's tools), the DRUM grid (+ BEAT, the lane's tools,
     * the lanes; CLAP selected), a song playing (every tool dimmed: STOP TO EDIT) */
    case S_SEQ_TOOLS: go_page(GR_ROLL); ui.layer = LAYER_SEQ; break;
    case S_SEQ_TOOLS_DRUM: drum(0); go_page(GR_ROLL); ui.lane = 2; ui.layer = LAYER_SEQ; break;
    case S_SEQ_TOOLS_SONG: drum(1); go_page(GR_ROLL); ui.lane = 5; chain.running = 1; ui.layer = LAYER_SEQ; break;
    /* (0.8.3) LENTE, SEQ's three levels: PATTERN (a LEN 64 line over four bars, the third edited, playing in the
     * second; a kit's 32; STRETCH), BAR (the roll over LEN 64; the grid), STEP (a chord, its second note in focus,
     * MICRO +3, an accent, an ARP, 60 %; COMP's and COMP 2's chips lit; a TIE holding it; a REST; a kit's hits) */
    case S_LENTE_PAT:
    case S_LENTE_STRETCH:
    case S_LENTE_BAR:
    case S_LENTE_STEP:
    case S_LENTE_STEP_COMP:
    case S_LENTE_STEP_COMP2:
    case S_LENTE_STEP_TIE:
    case S_LENTE_STEP_REST:
    case S_LENTE_BAR_LONG:                              /* (0.8.4) a chord 16 steps long: TIME 16 ST, the big step's LEN 16 */
    case S_LENTE_STEP_LONG:
    {
        static const uint8_t CH[3] = {60, 64, 69};
        track_t *t = TSEL;
        uint32_t i;
        track_defaults_steps(t);
        t->p[P_SLEN] = 64; t->p[P_VOICE] = V_POLY;
        for (i = 0; i < 64u; i++) {
            uint8_t n = (uint8_t)(48 + (i * 7u + i / 16u * 5u) % 24u);
            if (i % 5u == 4u) step_put(&t->step[i], ST_TIE, 0, 0, &n);
            else if (i % 7u == 6u) step_put(&t->step[i], ST_REST, 0, 0, &n);
            else step_put(&t->step[i], ST_NOTE, i % 8u == 0u ? SF_ACCENT : 0u, 1, &n);
        }
        step_put(&t->step[34], ST_NOTE, SF_ACCENT | SF_SLIDE, 3, CH);
        ST_MICRO_SET(&t->step[34], 13); step_set_chance(&t->step[34], 60);   /* (0.9.1: +13/64) */
        t->step[34].comp = CP_ARPN; t->step[34].comp2 = CP_ECHO;
        step_put(&t->step[35], ST_TIE, 0, 0, CH);
        step_put(&t->step[36], ST_REST, 0, 0, CH);
        if (s == S_LENTE_BAR_LONG || s == S_LENTE_STEP_LONG) {
            step_put(&t->step[40], ST_NOTE, 0, 3, CH);
            for (i = 41; i < 56u; i++) step_put(&t->step[i], ST_TIE, 0, 0, CH);
        }
        if (s == S_LENTE_BAR_LONG)                      /* (0.8.5) ALL TR at the right of the bars' strip */
            song.g[G_STRN] = -12;
        song.playing = 1; t->seq_idx = 21;
        ui.cursor = (uint8_t)(s == S_LENTE_STEP_TIE ? 35 : s == S_LENTE_STEP_REST ? 36 : s == S_LENTE_BAR_LONG || s == S_LENTE_STEP_LONG ? 40 : 34);
        if (s == S_LENTE_PAT || s == S_LENTE_BAR || s == S_LENTE_BAR_LONG) {   /* (3c-1 task 4: STRETCH NOTE COMP COMP 2: SEQ >
                                                         * MORE's rows) */
            nav_scene(SEC_SEQ, s == S_LENTE_PAT ? "PATTERN" : "STEPS");
            cursor_set(ui.cursor);
        } else
            seq_more_at(UXR_TRACK, s == S_LENTE_STRETCH ? ID_SQ_STRETCH : s == S_LENTE_STEP_COMP2 ? ID_SQ_TYPE2 :
                        s == S_LENTE_STEP ? ID_SQ_FOCUS : ID_SQ_TYPE);
        ui.pr_focus = s == S_LENTE_STEP ? 2u : 0u;
        break;
    }
    case S_LENTE_PAT_DRUM:
    case S_LENTE_BAR_DRUM:
    case S_LENTE_STEP_DRUM:
        drum(0); trk[3].p[P_SLEN] = 32;
        memcpy(&trk[3].step[16], &trk[3].step[0], 16u * sizeof(step_t));
        trk[3].step[20].hit |= 0x30u; trk[3].step[20].acc |= 0x10u;
        ui.cursor = 20; ui.lane = 4;
        if (s == S_LENTE_STEP_DRUM)
            seq_more_at(UXR_TRACK, ID_SQ_TYPE);
        else {
            nav_scene(SEC_SEQ, s == S_LENTE_PAT_DRUM ? "PATTERN" : "STEPS");
            cursor_set(20);
        }
        break;
    /* NAME (ui_name.c): USER SAVE prefilled; a letter cycling (RS: S, R next); 123 on a project; an empty project name
     * (the placeholder); 12 of the widest letters, the cursor past them; playing (OCT+ dim) */
    case S_NAME_USER: song.playing = 0; go_page(GR_SVSND); ui.uslot = 6; name_open(NK_USER_SAVE, 6); break;
    case S_NAME_TYPING:
        song.playing = 0; go_page(GR_SVSND); name_open(NK_USER_SAVE, 6);
        str_cpy(nm.s, "SUB BAS", sizeof nm.s); nm.len = 7; nm.cur = 6; nm.key = 9; nm.tap = 1; nm.t = fm1_ms;
        break;
    case S_NAME_123:
        song.playing = 0; go_page(GR_SVPROJ); name_open(NK_PROJ_SAVE, 1);
        str_cpy(nm.s, "LIVE 2026", sizeof nm.s); nm.len = 9; nm.cur = 5; nm.num = 1;
        break;
    case S_NAME_EMPTY: song.playing = 0; go_page(GR_SVPROJ); proj_name[0] = 0; name_open(NK_PROJ_SAVE, 2); break;
    case S_NAME_FULL:
        song.playing = 0; up_store(4, "PAD"); go_page(GR_SVSND); name_open(NK_USER_RENAME, 4);
        str_cpy(nm.s, "MWMWMWMWMWMW", sizeof nm.s); nm.len = nm.cur = 12;
        break;
    case S_NAME_PLAYING: go_page(GR_SVSND); name_open(NK_USER_SAVE, 6); ui_message("STOP TO SAVE"); break;
    case S_PROJECT_NAMED:
        song.playing = 0; project_save_as(0, "LOFI JAM"); project_save_as(1, "MWMWMWMWMWMW"); project_save_as(2, "");
        song.g[G_SLOT] = 2; go_page(GR_SVPROJ); ui.act = 4; ui.msg_t = 0;
        break;
    case S_SONG_NAMED:
        song.playing = 0; project_save_as(0, "LOFI JAM"); project_save_as(1, "MWMWMWMWMWMW");
        chain_config.count = 3;
        chain_config.row[0] = (chain_row_t){0, 2}; chain_config.row[1] = (chain_row_t){1, 4}; chain_config.row[2] = (chain_row_t){2, 1};
        ui.song_row = 0; go_page(GR_SVSONG); ui.msg_t = 0;
        break;
    case S_ROLL_EMPTY: case S_ROLL_ACID: case S_ROLL_CHORDS: case S_ROLL_TIES: case S_ROLL_LEN32: case S_ROLL_HIGH:
    case S_ROLL_LOW: case S_ROLL_WIDE: case S_ROLL_PLAYING: roll_scene(s); break;
    /* 1.1 parameter locks (ui_input.c lock_*): the roll's steps with a lock marked under them; a step held (the note
     * key of its entry) shows what KNOB 1..4 lock (HOME's: ANALOG E5 E6 ATK REL), the locked ones bright; the DRUM grid
     * with its marks over the steps; a step key held after FX (DIST CHO DLY REV, REV and DLY locked); AUTOMATION's
     * LOCK count */
    case S_ROLL_LOCKS: case S_ROLL_LOCK_HELD:
        roll_scene(S_ROLL_ACID);
        motion_set_lock(TSEL, 2, P_REV, 100); motion_set_lock(TSEL, 6, P_E4, 20); motion_set_lock(TSEL, 6, P_ATK, 90);
        motion_set_lock(TSEL, 9, P_E5, 110); motion_set_lock(TSEL, 13, P_REV, 80);
        if (s == S_ROLL_LOCK_HELD) {
            ui.plk_src = 0;
            ui.entry_open = 1;
            fm1_in.notes = 1u << 10;
            ui.hot_col = 0; ui.hot_t = 30;
        }
        break;
    case S_DRUM_LOCKS: case S_DRUM_LOCK_HELD:
        drum(0); go_page(GR_ROLL); ui.cursor = 4; ui.lane = 1; trk[3].seq_idx = 9;
        motion_set_lock(TSEL, 2, P_E1, 90); motion_set_lock(TSEL, 4, P_REV, 100); motion_set_lock(TSEL, 4, P_DLY, 70);
        motion_set_lock(TSEL, 8, P_E2, 30); motion_set_lock(TSEL, 15, P_E1, 40);
        if (s == S_DRUM_LOCK_HELD) {
            uint32_t i;
            for (i = 0; i < NPAGES && PAGES[i].graph != GR_FX; i++)
                ;
            ui.plk_src = (uint8_t)(i + 1u);
            fm1_in.notes = 1u << key_at(0, 4);
            ui.hot_col = 3; ui.hot_t = 30;
        }
        break;
    /* (0.9.12) a step held + SELECT: the knobs lock KIT 2 (NOIS FM FOLD DRV; FOLD locked on the step, its card bright),
     * the footer LOCK . KIT 2 and its place in the walk; the CP lane selected */
    case S_DRUM_LOCK_PAGE: {
        uint8_t v[16];
        uint32_t at, n;
        drum(0); go_page(GR_ROLL); ui.cursor = 4; ui.lane = 2;
        grid_hit(TSEL, 4, 2, 1);
        dx_ui_lane = 2;           /* (3b task 5: SOUND, the CP's LEVEL; KIT's DECAY) */
        motion_set_lock(TSEL, 4, P_LN2, 100); motion_set_lock(TSEL, 4, P_E3, 30);
        fm1_in.notes = 1u << key_at(0, 4);
        n = lock_chain(v, &at);
        ui.plk_src = n > 1u ? (uint8_t)(nv_page(SEC_EDIT, v[1]) + 1u) : 0u;   /* (KIT, SOUND: the walk's second) */
        ui.hot_col = 2; ui.hot_t = 30;
        break;
    }
    case S_MOTION_LOCKS:
        seq_line();
        motion_set_event(TSEL, 1, P_REV, 40); motion_set_lock(TSEL, 2, P_REV, 100); motion_set_lock(TSEL, 6, P_ATK, 90);
        seq_more_at(UXR_TRACK, ID_SQ_LOCKS);
        break;
    /* 1.1.5 SEQ > AUTO LIST (ui_events.c): locks and automation events of the track, the 4th row selected, its
     * VALUE just turned; empty (+ ADD LOCK: step 5, the first knob of HOME); a DRUM track's long list, scrolled
     * (its lane levels, E names, the last row), playing */
    case S_AUTO_LIST:
        seq_line();
        motion_set_event(TSEL, 0, P_LRATE, 70); motion_set_lock(TSEL, 2, P_REV, 100); motion_set_lock(TSEL, 2, P_ED_FLT, -30);
        motion_set_lock(TSEL, 6, P_ATK, 90); motion_set_lock(TSEL, 6, P_E4, 20); motion_set_event(TSEL, 8, P_PAN, -40);
        motion_set_lock(TSEL, 11, P_LD_FLT, 50); motion_set_lock(TSEL, 13, P_DLY, 80); motion_set_lock(TSEL, 15, P_GLIDE, 60);
        seq_more_at(UXR_TRACK, ID_SQ_EVAL); ui.ev_row = 3;
        break;
    case S_AUTO_LIST_ADD: ui.cursor = 4; seq_more_at(UXR_TRACK, ID_SQ_ROW); break;
    case S_AUTO_LIST_DRUM: {
        uint32_t i;
        drum(0); go_page(GR_ROLL);
        for (i = 0; i < 12u; i++)
            motion_set_lock(TSEL, i, i & 1u ? P_LN0 + i % 8u : P_E0 + i % 8u, 100);
        motion_set_event(TSEL, 15, P_REV, 127);
        seq_more_at(UXR_TRACK, ID_SQ_EPARAM); ui.ev_row = 9; song.playing = 1;
        break;
    }
    /* (3b task 8) FX on the ux pages (fx_scene's values): SENDS, SPACE (1/8, FDBK 70, ROOM 70), MASTER (the meter at -9 dB), MORE
     * (DIST TYPE picked; the PUNCH LANE rows; DELAY's; REVERB's on RESO; the SLICER's), SPACE with PING and ANALOG FDBK 110 */
    case S_FXL_SENDS: fx_scene(); nav_scene(SEC_FX, "SENDS"); break;
    case S_FXL_BUSES: fx_scene(); nav_scene(SEC_FX, "SPACE"); break;
    case S_FXL_MASTER: fx_scene(); fx_scope(11600); nav_scene(SEC_FX, "MASTER"); break;
    case S_FXL_DIST: fx_more_at(P_DTYPE, UXR_TRACK); break;
    case S_FXL_PAN: fx_more_at(UXM_A_LANE, UXR_ACTION); break;
    case S_FXL_DELAY:
        fx_scene(); song.g[G_DPING] = 1; song.g[G_DTYPE] = 0; song.g[G_DFDBK] = 110; song.g[G_RTYPE] = 1; nav_scene(SEC_FX, "SPACE");
        break;
    case S_FXL_DELAY2: fx_more_at(G_DMIX, UXR_GLOBAL); break;
    case S_FXL_REVERB: fx_more_at(G_RPRE, UXR_GLOBAL); song.g[G_RTYPE] = 3; song.g[G_RPRE] = 4; song.g[G_RDAMP] = 2; break;
    case S_FXL_SLICER: fx_more_at(G_SLPAT, UXR_GLOBAL); break;
    /* (0.8.5) SAVE's lens as the mock-up's data: projects NOCHE LARGA (A B C), ACID 03, DUB KIT, BOSQUE, TRANCE A; the
     * song B16 C16 A32 D8 C16 A32 (scenes on 2 and 4, END LOOP); seven user sounds. (ACID FM 3c-2 task 2) SOUND on U05 (WOBBLE,
     * used) and on U08 (empty), SAVE armed; MORE with ENGINE picked; (task 3) SONG: the song, section B picked, A playing, ROW 3
     * picked; and with no rows */
    case S_SAVE_PROJECT: case S_SAVE_MORE_END: case S_SAVE_SOUND: case S_SAVE_SOUND_EMPTY:
    case S_SAVE_MORE: case S_SAVE_SONG: case S_SAVE_SONG_EMPTY: {
        static const char *const PN[NSONG] = {"", "ACID 03", "", ""};   /* (ACID FM 0.2: four projects; 3c-2 task 4: two used) */
        static const uint8_t PM[NSONG] = {0, 3, 0, 0};
        static const chain_row_t AR[6] = {{1, 16}, {2, 16}, {0, 32}, {3, 8}, {2, 16}, {0, 32}};
        static const char *const UN[7] = {"DEEP SUB", "GLASS", "PAD NUBE", "WOBBLE", "BIRDCALL", "KEYS 2", "GRIT"};
        static const uint8_t UK[7] = {0, 1, 2, 4, 6, 9, 13}, UE[7] = {ENGI_ANALOG, ENGI_FM6, 2, ENGI_ANALOG, ENGI_NOISE_UI, ENGI_FM6, 3};   /* (BIRDCALL: NOISE, ANIMAL retired) */
        uint32_t k, e0 = TSEL->eng_req;
        song.playing = 0;
        project_save_as(0, "NOCHE LARGA"); project_save_as(1, "NOCHE B"); project_save_as(2, "NOCHE C");
        for (k = 0; k < NSONG; k++) { str_cpy(lib.name[k], PN[k], sizeof lib.name[k]); lib.mask[k] = PM[k]; }
        chain_config.count = 6;
        for (k = 0; k < 6u; k++) chain_config.row[k] = AR[k];
        SONG_END(&chain_config) = SONG_FOREVER;
        song_idx.scene[song_cur][1].on = 1; song_idx.scene[song_cur][3].on = 1;
        live.cur = 0; song.g[G_SLOT] = 2;
        for (k = 0; k < 7u; k++) { eng(UE[k]); up_store(UK[k], UN[k]); }
        eng(e0);
        ui.uslot = 0; ui.song_row = 0; ui.msg_t = 0; ui.frame += 7u;   /* (the sections' cache read again) */
        lib.gen = lib_gen(); lib.next = NSONG;          /* (3c-2 task 4: the other projects as scanned) */
        if (s == S_SAVE_PROJECT) { proj_unsaved = 1; nav_scene(SEC_SAVE, "PROJECT"); ui.act = 2; }   /* (the music changed since: */
        else if (s == S_SAVE_MORE_END) {                /* the dot; LOAD armed) */
            uxm_at[nav_key(SEC_SAVE)].sel = (uint8_t)(UXM_SAVE.n - 1u);
            uxm_at[nav_key(SEC_SAVE)].top = (uint8_t)(UXM_SAVE.n - UXM_ROWS);
            nav_scene(SEC_SAVE, "MORE");
        }
        else if (s == S_SAVE_SONG || s == S_SAVE_SONG_EMPTY) {
            if (s == S_SAVE_SONG_EMPTY) chain_config.count = 0;
            ui.song_row = s == S_SAVE_SONG ? 2u : 0u;
            nav_scene(SEC_SAVE, "SONG");
        }
        else if (s == S_SAVE_MORE) {
            uint32_t r;
            for (r = 0; r < UXM_SAVE.n && !(UXM_SAVE.rows[r].kind == UXR_GLOBAL && UXM_SAVE.rows[r].id == ID_SV_ENGINE); r++)
                ;
            uxm_at[nav_key(SEC_SAVE)].sel = (uint8_t)r;
            uxm_at[nav_key(SEC_SAVE)].top = 0;
            nav_scene(SEC_SAVE, "MORE");
        } else {
            nav_scene(SEC_SAVE, "SOUND");
            ui.uslot = s == S_SAVE_SOUND ? 4u : 7u;
            ui.act = 4;
        }
        uxs_seen |= 1u << SEC_SAVE; ux_hot_k = -1;
        ui.msg_t = 0;
        break;
    }
    /* (0.8.5) VOICE's words: EDIT > WORDS singing (TEXT, a consonant sounding; W3 in English; the dice in Japanese),
     * EDIT 1 singing, WORDS with LANG OFF; the LYRIC typing screen (a short lyric; a long one, its window scrolled) */
    case S_WORDS: case S_WORDS_EN: case S_WORDS_JA: case S_WORDS_EDIT: {
        static const char *const W[4][2] = {{"corazon", "la noche es azul y el mar canta para ti"}, {"sunshine", ""},
                                            {"sakura", ""}, {"strength", "construir estrellas"}};
        static const uint8_t PH[4][4] = {{PH_S, PH_O, PH_N, 0}, {PH_SH, PH_AA, PH_IH, PH_N}, {PH_K, PH_UJ, 0, 0},
                                         {PH_S, PH_T, PH_RA, PH_EH}};
        uint32_t k = s - S_WORDS;
        sing_t *g = &sing[song.sel];
        eng(ENGI_VOICE_UI);
        TSEL->p[P_VLANG] = (int16_t)(k == 1u ? LG_EN : k == 2u ? LG_JA : LG_ES);
        TSEL->p[P_VLYR] = (int16_t)(k == 1u ? 3 : k == 2u ? 4 : 0);
        TSEL->p[P_E1] = (int16_t)(k == 1u ? 8 : 64);
        lyr_set(song.sel, W[k][1]);
        memset(g, 0, sizeof *g);
        str_cpy(g->w, W[k][0], sizeof g->w);
        memcpy(g->ph, PH[k], 4); g->np = PH[k][3] ? 4u : PH[k][2] ? 3u : 2u; g->nu = 1; g->cd = 2; g->i = k == 3u ? 2u : 1u;
        g->nsy = (uint16_t)(7u + k * 5u); g->to = k ? 0u : 13u;
        g->f[0] = 520; g->f[1] = 950; g->f[2] = 2450; g->ff = 3300; g->fb = 250; g->bm = 16;
        g->gv = k == 3u ? 2000 : 30000; g->gf = k == 3u ? 22000 : 0;
        if (k == 3u) go_title("EDIT 1");
        else nav_scene(SEC_EDIT, "MORE");               /* (3b task 4: WORDS' rows are EDIT's MORE's) */
        break;
    }
    case S_WORDS_OFF: eng(ENGI_VOICE_UI); TSEL->p[P_VLANG] = 0; nav_scene(SEC_EDIT, "MORE"); break;
    case S_NAME_LYRIC:
        eng(ENGI_VOICE_UI); TSEL->p[P_VLANG] = LG_ES; lyr_set(song.sel, "LA NOCHE");
        nav_scene(SEC_EDIT, "MORE"); name_open(NK_LYRIC, song.sel);
        break;
    case S_NAME_LYRIC_LONG:
        eng(ENGI_VOICE_UI); TSEL->p[P_VLANG] = LG_JA; song.playing = 1;
        lyr_set(song.sel, "SAKURA SAKURA YAYOI NO SORA WA MI WATASU KAGIRI KASUMI KA KUMO");
        nav_scene(SEC_EDIT, "MORE"); name_open(NK_LYRIC, song.sel); nm.cur = 40; nm.key = 3; nm.tap = 1; nm.t = fm1_ms;
        break;
    /* (0.8.5) DRUM-X octaves: the grid's HIT as its modes (the hat on step 3 x4; the snare on step 5 GHOST, the hat
     * beside it FLAM); OCT on a DRUM track names the octave (+3 RATCHET x4 since 0.9.13, -3 FLAM); COMP's TOUCH and ECHO EVEN */
    case S_DRUM_X4: case S_DRUM_GHOST:
        drum(0); go_page(GR_ROLL); trk[3].seq_idx = 9;
        step_set_lmode(&trk[3].step[2], 3, LM_X4); step_set_lmode(&trk[3].step[4], 1, LM_GHOST);
        step_set_lmode(&trk[3].step[4], 3, LM_FLAM);
        if (s == S_DRUM_X4) { ui.cursor = 2; ui.lane = 3; } else { ui.cursor = 4; ui.lane = 1; }
        break;
    case S_DRUM_OCT_UP: case S_DRUM_OCT_DN:
        drum(0); go_page(GR_ROLL); ui.cursor = 4; ui.lane = 1;
        song.octave = s == S_DRUM_OCT_UP ? 3 : -3;
        ui_message(drum_oct_name(song.octave));
        break;
    case S_GLO_LIVE: case S_GLO_LIVE_TRICKS: case S_GLO_LIVE_LATCH: case S_GLO_MORE: live_scene(s); break;
    case S_COMP_TOUCH: case S_COMP_EVEN:
        drum(0);
        trk[3].step[0].comp = s == S_COMP_TOUCH ? CP_TOUCH : CP_ECHO;
        trk[3].step[0].carg = s == S_COMP_TOUCH ? 1u : (uint8_t)(2u | CP_ECHO_EVEN);
        ui.cursor = 0; seq_more_at(UXR_TRACK, ID_SQ_TYPE);
        break;
    /* (0.9.18) SEQ > COMP: the step the component goes on framed (THEME, a tab at the foot), the steps with the same
     * component a bar on top (ACCENT), those with another a thin one; a synth's roll, a kit's grid, a JUMP's step */
    case S_COMP_SEL: case S_COMP_JUMP: {
        static const uint8_t N[16] = {48, 0, 55, 51, 60, 0, 53, 58, 48, 63, 0, 55, 60, 51, 0, 58};
        track_t *t = TSEL;
        uint32_t i;
        track_defaults_steps(t);
        t->p[P_SLEN] = 16;
        for (i = 0; i < 16u; i++) {
            uint8_t n = N[i];
            step_put(&t->step[i], n ? ST_NOTE : ST_REST, 0, n ? 1u : 0u, &n);
        }
        t->step[2].comp = CP_ECHO; t->step[2].carg = 1;
        step_set_ratchet(&t->step[7], 3);                /* (a RATCH of its flags: the same component) */
        t->step[12].comp = CP_ECHO; t->step[12].carg = 2;
        t->step[4].comp = CP_RAND; t->step[4].carg = 2;
        t->step[9].comp = CP_JUMP; t->step[9].carg = 3;
        ui.cursor = (uint8_t)(s == S_COMP_JUMP ? 9 : 12);
        seq_more_at(UXR_TRACK, s == S_COMP_JUMP ? ID_SQ_VAL : ID_SQ_TYPE);
        break;
    }
    /* (ACID FM 3b task 4) EDIT on a synth track, the ux pages (ux_section.c): SOUND (its first opening: the hint), TONE,
     * VOICE (ANALOG) / FILTER (the others), MORE (its first setting picked); the modes' own names (LOFI BYTE, NOISE LFSR,
     * VOICE WORDS); VOICE's MORE scrolled to its end; KNOB 3 turned on MORE (what the knobs do); a knob just turned */
    case S_EDIT_ANALOG_1: case S_EDIT_ANALOG_2: case S_EDIT_ANALOG_3: case S_EDIT_ANALOG_MORE: case S_EDIT_ANALOG_TURN:
    case S_EDIT_PHASE_1: case S_EDIT_PHASE_2: case S_EDIT_PHASE_3: case S_EDIT_PHASE_MORE: case S_EDIT_PHASE_MORE_K3:
    case S_EDIT_LOFI_1: case S_EDIT_LOFI_2: case S_EDIT_LOFI_3: case S_EDIT_LOFI_MORE: case S_EDIT_LOFI_BYTE_1:
    case S_EDIT_LOFI_BYTE_MORE: case S_EDIT_NOISE_1: case S_EDIT_NOISE_2: case S_EDIT_NOISE_3: case S_EDIT_NOISE_MORE:
    case S_EDIT_NOISE_LFSR_2: case S_EDIT_VOICE_1: case S_EDIT_VOICE_2: case S_EDIT_VOICE_3: case S_EDIT_VOICE_MORE:
    case S_EDIT_VOICE_WORDS_1: case S_EDIT_VOICE_MORE_END: {
        static const struct { int s0, s1; uint32_t e; const char *pre; } E[] = {
            {S_EDIT_ANALOG_1, S_EDIT_ANALOG_MORE, ENGI_ANALOG, "ACID"}, {S_EDIT_PHASE_1, S_EDIT_PHASE_MORE, ENGI_PHASE, "BRASS"},
            {S_EDIT_LOFI_1, S_EDIT_LOFI_MORE, ENGI_LOFI, "PULSE LD"}, {S_EDIT_NOISE_1, S_EDIT_NOISE_MORE, ENGI_NOISE, "RAIN"},
            {S_EDIT_VOICE_1, S_EDIT_VOICE_MORE, ENGI_VOICE, "TALK BOX"}};
        static const char *const PG[2][4] = {{"SOUND", "TONE", "VOICE", "MORE"}, {"SOUND", "TONE", "FILTER", "MORE"}};
        uint32_t k = 0, i, pg = 0;
        const char *pre;
        for (k = 0; k + 1u < NELEM(E) && !(s >= E[k].s0 && s <= E[k].s1); k++)
            ;
        pre = E[k].pre;
        if (s >= E[k].s0 && s <= E[k].s1) pg = (uint32_t)(s - E[k].s0);
        if (s == S_EDIT_ANALOG_TURN) k = 0, pre = "SAW LEAD", pg = 1;
        if (s == S_EDIT_PHASE_MORE_K3) k = 1, pre = E[1].pre, pg = 3;
        if (s == S_EDIT_LOFI_BYTE_1 || s == S_EDIT_LOFI_BYTE_MORE) k = 2, pre = "BYTEBEAT", pg = s == S_EDIT_LOFI_BYTE_1 ? 0u : 3u;
        if (s == S_EDIT_NOISE_LFSR_2) k = 3, pre = "ARCADE", pg = 1;
        if (s == S_EDIT_VOICE_WORDS_1 || s == S_EDIT_VOICE_MORE_END) k = 4, pre = "CANTOR ES", pg = s == S_EDIT_VOICE_WORDS_1 ? 0u : 3u;
        eng(E[k].e);
        for (i = 0; i < ENGINES[E[k].e]->npresets; i++)
            if (!strcmp(ENGINES[E[k].e]->presets[i].name, pre)) apply_preset_to(TSEL, i);
        TSEL->engine = TSEL->eng_req;
        if (s == S_EDIT_NOISE_LFSR_2) TSEL->p[P_E0] = 2;
        uxs_seen = pg ? 0x3FFu : 0u;                    /* (SOUND: its first opening, the hint; the others: said) */
        ux_hot_k = -1;
        memset(uxm_at, 0, sizeof uxm_at);
        nav_scene(SEC_EDIT, PG[E[k].e != ENGI_ANALOG][pg]);
        /* (the turns through edit_param: no frame drawn here, the lint's boxes are the scene's own) */
        if (s == S_EDIT_ANALOG_TURN) { edit_param(0, -6); ux_knob_touch(0); }   /* (CUTOFF turned: its column lit, the readout) */
        if (s == S_EDIT_PHASE_MORE_K3) { edit_param(2, 1); ux_knob_touch(2); }  /* (KNOB 3 on MORE: what the knobs do) */
        if (s == S_EDIT_VOICE_MORE_END) { (void)ux_more_knob(UXM_SELECT, 10); (void)ux_more_knob(0, 20); ux_knob_touch(1); }
        if (s == S_EDIT_ANALOG_MORE) { (void)ux_more_knob(0, 1); ux_knob_touch(1); }   /* (FILTER picked, KNOB 2 just turned) */
        break;
    }
    /* (3b task 5) EDIT on FM6 (TINE EP: SOUND, OPERATOR with OP 3 picked, OP ENV with OP 1's DECAY stage, MORE; algorithm 4: its
     * feedback from OP 4 into OP 6; MOD LEVEL just turned) and DRUM-X (DRUM-X: KIT, SOUND with the snare picked, SOUND 2 with
     * the hi-hat, MORE; two drums muted by EDIT's black keys; MORE scrolled to its end) */
    case S_EDIT_FM6_1: case S_EDIT_FM6_2: case S_EDIT_FM6_3: case S_EDIT_FM6_MORE: case S_EDIT_FM6_ALG4: case S_EDIT_FM6_TURN:
    case S_EDIT_DRUM_1: case S_EDIT_DRUM_2: case S_EDIT_DRUM_3: case S_EDIT_DRUM_MORE: case S_EDIT_DRUM_MUTED:
    case S_EDIT_DRUM_MORE_END: {
        static const char *const PF[4] = {"SOUND", "OPERATOR", "OP ENV", "MORE"}, *const PD[4] = {"KIT", "SOUND", "SOUND 2", "MORE"};
        int fm = s <= S_EDIT_FM6_TURN;
        uint32_t pg = fm ? (s <= S_EDIT_FM6_MORE ? (uint32_t)(s - S_EDIT_FM6_1) : 0u) :
                           (s <= S_EDIT_DRUM_MORE ? (uint32_t)(s - S_EDIT_DRUM_1) : s == S_EDIT_DRUM_MUTED ? 1u : 3u);
        eng(fm ? ENGI_FM6 : ENGI_DRUM);
        apply_preset_to(TSEL, 0);
        TSEL->engine = TSEL->eng_req;
        fop_op = (uint8_t)(pg == 1u || pg == 3u ? 2u : 0u); fop_stg = 1;
        dx_ui_lane = (uint8_t)(pg == 2u ? 3u : 1u); dx_ui_side = 0;
        if (s == S_EDIT_FM6_ALG4) TSEL->p[P_E0] = 4, TSEL->p[P_E1] = 4;
        if (s == S_EDIT_DRUM_MUTED) dx_mute_set(DXM_LANE(2) | DXM_LANE(6));
        uxs_seen = pg ? 0x3FFu : 0u;
        ux_hot_k = -1;
        memset(uxm_at, 0, sizeof uxm_at);
        nav_scene(SEC_EDIT, fm ? PF[pg] : PD[pg]);
        if (s == S_EDIT_FM6_TURN) { edit_param(2, 12); ux_knob_touch(2); }   /* (MOD LEVEL turned: the modulators larger) */
        if (s == S_EDIT_DRUM_MORE_END) { (void)ux_more_knob(UXM_SELECT, 30); (void)ux_more_knob(0, 40); ux_knob_touch(1); }
        if (s == S_EDIT_FM6_MORE) { (void)ux_more_knob(0, 9); ux_knob_touch(1); }   /* (OP 3's DETUNE picked: its rows in view) */
        break;
    }
    /* (ACID FM 3b task 9) the held layers in the ACID look (ux_layer.c): EDIT (KNOB 1..4 EDIT's SOUND), MACRO (two routed, M1 just
     * turned: its readout), FX held (REPEAT 1/16, GLITCH, CRUSH, T2's mute; the macros up), SCL (D# MINOR), FX's STATE (EDIT on a
     * DRUM-X track: layer_edit_drum, S_EDIT_DRUM); a step held on STEP with EDIT's MORE the locks' page (MORPH locked on it) */
    case S_LAYER_EDIT: case S_LAYER_MACRO: case S_LAYER_FX: case S_LAYER_SCL: case S_LAYER_FX_STATE:
        go_title("ENV"); ui.uxlv = 0; ux_hot_k = -1;
        ui.layer = (uint8_t)(s == S_LAYER_EDIT ? LAYER_EDIT : s == S_LAYER_MACRO ? LAYER_MACRO :
                             s == S_LAYER_SCL ? LAYER_SCL : LAYER_FX);
        if (s == S_LAYER_MACRO) {
            macro_v[0] = 90; macro_v[1] = 40; macro_v[3] = 127;
            trk[0].p[P_M1SRC] = MS_M1; trk[0].p[P_M1DST] = MD_CUT; trk[0].p[P_M1AMT] = 40;
            trk[1].p[P_M1SRC] = MS_M1 + 1; trk[1].p[P_M1DST] = MD_CLIP; trk[1].p[P_M1AMT] = 63;
            ux_knob_touch(0);
        }
        if (s == S_LAYER_FX || s == S_LAYER_FX_STATE) {
            perf_held = perf_act = PF_BIT(PF_R16) | PF_BIT(PF_GLT) | PF_BIT(PF_SCRU) | PF_BIT(PF_M1 + 1);
            perf_m[0] = 6; perf_m[1] = 7; perf_m[2] = 40; perf_m[3] = 30;
            ui.uxlv = (uint16_t)(s == S_LAYER_FX_STATE ? 1u << LAYER_FX : 0u);
        }
        if (s == S_LAYER_SCL) { TSEL->p[P_ROOT] = 3; TSEL->p[P_SCALE] = 2; }
        break;
    case S_ROLL_LOCK_MORE: {                            /* (MORPH picked in EDIT's MORE, locked on the step held) */
        uint32_t r, i;
        roll_scene(S_ROLL_ACID);
        for (i = 0; i < NPAGES && PAGES[i].more != &UXM_SYN; i++)
            ;
        for (r = 0; r < UXM_SYN.n && UXM_SYN.rows[r].id != P_MORPH; r++)
            ;
        memset(uxm_at, 0, sizeof uxm_at);
        uxm_at[nav_key(SEC_EDIT) % NAV_KEYS].sel = (uint8_t)r;
        ui.plk_src = (uint8_t)(i + 1u);
        motion_set_lock(TSEL, ui.cursor, P_MORPH, 90);
        ui.entry_open = 1;
        fm1_in.notes = 1u << 10;
        ui.hot_col = 1; ui.hot_t = 30;
        break;
    }
    /* (ACID FM 3c-1 task 2) SEQ > STEPS: a synth's 16 (an acid line: accents, slides, a tie, a chord, a rest; step 5 picked,
     * G2 . ACC, playing on 9, KNOB 3 just turned); 64 steps on its third 16 (a long note, 3/4 in the header); the DRUM grid
     * (SNARE picked, accents, playing) */
    case S_SEQ_STEPS:
    case S_SEQ_STEPS_64: {
        static const uint8_t LINE[16] = {36, 0, 48, 43, 43, 0, 46, 48, 41, 0, 53, 48, 36, 0, 39, 43};
        static const uint8_t CH3[3] = {48, 51, 55};
        track_t *t = TSEL;
        uint32_t i, n = s == S_SEQ_STEPS_64 ? 64u : 16u;
        eng(ENGI_ANALOG); t->engine = t->eng_req;
        track_defaults_steps(t);
        t->p[P_SLEN] = (int16_t)n;
        for (i = 0; i < n; i++) {
            uint8_t nt = (uint8_t)(LINE[i % 16u] + (i / 16u) * 2u);
            if (!LINE[i % 16u])
                step_put(&t->step[i], i % 3u ? ST_REST : ST_TIE, 0, 0, &nt);
            else
                step_put(&t->step[i], ST_NOTE, i % 16u == 4u || i % 16u == 11u ? SF_ACCENT : i % 16u == 7u ? SF_SLIDE : 0u, 1, &nt);
        }
        step_put(&t->step[8], ST_NOTE, SF_SLIDE, 3, CH3);
        if (s == S_SEQ_STEPS_64) {
            step_put(&t->step[34], ST_NOTE, SF_ACCENT, 1, CH3);
            for (i = 35; i < 38u; i++) step_put(&t->step[i], ST_TIE, 0, 0, CH3);
        }
        song.playing = 1; t->seq_idx = s == S_SEQ_STEPS_64 ? 40u : 8u;
        nav_scene(SEC_SEQ, "STEPS");
        cursor_set(s == S_SEQ_STEPS_64 ? 34 : 4);
        if (s == S_SEQ_STEPS) ux_knob_touch(2);
        break;
    }
    case S_SEQ_STEPS_DRUM:
        drum(0); trk[3].p[P_SLEN] = 32;
        memcpy(&trk[3].step[16], &trk[3].step[0], 16u * sizeof(step_t));
        trk[3].step[20].hit |= 0x30u; trk[3].step[20].acc |= 0x10u;
        song.playing = 1; trk[3].seq_idx = 22;
        nav_scene(SEC_SEQ, "STEPS");
        cursor_set(20); ui.lane = 1;
        break;
    /* (ACID FM 3c-1 task 4) SEQ > MORE: step 5 an ARP (DOWN over two octaves) and a RATCHET x5 RISE, EVERY 1ST OF 4, TYPE picked;
     * the list scrolled to its end (AUTO LIST's rows, two records); a DRUM track's (no FOCUS PITCH MOVE); the lock card: a step
     * held on the roll, EDIT's MORE the locks' page, MORPH locked, KNOB 2 just turned (LOCK . MORPH . value) */
    case S_SEQ_MORE: case S_SEQ_MORE_END: {
        track_t *t = TSEL;
        static const uint8_t CH3[3] = {48, 51, 55};
        eng(ENGI_ANALOG); t->engine = t->eng_req;
        track_defaults_steps(t); t->p[P_SLEN] = 16;
        step_put(&t->step[4], ST_NOTE, SF_ACCENT, 3, CH3);
        t->step[4].comp = CP_ARPN; t->step[4].carg = (uint8_t)CP_ARG(1u, 5u, 1u);
        t->step[4].comp2 = CP_ECHO; t->step[4].carg2 = (uint8_t)(3u | CP_ECHO_RISE);
        ST_EVERY_SET(&t->step[4], EV_1_4);
        motion_set_event(t, 2, P_FCUT, 40); motion_set_lock(t, 4, P_REV, 100);
        ui.cursor = 4; ui.comp_at = 0;
        seq_more_at(UXR_TRACK, s == S_SEQ_MORE ? ID_SQ_TYPE : ID_SQ_EVAL);
        break;
    }
    case S_SEQ_MORE_DRUM:
        drum(0); ui.cursor = 4;
        trk[3].step[4].comp = CP_ECHO; trk[3].step[4].carg = (uint8_t)(2u | CP_ECHO_EVEN);
        seq_more_at(UXR_TRACK, ID_SQ_TYPE);
        break;
    case S_SEQ_LOCK: {
        uint32_t r, i;
        roll_scene(S_ROLL_ACID);
        for (i = 0; i < NPAGES && PAGES[i].more != &UXM_SYN; i++)
            ;
        for (r = 0; r < UXM_SYN.n && UXM_SYN.rows[r].id != P_MORPH; r++)
            ;
        memset(uxm_at, 0, sizeof uxm_at);
        uxm_at[nav_key(SEC_EDIT) % NAV_KEYS].sel = (uint8_t)r;
        ui.plk_src = (uint8_t)(i + 1u);
        motion_set_lock(TSEL, ui.cursor, P_MORPH, 90);
        ui.entry_open = 1;
        fm1_in.notes = 1u << 10;
        ux_knob_touch(1);
        break;
    }
    /* (ACID FM 3c-1 task 3) SEQ > STEP EXTRAS: the acid line with chances, ratchets (x2 .. x4), conditions and micro timing (step 5
     * picked: 1ST OF 4, late; KNOB 3 just turned); a DRUM track's; PATTERN: 16 steps straight, 32 swung with a short gate */
    case S_SEQ_EXTRAS:
    case S_SEQ_EXTRAS_ECHO:                             /* (task 4: step 5 an ECHO ratchet, x6 FADE: its parts, its shape) */
    case S_SEQ_PATTERN:
    case S_SEQ_PATTERN_32: {
        static const uint8_t LINE[16] = {36, 0, 48, 43, 43, 0, 46, 48, 41, 0, 53, 48, 36, 0, 39, 43};
        static const uint8_t CHN[16] = {100, 0, 70, 100, 100, 0, 40, 100, 85, 0, 25, 100, 100, 0, 55, 100};
        track_t *t = TSEL;
        uint32_t i, n = s == S_SEQ_PATTERN_32 ? 32u : 16u;
        eng(ENGI_ANALOG); t->engine = t->eng_req;
        track_defaults_steps(t);
        t->p[P_SLEN] = (int16_t)n;
        for (i = 0; i < n; i++) {
            uint8_t nt = LINE[i % 16u];
            if (!nt)
                step_put(&t->step[i], ST_REST, 0, 0, &nt);
            else
                step_put(&t->step[i], ST_NOTE, i % 16u == 4u || i % 16u == 11u ? SF_ACCENT : i % 16u == 7u ? SF_SLIDE : 0u, 1, &nt);
            if (s == S_SEQ_EXTRAS || s == S_SEQ_EXTRAS_ECHO) step_set_chance(&t->step[i], CHN[i]);
        }
        if (s == S_SEQ_EXTRAS || s == S_SEQ_EXTRAS_ECHO) {
            step_set_ratchet(&t->step[3], 2); step_set_ratchet(&t->step[8], 3); step_set_ratchet(&t->step[12], 4);
            t->step[4].comp = CP_TRIG; t->step[4].carg = 5;    /* (1ST OF 4; EVERY 2ND) */
            t->step[10].comp = CP_TRIG; t->step[10].carg = 1;
            ST_MICRO_SET(&t->step[4], 12); ST_MICRO_SET(&t->step[14], -20);
        }
        if (s == S_SEQ_PATTERN_32) { t->p[P_SSWING] = 60; t->p[P_SGATE] = 40; }
        else { t->p[P_SSWING] = 0; t->p[P_SGATE] = 100; }
        song.playing = 1; t->seq_idx = s == S_SEQ_PATTERN_32 ? 21u : 8u;
        if (s == S_SEQ_EXTRAS_ECHO) { t->step[4].comp2 = CP_ECHO; t->step[4].carg2 = 4u; }   /* (x6 FADE) */
        nav_scene(SEC_SEQ, s == S_SEQ_PATTERN || s == S_SEQ_PATTERN_32 ? "PATTERN" : "STEP EXTRAS");
        cursor_set(4);
        if (s == S_SEQ_EXTRAS) ux_knob_touch(2);
        break;
    }
    case S_SEQ_EXTRAS_DRUM:
        drum(0);
        step_set_chance(&trk[3].step[2], 50); step_set_ratchet(&trk[3].step[6], 3);
        trk[3].step[4].comp = CP_TRIG; trk[3].step[4].carg = 11; ST_MICRO_SET(&trk[3].step[4], -8);   /* (NOT 1ST) */
        nav_scene(SEC_SEQ, "STEP EXTRAS");
        cursor_set(4);
        break;
    case S_COMP_SEL_DRUM:
        drum(0);
        trk[3].step[2].comp = CP_ECHO; trk[3].step[2].carg = 1;
        trk[3].step[10].comp = CP_ECHO; trk[3].step[10].carg = 0;
        trk[3].step[6].comp = CP_RAND; trk[3].step[6].carg = 1;
        ui.cursor = 10; ui.lane = 3;
        seq_more_at(UXR_TRACK, ID_SQ_TYPE);
        break;
    default: break;
    }
    if (s != S_FUGUE)
        memset(fg_set, 0, sizeof fg_set);
    if (s != S_PFLASH)                            /* (0.6.51) the page's big name: its own screen only (the clock stands */
        pf_on = 0;                                /* still here: it would never go) */
}
static void draw(int s)
{
    nscr = npend = nrules = 0;
    ntight = 0;
    tight[0] = 0;
    memset(host_screen, 0, sizeof host_screen);
    if (s == S_UX_THEME) {                        /* ux_theme.c: gradient title over a level-3 glow, the band, the top line */
        lcd_fill(0, 0, 240, 240, UX_BG);
        cv_begin(240, 150, UX_BG);
        ux_glow(0, 10, 240, 130, 3);
        ux_text_grad(40, 50, &UF_TTL, "ACID 303", swap16(cv_px[62u * cv_w + 112u]));   /* (the glow's colour under the text) */
        ux_band(124, 14);
        ux_topline();
        cv_blit(0, 0);
        lcd_sync();
        return;
    }
    if (s == S_UX_DEMO || s == S_UX_DEMO_WARN) {                         /* ux_widgets.c: every piece on one page (the mock-up's glow option) */
        static const ux_knob_t K[4] = {{"CUTOFF", "42", 42}, {"RESO", "110", 110}, {"ENV MOD", "88", 88}, {"DECAY", "31", 31}};
        step_t st[16];
        uint32_t i;
        memset(st, 0, sizeof st);
        for (i = 0; i < 16u; i++) {
            st[i].n = (i % 4u) != 3u; st[i].time = (i % 4u) == 3u ? ST_REST : ST_NOTE;
            if (i == 2u) st[i].flags = SF_SLIDE;
            if (i == 4u || i == 9u) st[i].flags = SF_ACCENT;
            if (i == 6u) { st[i].time = ST_TIE; st[i].n = 0; }
        }
        lcd_fill(0, 0, 240, 240, UX_BG);
        dc_frame(1);
        ux_header(3, "ACID 303", "128 BPM");
        ux_knobs(K, 0);
        ux_panel_begin(3);
        ux_strip(110, st, 16, 4, 7, UX_EMPTY, 0);
        ux_panel_end();
        if (s == S_UX_DEMO) ux_status("5 \xB7 G2 \xB7 ACC", UX_TEXT);
        else ux_status("NOT A SLIDE STEP", UX_ACCENT);
        ux_footer("EDIT \xB7 TONE", "2 / 4");
        lcd_sync();
        return;
    }
    if (s == S_CALIBRATION) {                     /* the blocking setup screen: its two drawing steps */
        setup_title();
        setup_show("TURN RIGHT", "ALGORITHM");
        return;
    }
    if (s == S_SPLASH || s == S_SPLASH_MID) {     /* the power-on splash (main.c, before the UI): the wordmark (ux_splash.c); the version
                                                   * shown FELUCCA_VERSION (run_tests.sh: src/felucca.c's). S_SPLASH: whole (spl_anim 0);
                                                   * S_SPLASH_MID: 450 ms in, the line three quarters across and "ACI" typed */
        static const char WANT[] = "ACID FM|" FELUCCA_VERSION "|BY NORFOLKHACKER";
        uint16_t bg;
        uint32_t x, y, wm = 0, wmr = 0, lbl = 0, ln_l = 0, ln_r = 0, ln_end = 0, row4 = 0;
        splash_seen[0] = 0;
        in_splash = 1;
        if (s == S_SPLASH) {
            draw_splash();
        } else {
            spl_anim = 1;
            draw_splash();                        /* (the first frame: bare) */
            splash_write(450u);
            spl_anim = 0;
        }
        in_splash = 0;
        bg = host_screen[0];
        for (x = 0; x < 240u; x++) {              /* the line: 3 px at y 96 (96 .. 98), the blend across */
            if (host_screen[96u * 240u + x] != bg && host_screen[98u * 240u + x] != bg) { if (x < 120u) ln_l++; else ln_r++; if (x >= 200u) ln_end++; }
            if (host_screen[99u * 240u + x] != bg) row4++;           /* (not a 4th row) */
        }
        if (row4) {
            if (rep) fprintf(rep, "FIND %-26s the splash's line is more than 3 px thick (%u pixels on row 99)\n", cur_name, row4);
            nfind++;
        }
        for (y = 104; y < 164u; y++)              /* the wordmark band: left half, right quarter */
            for (x = 0; x < 240u; x++) {
                uint32_t ink = host_screen[y * 240u + x] != bg;
                if (x < 120u) wm += ink;
                if (x >= 190u) wmr += ink;
            }
        for (y = 166; y < 200u; y++)
            for (x = 0; x < 240u; x++)
                lbl += host_screen[y * 240u + x] != bg;
        if (s == S_SPLASH) {
            if (ln_l < 110u || ln_r < 110u || wm < 300u || wmr < 100u || lbl < 100u) {
                if (rep) fprintf(rep, "FIND %-26s the wordmark splash: line %u/%u px, wordmark ink %u/%u, labels %u (too little)\n",
                                 cur_name, ln_l, ln_r, wm, wmr, lbl);
                nfind++;
            }
            if (strcmp(splash_seen, WANT)) {
                if (rep) fprintf(rep, "FIND %-26s the splash shows '%s', not '%s'\n", cur_name, splash_seen, WANT);
                nfind++;
            } else {
                static uint8_t said;
                if (!said++) printf("ui_render: the splash shows the version " FELUCCA_VERSION " (FELUCCA_VERSION) and its lines\n");
            }
        } else if (ln_l < 110u || ln_r < 40u || ln_end || wm < 100u || wmr || lbl) {   /* partly across, "ACI" only, no labels yet */
            if (rep) fprintf(rep, "FIND %-26s the mid-splash: line %u/%u px (%u past 200), wordmark ink %u/%u, labels %u\n", cur_name, ln_l, ln_r, ln_end, wm, wmr, lbl);
            nfind++;
        }
        return;
    }
    ui.force = 1;
    if (ui.menu == 1 && !ui.mlv && s != S_MENU_SLIDE) ui.mlv = 1;   /* (0.8.5: the lens at TAB, as it opens; S_MENU_SLIDE: MENU) */
    ui_draw();
}

/* every value of every column on every page of every engine: labels and values fit their column */
static uint32_t nsweep, nmotion;
/* #63: the page drawn again with MOTION driving each of its cards' track parameters (the motion icon beside
 * every label, the long ones too): the lint over it; the motion cleared again */
static void sweep_motion(void)
{
    uint32_t c, any = 0;
    if (ui.home)                                  /* (ACID FM 3a: HOME's knobs are four tracks' levels, no MOTION marks) */
        return;
    for (c = 0; c < 4u; c++) {
        int16_t *vp = 0;
        const param_desc_t *d = page_desc(cur_page(), c, &vp);
        uintptr_t id = vp ? ((uintptr_t)vp - (uintptr_t)TSEL->p) / sizeof *vp : P_COUNT;
        if (d && d->label && d->label[0] != '-' && id < P_COUNT && !motion_set_event(TSEL, c, (uint32_t)id, *vp))
            any = 1;
    }
    if (!any) return;
    draw(-1);
    for (c = 0; c < 4u; c++) nmotion += (card_mot >> c) & 1u;
    lint();
    memset(&motion, 0, sizeof motion);
}
static void sweep_columns(void)
{
    uint32_t e, i, c;
    char name[64];
    for (e = 0; e < NENGINES; e++) {
        if (!eng_ok(e))
            continue;                                    /* (DIGITAL without FELUCCA_FM4: no track has it) */
        for (i = 0; i < NPAGES; i++) {
            state();
            pal(UI_ACID_INDEX);
            eng(e);
            ui.home = 0; ui.page = (uint8_t)i; page_entered();
            if (!page_visible(i)) continue;
            snprintf(name, sizeof name, "%s%s/%s", large_on ? "LARGE " : "", ENGINES[e]->name, PAGES[i].title);
            cur_name = name;
            for (c = 0; c < 4u; c++) {
                int16_t *vp;
                const param_desc_t *d = PAGES[i].scope == SC_GLOBAL || PAGES[i].scope == SC_TRACK || PAGES[i].scope == SC_ENGINE
                                        ? page_desc(cur_page(), c, &vp) : 0;
                int32_t v, v0;
                if (!d || !d->label || d->label[0] == '-' || PAGES[i].graph == GR_MOD) continue;
                v0 = *vp;
                for (v = d->min; v <= d->max; v++) {
                    *vp = (int16_t)v;
                    nscr = npend = 0;
                    ui.force = 1;
                    draw_columns();
                    lint();
                    nsweep++;
                    if (v - d->min > 300) v = d->max - 1;      /* wide ranges: the ends */
                }
                *vp = (int16_t)v0;
            }
            if (PAGES[i].scope == SC_STEP && PAGES[i].id[0] == 4u && !e) {   /* (0.9.1) NOTE's MICRO: every 64th, the card and
                                                         * the big step (its chip, the notes moved) */
                int32_t v;
                step_t *st = &TSEL->step[ui.cursor];
                step_t keep = *st;
                st->time = ST_NOTE; st->n = 2; st->note[0] = 60; st->note[1] = 64; st->vel = 100;
                for (v = MICRO_MIN; v <= MICRO_MAX; v++) {
                    ST_MICRO_SET(st, v);
                    nscr = npend = 0;
                    ui.force = 1;
                    draw(-1);
                    lint();
                    nsweep++;
                }
                *st = keep;
            }
            draw(-1);                                    /* the whole page */
            lint();
            sweep_motion();
        }
        state(); pal(UI_ACID_INDEX); eng(e); go_home();   /* HOME's four knobs of this engine */
        snprintf(name, sizeof name, "%s%s/HOME", large_on ? "LARGE " : "", ENGINES[e]->name);
        cur_name = name;
        draw(-1);
        lint();
        sweep_motion();
    }
    for (e = 0; e < 4u; e++) {                           /* the MOD page: every source and destination */
        int32_t v;
        state(); pal(UI_ACID_INDEX); go_title("MOD");
        cur_name = large_on ? "LARGE MOD sweep" : "MOD sweep";
        for (v = 0; v < MD_N; v++) {
            TSEL->p[P_M1SRC] = (int16_t)(v % MS_N); TSEL->p[P_M1DST] = (int16_t)v; TSEL->p[P_M1AMT] = (int16_t)(e * 40 - 64);
            mod_ui_slot = (uint8_t)(v & 3u);
            draw(-1);
            lint();
        }
    }
}

/* the rolling digits (ui_draw.c roll_*): PHRASES (3c-1 task 7: GLO > GLOBAL gone, the default build's last JIANT card with a
 * number: the pattern), the tempo and KNOB 1 turned together, BPM 129 -> 130 in the header and the pattern 09 -> 10 on a card,
 * then back. Every frame is linted and (GREY, MONO) checked for gray; with a directory,
 * filmstrips of both (each frame side by side, x4: the static frame before, then the roll's frames) as
 * DIR/<PALETTE>_bpm_roll.ppm and DIR/<PALETTE>_card_roll.ppm, the up roll above the down roll. */
#define FS_N (1u + ROLL_FRAMES)                   /* frames per filmstrip row */
#define FS_Z 4u                                   /* zoom */
#define FS_GAP 2u                                 /* px between frames, before the zoom */
static void roll_turns(int32_t s)                 /* one UI frame with the tempo and KNOB 1 moved by s */
{
    song.g[G_BPM] = (int16_t)(song.g[G_BPM] + s);  /* (0.6.67: SELECT alone scrolls the pages; the tempo set here) */
    host_enc[panel.enc[EN_K1]] += s * panel.dir[EN_K1];
    host_ticks += 16000u; fm1_ms += 16u;
    ui_input();
    ui_draw();
    host_ticks += 200000u;
}
static void roll_film_put(uint8_t *img, uint32_t iw, uint32_t row, uint32_t k, int32_t x0, int32_t y0, uint32_t w, uint32_t h)
{
    uint32_t x, y;
    for (y = 0; y < h * FS_Z; y++)
        for (x = 0; x < w * FS_Z; x++) {
            uint16_t c = swap16(host_screen[(uint32_t)(y0 + (int32_t)(y / FS_Z)) * 240u + (uint32_t)x0 + x / FS_Z]);
            uint8_t *p = img + (((row * (h + FS_GAP) + FS_GAP) * FS_Z + y) * iw + (k * (w + FS_GAP) + FS_GAP) * FS_Z + x) * 3u;
            p[0] = (uint8_t)((c >> 11) * 255u / 31u);
            p[1] = (uint8_t)(((c >> 5) & 63u) * 255u / 63u);
            p[2] = (uint8_t)((c & 31u) * 255u / 31u);
        }
}
static void roll_film_save(const char *dir, const char *pal, const char *name, const uint8_t *img, uint32_t iw, uint32_t ih)
{
    char path[512];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s_%s.ppm", dir, pal, name);
    f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", path); return; }
    fprintf(f, "P6\n%u %u\n255\n", iw, ih);
    fwrite(img, 1, (size_t)iw * ih * 3u, f);
    fclose(f);
}
static void roll_frames(const char *dir)
{
    enum { HX = 56, HW = 48, HH = H_HEAD };       /* the header crop: the tempo icon and the BPM strip */
    static uint8_t bimg[(FS_N * (HW + FS_GAP) + FS_GAP) * FS_Z * (2u * (HH + FS_GAP) + FS_GAP) * FS_Z * 3u];
    static uint8_t cimg[(FS_N * (COL_W + FS_GAP) + FS_GAP) * FS_Z * (2u * (COL_H + FS_GAP) + FS_GAP) * FS_Z * 3u];
    const uint32_t biw = (FS_N * (HW + FS_GAP) + FS_GAP) * FS_Z, bih = (2u * (HH + FS_GAP) + FS_GAP) * FS_Z;
    const uint32_t ciw = (FS_N * (COL_W + FS_GAP) + FS_GAP) * FS_Z, cih = (2u * (COL_H + FS_GAP) + FS_GAP) * FS_Z;
    uint32_t p, row, k;
    for (p = 0; p < NPALETTES; p++) {
        char name[64];
        memset(bimg, 90, sizeof bimg);
        memset(cimg, 90, sizeof cimg);
        state(); song.playing = 0; pal(p);
        song.g[G_BPM] = 129; ui.ppick = 8;
        go_title("PHRASES");
        snprintf(name, sizeof name, "%s/roll", UI_PALETTES[p].name);
        cur_name = name;
        draw(-1);
        for (k = 0; k < 8u; k++) { ui.hot_t = 0; ui.bpm_t = 0; ui_draw(); }   /* settled, nothing hot */
        lint();
        for (row = 0; row < 2u; row++) {
            roll_film_put(bimg, biw, row, 0, HX, 0, HW, HH);
            roll_film_put(cimg, ciw, row, 0, CARD_X(0), Y_LABEL, COL_W, COL_H);
            for (k = 1; k < FS_N; k++) {
                if (k == 1u) roll_turns(row ? -1 : 1);
                else ui_draw();
                lint();
                roll_film_put(bimg, biw, row, k, HX, 0, HW, HH);
                roll_film_put(cimg, ciw, row, k, CARD_X(0), Y_LABEL, COL_W, COL_H);
            }
            if (ui.roll[0].from[0] || ui.roll[ROLL_BPM].from[0] || song.g[G_BPM] != (row ? 129 : 130) ||
                ui.ppick != (row ? 8u : 9u)) {
                fprintf(rep, "ROLL %s: not over after %u frames (BPM %d pattern %u)\n", name, ROLL_FRAMES, song.g[G_BPM], ui.ppick);
                nfind++;
            }
            for (k = 0; k < 8u; k++) ui_draw();
        }
        if (dir && p == UI_ACID_INDEX) {
            roll_film_save(dir, UI_PALETTES[p].name, "bpm_roll", bimg, biw, bih);
            roll_film_save(dir, UI_PALETTES[p].name, "card_roll", cimg, ciw, cih);
        }
    }
}
/* the host cost of a roll frame: the header and the cards (draw_head, draw_columns) while both strips roll,
 * against the same calls on an idle frame (nothing drawn) and a full redraw of the four cards and the header */
static void roll_cost(void)
{
    enum { N = 400 };
    uint32_t i, k;
    double t_roll = 0, t_full = 0;
    uint64_t px_roll = 0, bl_roll = 0, nroll = 0, px_full = 0, bl_full = 0;
    for (i = 0; i < N; i++) {
        clock_t c0;
        state(); song.playing = 0; pal(1);
        song.g[G_BPM] = 129; ui.ppick = 8;
        go_title("PHRASES");
        draw(-1);
        for (k = 0; k < 8u; k++) { ui.hot_t = 0; ui.bpm_t = 0; ui_draw(); }
        px_visited = 0; blit_px = 0;
        c0 = clock();
        ui.force = 1; ui.frame++; draw_head(); draw_columns(); ui.force = 0;
        t_full += (double)(clock() - c0); px_full += px_visited; bl_full += blit_px;
        nscr = npend = 0;
        roll_turns(i & 1u ? -1 : 1);                /* the change: the card and the header redrawn whole */
        for (k = 2; k < ROLL_FRAMES; k++) {         /* the strip-only frames */
            px_visited = 0; blit_px = 0;
            c0 = clock();
            ui.frame++; draw_head(); draw_columns();
            t_roll += (double)(clock() - c0); px_roll += px_visited; bl_roll += blit_px; nroll++;
            nscr = npend = 0;
        }
    }
    fprintf(rep, "  roll frame (header BPM 129->130 and card 09->10, strips only): %.2f us, %llu glyph px, %llu px blitted;"
            " header + 4 cards redrawn whole: %.2f us, %llu glyph px, %llu px blitted\n",
            t_roll / CLOCKS_PER_SEC / (double)nroll * 1e6, (unsigned long long)(px_roll / nroll), (unsigned long long)(bl_roll / nroll),
            t_full / CLOCKS_PER_SEC / N * 1e6, (unsigned long long)(px_full / N), (unsigned long long)(bl_full / N));
}

/* the alignment check over every value its places can show, beyond the screens above (FLAT and LINE, GREY): the
 * track cushions 1..4 at 12 16 24 px; every icon in a card's icon slot; the dialogs of every kind; every menu row with
 * each of its values; NAME's every key, letter and cycle in ABC and 123, the field holding every character; the
 * piano roll's C labels C-1 .. C9; the drum lanes of every kit; the mixer's LEVEL, PAN and REV over their ranges */
static void align_sweeps(void)
{
    uint32_t st, i, k, nf = nfind;
    FILE *r0 = rep, *nul = fopen("/dev/null", "w");
    int32_t v;
    if (nul) rep = nul;                              /* (not lint scenes: their draws pile up) */
    for (st = 0; st < 1u; st++) {                    /* (ACID FM 0.3: one style; until then FLAT and LINE) */
        state(); pal(UI_ACID_INDEX);
        cur_name = "sweep: cushions";
        for (k = 0; k < NTRK; k++)
            for (i = 0; i < 3u; i++) {
                cv_begin(40, 40, T_SURF);
                cv_icon_on(4, 4, i == 0u ? 12u : i == 1u ? 16u : 24u, ICON_TRK + k, T_ACCENT, T_SURF);
            }
        cur_name = st ? "sweep LINE: card icons" : "sweep FLAT: card icons";
        for (i = 0; i < ICON_COUNT; i++) {
            ui.force = 1;
            draw_column(i & 3u, "LVL", "64", "", T_THEME, 500, i);
            ui.col[i & 3u][0] = 0;
        }
        cur_name = st ? "sweep LINE: LARGE card icons" : "sweep FLAT: LARGE card icons";
        ui_prefs = PREF_LARGE; ui.home = 1;             /* (tall: HOME) */
        for (i = 0; i < ICON_COUNT; i++) {
            ui.force = 1; ui.hot_col = (uint8_t)(i & 3u); ui.hot_t = (uint8_t)(i & 4u);
            draw_column(i & 3u, "LVL", i & 8u ? "OFF" : "64", i & 16u ? "%" : "", T_THEME, 500, i);
            ui.col[i & 3u][0] = 0;
        }
        ui.layer = LAYER_FX;                            /* (the labels in M: a layer's cards) */
        for (i = 0; i < ICON_COUNT; i++) {
            ui.force = 1; ui.hot_col = (uint8_t)(i & 3u); ui.hot_t = (uint8_t)(i & 4u);
            draw_column(i & 3u, "LVL", "64", "", T_THEME, 500, i);
            ui.col[i & 3u][0] = 0;
        }
        ui.layer = 0; ui.hot_t = 0; ui_prefs = 0;
        cur_name = st ? "sweep LINE: full-screen dialogs" : "sweep FLAT: full-screen dialogs";   /* (3c-2 tasks 1, 1b: ux_dialog.c,
                                                         * the dice too; ui_draw.c's draw_confirm gone) */
        for (i = CF_CLEAR_SEQ; i <= CF_LOAD_PROJ; i++)
            for (k = 0; k < 8u && ux_dialog_owns(i); k++) {
                ui.confirm = (uint8_t)i; ui.confirm_trk = (uint8_t)k; ui.force = 1;
                ux_dialog_draw();
            }
        ui.confirm = 0;
        cur_name = st ? "sweep LINE: menu rows" : "sweep FLAT: menu rows";
        ui.menu = 1;
        for (i = 0; i < MI_COUNT; i++)
            for (k = 0; k < 4u; k++) {
                ui.menu_sel = (uint8_t)i; ui_prefs = k & 1u ? 0xFFu : 0u; settings_hold = (uint8_t)k; settings.lowcut = (uint8_t)(k % 3u);
                ui_rec_prefs = (uint8_t)((k % 3u) | (k % 3u) << 2 | ((k + 1u) % 3u) << 4);   /* (CLICK, LEVEL, COUNT-IN: each value) */
                settings_leds = (uint8_t)(k % LEDS_COUNT); ui.force = 1;   /* (every LEDS name) */
                draw_menu();
            }
        cur_name = st ? "sweep LINE: menu levels" : "sweep FLAT: menu levels";
        for (i = 0; i < MI_COUNT; i++)                  /* (0.8.5) every row on each level of the lens, S and LARGE */
            for (k = 0; k < 6u; k++) {
                ui.menu_sel = (uint8_t)i; ui_prefs = k & 1u ? PREF_LARGE : 0u; ui.mlv = (uint8_t)(k / 2u); ui.force = 1;
                draw_menu();
            }
        ui.mlv = 1;
        ui.menu = 0; ui_prefs = 0; ui_rec_prefs = 0; settings_hold = 0; settings.lowcut = 0; settings_leds = 0;
        cur_name = st ? "sweep LINE: REC layer values" : "sweep FLAT: REC layer values";
        ui.home = 1; ui.layer = LAYER_REC;              /* (the map and the cards: every value of the three, each track) */
        for (k = 0; k < 27u * 2u; k++) {
            rp_put(RP_CLICK, k % 3u); rp_put(RP_COUNTIN, k / 3u % 3u); rp_put(RP_LEVEL, k / 9u % 3u);
            song.sel = (uint8_t)(k % NTRK); ui_prefs = k >= 27u ? PREF_LARGE : 0u;
            ui.force = 1; ux_layer_draw(); ui.force = 0;   /* (3c-2 task 4: ui_layer.c draw_layer gone) */
        }
        ui.layer = 0; song.sel = 0; ui_prefs = 0; ui_rec_prefs = 0;
        cur_name = st ? "sweep LINE: count-in" : "sweep FLAT: count-in";
        for (i = 1; i <= 2u; i++)                       /* 1 and 2 bars, every beat */
            for (k = 4u * i; k >= 1u; k--) {
                cin_total = (uint8_t)(4u * i); cin_left = (uint8_t)k; ui.force = 1;
                draw_countin();
            }
        cin_total = cin_left = 0;
        state(); pal(UI_ACID_INDEX);
        cur_name = st ? "sweep LINE: NAME" : "sweep FLAT: NAME";
        song.playing = 0; go_page(GR_SVSND); name_open(NK_USER_SAVE, 6);
        for (k = 0; k < 2u; k++) {                       /* ABC, 123: every key cycling, every letter typed next */
            nm.num = (uint8_t)k;
            for (i = 0; i <= 16u; i++) {
                uint32_t n = i ? str_len(nm_group(i - 1u)) : 1u, j;
                for (j = 0; j < n; j++) {
                    nm.key = (uint8_t)i; nm.tap = (uint8_t)j;
                    nm_draw_panel();
                }
            }
        }
        nm.key = 0;
        for (i = 0; i < sizeof NM_SET - 1u; i += NM_LEN) {   /* the field: every character */
            for (k = 0; k < NM_LEN; k++) nm.s[k] = NM_SET[(i + k) % (sizeof NM_SET - 1u)];
            nm.s[NM_LEN] = 0; nm.len = NM_LEN; nm.cur = 0;
            nm_draw_field();
        }
        name_close();
        /* (ACID FM 3c-1 task 4) JIANT's piano roll and DRUM grid sweeps gone with them (ux_seq.c's roll, grid: the SEQ scenes) */
        state(); pal(UI_ACID_INDEX);
        /* (ACID FM 3c-1 task 7) JIANT's mixer sweep (draw_tracks: LEVEL PAN REV, LARGE's strip) gone with the MIXER: GLO > MIX the
         * ux pages' (scenes glo_mix, mixer*) */

    }
    state();
    nscr = npend = nrules = 0;
    nfind = nf;
    rep = r0;
    if (nul) fclose(nul);
}

/* the keycaps' labels in their pills (tools/gen_aa_keycaps.py), from the data: the label's ink (nibbles 6..15) in the
 * pill (KC_H rows, its width) */
static void al_keycaps(void)
{
    uint32_t id;
    cur_name = "keycap data";
    for (id = 0; id < KC_COUNT; id++) {
        const kc_t *k = &KC[id];
        int32_t x, y, x0 = 999, y0 = 999, x1 = -1, y1 = -1, w = k->w;
        uint32_t i = 0;
        for (y = 0; y < KC_H; y++)
            for (x = 0; x < w; x++, i++)
                if (((KC_REF_DATA[KC_REF_OFF[id] + (i >> 1)] >> ((i & 1u) ? 0 : 4)) & 15u) >= 6u) {   /* (0.9.0: the nibbles) */
                    if (x < x0) x0 = x;
                    if (y < y0) y0 = y;
                    if (x + 1 > x1) x1 = x + 1;
                    if (y + 1 > y1) y1 = y + 1;
                }
        al_record("keycap label in its pill", (x0 + x1) - w, (y0 + y1) - KC_H, k->label, 0, 0, w, KC_H, x0, y0, x1, y1);
        if (al_rep) fprintf(al_rep, "keycap %-8s w %2d  ink x %2d..%2d y %d..%d  across %+.1f up/down %+.1f\n", k->label, w, x0, x1, y0, y1,
                            ((x0 + x1) - w) / 2.0, ((y0 + y1) - KC_H) / 2.0);
    }
}
static void al_table(void)
{
    uint32_t i;
    if (!al_rep) return;
    fprintf(al_rep, "\n%-30s %8s %5s %5s %5s %6s %6s\n", "place", "measured", "1px+", "-0.5", "+0.5", "across", "up/dn");
    for (i = 0; i < al_nst; i++) {
        const alstat_t *a = &al_st[i];
        if (!strcmp(a->tag, "self-test")) continue;
        fprintf(al_rep, "%-30s %8u %5u %5u %5u %6.1f %6.1f\n", a->tag, a->n, a->nfail, a->half_lo, a->half_hi, a->wh2 / 2.0,
                a->wv2 / 2.0);
        if (a->ex_h[0]) fprintf(al_rep, "    across  worst %s\n", a->ex_h);
        if (a->ex_v[0]) fprintf(al_rep, "    up/down worst %s\n", a->ex_v);
    }
    fprintf(al_rep, "\n%u measurements, %u places, %u off by 1 px or more; %u declared places drawn without their items\n",
            al_count, al_nst, al_fail, al_lost);
}

int main(int argc, char **argv)
{
    const char *out = argc > 1 ? argv[1] : "build/ui_new";
    char path[600];
    uint32_t p, s;
    static const char *const SHOW[] = {"ACID"};    /* (ACID FM 0.3: the one palette) */
    uint32_t nlow = 0, nhome = 0;
    snprintf(path, sizeof path, "%s/report.txt", out);
    rep = fopen(path, "w");
    if (!rep) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    {
        char ap[600];
        snprintf(ap, sizeof ap, "%s/align.txt", out);
        al_rep = fopen(ap, "w");
        if (al_rep) fprintf(al_rep, "alignment: the ink of each centred text / icon / keycap against its box (gfx.c GFX_HOOK_ALIGN);"
                            " offsets in px, + right / down\n\n");
    }
    {
        char ap[600];
        snprintf(ap, sizeof ap, "%s/text_audit.tsv", out);
        audf = fopen(ap, "w");
    }
    if (audf) fprintf(audf, "screen\ttexts\ttext_px\ticons\ticon_px\tkeycaps\tkeycap_px\tellipsised\ttight\twords\ttight_list\n");
    {   /* the lint itself: an overlap, a value too wide for its column, a text past its canvas, one past its cell */
        state(); pal(0); cur_name = "self-test";
        nscr = npend = 0;
        cv_begin(COL_W, COL_H, T_BG);
        cv_text(0, 0, &AF_S, "AAAA", T_TEXT);
        cv_text(10, 4, &AF_S, "BBBB", T_TEXT);
        cv_text_fit(0, 15, &AF_M, "WWWWWWWW", T_THEME, T_BG, COL_W);
        cv_text(40, 30, &AF_M, "CUT", T_THEME);
        cv_rrect(2, 31, 20, 13, 3, T_SURF, T_BG);          /* a cell narrower than its word */
        cv_text(4, 31, &AF_S, "SPILL", T_TEXT);
        cv_blit(4, Y_LABEL);
        lint();
        nrules = 0;                                         /* a word on a divider (until ACID FM 0.3 STYLE LINE's: none now) */
        lcd_fill(0, 120, 240, 1, T_RULE);
        GFX_HOOK_RULE(0, 120, 240, 1);
        cv_begin(40, 16, T_BG);
        cv_text(2, 0, &AF_S, "RULE", T_TEXT);
        cv_blit(8, 112);
        lint();
        nrules = 0;
        {   /* the alignment check: a word 2 px right of its box's centre, one centred */
            uint32_t f0 = al_fail;
            cv_begin(60, 20, T_BG);
            GFX_HOOK_ALIGN(0, 0, 60, 20, AL_HV, "self-test");
            cv_text_in(4, 0 + CAP_IN(S, 20), 60, &AF_S, "OFF", T_TEXT, T_BG);
            GFX_HOOK_ALIGN(0, 0, 60, 20, AL_HV, "self-test");
            cv_text_in(0, 0 + CAP_IN(S, 20), 60, &AF_S, "MID", T_TEXT, T_BG);
            if (al_fail - f0 != 1u) { fprintf(stderr, "ui_render: the alignment check missed its self-test (%u)\n", al_fail - f0); return 1; }
            al_fail = f0;
            al_count -= 2u;
            npend = 0;
        }
        if (nfind < 5u || !nspill) { fprintf(stderr, "ui_render: the lint missed its self-test (%u)\n", nfind); return 1; }
        fprintf(rep, "(self-test: %u findings above are expected)\n\n", nfind);
        nfind = 0;
    }
    for (p = 0; p < NPALETTES; p++)
        for (s = 0; s < S_COUNT; s++) {
            char name[64];
            uint32_t k;
            if (!FELUCCA_FM4 && (s == S_OP_ENV || (s >= S_ALG1 && s <= S_OP_LEVEL)))
                continue;                           /* (DIGITAL's own screens: FELUCCA_FM4=1 only) */
            snprintf(name, sizeof name, "%s/%s", UI_PALETTES[p].name, S_NAME[s]);
            cur_name = name;
            setup((int)s);
            pal(p);
            if (p == UI_ACID_INDEX) aud = audf;
            draw((int)s);
            nlow += (uint32_t)lz_low();                 /* (the one skin, AIR's: the knob row on top of the lens pages) */
            nhome += lz_fam() == LZ_HOME && !gen_on() && !dlg_on() && !name_on();   /* (ACID FM 3a: HOME, ux_home.c's strips) */
            lint();
            if (p == UI_ACID_INDEX) { audit_scene(S_NAME[s]); aud = 0; }
            for (k = 0; k < sizeof SHOW / sizeof SHOW[0]; k++)
                if (!strcmp(UI_PALETTES[p].name, SHOW[k])) write_ppm(out, SHOW[k], S_NAME[s]);
        }
    if (nlow < 250u || nhome < 3u) { fprintf(stderr, "ui_render: the one skin drew %u screens low, %u HOMEs only\n", nlow, nhome); return 1; }
    fprintf(rep, "the one skin (AIR's): %u screens with the knob row on top, %u HOMEs (ux_home.c)\n", nlow, nhome);
    {   /* MENU > LARGE (ui.c large_kind): every screen, linted; as OUTDIR/ppm/LARGE_ACID_<screen>.ppm (ui_render.py:
         * sheet_LARGE_ACID.png); the tall cards' value faces counted (lg_hook) (until ACID FM 0.3 in FLAT and LINE, every
         * palette) */
        static const char *const ST_N[1] = {"LARGE"};
        uint32_t st, n0 = nfind, ntall = 0;
        large_on = 1;
        for (st = 0; st < 1u; st++)
            for (p = 0; p < NPALETTES; p++)
                for (s = 0; s < S_COUNT; s++) {
                    char name[64], tag[24];
                    if (!FELUCCA_FM4 && (s == S_OP_ENV || (s >= S_ALG1 && s <= S_OP_LEVEL)))
                        continue;
                    snprintf(tag, sizeof tag, "%s_%s", ST_N[st], UI_PALETTES[p].name);
                    snprintf(name, sizeof name, "%s/%s", tag, S_NAME[s]);
                    cur_name = name;
                    setup((int)s);
                    ui_prefs |= PREF_LARGE;
                    pal(p);
                    draw((int)s);
                    ntall += !ui.menu && !ui.confirm && !name_on() && large_kind() == LK_TALL;
                    lint();
                    write_ppm(out, tag, S_NAME[s]);
                }
        sweep_columns();                                /* every value of every column, tall */
        large_on = 0;
        if (ntall < 100u) { fprintf(stderr, "ui_render: LARGE drew %u screens with tall cards only\n", ntall); return 1; }
        fprintf(rep, "MENU > LARGE (the page / value sweep too): %u lint findings; %u screens with tall"
                " cards\n", nfind - n0, ntall);
        fprintf(rep, "  tall card values: %llu in L, %llu in M (a glyph not in L: \"%s\"), %llu in M (L too wide), %llu in S\n",
                (unsigned long long)lg_n[0], (unsigned long long)lg_n[1], lg_miss, (unsigned long long)lg_n[2],
                (unsigned long long)lg_n[3]);
        fprintf(rep, "  too wide for L, e.g.:");
        for (s = 0; s < lg_nwide; s++) fprintf(rep, " '%s'", lg_wide[s]);
        fprintf(rep, "\n  in S, e.g.:");
        for (s = 0; s < lg_nsmall; s++) fprintf(rep, " '%s'", lg_small[s]);
        fprintf(rep, "\n");
    }
    {   /* every FM6 chart (the lint above, fm6_hook, runs on each) */
        uint32_t a, c0 = fmp_charts;
        for (a = 1; a <= 32u; a++) {
            char name[32];
            snprintf(name, sizeof name, "FM6 ALG %u", a);
            cur_name = name;
            state(); pal(UI_ACID_INDEX); eng(ENGI_FM6); TSEL->engine = TSEL->eng_req; TSEL->p[P_E0] = (int16_t)a;
            nav_scene(SEC_EDIT, "OPERATOR");            /* (0.8.5; 3b task 5: the ux page) */
            draw(-1);
            lint();
        }
        if (fmp_charts - c0 != 32u) { fprintf(stderr, "ui_render: %u FM6 charts drawn of 32\n", fmp_charts - c0); return 1; }
    }
    {   /* (0.9.6) FM6's operator pages keep every number they showed: the operator and its frequency (OPERATOR), OP ENV's
         * operator and stage, the four knobs (3b task 5: the ux pages; OPERATOR 2's three MORE's rows) */
        static const struct { int s; const char *w[6]; } FT[] = {
            {S_EDIT_FM6_2, {"OP 3 x", "OP", "RATIO", "FINE", "LEVEL", "OPERATOR"}},
            {S_EDIT_FM6_3, {"OP 1 \xB7 DEC", "STAGE", "RATE", "LEVEL", "DEC", "OP ENV"}},
            {S_EDIT_FM6_MORE, {"FREQ MODE", "OP DETUNE", "MOD TIME", "OP 3", "MOD VEL", "MORE"}}};
        uint32_t f, w, i, n0 = nfind;
        for (f = 0; f < NELEM(FT); f++) {
            cur_name = S_NAME[FT[f].s];
            setup(FT[f].s); pal(UI_ACID_INDEX);
            draw(FT[f].s);
            for (w = 0; w < 6u; w++) {
                for (i = 0; i < nscr && !strstr(scr[i].s, FT[f].w[w]); i++)
                    ;
                if (i == nscr) {
                    fprintf(rep, "FIND %-26s FM6's page lost '%s'\n", cur_name, FT[f].w[w]);
                    nfind++;
                }
            }
        }
        if (nfind != n0) { fprintf(stderr, "ui_render: FM6's operator pages lost numbers (report.txt)\n"); return 1; }
    }
    sweep_columns();
    roll_frames(argc > 2 ? argv[2] : 0);
    al_off = 1;                                     /* (the alignment check is not timed) */
    /* draw cost: a full redraw of a screen (all strips), and HOME frame by frame (the scope, every other frame) */
    {
        static const int COST[] = {S_HOME, S_SAVE_MORE, S_STEP, S_ROLL_CHORDS, S_DRUM, S_MIXER, S_MENU, S_ABOUT_CREDITS, S_DLG_OVR_PROJ,
                                   S_PATTERN, S_COMP, S_LENTE_PAT, S_LENTE_BAR, S_LENTE_STEP, S_LENTE_STEP_DRUM,
                                   S_SAVE_PROJECT, S_SAVE_SONG, S_SAVE_SOUND, S_FXL_SENDS, S_FXL_BUSES, S_FXL_REVERB,
                                   S_HOME_VISUAL, S_EDIT_ANALOG, S_ED_FILTER, S_EDIT_ANALOG_MORE, S_ENV_TRACKS, S_MOD, S_KEY_CHORD,
                                   S_SYSTEM, S_MENU_ROW, S_LY_FX_KNOBS, S_DLG_DICE_SOUND, S_EDIT_SAMPLE, S_EDIT_VOICE_1,
                                   S_EDIT_DRUM_1, S_EDIT_DRUM_2,
                                   S_EDIT_ANALOG_1, S_EDIT_FM6_1, S_EDIT_FM6_2, S_EDIT_FM6_3, S_EDIT_FM6_MORE, S_EDIT_NOISE_1,
                                   S_EDIT_PHASE_1, S_EDIT_LOFI_1, S_EDIT_LOFI_BYTE_1, S_EDIT_PHASE_3, S_EDIT_DRUM_MORE,
                                   S_EDIT_VOICE_MORE, S_LFO, S_LFO_DEST};   /* (3b task 4: the synths' EDIT the ux pages, their
                                                         * creatures gone; task 7: LFO's wave, DEST) */   /* (0.9.6: the six, EDIT, HOME, the whole panel) */   /* (0.9.4: the creatures against a being) */   /* (0.8.5: the lenses everywhere) */   /* (0.8.3:
                                                                         * LENTE's three levels) */
        enum { N = 200 };
        uint32_t i, k;
        fprintf(rep, "\ndraw cost on the host (cc -O1; only the ratios mean anything on the device):\n");
        fprintf(rep, "  screen            full redraw us   texts+icons   glyph px read\n");
        for (k = 0; k < sizeof COST / sizeof COST[0]; k++) {
            clock_t c0;
            uint64_t px, nt;
            setup(COST[k]); pal(1);
            draw(COST[k]);
            px_visited = n_text = 0;
            c0 = clock();
            for (i = 0; i < N; i++) draw(COST[k]);
            px = px_visited / N; nt = n_text / N;
            fprintf(rep, "  %-16s %15.1f %13llu %15llu\n", S_NAME[COST[k]], (double)(clock() - c0) / CLOCKS_PER_SEC / N * 1e6,
                    (unsigned long long)nt, (unsigned long long)px);
        }
        {
            clock_t c0;
            setup(S_HOME); pal(1); draw(S_HOME);
            px_visited = n_text = 0;
            c0 = clock();
            for (i = 0; i < N * 4; i++) { ui_draw(); nscr = npend = 0; }
            fprintf(rep, "  HOME, a frame without force (lazy strips): %.1f us, %llu texts\n",
                    (double)(clock() - c0) / CLOCKS_PER_SEC / (N * 4) * 1e6, (unsigned long long)(n_text / (N * 4)));
        }
        {   /* the STEP page's piano roll: its panel alone redrawn, and a frame that changes nothing (the signature only) */
            static const int G[] = {S_ROLL_CHORDS, S_ROLL_PLAYING, S_DRUM, S_PATTERN, S_COMP, S_LENTE_PAT, S_LENTE_STEP,
                                    S_SAVE_SOUND, S_FXL_BUSES, S_FXL_REVERB, S_FXL_DELAY, S_EDIT_ANALOG,
                                    S_ENV_TRACKS, S_MOD, S_SYSTEM, S_EDIT_VOICE_1, S_EDIT_NOISE_1, S_EDIT_DRUM_2, S_EDIT_FM6_2};   /* (ACID FM 0.2:
                                                         * NOISE, was ANIMAL; 3b task 5: DRUM-X's and FM6's ux pages; 3c-2 task 4: SAVE's LIBRARY, PROJECT
                                                         * a ux page, no JIANT panel to time) */
            for (k = 0; k < sizeof G / sizeof G[0]; k++) {
                clock_t c0;
                double tg, ti;
                setup(G[k]); pal(1); draw(G[k]);
                c0 = clock();
                for (i = 0; i < N * 4; i++) { ui.force = 1; draw_graph(); ui.force = 0; nscr = npend = 0; }
                tg = (double)(clock() - c0) / CLOCKS_PER_SEC / (N * 4) * 1e6;
                song.playing = 0;                        /* (no playhead moving) */
                ui_draw(); ui_draw(); nscr = npend = 0;
                c0 = clock();
                for (i = 0; i < N * 4; i++) { ui.frame++; draw_graph(); nscr = npend = 0; }
                ti = (double)(clock() - c0) / CLOCKS_PER_SEC / (N * 4) * 1e6;
                fprintf(rep, "  %s: the panel redrawn %.1f us; an unchanged frame (signature only) %.2f us\n", S_NAME[G[k]], tg, ti);
            }
        }
        roll_cost();
        {   /* (0.8.9) SKIN: HOME, EDIT, SEQ's BAR, FX's BUSES redrawn whole (us, texts, LCD px sent), and HOME frame by frame
             * (HUD's HOME, the one skin's: every strip each frame, on the beings) */
            static const int SC[] = {S_HOME, S_EDIT_ANALOG, S_LENTE_BAR, S_FXL_BUSES, S_EDIT_VOICE_1, S_EDIT_PHASE_1, S_EDIT_DRUM_2};   /* (ACID FM 0.2: PHASE, was ANIMAL;
                                                         * 3a: the kit on EDIT, HOME's BEING gone) */
            uint32_t sk;
            fprintf(rep, "  SKIN (full redraw us / texts / LCD px; HOME a frame without force):\n");
            for (sk = 0; sk < 1u; sk++) {               /* (ACID FM 0.3: the one skin; until then COLORS HUD AIR) */
                fprintf(rep, "  %-7s", "ACID");
                for (k = 0; k < sizeof SC / sizeof SC[0]; k++) {
                    clock_t c0;
                    setup(SC[k]); pal(UI_ACID_INDEX);
                    draw(SC[k]);
                    px_visited = n_text = blit_px = 0;
                    c0 = clock();
                    for (i = 0; i < N; i++) draw(SC[k]);
                    fprintf(rep, "  %s %.1f / %llu / %llu", S_NAME[SC[k]], (double)(clock() - c0) / CLOCKS_PER_SEC / N * 1e6,
                            (unsigned long long)(n_text / N), (unsigned long long)(blit_px / N));
                }
                {
                    clock_t c0;
                    setup(S_HOME); pal(UI_ACID_INDEX); draw(S_HOME);
                    blit_px = 0;
                    c0 = clock();
                    for (i = 0; i < N * 4; i++) { ui_draw(); nscr = npend = 0; }
                    fprintf(rep, "  HOME frame %.1f us, %llu LCD px\n", (double)(clock() - c0) / CLOCKS_PER_SEC / (N * 4) * 1e6,
                            (unsigned long long)(blit_px / (N * 4)));
                }
            }
        }
        {   /* MENU > LARGE: the same full redraws, tall cards and the strip */
            static const int LC[] = {S_HOME, S_EDIT_ANALOG, S_ENV, S_LFO, S_MIXER, S_FX, S_PATTERN};
            fprintf(rep, "  MENU > LARGE:\n");
            large_on = 1;
            for (k = 0; k < sizeof LC / sizeof LC[0]; k++) {
                clock_t c0;
                uint64_t px, nt;
                setup(LC[k]); pal(1);
                draw(LC[k]);
                px_visited = n_text = 0;
                c0 = clock();
                for (i = 0; i < N; i++) draw(LC[k]);
                px = px_visited / N; nt = n_text / N;
                fprintf(rep, "  %-16s %15.1f %13llu %15llu\n", S_NAME[LC[k]], (double)(clock() - c0) / CLOCKS_PER_SEC / N * 1e6,
                        (unsigned long long)nt, (unsigned long long)px);
            }
            {
                clock_t c0;
                setup(S_HOME); pal(1); draw(S_HOME);
                px_visited = n_text = 0;
                c0 = clock();
                for (i = 0; i < N * 4; i++) { ui_draw(); nscr = npend = 0; }
                fprintf(rep, "  LARGE HOME, a frame without force (lazy strips): %.1f us, %llu texts\n",
                        (double)(clock() - c0) / CLOCKS_PER_SEC / (N * 4) * 1e6, (unsigned long long)(n_text / (N * 4)));
                setup(S_ENV); pal(1); draw(S_ENV);
                px_visited = n_text = 0;
                c0 = clock();
                for (i = 0; i < N * 4; i++) { ui_draw(); nscr = npend = 0; }
                fprintf(rep, "  LARGE ENV, a frame without force (lazy strips): %.1f us, %llu texts\n",
                        (double)(clock() - c0) / CLOCKS_PER_SEC / (N * 4) * 1e6, (unsigned long long)(n_text / (N * 4)));
            }
            large_on = 0;
        }
    }
    al_off = 0;
    al_keycaps();
    align_sweeps();
    al_table();
    if (al_rep) fclose(al_rep);
    fprintf(rep, "\nalignment: %u measurements of %u places, %u off by 1 px or more (OUTDIR/align.txt)\n", al_count, al_nst - 1u, al_fail);
    if (audf) fclose(audf);
    fprintf(rep, "\n%u lint findings, %u inks on their own colour, %u ellipsised free texts; "
            "%u column values swept; %u FM6 charts linted; %u cards linted with MOTION's icon\n", nfind, nink, nfree,
            nsweep, fmp_charts, nmotion);
    if (!nmotion && FELUCCA_FM4) { fprintf(stderr, "ui_render: the MOTION sweep marked no card\n"); nfind++; }   /* (3c-1 task 7:
                                                         * GLO > SENDS and the MIXER gone, no JIANT card holds a track's setting
                                                         * but DIGITAL's, a FELUCCA_FM4 build's) */
    fclose(rep);
    printf("ui_render: %u screens x %u palettes + the page/value sweep; %u lint findings, %u ellipsised free texts (%u distinct), "
           "%u inks on their own colour; report %s\n",
           (unsigned)(S_COUNT - (FELUCCA_FM4 ? 0 : 1 + S_OP_LEVEL - S_ALG1 + 1)), (unsigned)NPALETTES, nfind, nfree, nfree_seen, nink,
           path);
    printf("ui_render: alignment: %u measurements of %u places, %u off by 1 px or more%s\n", al_count, al_nst - 1u, al_fail,
           al_fail ? " (see align.txt)" : "");
    return nfind || nink || al_fail ? 1 : 0;
}
