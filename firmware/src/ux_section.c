/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's section pages on the screen (ux_section.h; 3b spec 2, parent spec 3.3): any PAGES row drawn with the 3a widgets.
 *   header   T3 . the sound, the transport at the right (ux_header)
 *   knobs    the PAGES row's four: the label in plain words (UXS_NAME, UXS_LABEL), the value with its unit (uxs_val: "1.9K",
 *            "1.0MS"; MORE's rows and the status line the same), the bar its place in the range, KNOB 1..4 ACID SLIDE VIOLET
 *            ACCENT; MORE: KNOB 1 the setting picked (n/N), KNOB 2 its value
 *   panel    the page's picture over the glow: a synth's EDIT what it sounds like (the engine's signal path, ported; its
 *            FILTER page the filter's response; VOICE singing words the word); MORE its
 *            list (ux_more.c); the sections still to come (3b tasks 5..8) their title and the four values as bars
 *   status   a message (STOP TO EDIT in ACCENT), else the knob just turned: its full name and value ("RELEASE . 420MS"),
 *            else the section's first-time hint ("ARP AGAIN: NEXT PAGE", once a power-on, spec 0 rule 7), else on MORE
 *            what its controls do
 *   footer   SECTION . PAGE at the left; n / N of the section's chain (N counts MORE) and the knobs' names (the knob row's,
 *            when they fit) at the right
 * Strips as HOME's (dc_frame / cv_blit_cells): only the changed cells go out. Included after ux_more.c (and ux_home.c:
 * ux_under, ux_trk_name). */

/* a setting's plain name (spec 0 rule 4: one name everywhere it shows, the knob row, the status line, MORE's rows, the footer),
 * by the section showing it, the track's engine, global or the track's, and its id: where JIANT's label is cryptic or names
 * more than one thing (MODE: the arp's, the glide's, the slicer's, the LFO's). UXS_ALL: every section / engine; the most
 * specific row wins (its section over every section's, then its engine over every engine's). JIANT's labels themselves
 * (TP / GP: the web editor reads them, editor.c) stay as they are. Each fits a knob's column in UF_LBL (tests/ui_test.c) */
typedef char uxs_sec_edit_ok[SEC_EDIT == UXS_SEC_EDIT ? 1 : -1];   /* (ux_section.h's EDIT is ui_lens.c's) */
#define UXS_ALL 0xFFu
typedef struct { uint8_t sec, eng, glo, id; const char *p; } uxs_name_t;   /* id: a P_* (glo 0) or G_* (glo 1), or a view */
static const uxs_name_t UXS_NAME[] = {
    {UXS_ALL, UXS_ALL, 0, P_GLMODE, "GLIDE MODE"},      /* (JIANT's MODE: RATE / TIME, the glide's) */
    {UXS_ALL, UXS_ALL, 0, P_DETUNE, "SPREAD"},          /* (3b task 4) UNISON's DETUNE (ANALOG's own DETUNE is osc 2's) */
    {UXS_ALL, UXS_ALL, 0, P_FTYPE, "FILTER"},           /* .. the filter's TYPE: FILTER LP / HP / BP / COMB */
    {UXS_ALL, ENGI_LOFI, 0, P_LRTRG, "RETRIG"},         /* .. LOFI BYTE / FLOAT's RTRG */
    {UXS_ALL, ENGI_VOICE, 0, P_VLANG, "WORDS"},         /* .. VOICE's LANG (the same slot): WORDS OFF / ES / EN / JA */
    {UXS_ALL, ENGI_PHASE, 0, P_E2, "BRIGHT"},           /* .. PHASE's DCW: how far the phase is bent */
    {UXS_ALL, ENGI_PHASE, 0, P_E3, "SWEEP"},            /* .. its ENV: how far the envelope moves DCW */
    {UXS_ALL, ENGI_PHASE, 0, P_E5, "LINE 2"},           /* .. its LINE: line 2 MIX / RING */
    {UXS_ALL, ENGI_NOISE, 0, P_E0, "SOURCE"},           /* .. NOISE's MODE: ANLG DUST LFSR META */
};
static const char *uxs_name_in(const uxs_name_t *t, uint32_t n, uint32_t sec, uint32_t eng, uint32_t glo, uint32_t id)
{
    const char *p = 0;
    uint32_t i, best = 0;
    for (i = 0; i < n; i++) {
        uint32_t sc;
        if (t[i].id != id || (t[i].glo != glo && id < ID_LPHS) ||          /* (a view: either) */
            (t[i].sec != UXS_ALL && t[i].sec != sec) || (t[i].eng != UXS_ALL && t[i].eng != eng))
            continue;
        sc = 1u + (t[i].sec != UXS_ALL ? 2u : 0u) + (t[i].eng != UXS_ALL ? 1u : 0u);
        if (sc > best)
            best = sc, p = t[i].p;
    }
    return p;
}
/* .. else plain words for JIANT's short labels, by their text: the same everywhere */
typedef struct { const char *j, *p; } uxs_lbl_t;
static const uxs_lbl_t UXS_LABEL[] = {
    {"ATK", "ATTACK"}, {"DEC", "DECAY"}, {"SUS", "SUSTAIN"}, {"REL", "RELEASE"}, {"RES", "RESO"}, {"CUT", "CUTOFF"},
    {"DLY", "DELAY"}, {"REV", "REVERB"}, {"FEEDBK", "FEEDBACK"}, {"PRIO", "PRIORITY"}, {"DIST", "DRIVE"},
    /* (3b task 4) the synths' (the engines' .edit[] and their mode relabels, eng_*.c) */
    {"INT", "INTERVAL"}, {"WAVE2", "WAVE 2"}, {"FB", "FEEDBACK"}, {"VIB", "VIBRATO"}, {"WAV#", "TABLE"}, {"VAR", "VARIANT"},
    {"ALGO", "FORMULA"}, {"TRK", "KEY TRACK"}, {"DENSE", "DENSITY"}, {"LEN", "LENGTH"}, {"VOWEL2", "VOWEL 2"}, {"Q", "RESO"},
    {"RAND", "RANDOM"}, {"POS", "POSITION"}, {"ARTIC", "CRISP"},
};
static const char *uxs_label(uint32_t sec, uint32_t glo, uint32_t id, const char *l)
{
    uint32_t i;
    const char *p = id < 256u ? uxs_name_in(UXS_NAME, NELEM(UXS_NAME), sec, trk_eng(TSEL), glo ? 1u : 0u, id) : 0;
    if (p)
        return p;
    if (!l)
        return "";
    for (i = 0; i < NELEM(UXS_LABEL); i++)
        if (str_eq(l, UXS_LABEL[i].j))
            return UXS_LABEL[i].p;
    return l;
}

/* (3b task 4 fix) a value as the knob row, MORE's rows and the status line all show it (spec 0 rule 4: one form): params.c
 * param_format's, its unit after it, compact so a knob's column (48 px) holds it: "355HZ", "1.9K" (kHz; from 10 kHz whole:
 * "16K"), "1.0MS", "1.00S", "50%", "+12ST", "10CT". A unit that would not fit is left out (none on the synth pages: their
 * every value is checked, tests/ui_test.c) */
