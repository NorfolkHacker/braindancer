/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* Felucca UI: the panel (Y_GRAPH .. Y_GRAPH + H_GRAPH) between the cards and the footer: a SURF area
 * (rounded, on BG) holding the page graphs (ADSR, LFO, steps, piano roll, drum grid, scale, FX, SLICER,
 * MOD, lists: presets, user presets, patterns, project slots, song), the HOME oscilloscope, the
 * instrument diagrams; the MIXER page draws four SURF columns instead. Each graph is redrawn only when
 * graph_signature() changes. The look:
 * curves THEME (2 px), guides and empty marks RAISE, captions MID / DIM, the active thing ACCENT,
 * bars rounded; a list's selected row is a THEME bar with INK text. */
#define PANEL_X0 10                                  /* the graphs' inner area: x 10..230 */
#define PANEL_W 220
#define GOY 11                                       /* graphs drawn on a 100 px scale sit at y 11..111 */
/* a lens strip: cleared to BG, sent whole (ACID FM 3a: HUD's HOME, every strip on the beings, gone) */
static void lz_begin(uint32_t w, uint32_t h, uint32_t x, uint32_t y)
{
    (void)x; (void)y;
    cv_begin(w, h, T_BG);
}
static void lz_blit(uint32_t x, uint32_t y)
{
    cv_halo = 0;
    cv_blit(x, y);
}

/* (0.8.9 .. ACID FM 3b task 8) a lens's tiles (lz_tile_bg, lz_uline: FX's modules, the last of them) gone with FX's lens */

/* the height ADSR and LFO are drawn on: 100 px, or (MENU > LARGE's strip: draw_graph) the strip's ~ 56 */
static int32_t graph_ht = 100;
#include "ui_organic.c"                              /* JIANT FM's organic line art (drawn by code) */
#include "ui_fm6op.c"                                /* (0.6.10, from Felucca 1.5) FM6's operators (3b task 5: EDIT > OPERATOR, OP ENV) */
static uint16_t heat_col(int32_t v);
/* knob c's colour on a lens card, a layer's KNOBS, EDIT's chips: the knob's own (UI_KNOBS: ACID SLIDE VIOLET ACCENT;
 * until ACID FM 0.3 JIANT's palette its value's heat) */
static uint16_t knob_col(uint32_t c, int32_t ratio, int bip)
{
    (void)ratio; (void)bip;
    return T_K(c);
}
/* (ACID FM 3b task 5) ui_being.c (JIANT 0.5's beings) and ui_engvis.c (0.9.4's creatures, the last FM6's colony and
 * DRUM-X's kit) gone: the user's "get rid of any creatures"; (3b task 8) the output's level they and FX's lens read
 * (bg_energy) gone with the lens: FX > MASTER's meter reads the output itself (ux_section.c uxs_out_db10) */

/* Matches voice.c: attack is linear, decay and release are exponential
 * (env += (target - env) * k each tick, ~99 % after the set time). Time
 * axis is the parameter value (the times themselves are exponential).
 * (ACID FM 3b task 6 fix) Only DIGITAL's OP ENV pages (FELUCCA_FM4) still reach it, and MENU > LARGE's strip on them: ENV's
 * own page is ux_section.c's. Built with FELUCCA_FM4 only (tests/ui_test.c's FM4 build covers it) */
#if FELUCCA_FM4
static void graph_adsr(const track_t *t, uint16_t c)
{
    const page_t *pg = cur_page();
    int32_t a = 4 + t->p[pg->id[0]] * 50 / 127, d = 6 + t->p[pg->id[1]] * 50 / 127, r = 6 + t->p[pg->id[3]] * 60 / 127;
    int32_t top = 6 * graph_ht / 100, bot = 88 * graph_ht / 100, sus = t->p[pg->id[2]] * 1000 / 127;   /* 0..1000 */
    int32_t x0 = 12, x1 = x0 + a, x3 = 226 - r, i, px, py;
    int32_t e = 32768;                                                  /* exp(-4.6 u), Q15 */
#define EGY(lvl) (bot - (lvl) * (bot - top) / 1000)
    cv_rect(PANEL_X0, bot + 2, PANEL_W, 1, T_RAISE);
    cv_line_t(x0, bot, x1, top, c, 2);                                  /* attack: linear */
    px = x1;
    py = top;
    for (i = 1; i <= d; i++) {                                          /* decay: exponential to SUS */
        int32_t lvl;
        e = (e * (32768 - 150733 / d)) >> 15;           /* k^d = exp(-4.6) */
        lvl = sus + ((1000 - sus) * e >> 15);
        cv_line_t(px, py, x1 + i, EGY(lvl), c, 2);
        px = x1 + i;
        py = EGY(lvl);
    }
    cv_line_t(px, py, x3, EGY(sus), c, 2);                               /* sustain */
    px = x3;
    py = EGY(sus);
    e = 32768;
    for (i = 1; i <= r; i++) {                                          /* release: exponential to 0 */
        e = (e * (32768 - 150733 / r)) >> 15;
        cv_line_t(px, py, x3 + i, EGY(sus * e >> 15), c, 2);
        px = x3 + i;
        py = EGY(sus * e >> 15);
    }
    /* (ACID FM 3b task 6) ENV's own page is ux_section.c's: JIANT's dot of the envelope now (ENV's page only) gone with it;
     * this draws DIGITAL's OP ENV pages (FELUCCA_FM4) and MENU > LARGE's strip */
#undef EGY
}
#endif

/* (ACID FM 3b task 7) graph_lfo (LFO's wave, JIANT 0.5; MENU > LARGE's LFO strip) gone: LFO's pages are ux_section.c's, their
 * wave drawn from the engine's (voice.c lfo_wave, track_lfo_tick) */


/* (0.6.43) SEQ > MUTATE: a row a track, the next 32 bars: SOUND's changes an orange mark on top, PATTERN's a violet one
 * under it; the bar now ACCENT; the selected track's row lit; ORBIT / DRIFT at the right; FROZEN over it all */
