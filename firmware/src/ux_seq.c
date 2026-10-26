/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's SEQ pages on the screen (3c-1 spec 2.1, parent spec 4.4; ux_seq.h), drawn through ux_section.c's page (header, knob
 * row, panel, status line, footer) as the 3b sections are:
 *   STEPS    top: the roll of the 16 steps shown (ui.bank's), pitch up and time across, a column a step over the strip's cells:
 *            notes ACID, accents ACCENT, slides SLIDE joined to the next note, a TIE carrying its note on, the picked step
 *            outlined TEXT, the playing step a light column, a step lock a mark at its foot. A DRUM track: the eight lanes
 *            over the same columns, a hit in its lane's colour (the blend, BD .. CB), an accent brighter, the lane picked
 *            underlaid. Under it the strip (uxq_strip) and the picked step spelled out ("5 . G2 . ACC").
 *            Knobs STEP . NOTE . ACC/SLD . GATE (the inherited STEP page's STEP, NOTE, FLAG, TIME: ui_input.c step_edit by the
 *            PAGES row's ids); a DRUM track's STEP . LANE . HIT . ACC (ui_input.c grid_edit). A step held (a lock): the
 *            knobs show what they lock, as the inherited card did (task 4 restyles it).
 *            32 / 64 steps: SELECT on the strip's last cell the next 16, on its first the 16 before (uxq_select); the header
 *            says which 16 ("2/4"), before the transport.
 * The roll is redrawn when what it shows changes (ux_section.c's panel signature: uxq_sig), the playhead as it moves at most
 * every UX_TRK_MS (UX_TRK_SLOW_MS under load): one panel a played step at most, none while nothing changes. Included after
 * ux_more.c (uxm_lock_card), before ux_section.c (uxs_label, uxs_vtext: ux_section.h). */

#define UXQ_RY0 6                /* the roll / grid: its top row and height in the panel (spec: the top ~96 px) */
#define UXQ_RH 92
#define UXQ_STRIP_Y 104          /* the strip's row (UX_STRIP_H tall) */
#define UXQ_SAY_Y 128            /* the picked step spelled out (UF_HDR) */
#define UXQ_X(i) (UX_STRIP_X + (int32_t)(i) * UX_STRIP_PITCH)   /* cell / column i's left, UX_STRIP_W wide */

static const char *const UXQ_FLAG[4] = {"OFF", "ACC", "SLD", "BOTH"};   /* ACC/SLD's values (the inherited FLAG's - ACC SLD A+S) */
static const char *const UXQ_FLAG_SAY[4] = {"ACCENT AND SLIDE OFF", "ACCENT ON", "SLIDE ON", "ACCENT AND SLIDE ON"};
static const char *const UXQ_TIME[3] = {"NOTE", "TIE", "REST"};          /* GATE's (a NOTE one step long; longer: its steps) */
static const char *const UXQ_HIT[LM_N + 1] = {"OFF", "ON", "X2", "X4", "X8", "SHORT", "GHOST", "FLAM"};   /* (ui.c GRID_HIT's) */
/* the words the knob row, the status line and the spelled step are made of (one table: tests/ux_msg_scan.py checks each has its
 * glyphs in UF_LBL and UF_HDR); the knob labels, STEPS' and the grid's */
static const char *const UXQ_WORD[] = {"STEP ", " \xB7 ", " STEPS", "ACCENT ", "ON", "OFF", "ACC", "SLD", "--", "+", "/", " "};
enum { UXQ_W_STEP, UXQ_W_DOT, UXQ_W_STEPS, UXQ_W_ACCENT, UXQ_W_ON, UXQ_W_OFF, UXQ_W_ACC, UXQ_W_SLD, UXQ_W_NONE, UXQ_W_PLUS,
       UXQ_W_SLASH, UXQ_W_SP };
static const char *const UXQ_LABEL[7] = {"STEP", "NOTE", "ACC/SLD", "GATE", "LANE", "HIT", "ACC"};
enum { UXQ_L_STEP, UXQ_L_NOTE, UXQ_L_FLAG, UXQ_L_GATE, UXQ_L_LANE, UXQ_L_HIT, UXQ_L_ACC };
static int32_t uxq_ph_drawn = -1;                       /* the playing cell drawn (-1: none), when (ms): ux_section_draw's pace */
static uint32_t uxq_ph_ms;

static int uxq_owns(void)
{
    const page_t *pg;
    if (ui.home || ui.page >= NPAGES)
        return 0;
    pg = &PAGES[ui.page];
    return pg->fam == FAM_SEQ && pg->scope == SC_STEP && pg->graph == GR_ROLL && pg->id[0] == 0u;   /* (STEP: ids 0 1 3 2;
                                                         * NOTE's 4 ..) */
}
static uint32_t uxq_len(void) { return (uint32_t)clamp(TSEL->p[P_SLEN], 1, NSTEP); }
static uint32_t uxq_step(void) { uint32_t len = uxq_len(); return ui.cursor < len ? ui.cursor : len - 1u; }
static uint32_t uxq_first(void) { return (uxq_step() / 16u) * 16u; }   /* the first step of the 16 shown (ui.bank's) */
static int32_t uxq_ph(void)                             /* the playing cell of the 16 shown (-1: none) */
{
    const track_t *t = TSEL;
    uint32_t s = seq_src(t, t->seq_idx), f = uxq_first();
    return song.playing && s < uxq_len() && s >= f && s < f + 16u ? (int32_t)(s - f) : -1;
}
static void uxq_cat(char *b, uint32_t n, const char *s) { str_cpy(b + str_len(b), s, n - str_len(b)); }
static void uxq_num(char *b, uint32_t n, int32_t v) { char d[8]; fmt_int(d, v); uxq_cat(b, n, d); }
static uint32_t uxq_flag(const step_t *st) { return (st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u); }
static void uxq_note(const step_t *st, char *b, uint32_t n)   /* the step's note ("G2"; a chord "G2+2"; none "--") */
{
    char nb[8];
    b[0] = 0;
    if (st->time != ST_NOTE || !st->n) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_NONE]);
        return;
    }
    note_name(nb, st->note[0]);
    uxq_cat(b, n, nb);
    if (st->n > 1u) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_PLUS]);
        uxq_num(b, n, (int32_t)st->n - 1);
    }
}
static uint32_t uxq_gate(const step_t *st, uint32_t s)   /* GATE: a NOTE's length in steps (1: one), 0 a TIE or a REST */
{
    return st->time == ST_NOTE && st->n ? note_len(TSEL, s, uxq_len()) : 0u;
}
static void uxq_gate_txt(const step_t *st, uint32_t s, char *b, uint32_t n, int full)   /* "NOTE" "TIE" "REST" "3" (full: "3 STEPS") */
{
    uint32_t g = uxq_gate(st, s);
    b[0] = 0;
    if (g > 1u) {
        uxq_num(b, n, (int32_t)g);
        if (full)
            uxq_cat(b, n, UXQ_WORD[UXQ_W_STEPS]);
    } else
        uxq_cat(b, n, UXQ_TIME[st->time == ST_NOTE ? 0u : st->time == ST_TIE ? 1u : 2u]);
}
static void uxq_stepn(char *b, uint32_t n, int len)     /* "5/16" (len: with LEN) */
{
    b[0] = 0;
    uxq_num(b, n, (int32_t)uxq_step() + 1);
    if (len) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SLASH]);
        uxq_num(b, n, (int32_t)uxq_len());
    }
}
/* the picked step spelled out: "5 . G2 . ACC" (a chord G2+2, a TIE / REST so, SLD, a long note's steps); a DRUM track's
 * "5 . SNARE ON . ACC" (the lane picked, its hit) */