static const char *const uxs_unit_j[] = {"Hz", "kHz", "ms", "s", "st", "ct", "dB", "%"};   /* (param_format's: not drawn) */
static const char *const UXS_UNIT[] = {"HZ", "K", "MS", "S", "ST", "CT", "DB", "%"};       /* .. each as it is drawn */
typedef char uxs_unit_ok[NELEM(uxs_unit_j) == NELEM(UXS_UNIT) ? 1 : -1];
static void uxs_val(const param_desc_t *d, int32_t v, char *b, uint32_t nb)
{
    const char *u = "", *c = "";
    char t[16];
    uint32_t i;
    param_format(d, v, t, &u);
    for (i = 0; u && i < NELEM(UXS_UNIT); i++)
        if (str_eq(u, uxs_unit_j[i]))
            c = UXS_UNIT[i];
    if (d->fmt == F_CUTOFF && CUTOFF_HZ[v & 127] >= 10000u)   /* (10 kHz and up: whole kHz) */
        fmt_int(t, (int32_t)((CUTOFF_HZ[v & 127] + 500u) / 1000u));
    str_cpy(b, t, nb);
    if (c[0] && text_w(&UF_VAL, b) + text_w(&UF_VAL, c) <= UX_COL_W - 4)
        str_cpy(b + str_len(b), c, nb - str_len(b));
}

#define UXS_HINT " AGAIN: NEXT PAGE"                    /* (after the section's name: what its button does next) */
#define UXM_HINT "SELECT SCROLLS \xB7 KNOB 2 SETS"      /* MORE: what its controls do (KNOB 1 picks: its column says so) */
#define UXS_PICK "SETTING"                              /* MORE's KNOB 1 */
static uint16_t uxs_seen;        /* the sections opened since power-on (a bit per SEC_*): their hint said */
static uint8_t uxs_hint_sec = 0xFF, uxs_hint_page;      /* the page saying its section's hint (0xFF none): the first one
                                                         * opened there, until it is left or a knob turns */
static uint32_t uxs_frame;                              /* the UI frame last drawn here (another in between: left) */
static uint32_t uxs_panels;                             /* the panels drawn (tests/ui_test.c: one a change, none idle) */

/* the footer's left, "ARP . MORE"; its right's count, "3 / 3" (the chain's pages shown here, MORE too) */
static const char *uxs_where(void)
{
    static char b[32];
    str_cpy(b, SEC_NAME[nav_sec()], sizeof b);
    str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), nav_page_name(), sizeof b - str_len(b));
    return b;
}
static const char *uxs_count(void)
{
    static char b[12];
    uint8_t cnt[16], gi[16];
    uint32_t g, p, i, m;
    (void)nav_groups(cnt, gi, &g, &p, &i, &m);
    fmt_int(b, (int32_t)i + 1);
    str_cpy(b + str_len(b), " / ", sizeof b - str_len(b));
    fmt_int(b + str_len(b), (int32_t)m);
    return b;
}

/* the knob row of page pg (MORE: its pick and value), each label its plain name (uxs_label) */
static void uxs_knobs(const page_t *pg, ux_knob_t k[4])
{
    uint32_t c;
    for (c = 0; c < 4u; c++) {
        k[c].label = "";
        k[c].value[0] = 0;
        k[c].bar = 0;
    }
    if (pg->graph == GR_MORE) {
        uint32_t s = uxm_at[uxm_key()].sel, t = uxm_at[uxm_key()].top, n;
        uint64_t m;
        char v[24];
        if (!pg->more)
            return;
        m = uxm_mask(pg->more);
        n = uxm_n(pg->more);
        uxm_fix(pg->more, m, &s, &t);
        k[0].label = UXS_PICK;
        if (s < n) {
            uint32_t a = 0, b = 0, i;
            for (i = 0; i < n; i++)
                if (uxm_pick_ok(pg->more, m, i)) {
                    b++;
                    if (i <= s)
                        a = b;
                }
            fmt_int(k[0].value, (int32_t)a);
            str_cpy(k[0].value + str_len(k[0].value), "/", sizeof k[0].value - str_len(k[0].value));
            fmt_int(k[0].value + str_len(k[0].value), (int32_t)b);
            k[0].bar = (uint8_t)(b > 1u ? (a - 1u) * 255u / (b - 1u) : 255u);
            k[1].label = uxm_row_text(&pg->more->rows[s], v, sizeof v, &k[1].bar);
            if (pg->more->rows[s].kind == UXR_ACTION)
                str_cpy(v, "GO", sizeof v);
            str_cpy(k[1].value, v, sizeof k[1].value);
        }
        return;
    }
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, c, &vp);
        if (!d || !vp || !d->label || d->label[0] == '-')
            continue;
        uxs_val(d, *vp, k[c].value, sizeof k[c].value);
        k[c].label = uxs_label(nav_sec(), pg->scope == SC_GLOBAL, page_id(pg, c), d->label);
        if (d->max > d->min) {
            int32_t b = clamp(((int32_t)*vp - d->min) * 255 / (d->max - d->min), 0, 255);
            k[c].bar = (uint8_t)(d->fmt == F_PCTINV ? 255 - b : b);
        }
    }
}

/* the footer's right: cnt ("3 / 3"), then the knob row's names as it shows them (spec 0 rule 4: the same names; JIANT's short
 * labels were not: DIST under DRIVE) when they fit beside w, the footer's left */
static void uxs_foot(const char *w, const ux_knob_t k[4], const char *cnt, char *b, uint32_t nb)
{
    char t[48];
    uint32_t c, n = 0;
    str_cpy(b, cnt, nb);
    str_cpy(t, cnt, sizeof t);
    for (c = 0; c < 4u; c++)
        if (k[c].label[0]) {
            str_cpy(t + str_len(t), " \xB7 ", sizeof t - str_len(t));
            str_cpy(t + str_len(t), k[c].label, sizeof t - str_len(t));
            n++;
        }
    if (n && 8 + text_w(&UF_LBL, w) + 16 + text_w(&UF_LBL, t) <= 232)
        str_cpy(b, t, nb);
}