/* (0.6.92) the selected track's next 32 bars: a mark where its SOUND changes (warm) and where its PATTERN does (cool),
 * the bar now at the left; under it what it does (every project's steps written; ORBIT around the anchor OCT+ keeps,
 * DRIFT on and on) and the two hands */
/* (0.6.45) EDIT > MORPH: a cycle of the wave as it morphs (from WAVE towards the next ones), heat coloured by its
 * height; under it the two waves it is between */
/* (ACID FM 3b task 4) graph_sing (EDIT > WORDS: what VOICE sings) and graph_morph (EDIT > MORPH: the wave and where MORPH takes
 * it) gone: those pages are a synth's EDIT MORE rows now (ux_section.c draws the synth's wave, MORPH in it) */

/* (0.8.5 .. ACID FM 3c-1 task 4) the piano roll of JIANT's SEQ pages gone (SEQ is ux_seq.c's): the pieces ux_seq.c draws its roll
 * with stay, a step's notes (pr_note), the step a TIE's notes sound from (pr_src), a ratchet's parts (pr_split) */
static int32_t pr_note(const step_t *st, uint32_t j)   /* the step's note j (0..3), then its lane hits (-1: none) */
{
    if (j < 4u)
        return j < st->n ? st->note[j] : -1;
    return (st->hit >> (j - 4u)) & 1u ? DRUM_LANE_NOTE[j - 4u] : -1;
}
static uint32_t pr_src(const track_t *t, uint32_t si, uint32_t len)   /* the step whose notes sound at si (NSTEP: none) */
{
    uint32_t k;
    for (k = 0; k < len && seq_steps(t)[si].time == ST_TIE; k++)
        si = (si + len - 1u) % len;
    return step_on(&seq_steps(t)[si]) ? si : NSTEP;
}
/* a bar w px wide in `hits` equal parts, 1 px apart (a RATCH step; 1: the bar) */
static void pr_split(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c, uint32_t hits)
{
    int32_t k, pw = (w - (int32_t)hits + 1) / (int32_t)hits;
    if (hits < 2u) {
        cv_rect(x, y, w, h, c);
        return;
    }
    for (k = 0; k < (int32_t)hits; k++)
        cv_rect(x + k * (pw + 1), y, pw, h, c);
}
/* (ACID FM 3c-1 task 4) LENTE's bars and the roll and DRUM grid as JIANT's SEQ pages drew them (graph_roll, graph_grid) gone:
 * SEQ is ux_seq.c's */
static uint32_t steps_hash(const track_t *t)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < NSTEP; i++) {
        const step_t *st = &seq_steps(t)[i];
        h = (h ^ (st->note[0] + st->n * 128u + st->time * 1024u + st->flags * 4096u + st->note[1] * 65536u)) *
            16777619u;
        h = (h ^ (st->hit | (uint32_t)st->acc << 8 | (uint32_t)st->note[2] << 16 | (uint32_t)st->note[3] << 24)) * 16777619u;
        h = (h ^ ((uint32_t)st->cwhen >> 4)) * 16777619u;   /* (0.9.1: MICRO's sixteenth; its 64ths are in the flags) */
    }
    return h;
}

static uint32_t str_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}



/* The FM charts' parts: an operator box 21 x 17 (rows 23 px apart, columns 24), junction dots, Manhattan routes */
#define FM_BH 17
static void fm_dot(int32_t x, int32_t y, uint16_t c) { cv_rect(x - 1, y - 1, 3, 3, c); }
/* a Manhattan route, 1 px: down from (x0, y0) to the jog row jy, across to x1, down to y1 */
static void fm_route(int32_t x0, int32_t y0, int32_t x1, int32_t jy, int32_t y1, uint16_t c)
{
    cv_rect(x0, y0, 1, jy - y0, c);
    cv_rect(x0 < x1 ? x0 : x1, jy, (x0 < x1 ? x1 - x0 : x0 - x1) + 1, 1, c);
    cv_rect(x1, jy, 1, y1 - jy, c);
}
/* a feedback loop on the operator box at (x, y): out of its right side, up, back in on top with an arrow */
static void fm_loop(int32_t x, int32_t y, uint16_t c)
{
    cv_rect(x + 11, y + 5, 5, 1, c);
    cv_rect(x + 15, y - 4, 1, 9, c);
    cv_rect(x, y - 4, 15, 1, c);
    cv_rect(x, y - 4, 1, 4, c);
    cv_line(x - 2, y - 3, x - 1, y - 2, c); cv_line(x + 2, y - 3, x + 1, y - 2, c);
}
/* the output bus under the carriers (the leftmost at cl, the rightmost at cr) at y bus, an arrow out on the right */
static void fm_bus(int32_t cl, int32_t cr, int32_t bus)
{
    cv_rect(cl, bus, cr + 24 - cl, 1, T_MID);
    cv_line(cr + 20, bus - 3, cr + 23, bus, T_MID); cv_line(cr + 20, bus + 3, cr + 23, bus, T_MID);
}

/* FM6's 32 algorithms as charts (0.9.6; ACID FM 3b task 5: EDIT's FM6 picture, ux_section.c uxs_fm6_draw). FM6_CELL: per operator 1..6 its cell, the column in 24 px steps (bits 0..2) and
 * the row above the output bus (bits 4..5; row 0 = the carriers). The routes, the carriers and the feedback
 * operator are fm6_core.c's FM6_ALG itself (fm6_routes, fm6_car_ops, fm6_fb_op). ui_test.c checks the cells
 * against them (a modulator one row above what it modulates, the carriers on the bottom row and only they, one
 * operator per cell) and the routes against the 32 algorithms written out; ui_render.c lints every colony as drawn
 * (no two bichos overlapping, all inside the panel). The cells:
 * a search for the fewest and shortest bends, the operators numbered left to right. */