static __attribute__((noinline)) void uxq_spell(char *b, uint32_t n)
{
    uint32_t s = uxq_step(), f;
    const step_t *st = &TSEL->step[s];
    char v[16];
    uxq_stepn(b, n, 0);
    uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
    if (drum_track(TSEL)) {
        uint32_t l = ui.lane % NLANE;
        uxq_cat(b, n, drum_lane_name(TSEL, l));
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SP]);
        uxq_cat(b, n, UXQ_HIT[grid_hit_of(st, l)]);
        if ((step_accents(st) >> l) & 1u) {
            uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
            uxq_cat(b, n, UXQ_WORD[UXQ_W_ACC]);
        }
        return;
    }
    if (st->time != ST_NOTE || !st->n) {
        uxq_cat(b, n, UXQ_TIME[st->time == ST_TIE ? 1u : 2u]);
        return;
    }
    uxq_note(st, v, sizeof v);
    uxq_cat(b, n, v);
    f = uxq_flag(st);
    if (f & 1u) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_ACC]);
    }
    if (f & 2u) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SLD]);
    }
    if (uxq_gate(st, s) > 1u) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
        uxq_gate_txt(st, s, v, sizeof v, 1);
        uxq_cat(b, n, v);
    }
}
/* the header's right: which 16 of the pattern ("2/4 . "), then the transport (16 steps or fewer: the transport alone) */
static void uxq_head(char *b, uint32_t n)
{
    uint32_t pages = (uxq_len() + 15u) / 16u;
    char tr[16];
    b[0] = 0;
    if (pages > 1u) {
        uxq_num(b, n, (int32_t)(uxq_step() / 16u) + 1);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SLASH]);
        uxq_num(b, n, (int32_t)pages);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
    }
    ux_transport(tr, sizeof tr);
    uxq_cat(b, n, tr);
}
static int uxq_select(int32_t s)
{
    uint32_t c = uxq_step(), len = uxq_len();
    if (!uxq_owns() || !s)
        return 0;
    if (s > 0 && c % 16u == 15u && c + 1u < len) {     /* (the last cell: the next 16, from its first) */
        cursor_set((int32_t)c + 1);
        return 1;
    }
    if (s < 0 && c % 16u == 0u && c >= 16u) {          /* (the first: the 16 before, on its last) */
        cursor_set((int32_t)c - 1);
        return 1;
    }
    return 0;
}

