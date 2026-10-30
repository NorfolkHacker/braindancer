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
static const char *const UXM_HEAD[UXM_H_N] = {"ARP", "VOICE", "FILTER", "TONE", "MOD", "STEPS", "DELAY", "REVERB", "CHORUS",
                                              "SLICER", "WORDS", "KIT", "OP ", "DRUM \xB7 ", "MORPH LFO", "PATCH", "LFO", "DISTORT",
                                              "AUTO PAN", "SLICE PITCH", "PUNCH LANE", "TRACK", "STEP ", "MUTATE", "SHAPE",
                                              "PLAYHEADS", "AUTOMATION", "LIVE", "SYSTEM", "TEMPO", "UNDO", "MIDI", "SOUND",
                                              "TOOLS", "SONG", "SCREEN", "LEDS", "USB", "ABOUT", "MASTER", "TAPE"};   /* (3b task 7: JIANT's MSEQ, the
                                                         * modulation sequence, its STEPS; task 8: FX's, PUNCH LANE JIANT's PUNCH
                                                         * page, the punch-in lane: PUNCH alone is MASTER's, the drums') */
/* heading h's words: OP and DRUM with the operator or the drum picked after them ("OP 3", "DRUM . SD") */
static const char *uxm_head(uint32_t h)
{
    static char b[16];
    if (h >= UXM_H_N)
        return "";
    if (h != UXM_H_OP && h != UXM_H_DRUM && h != UXM_H_STEP)
        return UXM_HEAD[h];
    str_cpy(b, UXM_HEAD[h], sizeof b);
    if (h == UXM_H_STEP)                                /* (3c-1 task 4) SEQ's: the step picked (ux_seq.c), "STEP 5" */
        fmt_int(b + str_len(b), (int32_t)uxq_step() + 1);
    else
        str_cpy(b + str_len(b), h == UXM_H_OP ? N_FO_OP[fop_op % 6u] : N_DX_LANE[dx_ui_lane & 7u], sizeof b - str_len(b));
    return b;
}
/* the actions (UXR_ACTION rows' ids): KNOB 2 turned up does it; ask: JIANT's dialog first (a CF_* kind, OCT+ yes / OCT- no:
 * its own handler does the work), 0 at once; (3c-2 task 2) arg: what it acts on (ux_more.h UXA_*: the dialog's target) */
