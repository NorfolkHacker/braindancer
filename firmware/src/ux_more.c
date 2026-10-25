/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's MORE list (ux_more.h): the rows a section's MORE page lists, which are shown, the pick and the scroll, the
 * panel's list. A row is shown when JIANT would show its setting: a track setting whose descriptor names it on this track's
 * engine (a "-" label or no range: not this engine's) and, held by a JIANT page, by one JIANT's rules show (ui.c page_jiant:
 * LOAD ECO's chorus / slicer, a kit's lanes, MORPH, RETRIG, VOICE's WORDS .. gone as there; a setting on no page is shown);
 * a row for one engine (eng) on that engine's tracks only; not a setting a page of the chain shows now (3b task 4: ANALOG's
 * VOICE page holds VOICE GLIDE PRIORITY SPREAD, so its MORE does not list them again); a heading when a row under it is.
 * sel and top are row indices: sel a shown setting / action (KNOB 1 steps over the headings and the hidden rows), top the
 * first row on the panel (UXM_ROWS rows a page), kept a chain each (uxm_at: EDIT's per kind of track). KNOB 2 writes through
 * ui_input.c param_knob, as a page's knob; KNOB 3 and 4 say what the knobs do (ui_input.c, UXM_K34). Included after
 * ux_home.c (ux_under), before ux_section.c (uxs_label: ux_section.h). */

/* the headings' words (ux_more.h UXM_H_*) */
static const char *const UXM_HEAD[UXM_H_N] = {"ARP", "VOICE", "FILTER", "TONE", "MOD", "MSEQ", "DELAY", "REVERB", "CHORUS",
                                              "SLICER", "WORDS"};
/* the actions (UXR_ACTION rows' ids): KNOB 2 turned up does it; ask: JIANT's dialog first (a CF_* kind, OCT+ yes / OCT- no:
 * its own handler does the work), 0 at once */
typedef struct { const char *name; uint8_t ask; } uxm_act_t;
static const uxm_act_t UXM_ACT[UXM_A_N] = {
    {"TYPE LYRIC", 0},                                  /* VOICE: the lyric typed on the NAME screen (was WORDS' OCT+ TYPE) */
};
#define UXM_GO "\x86"                                   /* (an action's value in the list: UF_HDR's arrow) */
static int uxm_act_ok(uint32_t a) { return a == UXM_A_LYRIC ? trk_engp(TSEL) == &ENG_FORMANT : a < UXM_A_N; }
static void uxm_act_do(uint32_t a)
{
    if (a >= UXM_A_N || !uxm_act_ok(a))
        return;
    if (UXM_ACT[a].ask) {
        confirm_open(UXM_ACT[a].ask, song.sel);
        return;
    }
    if (a == UXM_A_LYRIC)
        name_open(NK_LYRIC, song.sel);
}

/* row r names something real: a track setting (< P_COUNT) or a global (< G_COUNT) or a view (params.c param_ref: else
 * &t->p[id] / song.g[id] past their ends), an action, a heading. A row that does not is never shown (tests/ui_test.c checks
 * every list's) */
static int uxm_row_ok(const ux_row_t *r)
{
    uint32_t id = r->id;
    int view = id == ID_ALLTR || id == G_TRKS || id == ID_LMODE || id == ID_LPHS || id == G_LOADP;
    return r->kind == UXR_TRACK ? id < P_COUNT || view : r->kind == UXR_GLOBAL ? id < G_COUNT || view :
           r->kind == UXR_ACTION ? id < UXM_A_N : r->kind == UXR_HEAD && id < UXM_H_N;
}
/* a setting or action row shown on this track now (above) */
static int ux_row_shown(const ux_row_t *r)
{
    uint32_t i, k, held = 0, glo = r->kind == UXR_GLOBAL;
    if (!uxm_row_ok(r))
        return 0;
    if (r->eng && trk_eng(TSEL) != r->eng - 1u)       /* (one engine's meaning of a shared slot) */
        return 0;
    if (r->kind == UXR_ACTION)
        return uxm_act_ok(r->id);
    if (r->kind != UXR_TRACK && !glo)
        return 0;
    if (!glo) {
        const param_desc_t *d = r->id < P_COUNT ? track_desc(TSEL, r->id) : 0;
        if (r->id < P_COUNT && (!d || !d->label || d->label[0] == '-' || d->max <= d->min))
            return 0;
    }
    for (i = 0; i < NPAGES; i++) {
        const page_t *pg = &PAGES[i];
        if (pg->graph == GR_MORE || pg->scope == SC_STEP || pg->scope == SC_TRK || (pg->scope == SC_GLOBAL) != glo)
            continue;
        for (k = 0; k < 4u && pg->id[k] != r->id; k++)
            ;
        if (k < 4u) {
            if (page_jiant(i))                          /* (JIANT's rule for it: ui.c) */
                return 1;
            held = 1;
        }
    }
    return !held;
}
/* the settings on the pages shown now of the chain whose MORE list m is (a bit each, [global][id]): MORE does not list them
 * again (ANALOG's VOICE page holds VOICE GLIDE PRIORITY SPREAD, the others' FILTER the filter's TYPE). A list on no page: none */
static void uxm_paged(const ux_more_t *m, uint32_t got[2][8])
{
    uint8_t v[16];
    uint32_t i, k, n, sl, sec;
    for (i = 0; i < 16u; i++)
        got[i >> 3][i & 7u] = 0;
    for (i = 0; i < NPAGES && PAGES[i].more != m; i++)
        ;
    if (i >= NPAGES)
        return;
    sec = sec_of_fam(PAGES[i].fam);
    n = nav_list(sec, v);
    for (k = 0; k < n; k++) {
        uint32_t pg = nv_page(sec, v[k]);
        if (pg >= NPAGES || PAGES[pg].graph == GR_MORE)
            continue;
        for (sl = 0; sl < 4u; sl++) {
            uint32_t id = page_id(&PAGES[pg], sl);
            if (id < 256u)
                got[PAGES[pg].scope == SC_GLOBAL][id >> 5] |= 1u << (id & 31u);
        }
    }
}
/* the rows of m shown now, a bit each (a list holds 64 at most) */
static uint64_t uxm_mask(const ux_more_t *m)
{
    uint64_t sh = 0;
    uint32_t i, n = m->n < 64u ? m->n : 64u, h = 64u, pd[2][8];
    uxm_paged(m, pd);
    for (i = 0; i < n; i++) {
        const ux_row_t *r = &m->rows[i];
        if (r->kind == UXR_HEAD) {
            h = uxm_row_ok(r) ? i : 64u;
            continue;
        }
        if ((r->kind == UXR_TRACK || r->kind == UXR_GLOBAL) && r->id < 256u &&
            ((pd[r->kind == UXR_GLOBAL][r->id >> 5] >> (r->id & 31u)) & 1u))
            continue;                                   /* (on a page of the chain shown now) */
        if (ux_row_shown(r)) {
            sh |= 1ull << i;
            if (h < 64u)                                /* (its heading with it) */
                sh |= 1ull << h, h = 64u;
        }
    }
    return sh;
}
static uint32_t uxm_n(const ux_more_t *m) { return m->n < 64u ? m->n : 64u; }
static int uxm_on(uint64_t sh, uint32_t i) { return i < 64u && ((sh >> i) & 1u); }
static int uxm_pick_ok(const ux_more_t *m, uint64_t sh, uint32_t i) { return uxm_on(sh, i) && m->rows[i].kind != UXR_HEAD; }
static uint32_t uxm_vis(uint64_t sh, uint32_t i)        /* shown rows before row i */
{
    uint32_t v = 0, k;
    for (k = 0; k < i && k < 64u; k++)
        v += (uint32_t)((sh >> k) & 1u);
    return v;
}
static uint32_t uxm_nvis(uint64_t sh) { return uxm_vis(sh, 64u); }
static uint32_t uxm_row_of(const ux_more_t *m, uint64_t sh, uint32_t v)   /* the v-th shown row (none: n) */
{
    uint32_t i;
    for (i = 0; i < uxm_n(m); i++)
        if (uxm_on(sh, i) && !v--)
            return i;
    return uxm_n(m);
}
/* sel a shown setting (the nearest after it, else before; none: n), top a shown row with sel in sight (sel on the top
 * row: its heading above it when it has one), not past the last page */
static void uxm_fix(const ux_more_t *m, uint64_t sh, uint32_t *sel, uint32_t *top)
{
    uint32_t n = uxm_n(m), s = *sel, t = *top, nv = uxm_nvis(sh), mt = nv > UXM_ROWS ? nv - UXM_ROWS : 0u, vs, vt;
    if (s >= n || !uxm_pick_ok(m, sh, s)) {
        uint32_t k;
        for (k = s < n ? s : 0u; k < n && !uxm_pick_ok(m, sh, k); k++)
            ;
        if (k >= n)
            for (k = s < n ? s : n; k-- > 0u && !uxm_pick_ok(m, sh, k);)
                ;
        s = k < n ? k : n;
    }
    vt = t < n ? uxm_vis(sh, t) : 0u;
    if (vt > mt)
        vt = mt;
    if (s < n) {
        vs = uxm_vis(sh, s);
        if (vs < vt)
            vt = vs && m->rows[uxm_row_of(m, sh, vs - 1u)].kind == UXR_HEAD ? vs - 1u : vs;
        else if (vs >= vt + UXM_ROWS)
            vt = vs + 1u - UXM_ROWS;
    }
    *sel = s;
    *top = uxm_row_of(m, sh, vt);
    if (*top >= n)
        *top = 0;
}

static int ux_more_turn(const ux_more_t *m, uint32_t *sel, uint32_t *top, uint32_t knob, int32_t steps)
{
    uint64_t sh;
    uint32_t n;
    if (!m || !steps)
        return 0;
    sh = uxm_mask(m);
    n = uxm_n(m);
    uxm_fix(m, sh, sel, top);
    if (knob == 0u) {                                   /* KNOB 1: the next / previous setting, stopping at the ends */
        uint32_t s = *sel, k;
        int32_t a = steps < 0 ? -steps : steps;
        while (s < n && a-- > 0) {
            if (steps > 0)
                for (k = s + 1u; k < n && !uxm_pick_ok(m, sh, k); k++)
                    ;
            else
                for (k = s; k-- > 0u && !uxm_pick_ok(m, sh, k);)
                    ;
            if (k >= n)
                break;
            s = k;
        }
        *sel = s;
        uxm_fix(m, sh, sel, top);
        return 1;
    }
    if (knob == 1u) {                                   /* KNOB 2: the picked one set (as a page's knob), an action done */
        const ux_row_t *r = *sel < n ? &m->rows[*sel] : 0;
        if (!r)
            return 1;
        if (r->kind == UXR_TRACK)
            param_knob(TSEL, r->id, steps, 1u);
        else if (r->kind == UXR_GLOBAL)
            param_knob(0, r->id, steps, 1u);
        else if (r->kind == UXR_ACTION && steps > 0)
            uxm_act_do(r->id);
        return 1;
    }
    if (knob == UXM_SELECT) {                           /* SELECT: a row a detent; the pick dragged along to stay in sight */
        uint32_t nv = uxm_nvis(sh), mt = nv > UXM_ROWS ? nv - UXM_ROWS : 0u, vt = uxm_vis(sh, *top), vs, k;
        int32_t to;
        if (steps < 0 && vt == 0u)
            return 0;                                   /* (before the first row: the page before) */
        to = clamp((int32_t)vt + steps, 0, (int32_t)mt);
        *top = uxm_row_of(m, sh, (uint32_t)to);
        if (*sel < n) {
            vs = uxm_vis(sh, *sel);
            if (vs < (uint32_t)to)                      /* (above the panel: the first setting on it) */
                for (k = *top; k < n && !uxm_pick_ok(m, sh, k); k++)
                    ;
            else if (vs >= (uint32_t)to + UXM_ROWS)     /* (below it: the last one on it) */
                for (k = uxm_row_of(m, sh, (uint32_t)to + UXM_ROWS); k-- > 0u && !uxm_pick_ok(m, sh, k);)
                    ;
            else
                k = *sel;
            if (k < n)
                *sel = k;
        }
        return 1;
    }
    return 0;
}

static struct { uint8_t sel, top; } uxm_at[NAV_KEYS];  /* each chain's MORE (ui_lens.c nav_key: EDIT's a kind of track each,
                                                         * the synths' FM6's DRUM's): its pick and scroll */
static uint32_t uxm_key(void) { return nav_key(nav_sec()) % NAV_KEYS; }
static int ux_more_knob(uint32_t knob, int32_t steps)
{
    const page_t *pg = cur_page();
    uint32_t key = uxm_key(), s, t;
    int r;
    if (ui.home || pg->graph != GR_MORE || !pg->more)
        return 0;
    s = uxm_at[key].sel;
    t = uxm_at[key].top;
    r = ux_more_turn(pg->more, &s, &t, knob, steps);
    uxm_at[key].sel = (uint8_t)s;
    uxm_at[key].top = (uint8_t)t;
    return r;
}

/* row r's words: its name (plain, as its section names it: ux_section.c uxs_label), its value into val (unit: with its
 * unit, "420 ms"), its place 0..255 into *bar (0: none) */
static const char *uxm_row_text(const ux_row_t *r, char *val, uint32_t nv, int unit, uint8_t *bar)
{
    int16_t *vp;
    const param_desc_t *d;
    const char *u = "";
    val[0] = 0;
    *bar = 0;
    if (r->kind == UXR_HEAD)
        return r->id < UXM_H_N ? UXM_HEAD[r->id] : "";
    if (r->kind == UXR_ACTION) {
        str_cpy(val, UXM_GO, nv);
        return r->id < UXM_A_N ? UXM_ACT[r->id].name : "";
    }
    d = param_ref(r->kind == UXR_GLOBAL ? 0 : TSEL, r->id, &vp);
    if (!d || !vp)
        return "";
    {
        char b[16];
        param_format(d, *vp, b, &u);
        str_cpy(val, b, nv);
    }
    if (unit && u && u[0]) {
        str_cpy(val + str_len(val), " ", nv - str_len(val));
        str_cpy(val + str_len(val), u, nv - str_len(val));
    }
    if (d->max > d->min) {
        int32_t b = (int32_t)(((int32_t)*vp - d->min) * 255 / (d->max - d->min));
        b = clamp(b, 0, 255);
        *bar = (uint8_t)(d->fmt == F_PCTINV ? 255 - b : b);
    }
    return uxs_label(nav_sec(), r->kind == UXR_GLOBAL, r->id, d->label);
}

static void ux_more_draw(const ux_more_t *m, uint32_t sel, uint32_t top)
{
    uint64_t sh;
    uint32_t n, i, r = 0, nv;
    if (!m)
        return;
    sh = uxm_mask(m);
    n = uxm_n(m);
    uxm_fix(m, sh, &sel, &top);
    for (i = top; i < n && r < UXM_ROWS; i++) {
        const ux_row_t *rw = &m->rows[i];
        int32_t y = 3 + (int32_t)r * UXM_PITCH, ty = y + 4, vx;
        char val[24], nm[24];
        uint8_t bar;
        const char *name;
        int on = i == sel;
        uint16_t bg;
        if (!uxm_on(sh, i))
            continue;
        r++;
        name = uxm_row_text(rw, val, sizeof val, 1, &bar);
        if (rw->kind == UXR_HEAD) {                     /* a heading: VIOLET capitals, a rule after them */
            int32_t w;
            bg = ux_under(12, ty + 4);
            GFX_HOOK_CELL(8, y, 228, y + UXM_PITCH - 2);
            w = text_w(&UF_LBL, name);
            cv_text_on(12, ty + 2, &UF_LBL, name, UX_VIOLET, bg);
            if (12 + w + 6 < 226)
                cv_rect(12 + w + 6, ty + 5, 226 - (12 + w + 6), 1, UX_LINE);
            continue;
        }
        if (on) {                                       /* the pick: BG2, KNOB 1's colour at its edge, KNOB 2's on its value */
            cv_rect(8, y, 220, UXM_PITCH - 2, UX_BG2);
            cv_rect(8, y, 2, UXM_PITCH - 2, UX_ACID);
            bg = UX_BG2;
        } else
            bg = ux_under(16, ty + 4);
        GFX_HOOK_CELL(8, y, 228, y + UXM_PITCH - 2);
        vx = 222 - text_w(&UF_HDR, val);
        str_cpy(nm, name, sizeof nm);
        while (nm[0] && 16 + text_w(&UF_HDR, nm) > vx - 8)   /* (a long name gives way to the value) */
            nm[str_len(nm) - 1u] = 0;
        cv_text_on(16, ty, &UF_HDR, nm, on ? UX_TEXT : UX_MID, bg);
        if (val[0])
            cv_text_on(vx, ty, &UF_HDR, val, on ? UX_SLIDE : UX_TEXT, on ? bg : ux_under(vx, ty + 4));
        if (bar && !on)                                 /* (a hairline of its place under the row) */
            cv_rect(16, y + UXM_PITCH - 3, (int32_t)bar * 206 / 255, 1, ux_mix(UX_LINE, UX_SLIDE, 35));
    }
    nv = uxm_nvis(sh);
    if (nv > UXM_ROWS) {                                /* the scroll: where the panel is in the list */
        int32_t h = (int32_t)(UXM_ROWS * UXM_PITCH) - 4, th = h * (int32_t)UXM_ROWS / (int32_t)nv,
                ty = 3 + (h - th) * (int32_t)uxm_vis(sh, top) / (int32_t)(nv - UXM_ROWS);
        cv_rect(233, 3, 2, h, UX_LINE);
        cv_rect(233, ty, 2, th < 6 ? 6 : th, UX_MID);
    }
}
