/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's section pages on the screen (ux_section.h; 3b spec 2, parent spec 3.3): any PAGES row drawn with the 3a widgets.
 *   header   T3 . the sound, the transport at the right (ux_header)
 *   knobs    the PAGES row's four: the label in plain words (UXS_NAME, UXS_LABEL), the value with its unit (uxs_val: "1.9K",
 *            "1.0MS"; MORE's rows and the status line the same), the bar its place in the range, KNOB 1..4 ACID SLIDE VIOLET
 *            ACCENT; MORE: KNOB 1 the setting picked (n/N), KNOB 2 its value
 *   panel    the page's picture over the glow (3b task 6: ENV's envelope, ARP's notes, SCL's keys and chord, below): a
 *            synth's EDIT what it sounds like (the engine's signal path, ported; its
 *            FILTER page the filter's response; VOICE singing words the word); FM6's its algorithm (the picked operator
 *            outlined; OP ENV its envelope); DRUM-X's the picked drum's hit (dx_run) and the eight drums; MORE its
 *            list (ux_more.c); the sections still to come (3b tasks 7, 8) their title and the four values as bars
 *   status   a message (STOP TO EDIT in ACCENT), else the knob just turned: its full name and value ("RELEASE . 420MS"),
 *            else the section's first-time hint ("ARP AGAIN: NEXT PAGE", once a power-on, spec 0 rule 7), else on MORE
 *            what its controls do
 *   footer   SECTION . PAGE at the left; at the right n / N of the section's chain (N counts MORE) and NEXT the page the
 *            section's button goes to (spec 0 rule 1: where the next press goes; the knob row's names not again), MORE its
 *            knobs' names (SETTING, the row picked). One rule on every page drawn here (3b task 5 fix)
 * Strips as HOME's (dc_frame / cv_blit_cells): only the changed cells go out. Included after ux_more.c (and ux_home.c:
 * ux_under, ux_trk_name). */

/* a setting's plain name (spec 0 rule 4: one name everywhere it shows, the knob row, the status line, MORE's rows, the footer),
 * by the section showing it, the track's engine, global or the track's, and its id: where JIANT's label is cryptic or names
 * more than one thing (MODE: the arp's, the glide's, the slicer's, the LFO's). UXS_ALL: every section / engine; the most
 * specific row wins (its section over every section's, then its engine over every engine's). JIANT's labels themselves
 * (TP / GP: the web editor reads them, editor.c) stay as they are. Each fits a knob's column in UF_LBL (tests/ui_test.c) */
typedef char uxs_sec_edit_ok[SEC_EDIT == UXS_SEC_EDIT && SEC_ENV == UXS_SEC_ENV && SEC_SCL == UXS_SEC_SCL &&
                            SEC_ARP == UXS_SEC_ARP ? 1 : -1];   /* (ux_section.h's sections are ui_lens.c's) */
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
    {UXS_ALL, ENGI_FM6, 0, P_E0, "ALGORITHM"},          /* (3b task 5) FM6's ALG; FB is FEEDBACK (UXS_LABEL) */
    {UXS_ALL, ENGI_FM6, 0, P_E2, "MOD LEVEL"},          /* .. MOD (MLVL): the modulators' level, the brightness */
    {UXS_ALL, ENGI_FM6, 0, P_E3, "MOD RATIO"},          /* .. RATIO (MRAT): the modulators' RATIO moved (an operator's is RATIO) */
    {UXS_ALL, ENGI_FM6, 0, P_E4, "MOD TIME"},           /* .. MEG: the modulators' envelopes slower / faster */
    {UXS_ALL, ENGI_FM6, 0, P_E5, "MOD VEL"},            /* .. VMOD: the modulators' VELOCITY moved */
    {UXS_ALL, ENGI_FM6, 0, P_E7, "PATCH"},              /* .. SLOT: the patch, F1..F8 OWN B1..B32 */
    {UXS_ALL, ENGI_DRUM, 0, P_E5, "CLICK"},             /* .. DRUM-X's FM ENV: the hit's beater, click, metal edge */
    {UXS_ALL, ENGI_DRUM, 0, P_DXIMG, "STEREO"},         /* .. IMAGE: the kit across the stereo field */
    {UXS_ALL, ENGI_DRUM, 0, ID_DX_SNAP, "CRACK"},       /* (3b task 6 fix) .. a drum's SNAP: its noise a quarter as long
                                                         * (drumx_voice.c; SNAP is SCL's: the keys onto the scale) */
    /* (3b task 6) ARP's, SCL's */
    {UXS_ALL, UXS_ALL, 0, P_AMODE, "ARP"},              /* ARP's MODE: ARP OFF / UP / DN .. (MODE names four things) */
    {UXS_ALL, UXS_ALL, 0, P_AOCT, "OCTAVES"},           /* .. OCT: the octaves it runs over */
    {UXS_ALL, UXS_ALL, 0, P_APROB, "CHANCE"},           /* .. PROB: each note's chance (SEQ's CHANCE's word) */
    {UXS_ALL, UXS_ALL, 0, P_QUANT, "SNAP"},             /* SCL's QUANT: the keys onto the scale (OFF SNAP WHITE SEQ) */
    {UXS_ALL, UXS_ALL, 0, ID_ALLTR, "ALL TRACKS"},      /* .. ALL TR: every sequence a number of scale steps up / down */
    {UXS_ALL, UXS_ALL, 1, G_STRN, "ALL TRACKS"},        /* .. (the same global, where a page names it as one) */
    {UXS_ALL, UXS_ALL, 0, P_POFS, "SEQ PITCH"},         /* .. SEQ > TRACK's PITCH: the sequence's notes, semitones */
    {UXS_ALL, UXS_ALL, 0, P_TRANS, "TRANSPOSE"},        /* .. TRANS: every note the track plays, semitones */
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
    if (!glo && id >= P_LN0 && id <= P_LN7 && trk_eng(TSEL) == ENGI_DRUM)
        return "LEVEL";                                 /* (3b task 5) DRUM-X's SOUND: the drum picked's (its name: KNOB 1) */
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

/* (3b task 5 fix) setting id's value v (d its descriptor) as shown: uxs_val's; FM6's ALGORITHM at 0 (JIANT's PAT: the patch's own
 * algorithm plays) "P" and that algorithm's number, "P 5" (PAT named a pattern elsewhere; the status line adds UXS_ALGP_SAY) */
static const char *const UXS_ALGP[] = {"P "};
#define UXS_ALGP_SAY " (THE PATCH'S)"
static int uxs_algp(uint32_t glo, uint32_t id, int32_t v) { return !glo && id == P_E0 && !v && trk_eng(TSEL) == ENGI_FM6; }
/* (3b task 6) the ARP's pattern on the knob where JIANT's word is wider than its column (REPEAT, THUMB) or the same as another
 * (ORD plays as UP: ui_input.c param_knob steps over it); the status line says each in full (uxs_vfull), and SCALE's */
static const char *const UXS_AMODE_K[16] = {0, 0, 0, 0, 0, "UP", "REPT", 0, 0, 0, 0, 0, "THMB", 0, 0, 0};
static const char *const UXS_AMODE_FULL[16] = {"OFF", "UP", "DOWN", "UP-DOWN", "RANDOM", "UP", "REPEAT", "DOWN-UP",
                                               "UP + OCTAVE", "CONVERGE", "DIVERGE", "PINKY", "THUMB", "WALK", "CHORD",
                                               "KEYS MOVE THE SEQUENCE"};
static const char *const UXS_SCALE_FULL[16] = {"CHROMATIC", "MAJOR", "MINOR", "DORIAN", "MIXOLYDIAN", "PENTATONIC",
                                               "MINOR PENT", "HARMONIC MINOR", "PHRYGIAN", "LYDIAN", "LOCRIAN",
                                               "MELODIC MINOR", "BLUES", "WHOLE TONE", "DIMINISHED H-W", "DIMINISHED W-H"};
