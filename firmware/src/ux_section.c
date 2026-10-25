/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's section pages on the screen (ux_section.h; 3b spec 2, parent spec 3.3): any PAGES row drawn with the 3a widgets.
 *   header   T3 . the sound, the transport at the right (ux_header)
 *   knobs    the PAGES row's four: the label in plain words (UXS_NAME, UXS_LABEL), the value as params.c param_format shows it, the
 *            bar its place in the range, KNOB 1..4 ACID SLIDE VIOLET ACCENT; MORE: KNOB 1 the setting picked (n/N), KNOB 2
 *            its value
 *   panel    the page's picture over the glow: a synth's EDIT its wave (its FILTER page the filter's response); MORE its
 *            list (ux_more.c); the sections still to come (3b tasks 5..8) their title and the four values as bars
 *   status   a message (STOP TO EDIT in ACCENT), else the knob just turned: its full name and value ("RELEASE . 420 ms"),
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
            k[1].label = uxm_row_text(&pg->more->rows[s], v, sizeof v, 0, &k[1].bar);
            if (pg->more->rows[s].kind == UXR_ACTION)
                str_cpy(v, "GO", sizeof v);
            str_cpy(k[1].value, v, sizeof k[1].value);
        }
        return;
    }
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, c, &vp);
        const char *u;
        char v[16];
        if (!d || !vp || !d->label || d->label[0] == '-')
            continue;
        param_format(d, *vp, v, &u);
        str_cpy(k[c].value, v, sizeof k[c].value);
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
                nm = uxm_row_text(&pg->more->rows[s], v, sizeof v, 1, &bar);
        } else if (pg->graph != GR_MORE) {
            int16_t *vp;
            const param_desc_t *d = page_desc(pg, (uint32_t)h, &vp);
            if (d && vp && d->label && d->label[0] != '-') {
                const char *u = "";
                param_format(d, *vp, v, &u);
                if (u && u[0]) {
                    str_cpy(v + str_len(v), " ", sizeof v - str_len(v));
                    str_cpy(v + str_len(v), u, sizeof v - str_len(v));
                }
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
/* (3b task 4, spec 2.1) a synth's EDIT pages draw its waveform: two cycles of the engine's shape from the track's stored
 * settings (the knobs' values: a turn redraws it, once; silence or playing, nothing per frame), a short render of the
 * oscillator as the old creatures' own math had it (ANALOG's two oscillators through its filter and DRIVE, PHASE's bent
 * cosine, LOFI's chip waves, its WRAM table, its BYTE / FLOAT formula (eng_lofi.c lofi_algo_at), NOISE's colour or its
 * register, VOICE's glottal pulses through its vowel's formants (eng_formant.c vowel_at)); FILTER the part's filter's
 * response (ui_lzedit.c ed_resp). Nothing here runs in the audio path */
#define UXS_WN 128u              /* two cycles of 64 */
static int32_t uxs_x2(int32_t l)                        /* 2^(l / 256), Q12 */
{
    int32_t i = l >> 8, f = l & 255, m = 4096 + ((f * (2690 + ((f * 1407) >> 8))) >> 8);
    return i >= 0 ? m << i : i > -31 ? m >> -i : 0;
}
static int32_t uxs_anb(uint32_t w, uint32_t q)          /* ANALOG's SAW SQR TRI SIN at phase q (16 bit), Q15 */
{
    q &= 65535u;
    w &= 3u;
    return w == 0u ? 32767 - (int32_t)q : w == 1u ? (q < 32768u ? 32767 : -32767) :
           w == 2u ? (q < 32768u ? (int32_t)q * 2 - 32768 : 98303 - (int32_t)q * 2) : og_sin(q);
}
static uint32_t uxs_rnd(uint32_t *r) { *r ^= *r << 13; *r ^= *r >> 17; *r ^= *r << 5; return *r; }
/* ANALOG: WAVE (MORPH towards the next), osc 2 at INT and DETUNE mixed in by MIX, NOISE, the filter (CUTOFF at the note
 * C4, RESO), DRIVE (tanh); a cycle for the filter to settle first */
static void uxs_wave_analog(const int16_t *p, int32_t *o)
{
    uint32_t w = (uint32_t)p[P_E0] & 7u, i, rep, r = 4096, rs = 2654435761u, inc2;
    int32_t det = p[P_E1], mix = (p[P_E2] * 516 >> 8), nz = (p[P_E3] * 361 >> 8), mo = clamp(p[P_MORPH] * 256 / 127, 0, 256), k;
    int32_t pos = (int32_t)w * 256 + mo * 3, fr = pos & 255, i0 = pos >> 8, g, a1, a2, a3, ic1 = 0, ic2 = 0, dg;
    int32_t r5 = 256 + det * 2022 / 127 * (256 - mo / 2) / 256 + mo * 3, d3 = det * 218 * (256 + 4 * mo) >> 8;
    uint32_t wd = (uint32_t)(32768 - (mo * 27525 >> 8));   /* (PWM's width) */
    for (k = p[P_E7]; k > 0; k--) r = (r * 4340u) >> 12;   /* (INTERVAL: osc 2 semitones away, DETUNE cents) */
    for (k = p[P_E7]; k < 0; k++) r = (r * 3866u) >> 12;
    r += r * (uint32_t)det / 1200u;
    inc2 = 1024u * r >> 12;
    g = clamp(uxs_x2(-992 - 1536 + ((p[P_E4] * 4851) >> 8) + 9 * 256 / 12), 4, 1843);
    g = (g * 12868) >> 12;
    g = clamp(g + (((((g * g) >> 12) * g) >> 12) / 3), 1, 12288);
    k = 8192 - (p[P_E5] * 16017 >> 8);
    a1 = (4096 * 4096) / (4096 + ((g * (g + k)) >> 12));
    a2 = (g * a1) >> 12;
    a3 = (g * a2) >> 12;
    dg = 256 + (p[P_E6] * 3612 >> 8);
    for (rep = 0; rep < 2u; rep++)
        for (i = rep ? 0u : 64u; i < UXS_WN; i++) {
            uint32_t P = i * 1024u, q = P & 65535u, q2 = (i * inc2) & 65535u;
            int32_t s, v1, v2, v3, u;
            if (w < 4u) {
                int32_t x1 = (uxs_anb((uint32_t)i0, q) * (256 - fr) + uxs_anb((uint32_t)i0 + 1u, q) * fr) >> 8;
                int32_t x2 = (uxs_anb((uint32_t)i0, q2) * (256 - fr) + uxs_anb((uint32_t)i0 + 1u, q2) * fr) >> 8;
                s = (x1 * (256 - mix / 2) + x2 * (mix / 2)) >> 8;
            } else if (w == 4u)
                s = ((q < wd ? 32767 : -32767) * (256 - mix / 2) + (q2 < wd ? 32767 : -32767) * (mix / 2)) >> 8;
            else if (w == 5u)
                s = (uxs_anb(0, q) * (256 - mix) + uxs_anb(0, (q * (uint32_t)r5) >> 8) * mix) >> 8;
            else if (w == 6u)
                s = (uxs_anb(0, q) * (256 - mix) + ((uxs_anb(0, q) * og_sin((P * (uint32_t)r5) >> 8)) >> 15) * mix) >> 8;
            else
                s = (uxs_anb(0, P) + uxs_anb(0, P + (((P >> 4) * (uint32_t)d3) >> 12)) +
                     uxs_anb(0, P - (((P >> 4) * (uint32_t)d3) >> 12))) / 3;
            s = (s + ((((int32_t)(uxs_rnd(&rs) >> 16) - 32768) * nz) >> 7)) >> 3;   /* (NOISE) Q12 */
            v3 = s - ic2;
            v1 = (a1 * ic1 + a2 * v3) >> 12;
            v2 = ic2 + ((a2 * ic1 + a3 * v3) >> 12);
            ic1 = clamp(2 * v1 - ic1, -150000, 150000);
            ic2 = clamp(2 * v2 - ic2, -150000, 150000);
            if (!rep)
                continue;
            u = clamp(((v2 * dg) >> 8) >> 4, -768, 768);   /* (DRIVE: tanh, Q8) */
            o[i] = (u * (1769472 + u * u) / (1769472 + 9 * u * u)) << 4;
        }
}
/* PHASE: the -cos of the phase bent by SHAPE (DCW) as eng_phase.c bends it, WAVE then WAVE 2 (none: WAVE again), SUB an
 * octave down */
static uint32_t uxs_pd(uint32_t w, uint32_t dcw, uint32_t ph)
{
    uint32_t x, pk;
    switch (w) {
    case 0: x = 32768u + ((dcw * 32767u) >> 16); return ph < x ? ph * 32768u / x : 32768u + (ph - x) * 32767u / (65535u - x);
    case 1:
        x = 32768u - ((dcw * 31120u) >> 16);
        return ph < x ? ph * 32768u / x : ph < 32768u ? 32768u : ph < 32768u + x ? 32768u + (ph - 32768u) * 32767u / x : 65535u;
    case 2: x = 65535u - ((dcw * 63487u) >> 16); return ph < x ? ph * 65535u / x : 65535u;
    case 3: x = 65535u - ((dcw * 49151u) >> 16); return ph < x ? ph * 65535u / x : (ph - x) * 65535u / (65535u - x);
    case 4:
        x = 65535u - ((dcw * 32767u) >> 16);
        pk = (x * (32768u + ((dcw * 29491u) >> 16))) >> 16;
        pk = pk ? pk : 1u;
        return ph >= x ? 65535u : ph < pk ? ph * 32768u / pk : 32768u + (ph - pk) * 32767u / (x - pk);
    default: return (ph + ((ph * ((dcw * 44000u) >> 16)) >> 16) * 7u) & 65535u;
    }
}
static void uxs_wave_phase(const int16_t *p, int32_t *o)
{
    uint32_t i, dcw = (uint32_t)clamp(p[P_E2], 0, 127) * 516u;
    for (i = 0; i < UXS_WN; i++) {
        uint32_t ph = (i & 63u) * 1024u, w = i < 64u || !p[P_E1] ? (uint32_t)p[P_E0] & 7u : (uint32_t)(p[P_E1] - 1) & 7u;
        int32_t c = -og_cos(uxs_pd(w, dcw, ph)), win;
        if (w >= 5u) {                                  /* (RSAW RTRI RTRP windowed) */
            win = w == 5u ? 65535 - (int32_t)ph : w == 6u ? (ph < 32768u ? (int32_t)ph * 2 : (65535 - (int32_t)ph) * 2) :
                  (ph < 16384u ? (int32_t)ph * 4 : ph > 49152u ? (65535 - (int32_t)ph) * 4 : 65535);
            c = (c * (win > 65535 ? 65535 : win)) >> 16;
        }
        o[i] = c + ((og_sin(i * 512u) * p[P_E6]) >> 8);   /* (SUB) */
    }
}
/* LOFI: its WAVE (PLS at DUTY, TRI, SAW, NOIS, a WRAM table, the BYTE / FLOAT formula with VARIANT) in CHIP's bits */
static void uxs_wave_lofi(const int16_t *p, int32_t *o)
{
    uint32_t i, w = (uint32_t)p[P_E1], chip = (uint32_t)p[P_E0], rs = 2463534242u, lv;
    int32_t h = 0;
    lv = chip == CHIP_8BIT || w == RW_FLOAT ? 256u : chip == CHIP_1BIT ? 2u : chip == CHIP_4B2 ? 8u : 16u;
    for (i = 0; i < UXS_WN; i++) {
        uint32_t ph = (i & 63u) * 1024u;
        int32_t s;
        if (w == RW_PLS)
            s = ph < 1024u + (uint32_t)clamp(p[P_E2], 0, 127) * 500u ? 32767 : -32767;
        else if (w == RW_TRI)
            s = ph < 32768u ? (int32_t)ph * 2 - 32768 : 98303 - (int32_t)ph * 2;
        else if (w == RW_SAW)
            s = (int32_t)ph - 32768;
        else if (w == RW_NOIS) {
            if (!(i & 3u))
                h = (int32_t)(uxs_rnd(&rs) >> 16) - 32768;
            s = h;
        } else if (w == RW_WRAM)
            s = (int32_t)lofi_wram_nib((uint32_t)p[P_E2] >> 3, (i & 63u) >> 1) * 4369 - 32767;
        else if (w == RW_BYTE)
            s = ((int32_t)(lofi_algo_at(0, (uint32_t)p[P_E2] >> 2, i * 8u, 1u + ((uint32_t)p[P_E3] >> 3)) & 255) - 128) * 256;
        else
            s = lofi_algo_at(1, (uint32_t)p[P_E2] >> 2, i * 8u, 0);
        s = clamp(s, -32767, 32767);
        if (lv < 256u) {                                /* (the chip's bits: its steps) */
            int32_t q = (int32_t)(((uint32_t)(s + 32768) * lv) >> 16);
            s = (int32_t)((q * 65535) / (int32_t)(lv - 1u)) - 32768;
        }
        o[i] = s;
    }
}
/* NOISE: ANLG / DUST a noise as dark as COLOR (DUST its impulses), LFSR / META the register's bits, held for CRUSH / CLOCK */
static void uxs_wave_noise(const int16_t *p, int32_t *o)
{
    uint32_t i, rs = 88172645u, reg = 1u, mode = (uint32_t)p[P_E0];
    int32_t y = 0, k = 4096 - clamp(p[P_E1], 0, 127) * 30, hold = 1 + p[P_E7] / 16, h = 0;
    for (i = 0; i < UXS_WN; i++) {
        int32_t x = (int32_t)(uxs_rnd(&rs) >> 16) - 32768;
        if (mode >= 2u) {                               /* LFSR, META: a 9-bit register, a bit a step */
            if (!(i % (uint32_t)hold)) {
                reg = (reg >> 1) ^ (-(reg & 1u) & 0x110u);
                h = reg & 1u ? 30000 : -30000;
            }
            o[i] = h;
            continue;
        }
        if (mode == 1u)                                 /* DUST: a few impulses */
            x = (x & 0x7E00) == 0x7E00 ? x * 4 : 0;
        y += ((x - y) * k) >> 12;                        /* (COLOR: white .. brown) */
        o[i] = y;
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
/* the synth's two cycles, -4096..4096 (Q12, scaled to the largest; silence flat) */
static void uxs_wave(const track_t *t, int16_t *w)
{
    static int32_t o[UXS_WN];
    const engine_t *e = trk_engp(t);
    int32_t mx = 1;
    uint32_t i;
    for (i = 0; i < UXS_WN; i++)
        o[i] = 0;
    if (e == &ENG_ANALOG)
        uxs_wave_analog(t->p, o);
    else if (e == &ENG_PHASE)
        uxs_wave_phase(t->p, o);
    else if (e == &ENG_LOFI)
        uxs_wave_lofi(t->p, o);
    else if (e == &ENG_NOISE)
        uxs_wave_noise(t->p, o);
    else if (e == &ENG_FORMANT)
        uxs_wave_voice(t->p, o);
    for (i = 0; i < UXS_WN; i++)
        mx = o[i] > mx ? o[i] : -o[i] > mx ? -o[i] : mx;
    for (i = 0; i < UXS_WN; i++)
        w[i] = (int16_t)(mx > 64 ? (int64_t)o[i] * 4096 / mx : 0);
}
/* the panel: the page's name small at the top left, the wave across it in KNOB 1's colour over a dim fill, its axis */
#define UXS_WY 78                /* the wave's axis (the panel's 150 rows), its height either side */
#define UXS_WA 50
static void uxs_wave_draw(const track_t *t, const char *name)
{
    int16_t w[UXS_WN];
    int32_t i, px = 0, py = 0;
    uint16_t fill = ux_mix(UX_BG, UX_ACID, 22);
    uxs_wave(t, w);
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "EDIT picture's name");
    cv_text_on(12, 8, &UF_LBL, name, UX_MID, ux_under(12, 12));
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
 * row 70), CUTOFF a line in KNOB 2's colour, DRIVE a row of cells in KNOB 4's */
static void uxs_filter_draw(const track_t *t, const page_t *pg, const char *name)
{
    int16_t *vp;
    const param_desc_t *d;
    int32_t r[4], x, py = 0, xc, k, nd;
    uint32_t ty = (uint32_t)clamp(t->p[P_FTYPE], 0, 3);
    uint16_t fill = ux_mix(UX_BG, UX_SLIDE, 20);
    for (k = 0; k < 4; k++) {
        d = page_desc(pg, (uint32_t)k, &vp);
        r[k] = d && vp && d->max > d->min ? clamp(RATIO(d, enum_rank(d, *vp)), 0, 1000) : 0;
    }
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "EDIT picture's name");
    cv_text_on(12, 8, &UF_LBL, name, UX_MID, ux_under(12, 12));
    xc = 10 + r[1] * 216 / 1000;
    for (x = 10; x <= 226; x++) {
        int32_t y = ed_resp(ty, x, xc, r[2]) + 8;
        cv_rect(x, y, 1, 113 - y, fill);
        if (x > 10)
            cv_line_t(x - 1, py, x, y, UX_ACID, 2);
        py = y;
    }
    cv_rect(xc, 14, 1, 99, UX_SLIDE);
    nd = r[3] * 20 / 1000;
    for (k = 0; k < 20; k++)
        cv_rect(60 + k * 6, 126, 4, 10, k < nd ? UX_ACCENT : UX_EMPTY);
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
            uxs_wave_draw(TSEL, nav_page_name());
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
                                    P_DIST};
        for (c = 0; c < sizeof W; c++)
            sig = sig * 31u + (uint16_t)TSEL->p[W[c]];
        sig = sig * 31u + trk_eng(TSEL);
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