typedef struct { const char *name; uint8_t ask, arg; } uxm_act_t;
static const uxm_act_t UXM_ACT[UXM_A_N] = {
    {"TYPE LYRIC", 0, UXA_SEL},                         /* VOICE: the lyric typed on the NAME screen (was WORDS' OCT+ TYPE) */
    {"CLEAR LANE", CF_CLEAR_PFX, UXA_SEL},              /* (3b task 8) FX: the punch-in lane emptied (was FX > PUNCH's CLEAR) */
    {"CLEAR", CF_CLEAR_MOTION, UXA_SEL},                /* (3c-1 task 4) SEQ: the track's automation and locks (was AUTOMATION's
                                                         * CLEAR, OCT+ then the same dialog) */
    {"UNDO", 0, UXA_SEL},                               /* (3c-1 task 7) GLO: the last big change back, again forward (the GLO
                                                         * layer's G4: ui_layer.c glo_undo) */
    {"CLEAR", 0, UXA_SEL},                              /* (3c-1 task 8) GLO: LEARN CLEAR (uxm_list_name; CLEAR on the knob, as
                                                         * MENU's card was), every learned CC forgotten (MENU > MIDI's until 0.5:
                                                         * no dialog, LEARN again brings one back) */
    {"LOAD SLOT", 0, UXA_USLOT},                        /* (3c-2 task 2) SAVE: the slot SOUND picks loaded (the inherited USER's
                                                         * LOAD: upreset.c up_ui; an empty slot said) */
    {"ERASE", CF_ERASE_USER, UXA_USLOT},                /* .. that slot erased, asking first (the inherited USER's ERASE; ERASE USER
                                                         * SLOT in the list: uxm_list_name) */
    {"DELETE", CF_DEL_ROW, UXA_ROW},                    /* (3c-2 task 3) SAVE: the row SONG picks deleted, asking first (the inherited
                                                         * TOOLS' DELETE ROW and ARRANGE's SECT left of A; DELETE ROW in the list:
                                                         * uxm_list_name); no row: NO ROW */
    {"CLEAR", CF_CLEAR_SONG, UXA_SEL},                  /* .. every row gone, asking first (the inherited TOOLS' CLEAR SONG; CLEAR
                                                         * SONG in the list); no rows: NOTHING TO CLEAR, as TOOLS */
    {"CLEAR", CF_CLEAR_SEQ, UXA_SEL},                   /* (3c-2 task 4) SAVE: the selected track's steps, asking first (the inherited
                                                         * TOOLS' CLEAR SEQ; CLEAR SEQ in the list); nothing: NOTHING TO CLEAR */
    {"INIT", CF_INIT_SOUND, UXA_SEL},                   /* .. its sound at its engine's defaults (TOOLS' INIT SOUND) */
    {"NEW", CF_INIT_SYS, UXA_SEL},                      /* .. everything as at power-on (TOOLS' NEW PROJECT) */
    {"CPU TEST", CF_CPU_TEST, UXA_SEL},                 /* .. the heaviest song (TOOLS' CPU TEST) */
};
/* (3c-2 task 2) action a's target (its dialog's: ui.confirm_trk) */
static uint32_t uxm_act_arg(uint32_t a)
{
    uint32_t g = a < UXM_A_N ? UXM_ACT[a].arg : UXA_SEL;
    return g == UXA_USLOT ? ui.uslot : g == UXA_ROW ? ui.song_row : song.sel;
}
#define UXM_GO "\x86"                                   /* (an action's value in the list: UF_HDR's arrow) */
#define UXM_GO_K "GO"                                   /* (.. on the knob row: the word; 3c-2 task 9: MENU's too, one source) */
#define UXM_CPU_SAY "CPU: A READOUT"                    /* (3c-1 task 7) KNOB 2 on GLO > MORE's CPU */
static int uxm_act_ok(uint32_t a) { return a == UXM_A_LYRIC ? trk_engp(TSEL) == &ENG_FORMANT : a < UXM_A_N; }
static void uxm_act_do(uint32_t a)
{
    if (a >= UXM_A_N || !uxm_act_ok(a))
        return;
    if (UXM_ACT[a].ask && chain_busy()) {               /* (as JIANT's pages: not while a song plays, ui_input.c act_do; 3c-2 task 4: */
        ui_message(UX_STOP);                            /* first, as the inherited TOOLS) */
        return;
    }
    if ((a == UXM_A_CLRSEQ && seq_is_empty(TSEL) && !motion_count(TSEL)) || (a == UXM_A_CLRSONG && !chain_config.count)) {
        ui_message("NOTHING TO CLEAR");                 /* (3c-2 task 4: as the inherited TOOLS' act_ready, nothing there) */
        return;
    }
    if (a == UXM_A_ERASE && (!up_used(ui.uslot) || transport_busy())) {   /* (3c-2 task 2: as the inherited USER's ERASE, the */
        up_ui(1u, ui.uslot);                            /* dialog only over a used slot, stopped; else up_ui says why: EMPTY SLOT, */
        return;                                         /* STOP TO SAVE) */
    }
    if (a == UXM_A_DELROW) {                            /* (3c-2 task 3) the row SONG shows picked (past the last: the last); */
        if (!chain_config.count) {                      /* no row: said */
            ui_message("NO ROW");
            return;
        }
        if (ui.song_row >= chain_config.count)
            ui.song_row = (uint8_t)(chain_config.count - 1u);
    }
    if (UXM_ACT[a].ask) {                               /* (then the dialog, OCT+ yes / OCT- no) */
        confirm_open(UXM_ACT[a].ask, uxm_act_arg(a));
        return;
    }
    if (a == UXM_A_ULOAD)                               /* (3c-2 task 2) the inherited USER's LOAD */
        up_ui(0u, ui.uslot);
    if (a == UXM_A_LYRIC)
        name_open(NK_LYRIC, song.sel);
    if (a == UXM_A_UNDO)
        glo_undo();
    if (a == UXM_A_LCLEAR)
        ui_message(ml_clear_all() ? "LEARN CLEARED" : "NOTHING LEARNED");
}

/* row r names something real: a track setting (< P_COUNT) or a global (< G_COUNT) or a view (params.c param_ref: else
 * &t->p[id] / song.g[id] past their ends), an action, a heading. A row that does not is never shown (tests/ui_test.c checks
 * every list's) */
static int uxm_row_ok(const ux_row_t *r)
{
    uint32_t id = r->id;
    int view = id == ID_ALLTR || id == G_TRKS || id == ID_LMODE || id == ID_LPHS || id == G_LOADP || id == ID_MSLOT;
    return r->kind == UXR_TRACK ? id < P_COUNT || view || fld_is(id) || sqv_is(id) :
           r->kind == UXR_GLOBAL ? id < G_COUNT || view || lvv_is(id) || mov_is(id) || svv_is(id) :
           r->kind == UXR_ACTION ? id < UXM_A_N : r->kind == UXR_MENU ? id < MI_COUNT : r->kind == UXR_HEAD && id < UXM_H_N;
}
/* (3b task 8) a global LOAD ECO takes out of the sound (fx.c fx_buses: no chorus; slicer.c slicer_on: no SLICER): its row not
 * shown under ECO, as JIANT hid the CHORUS and SLICER pages (ui.c page_jiant, 0.6.30) */