/* (3b task 6 fix) SNAP's values in plain words, as the KEY picture's line says each (UXS_SNAP_SAY: JIANT's QUANT OFF SNAP
 * WHITE SEQ: "SNAP . SNAP" said nothing, SEQ not what it does); ALL TRACKS' (G_STRN, a number of scale steps) OFF at 0, else
 * signed, "+2" ("+2 STEPS" in the status line: ST is semitones) */
static const char *const UXS_QUANT_K[4] = {"OFF", "KEYS", "WHITE", "BOTH"};   /* (BOTH: the keys and the sequence; KEYS+SEQ
                                                         * is 59 px) */
static const char *const UXS_QUANT_FULL[4] = {"OFF", "KEYS SNAP", "WHITE KEYS", "KEYS + SEQUENCE"};
static const char *const UXS_STEPS[] = {" STEP", " STEPS"};
static int uxs_is_strn(uint32_t glo, uint32_t id) { return glo ? id == G_STRN : id == ID_ALLTR; }
static void uxs_strn(int32_t v, char *b, uint32_t nb, int full)   /* ALL TRACKS' v: "OFF", "+2", "-3" (full: "+2 STEPS") */
{
    if (!v) {
        str_cpy(b, UXS_QUANT_K[0], nb);
        return;
    }
    str_cpy(b, v > 0 ? "+" : "-", nb);
    fmt_int(b + str_len(b), v > 0 ? v : -v);
    if (full)
        str_cpy(b + str_len(b), UXS_STEPS[v != 1 && v != -1], nb - str_len(b));
}
static const char *uxs_vfull(uint32_t glo, uint32_t id, int32_t v)   /* value v of setting id in full (0: as the knob's) */
{
    static char b[16];
    if (uxs_is_strn(glo, id)) {
        uxs_strn(v, b, sizeof b, 1);
        return b;
    }
    if (glo || v < 0 || v > 15)
        return 0;
    return id == P_AMODE ? UXS_AMODE_FULL[v] : id == P_SCALE ? UXS_SCALE_FULL[v] : id == P_QUANT && v < 4 ? UXS_QUANT_FULL[v] : 0;
}
static void uxs_vtext(const param_desc_t *d, int32_t v, uint32_t glo, uint32_t id, char *b, uint32_t nb)
{
    if (!glo && id == P_AMODE && v >= 0 && v < 16 && UXS_AMODE_K[v]) {
        str_cpy(b, UXS_AMODE_K[v], nb);
        return;
    }
    if (!glo && id == P_QUANT && v >= 0 && v < 4) {
        str_cpy(b, UXS_QUANT_K[v], nb);
        return;
    }
    if (uxs_is_strn(glo, id)) {
        uxs_strn(v, b, nb, 0);
        return;
    }
    if (!uxs_algp(glo, id, v)) {
        uxs_val(d, v, b, nb);
        return;
    }
    str_cpy(b, UXS_ALGP[0], nb);
    fmt_int(b + str_len(b), (int32_t)(fm6_patch[song.sel % NTRK][FP_ALG] & 31u) + 1);
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
        uxs_vtext(d, *vp, pg->scope == SC_GLOBAL, page_id(pg, c), k[c].value, sizeof k[c].value);
        k[c].label = uxs_label(nav_sec(), pg->scope == SC_GLOBAL, page_id(pg, c), d->label);
        if (d->max > d->min) {
            int32_t b = clamp(((int32_t)*vp - d->min) * 255 / (d->max - d->min), 0, 255);
            k[c].bar = (uint8_t)(d->fmt == F_PCTINV ? 255 - b : b);
        }
    }
}

/* the page the section's button goes to from here, by its name in the chain ("OPERATOR"); 0: none (ui_lens.c nav_next) */
static const char *uxs_next(void)
{
    static char b[24];
    uint32_t sec = nav_sec(), e;
    if (sec == SEC_HOME || (e = nav_next(sec)) == 0xFFu || e == nav_cur())
        return 0;
    str_cpy(b, nv_name(sec, e, nv_page(sec, e)), sizeof b);
    return b;
}
/* the footer's right (3b task 5 fix: one rule, spec 0 rule 1): cnt ("1 / 4"), then NEXT and next, the page the section's
 * button goes to ("1 / 4 . NEXT OPERATOR"); on MORE its knobs' names instead ("4 / 4 . SETTING . CUTOFF": what KNOB 1 picks
 * and KNOB 2 sets). The count alone when nothing follows or it would not fit beside w, the footer's left */