static const uint8_t FM6_CELL[32][6] = {
    {0x00, 0x10, 0x01, 0x11, 0x21, 0x31}, {0x00, 0x10, 0x02, 0x12, 0x22, 0x32}, {0x00, 0x10, 0x20, 0x01, 0x11, 0x21},
    {0x00, 0x10, 0x20, 0x01, 0x11, 0x21}, {0x00, 0x10, 0x01, 0x11, 0x02, 0x12}, {0x00, 0x10, 0x01, 0x11, 0x02, 0x12},
    {0x00, 0x10, 0x01, 0x11, 0x12, 0x22}, {0x00, 0x10, 0x01, 0x12, 0x11, 0x21}, {0x00, 0x10, 0x02, 0x12, 0x13, 0x23},
    {0x00, 0x10, 0x20, 0x01, 0x11, 0x12}, {0x00, 0x10, 0x20, 0x01, 0x11, 0x12}, {0x00, 0x10, 0x03, 0x12, 0x13, 0x14},
    {0x00, 0x10, 0x02, 0x11, 0x12, 0x13}, {0x00, 0x10, 0x01, 0x11, 0x20, 0x21}, {0x00, 0x10, 0x02, 0x12, 0x21, 0x22},
    {0x01, 0x10, 0x11, 0x21, 0x12, 0x22}, {0x01, 0x12, 0x10, 0x20, 0x11, 0x21}, {0x01, 0x10, 0x12, 0x11, 0x21, 0x31},
    {0x00, 0x10, 0x20, 0x01, 0x02, 0x11}, {0x00, 0x01, 0x10, 0x02, 0x12, 0x13}, {0x00, 0x01, 0x10, 0x02, 0x03, 0x12},
    {0x00, 0x10, 0x01, 0x02, 0x03, 0x12}, {0x00, 0x01, 0x11, 0x02, 0x03, 0x12}, {0x00, 0x01, 0x02, 0x03, 0x04, 0x13},
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x13}, {0x00, 0x01, 0x11, 0x02, 0x12, 0x13}, {0x00, 0x01, 0x11, 0x03, 0x13, 0x14},
    {0x00, 0x10, 0x01, 0x11, 0x21, 0x02}, {0x00, 0x01, 0x02, 0x12, 0x03, 0x13}, {0x00, 0x01, 0x02, 0x12, 0x22, 0x03},
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x14}, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}};
/* algorithm a's routes: m[i] bit j: operator j + 1 modulates operator i + 1. FM6_ALG runs the operators sixth
 * first, each reading a bus (or none) and writing one (or adding to it): who wrote a bus modulates its reader */
static void fm6_routes(uint32_t a, uint8_t *m)
{
    const uint8_t *f = FM6_ALG[a & 31u];
    uint8_t bus[4] = {0, 0, 0, 0};
    uint32_t k;
    for (k = 0; k < 6u; k++) {
        uint32_t op = 5u - k, in = (f[k] >> 4) & 3u, o = f[k] & 3u;
        m[op] = in ? bus[in] : 0u;
        bus[o] = (uint8_t)(((f[k] & FM6_OADD) ? bus[o] : 0u) | 1u << op);
    }
}
/* the operator with feedback in algorithm a (0..5: operator 1..6) */
static uint32_t fm6_fb_op(uint32_t a)
{
    uint32_t k;
    for (k = 0; k < 5u && (FM6_ALG[a & 31u][5u - k] & 0xC0u) != 0xC0u; k++)
        ;
    return k;
}
/* the carriers of algorithm a: bit k operator k + 1 (fm6_carriers counts the sixth first) */
static uint32_t fm6_car_ops(uint32_t a)
{
    uint32_t c = fm6_carriers(a), r = 0, k;
    for (k = 0; k < 6u; k++)
        r |= (c >> (5u - k) & 1u) << k;
    return r;
}
/* the algorithm FM6 plays on track t (0..31): ALG, or the patch's at PAT */
static uint32_t fm6_alg_of(const track_t *t)
{
    return t->p[P_E0] >= 1 && t->p[P_E0] <= 32 ? (uint32_t)t->p[P_E0] - 1u : fm6_patch[(t - trk) % NTRK][FP_ALG] & 31u;
}
#if FELUCCA_FM4
/* DIGITAL's eight algorithms as FM charts, read from src/eng_digital.c's switch (alg) (ui_test.c checks
 * these tables against it). FM_CELL: per operator 1..4 its grid cell, the column in 24 px steps (bits 0..2)
 * and the row above the output bus (bits 4..5; row 0 = the carriers). FM_MOD: per destination operator d a
 * nibble at bit 4 d of the operators modulating it (bit k = operator k + 1). Operator 4 feeds itself (FB). */
static const uint8_t FM_CELL[8][4] = {
    {0x00, 0x10, 0x20, 0x30}, {0x01, 0x11, 0x20, 0x22}, {0x01, 0x10, 0x20, 0x12}, {0x01, 0x12, 0x10, 0x20},
    {0x00, 0x10, 0x02, 0x12}, {0x00, 0x02, 0x04, 0x12}, {0x00, 0x02, 0x04, 0x14}, {0x00, 0x02, 0x04, 0x06}};
static const uint16_t FM_MOD[8] = {0x0842, 0x00C2, 0x004A, 0x0806, 0x0802, 0x0888, 0x0800, 0x0000};
/* Modulators above what they modulate, the carriers on the bottom row over the output bus (an arrow out).
 * Carriers THEME with INK numerals, modulators RAISE with THEME ones; an operator at LEVEL 0: DIM numeral.
 * Routes by IDX: DIM at 0, MID, THEME from 64; op 4's feedback loop MID (DIM at FB 0). The knob just turned
 * (an operator's ratio or level, IDX, FB): ACCENT. Junction dots where routes split or merge. */