static int uxm_eco_out(uint32_t id) { return id == G_CRATE || id == G_CDEPTH || id == G_CSTER || (id >= G_SLMODE && id <= G_SLP15); }
/* a setting or action row shown on this track now (above) */
static int ux_row_shown(const ux_row_t *r)
{
    uint32_t i, k, held = 0, glo = r->kind == UXR_GLOBAL, seq = r >= UXM_SEQ.rows && r < UXM_SEQ.rows + UXM_SEQ.n;
    if (!uxm_row_ok(r))
        return 0;
    if (r->kind == UXR_MENU)                            /* (3c-2 task 5) MENU's: always (COLOR SKIN STYLE SCOPE: on no list) */
        return !menu_hidden(r->id);
    if (glo && sys_load == LOAD_ECO && uxm_eco_out(r->id))
        return 0;
    if (r->eng && trk_eng(TSEL) != r->eng - 1u)       /* (one engine's meaning of a shared slot) */
        return 0;
    if (r->kind == UXR_ACTION)
        return uxm_act_ok(r->id);
    if (r->kind == UXR_TRACK && sqv_is(r->id))          /* (3c-1 task 4) SEQ's views: by the step picked, the track (ux_seq.c) */
        return uxq_vshown(r->id);
    if (glo && (lvv_is(r->id) || mov_is(r->id) || svv_is(r->id)))   /* (3c-1 task 6) GLO's RAILS, (task 8) its MIDI views, (3c-2
                                                         * task 2) SAVE's SOUND views: always */
        return 1;
    if (r->kind != UXR_TRACK && !glo)
        return 0;
    if (!glo) {
        const param_desc_t *d = r->id < P_COUNT ? track_desc(TSEL, r->id) : 0;
        if (r->id < P_COUNT && (!d || !d->label || d->label[0] == '-' || d->max <= d->min))
            return 0;
    }
    for (i = 0; i < NPAGES; i++) {
        const page_t *pg = &PAGES[i];
        if (pg->graph == GR_MORE || pg->scope == SC_STEP || (pg->scope == SC_GLOBAL) != glo ||
            (seq && pg->fam != FAM_SEQ))                /* (3c-1 task 4: SEQ's by SEQ's pages alone: TRACK's and MUTATE's ids */
            continue;                                   /* are DIGITAL's operator slots too, core.h) */
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
/* the rows of m shown now, a bit each (a list holds UXM_MAXROWS at most) */
static void uxm_bit(uxm_bits_t *sh, uint32_t i) { sh->w[i >> 6] |= 1ull << (i & 63u); }
static uxm_bits_t uxm_mask(const ux_more_t *m)
{
    uxm_bits_t sh = {{0, 0}};
    uint32_t i, n = m->n < UXM_MAXROWS ? m->n : UXM_MAXROWS, h = UXM_MAXROWS, pd[2][8];
    uxm_paged(m, pd);
    for (i = 0; i < n; i++) {
        const ux_row_t *r = &m->rows[i];
        if (r->kind == UXR_HEAD) {
            h = uxm_row_ok(r) ? i : UXM_MAXROWS;
            continue;
        }
        if ((r->kind == UXR_TRACK || r->kind == UXR_GLOBAL) && r->id < 256u &&
            ((pd[r->kind == UXR_GLOBAL][r->id >> 5] >> (r->id & 31u)) & 1u))
            continue;                                   /* (on a page of the chain shown now) */
        if (ux_row_shown(r)) {
            uxm_bit(&sh, i);
            if (h < UXM_MAXROWS)                        /* (its heading with it) */
                uxm_bit(&sh, h), h = UXM_MAXROWS;
        }
    }
    return sh;
}
static uint32_t uxm_n(const ux_more_t *m) { return m->n < UXM_MAXROWS ? m->n : UXM_MAXROWS; }
/* (Braindancer 0.8) the rows shown folded to 32 bits for a change signature (ux_section.c): the high word turned a bit
 * further (rotated by one) before the fold, so rows i and i + 64 swapped (one shown, the other hidden) change it; a list of
 * 64 rows or less (w[1] 0) folds as before */
static uint32_t uxm_sig_fold(const uxm_bits_t *sh)
{
    uint64_t x = sh->w[0] ^ (sh->w[1] << 1 | sh->w[1] >> 63);
    return (uint32_t)(x ^ (x >> 32));
}
static int uxm_on(const uxm_bits_t *sh, uint32_t i) { return i < UXM_MAXROWS && ((sh->w[i >> 6] >> (i & 63u)) & 1u); }
static int uxm_pick_ok(const ux_more_t *m, const uxm_bits_t *sh, uint32_t i) { return uxm_on(sh, i) && m->rows[i].kind != UXR_HEAD; }
static uint32_t uxm_vis(const uxm_bits_t *sh, uint32_t i)       /* shown rows before row i */
{
    uint32_t v = 0, k;
    for (k = 0; k < i && k < UXM_MAXROWS; k++)
        v += (uint32_t)((sh->w[k >> 6] >> (k & 63u)) & 1u);
    return v;
}
static uint32_t uxm_nvis(const uxm_bits_t *sh) { return uxm_vis(sh, UXM_MAXROWS); }
static uint32_t uxm_row_of(const ux_more_t *m, const uxm_bits_t *sh, uint32_t v)   /* the v-th shown row (none: n) */
{
    uint32_t i;
    for (i = 0; i < uxm_n(m); i++)
        if (uxm_on(sh, i) && !v--)
            return i;
    return uxm_n(m);
}
/* (3b task 8) row s's heading (UXM_H_N: none, or s a heading itself) */
static uint32_t uxm_head_of(const ux_more_t *m, uint32_t s)
{
    if (s >= uxm_n(m) || m->rows[s].kind == UXR_HEAD)
        return UXM_H_N;
    while (s-- > 0u)
        if (m->rows[s].kind == UXR_HEAD)
            return m->rows[s].id;
    return UXM_H_N;
}
/* (3b task 7 fix) row s is under the STEPS heading (LFO's MORE: JIANT's MSEQ): the panel shows the sixteen steps' levels as
 * bars under the list (ux_more_draw), the list UXM_STEPS_ROWS rows */
#define UXM_STEPS_ROWS (UXM_ROWS - 3u)
static int uxm_steps_pic(const ux_more_t *m, uint32_t s) { return uxm_head_of(m, s) == UXM_H_MSEQ; }
static uint32_t uxm_rows(const ux_more_t *m, uint32_t s) { return uxm_steps_pic(m, s) ? UXM_STEPS_ROWS : UXM_ROWS; }   /* the
                                                         * rows on the panel with s picked */
/* sel a shown setting (the nearest after it, else before; none: n), top a shown row with sel in sight (sel on the top
 * row: its heading above it when it has one), not past the last page */
static void uxm_fix(const ux_more_t *m, const uxm_bits_t *sh, uint32_t *sel, uint32_t *top)
{
    uint32_t n = uxm_n(m), s = *sel, t = *top, nv = uxm_nvis(sh), mt, vs, vt, rows;
    if (s >= n || !uxm_pick_ok(m, sh, s)) {
        uint32_t k;
        for (k = s < n ? s : 0u; k < n && !uxm_pick_ok(m, sh, k); k++)
            ;
        if (k >= n)
            for (k = s < n ? s : n; k-- > 0u && !uxm_pick_ok(m, sh, k);)
                ;
        s = k < n ? k : n;
    }
    rows = uxm_rows(m, s);
    mt = nv > rows ? nv - rows : 0u;
    vt = t < n ? uxm_vis(sh, t) : 0u;
    if (vt > mt)
        vt = mt;
    if (s < n) {
        vs = uxm_vis(sh, s);
        if (vs < vt)
            vt = vs && m->rows[uxm_row_of(m, sh, vs - 1u)].kind == UXR_HEAD ? vs - 1u : vs;
        else if (vs >= vt + rows)
            vt = vs + 1u - rows;
    }
    *sel = s;
    *top = uxm_row_of(m, sh, vt);
    if (*top >= n)
        *top = 0;
}

static void uxn_set(uint32_t row, int32_t steps);       /* (3c-2 task 5: ux_menu.c, a MENU row's KNOB 2) */
static int ux_more_turn(const ux_more_t *m, uint32_t *sel, uint32_t *top, uint32_t knob, int32_t steps)
{
    uxm_bits_t sh;
    uint32_t n;
    if (!m || !steps)
        return 0;
    sh = uxm_mask(m);
    n = uxm_n(m);
    uxm_fix(m, &sh, sel, top);
    if (knob == 0u) {                                   /* KNOB 1: the next / previous setting, stopping at the ends */
        uint32_t s = *sel, k;
        int32_t a = steps < 0 ? -steps : steps;
        while (s < n && a-- > 0) {
            if (steps > 0)
                for (k = s + 1u; k < n && !uxm_pick_ok(m, &sh, k); k++)
                    ;
            else
                for (k = s; k-- > 0u && !uxm_pick_ok(m, &sh, k);)
                    ;
            if (k >= n)
                break;
            s = k;
        }
        *sel = s;
        uxm_fix(m, &sh, sel, top);
        return 1;
    }
    if (knob == 1u) {                                   /* KNOB 2: the picked one set (as a page's knob), an action done */
        const ux_row_t *r = *sel < n ? &m->rows[*sel] : 0;
        if (!r)
            return 1;
        if (r->kind == UXR_TRACK)
            param_knob(TSEL, r->id, steps, 1u);
        else if (r->kind == UXR_GLOBAL && r->id == G_INFO)   /* (3c-1 task 7) GLO's CPU: shown, not set (spec 0 rule 6: said) */
            ui_message(UXM_CPU_SAY);
        else if (r->kind == UXR_GLOBAL)
            param_knob(0, r->id, steps, 1u);
        else if (r->kind == UXR_ACTION && steps > 0)
            uxm_act_do(r->id);
        else if (r->kind == UXR_MENU)                   /* (3c-2 task 5) MENU's: as its pages' knobs, an action on a turn up */
            uxn_set(r->id, steps);
        return 1;
    }
    if (knob == UXM_SELECT) {                           /* SELECT: a row a detent; the pick dragged along to stay in sight */
        uint32_t nv = uxm_nvis(&sh), rows = uxm_rows(m, *sel), mt = nv > rows ? nv - rows : 0u, vt = uxm_vis(&sh, *top), vs, k;
        int32_t to;
        if (steps < 0 && vt == 0u)
            return 0;                                   /* (before the first row: the page before) */
        to = clamp((int32_t)vt + steps, 0, (int32_t)mt);
        *top = uxm_row_of(m, &sh, (uint32_t)to);
        if (*sel < n) {
            vs = uxm_vis(&sh, *sel);
            if (vs < (uint32_t)to)                      /* (above the panel: the first setting on it) */
                for (k = *top; k < n && !uxm_pick_ok(m, &sh, k); k++)
                    ;
            else if (vs >= (uint32_t)to + rows)         /* (below it: the last one on it) */
                for (k = uxm_row_of(m, &sh, (uint32_t)to + rows); k-- > 0u && !uxm_pick_ok(m, &sh, k);)
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

/* MORE's knob row (3b; 3c-2 task 5: MENU's too): KNOB 1 the pick, "3/12" (SETTING), KNOB 2 the row picked and its value (an action:
 * GO); the rest blank */
static const char *uxm_row_text(const ux_row_t *r, char *val, uint32_t nv, uint8_t *bar);   /* (below) */
static void uxs_marks(uint32_t m[(P_COUNT + 31u) / 32u]);   /* (ux_section.c) */
/* (3c-2 task 9) row r moves on its own: a track setting with automation or a lock on the step (uxs_marks' m[]); its KNOB 2 and
 * its place in the list marked as the knob row's (ux_widgets.c) */
static int uxm_marked(const ux_row_t *r, const uint32_t *m)
{
    int16_t *vp;
    uint32_t i;
    if (r->kind != UXR_TRACK || sqv_is(r->id) || !param_ref(TSEL, r->id, &vp) || !vp || vp < TSEL->p || vp >= TSEL->p + P_COUNT)
        return 0;
    i = (uint32_t)(vp - TSEL->p);
    return (int)((m[i / 32u] >> (i % 32u)) & 1u);
}
static void uxm_knobs(const ux_more_t *m, uint32_t s, uint32_t t, ux_knob_t k[4])
{
    uxm_bits_t sh = uxm_mask(m);
    uint32_t n = uxm_n(m), a = 0, b = 0, i;
    char v[24];
    uxm_fix(m, &sh, &s, &t);
    k[0].label = UXS_PICK;
    if (s >= n)
        return;
    for (i = 0; i < n; i++)
        if (uxm_pick_ok(m, &sh, i)) {
            b++;
            if (i <= s)
                a = b;
        }
    fmt_int(k[0].value, (int32_t)a);
    str_cpy(k[0].value + str_len(k[0].value), "/", sizeof k[0].value - str_len(k[0].value));
    fmt_int(k[0].value + str_len(k[0].value), (int32_t)b);
    k[0].bar = (uint8_t)(b > 1u ? (a - 1u) * 255u / (b - 1u) : 255u);
    k[1].label = uxm_row_text(&m->rows[s], v, sizeof v, &k[1].bar);
    if (m->rows[s].kind == UXR_ACTION || (m->rows[s].kind == UXR_MENU && !menu_valued(m->rows[s].id)))
        str_cpy(v, UXM_GO_K, sizeof v);
    str_cpy(k[1].value, v, sizeof k[1].value);
}

/* (3b task 9, the user: every row) a MORE page as the step locks' page (ux_section.h): its section's pick, KNOB 1 moving it, which
 * rows a lock holds (a track setting motion.c keeps per step), the cards a step held shows (ui_draw.c) */
static uint32_t uxm_key_of(const page_t *pg) { return nav_key(sec_of_fam(pg->fam)) % NAV_KEYS; }
static const ux_row_t *uxm_lock_row(const page_t *pg)
{
    uint32_t key, s, t;
    if (!pg || pg->graph != GR_MORE || !pg->more)
        return 0;
    key = uxm_key_of(pg);
    s = uxm_at[key].sel;
    t = uxm_at[key].top;
    { uxm_bits_t b = uxm_mask(pg->more); uxm_fix(pg->more, &b, &s, &t); }
    return s < uxm_n(pg->more) ? &pg->more->rows[s] : 0;
}
static void uxm_lock_pick(const page_t *pg, int32_t steps)
{
    uint32_t key, s, t;
    if (!pg || pg->graph != GR_MORE || !pg->more)
        return;
    key = uxm_key_of(pg);
    s = uxm_at[key].sel;
    t = uxm_at[key].top;
    (void)ux_more_turn(pg->more, &s, &t, 0u, steps);
    uxm_at[key].sel = (uint8_t)s;
    uxm_at[key].top = (uint8_t)t;
}
static const char *const UXM_WHY[UXM_NL_N] = {"", "GLOBAL: NOT PER STEP", "PATCH: NOT PER STEP", "KIT SOUND: NO STEP LOCK",
                                              "ACTION: NOT PER STEP", ": NOT PER STEP"};   /* (UXM_NL_SET: after the row's name) */
static uint32_t uxm_lock_why(const ux_row_t *r)
{
    if (!r)
        return UXM_NL_SET;
    if (r->kind == UXR_GLOBAL)
        return UXM_NL_GLOBAL;
    if (r->kind == UXR_ACTION)
        return UXM_NL_ACTION;
    if (r->kind == UXR_TRACK && r->id >= ID_FO_OP && r->id < ID_FO_END)
        return UXM_NL_PATCH;
    if (r->kind == UXR_TRACK && fld_is(r->id))
        return UXM_NL_KIT;
    return r->kind == UXR_TRACK && r->id < P_COUNT && motion_param(r->id) ? UXM_LOCKS : UXM_NL_SET;
}
static void uxm_lock_say(const ux_row_t *r)
{
    uint32_t why = uxm_lock_why(r);
    char v[24];
    uint8_t bar;
    if (why != UXM_LOCKS)
        ui_say(why == UXM_NL_SET && r ? uxm_row_text(r, v, sizeof v, &bar) : "", UXM_WHY[why]);
}
static const char *uxm_lock_card(const page_t *pg, uint32_t c, uint32_t step, char *val, uint32_t nv, int *on, int32_t *ratio)
{
    const ux_row_t *r = uxm_lock_row(pg);
    const char *nm;
    uint8_t bar;
    val[0] = 0;
    *on = 0;
    *ratio = -1;
    if (!r || c > 1u)
        return "";
    if (c == 0u) {                                      /* KNOB 1: the pick, "3/12" (as MORE's knob row) */
        uxm_bits_t m = uxm_mask(pg->more);
        uint32_t i, a = 0, b = 0;
        for (i = 0; i < uxm_n(pg->more); i++)
            if (uxm_pick_ok(pg->more, &m, i)) {
                b++;
                if (&pg->more->rows[i] <= r)
                    a = b;
            }
        fmt_int(val, (int32_t)a);
        str_cpy(val + str_len(val), "/", nv - str_len(val));
        fmt_int(val + str_len(val), (int32_t)b);
        *on = 1;
        return UXS_PICK;
    }
    nm = uxm_row_text(r, val, nv, &bar);                /* KNOB 2: the row, the sound's value (DIM) or the step's lock */
    if (!uxm_lock_why(r)) {
        const param_desc_t *d = track_desc(TSEL, r->id);
        int16_t v;
        *on = motion_lock_get(TSEL, step, r->id, &v);
        if (!*on)
            v = motion_base_value(TSEL, r->id);
        uxs_vtext(d, v, 0, r->id, val, nv);
        if (d->max > d->min)
            *ratio = clamp(((int32_t)v - d->min) * 1000 / (d->max - d->min), 0, 1000);
    }
    return nm;
}

/* row r's words: its name (plain, as its section names it: ux_section.c uxs_label), its value into val (as the knob row
 * shows it, its unit with it: "420MS"), its place 0..255 into *bar (0: none) */
static const char *uxm_row_text(const ux_row_t *r, char *val, uint32_t nv, uint8_t *bar)
{
    int16_t *vp;
    const param_desc_t *d;
    val[0] = 0;
    *bar = 0;
    if (r->kind == UXR_HEAD)
        return uxm_head(r->id);
    if (r->kind == UXR_ACTION) {
        str_cpy(val, UXM_GO, nv);
        return r->id < UXM_A_N ? UXM_ACT[r->id].name : "";
    }
    if (r->kind == UXR_MENU) {                          /* (3c-2 task 5) MENU's: its value's name, its place; its knob label */
        int32_t q = menu_ratio(r->id);
        str_cpy(val, menu_valued(r->id) ? menu_vname(r->id, menu_get(r->id)) : UXM_GO, nv);
        *bar = (uint8_t)(q < 0 ? 0 : q * 255 / 1000);
        return mi_short(r->id);
    }
    if (r->kind == UXR_TRACK && sqv_is(r->id))          /* (3c-1 task 4) SEQ's views: their words (ux_seq.c) */
        return uxq_vtext(r->id, val, nv, bar);
    if (r->kind == UXR_GLOBAL && lvv_is(r->id))         /* (3c-1 task 6) GLO's RAILS: their words (ux_glo.c) */
        return uxg_vtext(r->id, val, nv, bar);
    if (r->kind == UXR_GLOBAL && r->id == G_INFO)       /* (3c-1 task 7) GLO's CPU: the load now, as JIANT's SYSTEM card read it */
        return uxg_cpu_text(val, nv, bar);
    d = param_ref(r->kind == UXR_GLOBAL ? 0 : TSEL, r->id, &vp);
    if (!d || !vp)
        return "";
    uxs_vtext(d, *vp, r->kind == UXR_GLOBAL, r->id, val, nv);   /* (the knob row's form, units too: ux_section.c) */
    if (d->max > d->min) {
        int32_t b = (int32_t)(((int32_t)*vp - d->min) * 255 / (d->max - d->min));
        b = clamp(b, 0, 255);
        *bar = (uint8_t)(d->fmt == F_PCTINV ? 255 - b : b);
    }
    return uxs_label(nav_sec(), r->kind == UXR_GLOBAL, r->id, d->label);
}

/* (3c-1 task 8) row r's name in the list and the status line, where the knob row's (name, what uxm_row_text gave: it fits a
 * knob's column) is short for it: GLO's KEYBOARD, KEYBOARD PLAYS SELECTED in full; its CLEAR, LEARN CLEAR; (3c-2 task 2) SAVE's
 * ERASE, ERASE USER SLOT; (task 3) its DELETE, DELETE ROW; its CLEAR, CLEAR SONG; (task 4) CLEAR SEQ, INIT SOUND, NEW PROJECT */
static const char *uxm_list_name(const ux_row_t *r, const char *name)
{
    if (r->kind == UXR_MENU)                            /* (3c-2 task 5) MENU's: its full name (the knob's: mi_short) */
        return r->id < MI_COUNT ? MI_NAME[r->id] : name;
    if (r->kind == UXR_ACTION)
        return r->id == UXM_A_LCLEAR ? UXM_LCLEAR : r->id == UXM_A_ERASE ? UXM_ERASE_U : r->id == UXM_A_DELROW ? UXM_DELROW :
               r->id == UXM_A_CLRSONG ? UXM_CLRSONG : r->id == UXM_A_CLRSEQ ? UXM_CLRSEQ : r->id == UXM_A_INITSND ? UXM_INITSND :
               r->id == UXM_A_NEWPROJ ? UXM_NEWPROJ : name;
    return r->kind == UXR_GLOBAL && r->id == G_MRSEL ? UXM_KBD_SEL : name;
}

/* row r's place in its range: a short meter (UXM_BAR_W x 3) in a column of its own left of the values (ending UXM_BAR_X1),
 * on the text's middle row y: the track UX_LINE, the fill KNOB 2's colour on the pick (else dim); a range either side of 0
 * (min < 0 < max) filled from the middle, 0 a tick there; none for a list of words (an enum). (3b task 6 fix: a line under
 * the name, and then one under the value, read as an underline, a link) */
#define UXM_BAR_W 24
#define UXM_BAR_X1 168
static int uxm_bar(const ux_row_t *r, int on, int32_t y, int draw)   /* 1: the row has one */
{
    int16_t *vp;
    const param_desc_t *d = r->kind == UXR_TRACK || r->kind == UXR_GLOBAL ? param_ref(r->kind == UXR_GLOBAL ? 0 : TSEL, r->id, &vp) : 0;
    int32_t x0 = UXM_BAR_X1 - UXM_BAR_W, b, f;
    uint16_t c = on ? UX_SLIDE : ux_mix(UX_LINE, UX_SLIDE, 55);
    if (!d || !vp || d->max <= d->min || d->fmt == F_ENUM)
        return 0;
    if (!draw)
        return 1;
    b = clamp(((int32_t)*vp - d->min) * UXM_BAR_W / (d->max - d->min), 0, UXM_BAR_W);
    if (d->fmt == F_PCTINV)
        b = UXM_BAR_W - b;
    cv_rect(x0, y - 1, UXM_BAR_W, 3, on ? ux_mix(UX_BG2, UX_LINE, 80) : UX_LINE);
    if (d->min < 0 && d->max > 0) {                     /* (bipolar: from the middle, 0 a tick) */
        int32_t m = (int32_t)(-d->min) * UXM_BAR_W / (d->max - d->min);
        f = b < m ? b : m;
        if (b != m)
            cv_rect(x0 + f, y - 1, b > m ? b - m : m - b, 3, c);
        cv_rect(x0 + m, y - 2, 1, 5, on ? UX_TEXT : UX_MID);
    } else if (b)
        cv_rect(x0, y - 1, b, 3, c);
    return 1;
}
/* (3b task 7 fix) the STEPS' picture under the list: the sixteen levels as bars (KNOB 2's colour the step picked, those past
 * LENGTH dim), LENGTH's end a line in ACCENT */
static void uxm_steps_draw(const ux_more_t *m, uint32_t sel)
{
    int32_t y0 = 3 + (int32_t)(UXM_STEPS_ROWS * UXM_PITCH) + 4, h = 44, len = clamp(TSEL->p[P_MSLEN], 1, 16), i, x;
    uint32_t id = m->rows[sel].id;
    cv_rect(16, y0 + h, 208, 1, UX_LINE);
    for (i = 0; i < 16; i++) {
        int32_t lv = clamp(TSEL->p[P_MS0 + (uint32_t)i], 0, 127), bh = 1 + lv * (h - 1) / 127, lit = id == P_MS0 + (uint32_t)i;
        uint16_t c = lit ? UX_SLIDE : i < len ? ux_mix(UX_BG, UX_ACID, 70) : ux_mix(UX_BG, UX_LINE, 70);
        x = 16 + i * 13;
        cv_rect(x, y0 + h - bh, 11, bh, c);
        if (lit)
            cv_rect(x, y0 + h + 2, 11, 2, UX_SLIDE);
    }
    x = 16 + len * 13 - 2;                              /* (LENGTH: where the steps go round) */
    cv_rect(x > 222 ? 222 : x, y0 - 2, 1, h + 6, UX_ACCENT);
}
static void ux_more_draw(const ux_more_t *m, uint32_t sel, uint32_t top)
{
    uxm_bits_t sh;
    uint32_t n, i, r = 0, nv, rows, mm[(P_COUNT + 31u) / 32u];
    if (!m)
        return;
    sh = uxm_mask(m);
    n = uxm_n(m);
    uxm_fix(m, &sh, &sel, &top);
    rows = uxm_rows(m, sel);
    uxs_marks(mm);
    if (rows != UXM_ROWS)
        uxm_steps_draw(m, sel);
    for (i = top; i < n && r < rows; i++) {
        const ux_row_t *rw = &m->rows[i];
        int32_t y = 3 + (int32_t)r * UXM_PITCH, ty = y + 4, vx;
        char val[24], nm[24];
        uint8_t bar;
        const char *name;
        int on = i == sel;
        uint16_t bg;
        if (!uxm_on(&sh, i))
            continue;
        r++;
        name = uxm_list_name(rw, uxm_row_text(rw, val, sizeof val, &bar));
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
        while (nm[0] && 16 + text_w(&UF_HDR, nm) > (uxm_bar(rw, on, 0, 0) ? UXM_BAR_X1 - UXM_BAR_W : vx) - 8)   /* (a long
                                                         * name gives way to the value, its meter) */
            nm[str_len(nm) - 1u] = 0;
        cv_text_on(16, ty, &UF_HDR, nm, on ? UX_TEXT : UX_MID, bg);
        if (val[0])
            cv_text_on(vx, ty, &UF_HDR, val, on ? UX_SLIDE : UX_TEXT, on ? bg : ux_under(vx, ty + 4));
        (void)uxm_bar(rw, on, ty + UF_HDR_CAP_Y + UF_HDR_CAP_H / 2, 1);   /* (its place: a meter beside the value) */
        if (uxm_marked(rw, mm))                         /* (3c-2 task 9) it moves on its own: the row's top right end lit, as */
            cv_rect(228 - UX_MARK_W, y, UX_MARK_W, UX_MARK_H, UX_TEXT);   /* the knob row's rule (ux_widgets.c) */
    }
    nv = uxm_nvis(&sh);
    if (nv > rows) {                                    /* the scroll: where the panel is in the list */
        int32_t h = (int32_t)(rows * UXM_PITCH) - 4, th = h * (int32_t)rows / (int32_t)nv,
                ty = 3 + (h - th) * (int32_t)uxm_vis(&sh, top) / (int32_t)(nv - rows);
        cv_rect(233, 3, 2, h, UX_LINE);
        cv_rect(233, ty, 2, th < 6 ? 6 : th, UX_MID);
    }
}