static const char *const UXS_FOOT[] = {" \xB7 NEXT ", " \xB7 "};
static void uxs_foot(const char *w, const ux_knob_t k[4], const char *cnt, const char *next, char *b, uint32_t nb)
{
    char t[48];
    uint32_t c, n = 0;
    str_cpy(b, cnt, nb);
    str_cpy(t, cnt, sizeof t);
    if (str_eq(k[0].label, UXS_PICK)) {                 /* (MORE) */
        for (c = 0; c < 4u; c++)
            if (k[c].label[0]) {
                str_cpy(t + str_len(t), UXS_FOOT[1], sizeof t - str_len(t));
                str_cpy(t + str_len(t), k[c].label, sizeof t - str_len(t));
                n++;
            }
    } else if (next && next[0]) {
        str_cpy(t + str_len(t), UXS_FOOT[0], sizeof t - str_len(t));
        str_cpy(t + str_len(t), next, sizeof t - str_len(t));
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
                uint32_t id = page_id(pg, (uint32_t)h);
                uxs_vtext(d, *vp, pg->scope == SC_GLOBAL, id, v, sizeof v);
                if (uxs_vfull(pg->scope == SC_GLOBAL, id, *vp))   /* (3b task 6: "ARP . REPEAT", "SCALE . MINOR") */
                    str_cpy(v, uxs_vfull(pg->scope == SC_GLOBAL, id, *vp), sizeof v);
                nm = uxs_label(sec, pg->scope == SC_GLOBAL, id, d->label);
                if (uxs_algp(pg->scope == SC_GLOBAL, id, *vp))
                    str_cpy(v + str_len(v), UXS_ALGP_SAY, sizeof v - str_len(v));
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
static int32_t uxs_cols[UXS_WN];                       /* (the pictures' columns: a synth's wave, DRUM-X's hit; one at a time) */
/* track t's picture (raw: ANALOG's oscillators alone), -4096..4096 */
static void uxs_wave(const track_t *t, int16_t *w, int raw)
{
    int32_t *o = uxs_cols;
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
/* ------------------------------------------------------------------------------ FM6's and DRUM-X's pictures --- */
/* (3b task 5) FM6: the algorithm (ui_graph.c FM6_CELL: each operator's place, the carriers on the bottom row; fm6_routes,
 * fm6_car_ops, fm6_fb_op: fm6_core.c's FM6_ALG itself) as dots in the blend's colours, numbered, each as large as its level
 * (a modulator's with MOD LEVEL on it; silent: a ring), joined by lines: a modulator's line waves as many times as its ratio
 * (MOD RATIO on it; FIXED: straight), the carriers' down to the output, the feedback a loop on its operator as large as
 * FEEDBACK (0: a hairline). OPERATOR: the operator picked outlined, its ratio top right ("OP 3 x1.50", "OP 3 440HZ").
 * OP ENV: the picked operator's envelope (L4 to L1 at R1, L2 at R2, L3 at R3, held, back to L4 at R4: a stage as wide as
 * its rate is slow), the stage picked in KNOB 2's colour. All from the patch and the knobs: a turn redraws it, once */
#ifndef UXS_FM6_HOOK
#define UXS_FM6_HOOK(k, x, y, r) ((void)0)              /* (tests/ui_render.c: each operator as drawn, Q4 px: its lint) */
#endif
#define UXS_FY0 38               /* the operators' rows: the top one's y at most, the carriers' */
#define UXS_FY1 112
#define UXS_FBUS 134             /* the output */
static int32_t uxs_fop_lvl(const uint8_t *op, int32_t mod, int32_t mlvl)   /* an operator's level drawn, 0..99 */
{
    int32_t l = op[FP_OL] % 100u;
    return l && mod ? clamp(l + mlvl * 3 / 4, 1, 99) : l;   /* (MOD LEVEL: -36 .. +36 dB, 3/4 dB a step of LEVEL) */
}
static int32_t uxs_fop_ratio(const uint8_t *op, int32_t mrat, int mod)   /* its ratio x100 (FIXED: 0), MOD RATIO on a modulator */
{
    int32_t c = op[FP_FC] & 31;
    if (op[FP_MODE])
        return 0;
    if (mod)
        c = clamp(c + mrat, 0, 31);
    return (c ? c * 100 : 50) * (100 + op[FP_FF] % 100u) / 100;
}
/* "x1.50" (ratio x100), "440HZ" (FIXED: as fm6_core.c fm6_op_freq) */
static void uxs_fop_freq(const uint8_t *op, char *b, uint32_t nb)
{
    char t[16];
    uint32_t i, j = 0;
    fop_freq(op, t);                                    /* ("x1.50", "440 HZ", "1.00 KHZ") */
    for (i = 0; t[i] && j + 1u < nb; i++)
        if (t[i] != ' ')
            b[j++] = t[i] == 'K' && t[i + 1] == 'H' ? 'K' : t[i];
    b[j] = 0;
}
static void uxs_wave_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t cyc, uint16_t c)
{
    int32_t i, px = x0, py = y0, dx = x1 - x0, dy = y1 - y0, len = 1, n = 24;
    for (len = 1; len * len < dx * dx + dy * dy; len++)
        ;
    for (i = 1; i <= n; i++) {
        int32_t w = cyc ? (og_sin((uint32_t)(i * cyc * 65536 / n)) * 3) >> 15 : 0;   /* (3 px either side) */
        int32_t x = x0 + dx * i / n - dy * w / len, y = y0 + dy * i / n + dx * w / len;
        cv_line_t(px, py, x, y, c, 1);
        px = x;
        py = y;
    }
}
static void uxs_fm6_env(const track_t *t, const char *name)
{
    const uint8_t *op = fop_bytes(song.sel);
    int32_t x[6], y[6], k, w, sg = fop_stg & 3u;
    uint16_t fill = ux_mix(UX_BG, UX_ACID, 20);
    char b[24];
    (void)t;
    x[0] = 14;
    y[0] = 120 - op[FP_L1 + 3] % 100u * 90 / 99;        /* (from L4) */
    for (k = 0; k < 4; k++) {                           /* R1 L1, R2 L2, R3 L3, held, R4 L4 */
        int32_t r = op[FP_R1 + (k < 3 ? k : 3)] % 100u, l = op[FP_L1 + k] % 100u;
        w = 6 + (99 - r) * (99 - r) * 40 / 9801;
        if (k == 3) {                                   /* (the key held: SUS's level on) */
            x[4] = x[3] + 26;
            y[4] = y[3];
            x[5] = x[4] + w;
            y[5] = 120 - l * 90 / 99;
        } else {
            x[k + 1] = x[k] + w;
            y[k + 1] = 120 - l * 90 / 99;
        }
    }
    for (k = 0; k < 5; k++) {                           /* the fill, then the line: the stage picked in KNOB 2's colour */
        int32_t xx;
        for (xx = x[k]; xx < x[k + 1]; xx++) {
            int32_t yy = y[k] + (y[k + 1] - y[k]) * (xx - x[k]) / (x[k + 1] - x[k] ? x[k + 1] - x[k] : 1);
            cv_rect(xx, yy, 1, 121 - yy, fill);
        }
    }
    cv_rect(14, 121, 212, 1, UX_LINE);
    for (k = 0; k < 5; k++) {
        int st = k < 3 ? k : k == 4 ? 3 : -1;           /* (k 3: the hold) */
        cv_line_t(x[k], y[k], x[k + 1], y[k + 1], st == sg ? UX_SLIDE : UX_ACID, st == sg ? 3 : 2);
    }
    uxs_pic_head(t, name);
    str_cpy(b, "OP ", sizeof b);
    str_cpy(b + 3, N_FO_OP[fop_op % 6u], sizeof b - 3);
    str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), N_FO_STG[sg], sizeof b - str_len(b));
    k = 228 - text_w(&UF_LBL, b);
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "EDIT picture's word");
    cv_text_on(k, 8, &UF_LBL, b, UX_TEXT, ux_under(k, 12));
}
static void uxs_fm6_draw(const track_t *t, const page_t *pg, const char *name)
{
    uint32_t tr = song.sel % NTRK, a = fm6_alg_of(t), car = fm6_car_ops(a), fbo = fm6_fb_op(a), k, j, hi = 0, top = 0;
    const uint8_t *p = fm6_patch[tr];
    uint8_t m[6];
    int32_t x[6], y[6], r[6], lv[6], dx, dy, fb, cl = 999, cr = -999, pick = pg->graph == GR_FM6P ? (int32_t)(fop_op % 6u) : -1;
    char b[24];
    if (pg->graph == GR_FM6P && pg->id[1] == ID_FO_STG) {
        uxs_fm6_env(t, name);
        return;
    }
    fm6_routes(a, m);
    for (k = 0; k < 6u; k++) {
        hi = (FM6_CELL[a][k] & 7u) > hi ? FM6_CELL[a][k] & 7u : hi;
        top = (uint32_t)(FM6_CELL[a][k] >> 4) > top ? (uint32_t)FM6_CELL[a][k] >> 4 : top;
    }
    dx = hi ? (190 / (int32_t)hi < 52 ? 190 / (int32_t)hi : 52) : 0;
    dy = top ? ((UXS_FY1 - UXS_FY0) / (int32_t)top < 37 ? (UXS_FY1 - UXS_FY0) / (int32_t)top : 37) : 0;
    for (k = 0; k < 6u; k++) {
        const uint8_t *op = p + (5u - k) * FP_OP;
        x[k] = 120 + ((int32_t)(FM6_CELL[a][k] & 7u) * 2 - (int32_t)hi) * dx / 2;
        y[k] = UXS_FY1 - (int32_t)(FM6_CELL[a][k] >> 4) * dy;
        lv[k] = uxs_fop_lvl(op, !((car >> k) & 1u), t->p[P_E2]);
        r[k] = 6 + lv[k] * 5 / 99;
        if ((car >> k) & 1u) {
            cl = x[k] < cl ? x[k] : cl;
            cr = x[k] > cr ? x[k] : cr;
        }
    }
    uxs_pic_head(t, name);
    cv_rect(cl, UXS_FBUS, cr + 14 - cl, 1, UX_MID);     /* the output: the carriers' line, an arrow out */
    cv_line_t(cr + 10, UXS_FBUS - 3, cr + 14, UXS_FBUS, UX_MID, 1);
    cv_line_t(cr + 10, UXS_FBUS + 3, cr + 14, UXS_FBUS, UX_MID, 1);
    for (k = 0; k < 6u; k++) {
        if ((car >> k) & 1u)
            cv_rect(x[k], y[k] + r[k], 1, UXS_FBUS - y[k] - r[k], lv[k] ? UX_MID : UX_LINE);
        for (j = 0; j < 6u; j++)                        /* (j modulates k: its ratio waves along the line) */
            if ((m[k] >> j) & 1u) {
                int32_t rt = uxs_fop_ratio(p + (5u - j) * FP_OP, t->p[P_E3], 1);
                uxs_wave_line(x[j], y[j] + r[j], x[k], y[k] - r[k], rt ? clamp((rt + 50) / 100, 1, 12) : 0,
                              lv[j] ? ux_mix(UX_LINE, UX_BLEND[j * 51u], 30 + lv[j] * 70 / 99) : UX_LINE);
            }
    }
    fb = clamp(p[FP_FB] + t->p[P_E1], 0, 7);            /* FEEDBACK: a loop, as large as it is, from the operator it is taken */
    {                                                   /* from (FM6_ALG's last FBOUT) into the one it feeds (FBIN): */
        uint32_t src = fbo;                             /* (algorithms 4 and 6: from OP 4 / OP 5 back to OP 6) */
        int32_t Xs, Ys, Rs, X = x[fbo], Y = y[fbo], R = r[fbo], w = 5 + fb * 2, h = 4 + fb, xm;
        uint16_t c = fb ? UX_ACCENT : UX_LINE;
        int32_t th = fb >= 4 ? 2 : 1;
        for (k = 0; k < 6u; k++)                        /* (run sixth first: the last FBOUT is what is fed back) */
            if (FM6_ALG[a][k] & FM6_FBOUT)
                src = 5u - k;
        Xs = x[src]; Ys = y[src]; Rs = r[src];
        xm = (Xs + Rs > X + R ? Xs + Rs : X + R) + w;
        cv_line_t(Xs + Rs, Ys, xm, Ys, c, th);
        cv_line_t(xm, Ys, xm, Y - R - h, c, th);
        cv_line_t(xm, Y - R - h, X, Y - R - h, c, th);
        cv_line_t(X, Y - R - h, X, Y - R, c, th);
    }
    for (k = 0; k < 6u; k++) {                          /* the operators, numbered */
        char n[2] = {(char)('1' + k), 0};
        uint16_t c = UX_BLEND[k * 51u];
        int32_t i, nx = x[k] - text_w(&UF_LBL, n) / 2;
        if (lv[k]) {
            for (i = -r[k]; i <= r[k]; i++) {           /* (a disc) */
                int32_t hw = 0;
                while ((hw + 1) * (hw + 1) + i * i <= r[k] * r[k])
                    hw++;
                cv_rect(x[k] - hw, y[k] + i, 2 * hw + 1, 1, ux_mix(UX_BG, c, 45 + lv[k] * 55 / 99));
            }
            cv_text_on(nx, y[k] - 5, &UF_LBL, n, UX_BG, ux_mix(UX_BG, c, 45 + lv[k] * 55 / 99));
        } else {
            og_ring(x[k] * 16, y[k] * 16, r[k] * 16, UX_MID);
            cv_text_on(nx, y[k] - 5, &UF_LBL, n, UX_MID, ux_under(nx, y[k]));
        }
        if ((int32_t)k == pick)                         /* (OPERATOR: the operator picked) */
            og_ring(x[k] * 16, y[k] * 16, (r[k] + 3) * 16, UX_TEXT);
        UXS_FM6_HOOK((int32_t)k, x[k] * 16, y[k] * 16, r[k] * 16);
    }
    if (pick >= 0) {                                    /* its ratio (FIXED: its frequency) */
        int32_t tx;
        str_cpy(b, "OP ", sizeof b);
        str_cpy(b + 3, N_FO_OP[pick], sizeof b - 3);
        str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
        uxs_fop_freq(p + (5u - (uint32_t)pick) * FP_OP, b + str_len(b), sizeof b - str_len(b));
        tx = 228 - text_w(&UF_HDR, b);
        GFX_HOOK_ALIGN(0, 6 + UF_HDR_CAP_Y, 0, 6 + UF_HDR_CAP_Y + UF_HDR_CAP_H, AL_V | AL_PASS, "EDIT picture's word");
        cv_text_on(tx, 6, &UF_HDR, b, UX_TEXT, ux_under(tx, 12));
    }
}