/* the status line (above); a knob's readout: "NAME . VALUE unit" */
static const char *uxs_status(const page_t *pg, uint16_t *col)
{
    static char b[48];
    int32_t h = ux_knob_hot();
    uint32_t sec = nav_sec();
    if (ui.msg_t && ui.msg[0]) {
        *col = str_eq(ui.msg, UX_STOP) ? UX_ACCENT : UX_TEXT;
        return ui.msg;
    }
    if (h >= 0) {
        const char *nm = 0;
        char v[24];
        uint8_t bar;
        if (pg->graph == GR_MORE && pg->more && h < 2) {   /* (KNOB 1 or 2: the setting picked) */
            uint32_t s = uxm_at[uxm_key()].sel, t = uxm_at[uxm_key()].top;
            uxm_fix(pg->more, uxm_mask(pg->more), &s, &t);
            if (s < uxm_n(pg->more))
                nm = uxm_row_text(&pg->more->rows[s], v, sizeof v, &bar);
        } else if (pg->graph != GR_MORE) {
            int16_t *vp;
            const param_desc_t *d = page_desc(pg, (uint32_t)h, &vp);
            if (d && vp && d->label && d->label[0] != '-') {
                uxs_val(d, *vp, v, sizeof v);
                nm = uxs_label(sec, pg->scope == SC_GLOBAL, page_id(pg, (uint32_t)h), d->label);
            }
        }
        if (nm && nm[0]) {
            str_cpy(b, nm, sizeof b);
            str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
            str_cpy(b + str_len(b), v, sizeof b - str_len(b));
            *col = UX_TEXT;
            return b;
        }
    }
    *col = UX_MID;
    if (sec == uxs_hint_sec) {
        str_cpy(b, SEC_NAME[sec], sizeof b);
        str_cpy(b + str_len(b), UXS_HINT, sizeof b - str_len(b));
        return b;
    }
    return pg->graph == GR_MORE ? UXM_HINT : 0;
}

/* ------------------------------------------------------------------------- the synths' EDIT picture --- */
/* (3b task 4; its fix: the picture is the sound) a synth's EDIT pages draw what it plays: the engine's own signal path, in
 * its order and with its ranges, ported from its render (eng_analog.c analog_dispatch, eng_phase.c phase_render, eng_lofi.c
 * lofi_render, eng_noise.c noise_render; VOICE: glottal pulses through its vowel's formants) with the dsp.c helpers they use,
 * from the track's stored settings. The note is C4 as it strikes (uxs_vmod: the envelope at its peak, ENV > FILTER and
 * SHAPE with it; no LFO), MORPH where the knob is (the engines slew to it). A short render a change (a turn redraws it,
 * once; idle or playing, nothing a frame), never in the audio path; tests/ui_test.c checks it against the engines' own
 * output. ANALOG's SOUND page draws its oscillators raw (WAVE DETUNE MIX NOISE: before DRIVE and the filter), its TONE
 * and VOICE the sound out of DRIVE and the filter. The panel's 128 columns hold the window's samples (uxs_win: two cycles
 * of C4 after two to settle; DUST a fifth of a second, for its sparse impulses; LFSR 16 cycles, its register's steps),
 * each the sample farthest from 0 in it (of two about as far, the first). FILTER draws the part's filter's response (ui_lzedit.c ed_resp) */
#define UXS_WN 128u              /* the panel's columns */
#define UXS_C4 (60 * 16)         /* the note drawn (1/16 semitones) */
#define UXS_DUST 8192u           /* DUST's window, samples */
static uint32_t uxs_rnd(uint32_t *r) { *r ^= *r << 13; *r ^= *r >> 17; *r ^= *r << 5; return *r; }
/* the window's samples into the columns o[]: pre samples to settle first, then len across the 128 */
typedef struct { int32_t *o; uint32_t pre, len, j, c; } uxs_tap_t;
static void uxs_tap_on(uxs_tap_t *t, int32_t *o, uint32_t pre, uint32_t len)
{
    t->o = o;
    t->pre = pre;
    t->len = len ? len : 1u;
    t->j = 0;
    t->c = 0xFFFFFFFFu;
}
static int uxs_tap(uxs_tap_t *t, int32_t s)              /* a sample in; 0: the window is full */
{
    uint32_t j = t->j++, c;
    if (j < t->pre)
        return 1;
    j -= t->pre;
    c = j * UXS_WN / t->len;
    if (c != t->c) {
        t->c = c;
        t->o[c] = s;
    } else {
        int32_t a = s < 0 ? -s : s, b = t->o[c] < 0 ? -t->o[c] : t->o[c];
        if (a > b + (b >> 6) + 2)                       /* (clearly farther: two of a size, the first stays) */
            t->o[c] = s;
    }
    return j + 1u < t->len;
}
/* the window for engine e at settings p: pre samples to settle, len drawn */
static void uxs_win(const engine_t *e, const int16_t *p, uint32_t *pre, uint32_t *len)
{
    uint32_t cyc = 0xFFFFFFFFu / pitch_inc(UXS_C4);     /* (a cycle of C4: ~169 samples) */
    *pre = 2u * cyc;
    *len = 2u * cyc;
    if (e == &ENG_NOISE && p[P_E0] == NZ_DUST)
        *len = UXS_DUST;
    else if (e == &ENG_NOISE && p[P_E0] == NZ_LFSR)
        *len = 16u * cyc;
}
/* the note as voice.c's track_render hands it to the engine: C4 at full level, the envelope at its peak (its FILTER and
 * SHAPE amounts with it), velocity 100 (no accent), no LFO, no drift, no bend */
static void uxs_vmod(const int16_t *p, vmod_t *m)
{
    m->pitch16 = UXS_C4;
    m->inc = pitch_inc(UXS_C4);
    m->amp0 = m->amp1 = 32767;
    m->envq15 = 32767;
    m->fine = 0;
    m->cutoff = (32767 * p[P_ED_FLT]) >> 7;
    m->shape = (64 << 8) + ((32767 * p[P_ED_SHP]) >> 7);
}
/* ANALOG (analog_dispatch: analog_render, analog_ext, analog_morph): the two oscillators (MORPH, PWM's width, SYNC RING
 * SAW3, osc 2 at INTERVAL and DETUNE), MIX (osc 2 0 .. 100 %), NOISE; then (raw 0) DRIVE (1x .. 3x, only above 0) before
 * the filter (FILTER LP HP BP; COMB: open here, its comb the part's) at CUTOFF and RESO, its soft knee */