static void graph_fm(const track_t *t, uint16_t c)
{
    uint32_t alg = (uint32_t)t->p[P_E0] & 7u, k, j, mods = FM_MOD[alg], car = 0, hop = 9;
    uint32_t hot = ui.hot_t ? (uint32_t)cur_page()->id[ui.hot_col & 3u] : 0u;
    int32_t x[4], y[4], lo = 7, hi = 0, top = 0, bus, cl = 240, cr = 0, idx = t->p[P_E4];
    uint16_t ec = hot == P_E4 ? T_ACCENT : !idx ? T_DIM : idx < 64 ? T_MID : c;
    if (hot >= P_E1 && hot <= P_E3) hop = hot - P_E1 + 1u;
    else if (hot >= P_FM1_ATK && hot < P_E0) hop = (hot - P_FM1_ATK) / 5u;
    for (k = 0; k < 4u; k++) {
        int32_t col = FM_CELL[alg][k] & 7, row = FM_CELL[alg][k] >> 4;
        lo = col < lo ? col : lo; hi = col > hi ? col : hi; top = row > top ? row : top;
    }
    bus = 50 + (27 + 23 * top) / 2;                  /* the chart centred on the 100 px scale */
    for (k = 0; k < 4u; k++) {
        x[k] = 120 + (FM_CELL[alg][k] & 7) * 24 - (lo + hi) * 12;
        y[k] = bus - 6 - FM_BH - 23 * (FM_CELL[alg][k] >> 4);
        if (FM_CELL[alg][k] < 0x10) { car |= 1u << k; cl = x[k] < cl ? x[k] : cl; cr = x[k] > cr ? x[k] : cr; }
    }
    for (k = 0; k < 4u; k++) {                       /* into k: sources down to its jog row, across, down */
        uint32_t m = (mods >> (4u * k)) & 15u;
        for (j = 0; j < 4u; j++) if (m >> j & 1u) {
            uint32_t fan = mods & (0x1111u << j);
            fm_route(x[j], y[j] + FM_BH, x[k], y[k] - 3, y[k], ec);
            if (fan & (fan - 1u)) fm_dot(x[j], y[k] - 3, ec);   /* one operator modulating several */
        }
        if (m & (m - 1u)) fm_dot(x[k], y[k] - 3, ec);           /* several modulating one */
        if (car >> k & 1u) {
            cv_rect(x[k], y[k] + FM_BH, 1, bus - y[k] - FM_BH, T_MID);
            if (x[k] != cl) fm_dot(x[k], bus, T_MID);
        }
    }
    fm_bus(cl, cr, bus);                             /* the output bus and its arrow */
    fm_loop(x[3], y[3], hot == P_E6 ? T_ACCENT : t->p[P_E6] ? T_MID : T_DIM);   /* op 4's feedback */
    for (k = 0; k < 4u; k++) {
        char b[2] = {(char)('1' + k), 0};
        uint32_t on = t->p[P_FM1_LEVEL + k * 5u] != 0, fill_c = (car >> k & 1u) && on;
        uint16_t f = k == hop ? T_ACCENT : c, fill = fill_c ? f : T_RAISE;
        cv_rrect(x[k] - 10, y[k], 21, FM_BH, 3, fill, T_SURF);
        GFX_HOOK_ALIGN(x[k] - 10, y[k], x[k] + 11, y[k] + FM_BH, AL_HV, "DIGITAL operator number in its box");
        cv_text_in(x[k] - 10, y[k] + CAP_IN(S, FM_BH), 21, &AF_S, b, fill_c ? T_INK : on ? f : T_DIM, fill);
    }
}
#endif
/* Save-bank validity is expensive (CRC/import). Refresh it once per redraw or
 * twice per second, rather than parsing four projects on every UI frame. */
static char graph_pname[4][13];                       /* .. and the slots' names ("" none) */
static uint32_t graph_pname_sig;
static int graph_project_used(uint32_t slot)
{
    static uint32_t ms, frame;
    static uint8_t mask, ready;
    if (!ready || fm1_ms - ms >= 500u || (ui.force && frame != ui.frame)) {
        uint32_t i; mask = 0;
        for (i = 0; i < 4u; i++) mask |= (uint8_t)((project_name(i, graph_pname[i]) != 0) << i);
        graph_pname_sig = fnv(2166136261u, graph_pname, sizeof graph_pname);
        ready = 1; ms = fm1_ms; frame = ui.frame;
    }
    return (mask >> (slot & 3u)) & 1u;
}
static const char *graph_project_name(uint32_t slot)  /* (after graph_project_used) */
{
    return graph_pname[slot & 3u];
}

/* MENU > LARGE's strip under the tall cards (ui.c LK_TALL): HOME's scope, DIGITAL's ADSR, PATTERN's steps; on the other pages
 * the page's title and number (their charts need the whole panel) */
enum { SK_NONE, SK_SCOPE, SK_ADSR, SK_STEPS, SK_TITLE };   /* (3b task 7: SK_LFO gone, LFO the ux pages'; 3c-1 task 7: SK_TRK,
                                                         * MIXER's tracks drawn small, gone with it) */
static uint32_t strip_kind(void)
{
    uint32_t g;
    if (large_kind() != LK_TALL)
        return SK_NONE;
    if (ui.home)
        return SK_SCOPE;
    g = cur_page()->graph;
    return g == GR_ADSR && FELUCCA_FM4 ? SK_ADSR : g == GR_STEPS ? SK_STEPS : SK_TITLE;
}
/* the page's title and its number in its family ("ENV DEST 2/2", EDIT: the engine's "OSC 1/2"); the piano roll: its
 * scale ("C MIN"); ti holds 20 (the footer, the strip) */