/* DRUM-X: the drum picked's hit (its stroke as the engine plays it: drumx_voice.c dx_run on a scratch hit, from the kit and
 * the track's knobs, MORPH where KIT has it, no LFO, then its LEVEL and the kit's DRIVE as eng_drum.c drum_render and
 * drum_bus do), a fifth of a second, its envelope over the panel; under it the eight drums as cells, each its LEVEL as a
 * bar, the one picked lit, a muted one dim (EDIT's held layer: its black keys). The drum's name and side top right */
#define UXS_DXN 8192u            /* the hit drawn, samples (as NOISE's DUST: ~0.19 s) */
#define UXS_DY 58                /* its axis, its height either side */
#define UXS_DA 34
static void uxs_drum_hit(const track_t *t, uint32_t l, int32_t *o)
{
    static dx_voice_t v;
    int32_t y[CTL], lv = clamp(t->p[P_LN0 + (l & 7u)], 0, 127), drv = t->p[P_E7], g = 0, mk = 0, gl;
    uint32_t i, k, j = 0;
    const int16_t *p = t->p;
    memset(&v, 0, sizeof v);
    v.trig = 1;
    v.rng = 0x2545F491;
    if ((l & 7u) == DV_BELL)                            /* (as drum_render: the cowbell 4 dB under) */
        lv = lv * 100 / 127;
    gl = lv * lv * 2 + (lv * lv >> 6);
    if (drv > 0) {                                      /* (drum_bus: DRIVE x1..x4 into the soft clip, its level kept) */
        g = 4096 + drv * 3 * 4096 / 127;
        mk = (int32_t)((19661u << 15) / (uint32_t)softclip((19661 * g) >> 12));
    }
    for (i = 0; i < UXS_WN; i++)
        o[i] = 0;
    for (k = 0; k < UXS_DXN / CTL; k++) {
        dx_run(&dx_kit[l & 7u], &v, p[P_E0], (p[P_E1] - 64) * 3, p[P_E3] - 64, p[P_E2] - 64, p[P_E4] - 64, p[P_E6], p[P_E5], y,
               CTL);
        for (i = 0; i < CTL; i++, j++) {
            int32_t s = y[i], c = (int32_t)(j * UXS_WN / UXS_DXN);
            if (lv < 127)
                s = (int32_t)(((int64_t)s * gl) >> 15);
            s += s >> 2;
            if (g)
                s = (softclip((clamp(s, -131071, 131071) * g) >> 12) * mk) >> 15;
            s = soft_knee(s, 24000);
            s = s < 0 ? -s : s;
            if (s > o[c])
                o[c] = s;
        }
    }
}
static void uxs_drum_draw(const track_t *t, const char *name)
{
    int32_t *o = uxs_cols;
    uint32_t l = dx_ui_lane & 7u, i;
    uint16_t fill = ux_mix(UX_BG, UX_ACID, 30);
    char b[24];
    int32_t x;
    uxs_drum_hit(t, l, o);
    uxs_pic_head(t, name);
    str_cpy(b, drum_lane_abbr(t, l), sizeof b);         /* "SD . SNARE . A" */
    str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), drum_lane_name(t, l), sizeof b - str_len(b));
    str_cpy(b + str_len(b), dx_ui_side ? " \xB7 B" : " \xB7 A", sizeof b - str_len(b));
    x = 228 - text_w(&UF_LBL, b);
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "EDIT picture's word");
    cv_text_on(x, 8, &UF_LBL, b, UX_TEXT, ux_under(x, 12));
    cv_rect(12, UXS_DY, 216, 1, UX_LINE);
    for (i = 0; i < UXS_WN; i++) {                      /* the hit: its envelope either side of the axis */
        int32_t xx = 12 + (int32_t)i * 216 / (int32_t)UXS_WN, h = clamp(o[i] * UXS_DA / 32768, 0, UXS_DA);
        if (h) {
            cv_rect(xx, UXS_DY - h, 2, 2 * h + 1, fill);
            cv_rect(xx, UXS_DY - h, 2, 1, UX_ACID);
            cv_rect(xx, UXS_DY + h, 2, 1, UX_ACID);
        }
    }
    for (i = 0; i < 8u; i++) {                          /* the drums: a cell each, its LEVEL a bar */
        int32_t cx = 12 + (int32_t)i * 27, cy = 102, lv = clamp(t->p[P_LN0 + i], 0, 127), h = lv * 24 / 127;
        int on = i == l, mute = dx_lane_muted(i);
        uint16_t bg = on ? UX_BG2 : ux_under(cx + 12, cy + 20), ink = mute ? UX_LINE : on ? UX_TEXT : UX_MID;
        const char *ab = drum_lane_abbr(t, i);
        if (on) {
            cv_rect(cx, cy, 25, 42, UX_BG2);
            cv_rect(cx, cy, 25, 1, UX_ACID);
            cv_rect(cx, cy + 41, 25, 1, UX_ACID);
            cv_rect(cx, cy, 1, 42, UX_ACID);
            cv_rect(cx + 24, cy, 1, 42, UX_ACID);
        }
        GFX_HOOK_CELL(cx, cy, cx + 25, cy + 42);
        cv_text_on(cx + 13 - text_w(&UF_LBL, ab) / 2, cy + 4, &UF_LBL, ab, ink, bg);
        cv_rect(cx + 8, cy + 15, 9, 24, on ? ux_mix(UX_BG2, UX_LINE, 60) : UX_EMPTY);
        if (h)
            cv_rect(cx + 8, cy + 39 - h, 9, h, mute ? UX_LINE : on ? UX_ACID : UX_MID);
    }
}