/* the knob row: STEP NOTE ACC/SLD GATE (a DRUM track STEP LANE HIT ACC); a step held: what KNOB 1..4 lock (ui_input.c lock_turn:
 * the sound page's four, a MORE list's pick and row), a value not locked dim (the inherited card's) */
static void uxq_lock_knobs(ux_knob_t k[4])
{
    uint64_t held = lock_held();
    const page_t *lp = lock_src();
    uint32_t st = 0, c;
    while (st < NSTEP && !((held >> st) & 1u))
        st++;                                           /* (the first step held: its values) */
    for (c = 0; c < 4u; c++) {
        int on = 0;
        if (lp && lp->graph == GR_MORE) {
            int32_t r;
            char v[24];
            k[c].label = uxm_lock_card(lp, c, st, v, sizeof v, &on, &r);
            str_cpy(k[c].value, v, sizeof k[c].value);
            k[c].bar = (uint8_t)(r >= 0 ? r * 255 / 1000 : 0);
        } else {
            uint32_t id = lock_id(c);
            const param_desc_t *d;
            int16_t v;
            if (id == 0xFFu)
                continue;
            d = track_desc(TSEL, id);
            on = motion_lock_get(TSEL, st, id, &v);
            if (!on)
                v = motion_base_value(TSEL, id);        /* (not locked: the sound's own, dim) */
            k[c].label = uxs_label(sec_of_fam(lp ? lp->fam : FAM_SEQ), 0, id, d->label);
            uxs_vtext(d, v, 0, id, k[c].value, sizeof k[c].value);
            if (d->max > d->min)
                k[c].bar = (uint8_t)clamp(((int32_t)v - d->min) * 255 / (d->max - d->min), 0, 255);
        }
        k[c].off = (uint8_t)!on;
    }
}
static __attribute__((noinline)) void uxq_knobs(const page_t *pg, ux_knob_t k[4])
{
    uint32_t s = uxq_step(), len = uxq_len();
    const step_t *st = &TSEL->step[s];
    (void)pg;
    if (lock_held()) {
        uxq_lock_knobs(k);
        return;
    }
    k[0].label = UXQ_LABEL[UXQ_L_STEP];
    uxq_stepn(k[0].value, sizeof k[0].value, 1);
    k[0].bar = (uint8_t)(len > 1u ? s * 255u / (len - 1u) : 255u);
    if (drum_track(TSEL)) {
        uint32_t l = ui.lane % NLANE, h = grid_hit_of(st, l), a = (step_accents(st) >> l) & 1u;
        k[1].label = UXQ_LABEL[UXQ_L_LANE];
        str_cpy(k[1].value, drum_lane_name(TSEL, l), sizeof k[1].value);
        k[1].bar = (uint8_t)(l * 255u / (NLANE - 1u));
        k[2].label = UXQ_LABEL[UXQ_L_HIT];
        str_cpy(k[2].value, UXQ_HIT[h], sizeof k[2].value);
        k[2].bar = (uint8_t)(h * 255u / LM_N);
        k[2].off = (uint8_t)!h;
        k[3].label = UXQ_LABEL[UXQ_L_ACC];
        str_cpy(k[3].value, UXQ_WORD[a ? UXQ_W_ON : UXQ_W_OFF], sizeof k[3].value);
        k[3].bar = (uint8_t)(a ? 255u : 0u);
        k[3].off = (uint8_t)!a;
        return;
    }
    {
        uint32_t f = uxq_flag(st), g = uxq_gate(st, s);
        k[1].label = UXQ_LABEL[UXQ_L_NOTE];
        uxq_note(st, k[1].value, sizeof k[1].value);
        if (text_w(&UF_VAL, k[1].value) > UX_COL_W - 4) {   /* (a chord's "+2" past the column, "A#-1+2": the note; the status
                                                         * line says it whole) */
            char *p = k[1].value;
            while (*p && *p != '+')
                p++;
            *p = 0;
        }
        k[1].bar = (uint8_t)(st->time == ST_NOTE && st->n ? st->note[0] * 255u / 127u : 0u);
        k[1].off = (uint8_t)!(st->time == ST_NOTE && st->n);
        k[2].label = UXQ_LABEL[UXQ_L_FLAG];
        str_cpy(k[2].value, UXQ_FLAG[f], sizeof k[2].value);
        k[2].bar = (uint8_t)(f * 85u);
        k[2].off = (uint8_t)!f;
        k[3].label = UXQ_LABEL[UXQ_L_GATE];
        uxq_gate_txt(st, s, k[3].value, sizeof k[3].value, 0);
        k[3].bar = (uint8_t)(g ? (g > 16u ? 255u : g * 255u / 16u) : 0u);
    }
}
/* the status line for knob c just turned, in words: "STEP 5 . G2", "NOTE G#2", "SLIDE ON", "GATE TIE", "GATE 3 STEPS"; a DRUM
 * track's "STEP 5 . SNARE ON", "LANE SNARE", "HIT X2", "ACCENT ON"; a step held: the lock's "NAME . VALUE". 0: none */