static int roll_view(const page_t *pg);
static void page_title(char *ti)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    const engine_t *e = trk_engp(t);
    uint32_t i, n = 0, k = 0;
    char nb[24];
    const char *pt = pg->scope == SC_ENGINE ? e->page_title[pg->id[0] != P_E0] : 0;   /* EDIT: the engine's */
    str_cpy(nb, pg->title, sizeof nb);
    if (!page_over && !ui.home) {                       /* (0.9.2) the page within its group ("ENV DEST 2/2"; 3b task 8: FX's
                                                         * effect's name, "DELAY 2/3", gone with its lens) */
        uint8_t cnt[8], gi[8];
        uint32_t g, m;
        (void)nav_groups(cnt, gi, &g, &k, &i, &m);
        n = cnt[g];
        k++;
        str_cpy(nb, nav_page_name(), sizeof nb);
    } else
        for (i = 0; i < NPAGES; i++)
            if (PAGES[i].fam == pg->fam && page_visible(i)) {
                n++;
                if (i == ui.page)
                    k = n;
            }
    str_cpy(ti, pt ? pt : grid_on() && roll_view(pg) ? "GRID" : nb, 12);
    if (n > 1) {
        str_cpy(ti + str_len(ti), " ", 4);
        fmt_int(ti + str_len(ti), (int32_t)k);
        str_cpy(ti + str_len(ti), "/", 4);
        fmt_int(ti + str_len(ti), (int32_t)n);
    }
    if (roll_view(pg) && !drum_track(t)) {          /* the piano roll: its tinted rows' scale ("C MIN") */
        str_cpy(ti, N_NOTE[(uint32_t)t->p[P_ROOT] % 12u], 4);
        str_cpy(ti + str_len(ti), " ", 4);
        str_cpy(ti + str_len(ti), N_SCALE[clamp(t->p[P_SCALE], 0, (int32_t)(sizeof N_SCALE / sizeof N_SCALE[0]) - 1)], 8);
    }
}
/* the strip's title: L centred (M when L lacks a glyph or is too wide) */
/* a value 0..256 as heat: the surface toward the accent, then the text (ACID FM 0.3; until then JIANT's palette a thermal
 * camera's ramp, indigo violet red orange amber cream white) */
static uint16_t heat_col(int32_t v)
{
    v = clamp(v, 0, 256);
    return v < 128 ? ux_mix(T_SURF, T_ACCENT, v * 100 / 128) : ux_mix(T_ACCENT, T_TEXT, (v - 128) * 100 / 128);
}

static void graph_title(void)
{
    char ti[20];
    const aafont_t *f = &AF_L;
    int32_t y;
    page_title(ti);
    if (!large_face_has(ti) || text_w(f, ti) > 220)
        f = &AF_M;
    y = f == &AF_L ? CAP_IN(L, LG_H_GRAPH) : CAP_IN(M, LG_H_GRAPH);
    GFX_HOOK_ALIGN(0, 0, 240, LG_H_GRAPH, AL_HV, "large strip title centred");
    cv_text_in(0, y, 240, f, ti, T_THEME, T_SURF);
}

static int roll_view(const page_t *pg)
{
    return pg->fam == FAM_SEQ && pg->graph == GR_ROLL;
}

/* (0.6.51) the page's name big (page_flash). (0.6.61) In the header, not over the cards or the graph (covering them
 * lost the sense of what a turn did): for PF_MS the header shows the page's name (M) and, smaller at the right, the
 * track and its engine; on HOME (a track picked there) the track's engine big and "HOME" small. Then the header comes
 * back (ui.force). (0.6.64) Over the footer instead, which says the same: the header (load, battery) stays */
static void draw_crumbs(int32_t y, uint16_t fg, uint16_t last);   /* (ui_draw.c) */
#define CRUMB_Y 4                                    /* (ui_draw.c: a lens footer's breadcrumb line) */
static void page_flash_draw(void)
{
    static uint32_t drawn;
    uint32_t e = fm1_ms - pf_ms, ph, dim;
    char nm[20], sub[20] = {'T', 0, 0};
    const page_t *pg = cur_page();
    const char *pt = pg->scope == SC_ENGINE ? trk_engp(TSEL)->page_title[pg->id[0] != P_E0] : 0;
    const char *en = trk_engp(TSEL)->name;
    if (e >= PF_MS) {
        pf_on = 0;
        ui.foot_sig ^= 1u;                           /* (0.6.64) the footer, whole again: that alone redrawn */
        return;
    }
    dim = e > PF_MS - 120u;
    ph = (pf_ms << 1 | dim) + 1u;                     /* (a new flash or its dimming; else every 4th frame: cheap) */
    if (!ui.force && ph == drawn && (ui.frame & 3u))
        return;
    drawn = ph;
    sub[1] = (char)('1' + song.sel % NTRK);
    sub[2] = ' ';
    if (ui.home) {                                   /* HOME: the engine big, "T2 HOME" small */
        str_cpy(nm, en, sizeof nm);
        str_cpy(sub + 3, "HOME", sizeof sub - 3u);
    } else {
        str_cpy(nm, pt && pt[0] != '-' ? pt : pg->title, sizeof nm);
        str_cpy(sub + 3, en, sizeof sub - 3u);
    }
    if (lz_on()) {                                   /* (0.8.5) a lens page: its breadcrumb lit (the tabs say the level) */
        lz_begin(240, foot_h(), 0, foot_y());
        draw_crumbs(CRUMB_Y + LZ_FOOT_Y - (int32_t)foot_y(), dim ? T_MID : T_TEXT, dim ? T_MID : T_TEXT);
        lz_blit(0, foot_y());
        return;
    }
    {   /* (0.6.64) at the bottom: the footer says the same */
        int32_t fh = H_FOOT;
        cv_begin(240, (uint32_t)fh, T_BG);
        cv_bg = T_BG;
        cv_text(8, (fh - (int32_t)AF_M.h) / 2, &AF_M, nm, dim ? T_MID : T_TEXT);
        cv_text_r(232, (fh - (int32_t)AF_S.h) / 2, &AF_S, sub, dim ? T_DIM : T_ACCENT, T_BG);
        cv_blit(0, foot_y());
    }
}

/* (0.6.61) the panels' cytoplasm: a few granules drifting behind the graph, as HOME's, so the screen breathes. LOAD
 * FULL and ANIM ON only, not on HOME (its own), the lists and the dense grids (the roll, the steps); (0.6.64) no
 * redraw of their own: they move when the panel redraws anyway (a knob, the LFO, the envelope), so they cost nothing */