/* -------------------------------------------------------------------------- ENV's, ARP's and SCL's pictures --- */
/* (3b task 6) Each drawn from the track's stored settings, a turn redrawing it once (the panel's signature: ux_section_draw),
 * nothing a frame while idle, nothing in the audio path; the four knobs' colours mark what each moves.
 * ENV  AMP: the envelope as it sounds, a stage the colour of its knob (ATTACK ACID, DECAY SLIDE, SUSTAIN VIOLET, RELEASE
 *      ACCENT; voice.c env_tick: the attack straight, decay and release falling as the engine's do), a stage as wide as its
 *      time (the knob's place: its times run 1 ms .. 10 s), LOOP on: the attack and decay again while the key is held.
 *      ENV TO: that envelope again (dim), what it does to FILTER, PITCH and SHAPE: each one's amount times it, up or
 *      down from the middle, in its knob's colour (LOOP's the dim one's: ACCENT when on)
 * ARP  the notes it plays from C G E held (played in that order) over one bar (a longer RATE: the step), the keys let go at
 *      three quarters (HOLD on: it plays on): each note a bar as long as its GATE at its RATE, SWING moving the odd ones late,
 *      CHANCE's misses hollow, the pitch up the panel (OCTAVES, ORDER, the ARP's pattern: seq.c arp_step / arp_pick, its
 *      random ones from a fixed seed); OFF: the chord as played; TRNS: the keys move the sequence
 * SCL  KEY: an octave of keys, the scale's lit (SLIDE), ROOT's in ACID, a key that plays another note (SNAP: seq.c kb_map)
 *      in VIOLET with that note's name on it ("-" silent), the line under them saying what SNAP does, ALL TRACKS' move
 *      (every sequence that many scale steps: a mark under the note the ROOT goes to, the line under) in ACCENT. CHORD: the chord the root key plays (seq.c chord_build: CHORD, VOICING) on three octaves, STRUM's notes one
 *      after another in time, ROTATE: the sequence's sixteen steps as they play, its first step marked */
#define UXS_EX0 14               /* the envelope's left, its width */
#define UXS_EW 212
/* a column's factor (Q15) for a stage n columns long that falls 99 % over them, exp(-4.6 / n) as the engine's ENV_EXP is
 * (tools/gen_tables.py; 3b task 6 fix: 1 - 4.6 / n fell too fast on a short stage): its series to x^4, within 0.3 % */
static int32_t uxs_exp_k(int32_t n)
{
    int64_t x = 150733 / n, x2 = x * x >> 15, x3 = x2 * x >> 15, x4 = x3 * x >> 15;
    return (int32_t)(32768 - x + x2 / 2 - x3 / 6 + x4 / 24);
}
static void uxs_env_shape(const track_t *t, int16_t *e, uint8_t *st)   /* its level a column (0..1000), its stage (0..3) */
{
    const int16_t *p = t->p;
    int32_t a = 4 + p[P_ATK] * 46 / 127, d = 6 + p[P_DEC] * 46 / 127, r = 6 + p[P_REL] * 46 / 127, off = UXS_EW - r;
    int32_t sus = p[P_SUS] * 64000 / 128, kd = uxs_exp_k(d), kr = uxs_exp_k(r), lv = 0, x, stage = 0;
    int loop = p[P_ELOOP] > 0 && sus < 62976;           /* (LOOP: at the sustain, held, the attack again; voice.c env_tick: its
                                                         * sustain P_SUS << 17 of 1 << 24, 127: 99 %. The level in 64000ths
                                                         * here (3b task 6 fix: 1000ths fell too fast, the engine's times not) */
    for (x = 0; x < UXS_EW; x++) {
        if (x >= off)
            stage = 3;
        if (stage == 0) {                               /* (straight up, at the attack's rate: from the sustain too) */
            lv += (64000 + a - 1) / a;
            if (lv >= 64000)
                lv = 64000, stage = 1;
        } else if (stage == 3)
            lv = (int32_t)(((int64_t)lv * kr) >> 15);
        else {
            lv = sus + (int32_t)(((int64_t)(lv - sus) * kd) >> 15);
            if (loop && lv - sus < 1000)                /* (within 1/64, as the engine's) */
                stage = 0;
            else if (!loop && x >= a + d)
                stage = 2;
        }
        e[x] = (int16_t)clamp((lv + 32) / 64, 0, 1000);
        st[x] = (uint8_t)stage;
    }
}
static void uxs_env_draw(const track_t *t, const page_t *pg, const char *name)
{
    int16_t e[UXS_EW];
    uint8_t st[UXS_EW];
    int32_t x, k, y0 = 132, y1 = 34, mid = 82, amp = 44;
    int mod = pg->id[0] == P_ED_FLT;
    uxs_env_shape(t, e, st);
    if (!mod) {
        uint16_t fill = ux_mix(UX_BG, UX_ACID, 18);
        for (x = 0; x < UXS_EW; x++) {
            int32_t y = y0 - e[x] * (y0 - y1) / 1000;
            if (y < y0)
                cv_rect(UXS_EX0 + x, y, 1, y0 - y, fill);
        }
        cv_rect(UXS_EX0, y0, UXS_EW, 1, UX_LINE);
        for (x = 1; x < UXS_EW; x++)
            cv_line_t(UXS_EX0 + x - 1, y0 - e[x - 1] * (y0 - y1) / 1000, UXS_EX0 + x, y0 - e[x] * (y0 - y1) / 1000,
                      ux_knob_col(st[x]), 2);
    } else {
        uint16_t gc = t->p[P_ELOOP] > 0 ? ux_mix(UX_BG, UX_ACCENT, 45) : UX_LINE;
        cv_rect(UXS_EX0, mid, UXS_EW, 1, UX_LINE);
        for (x = 1; x < UXS_EW; x++)                    /* the envelope itself, dim */
            cv_line_t(UXS_EX0 + x - 1, mid - e[x - 1] * amp / 1000, UXS_EX0 + x, mid - e[x] * amp / 1000, gc, 1);
        for (k = 0; k < 3; k++) {                       /* FILTER PITCH SHAPE: its amount times the envelope */
            int32_t am = t->p[P_ED_FLT + (uint32_t)k];
            if (!am)
                continue;
            for (x = 1; x < UXS_EW; x++)
                cv_line_t(UXS_EX0 + x - 1, mid - e[x - 1] * am * amp / 64000, UXS_EX0 + x, mid - e[x] * am * amp / 64000,
                          ux_knob_col((uint32_t)k), 2);
        }
    }
    uxs_pic_head(t, name);
}