static void uxs_wave_analog(const int16_t *p, int32_t *o, uint32_t pre, uint32_t len, int raw)
{
    vmod_t m;
    uxs_tap_t tp;
    tsvf_t flt;
    uint32_t wave = (uint32_t)p[P_E0] & 7u, ft = (uint32_t)p[P_FTYPE] & 3u, ph0 = 0, ph1 = 0x40000000u, ph2 = 0, inc1, inc2;
    uint32_t inc3 = 0, r = 0, pw;
    int32_t mq = clamp(p[P_MORPH], 0, 127) << 8, mpos = -1, mx, det, m2 = p[P_E2] * 258, m1 = 32767 - m2, nz = p[P_E3] * 200;
    int32_t drv = p[P_E6], drive = 32768 + drv * 512, kd = 8192 - p[P_E5] * 7600 / 127, ic1 = 0, ic2 = 0, nst = 0x1234567, s;
    uxs_vmod(p, &m);
    inc1 = m.inc;
    tsvf_coef(&flt, (p[P_E4] << 8) + m.cutoff, p[P_E5]);   /* (C4: no key tracking) */
    if (wave < 4u && (mq || m.shape != (64 << 8)))      /* (an_mpos) */
        mpos = clamp((int32_t)wave * 256 + clamp(mq + m.shape - (64 << 8), 0, 127 << 8) * 768 / (127 << 8), 0, 1536);
    mx = wave >= AW_SYNC ? clamp(mq + m.shape - (64 << 8), 0, 127 << 8) : 0;   /* (an_mext) */
    det = p[P_E1] + (wave == AW_SAW3 ? mx * 40 / (127 << 8) : 0);
    pw = 0x80000000u + (uint32_t)(clamp((m.shape - (64 << 8)) + (wave == 4u ? mq * 2 : 0), -16384, 65000) << 15);   /* (an_pw) */
    if (wave >= AW_SYNC) {                              /* (analog_ext) */
        r = 16u + (uint32_t)det + (wave != AW_SAW3 ? (uint32_t)(mx * 64 / (127 << 8)) : 0u);
        inc2 = wave == AW_SAW3 ? cents_inc(m.pitch16 + p[P_E7] * 16, det, 0) : (inc1 >> 4) * r;
        if (wave != AW_SAW3 && p[P_E7]) {
            inc2 = (cents_inc(m.pitch16 + p[P_E7] * 16, 0, 0) >> 4) * r;
            r = inc1 >> 8 ? (inc2 / (inc1 >> 8)) >> 4 : r;
            if (r < 1u)
                r = 1u;
        }
        inc3 = wave == AW_SAW3 ? cents_inc(m.pitch16, -det, 0) : 0u;
    } else
        inc2 = p[P_E1] || p[P_E7] ? cents_inc(m.pitch16 + p[P_E7] * 16, p[P_E1], 0) : inc1;
    uxs_tap_on(&tp, o, pre, len);
    do {
        int32_t a, b;
        if (mpos >= 0) {
            a = an_morph(mpos, ph0, inc1);
            b = an_morph(mpos, ph1, inc2);
        } else if (wave < AW_SYNC) {
            switch (wave) {
            case 1: a = osc_pulse(ph0, inc1, 0x80000000u); b = osc_pulse(ph1, inc2, 0x80000000u); break;
            case 2: a = osc_tri(ph0); b = osc_tri(ph1); break;
            case 3: a = osc_sine(ph0); b = osc_sine(ph1); break;
            case 4: a = osc_pulse(ph0, inc1, pw); b = osc_pulse(ph1, inc2, pw); break;
            default: a = osc_saw(ph0, inc1); b = osc_saw(ph1, inc2); break;
            }
        } else {
            a = osc_saw(ph0, inc1);
            if (wave == AW_SAW3) {
                b = (osc_saw(ph1, inc2) + osc_saw(ph2, inc3)) >> 1;
                ph2 += inc3;
            } else if (wave == AW_SYNC)
                b = osc_saw(ph1, inc2);
            else
                b = ((a >> 1) * (osc_sine(ph1) >> 1)) >> 13;
        }
        ph1 += inc2;
        if (wave == AW_SYNC && ph0 + inc1 < ph0)
            ph1 = ((ph0 + inc1) >> 4) * r;
        ph0 += inc1;
        s = mulq15(a, m1) + mulq15(b, m2);
        if (nz)
            s += mulq15((int32_t)(noise32(&nst) >> 17) - 16384, nz);
        if (!raw) {
            if (drv)
                s = softclip(((s >> 2) * (drive >> 2)) >> 11);
            s = soft_knee(svf_mode(&flt, s >> 1, &ic1, &ic2, ft, kd), 16000) << 1;
        }
    } while (uxs_tap(&tp, s));
}
/* PHASE (phase_render): the bent -cos at BRIGHT (DCW, the classic range: up to 56000 of 65535) and SWEEP (ENV, at the
 * strike), WAVE then WAVE 2 a cycle each, MORPH towards the next WAVE, FEEDBACK bending its phase, line 2 at DETUNE
 * (LINE 2 MIX / RING), SUB an octave down */
static void uxs_wave_phase(const int16_t *p, int32_t *o, uint32_t pre, uint32_t len)
{
    vmod_t m;
    uxs_tap_t tp;
    pd_t b1, b2, bm;
    uint32_t w1 = (uint32_t)p[P_E0] & 7u, w2 = (uint32_t)p[P_E1], dcw, inc, inc2, ph0 = 0, ph1 = 0, ph2 = 0;
    int32_t depth, det = p[P_E4], line2 = det != 0 || p[P_E5], ring = p[P_E5], sub = p[P_E6] * 200;
    int32_t mq = clamp(p[P_MORPH], 0, 127) << 8, tg0 = 0, tg1 = 0, fb = p[P_E7] << 8, f1 = 0, f2 = 0, mf = 0, s;
    uxs_vmod(p, &m);
    inc = m.inc;
    depth = (p[P_E2] << 8) + m.cutoff + mulq15(m.envq15, p[P_E3] * 256);
    depth = clamp(depth + (m.shape - (64 << 8)), 0, 127 << 8);
    dcw = (uint32_t)depth * 65535u / (127u << 8);
    dcw = (dcw * 56000u) >> 16;
    inc2 = cents_inc(m.pitch16, det, 0);
    if (mq) {
        int32_t pos = (int32_t)w1 * 256 + mq * 3 / 127;
        w1 = ((uint32_t)pos >> 8) & 7u;
        mf = (pos & 255) << 8;
        pd_setup(&bm, (w1 + 1u) & 7u, dcw);
    }
    pd_setup(&b1, w1, dcw);
    pd_setup(&b2, w2 ? w2 - 1u : w1, dcw);
    uxs_tap_on(&tp, o, pre, len);
    do {
        uint32_t old = ph0, pp = ((ph0 >> 16) + (uint32_t)((((f1 + f2) >> 1) * (fb >> 8)) >> 8)) & 0xFFFFu;
        s = pd_wave(tg0 ? &b2 : &b1, pp);
        if (mf && !tg0)
            s += ((pd_wave(&bm, pp) - s) * (mf >> 8)) >> 8;
        f2 = f1;
        f1 = s;
        ph0 += inc;
        if (ph0 < old)
            tg0 ^= 1;
        if (line2) {
            uint32_t old2 = ph1;
            int32_t s2 = pd_wave(tg1 ? &b2 : &b1, ph1 >> 16);
            ph1 += inc2;
            if (ph1 < old2)
                tg1 ^= 1;
            s = ring ? mulq15(s, s2) : (s + s2) >> 1;
        }
        if (sub) {
            ph2 += inc >> 1;
            s += mulq15(osc_sine(ph2), sub);
        }
    } while (uxs_tap(&tp, s));
}
/* LOFI (lofi_render): PLS (DUTY: 12.5 .. 50 %; STEP 12.5 25 50 75), TRI (STEP: 32 steps), SAW, NOIS, a WRAM TABLE; MORPH
 * towards the next; BEND, VIBRATO (a block at a time); held for CHIP (4B/2: two samples) and CRUSH, in CHIP's bits
 * (4BIT 4B/2 STEP 4, 8BIT 8, 1BIT 1), MASK, LOFI_SOFT and the FILTER page's filter. BYTE / FLOAT: the formula over its
 * first 1024 steps of t (lofi_algo_at), in CHIP's bits */