#ifndef UI_NO_DRIFT
#define UI_NO_DRIFT 0                                /* (tests/ui_test.c: 1, its frames compared pixel by pixel) */
#endif
static int drift_on(void)
{
    uint32_t g = cur_page()->graph;
    if (UI_NO_DRIFT)
        return 0;
    return !ui.home && !sys_load && !(ui_prefs & PREF_ANIM_OFF) && strip_kind() == SK_NONE && !roll_view(cur_page()) && !lens_on() &&
           g != GR_ROLL && g < GR_LIVE &&               /* (0.8.3: LENTE's step; 0.9.11: LIVE's rails, the enum's last) */
           g != GR_STEPS && g != GR_PATS;
}
static void graph_drift(void)
{
    uint32_t k, gh = graph_h();
    for (k = 0; k < 14u; k++) {
        uint32_t h = k * 2654435761u + 0x9E37u;
        int32_t gx = 10 + (int32_t)((h >> 8) % 220u + ui.frame * (1u + (h >> 28)) / 8u) % 220;
        int32_t gy = 4 + (int32_t)(((h >> 16) % (gh - 8u)) + (uint32_t)(sine_i((ui.frame * 200u + h) << 16) >> 13)) % (int32_t)(gh - 8u);
        cv_rect(gx, gy, 1 + (int32_t)(h >> 31), 1 + (int32_t)(h >> 31), ux_mix(T_SURF, T_THEME, 18 + (int32_t)(h >> 29) * 3));
    }
}
/* (0.8.5) SAVE > LIBRARY's picture of the other projects: (ACID FM 3c-2 task 4) SAVE > PROJECT's (ux_save.c lib, lib_poll) */
/* (0.9.5) a toast (ui_say) on a lens page: a pill of BG behind its words alone, near the panel's foot, over the page
 * drawn as ever (its graph, roll, beings live behind it); HOME and the layers: the header says it.
 * The pill keeps clear of the page's own words (gfx.c cv_txt: every text drawn on the panel this frame): near the foot
 * if nothing is written there, else the nearest rows lower, then higher, with none; a page written all over (MUTATE's
 * lines, AUTO LIST's rows where no gap is a pill high): no pill, the header says it alone, as on HOME */
static int toast_on(void) { return lz_on() && ui.msg_t && ui.msg[0] && !ui.layer && !ui.home; }
#define TOAST_H 17
#define TOAST_GAP_X 3                                /* (the clearance round a word: its 2 px halo and a pixel) */
#define TOAST_GAP_Y 2
static int16_t toast_at[4];                          /* the pill drawn last (screen x0 y0 x1 y1; x1 == x0: none) */
static int toast_clear(int32_t x, int32_t y, int32_t w)
{
    return !cv_txt_in(x - TOAST_GAP_X, y - TOAST_GAP_Y, x + w + TOAST_GAP_X, y + TOAST_H + TOAST_GAP_Y);
}
/* the pill's top row on the panel (canvas rows), -1: nowhere clear */
static int32_t toast_row(int32_t x, int32_t w)
{
    int32_t gh = (int32_t)graph_h(), y0 = gh >= 3 * TOAST_H ? gh - TOAST_H - 5 : (gh - TOAST_H) / 2, y;
    for (y = y0; y <= gh - TOAST_H - 1; y++)         /* where it was, or lower (nearer the foot) */
        if (toast_clear(x, y, w))
            return y;
    for (y = y0 - 1; y >= 1; y--)                    /* .. else higher */
        if (toast_clear(x, y, w))
            return y;
    return -1;
}
static void toast_pill(void)
{
    int32_t w = clamp(text_w(&AF_S, ui.msg) + 16, 40, 224), x = 120 - w / 2, y = toast_row(x, w);
    uint8_t hl = cv_halo;
    char f[40];
    toast_at[0] = toast_at[2] = 0;
    if (y < 0)                                       /* (no clear rows: the header's words alone) */
        return;
    toast_at[0] = (int16_t)x; toast_at[1] = (int16_t)(y + (int32_t)graph_y());
    toast_at[2] = (int16_t)(x + w); toast_at[3] = (int16_t)(toast_at[1] + TOAST_H);
    text_fit(f, sizeof f, ui.msg, &AF_S, w - 12);
    GFX_HOOK_COVER(x, y + cv_oy, x + w, y + cv_oy + TOAST_H);   /* (what the page has under it: hidden) */
    cv_halo = 1;                                     /* (its round ends blended onto what is there) */
    cv_rrect(x, y, w, TOAST_H, -(TOAST_H / 2), ux_mix(T_BG, T_ACCENT, 70), T_BG);
    cv_rrect(x + 1, y + 1, w - 2, TOAST_H - 2, -(TOAST_H / 2 - 1), T_BG, T_BG);
    cv_halo = 0;
    GFX_HOOK_ALIGN(x, y, x + w, y + TOAST_H, AL_HV, "toast message centred in its pill");
    cv_text_flags(ink_in(&AF_S, f, w) + x, y + CAP_IN(S, TOAST_H), &AF_S, f, T_ACCENT, T_BG, 8u | (f[str_len(f) - 1u] == ELLIPSIS));
    cv_halo = hl;
}
static void sound_name(const track_t *t, char *b);   /* (ui_draw.c) */
static void gen_panel(void);                         /* (0.8.5) gen.c: GEN in HOME's lens */
static uint32_t gen_draw_sig(void);
static uint32_t graph_signature(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint32_t h = 2166136261u, i;
    if (toast_on())                                  /* (0.8.5) the toast over the panel (0.9.5: a pill on the page) */
        h = str_hash(h ^ 0x70A57u, ui.msg);
    if (ui.confirm == CF_DICE_ALL)                   /* (0.8.5) GEN: its settings, its page (0.9.0: MENU > LARGE's strip is
                                                      * HOME's scope there: live) */
        return h ^ gen_draw_sig() ^ (strip_kind() == SK_SCOPE ? ui.frame / 2u * 2654435761u : 0u);
    h ^= (uint32_t)pg->graph * 131u + TSEL->eng_req + song.sel * 7777u + ui.page * 1291u;
    if (strip_kind() == SK_TITLE)                    /* MENU > LARGE: the page's title (its engine, its number) */
        return h ^ 0x5A17u;
    if ((pg->graph == GR_NONE && !fx_on()) || 0) h ^= ui.frame / 2u;   /* (3b task 8:
                                                         * FX the ux pages', its lens's FX live gone) */
    for (i = 0; i < P_COUNT; i++)
        h = (h ^ (uint32_t)t->p[i]) * 16777619u;
    h ^= (uint32_t)TSEL->preset * 7u + (uint32_t)song.g[G_SLOT] * 13u + TSEL->user * 257u + up_gen * 7919u + ui.uslot * 104729u +
         ui.ppick * 1299709u;
    /* (ACID FM 3c-2 task 3) ARRANGE's (the rows, the scenes, the END, where the song is): SAVE > SONG is ux_save.c's (uxv_sig) */
    if (pd_on()) {                                   /* .. a lens of ui_lens.c: its level (ENV's TRACKS: every track live) */
        h ^= (pd_level() + 1u) * 668265263u;
    }
    /* (3b task 8: the SLICER's pages, their step and globals, gone: FX > MORE's rows) */
    /* (ACID FM 3c-2 task 4) TOOLS' and PROJECT's (the action picked, the slots, the project): SAVE > MORE's and PROJECT's
     * (ux_save.c uxv_sig) */
    /* (ACID FM 3c-1 task 4) AUTOMATION's, AUTO LIST's, PLAYHEADS', MUTATE's and COMP's: SEQ > MORE's (ux_section.c) */
    if (FELUCCA_FM4 && pg->scope == SC_ENGINE && trk_eng(t) == ENGI_DIGITAL)   /* (ACID FM 3b task 5: FM6's and DRUM-X's EDIT
                                                         * are ux_section.c's, their creatures gone) */
        h ^= (ui.hot_t ? ui.hot_col + 1u : 0u) * 65537u;
    if (lens_on()) {                                 /* (0.8.3) LENTE: the level; its STEP, the cursor step's every byte */
        h = (h ^ (lens_level() + 1u) * 2654435761u) * 16777619u;
        h = (h ^ (song.playing ? seq_src(t, t->seq_idx) / 16u + 1u : 0u) * 40503u) * 16777619u;   /* (0.8.5) the bar
                                                         * playing: its number lit in the bars */
        h = (h ^ (ui.pr_focus + 1u) * 40503u) * 16777619u;
    }
    if (pg->graph == GR_STEPS) {                     /* (3c-1 task 4: the roll's view, its locks, the keys held gone with it) */
        uint32_t ph = song.playing ? seq_src(t, t->seq_idx) : 0xFFFFu;
        h ^= steps_hash(t) + ph * 31u + ui.cursor * 7919u + ui.lane * 104723u + ui.bank * 613u + (uint32_t)song.g[G_SWING] * 3u;
    }
    return h;
}
/* a list row (17 px): the selected one a THEME bar with INK text; tag at x 14, free text from x 54 to x1 */
#define LIST_Y(k) (3 + 17 * (int32_t)(k))
static void list_row(int32_t y, int sel, const char *tag, uint16_t tc, const char *name, uint16_t nc, int32_t x1)
{
    uint16_t bg = sel ? T_THEME : T_SURF;
    if (sel)
        cv_rrect(6, y, 228, 16, 4, T_THEME, T_SURF);
    GFX_HOOK_ALIGN(0, y, 0, y + 16, AL_V, "list row text centred up/down");
    cv_text_on(14, y + CAP_IN(S, 16), &AF_S, tag, sel ? T_INK : tc, bg);
    if (name[0])
        GFX_HOOK_ALIGN(0, y, 0, y + 16, AL_V, "list row text centred up/down");
    cv_free_text(54, y + CAP_IN(S, 16), &AF_S, name, sel ? T_INK : nc, bg, x1 - 54);
}