static __attribute__((noinline)) const char *uxq_say(uint32_t c, char *b, uint32_t n)
{
    uint32_t s = uxq_step(), i;
    const step_t *st = &TSEL->step[s];
    int drum = drum_track(TSEL), lk = lock_held() != 0;
    ux_knob_t k[4];
    for (i = 0; i < 4u; i++) { k[i].label = ""; k[i].value[0] = 0; k[i].bar = 0; k[i].off = 0; }
    uxq_knobs(0, k);
    c &= 3u;
    b[0] = 0;
    if (!k[c].label || !k[c].label[0])
        return 0;
    if (!lk && c == 0u) {                               /* STEP: "STEP " and the spelled step to its second part ("5 . G2", */
        char *p;                                        /* "5 . SNARE ON") */
        uxq_cat(b, n, UXQ_WORD[UXQ_W_STEP]);
        uxq_spell(b + str_len(b), n - str_len(b));
        for (p = b, i = 0; *p && i < 2u; p++)
            i += *p == UXQ_WORD[UXQ_W_DOT][1];
        if (i == 2u)
            p[-2] = 0;                                  /* (the " . " before the third) */
        return b;
    }
    if (!lk && c == 2u && !drum) {                      /* ACC/SLD in words */
        uxq_cat(b, n, UXQ_FLAG_SAY[uxq_flag(st)]);
        return b;
    }
    if (lk) {                                           /* a lock's: "NAME . VALUE" */
        uxq_cat(b, n, k[c].label);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
    } else if (c == 3u && drum) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_ACCENT]);          /* (the grid's ACC: "ACCENT ON") */
    } else {
        uxq_cat(b, n, k[c].label);                      /* "NOTE G#2", "GATE TIE", "LANE SNARE", "HIT X2" */
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SP]);
    }
    if (!lk && !drum && c == 1u)
        uxq_note(st, b + str_len(b), n - str_len(b));   /* (NOTE: a chord's "+2" whole) */
    else if (!lk && !drum && c == 3u)
        uxq_gate_txt(st, s, b + str_len(b), n - str_len(b), 1);   /* (GATE: "3 STEPS") */
    else
        uxq_cat(b, n, k[c].value);
    return b;
}