static void uxs_wave_lofi(const int16_t *p, int32_t *o, uint32_t pre, uint32_t len)
{
    static const uint8_t STEP_DUTY[4] = {1, 2, 4, 6};
    vmod_t m;
    uxs_tap_t tp;
    tsvf_t flt;
    uint32_t chip = (uint32_t)p[P_E0], wave = (uint32_t)p[P_E1], stepc = chip == CHIP_STEP, ft = (uint32_t)p[P_FTYPE] & 3u;
    uint32_t ph = 0, lfsr = 0x7FFFu, inc0, bx = 0, bk0 = 0, bk1 = 0, vph = 0, i;
    uint32_t duty = stepc ? 0x20000000u * STEP_DUTY[(p[P_E2] / 32) & 3] : 0x20000000u * (1u + (uint32_t)p[P_E2] / 32u);
    int32_t crush = stepc ? 0 : p[P_E3], bits = chip == CHIP_8BIT ? 8 : chip == CHIP_1BIT ? 1 : 4;
    int32_t hold = 1 + crush / 8 + (chip == 1u ? 1 : 0), held = 0, cnt = 0, ic1 = 0, ic2 = 0, msk = p[P_E7] >> 1, sm = 0;
    int32_t wdc = 0, mf = 0, rs = p[P_FRES] > 50 ? p[P_FRES] : 50, kd = 8192 - rs * 7600 / 127, mo, y = 0;
    const uint8_t *wr = WRAM[((uint32_t)p[P_E2] >> 3) & 15u];
    if (wave >= RW_BYTE) {                              /* BYTE / FLOAT: the formula, in the chip's bits */
        uint32_t lv = chip == CHIP_8BIT ? 256u : chip == CHIP_1BIT ? 2u : 16u;
        for (i = 0; i < UXS_WN; i++) {
            int32_t s = wave == RW_BYTE ?
                ((int32_t)(lofi_algo_at(0, (uint32_t)p[P_E2] >> 2, i * 8u, 1u + ((uint32_t)p[P_E3] >> 3)) & 255) - 128) * 256 :
                lofi_algo_at(1, (uint32_t)p[P_E2] >> 2, i * 8u, 0);
            s = clamp(s, -32767, 32767);
            if (lv < 256u) {
                int32_t q = (int32_t)(((uint32_t)(s + 32768) * lv) >> 16);
                s = (int32_t)((q * 65535) / (int32_t)(lv - 1u)) - 32768;
            }
            o[i] = s;
        }
        return;
    }
    uxs_vmod(p, &m);
    inc0 = pitch_inc(m.pitch16);
    tsvf_coef(&flt, (p[P_FCUT] << 8) + m.cutoff, rs);
    if (p[P_E6]) {                                      /* (lofi_bend_set) */
        bx = 32768u - (uint32_t)p[P_E6] * 250u;
        bk0 = (32768u << 16) / bx;
        bk1 = (32767u << 16) / (65535u - bx);
    }
    mo = clamp((clamp(p[P_MORPH], 0, 127) << 8) + m.shape - (64 << 8), 0, 127 << 8);
    if (wave == RW_WRAM || mo) {
        if (mo) {
            int32_t pos = clamp((int32_t)wave * 256 + mo * 768 / (127 << 8), 0, RW_WRAM * 256);
            wave = (uint32_t)pos >> 8;
            mf = (pos & 255) << 8;
        }
        if ((uint32_t)p[P_E1] == RW_WRAM || wave == RW_WRAM || (mf && wave + 1u == RW_WRAM)) {   /* (the table's mean) */
            int32_t sum = 0;
            for (i = 0; i < 16u; i++)
                sum += bits == 1 ? ((wr[i] >> 7) + ((wr[i] >> 3) & 1)) * 2 - 2 : (wr[i] >> 4) + (wr[i] & 15) - 15;
            wdc = bits == 1 ? sum * 32767 / 32 : sum * 2 * Q4 / 32;
        }
    }
    uxs_tap_on(&tp, o, pre, len);
    for (;;) {                                          /* a block of CTL: the vibrato's step */
        uint32_t inc = inc0;
        if (p[P_E5])
            inc += (uint32_t)(((int32_t)(inc >> 12) * (((osc_sine(vph) >> 8) * p[P_E5]) >> 4)) >> 4);
        vph += 0x01000000u;
        for (i = 0; i < CTL; i++) {
            if (--cnt <= 0) {
                uint32_t q = bits < 16, ph0 = ph;
                int32_t s;
                if (p[P_E6] && wave != RW_NOIS) {       /* (lofi_bend) */
                    uint32_t b = ph >> 16;
                    b = b < bx ? (b * bk0) >> 16 : 32768u + (((b - bx) * bk1) >> 16);
                    ph0 = b << 16 | (ph & 0xFFFFu);
                }
                cnt = hold;
                switch (wave) {
                case RW_TRI:
                    if (stepc) {
                        uint32_t k = ph0 >> 27;
                        s = (int32_t)(k < 16u ? 15u - k : k - 16u) * (2 * Q4) - 15 * Q4;
                        q = 0;
                    } else
                        s = osc_tri(ph0);
                    break;
                case RW_SAW:
                    s = (int32_t)(ph0 >> 16) - 32768;
                    break;
                case RW_NOIS: {
                    uint32_t l = lfsr;
                    if ((ph + inc * (uint32_t)hold) < ph || chip == 3u)
                        l = (l >> 1) | (((l ^ (l >> 1)) & 1u) << 14);
                    lfsr = l;
                    s = (l & 1u) ? 32767 : -32768;
                    break;
                }
                case RW_WRAM: {
                    uint32_t k = ph0 >> 27, b = wr[k >> 1];
                    b = (k & 1u) ? b & 15u : b >> 4;
                    s = (bits == 1 ? (b >= 8u ? 32767 : -32767) : ((int32_t)b * 2 - 15) * Q4) - wdc;
                    q = 0;
                    break;
                }
                default:
                    s = (ph0 < duty ? 32767 : -32768) - ((int32_t)(duty >> 16) - 32768);
                    break;
                }
                if (mf >> 8)
                    s += ((lofi_mto(wave + 1u, ph0, duty, wr, wdc, lfsr) - s) * (mf >> 8)) >> 8;
                if (q)
                    s = ((s + (1 << (15 - bits))) >> (16 - bits)) << (16 - bits);
                if (crush > 64)
                    s = (s >> 12) << 12;
                held = msk ? (((s >> 8) ^ msk) << 8) : s;   /* (lofi_mask) */
            }
            ph += inc;
            sm = LOFI_SOFT(held >> 1, sm);
            y = soft_knee(svf_mode(&flt, sm, &ic1, &ic2, ft, kd), 16000) << 1;
            if (!uxs_tap(&tp, y))
                return;
        }
    }
}
/* eng_noise.c's noise_inc and isqrt32, again: called from here too, the compiler would stop putting them inside noise_render
 * (its code changed: tests/target_budget.py) */