/* SEQ > PATTERNS: the pattern list around the one picked ("03  MELODY", "U07  MY BASS") */
static void graph_pats(void)
{
    uint32_t n = pat_count(), cur = pat_pick();
    int32_t row, first = clamp((int32_t)cur - 3, 0, (int32_t)n > 7 ? (int32_t)n - 7 : 0);
    for (row = 0; row < 7 && (uint32_t)(first + row) < n; row++) {
        uint32_t k = (uint32_t)(first + row);
        char tag[4], nm[13];
        pat_label(k, tag, nm);
        list_row(LIST_Y(row), k == cur, tag, T_MID, nm, T_TEXT, 232);
    }
}
/* (ACID FM 3c-1 task 7) JIANT's MIXER (four SURF columns: the cushion, REC / ARM / MUTE, LEVEL's knob with the meter, PAN and REV;
 * MENU > LARGE's strip; the JIANT style's six columns, SENDS' bars: draw_tracks, track_col6, track_strip, the knob rings) gone with
 * the page: GLO > MIX is ux_glo.c's */
/* oscilloscope of the output, triggered on a rising zero crossing: a RAISE centre line, the trace 2 px (in silence
 * a flat line on it; MENU > SCREEN OFF keeps it from staying on the panel for hours) */
static void scope_at(int32_t x0, int32_t pw, int32_t cy, int32_t a, uint16_t c);
static void graph_scope(uint16_t c)
{
    scope_at(PANEL_X0, PANEL_W, (int32_t)cv_h / 2, cv_h == H_GRAPH ? 46 : (int32_t)cv_h / 2 - 6, c);   /* (LARGE: the strip) */
}
/* .. pw px wide from x0, about the line cy, +-a px (0.8.5: EDIT's SOUND, in its well) */
static void scope_at(int32_t x0, int32_t pw, int32_t cy, int32_t a, uint16_t c)
{
    static int16_t snap[SCOPE_N];
    uint32_t w = scope_w, i, trig = 0;
    int32_t py = cy, x, peak = 1500;
    for (i = 0; i < SCOPE_N; i++) {
        snap[i] = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (snap[i] > peak)
            peak = snap[i];
        else if (-snap[i] > peak)
            peak = -snap[i];
    }
    for (i = 1; i < SCOPE_N - 240u; i++)
        if (snap[i - 1] < 0 && snap[i] >= 0) {
            trig = i;
            break;
        }
    cv_rect(x0, cy, pw, 1, T_RAISE);
    for (x = 0; x < pw; x++) {
        int32_t y = cy - snap[trig + (uint32_t)x] * a / peak;    /* auto-scaled */
        if (x)
            cv_line_t(x0 - 1 + x, py, x0 + x, y, c, 2);
        py = y;
    }
}