/* ARP: the notes over the window (960 a bar) */
static const uint16_t UXS_ADIV[10] = {240, 120, 60, 30, 80, 40, 480, 960, 1920, 3840};   /* (params.c N_DIV: 1/4 .. 4BAR, in
                                                         * 960ths of a bar) */
static const char *const UXS_ARP_SAY[] = {"C G E HELD", "KEYS PLAY AS PLAYED", "KEYS MOVE THE SEQUENCE"};   /* (the top right:
                                                         * the keys the picture holds, the bar along its foot) */
static uint32_t uxs_arp_list(const track_t *t, int32_t *list)   /* the note list arp_step builds from C G E held */
{
    static const uint8_t HELD[3] = {60, 67, 64};
    int32_t h[3];
    uint32_t i, j, len = 0, o, oc = (uint32_t)clamp(t->p[P_AOCT], 1, 4);
    for (i = 0; i < 3u; i++)
        h[i] = HELD[i];
    if (!t->p[P_AORDER])                              /* NOTE: low to high; PLAY: as played */
        for (i = 1; i < 3u; i++)
            for (j = i; j > 0 && h[j - 1] > h[j]; j--) {
                int32_t x = h[j]; h[j] = h[j - 1]; h[j - 1] = x;
            }
    for (o = 0; o < oc; o++)
        for (i = 0; i < 3u; i++)
            list[len++] = h[i] + 12 * (int32_t)o;
    if (t->p[P_AMODE] == 6)                           /* REPEAT: the last played */
        list[0] = HELD[2], len = 1;
    if (t->p[P_AMODE] == 8)                           /* UP+8: the first again over the top */
        list[len] = list[0] + 12 * (int32_t)oc, len++;
    return len;
}
static uint32_t uxs_arp_pick(uint32_t m, uint32_t i, uint32_t len, uint32_t *walk, uint32_t *rnd)   /* seq.c arp_pick's */
{
    uint32_t cyc = len > 1u ? 2u * len - 2u : 1u, k = i % cyc, w;
    switch (m) {
    case 2: return len - 1u - i % len;
    case 3: return k < len ? k : cyc - k;
    case 7: return k < len ? len - 1u - k : k - (len - 1u);
    case 4: return uxs_rnd(rnd) % len;
    case 9: case 10:
        k = i % len;
        if (m == 10) k = len - 1u - k;
        return (k & 1u) ? len - 1u - k / 2u : k / 2u;
    case 11: return len < 2u ? 0u : (k & 1u) ? len - 1u : k / 2u;
    case 12: return len < 2u ? 0u : (k & 1u) ? 1u + k / 2u : 0u;
    case 13:
        w = *walk;
        w = !i ? 0u : len < 2u ? 0u : !w ? 1u : w >= len - 1u ? len - 2u : (uxs_rnd(rnd) >> 7) & 1u ? w + 1u : w - 1u;
        *walk = w;
        return w;
    default: return i % len;
    }
}
/* note i of the picture: its start t0 (960ths of a bar), its place j in the list (uxs_arp_pick), whether CHANCE plays it. SWING
 * as seq.c arp_step's (3b task 6 fix: the sequencer's, swing_step_len): after an even note per + sw, after an odd one per - sw,
 * so an odd note lands sw late */
static void uxs_arp_note(const track_t *t, int32_t i, uint32_t len, uint32_t *walk, uint32_t *rnd, int32_t *t0, uint32_t *j,
                         int *on)
{
    uint32_t m = (uint32_t)clamp(t->p[P_AMODE], 0, 15);
    int32_t per = UXS_ADIV[clamp(t->p[P_ARATE], 0, 9)];
    *t0 = i * per + ((i & 1) ? t->p[P_ASWING] * per / 250 : 0);
    *j = m == 14 ? 0u : uxs_arp_pick(m, (uint32_t)i, len, walk, rnd);
    *on = (int32_t)(((uint32_t)i * 2654435761u >> 9) & 127u) <= t->p[P_APROB];
}
static void uxs_arp_draw(const track_t *t, const char *name)
{
    int32_t list[16], lo = 127, hi = 0, x0 = 14, w = 212, yb = 118, yh = 76, i, n, per, win, gate, up, x;
    uint32_t m = (uint32_t)clamp(t->p[P_AMODE], 0, 15), len, walk = 0, rnd = 0x2545F491u, oc = (uint32_t)clamp(t->p[P_AOCT], 1, 4);
    uint16_t fill = ux_mix(UX_BG, UX_ACID, 30);
    len = uxs_arp_list(t, list);
    for (i = 0; i < (int32_t)len; i++) {
        lo = list[i] < lo ? list[i] : lo;
        hi = list[i] > hi ? list[i] : hi;
    }
    if (m == 14)                                        /* (CHORD: an octave up each step) */
        hi = 67 + 12 * ((int32_t)oc - 1), lo = 60;
    if (hi - lo < 12)
        hi = lo + 12;
    per = UXS_ADIV[clamp(t->p[P_ARATE], 0, 9)];
    win = per > 960 ? per : 960;
    up = t->p[P_AHOLD] ? win : win * 3 / 4;             /* (the keys let go: HOLD plays on) */
    gate = per * clamp(t->p[P_AGATE], 1, 127) / 128;
    cv_rect(x0, yb + 6, w, 1, UX_LINE);
    for (i = 0; i <= win; i += 240)                     /* the beats, the bars taller */
        cv_rect(x0 + i * w / win, yb + (i % 960 ? 4 : 2), 1, i % 960 ? 2 : 4, UX_LINE);
    cv_rect(x0, yb + 10, win * 3 / 4 * w / win, 3, UX_MID);   /* the keys held */
    if (!m || m == 15) {                                /* OFF: the chord as played; TRNS: no notes of its own */
        for (i = 0; i < 3; i++) {
            int32_t y = yb - (list[i] - lo) * yh / (hi - lo);
            cv_rect(x0, y - 2, win * 3 / 4 * w / win, 5, m ? UX_LINE : fill);
        }
    } else {
        n = win / per;
        for (i = 0; i < n; i++) {
            int32_t t0, xs, xe, nn = m == 14 ? 3 : 1, c;
            uint32_t j;
            int on;
            uxs_arp_note(t, i, len, &walk, &rnd, &t0, &j, &on);
            if (t0 >= up)
                break;
            xs = x0 + t0 * w / win;
            xe = x0 + (t0 + gate) * w / win;
            if (xe > x0 + w)
                xe = x0 + w;
            if (xe < xs + 2)
                xe = xs + 2;
            for (c = 0; c < nn; c++) {
                int32_t note = m == 14 ? list[c] + 12 * (int32_t)((uint32_t)i % oc) : list[j % len], y;
                y = yb - (note - lo) * yh / (hi - lo);
                if (on)
                    cv_rect(xs, y - 2, xe - xs, 5, UX_ACID);
                else {                                  /* (CHANCE: this one left out) */
                    cv_rect(xs, y - 2, xe - xs, 1, UX_MID);
                    cv_rect(xs, y + 2, xe - xs, 1, UX_MID);
                    cv_rect(xs, y - 2, 1, 5, UX_MID);
                    cv_rect(xe - 1, y - 2, 1, 5, UX_MID);
                }
            }
        }
    }
    uxs_pic_head(t, name);
    x = 228 - text_w(&UF_LBL, UXS_ARP_SAY[!m ? 1 : m == 15 ? 2 : 0]);
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "ARP picture's word");
    cv_text_on(x, 8, &UF_LBL, UXS_ARP_SAY[!m ? 1 : m == 15 ? 2 : 0], UX_TEXT, ux_under(x, 12));
}