static uint32_t uxs_ninc(int32_t p)
{
    uint32_t sh = 0, inc;
    while (p > 2047) {
        p -= 192;
        sh++;
    }
    inc = pitch_inc((uint32_t)clamp(p, 0, 2047));
    return sh && inc >= 0x80000000u >> (sh - 1u) ? 0xFFFFFFFFu : inc << sh;
}
static uint32_t uxs_isqrt(uint32_t x)
{
    uint32_t r = 0, b = 1u << 30;
    while (b > x)
        b >>= 2;
    while (b) {
        if (x >= r + b) {
            x -= r + b;
            r = (r >> 1) + b;
        } else
            r >>= 1;
        b >>= 2;
    }
    return r;
}
/* NOISE (noise_render): SOURCE ANLG (white, DENSITY's impulses), DUST (the impulses alone), LFSR (the register of LENGTH
 * clocked CLOCK semitones above the key), META (a short register's cycle a period of the key); COLOR white .. pink ..
 * brown; the filter at FREQ (KEY TRACK: none at C4), low-pass to band-pass as PEAK rises, its level made up; DRIFT a block
 * at a time (from its start: its wander takes seconds); CRUSH (ANLG, DUST) held and fewer bits */
static void uxs_wave_noise(const int16_t *p, int32_t *o, uint32_t pre, uint32_t len)
{
    vmod_t m;
    uxs_tap_t tp;
    uint32_t mode = (uint32_t)p[P_E0] & 3u, sel = ((uint32_t)p[P_E5] >> 4) & 7u, ph = 0, reg = 0, taps = NOISE_TAPS[sel], i;
    int32_t res = p[P_E3], drft = p[P_E6], crsh = mode < NZ_LFSR ? p[P_E7] : 0, ic1 = 0, ic2 = 0, b0 = 0, b1 = 0, b2 = 0;
    int32_t br = 0, s3 = 0, drift = 0, rng = (int32_t)((60u + 1u) * 0x9E3779B9u);   /* (noise_note_on's seed, the note C4) */
    uxs_vmod(p, &m);
    uxs_tap_on(&tp, o, pre, len);
    for (;;) {                                          /* a block of CTL: noise_render's set-up */
        int32_t d16 = 0, gain = 32767, gq, colr, cut, k, a1, a2, a3, g, bpg, mix, cf, thr = 0;
        uint32_t inc = 0;
        if (drft) {
            int32_t tg = s3 >> 16, cnt = s3 & 0xFFFF;
            if (--cnt <= 0) {
                uint32_t r = noise32(&rng);
                tg = (int32_t)(r >> 16) - 32768;
                cnt = 1024 + (int32_t)(r & 3071u);
            }
            s3 = (int32_t)((uint32_t)tg << 16 | (uint32_t)cnt);
            drift += ((tg << 4) - drift) >> 10;
            d16 = ((drift >> 11) * drft * drft) >> 14;
            gain = 32767 - ((((32767 - (drift >> 4)) >> 1) * drft) >> 8);
        }
        colr = clamp((p[P_E1] << 8) + m.shape - (64 << 8), 0, 127 << 8);
        cf = colr <= (64 << 8) ? colr << 1 : ((colr - (64 << 8)) * 520) >> 8;
        cut = (p[P_E2] << 8) + m.cutoff + (((m.pitch16 - 60 * 16) * p[P_E4] * 151) >> 10) + ((d16 * 4785) >> 8);
        cut = clamp(cut, 0, 127 << 8);
        g = SVF_G[cut >> 8];
        if (cut < (127 << 8))
            g += ((SVF_G[(cut >> 8) + 1] - g) * (cut & 255)) >> 8;
        k = 8192 - res * 7800 / 127;
        a1 = (int32_t)((4096u << 13) / (uint32_t)(4096 + ((g * (g + k)) >> 12)));
        a2 = (a1 * g) >> 12;
        a3 = (a2 * g) >> 12;
        bpg = (int32_t)uxs_isqrt((uint32_t)k << 12);
        mix = res * 258;
        {
            int32_t wf = 32767 - ((colr * 230) >> 8), mk = (((((127 << 8) - cut) >> 8) * 7) * wf) >> 15;
            mk += 214 + (mode == NZ_DUST ? 512 : 0);
            if (mk > 1279)
                mk = 1279;
            gq = (4096 + ((mk & 255) << 4)) << (mk >> 8);
            gq = (gq * (gain >> 3)) >> 12;
            if (gq > 65535)
                gq = 65535;
        }
        if (mode <= NZ_DUST && (p[P_E5] || mode == NZ_DUST)) {
            uint32_t dn = p[P_E5] ? (uint32_t)p[P_E5] : 48u, e = dn * 22u, r = 1u << (e >> 8);
            r += (r * (e & 255u)) >> 8;
            thr = (int32_t)((r * 97391u) >> 16);
        }
        if (mode == NZ_LFSR) {
            uint32_t mask = taps;
            inc = uxs_ninc(m.pitch16 + p[P_E7] * 16 + d16);
            mask |= mask >> 1;
            mask |= mask >> 2;
            mask |= mask >> 4;
            mask |= mask >> 8;
            mask |= mask >> 16;
            if (!(reg & mask))
                reg = (uint32_t)rng | 1u;
            reg &= mask;
        } else if (mode == NZ_META)
            inc = p[P_E7] || d16 ? uxs_ninc(m.pitch16 + p[P_E7] * 16 + d16) : m.inc;
        for (i = 0; i < CTL; i++) {
            int32_t w, x, v1, v2, v3, y;
            if (mode <= NZ_DUST) {
                uint32_t r = noise32(&rng);
                w = mode == NZ_ANLG ? (int32_t)(r >> 17) - 16384 : 0;
                if ((int32_t)(r & 0xFFFFu) < thr) {
                    int32_t a = 8192 + (int32_t)((r >> 18) & 0x7FFFu);
                    w += (r & 0x20000u) ? a : -a;
                }
            } else if (mode == NZ_LFSR) {
                uint32_t ol = ph;
                ph += inc;
                if (ph < ol)
                    reg = (reg >> 1) ^ (-(reg & 1u) & taps);
                w = (reg & 1u) ? 12000 : -12000;
            } else {
                uint32_t j = ((ph >> 16) * NOISE_SEQ[sel].len) >> 16;
                ph += inc;
                w = (NOISE_SEQ[sel].w[j >> 5] >> (j & 31u)) & 1u ? NOISE_SEQ[sel].hi : NOISE_SEQ[sel].lo;
            }
            x = w;
            if (colr) {
                int32_t pk;
                b0 = ((b0 * 32690) >> 15) + ((w * 3245) >> 15);
                b1 = ((b1 * 31555) >> 15) + ((w * 9716) >> 15);
                b2 = ((b2 * 18678) >> 15) + ((w * 17247) >> 14);
                pk = (b0 + b1 + b2 + ((w * 6055) >> 15)) >> 2;
                if (colr <= (64 << 8))
                    x = w + mulq15(pk - w, cf);
                else {
                    br = ((br * 32580) >> 15) + ((w * 3480) >> 15);
                    x = pk + mulq15(br - pk, cf);
                }
            }
            v3 = x - ic2;
            v1 = (a1 * ic1 + a2 * v3 + 4096) >> 13;
            v2 = ic2 + ((a2 * ic1 + a3 * v3 + 4096) >> 13);
            ic1 = clamp(2 * v1 - ic1, -150000, 150000);
            ic2 = clamp(2 * v2 - ic2, -150000, 150000);
            v1 = (v1 * bpg * 3) >> 13;
            y = v2 + (((v1 - v2) * (mix >> 3)) >> 12);
            y = soft_knee(((y >> 4) * gq) >> 8, 24000);
            if (mode == NZ_META) {
                reg += (uint32_t)(((y << 8) - (int32_t)reg) >> 9);
                y -= (int32_t)reg >> 8;
            }
            if (crsh) {
                if ((int32_t)--ph <= 0 || ph > 16u) {
                    ph = 1u + (uint32_t)crsh / 8u;
                    reg = (uint32_t)(y >> (crsh / 11) << (crsh / 11));
                }
                y = (int32_t)reg;
            }
            if (!uxs_tap(&tp, y))
                return;
        }
    }
}
/* VOICE: a glottal pulse a cycle (BUZZ), BREATH's noise, through the vowel's first two formants (the first cycle VOWEL's,
 * the second VOWEL 2's, where TALK takes it; WORDS: the middle one), moved by SHIFT, as narrow as RESO (Q) */