/* (ACID FM 3b task 7) graph_mseq (JIANT's MSEQ: the 16 levels as bars) gone: the STEPS are LFO's MORE rows */
/* (0.6.22) ARRANGE / SEQ > SONG's picture gone (ACID FM 3c-2 task 3: SAVE > SONG, ux_save.c). A section's colour (A..D, each its own
 * heat): the library's chips, SAVE > SONG's sections and rows */
static uint16_t sec_col(uint32_t s)
{
    static const uint8_t HO[4] = {120, 160, 200, 240};   /* (ACID FM 0.3: JIANT's palette's 0 85 171 213 gone) */
    return heat_col(HO[s & 3u]);
}
#include "ui_tech.c"                                /* (JIANT 0.5) the technical graphs (FX, DEST, VOICE, GLOBAL) */
/* (ACID FM 3c-2 task 4) ui_lzsave.c, SAVE's lens (LIBRARY: PROJECTS, TOOLS), gone: SAVE is ux_save.c's */
/* (ACID FM 3b task 8: ui_lzfx.c, FX's lens, gone: FX is ux_section.c's) */
#include "ui_lzedit.c"                               /* (0.8.5) EDIT's lens panels */
/* (ACID FM 3c-1 task 7) ui_lzpage.c gone: its last panel, GLO's SYSTEM (graph_system), with the page (GLO > MORE's SYSTEM rows) */
/* (0.9.17 .. ACID FM 3c-1 task 4) the SEQ pages' own marks over the roll (seq_overlay) gone: SEQ is ux_seq.c's */
static void draw_graph(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint16_t c = ACC;
    uint32_t sig;
    sig = graph_signature();
    if (!ui.force && sig == ui.graph_sig)
        return;
    {   /* (0.6.57) LOAD LITE / ECO: the panel's animation (the beings, the scope, LFO, the envelope's dot, a turn
         * redrawing a curve) at most every 50 / 70 ms (~20 / 14 frames a second, FULL: as fast as it changes), so the
         * main loop keeps its time for the keys, the knobs and the LCD. A new page, track or engine: at once */
        static uint32_t gr_ms, gr_key;
        uint32_t key = ui.page * 7u + song.sel * 131u + ui.home * 3u + TSEL->eng_req * 1031u;
        if (sys_load && !ui.force && key == gr_key && fm1_ms - gr_ms < (sys_load == LOAD_ECO ? 70u : 50u))
            return;                                  /* (graph_sig stays: drawn on a later frame) */
        gr_key = key;
        gr_ms = fm1_ms;
    }
    ui.graph_sig = sig;
    cv_begin(240, graph_h(), T_BG);
    if (lz_low())                                    /* (0.8.9) BOTTOM, HUD: no panel, the picture on the background */
        ;
    else if (lz_on())                                /* (0.8.5) a lens page: the mock-up's panel, 4 .. 236 (0.8.9 COLORS: */
        cv_rrect(4, 0, 232, (int32_t)graph_h(), 6, T_SURF, T_BG);   /* rounded, as the 0.6's) */
    else
        cv_rrect(3, 0, 234, graph_h(), 5, T_SURF, T_BG);   /* the panel (MENU > LARGE: the strip) */
    cv_bg = T_SURF;                                  /* (text drawn with cv_text lands on it) */
    if (drift_on())
        graph_drift();
    cv_oy = GOY;                                     /* graphs on a 100 px scale */
    if (strip_kind() != SK_NONE) {            /* MENU > LARGE: the strip */
        uint32_t k = strip_kind();
        cv_oy = k == SK_ADSR ? 3 : 0;
        graph_ht = LG_H_GRAPH - 6;
        if (k == SK_SCOPE) graph_scope(c);
#if FELUCCA_FM4
        else if (k == SK_ADSR) graph_adsr(t, c);
#endif
        else graph_title();
        graph_ht = 100;
    } else if (ui.confirm == CF_DICE_ALL) {          /* (0.8.5) GEN, HOME's lens's last level (gen.c) */
        cv_oy = 0;
        gen_panel();
#if FELUCCA_FM4
    } else if (lz_on() && lz_fam() == LZ_EDIT && (cv_oy = 0, graph_edit_lz(t, pg))) {   /* (0.8.5) EDIT's (ui_lzedit.c): DIGITAL's */
#endif
    } else {
        cv_oy = GOY;
        switch (pg->graph) {
#if FELUCCA_FM4
        case GR_ADSR:                                /* (DIGITAL's OP ENV: ENV's page is ux_section.c's) */
            graph_adsr(t, c);
            break;
#endif
        case GR_PATS:
            cv_oy = 0;
            graph_pats();
            break;
        default:
#if FELUCCA_FM4
            if ((pg->scope == SC_ENGINE || pg->id[0] == P_FM1_LEVEL) && trk_eng(t) == ENGI_DIGITAL)
                graph_fm(t, c);                      /* (OP LEVEL too: the levels on the chart) */
            else
#endif
            if (pg->scope == SC_ENGINE)              /* (ACID FM 3b task 5: the synths', FM6's and DRUM-X's EDIT are */
                cv_oy = 0;                           /* ux_section.c's; a page put by code meanwhile: blank, no creature) */
            else {                                   /* the FX, DEST, VOICE, GLOBAL pages: their graphs (else the scope) */
                cv_oy = 0;
                if (!graph_tech(t, pg))
                    graph_scope(c);
            }
            break;
        }
    }
    cv_oy = 0;
    if (toast_on())                                  /* (0.9.5) a toast: a small pill near the panel's foot, over the page,
                                                      * which goes on drawing and moving behind it (0.8.5 .. 0.9.4: the
                                                      * panel cleared, the message boxed alone in it) */
        toast_pill();
    else
        toast_at[0] = toast_at[2] = 0;
    cv_blit(0, graph_y());
}