/* SCL */
static const uint8_t UXS_BLK[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
static const uint8_t UXS_WX[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};   /* a note's white key (a black one's: left of it) */
static const char *const UXS_SNAP_SAY[] = {"KEYS PLAY AS MARKED", "KEYS SNAP TO THE SCALE", "WHITE KEYS PLAY THE SCALE",
                                           "KEYS AND SEQUENCE SNAP", "A KIT: ITS KEYS PLAY THE DRUMS"};   /* (SNAP's; a kit's) */
static const char *const UXS_STRN_SAY[] = {"ALL TRACKS AS WRITTEN", "ALL TRACKS "};   /* (then uxs_strn's "+2 STEPS") */
static const char *const UXS_CHORD_SAY[] = {"ONE NOTE A KEY", "ONE NOTE: VOICE NOT POLY", "ROTATE", "A KIT: ONE HIT A KEY"};
/* the KEY picture's line: what SNAP does to the keys (a kit: its keys play its drums, never snapped: seq.c kb_map) */
static const char *uxs_snap_say(const track_t *t)
{
    return UXS_SNAP_SAY[drum_track(t) ? 4u : (uint32_t)clamp(t->p[P_QUANT], 0, 3)];
}
/* the CHORD picture's word: the chord the ROOT key plays (n its notes: "A 7TH"), else why a key plays one note (CHORD off, a
 * kit's hits: seq.c chord_build's e->oneshot, a voice not POLY) */
static void uxs_chord_word(const track_t *t, uint32_t n, char *b, uint32_t nb)
{
    const char *u = "";
    uint32_t rt = (uint32_t)clamp(t->p[P_ROOT], 0, 11);
    if (t->p[P_CHRD] && n > 1u) {
        str_cpy(b, N_NOTE[rt], nb);
        str_cpy(b + str_len(b), " ", nb - str_len(b));
        param_format(&CHORD_DESC[0], t->p[P_CHRD], b + str_len(b), &u);
    } else
        str_cpy(b, UXS_CHORD_SAY[!t->p[P_CHRD] ? 0 : ENGINES[eng_idx(t->eng_req)]->oneshot ? 3 : 1], nb);
}
/* one key of an octave drawn as a piano's: x0 its white keys' left, kw their width, y, h the white keys' (a black key 2/3
 * as wide, 0.6 as tall); its fill, its label (0: none) in ink */
static void uxs_key(int32_t x0, int32_t kw, int32_t y, int32_t h, uint32_t n, uint16_t fill, const char *lab, uint16_t ink)
{
    int32_t x, w, hh;
    if (UXS_BLK[n % 12u]) {
        w = kw * 2 / 3;
        x = x0 + (int32_t)(UXS_WX[n % 12u] + 1u) * kw - w / 2;
        hh = h * 3 / 5;
    } else {
        w = kw - 1;
        x = x0 + (int32_t)UXS_WX[n % 12u] * kw;
        hh = h;
    }
    cv_rect(x, y, w, hh, fill);
    if (lab && lab[0]) {
        int32_t tw = text_w(&UF_LBL, lab);
        cv_text_on(x + (w - tw) / 2, y + hh - 12, &UF_LBL, lab, ink, fill);
    }
}
static void uxs_scl_key(const track_t *t, const char *name)
{
    uint32_t mask = scale_mask(t), rt = (uint32_t)clamp(t->p[P_ROOT], 0, 11), q = (uint32_t)clamp(t->p[P_QUANT], 0, 3), n, pass;
    int32_t x0 = 15, kw = 30, y = 30, h = 62, s = song.g[G_STRN], tgt = -1;
    int kit = drum_track(t);                            /* (a kit: its keys play its drums, never snapped: seq.c kb_map) */
    char b[32];
    if (s && (mask & 0xFFFu)) {                         /* ALL TRACKS: the root s scale steps on (seq.c scale_steps) */
        int32_t m = (int32_t)rt, d = s;
        for (; d > 0; d--)
            do m++; while (!((mask >> (uint32_t)((m - (int32_t)rt + 120) % 12)) & 1u));
        for (; d < 0; d++)
            do m--; while (!((mask >> (uint32_t)((m - (int32_t)rt + 120) % 12)) & 1u));
        tgt = (m % 12 + 12) % 12;
    }
    for (pass = 0; pass < 2u; pass++)                   /* the white keys, then the black over them */
        for (n = 0; n < 12u; n++) {
            uint32_t deg = (n + 12u - rt) % 12u, in = (mask >> deg) & 1u, k = 7u + n, pl;
            uint16_t fill, ink;
            const char *lab = 0;
            if (UXS_BLK[n] != pass)
                continue;
            fill = !deg ? UX_ACID : in ? (pass ? ux_mix(UX_BG, UX_SLIDE, 75) : UX_SLIDE) : pass ? UX_BG2 : UX_LINE;
            ink = in ? UX_BG : UX_MID;
            pl = kit ? n : kb_map(t, k);                /* (what this key plays: SNAP's, the octave's) */
            if (pl == KB_SILENT)
                lab = "-", ink = in ? UX_BG : UX_MID;
            else if (pl % 12u != n) {
                lab = N_NOTE[pl % 12u];                 /* (it plays another note: that one, in VIOLET) */
                fill = pass ? ux_mix(UX_BG, UX_VIOLET, 55) : UX_VIOLET;
                ink = UX_TEXT;
            } else if (in)
                lab = N_NOTE[n];
            uxs_key(x0, kw, y, h, n, fill, lab, ink);
            if ((int32_t)n == tgt)                      /* (ALL TRACKS: where the root moves) */
                cv_rect(x0 + (int32_t)UXS_WX[n] * kw + (UXS_BLK[n] ? kw - 4 : kw / 2 - 4), y + h + 3, 8, 3, UX_ACCENT);
        }
    uxs_pic_head(t, name);
    str_cpy(b, N_NOTE[rt], sizeof b);
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), UXS_SCALE_FULL[clamp(t->p[P_SCALE], 0, 15)], sizeof b - str_len(b));
    n = (uint32_t)(228 - text_w(&UF_LBL, b));
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "SCL picture's word");
    cv_text_on((int32_t)n, 8, &UF_LBL, b, UX_TEXT, ux_under((int32_t)n, 12));
    cv_text_on(15, 104, &UF_LBL, uxs_snap_say(t), q && !kit ? UX_VIOLET_LT : UX_MID, ux_under(15, 108));
    if (s) {                                            /* ("ALL TRACKS +2 STEPS") */
        str_cpy(b, UXS_STRN_SAY[1], sizeof b);
        uxs_strn(s, b + str_len(b), sizeof b - str_len(b), 1);
    } else
        str_cpy(b, UXS_STRN_SAY[0], sizeof b);
    cv_text_on(15, 122, &UF_LBL, b, s ? UX_ACCENT : UX_MID, ux_under(15, 126));
}
static void uxs_scl_chord(track_t *t, const char *name)
{
    uint8_t nt[KB_CH];
    uint32_t rt = (uint32_t)clamp(t->p[P_ROOT], 0, 11), n, i, base, len;
    int32_t x0 = 15, kw = 10, y = 28, h = 46, gap = t->p[P_CSTR], k;
    char b[32];
    n = chord_peek(t, 60u + rt, nt);                    /* (LEAD's memory as it was: the picture plays nothing; ui_input.c) */
    base = nt[0] / 12u * 12u;
    if (nt[n - 1u] >= base + 36u)
        base += 12u;
    {                                                   /* three octaves, the white keys then the black */
        uint32_t pass, o;
        for (pass = 0; pass < 2u; pass++)
            for (o = 0; o < 3u; o++)
                for (i = 0; i < 12u; i++) {
                    uint32_t note = base + o * 12u + i, c, on = 0;
                    if (UXS_BLK[i] != pass)
                        continue;
                    for (c = 0; c < n; c++)
                        on |= nt[c] == note;
                    uxs_key(x0 + (int32_t)o * 7 * kw, kw, y, h, i, on ? UX_ACID : pass ? UX_BG2 : UX_LINE, 0, UX_BG);
                }
    }
    for (i = 0; i < n; i++) {                           /* STRUM: the notes one after another, low to high */
        int32_t xs = x0 + (int32_t)i * gap * 36 / 127, yy = 84 + (int32_t)i * 6;
        cv_rect(xs, yy, 100, 4, i ? ux_mix(UX_BG, UX_VIOLET, 80) : UX_VIOLET);
    }
    len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP);
    for (i = 0; i < 16u; i++) {                         /* ROTATE: the steps as they play (seq.c seq_src), the first marked */
        int32_t ln = (int32_t)(len < 16u ? len : 16u), o = t->p[P_SOFS] % (int32_t)len;
        uint32_t src = (uint32_t)((((int32_t)(i % len) - o) % (int32_t)len + (int32_t)len) % (int32_t)len);
        int32_t cx = x0 + (int32_t)i * 10, on = step_on(&t->step[src]);
        if ((int32_t)i >= ln)
            break;
        cv_rect(cx, 116, 9, 14, on ? UX_MID : UX_EMPTY);
        if (src == 0u)
            cv_rect(cx, 132, 9, 3, UX_ACCENT);
    }
    k = 228 - text_w(&UF_LBL, UXS_CHORD_SAY[2]);        /* (its name beside it, in KNOB 4's colour) */
    cv_text_on(k, 118, &UF_LBL, UXS_CHORD_SAY[2], UX_ACCENT, ux_under(k, 122));
    uxs_pic_head(t, name);
    uxs_chord_word(t, n, b, sizeof b);
    k = 228 - text_w(&UF_LBL, b);
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "SCL picture's word");
    cv_text_on(k, 8, &UF_LBL, b, UX_TEXT, ux_under(k, 12));
}