/* ------------------------------------------------------------------------------------------------------- the pictures --- */
static uint16_t uxq_lane_col(uint32_t l) { return UX_BLEND[(l % NLANE) * 255u / (NLANE - 1u)]; }
/* the strip (ux_widgets.c ux_strip) for steps first..first+15: a TIE first carries its note's colour from before; the step playing
 * lit and the picked one outlined where they are in these 16; a chord being entered in step recording, KNOB 1 its length: the
 * steps it will hold outlined in ACCENT (the inherited roll's) */
static void uxq_strip(int y, uint32_t first)
{
    const track_t *t = TSEL;
    uint32_t len = uxq_len(), i, pick = uxq_step(), s = seq_src(t, t->seq_idx), n16 = first < len ? len - first : 0u;
    int drum = drum_track(t);
    uint16_t prev = UX_EMPTY;
    if (first && first < len && t->step[first].time == ST_TIE) {
        uint32_t src = pr_src(t, first, len);
        prev = src < NSTEP ? ux_step_col(&t->step[src], UX_EMPTY, drum) : UX_EMPTY;
    }
    ux_strip(y, &t->step[first < NSTEP ? first : 0u], n16 < 16u ? n16 : 16u, pick >= first && pick < first + 16u ? (int32_t)(pick - first) : -1,
             song.playing && s < len && s >= first && s < first + 16u ? (int32_t)(s - first) : -1, prev, drum);
    for (i = 0; ui.entry_open && !ui.ent_arp && ui.ent_nn && ui.ent_len > 1u && i < 16u; i++) {
        uint32_t si = first + i;
        if (si != pick && si < len && (si + len - pick) % len < ui.ent_len)
            cv_frame(UXQ_X(i), y, UX_STRIP_W, UX_STRIP_H, UX_ACCENT);
    }
}
/* the columns' common parts: past LEN dark, the playing step a light column, the beats a line before them */
static void uxq_cols(int32_t y0, int32_t h, uint32_t ncol, int32_t ph)
{
    uint32_t i;
    for (i = 4u; i < 16u; i += 4u)
        cv_rect(UXQ_X(i) - 1, y0, 1, h, UX_LINE);
    for (i = ncol; i < 16u; i++)
        cv_rect(UXQ_X(i), y0, UX_STRIP_W, h, UX_BG2);
    if (ph >= 0)
        cv_rect(UXQ_X(ph), y0, UX_STRIP_W, h, ux_mix(UX_BG, UX_TEXT, 14));
}
static void uxq_marks(int32_t y0, int32_t h, uint32_t ncol)   /* the step locks (a mark at the foot), the picked step outlined */
{
    uint64_t locks = motion_lock_steps(trk_index(TSEL));
    uint32_t i, f = uxq_first(), pick = uxq_step();
    for (i = 0; i < ncol; i++)
        if ((locks >> (f + i)) & 1u)
            cv_rect(UXQ_X(i) + 3, y0 + h - 3, UX_STRIP_W - 6, 2, UX_VIOLET_LT);
    if (pick >= f && pick < f + 16u)
        cv_frame(UXQ_X(pick - f) - 1, y0, UX_STRIP_W + 2, h, UX_TEXT);
}
static __attribute__((noinline)) void uxq_roll(int y0, int h)
{
    const track_t *t = TSEL;
    uint32_t len = uxq_len(), f = uxq_first(), ncol = f < len ? (len - f < 16u ? len - f : 16u) : 0u, i, j;
    int32_t lo = 127, hi = -1, span, rh, top, ph = uxq_ph();
    for (i = 0; i < ncol; i++) {                        /* (the notes of the 16 shown, a TIE's its note's) */
        uint32_t s = pr_src(t, f + i, len);
        for (j = 0; s < NSTEP && j < 4u + NLANE; j++) {
            int32_t n = pr_note(&t->step[s], j);
            if (n < 0)
                continue;
            lo = n < lo ? n : lo;
            hi = n > hi ? n : hi;
        }
    }
    if (hi < 0)
        lo = hi = 60;                                   /* (none shown: no row is drawn, the view is moot; fix 1: not last_note,
                                                         * which every key played moves) */
    span = hi - lo + 1;
    if (span < 12) {                                    /* (an octave at least, the notes centred) */
        lo -= (12 - span) / 2;
        span = 12;
    }
    rh = h / span;
    if (rh < 2) {                                       /* (too wide: two rows a px, the picked step's note in the middle) */
        const step_t *ps = &t->step[uxq_step()];
        rh = 2;
        span = h / 2;
        lo = (ps->time == ST_NOTE && ps->n ? ps->note[0] : (lo + hi) / 2) - span / 2;
    }
    if (rh > 6)
        rh = 6;
    lo = clamp(lo, 0, 128 - span);
    top = y0 + (h - span * rh) / 2;
    uxq_cols(y0, h, ncol, ph);
    for (i = 0; i < ncol; i++) {
        uint32_t si = f + i, s = pr_src(t, si, len), nx = (si + 1u) % len;
        const step_t *st, *ns = &t->step[nx];
        int32_t x = UXQ_X(i), w = UX_STRIP_W;
        int slide;
        uint16_t c;
        if (s == NSTEP)
            continue;                                   /* (a REST, empty, a TIE after one) */
        st = &t->step[s];
        slide = (st->flags & SF_SLIDE) && step_ratchet(st) == 1u && ns->time == ST_NOTE && step_on(ns) && i + 1u < ncol;   /* (as
                                                         * the sequencer: no slide out of a ratchet) */
        c = st->flags & SF_ACCENT ? UX_ACCENT : st->flags & SF_SLIDE ? UX_SLIDE : UX_ACID;
        if (s != si) {                                  /* (a TIE: its note carried on through the gap before) */
            x -= 1;
            w += 1;
        }
        if (ns->time == ST_TIE && i + 1u < ncol)        /* (held on: into the gap after) */
            w += 1;
        for (j = 0; j < 4u + NLANE; j++) {
            int32_t n = pr_note(st, j), y;
            if (n < 0)
                continue;
            y = top + (lo + span - 1 - clamp(n, lo, lo + span - 1)) * rh;
            pr_split(x, y, w, rh > 2 ? rh - 1 : rh, (n < lo || n >= lo + span) ? UX_DIM(c, 50) : c, s == si ? step_ratchet(st) : 1u);
            if (slide && j == 0u) {                     /* (a slide: joined to the next note, in SLIDE) */
                int32_t yn = top + (lo + span - 1 - clamp((int32_t)ns->note[0], lo, lo + span - 1)) * rh, a = y < yn ? y : yn,
                        b = y < yn ? yn : y;
                cv_rect(UXQ_X(i) + UX_STRIP_W - 2, a, 3, b - a + (rh > 2 ? rh - 1 : rh), UX_SLIDE);
            }
        }
    }
    uxq_marks(y0, h, ncol);
}
static __attribute__((noinline)) void uxq_grid(int y0, int h)
{
    const track_t *t = TSEL;
    uint32_t len = uxq_len(), f = uxq_first(), ncol = f < len ? (len - f < 16u ? len - f : 16u) : 0u, i, l, sel = ui.lane % NLANE;
    int32_t lp = h / (int32_t)NLANE, ph = uxq_ph();
    cv_rect(UXQ_X(0) - 2, y0 + (int32_t)sel * lp, UXQ_X(15) + UX_STRIP_W + 2 - (UXQ_X(0) - 2), lp, UX_BG2);   /* (the lane picked) */
    uxq_cols(y0, h, ncol, ph);
    for (l = 0; l < NLANE; l++) {
        int32_t y = y0 + (int32_t)l * lp + 1;
        uint16_t c = uxq_lane_col(l);
        for (i = 0; i < ncol; i++) {
            const step_t *st = &t->step[f + i];
            int32_t x = UXQ_X(i);
            if ((step_lanes(st) >> l) & 1u)
                pr_split(x + 1, y, UX_STRIP_W - 2, lp - 2, (step_accents(st) >> l) & 1u ? ux_mix(c, UX_TEXT, 55) : c, step_ratchet(st));
            else
                cv_rect(x + UX_STRIP_W / 2 - 1, y + lp / 2 - 2, 2, 2, l == sel || i % 4u == 0u ? UX_MID : UX_LINE);
        }
    }
    uxq_marks(y0, h, ncol);
}
static __attribute__((noinline)) void uxq_picture(const page_t *pg)
{
    char b[40];
    int32_t x = UX_STRIP_X;
    (void)pg;
    if (drum_track(TSEL))
        uxq_grid(UXQ_RY0, UXQ_RH);
    else
        uxq_roll(UXQ_RY0, UXQ_RH);
    uxq_strip(UXQ_STRIP_Y, uxq_first());
    uxq_spell(b, sizeof b);
    GFX_HOOK_ALIGN(0, UXQ_SAY_Y + UF_HDR_CAP_Y, 0, UXQ_SAY_Y + UF_HDR_CAP_Y + UF_HDR_CAP_H, AL_V | AL_PASS, "STEPS' step spelled");
    cv_text_on(x, UXQ_SAY_Y, &UF_HDR, b, UX_TEXT, ux_under(x, UXQ_SAY_Y + 4));
}
/* what the picture is drawn from but the playhead (that one paced: ux_section_draw): the steps, LEN, the step picked and its 16,
 * the lane, the locks, a DRUM track or not, a chord being entered (fix 1: not last_note, which a key played moves and an empty
 * 16 never shows) */
static __attribute__((noinline)) uint32_t uxq_sig(void)
{
    const track_t *t = TSEL;
    uint64_t locks = motion_lock_steps(trk_index(t));
    return ((steps_hash(t) * 31u + uxq_len()) * 131u + uxq_step()) * 257u + ui.lane * 7u + (uint32_t)drum_track(t) * 3u +
           (uint32_t)(locks ^ (locks >> 32)) * 977u +
           (ui.entry_open * 2u + ui.ent_arp + ui.ent_nn * 4u + ui.ent_len * 64u) * 40503u;   /* (the chord's length outlined) */
}