static void uxs_wave_voice(const int16_t *p, int32_t *o)
{
    int32_t f[3], b[3], j, a[2] = {0, 0}, bz = p[P_E4], br = p[P_E5];
    uint32_t i, ph[2] = {0, 0}, inc[2] = {0, 0}, rs = 362436069u, r = fmt_ratio(p[P_E3] * 16);
    int32_t dk[2] = {0, 0};
    for (i = 0; i < UXS_WN; i++) {
        int32_t s = 0;
        if (!(i & 63u)) {                                /* (the glottis opens: both ring anew, at this cycle's vowel) */
            vowel_at(p[P_VLANG] ? 64 << 8 : p[i ? P_E1 : P_E0] << 8, f, b);
            for (j = 0; j < 2; j++) {                    /* (a cycle of 64 samples at 110 Hz: 7040 a second) */
                inc[j] = (uint32_t)(((uint64_t)(uint32_t)f[j] * r >> 16) * 65536u / (16u * 7040u));
                dk[j] = 32768 - clamp((b[j] >> 4) * (192 - p[P_E6]) / 64, 8, 600) * 15;   /* (the decay a sample, Q15) */
                a[j] = 256 * bz / 2 + 4096;
                ph[j] = 0;
            }
        }
        for (j = 0; j < 2; j++) {
            s += (og_sin(ph[j]) * a[j]) >> (12 + j);
            ph[j] += inc[j];
            a[j] = (a[j] * dk[j]) >> 15;
        }
        o[i] = s + ((((int32_t)(uxs_rnd(&rs) >> 16) - 32768) * br) >> 9);
    }
}
/* the columns o[] scaled to the largest, -4096..4096 (Q12; silence flat) */
static void uxs_norm(const int32_t *o, int16_t *w)
{
    int32_t mx = 1;
    uint32_t i;
    for (i = 0; i < UXS_WN; i++)
        mx = o[i] > mx ? o[i] : -o[i] > mx ? -o[i] : mx;
    for (i = 0; i < UXS_WN; i++)
        w[i] = (int16_t)(mx > 64 ? (int64_t)o[i] * 4096 / mx : 0);
}
/* track t's picture (raw: ANALOG's oscillators alone), -4096..4096 */
static void uxs_wave(const track_t *t, int16_t *w, int raw)
{
    static int32_t o[UXS_WN];
    const engine_t *e = trk_engp(t);
    uint32_t i, pre, len;
    for (i = 0; i < UXS_WN; i++)
        o[i] = 0;
    uxs_win(e, t->p, &pre, &len);
    if (e == &ENG_ANALOG)
        uxs_wave_analog(t->p, o, pre, len, raw);
    else if (e == &ENG_PHASE)
        uxs_wave_phase(t->p, o, pre, len);
    else if (e == &ENG_LOFI)
        uxs_wave_lofi(t->p, o, pre, len);
    else if (e == &ENG_NOISE)
        uxs_wave_noise(t->p, o, pre, len);
    else if (e == &ENG_FORMANT)
        uxs_wave_voice(t->p, o);
    uxs_norm(o, w);
}
/* (3b task 4 fix, the user's YES) VOICE singing words (WORDS ES / EN / JA): the word it sings now (sing.c sing_view), else
 * "" (b holds 20 at least): the picture's top right says it, redrawn when it changes */
#define UXS_SINGS "SINGS "
static void uxs_word(const track_t *t, char *b, uint32_t nb)
{
    const sing_t *s;
    uint32_t i, n = 0;
    b[0] = 0;
    if (trk_engp(t) != &ENG_FORMANT || !t->p[P_VLANG] || nb < 2u)
        return;
    s = sing_view(t);
    for (i = 0; i < sizeof s->w && s->w[i] && n + 1u < nb; i++)   /* (the audio's: read once, a printable copy) */
        if (s->w[i] >= ' ' && s->w[i] <= '~')
            b[n++] = s->w[i];
    b[n] = 0;
}
/* the panel: the page's name small at the top left (VOICE's sung word at the right), the wave across it in KNOB 1's colour
 * over a dim fill, its axis */
#define UXS_WY 78                /* the wave's axis (the panel's 150 rows), its height either side */
#define UXS_WA 50
static void uxs_pic_head(const track_t *t, const char *name)   /* the name; the word VOICE sings */
{
    char w[20], b[32];
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "EDIT picture's name");
    cv_text_on(12, 8, &UF_LBL, name, UX_MID, ux_under(12, 12));
    if (t->p[P_VLANG] && trk_engp(t) == &ENG_FORMANT) {
        int32_t x;
        uxs_word(t, w, sizeof w);
        str_cpy(b, UXS_SINGS, sizeof b);
        str_cpy(b + str_len(b), w[0] ? w : "-", sizeof b - str_len(b));
        x = 228 - text_w(&UF_LBL, b);
        GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "EDIT picture's word");
        cv_text_on(x, 8, &UF_LBL, b, UX_TEXT, ux_under(x, 12));
    }
}
static void uxs_wave_draw(const track_t *t, const char *name, int raw)
{
    int16_t w[UXS_WN];
    int32_t i, px = 0, py = 0;
    uint16_t fill = ux_mix(UX_BG, UX_ACID, 22);
    uxs_wave(t, w, raw);
    uxs_pic_head(t, name);
    cv_rect(12, UXS_WY, 216, 1, UX_LINE);
    for (i = 0; i < (int32_t)UXS_WN; i++) {
        int32_t x = 12 + i * 216 / (int32_t)(UXS_WN - 1u), y = UXS_WY - w[i] * UXS_WA / 4096;
        if (y < UXS_WY)
            cv_rect(x, y, 2, UXS_WY - y, fill);
        else if (y > UXS_WY)
            cv_rect(x, UXS_WY + 1, 2, y - UXS_WY, fill);
        if (i)
            cv_line_t(px, py, x, y, UX_ACID, 2);
        px = x;
        py = y;
    }
}
/* FILTER (the part's): its response, TYPE at CUTOFF with RESO (ui_lzedit.c ed_resp, 62 its unity line: here the panel's
 * row 70), CUTOFF a line in KNOB 2's colour (DRIVE: its knob's value says it) */