/* the page's picture: MORE its list; EDIT's its engine's, ENV's, ARP's, SCL's (above); the others (until Tasks 7 and 8 give
 * each its own) the page's title over its four values as bars in the knobs' colours */
static void uxs_picture(const page_t *pg, const ux_knob_t k[4])
{
    const aafont_t *f = &UF_TTL;
    int32_t w, x, i;
    if (pg->graph == GR_MORE) {
        ux_more_draw(pg->more, uxm_at[uxm_key()].sel, uxm_at[uxm_key()].top);
        return;
    }
    if (pg->fam == FAM_EDIT && edit_kind() == 1u) {     /* (3b task 5) FM6: its algorithm, OP ENV the operator's envelope */
        uxs_fm6_draw(TSEL, pg, nav_page_name());
        return;
    }
    if (pg->fam == FAM_EDIT && edit_kind() == 2u) {     /* .. DRUM-X: the drum's hit, the eight drums */
        uxs_drum_draw(TSEL, nav_page_name());
        return;
    }
    if (pg->fam == FAM_ENV) {                           /* (3b task 6) ENV: the envelope; ENV TO what it moves */
        uxs_env_draw(TSEL, pg, nav_page_name());
        return;
    }
    if (pg->fam == FAM_ARP) {                           /* .. ARP: the notes it plays */
        uxs_arp_draw(TSEL, nav_page_name());
        return;
    }
    if (pg->fam == FAM_SCL) {                           /* .. SCL: the keys (KEY), the chord a key plays (CHORD) */
        if (pg->id[0] == P_CHRD)
            uxs_scl_chord(TSEL, nav_page_name());
        else
            uxs_scl_key(TSEL, nav_page_name());
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
    if (pg->fam == FAM_EDIT && edit_kind() == 1u) {     /* (3b task 5) FM6: the patch, the knobs, the operator and stage */
        const uint8_t *pp = fm6_patch[song.sel % NTRK];
        for (c = 0; c < FP_SIZE; c++)
            sig = sig * 31u + pp[c];
        for (c = 0; c < 8u; c++)
            sig = sig * 31u + (uint16_t)TSEL->p[P_E0 + c];
        sig = sig * 31u + fop_op * 7u + fop_stg;
    }
    if (pg->fam == FAM_EDIT && edit_kind() == 2u) {     /* .. DRUM-X: the kit, the knobs, the levels, the drum, the mutes */
        const uint8_t *kp = (const uint8_t *)&dx_kit[dx_ui_lane & 7u];
        for (c = 0; c < sizeof dx_kit[0]; c++)
            sig = sig * 31u + kp[c];
        for (c = 0; c < 8u; c++)
            sig = sig * 31u + (uint16_t)TSEL->p[P_E0 + c] + (uint16_t)TSEL->p[P_LN0 + c] * 131u + dx_lay[c] * 7u;
        sig = sig * 31u + dx_tone + dx_hands * 257u + dx_ui_lane * 7u + dx_ui_side + (uint32_t)dx_mute * 65537u;
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
    if ((pg->fam == FAM_ENV || pg->fam == FAM_ARP || pg->fam == FAM_SCL) && pg->graph != GR_MORE) {   /* (3b task 6: the
                                                         * settings their pictures are drawn from, the keys' octave, SCL's
                                                         * steps as they play and LEAD's last chord) */
        static const uint8_t S[] = {P_ATK, P_DEC, P_SUS, P_REL, P_ED_FLT, P_ED_PIT, P_ED_SHP, P_ELOOP, P_AMODE, P_ARATE, P_AOCT,
                                    P_AGATE, P_ASWING, P_APROB, P_AHOLD, P_AORDER, P_ROOT, P_SCALE, P_QUANT, P_CHRD, P_CVOI,
                                    P_CSTR, P_SOFS, P_SLEN, P_VOICE};
        for (c = 0; c < sizeof S; c++)
            sig = sig * 31u + (uint16_t)TSEL->p[S[c]];
        sig = sig * 31u + (uint16_t)song.g[G_STRN] + TSEL->eng_req * 7u + (uint32_t)(song.octave + 8) * 131u;
        if (pg->fam == FAM_SCL) {
            uint32_t ti = song.sel % NTRK;
            for (c = 0; c < NSTEP; c++)
                sig = sig * 3u + (uint32_t)step_on(&TSEL->step[c]);
            for (c = 0; c < KB_CH; c++)
                sig = sig * 131u + chord_last[ti][c];
            sig = sig * 7u + chord_nlast[ti];
        }
    }
    if (all || sig != panel_sig) {
        panel_sig = sig;
        uxs_panels++;
        ux_panel_begin(1);
        uxs_picture(pg, k);
        ux_panel_end();
    }
    {   /* the footer: where, n / N and NEXT the next page (MORE: its knobs' names), when they fit */
        const char *w = uxs_where();
        uxs_foot(w, k, uxs_count(), uxs_next(), b, sizeof b);
        sig = str_hash(str_hash(1u, w), b);
        if (all || sig != foot_sig) {
            foot_sig = sig;
            ux_footer(w, b);
        }
    }
    st = uxs_status(pg, &sc);
    ux_status(st, sc);
}