static void uxs_filter_draw(const track_t *t, const page_t *pg, const char *name)
{
    int16_t *vp;
    const param_desc_t *d;
    int32_t r[3], x, py = 0, xc, k;
    uint32_t ty = (uint32_t)clamp(t->p[P_FTYPE], 0, 3);
    uint16_t fill = ux_mix(UX_BG, UX_SLIDE, 20);
    for (k = 0; k < 3; k++) {
        d = page_desc(pg, (uint32_t)k, &vp);
        r[k] = d && vp && d->max > d->min ? clamp(RATIO(d, enum_rank(d, *vp)), 0, 1000) : 0;
    }
    uxs_pic_head(t, name);
    xc = 10 + r[1] * 216 / 1000;
    for (x = 10; x <= 226; x++) {
        int32_t y = ed_resp(ty, x, xc, r[2]) + 8;
        cv_rect(x, y, 1, 113 - y, fill);
        if (x > 10)
            cv_line_t(x - 1, py, x, y, UX_ACID, 2);
        py = y;
    }
    cv_rect(xc, 14, 1, 99, UX_SLIDE);
}
/* the page's picture: MORE its list; the others (until Tasks 4..8 give each its own) the page's title over its four
 * values as bars in the knobs' colours */
static void uxs_picture(const page_t *pg, const ux_knob_t k[4])
{
    const aafont_t *f = &UF_TTL;
    int32_t w, x, i;
    if (pg->graph == GR_MORE) {
        ux_more_draw(pg->more, uxm_at[uxm_key()].sel, uxm_at[uxm_key()].top);
        return;
    }
    if (pg->fam == FAM_EDIT && !edit_kind()) {          /* (a synth: its wave; the filter's page its response) */
        if (pg->id[0] == P_FTYPE)
            uxs_filter_draw(TSEL, pg, nav_page_name());
        else
            uxs_wave_draw(TSEL, nav_page_name(), trk_engp(TSEL) == &ENG_ANALOG && pg->id[0] == P_E0);   /* (ANALOG's
                                                         * SOUND: its oscillators) */
        return;
    }
    w = text_w(f, pg->title);
    if (w > 224) {
        f = &UF_VAL;
        w = text_w(f, pg->title);
    }
    x = (240 - w) / 2;
    ux_text_grad(x, 30, f, pg->title, ux_under(x + w / 2, 40));
    for (i = 0; i < 4; i++) {
        int32_t y = 86 + i * 14, fw = (int32_t)k[i].bar * 200 / 255;
        cv_rect(20, y, 200, 6, UX_EMPTY);
        if (fw > 0 && k[i].label[0])
            cv_rect(20, y, fw, 6, ux_knob_col((uint32_t)i));
    }
}

static void ux_section_draw(void)
{
    static uint32_t head_sig, panel_sig, foot_sig;
    const page_t *pg = cur_page();
    uint32_t sec = nav_sec(), sig, all, c;
    ux_knob_t k[4];
    const char *st;
    uint16_t sc;
    char b[48];
    if (ui.frame != uxs_frame + 1u || sec != uxs_hint_sec || ui.page != uxs_hint_page)
        uxs_hint_sec = 0xFF;                            /* (the first page left: its hint gone) */
    uxs_frame = ui.frame;
    if (sec < 16u && !((uxs_seen >> sec) & 1u)) {       /* the section's first opening since power-on: its hint */
        uxs_seen |= (uint16_t)(1u << sec);
        uxs_hint_sec = (uint8_t)sec;
        uxs_hint_page = ui.page;
    }
    if (ux_knob_hot() >= 0)                             /* (a knob turned: the page found out) */
        uxs_hint_sec = 0xFF;
    dc_frame(ui.force);
    all = dc_all;
    {   /* the header: T3 . the sound, the transport */
        char nm[24], tr[16];
        ux_trk_name(song.sel % NTRK, nm, sizeof nm, 1);
        ux_transport(tr, sizeof tr);
        sig = str_hash(str_hash(song.sel + 1u, nm), tr);
        if (all || sig != head_sig) {
            head_sig = sig;
            ux_header(song.sel % NTRK + 1u, nm + 3, tr);   /* (ux_trk_name's "T3 " left out: the header adds it) */
        }
    }
    uxs_knobs(pg, k);
    ux_knobs(k, -1);
    sig = (uint32_t)(pg - PAGES) * 2654435761u ^ ux_glow_eff(1) * 977u;   /* (the panel: drawn when what it shows changed) */
    for (c = 0; c < 4u; c++)
        sig = str_hash(sig * 31u + k[c].bar, k[c].value);
    if (pg->graph == GR_MORE && pg->more) {             /* (MORE: the pick, the scroll, the rows shown and their values) */
        uint64_t m = uxm_mask(pg->more);
        uint32_t i;
        sig = sig * 131u + uxm_at[uxm_key()].sel * 7u + uxm_at[uxm_key()].top + (uint32_t)(m ^ (m >> 32));
        for (i = 0; i < uxm_n(pg->more); i++)
            if (uxm_on(m, i) && pg->more->rows[i].kind != UXR_HEAD) {
                int16_t *vp;
                if (param_ref(pg->more->rows[i].kind == UXR_GLOBAL ? 0 : TSEL, pg->more->rows[i].id, &vp) && vp)
                    sig = sig * 33u + (uint16_t)*vp;
            }
    }
    if (pg->fam == FAM_EDIT && !edit_kind() && pg->graph != GR_MORE) {   /* (a synth's wave: every setting it is drawn from) */
        static const uint8_t W[] = {P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7, P_MORPH, P_VLANG, P_FTYPE, P_FCUT, P_FRES,
                                    P_DIST, P_ED_FLT, P_ED_SHP};
        char wd[20];
        for (c = 0; c < sizeof W; c++)
            sig = sig * 31u + (uint16_t)TSEL->p[W[c]];
        sig = sig * 31u + trk_eng(TSEL);
        uxs_word(TSEL, wd, sizeof wd);                  /* (VOICE singing: the word, when it changes) */
        sig = str_hash(sig, wd);
    }
    if (all || sig != panel_sig) {
        panel_sig = sig;
        uxs_panels++;
        ux_panel_begin(1);
        uxs_picture(pg, k);
        ux_panel_end();
    }
    {   /* the footer: where, n / N and the knobs' names (when they fit) */
        const char *w = uxs_where();
        uxs_foot(w, k, uxs_count(), b, sizeof b);
        sig = str_hash(str_hash(1u, w), b);
        if (all || sig != foot_sig) {
            foot_sig = sig;
            ux_footer(w, b);
        }
    }
    st = uxs_status(pg, &sc);
    ux_status(st, sc);
}
