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
 *            list (ux_more.c); (3b task 7) LFO's wave, DEST, MOD MATRIX; (task 8) FX's SENDS bars, SPACE's echoes and
 *            reverb tail, MASTER's output meter
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
                            SEC_ARP == UXS_SEC_ARP && SEC_LFO == UXS_SEC_LFO && SEC_FX == UXS_SEC_FX ? 1 : -1];   /* (ux_section.h's sections are ui_lens.c's) */
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
    /* (3b task 7) LFO's: MOD MATRIX's SRC1 DEST1 AMT1 .. are SOURCE DEST AMOUNT, the STEPS' LEVELs STEP 1 .. 16 (uxs_label) */
    {UXS_ALL, UXS_ALL, 0, P_MSDIV, "STEP RATE"},        /* MSEQ's DIV: a step's length (3b task 7 fix: not RATE, the LFO's) */
    {UXS_ALL, UXS_ALL, 0, P_MSSLW, "SMOOTH"},           /* .. SLEW: gliding from one step's level to the next */
    /* (3b task 8) FX's (spec 2.4; the user's short names that fit, one abbreviation each). The track's distortion P_DIST is
     * DISTORT everywhere (UXS_LABEL: SENDS, a synth's FILTER, FM6's MORE, the matrix's DEST; the engines' own DRIVE keeps DRIVE),
     * its TYPE and TONE DIST .., AUTO PAN's four PAN ... The buses' (globals) with their bus first, as the matrix names them
     * (DLY TIME .. CHO DEPTH, 3b task 7 fix): DLY REV CHO, FEEDBACK the one word cut (FDBK: "DLY FEEDBACK" fits no face's
     * 48 px); the SLICER's SLICE .., its pitch steps PITCH ..; the punch-in lane's LANE ... REV PRE's slot named by the TYPE,
     * DAMP FOLLOW on RESO (uxs_label) */
    {UXS_ALL, UXS_ALL, 0, P_DTYPE, "DIST TYPE"},
    {UXS_ALL, UXS_ALL, 0, P_DTONE, "DIST TONE"},
    {UXS_ALL, UXS_ALL, 0, P_APDEP, "PAN DEPTH"},
    {UXS_ALL, UXS_ALL, 0, P_APRATE, "PAN RATE"},
    {UXS_ALL, UXS_ALL, 0, P_APSYNC, "PAN SYNC"},
    {UXS_ALL, UXS_ALL, 0, P_APWAVE, "PAN WAVE"},
    {UXS_ALL, UXS_ALL, 1, G_DTIME, "DLY TIME"},
    {UXS_ALL, UXS_ALL, 1, G_DFDBK, "DLY FDBK"},
    {UXS_ALL, UXS_ALL, 1, G_DCOLOR, "DLY TONE"},
    {UXS_ALL, UXS_ALL, 1, G_DMIX, "DLY MIX"},
    {UXS_ALL, UXS_ALL, 1, G_WIDTH, "DLY WIDE"},         /* (JIANT's WIDE: the right echo later; the chorus's taps apart too) */
    {UXS_ALL, UXS_ALL, 1, G_DPIT, "DLY PITCH"},
    {UXS_ALL, UXS_ALL, 1, G_DSPRY, "DLY SPRAY"},
    {UXS_ALL, UXS_ALL, 1, G_DMOD, "DLY MOD"},
    {UXS_ALL, UXS_ALL, 1, G_DTYPE, "DLY TYPE"},
    {UXS_ALL, UXS_ALL, 1, G_DPING, "DLY PING"},
    {UXS_ALL, UXS_ALL, 1, G_RTYPE, "REV TYPE"},
    {UXS_ALL, UXS_ALL, 1, G_RSIZE, "REV SIZE"},
    {UXS_ALL, UXS_ALL, 1, G_RDAMP, "REV DAMP"},
    {UXS_ALL, UXS_ALL, 1, G_RMOD, "REV MOD"},
    {UXS_ALL, UXS_ALL, 1, G_RRATE, "REV RATE"},
    {UXS_ALL, UXS_ALL, 1, G_RFILT, "REV TONE"},         /* (JIANT's FILT: the reverb's input darker / brighter, as DLY TONE) */
    {UXS_ALL, UXS_ALL, 1, G_RWIDE, "REV WIDE"},
    {UXS_ALL, UXS_ALL, 1, G_CRATE, "CHO RATE"},
    {UXS_ALL, UXS_ALL, 1, G_CDEPTH, "CHO DEPTH"},
    {UXS_ALL, UXS_ALL, 1, G_CSTER, "CHO WIDE"},         /* (JIANT's STEREO: "CHO STEREO" is 49 px; WIDE as DLY's and REV's) */
    {UXS_ALL, UXS_ALL, 1, G_SLMODE, "SLICE MODE"},
    {UXS_ALL, UXS_ALL, 1, G_SLPAT, "SLICE PAT"},        /* ("SLICE PATTERN" is 60 px; the status line spells the pattern out) */
    {UXS_ALL, UXS_ALL, 1, G_SLRATE, "SLICE RATE"},
    {UXS_ALL, UXS_ALL, 1, G_SLDEP, "SLICE DEPTH"},
    {UXS_ALL, UXS_ALL, 1, G_SLT1, "SLICE T1"}, {UXS_ALL, UXS_ALL, 1, G_SLT1 + 1, "SLICE T2"},
    {UXS_ALL, UXS_ALL, 1, G_SLT1 + 2, "SLICE T3"}, {UXS_ALL, UXS_ALL, 1, G_SLT1 + 3, "SLICE T4"},
    {UXS_ALL, UXS_ALL, 1, G_SLT1 + 4, "SLICE T5"}, {UXS_ALL, UXS_ALL, 1, G_SLT6, "SLICE T6"},
    {UXS_ALL, UXS_ALL, 1, G_SLFX, "SLICE FX"},
    {UXS_ALL, UXS_ALL, 1, G_SLPLEN, "PITCH LOOP"},      /* (JIANT's SL PITCH LEN: the pitches go round after so many steps) */
    {UXS_ALL, UXS_ALL, 1, G_SLPQ, "PITCH SNAP"},        /* (.. QUANT: onto the scale, as SCL's SNAP the keys) */
    {UXS_ALL, UXS_ALL, 1, G_SLP0, "PITCH 1"}, {UXS_ALL, UXS_ALL, 1, G_SLP0 + 1, "PITCH 2"},
    {UXS_ALL, UXS_ALL, 1, G_SLP0 + 2, "PITCH 3"}, {UXS_ALL, UXS_ALL, 1, G_SLP0 + 3, "PITCH 4"},
    {UXS_ALL, UXS_ALL, 1, G_SLP0 + 4, "PITCH 5"}, {UXS_ALL, UXS_ALL, 1, G_SLP0 + 5, "PITCH 6"},
    {UXS_ALL, UXS_ALL, 1, G_SLP0 + 6, "PITCH 7"}, {UXS_ALL, UXS_ALL, 1, G_SLP0 + 7, "PITCH 8"},
    {UXS_ALL, UXS_ALL, 1, G_SLP0 + 8, "PITCH 9"}, {UXS_ALL, UXS_ALL, 1, G_SLP0 + 9, "PITCH 10"},
    {UXS_ALL, UXS_ALL, 1, G_SLP0 + 10, "PITCH 11"}, {UXS_ALL, UXS_ALL, 1, G_SLP0 + 11, "PITCH 12"},
    {UXS_ALL, UXS_ALL, 1, G_SLP0 + 12, "PITCH 13"}, {UXS_ALL, UXS_ALL, 1, G_SLP0 + 13, "PITCH 14"},
    {UXS_ALL, UXS_ALL, 1, G_SLP0 + 14, "PITCH 15"}, {UXS_ALL, UXS_ALL, 1, G_SLP15, "PITCH 16"},
    {UXS_ALL, UXS_ALL, 1, G_PFXLEN, "LANE STEPS"},      /* (JIANT's PUNCH LEN: the punch-in lane's steps, */
    {UXS_ALL, UXS_ALL, 1, G_PFXRES, "LANE RATE"},       /* .. RES a step's length: as LFO's STEP RATE) */
    {UXS_ALL, UXS_ALL, 1, G_DREL, "DUCK TIME"},         /* (JIANT's REL: DUCK's release, how long the others take back) */
};
/* (3b task 8) REVERB's fourth setting by its TYPE (params.c param_ref: ROOM / SPRING the pre-delay, SHIMMER / CLOUD AIR, RESO its
 * chord's PITCH), and DAMP on RESO: FOLLOW (the track its strings take their notes from) */
static const char *const UXS_RPRE[3] = {"REV PRE", "REV AIR", "REV PITCH"};
static const char *const UXS_RFOLW[1] = {"FOLLOW"};   /* ("REV FOLLOW" is 49 px) */
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
static const char *const UXS_MTX_NAME[3] = {"SOURCE", "DEST", "AMOUNT"};   /* (3b task 7: uxs_label) */
static const char *const UXS_MSTEP[16] = {"STEP 1", "STEP 2", "STEP 3", "STEP 4", "STEP 5", "STEP 6", "STEP 7", "STEP 8",
                                          "STEP 9", "STEP 10", "STEP 11", "STEP 12", "STEP 13", "STEP 14", "STEP 15", "STEP 16"};
/* .. else plain words for JIANT's short labels, by their text: the same everywhere */
typedef struct { const char *j, *p; } uxs_lbl_t;
static const uxs_lbl_t UXS_LABEL[] = {
    {"ATK", "ATTACK"}, {"DEC", "DECAY"}, {"SUS", "SUSTAIN"}, {"REL", "RELEASE"}, {"RES", "RESO"}, {"CUT", "CUTOFF"},
    {"DLY", "DELAY"}, {"REV", "REVERB"}, {"FEEDBK", "FEEDBACK"}, {"PRIO", "PRIORITY"}, {"DIST", "DISTORT"},   /* (3b task 8: DISTORT, not DRIVE:
                                                         * the engines' own DRIVE, ANALOG's and DRUM-X's, is DRIVE) */
    /* (3b task 4) the synths' (the engines' .edit[] and their mode relabels, eng_*.c) */
    {"INT", "INTERVAL"}, {"WAVE2", "WAVE 2"}, {"FB", "FEEDBACK"}, {"VIB", "VIBRATO"}, {"WAV#", "TABLE"}, {"VAR", "VARIANT"},
    {"ALGO", "FORMULA"}, {"TRK", "KEY TRACK"}, {"DENSE", "DENSITY"}, {"LEN", "LENGTH"}, {"VOWEL2", "VOWEL 2"}, {"Q", "RESO"},
    {"RAND", "RANDOM"}, {"POS", "POSITION"}, {"ARTIC", "CRISP"},
};
static const char *uxs_label(uint32_t sec, uint32_t glo, uint32_t id, const char *l)
{
    uint32_t i;
    const char *p;
    if (glo && id == G_RPRE)                            /* (3b task 8) the reverb's by its TYPE */
        return UXS_RPRE[song.g[G_RTYPE] == 3 ? 2 : song.g[G_RTYPE] == 2 || song.g[G_RTYPE] == 4 ? 1 : 0];
    if (glo && id == G_RDAMP && song.g[G_RTYPE] == 3)
        return UXS_RFOLW[0];
    p = id < 256u ? uxs_name_in(UXS_NAME, NELEM(UXS_NAME), sec, trk_eng(TSEL), glo ? 1u : 0u, id) : 0;
    if (p)
        return p;
    if (!glo && id >= P_LN0 && id <= P_LN7 && trk_eng(TSEL) == ENGI_DRUM)
        return "LEVEL";                                 /* (3b task 5) DRUM-X's SOUND: the drum picked's (its name: KNOB 1) */
    if (!glo && id >= P_M1SRC && id <= P_M4AMT)       /* (3b task 7) a matrix slot's: its SOURCE DEST AMOUNT (SLOT names it) */
        return UXS_MTX_NAME[(id - P_M1SRC) % 3u];
    if (!glo && id >= P_MS0 && id <= P_MS15)           /* .. the STEPS' levels (JIANT: LEVEL, the step KNOB 1 picked) */
        return UXS_MSTEP[id - P_MS0];
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
/* (3b task 7) LFO's values in plain words (spec 0 rule 4), the knob's short (48 px) and the status line's full:
 * RATE  in time its note ("1/8D", "1/8 DOTTED"); free from 1 Hz in hertz ("1.2HZ", "1.18 HZ"; from 10 Hz whole, "40HZ"), below
 *       1 Hz a cycle's length ("20S": a cycle every 20 s, as a note's length is one; "0.05 HZ: 20S A CYCLE")
 * WAVE  SINE TRI SAW SQR RAND (S&H: a random level a cycle; "&" is in no ux face); MODE TRIG / FREE and what each does (TRIG:
 *       a fresh phrase restarts it, voice.c trk_note_on: no gate held)
 * PHASE in degrees ("90\xB0"; 3b task 7 fix: the degree sign in UF_VAL / UF_HDR); FADE OFF at 0
 * the PITCH amounts (LFO > DEST's, ENV > ENV TO's: (source x amount x 3) >> 15 sixteenths of a semitone, voice.c; the LFO's top
 *       32767, the envelope's 32768: ENV TO's v x 3 / 16 semitones exactly) in cents and semitones ("+19CT", "+5.9ST",
 *       "+12ST"); every amount OFF at 0
 * MOD MATRIX's SOURCE and DEST (3b task 7 fix): ONE name each, the same on the knob, the picture's box and the status line
 *       (UXS_MSRC_N, UXS_MDST_N; an engine's E1..E8 its EDIT knob's label, uxs_label); the status line adds what it is after
 *       a colon (UXS_MSRC_X, UXS_MDST_X: "WHEEL: MOD WHEEL CC1"). A name wider than UF_VAL's 48 px is drawn in a smaller face
 *       (ux_widgets.c ux_val_face). The FX buses' (D.TIM .. C.DEP) are their section's (DLY REV CHO) and the FX page's knob
 *       (TIME FDBK TONE MIX, SIZE DAMP, RATE DEPTH: 3b task 8's UXS_NAME, the same words; DIST: DISTORT, as UXS_LABEL's) */
static const char *const UXS_LSYNC_F[12] = {"4 BARS", "2 BARS", "1 BAR", "1/2 NOTE", "1/4 NOTE", "1/8 DOTTED", "1/4 TRIPLET",
                                            "1/8 NOTE", "1/8 TRIPLET", "1/16 NOTE", "1/16 TRIPLET", "1/32 NOTE"};
static const char *const UXS_LWAVE_K[5] = {"SINE", "TRI", "SAW", "SQR", "RAND"};
static const char *const UXS_LWAVE_F[5] = {"SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM STEPS"};
static const char *const UXS_LMODE_F[2] = {"TRIG: NEW PHRASE RESTARTS IT", "FREE: RUNS ON ITS OWN"};
static const char *const UXS_LMODE_SAY[2] = {"EACH NEW PHRASE RESTARTS IT", "RUNS FREE"};   /* (the LFO picture's words) */
static const char *const UXS_MSRC_N[15] = {"OFF", "LFO", "ENV", "VELOCITY", "KEY", "NOTE RAND", "WHEEL", "PRESSURE", "EXPR",
                                           "MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4", "STEPS", "NOISE"};
static const char *const UXS_MSRC_X[15] = {0, 0, "THE ENVELOPE", "HOW HARD", "THE NOTE PLAYED", "NEW PER NOTE", "MOD WHEEL CC1",
                                           "AFTERTOUCH", "EXPRESSION CC11", "ON HOME VISUAL", "ON HOME VISUAL", "ON HOME VISUAL",
                                           "ON HOME VISUAL", "SET IN MORE", "A RANDOM WANDER"};
static const char *const UXS_MDST_N[MD_N] = {"OFF", "PITCH", "FILTER", "SHAPE", "AMP", "PAN", "DISTORT", "CHORUS", "DELAY",
                                             "REVERB", "LFO RATE", "VIBRATO", 0, 0, 0, 0, 0, 0, 0, 0, "CLIP", "PUNCH",
                                             "DLY TIME", "DLY FDBK", "DLY TONE", "DLY MIX", "REV SIZE", "REV DAMP", "CHO RATE",
                                             "CHO DEPTH"};
static const char *const UXS_MDST_X[MD_N] = {0, 0, 0, 0, 0, 0, 0, "SEND LEVEL", "SEND LEVEL", "SEND LEVEL", 0, "LFO PITCH DEPTH",
                                             0, 0, 0, 0, 0, 0, 0, 0, "MASTER", "MASTER", "DELAY TIME", "DELAY FEEDBACK",
                                             "DELAY TONE", "DELAY MIX", "REVERB SIZE", "REVERB DAMP", "CHORUS RATE",
                                             "CHORUS DEPTH"};
#define UXS_DEG "\xB0"                                   /* (the degree sign: UF_VAL, UF_HDR) */
static const char *const UXS_MDST_UN[2] = {"UNUSED", "UNUSED ON THIS ENGINE"};   /* (an E1..E8 the engine does not use) */
static void uxs_cat(char *b, uint32_t nb, const char *s) { str_cpy(b + str_len(b), s, nb - str_len(b)); }
static void uxs_lrate(int32_t v, char *b, uint32_t nb, int full)
{
    uint32_t h = LFO_HZ_X100[v & 127];
    char t[12];
    if (v < 0) {
        str_cpy(b, full ? UXS_LSYNC_F[(uint32_t)(-v - 1) % 12u] : N_LSYNC[(uint32_t)(-v - 1) % 12u], nb);
        return;
    }
    if (h < 100u) {                                     /* (below 1 Hz: a cycle's length, tenths of a second) */
        uint32_t t10 = (1000u + h / 2u) / h;
        if (t10 >= 100u)
            fmt_int(t, (int32_t)((t10 + 5u) / 10u));
        else
            fmt_fix(t, (int32_t)t10, 1);
        uxs_cat(t, sizeof t, "S");
        b[0] = 0;
        if (full) {
            str_cpy(b, "0", nb);                        /* (fmt_fix: ".05") */
            fmt_fix(b + 1, (int32_t)h, 2);
            uxs_cat(b, nb, " HZ: ");
        }
        uxs_cat(b, nb, t);
        if (full)
            uxs_cat(b, nb, " A CYCLE");
        return;
    }
    if (full)
        fmt_fix(b, (int32_t)(h < 1000u ? h : h / 10u), h < 1000u ? 2 : 1);
    else if (h < 1000u)
        fmt_fix(b, (int32_t)((h + 5u) / 10u), 1);
    else
        fmt_int(b, (int32_t)((h + 50u) / 100u));
    uxs_cat(b, nb, full ? " HZ" : "HZ");
}
/* a PITCH amount v: what a full source (its top: top) moves the note, +- (voice.c: (top x v x 3) >> 15 sixteenths) */
static void uxs_semis(int32_t v, int32_t top, char *b, uint32_t nb, int full)
{
    int32_t x = (top * v * 3) >> 15, c = (x * 25 + (x < 0 ? -2 : 2)) / 4, a = c < 0 ? -c : c;   /* (1/16 semitones; cents) */
    str_cpy(b, c < 0 ? "-" : "+", nb);
    if (a < 100) {
        fmt_int(b + 1, a);
        uxs_cat(b, nb, full ? " CENTS" : "CT");
        return;
    }
    if (a < 1000)
        fmt_fix(b + 1, (a + 5) / 10, 1);
    else
        fmt_int(b + 1, (a + 50) / 100);
    uxs_cat(b, nb, full ? " SEMITONES" : "ST");
}
static int uxs_mtx_e(uint32_t id) { return id >= P_M1SRC && id <= P_M4AMT ? (int)((id - P_M1SRC) % 3u) : -1; }   /* 0 SOURCE 1 DEST
                                                         * 2 AMOUNT, -1 none */
/* a matrix SOURCE (e 0) or DEST (e 1) v: its one name; full: what it is after it ("WHEEL: MOD WHEEL CC1"; an engine's E1..E8
 * its engine in brackets, "CUTOFF (ANALOG)") */
static void uxs_mtx_say(const track_t *t, int e, int32_t v, char *b, uint32_t nb, int full)
{
    const char *x;
    if (!e) {
        v = clamp(v, 0, MS_N - 1);
        str_cpy(b, UXS_MSRC_N[v], nb);
        x = UXS_MSRC_X[v];
    } else if ((v = clamp(v, 0, MD_N - 1)) < MD_E1 || v >= MD_CLIP) {
        str_cpy(b, UXS_MDST_N[v], nb);
        x = UXS_MDST_X[v];
    } else {                                            /* (the engine's E1..E8: its EDIT knob's name, uxs_label) */
        const char *l = mod_dst_name(t, v);
        if (str_eq(l, N_MDST[v])) {                     /* (a parameter the engine does not use) */
            str_cpy(b, UXS_MDST_UN[full], nb);
            return;
        }
        str_cpy(b, uxs_label(SEC_EDIT, 0, P_E0 + (uint32_t)(v - MD_E1), l), nb);
        if (full) {
            uxs_cat(b, nb, " (");
            uxs_cat(b, nb, trk_engp(t)->name);
            uxs_cat(b, nb, ")");
        }
        return;
    }
    if (full && x) {
        uxs_cat(b, nb, ": ");
        uxs_cat(b, nb, x);
    }
}
/* setting id's (a track's) value v in plain words: 1 done (b holds it), 0 not one of LFO's */
static int uxs_lfo_val(uint32_t id, int32_t v, char *b, uint32_t nb, int full)
{
    int e = uxs_mtx_e(id);
    if (id == P_LRATE) {
        uxs_lrate(v, b, nb, full);
        return 1;
    }
    if (id == P_LWAVE && v >= 0 && v < 5) {
        str_cpy(b, full ? UXS_LWAVE_F[v] : UXS_LWAVE_K[v], nb);
        return 1;
    }
    if (id == ID_LMODE && full) {
        str_cpy(b, UXS_LMODE_F[v != 0], nb);
        return 1;
    }
    if (id == ID_LPHS) {
        fmt_int(b, v * 45 / 16);                        /* (0..127 of a cycle: 2.8 degrees a step) */
        uxs_cat(b, nb, UXS_DEG);
        if (full && lfo_mode_of(TSEL))
            uxs_cat(b, nb, ", FOR TRIG");
        return 1;
    }
    if (!v && (id == P_LFADE || (id >= P_LD_PIT && id <= P_LD_AMP) || (id >= P_ED_FLT && id <= P_ED_SHP) || e == 2)) {
        str_cpy(b, "OFF", nb);
        return 1;
    }
    if (id == P_LD_PIT || id == P_ED_PIT) {             /* (the LFO's top 32767, the envelope's 32768: voice.c env_tick) */
        uxs_semis(v, id == P_ED_PIT ? 32768 : 32767, b, nb, full);
        return 1;
    }
    if (e == 0 || e == 1) {
        uxs_mtx_say(TSEL, e, v, b, nb, full);
        return 1;
    }
    return 0;
}
/* ------------------------------------------------------------------------------------------ FX's values (3b task 8) --- */
/* (spec 0 rule 4: plain values) what the engine does with each, from its own code: DLY TIME the time it plays at the tempo (fx.c
 * delay_samples, PING within half the line), DUCK in dB (fx.c duck_block), DUCK TIME in ms (40 .. 600), CLIP's drive (master_begin),
 * PUNCH's body gain (eng_drum.c punch_gain), REV PRE in ms (fx_buses: PRE x 40 samples), the TONEs darker / brighter, every amount
 * OFF at 0, JIANT's cut names whole (RECT, STUT, 16T, CHR SCL); the knob's short (48 px: ux_widgets.c ux_val_face), the status
 * line's in full; LOAD LITE / ECO what plays instead said (JIANT's cards said LITE / ECO) */
/* log2 x, Q16 (x > 0); 20 log10 (n / d) in tenths of a dB (no 64-bit division: shifts) */
static int32_t uxs_log2(uint32_t x)
{
    int32_t r, b;
    uint32_t y, n = 31;
    if (!x)
        return -(32 << 16);
    while (!(x >> n))
        n--;
    r = (int32_t)n << 16;
    y = n >= 31u ? x : x << (31u - n);                  /* (Q31: 1 <= y < 2) */
    for (b = 15; b >= 0; b--) {
        uint64_t q = ((uint64_t)y * y) >> 31;           /* (y squared, Q31: < 4) */
        if (q >> 32) {
            y = (uint32_t)(q >> 1);
            r |= 1 << b;
        } else
            y = (uint32_t)q;
    }
    return r;
}
static int32_t uxs_db10(uint32_t n, uint32_t d)          /* (200 log10 2 = 60.206: x 3945659 / 2^32 in Q16) */
{
    return (int32_t)(((int64_t)(uxs_log2(n) - uxs_log2(d)) * 3945659 + (1ll << 31)) >> 32);
}
static void uxs_fix1(char *b, uint32_t nb, int32_t v10)  /* "-5.7", "0.5" (fmt_fix: ".5") */
{
    char t[12];
    uint32_t a = (uint32_t)(v10 < 0 ? -v10 : v10);
    str_cpy(b, v10 < 0 ? "-" : "", nb);
    fmt_int(t, (int32_t)(a / 10u));
    uxs_cat(b, nb, t);
    uxs_cat(b, nb, ".");
    fmt_int(t, (int32_t)(a % 10u));
    uxs_cat(b, nb, t);
}
static void uxs_dbtxt(char *b, uint32_t nb, int32_t d10, int plus)   /* "-5.7DB", "-12DB", "+2.6DB" */
{
    char t[12];
    if (d10 > -100 && d10 < 100)
        uxs_fix1(t, sizeof t, d10);
    else
        fmt_int(t, (d10 + (d10 < 0 ? -5 : 5)) / 10);
    str_cpy(b, plus && d10 > 0 ? "+" : "", nb);
    uxs_cat(b, nb, t);
    uxs_cat(b, nb, "DB");
}
static void uxs_mstxt(char *b, uint32_t nb, uint32_t ms)   /* "250MS", "1.25S" */
{
    char t[12];
    if (ms < 1000u) {
        fmt_int(b, (int32_t)ms);
        uxs_cat(b, nb, "MS");
        return;
    }
    ms = (ms + 5u) / 10u;                               /* (hundredths, rounded) */
    fmt_int(b, (int32_t)(ms / 100u));
    uxs_cat(b, nb, ".");
    fmt_int(t, (int32_t)(ms % 100u + 100u));            /* (two decimals: "1.05S") */
    uxs_cat(b, nb, t + 1);
    uxs_cat(b, nb, "S");
}
static uint32_t uxs_smp_ms(uint32_t smp) { return (smp * 10u + FS / 200u) / (FS / 100u); }   /* (samples to ms, rounded) */
static uint32_t uxs_ms_smp(uint32_t ms) { return ms * (FS / 100u) / 10u; }
/* the delay as it plays: its TIME in samples (fx.c fx_buses: delay_samples, the knob's TIME, not the matrix's offset: a picture
 * moving with an LFO would redraw unpaced; PING-PONG, not with the GRAIN, within half the line: dly_rate) */
static uint32_t uxs_dly_smp(void)
{
    uint32_t g = (uint32_t)song.g[G_DTIME], d;
    int gr = !LOAD_EFF() && (song.g[G_DPIT] || song.g[G_DSPRY]);
    d = g < DT_FREE ? div_samples(g) : (uint32_t)DTIME_MS[(g - DT_FREE) % NELEM(DTIME_MS)] * 441u / 10u;
    d = d < 16u ? 16u : d > DLY_MAX ? DLY_MAX : d;
    return song.g[G_DPING] == 1 && !gr && d > DLY_MAX / 2u ? DLY_MAX / 2u : d;
}
/* the reverb's tail: the time its model takes to fall 60 dB (RT60, the Schroeder integral's -5 .. -35 dB slope: tests/ui_test.c
 * uxt8_rt60 measures fx.c rev_run and checks these against it), ms, by SIZE (UXS_TSZ) and DAMP (UXS_TDP); RESO's by SIZE (its
 * strings at C3: DAMP fixed, rev_reso). SHIMMER with MOD (its octaves fed back) and CLOUD (its grains) do not die away by themselves
 * (they hold, or grow, as long as the picture's 8 s and more, until the bus's gate fades a quiet residue, fx.c fx_gate): HOLDS.
 * SHIMMER at MOD 0 is ROOM */
#define UXS_TAIL_LONG 0xFFFFu
static const uint8_t UXS_TSZ[11] = {0, 16, 32, 48, 64, 80, 96, 104, 112, 120, 127};
static const uint8_t UXS_TDP[5] = {0, 32, 64, 96, 127};
static const uint16_t UXS_TAIL_ROOM[11][5] = {
    {728, 594, 568, 522, 436}, {806, 656, 628, 577, 481}, {916, 749, 708, 648, 535}, {1072, 861, 811, 761, 617},
    {1299, 1033, 979, 887, 740}, {1657, 1315, 1229, 1110, 894}, {2351, 1814, 1684, 1487, 1117}, {2988, 2256, 2072, 1797, 1318},
    {4148, 3008, 2664, 2270, 1606}, {6779, 4440, 3815, 3081, 2014}, {15644, 7891, 6264, 4623, 2683}};
static const uint16_t UXS_TAIL_SPRING[11][5] = {
    {348, 312, 288, 288, 228}, {451, 407, 397, 340, 273}, {531, 510, 509, 434, 372}, {685, 663, 643, 562, 478},
    {904, 837, 814, 725, 612}, {1256, 1197, 1150, 966, 762}, {1795, 1686, 1581, 1346, 1030}, {2181, 2071, 1963, 1613, 1114},
    {2907, 2627, 2469, 2000, 1350}, {3728, 3527, 3119, 2452, 1608}, {5134, 4768, 4048, 3000, 1898}};
static const uint16_t UXS_TAIL_RESO[11] = {160, 187, 218, 260, 321, 414, 567, 705, 955, 1451, 2670};
static uint32_t uxs_grid(const uint8_t *g, uint32_t n, int32_t v, uint32_t *f)   /* v's cell of grid g (n points), *f in it 0..256 */
{
    uint32_t i = 0;
    v = clamp(v, g[0], g[n - 1u]);
    while (i + 2u < n && v > g[i + 1u])
        i++;
    *f = (uint32_t)(v - g[i]) * 256u / (uint32_t)(g[i + 1u] - g[i]);
    return i;
}
static uint32_t uxs_lerp(uint32_t a, uint32_t b, uint32_t f) { return (a * (256u - f) + b * f + 128u) >> 8; }
static uint32_t uxs_rev_eff(void)                       /* the model playing (LOAD LITE / ECO: each reverb's kin, fx.c REV_LITE) */
{
    return REV_LITE[LOAD_EFF() % 3u][(uint32_t)clamp(song.g[G_RTYPE], 0, 4)];
}
static uint32_t uxs_tail_ms(void)
{
    uint32_t e = uxs_rev_eff(), fs, fd, i, j;
    const uint16_t (*T)[5] = e == 1u ? UXS_TAIL_SPRING : UXS_TAIL_ROOM;   /* (SHIMMER at MOD 0: ROOM) */
    if (e == 4u || (e == 2u && song.g[G_RMOD] > 0))
        return UXS_TAIL_LONG;
    i = uxs_grid(UXS_TSZ, 11u, song.g[G_RSIZE], &fs);
    if (e == 3u)
        return uxs_lerp(UXS_TAIL_RESO[i], UXS_TAIL_RESO[i + 1u], fs);
    j = uxs_grid(UXS_TDP, 5u, song.g[G_RDAMP], &fd);
    return uxs_lerp(uxs_lerp(T[i][j], T[i][j + 1u], fd), uxs_lerp(T[i + 1u][j], T[i + 1u][j + 1u], fd), fs);
}
static uint32_t uxs_rev_pre(void)                       /* the pre-delay heard, samples (fx_buses: ROOM and SPRING's, PRE x 40) */
{
    return song.g[G_RPRE] > 0 && song.g[G_RTYPE] <= 1 && uxs_rev_eff() <= 1u ? (uint32_t)song.g[G_RPRE] * 40u : 0u;
}
static const char *const UXS_DIV_K[10] = {"1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T", "1/2", "1/1", "2BAR", "4BAR"};
static const char *const UXS_DIV_F[10] = {"1/4 NOTE", "1/8 NOTE", "1/16 NOTE", "1/32 NOTE", "1/8 TRIPLET", "1/16 TRIPLET",
                                          "1/2 NOTE", "1 BAR", "2 BARS", "4 BARS"};
static const char *const UXS_APDIV_F[8] = {"4 BARS", "2 BARS", "1 BAR", "1/2 NOTE", "1/4 NOTE", "1/8 NOTE", "1/16 NOTE",
                                           "1/32 NOTE"};
static const char *const UXS_SLDIV_K[6] = {"1/8", "1/16", "1/32", "1/8T", "1/16T", "1/32T"};
static const char *const UXS_SLDIV_F[6] = {"1/8 NOTE", "1/16 NOTE", "1/32 NOTE", "1/8 TRIPLET", "1/16 TRIPLET", "1/32 TRIPLET"};
static const char *const UXS_DTYPE_K[6] = {"SOFT", "HARD", "FOLD", "CRUSH", "RECTIFY", "AMP"};
static const char *const UXS_DTYPE_F[6] = {"SOFT CLIP", "HARD CLIP", "WAVEFOLD", "BIT CRUSH", "RECTIFY: AN OCTAVE UP",
                                           "GUITAR AMP"};
static const char *const UXS_RTYPE_K[5] = {"ROOM", "SPRING", "SHIMMER", "RESO", "CLOUD"};
static const char *const UXS_SLMODE_K[3] = {"OFF", "GATE", "STUTTER"};
static const char *const UXS_SLMODE_F[3] = {"OFF", "GATE: CUTS THE GAPS", "STUTTER: REPEATS IN THE GAPS"};
static const char *const UXS_LOAD_N[3] = {"", " IN LITE", " IN ECO"};
/* the words FX's full values are made of (UXF_*: an index each) */
static const char *const UXS_FX_SAY[] = {"FLAT", "OPEN", "MONO", "% DARKER", "% BRIGHTER", "% THINNER", "% NARROWER", "% WIDER",
                                         " STEPS", ": AS ", " AT MOST", ": ", "OTHERS ", " ON A KICK", ": DRIVE X",
                                         ": DRUMS ", " SCALE STEPS", "TUNED TO ", "FREE: RATE IN HZ", "BEAT: IN TIME",
                                         ": LEFT RIGHT LEFT", "ANALOG: WARM REPEATS", "DIGITAL: EXACT REPEATS",
                                         ": THE FX RETURNS TOO", "ANY: EVERY SEMITONE", "SCALE: ONTO THE SCALE", " NOTE", ": GROWS",
                                         ": ONE ECHO", ": NO CHORUS IN ECO", ": OFF", "ANY", "SCALE"};
enum { UXF_FLAT, UXF_OPEN, UXF_MONO, UXF_DARK, UXF_BRIGHT, UXF_THIN, UXF_NARROW, UXF_WIDE, UXF_STEPS, UXF_AS, UXF_LONGEST,
       UXF_COLON, UXF_OTHERS, UXF_KICK, UXF_DRIVE, UXF_DRUMS, UXF_SCLSTEPS, UXF_TUNED, UXF_AP_FREE, UXF_AP_BEAT, UXF_PING,
       UXF_ANALOG, UXF_DIGITAL, UXF_SLFX, UXF_SNAP0, UXF_SNAP1, UXF_NOTE, UXF_GROWS, UXF_ONE, UXF_NOCHO, UXF_OFFIN, UXF_ANY,
       UXF_SCALE, UXF_N };
typedef char uxs_fx_say_ok[NELEM(UXS_FX_SAY) == UXF_N ? 1 : -1];
/* a bipolar amount v (0 at w: below to w - 64, above to w + 63) as the knob shows it, "-50%" / "+50%" / zero's word; in full
 * "50% DARKER" */
static void uxs_bip(int32_t v, int32_t w, uint32_t zero, uint32_t neg, uint32_t pos, char *b, uint32_t nb, int full)
{
    int32_t p = v < w ? (v - w) * 100 / 64 : (v - w) * 100 / 63;
    if (v == w) {
        str_cpy(b, UXS_FX_SAY[zero], nb);
        return;
    }
    if (full) {
        fmt_int(b, p < 0 ? -p : p);
        uxs_cat(b, nb, UXS_FX_SAY[p < 0 ? neg : pos]);
        return;
    }
    str_cpy(b, p > 0 ? "+" : "", nb);
    fmt_int(b + str_len(b), p);
    uxs_cat(b, nb, "%");
}
static void uxs_pct(int32_t v, int32_t max, char *b, uint32_t nb)   /* "50%" (F_PCT's, of max) */
{
    fmt_int(b, (v * 100 + max / 2) / max);
    uxs_cat(b, nb, "%");
}
/* setting id's (a G_* when glo, else the track's) value v in FX's words: 1 done (b holds it), 0 not one of these (uxs_val's) */
static int uxs_fx_val(uint32_t glo, uint32_t id, int32_t v, char *b, uint32_t nb, int full)
{
    char t[16];
    uint32_t ld = (uint32_t)LOAD_EFF() % 3u;
    if (!glo) {
        if (!v && (id == P_DIST || id == P_CHOR || id == P_DLY || id == P_REV || id == P_APDEP)) {
            str_cpy(b, N_ONOFF[0], nb);
            return 1;
        }
        if (id == P_CHOR && full && sys_load == LOAD_ECO) {   /* (fx.c: LOAD ECO no chorus, the send goes nowhere) */
            uxs_pct(v, 127, b, nb);
            uxs_cat(b, nb, UXS_FX_SAY[UXF_NOCHO]);
            return 1;
        }
        if (id == P_DTYPE) {
            v = clamp(v, 0, 5);
            str_cpy(b, full ? UXS_DTYPE_F[v] : UXS_DTYPE_K[v], nb);
            return 1;
        }
        if (id == P_DTONE) {
            uxs_bip(v, 0, UXF_FLAT, UXF_DARK, UXF_BRIGHT, b, nb, full);
            return 1;
        }
        if (id == P_APRATE) {                           /* (BEAT: a division, params.c AP_BEAT; FREE: the LFO's rate) */
            if (TSEL->p[P_APSYNC])
                str_cpy(b, full ? UXS_APDIV_F[clamp(v, 0, 7)] : N_APDIV[clamp(v, 0, 7)], nb);
            else
                uxs_lrate(v, b, nb, full);
            return 1;
        }
        if (id == P_APSYNC) {
            str_cpy(b, full ? UXS_FX_SAY[v ? UXF_AP_BEAT : UXF_AP_FREE] : N_APSYNC[v != 0], nb);
            return 1;
        }
        if (id == P_APWAVE && v >= 0 && v < 5) {        /* (S&H: "&" in no ux face; LFO's words) */
            str_cpy(b, full ? UXS_LWAVE_F[v] : UXS_LWAVE_K[v], nb);
            return 1;
        }
        return 0;
    }
    switch (id) {
    case G_DTIME:
        if (v >= 0 && v < DT_FREE) {                    /* (a division: in full, the time it plays at the tempo) */
            str_cpy(b, full ? UXS_DIV_F[v] : UXS_DIV_K[v], nb);
            if (full) {
                uint32_t d = uxs_dly_smp();
                uxs_cat(b, nb, UXS_FX_SAY[UXF_COLON]);
                uxs_mstxt(t, sizeof t, uxs_smp_ms(d));
                uxs_cat(b, nb, t);
                if (d < div_samples((uint32_t)v))       /* (past what the line holds: its longest) */
                    uxs_cat(b, nb, UXS_FX_SAY[UXF_LONGEST]);
            }
        } else
            uxs_mstxt(b, nb, DTIME_MS[(uint32_t)(v - DT_FREE) % NELEM(DTIME_MS)]);
        return 1;
    case G_DFDBK:
        fmt_int(b, v);
        uxs_cat(b, nb, "%");
        if (full && !v)
            uxs_cat(b, nb, UXS_FX_SAY[UXF_ONE]);
        else if (full && v > 100 && song.g[G_DTYPE] != 1)   /* (ANALOG past 100 %: it grows into its saturation, fx.c dly_sat) */
            uxs_cat(b, nb, UXS_FX_SAY[UXF_GROWS]);
        return 1;
    case G_DCOLOR:                                      /* (TONE: 64 open, below darker, above thinner: fx.c tone_set) */
        uxs_bip(v, 64, UXF_OPEN, UXF_DARK, UXF_THIN, b, nb, full);
        return 1;
    case G_DPIT: case G_DSPRY:                          /* (the GRAIN: LOAD LITE / ECO a plain delay, fx_buses) */
        if (!v) {
            str_cpy(b, N_ONOFF[0], nb);
            return 1;
        }
        if (!full || !ld)
            return 0;
        uxs_val(&GP[id], v, b, nb);
        uxs_cat(b, nb, UXS_FX_SAY[UXF_OFFIN]);
        uxs_cat(b, nb, UXS_LOAD_N[ld]);
        return 1;
    case G_DMIX: case G_WIDTH: case G_DMOD: case G_RMOD: case G_RWIDE: case G_CDEPTH: case G_SLDEP:
        if (v)
            return 0;
        str_cpy(b, N_ONOFF[0], nb);
        return 1;
    case G_DTYPE:
        str_cpy(b, full ? UXS_FX_SAY[v ? UXF_DIGITAL : UXF_ANALOG] : N_DLYTYPE[v != 0], nb);
        return 1;
    case G_DPING: case G_SLFX:
        str_cpy(b, N_ONOFF[v != 0], nb);
        if (full && v)
            uxs_cat(b, nb, UXS_FX_SAY[id == G_SLFX ? UXF_SLFX : UXF_PING]);
        return 1;
    case G_RTYPE: {
        uint32_t e = uxs_rev_eff();
        v = clamp(v, 0, 4);
        str_cpy(b, UXS_RTYPE_K[v], nb);
        if (full && e != (uint32_t)v) {                 /* (LOAD LITE / ECO: its kin plays, fx.c REV_LITE) */
            uxs_cat(b, nb, UXS_FX_SAY[UXF_AS]);
            uxs_cat(b, nb, UXS_RTYPE_K[e]);
            uxs_cat(b, nb, UXS_LOAD_N[ld]);
        }
        return 1;
    }
    case G_RDAMP:
        if (song.g[G_RTYPE] != 3)
            return 0;
        v = clamp(v, 0, 6);                             /* (RESO: FOLLOW, the track whose notes its strings take) */
        str_cpy(b, full && v ? UXS_FX_SAY[UXF_TUNED] : "", nb);
        uxs_cat(b, nb, N_RFOLW[v]);
        return 1;
    case G_RPRE:
        if (!v) {
            str_cpy(b, N_ONOFF[0], nb);
            return 1;
        }
        if (song.g[G_RTYPE] == 3) {                     /* (RESO: PITCH, its chord this many steps up the scale) */
            str_cpy(b, "+", nb);
            fmt_int(b + 1, v);
            if (full)
                uxs_cat(b, nb, UXS_FX_SAY[UXF_SCLSTEPS]);
            return 1;
        }
        if (song.g[G_RTYPE] == 2 || song.g[G_RTYPE] == 4)
            return 0;                                   /* (AIR: semitones, uxs_val's) */
        uxs_mstxt(b, nb, uxs_smp_ms((uint32_t)clamp(v, 0, 100) * 40u));   /* (PRE: fx_buses' v x 40 samples) */
        return 1;
    case G_RRATE: case G_CRATE:
        uxs_lrate(v, b, nb, full);
        return 1;
    case G_RFILT:
        uxs_bip(v, 0, UXF_FLAT, UXF_DARK, UXF_BRIGHT, b, nb, full);
        return 1;
    case G_CSTER:
        if (v <= -64) {
            str_cpy(b, UXS_FX_SAY[UXF_MONO], nb);
            return 1;
        }
        uxs_bip(v, 0, UXF_FLAT, UXF_NARROW, UXF_WIDE, b, nb, full);
        return 1;
    case G_SLMODE:
        v = clamp(v, 0, 2);
        str_cpy(b, full ? UXS_SLMODE_F[v] : UXS_SLMODE_K[v], nb);
        return 1;
    case G_SLPAT:
        fmt_int(b, v);
        if (full) {                                     /* (the rhythm itself: X open, . a gap, slicer.c SL_PAT) */
            uint32_t i, pat = SL_PAT[(uint32_t)(v - 1) % SL_NPAT], n;
            uxs_cat(b, nb, UXS_FX_SAY[UXF_COLON]);
            n = str_len(b);
            for (i = 0; i < 16u && n + 1u < nb; i++)
                b[n++] = (pat >> i) & 1u ? 'X' : '.';
            b[n] = 0;
        }
        return 1;
    case G_SLRATE:
        v = clamp(v, 0, 5);
        str_cpy(b, full ? UXS_SLDIV_F[v] : UXS_SLDIV_K[v], nb);
        return 1;
    case G_SLPLEN: case G_PFXLEN:
        fmt_int(b, v);
        if (full)
            uxs_cat(b, nb, UXS_FX_SAY[UXF_STEPS]);
        return 1;
    case G_SLPQ:
        str_cpy(b, UXS_FX_SAY[full ? (v ? UXF_SNAP1 : UXF_SNAP0) : v ? UXF_SCALE : UXF_ANY], nb);
        return 1;
    case G_PFXRES:
        str_cpy(b, N_PFXRES[clamp(v, 0, 2)], nb);
        if (full)
            uxs_cat(b, nb, UXS_FX_SAY[UXF_NOTE]);
        return 1;
    case G_CLIP:                                        /* (fx.c master_begin: the mix driven x1 .. x4 into the soft clip) */
        if (!v) {
            str_cpy(b, N_ONOFF[0], nb);
            return 1;
        }
        fmt_int(b, v);
        uxs_cat(b, nb, "%");
        if (full) {
            uxs_cat(b, nb, UXS_FX_SAY[UXF_DRIVE]);
            uxs_fix1(t, sizeof t, 10 + v * 30 / 100);
            uxs_cat(b, nb, t);
        }
        return 1;
    case G_PUNCH:                                       /* (eng_drum.c punch_gain: a hit's body up to x1.7) */
        if (!v) {
            str_cpy(b, N_ONOFF[0], nb);
            return 1;
        }
        fmt_int(b, v);
        uxs_cat(b, nb, "%");
        if (full) {
            uxs_cat(b, nb, UXS_FX_SAY[UXF_DRUMS]);
            uxs_dbtxt(t, sizeof t, uxs_db10(32768u + (uint32_t)clamp(v, 0, 100) * 230u, 32768u), 1);
            uxs_cat(b, nb, t);
        }
        return 1;
    case G_DUCK: {                                      /* (fx.c duck_block: the others down by up to 30 dB on a kick) */
        uint32_t g = 32767u - (uint32_t)(clamp(v, 0, 100) * 31731 / 100);
        if (!v) {
            str_cpy(b, N_ONOFF[0], nb);
            return 1;
        }
        str_cpy(b, full ? UXS_FX_SAY[UXF_OTHERS] : "", nb);
        uxs_dbtxt(t, sizeof t, uxs_db10(g, 32767u), 0);
        uxs_cat(b, nb, t);
        if (full)
            uxs_cat(b, nb, UXS_FX_SAY[UXF_KICK]);
        return 1;
    }
    case G_DREL:                                        /* (.. its release, 40 .. 600 ms: dv_kb(40000 + REL x 5600) us) */
        uxs_mstxt(b, nb, 40u + (uint32_t)clamp(v, 0, 100) * 56u / 10u);
        return 1;
    default:
        return 0;
    }
}
static const char *uxs_vfull(uint32_t glo, uint32_t id, int32_t v)   /* value v of setting id in full (0: as the knob's) */
{
    static char b[32];
    if (uxs_fx_val(glo, id, v, b, sizeof b, 1))         /* (3b task 8) */
        return b;
    if (!glo && uxs_lfo_val(id, v, b, sizeof b, 1))   /* (3b task 7) */
        return b;
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
    if (uxs_fx_val(glo, id, v, b, nb, 0))               /* (3b task 8) FX's */
        return;
    if (!glo && uxs_lfo_val(id, v, b, nb, 0))         /* (3b task 7) LFO's, the PITCH amounts, every amount's OFF */
        return;
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
#define UXS_LANE_SAY "REC + PLAY, HOLD FX + A KEY"      /* (3b task 8) FX > MORE's PUNCH LANE rows: how a lane is recorded (was
                                                         * PUNCH's hint, ui_layer.c hint_tick) */
static uint16_t uxs_seen;        /* the sections opened since power-on (a bit per SEC_*): their hint said */
static uint8_t uxs_hint_sec = 0xFF, uxs_hint_page;      /* the page saying its section's hint (0xFF none): the first one
                                                         * opened there, until it is left or a knob turns */
static uint32_t uxs_frame;                              /* the UI frame last drawn here (another in between: left) */
static uint32_t uxs_panels;                             /* the panels drawn (tests/ui_test.c: one a change, none idle) */
static uint32_t uxs_lfo_sc, uxs_lfo_ms;                 /* (3b task 7) LFO's wave: where it was drawn from, when (ms) */
static int32_t uxs_mtr_px;                              /* (3b task 8) FX > MASTER's meter as drawn (px), SPACE's tempo key, */
static uint32_t uxs_fx_ms, uxs_sp_win;                  /* .. when either was drawn (ms) */
static uint32_t uxs_heavy_ms;                           /* (3b task 10) the panel's last drawing (ms) and its page: NOISE's and */
static const page_t *uxs_heavy_pg;                      /* .. DRUM-X's pictures paced from it */

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
        k[c].off = 0;
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
        char v[32];                                     /* (3b task 7 fix: a matrix name and what it is, uxs_mtx_say) */
        uint8_t bar;
        if (pg->graph == GR_MORE && pg->more && h < 2) {   /* (KNOB 1 or 2: the setting picked) */
            uint32_t s = uxm_at[uxm_key()].sel, t = uxm_at[uxm_key()].top;
            uxm_fix(pg->more, uxm_mask(pg->more), &s, &t);
            if (s < uxm_n(pg->more)) {
                const ux_row_t *r = &pg->more->rows[s];
                int16_t *vp;
                nm = uxm_row_text(r, v, sizeof v, &bar);
                if ((r->kind == UXR_TRACK || r->kind == UXR_GLOBAL) && param_ref(r->kind == UXR_GLOBAL ? 0 : TSEL, r->id, &vp) &&
                    vp && uxs_vfull(r->kind == UXR_GLOBAL, r->id, *vp))   /* (3b task 7: in full, as a page's knob) */
                    str_cpy(v, uxs_vfull(r->kind == UXR_GLOBAL, r->id, *vp), sizeof v);
            }
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
    if (pg->graph == GR_MORE && pg->more) {             /* (3b task 8) a PUNCH LANE row picked: how a lane is recorded */
        uint32_t s = uxm_at[uxm_key()].sel, t = uxm_at[uxm_key()].top;
        uxm_fix(pg->more, uxm_mask(pg->more), &s, &t);
        if (uxm_head_of(pg->more, s) == UXM_H_LANE)
            return UXS_LANE_SAY;
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

/* ------------------------------------------------------------------------------------------------------- LFO --- */
/* (3b task 7) LFO's pages (spec 2.3), each picture from the engine's own LFO (voice.c lfo_wave, track_lfo_tick) and matrix (mod.c):
 * LFO  the wave over a window of the tempo (1 BAR; 4 BARS when a cycle is longer than a bar, 1 BEAT when more than eight fit in
 *      one), scrolling as the LFO runs: the phase now at the playhead a quarter in (what it played dim behind it), the level
 *      it sends now a dot (its FADE in it), a tick on the axis where each cycle starts (RATE's colour), the wave in WAVE's,
 *      MODE's words at the top right in MODE's, FADE as a ramp from the playhead along the foot (how long the LFO takes to
 *      reach its depth after a note), in FADE's. RANDOM (S&H) a level a cycle: this cycle's the engine's own, those played
 *      as they were drawn, those to come a hash of their number (what they will be is not known; 3b task 7 fix: uxs_rh). The
 *      scroll is the one animated ux picture: it redraws when the wave moved a column or the tempo moved the window, no more
 *      often than HOME's ridges (ux_pace_at UX_TRK_MS, slower while the load is tight), frozen while the guard sheds
 *      (UX_SHED_HOLD_MS) or the governor's LOAD LITE is on; with MENU > ANIM OFF it does not scroll
 * DEST the four amounts as rows, each the LFO's wave (a note's: from PHASE in TRIG) moved by its amount: PITCH FILTER SHAPE
 *      up and down (a negative amount turns the wave over), AMP dipping from the top (the level it takes away), each in its
 *      knob's colour with its value (PITCH in semitones, as the engine moves it)
 * MOD MATRIX the four slots as rows: the source's box, a line to the destination's box as thick as AMOUNT, AMOUNT on it;
 *      the slot KNOB 1 picked lit, an OFF slot dim */
#define UXS_LX0 14               /* the wave's left, its width (columns), the playhead's column */
#define UXS_LW 212
#define UXS_LXP (UXS_LW / 4)
static const uint16_t UXS_LSYNC_U[12] = {384, 192, 96, 48, 24, 18, 16, 12, 8, 6, 4, 3};   /* (voice.c track_lfo_tick's U: 1/24
                                                         * of a quarter, N_LSYNC's 4BAR .. 1/32) */
static const char *const UXS_LWIN[3] = {"1 BEAT", "1 BAR", "4 BARS"};
/* n / d for the pictures (the firmware links no 64-bit division: __udivdi3), by shifts; d > 0 */
static __attribute__((noinline)) uint64_t uxs_div64(uint64_t n, uint32_t d)
{
    uint64_t q = 0, r = 0;
    int32_t i;
    for (i = 63; i >= 0; i--) {
        r = r << 1 | (n >> i & 1u);
        if (r >= d) {
            r -= d;
            q |= 1ull << i;
        }
    }
    return q;
}
typedef struct { uint32_t win, cyc, inc; uint8_t wb; } uxs_lw_t;   /* the window (samples), a cycle (samples), the phase a column
                                                         * (2^32 a cycle), which window (UXS_LWIN) */
static void uxs_lfo_win(const track_t *t, uxs_lw_t *w)
{
    uint32_t beat = beat_samples(), r;
    int32_t v = t->p[P_LRATE];
    uint64_t cyc;
    if (v < 0) {                                        /* (in time: track_lfo_tick's len) */
        cyc = beat * UXS_LSYNC_U[(uint32_t)(-v - 1) % 12u] / 24u;
        if (cyc < 64u)
            cyc = 64u;
    } else
        cyc = uxs_div64((uint64_t)CTL << 32, LFO_INC[v & 127]);   /* (free: LFO_INC a block of CTL samples) */
    w->wb = 1u;                                         /* (1 BAR; 4 BARS for a cycle past a bar; 1 BEAT for more than 8) */
    if (cyc > 4ull * beat)
        w->wb = 2u;
    else if (cyc * 8u < 4ull * beat)
        w->wb = 0u;
    w->win = beat * (w->wb == 2u ? 16u : w->wb ? 4u : 1u);
    w->cyc = (uint32_t)(cyc > 0xFFFFFFFFull ? 0xFFFFFFFFull : cyc);
    r = (uint32_t)uxs_div64((uint64_t)w->win << 32, (uint32_t)(UXS_LW * cyc));   /* (UXS_LW x a cycle: within 32 bits) */
    w->inc = r ? r : 1u;
}
/* (3b task 7 fix) RANDOM's cycles as the LFO page saw them (uxs_lfo_poll, each frame it is shown): the shown track's cycles
 * counted since it was first shown (cyc: the one playing now), its phase and level then, and the levels of the last
 * UXS_RH_N cycles played (past[0] the one just ended; its seen bit clear: not seen, the frames missed it): the past keeps the level
 * it was drawn with, a cycle to come the hash of its own number (it scrolls in unchanged; its real level is the engine's
 * draw, shared with the engines' noise: not known before it plays) */
#define UXS_RH_N 4u
static struct { uint32_t cyc, ph, rnd, ms, win; int16_t past[UXS_RH_N]; uint8_t trk, seen; } uxs_rh;   /* (trk: the track + 1,
                                                         * 0 none; seen: a bit a past[] level, it was seen; win: the LFO and DEST
                                                         * pictures' window when last drawn, ux_section_draw) */
static int32_t uxs_lfo_rlev(uint32_t r) { return (int32_t)(r >> 16) - 32768; }   /* (lfo_wave's RANDOM: a cycle's level) */
static void uxs_lfo_poll(const track_t *t, const uxs_lw_t *w)
{
    uint32_t ti = (uint32_t)(t - trk) + 1u, k, j;
    if (uxs_rh.trk != ti) {                             /* (another track shown: its count from here, its past unseen) */
        uxs_rh.trk = (uint8_t)ti;
        uxs_rh.cyc = 0;
        uxs_rh.seen = 0;
    } else if (t->lfo_rnd != uxs_rh.rnd || t->lfo_ph < uxs_rh.ph) {   /* (a cycle ended: track_lfo_tick drew a new level) */
        uint64_t e = (uint64_t)(fm1_ms - uxs_rh.ms) * FS / 1000u, a = e < 0xFFFFFFFFull ? uxs_div64(e << 32, w->cyc) : 0;
        int64_t q = ((int64_t)uxs_rh.ph + (int64_t)a - (int64_t)t->lfo_ph + 0x80000000ll) >> 32;   /* (how many: the time between,
                                                         * the phases; at least the one seen ending) */
        k = q < 1 ? 1u : q > 64 ? 64u : (uint32_t)q;
        for (j = 0; j < k; j++) {                       /* (the one seen, then those the frames missed) */
            uint32_t i;
            for (i = UXS_RH_N - 1u; i > 0u; i--)
                uxs_rh.past[i] = uxs_rh.past[i - 1u];
            uxs_rh.past[0] = (int16_t)uxs_lfo_rlev(uxs_rh.rnd);
            uxs_rh.seen = (uint8_t)(uxs_rh.seen << 1 | (j ? 0u : 1u));
        }
        uxs_rh.cyc += k;
    }
    uxs_rh.ph = t->lfo_ph;
    uxs_rh.rnd = t->lfo_rnd;
    uxs_rh.ms = fm1_ms;
}
/* the level (Q15, bipolar) the picture draws dc columns from now (the phase now ph): lfo_wave's at that phase; RANDOM's this
 * cycle the engine's own (lfo_rnd), one played as it was seen (uxs_rh), another a hash of its number */
static int32_t uxs_lfo_at(const track_t *t, const uxs_lw_t *w, uint32_t ph, int32_t dc)
{
    int64_t p = (int64_t)ph + (int64_t)dc * (int64_t)w->inc;
    int32_t d = (int32_t)(p >> 32);                     /* (the cycles from this one: floor) */
    uint32_t ti = (uint32_t)(t - trk) + 1u, h;
    int seen = uxs_rh.trk == ti;
    if (t->p[P_LWAVE] != 4)
        return lfo_wave((track_t *)t, (uint32_t)p);
    if (!d)
        return uxs_lfo_rlev(t->lfo_rnd);
    if (seen && d < 0 && (uint32_t)-d <= UXS_RH_N && (uxs_rh.seen >> (-d - 1) & 1u))
        return uxs_rh.past[-d - 1];
    h = ((seen ? uxs_rh.cyc : 0u) + (uint32_t)d) * 2654435761u ^ ti * 0x85EBCA6Bu;
    h ^= h >> 13;
    h *= 0xC2B2AE35u;
    h ^= h >> 16;
    return uxs_lfo_rlev(h);   /* (a level a cycle number: the same as it scrolls in) */
}
/* the columns FADE takes from a note to the LFO's whole depth (track_lfo_tick: its step a block); 0: no FADE */
static int32_t uxs_lfo_fade_cols(const track_t *t, const uxs_lw_t *w)
{
    int32_t st = (int32_t)(ENV_LIN[t->p[P_LFADE] & 127] >> 9);
    uint64_t smp;
    if (!t->p[P_LFADE])
        return 0;
    if (st < 1)
        st = 1;
    smp = (uint64_t)((32767 + st - 1) / st) * CTL;
    smp = uxs_div64(smp * UXS_LW, w->win);
    return (int32_t)(smp > 4096u ? 4096u : smp);
}
static uint32_t uxs_lfo_scroll(const track_t *t, const uxs_lw_t *w)   /* where the wave is drawn from: a column's step */
{
    return t->lfo_ph / w->inc + (t->p[P_LWAVE] == 4 ? t->lfo_rnd * 7u : 0u) +
           (uint32_t)(mulq15(t->lfo_val, t->lfo_fade) >> 9) * 131u;   /* (the dot: its level, 1/64 steps) */
}
static void uxs_lfo_draw(const track_t *t, const char *name)
{
    uxs_lw_t w;
    int32_t cy = 74, a = 40, c, y, py = 0, now, fc, x, yb = 128;
    uint32_t ph = t->lfo_ph, beats, b, k;
    uint16_t past = ux_mix(UX_BG, UX_SLIDE, 45);
    const char *say;
    uxs_lfo_win(t, &w);
    cv_rect(UXS_LX0, cy, UXS_LW, 1, UX_LINE);
    for (c = 1; c < UXS_LW; c++) {                      /* each cycle's start: a tick on the axis (RATE's), under the wave */
        int64_t p0 = (int64_t)ph + (int64_t)(c - 1 - UXS_LXP) * w.inc, p1 = p0 + w.inc;
        if ((p0 >> 32) != (p1 >> 32))
            cv_rect(UXS_LX0 + c, cy - 3, 1, 7, UX_ACID);
    }
    for (c = 0; c < UXS_LW; c++) {                      /* the wave */
        y = cy - uxs_lfo_at(t, &w, ph, c - UXS_LXP) * a / 32768;
        if (c)
            cv_line_t(UXS_LX0 + c - 1, py, UXS_LX0 + c, y, c <= UXS_LXP ? past : UX_SLIDE, 2);
        py = y;
    }
    beats = w.wb == 2u ? 16u : w.wb ? 4u : 1u;          /* the beats along the foot, the bars taller (1 BEAT: its sixteenths) */
    cv_rect(UXS_LX0, yb + 6, UXS_LW, 1, UX_LINE);
    for (b = 0; b <= (beats > 1u ? beats : 4u); b++) {
        uint32_t n = beats > 1u ? beats : 4u;
        int bar = beats > 1u ? !(b % 4u) : !b || b == n;
        cv_rect(UXS_LX0 + (int32_t)(b * (UXS_LW - 1) / n), yb + (bar ? 2 : 4), 1, bar ? 4 : 2, UX_LINE);
    }
    fc = uxs_lfo_fade_cols(t, &w);                      /* FADE: a ramp from the playhead to the LFO's whole depth */
    if (fc > 0) {
        int32_t xe = UXS_LXP + fc;
        if (xe > UXS_LW - 1)
            xe = UXS_LW - 1;
        for (x = UXS_LXP; x <= xe; x++) {
            int32_t h = fc ? (x - UXS_LXP) * 6 / fc : 6;
            cv_rect(UXS_LX0 + x, yb - h, 1, h + 1, UX_ACCENT);
        }
        if (xe < UXS_LW - 1)
            cv_rect(UXS_LX0 + xe, yb - 6, UXS_LW - 1 - xe, 1, UX_ACCENT);
        GFX_HOOK_ALIGN(0, yb - 6 + UF_LBL_CAP_Y, 0, yb - 6 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "LFO's FADE");
        cv_text_on(UXS_LX0, yb - 6, &UF_LBL, "FADE", UX_ACCENT, ux_under(UXS_LX0, yb - 2));
    }
    cv_rect(UXS_LX0 + UXS_LXP, cy - a - 4, 1, 2 * a + 8, UX_MID);   /* the playhead, the level sent now */
    now = cy - mulq15(t->lfo_val, t->lfo_fade) * a / 32768;
    cv_rect(UXS_LX0 + UXS_LXP - 2, now - 2, 5, 5, UX_TEXT);
    uxs_pic_head(t, name);
    say = UXS_LMODE_SAY[lfo_mode_of(t)];
    k = (uint32_t)(228 - text_w(&UF_LBL, say));
    GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "LFO picture's MODE");
    cv_text_on((int32_t)k, 8, &UF_LBL, say, UX_VIOLET, ux_under((int32_t)k, 12));
    k = (uint32_t)(228 - text_w(&UF_LBL, UXS_LWIN[w.wb]));
    GFX_HOOK_ALIGN(0, yb + 9 + UF_LBL_CAP_Y, 0, yb + 9 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "LFO picture's window");
    cv_text_on((int32_t)k, yb + 9, &UF_LBL, UXS_LWIN[w.wb], UX_MID, ux_under((int32_t)k, yb + 13));
}
/* DEST: a note's LFO (TRIG: from PHASE; FREE: from 0) over the LFO page's window, each destination's row */
static void uxs_lfo_dest_draw(const track_t *t, const page_t *pg, const char *name)
{
    uxs_lw_t w;
    uint32_t k, ph0 = t->p[P_LPHASE] > 0 ? (uint32_t)t->p[P_LPHASE] << 25 : 0u;
    int32_t c, x0 = 70, wd = 116, h = 9;
    uxs_lfo_win(t, &w);
    w.inc = (uint32_t)uxs_div64((uint64_t)w.inc * UXS_LW, (uint32_t)wd);   /* (the same window in the row's width) */
    for (k = 0; k < 4u; k++) {
        int32_t mid = 38 + (int32_t)k * 28, am = t->p[pg->id[k]], py = 0, y;
        uint16_t col = ux_knob_col(k);
        char v[16];
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, k, &vp);
        GFX_HOOK_ALIGN(0, mid - 4 + UF_LBL_CAP_Y, 0, mid - 4 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "DEST's row");
        cv_text_on(UXS_LX0, mid - 4, &UF_LBL, uxs_label(UXS_SEC_LFO, 0, pg->id[k], d ? d->label : ""), am ? UX_TEXT : UX_MID,
                   ux_under(UXS_LX0, mid));
        v[0] = 0;
        if (d && vp)
            uxs_vtext(d, *vp, 0, pg->id[k], v, sizeof v);
        cv_text_r(228, mid - 4, &UF_LBL, v, am ? col : UX_MID, ux_under(200, mid));
        cv_rect(x0, k == 3u ? mid - h : mid, wd, 1, UX_LINE);
        if (!am)
            continue;
        for (c = 0; c < wd; c++) {
            int32_t l = uxs_lfo_at(t, &w, ph0, c);
            if (k == 3u)                                /* AMP: the level it takes away, (lfo + 1) / 2 x AMP (voice.c) */
                y = mid - h + mulq15((l + 32768) >> 1, am * 258) * 2 * h / 32768;
            else
                y = mid - l * am / 64 * h / 32768;
            if (c)
                cv_line_t(x0 + c - 1, py, x0 + c, y, col, 2);
            py = y;
        }
    }
    uxs_pic_head(t, name);
}
/* MOD MATRIX: a row a slot, SOURCE's box to DEST's, the line AMOUNT thick (its colour), its value over it */
static void uxs_mod_box(int32_t x, int32_t y, const char *s, uint16_t frame, uint16_t ink, int lit)
{
    uint16_t bg = lit ? UX_BG2 : UX_BG;
    cv_rect(x, y, 70, 18, bg);
    cv_rect(x, y, 70, 1, frame); cv_rect(x, y + 17, 70, 1, frame);
    cv_rect(x, y, 1, 18, frame); cv_rect(x + 69, y, 1, 18, frame);
    GFX_HOOK_ALIGN(0, y + 5 + UF_LBL_CAP_Y, 0, y + 5 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "MOD MATRIX's box");
    cv_text_on(x + 5, y + 5, &UF_LBL, s, ink, bg);
}
static void uxs_mod_draw(const track_t *t, const char *name)
{
    uint32_t k;
    for (k = 0; k < NMSLOT; k++) {
        const int16_t *p = &t->p[P_M1SRC + 3u * k];
        int32_t y = 28 + (int32_t)k * 30, on = p[0] > 0 && p[1] > 0 && p[2], sel = k == (mod_ui_slot & 3u), a = p[2] < 0 ? -p[2] : p[2];
        char s[24], d[24], b[2] = {(char)('1' + k), 0};
        uint16_t dim = sel ? UX_MID : ux_mix(UX_BG, UX_MID, 55);
        uxs_mtx_say(t, 0, p[0], s, sizeof s, 0);       /* (the knob's names: 3b task 7 fix) */
        uxs_mtx_say(t, 1, p[1], d, sizeof d, 0);
        GFX_HOOK_ALIGN(0, y + 5 + UF_LBL_CAP_Y, 0, y + 5 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "MOD MATRIX's slot");
        cv_text_on(UXS_LX0, y + 5, &UF_LBL, b, sel ? UX_ACID : UX_MID, ux_under(UXS_LX0, y + 9));
        uxs_mod_box(26, y, s, sel ? UX_SLIDE : UX_LINE, p[0] ? (sel ? UX_TEXT : UX_MID) : dim, sel);
        uxs_mod_box(158, y, d, sel ? UX_VIOLET : UX_LINE, p[1] ? (sel ? UX_TEXT : UX_MID) : dim, sel);
        if (on) {
            char v[12];
            int32_t th = 1 + a * 4 / 64, x;
            uxs_vtext(&TP[P_M1AMT], p[2], 0, P_M1AMT + 3u * k, v, sizeof v);
            cv_line_t(96, y + 9, 158, y + 9, sel ? UX_ACCENT : ux_mix(UX_BG, UX_ACCENT, 55), th);
            x = 127 - text_w(&UF_LBL, v) / 2;
            GFX_HOOK_ALIGN(0, y - 4 + UF_LBL_CAP_Y, 0, y - 4 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "MOD MATRIX's amount");
            cv_text_on(x, y - 4, &UF_LBL, v, sel ? UX_ACCENT : UX_MID, ux_under(x, y));
        } else
            cv_rect(100, y + 9, 54, 1, ux_mix(UX_BG, UX_LINE, 60));
    }
    uxs_pic_head(t, name);
}

/* ---------------------------------------------------------------------------------------------------------------- FX (3b task 8) --- */
/* SENDS: four send bars, one under each knob (DISTORT CHORUS DELAY REVERB: the track's, 0 .. 127), in its knob's colour; under
 *        each what it goes to: DISTORT its TYPE, CHORUS nothing (LOAD ECO: OFF, fx.c fx_buses), DELAY its TIME, REVERB its TYPE */
#define UXS_BY0 24               /* the bars' top, their height, the captions' row */
#define UXS_BH 80
#define UXS_BCAP 110
static void uxs_sends_draw(const track_t *t, const page_t *pg, const char *name)
{
    uint32_t k;
    char b[16];
    uxs_pic_head(t, name);
    for (k = 0; k < 4u; k++) {
        int32_t cx = UX_COL_X0 + (int32_t)k * UX_COL_PITCH + UX_COL_W / 2, v = clamp(t->p[pg->id[k]], 0, 127), h = v * UXS_BH / 127;
        const char *cap = b;
        cv_rect(cx - 10, UXS_BY0, 20, UXS_BH, UX_EMPTY);
        if (h > 0)
            cv_rect(cx - 10, UXS_BY0 + UXS_BH - h, 20, h, k == 1u && sys_load == LOAD_ECO ? ux_mix(UX_BG, ux_knob_col(k), 40) :
                                                                                            ux_knob_col(k));
        b[0] = 0;
        if (k == 0u)
            cap = UXS_DTYPE_K[clamp(t->p[P_DTYPE], 0, 5)];
        else if (k == 1u)
            cap = sys_load == LOAD_ECO ? "ECO: OFF" : "";
        else if (k == 2u)
            (void)uxs_fx_val(1, G_DTIME, song.g[G_DTIME], b, sizeof b, 0);
        else
            cap = UXS_RTYPE_K[uxs_rev_eff()];
        if (cap[0]) {
            int32_t w = text_w(&UF_LBL, cap);
            GFX_HOOK_ALIGN(cx - w / 2, UXS_BCAP + UF_LBL_CAP_Y, cx - w / 2 + w, UXS_BCAP + UF_LBL_CAP_Y + UF_LBL_CAP_H,
                           AL_V | AL_PASS, "SENDS' caption");
            cv_text_on(cx - w / 2, UXS_BCAP, &UF_LBL, cap, v ? UX_MID : ux_mix(UX_BG, UX_MID, 60), ux_under(cx, UXS_BCAP + 4));
        }
    }
}
/* SPACE: a note sent to the buses and what comes back over time, as the engine plays it: along the axis the time (the beats under
 * it), up the level in dB (0 at the top, -60 at the foot); the note at 0, the delay's taps TIME apart (uxs_dly_smp), each as loud
 * as MIX and the FEEDBACK before it (fx.c dly_fb: ANALOG past 100 % grows into its saturation, held at the top; DIGITAL at most
 * 100 %), a line through their tops; the reverb's tail after its pre-delay, a line falling 60 dB in its RT60 (uxs_tail_ms: the model
 * playing, LOAD LITE / ECO's kin; SHIMMER with MOD and CLOUD past the window). The window the taps' and the tail's (a quarter of a
 * second .. 8 s). A track that sends nothing to one: it says so. A turn redraws it, once; a tempo change (an outside clock) paced */
#define UXS_SX0 14               /* the axis: its left, its width (columns), 0 dB's row and -60's */
#define UXS_SW 212
#define UXS_SY0 24
#define UXS_SY1 118
static const uint16_t UXS_SWIN[9] = {250, 500, 1000, 1500, 2000, 3000, 4000, 6000, 8000};   /* (the windows, ms) */
typedef struct { uint32_t T, pre, t60, win; int16_t lv[24]; uint8_t n; } uxs_sp_t;   /* (samples; lv: a tap's dB x 10) */
static void uxs_space_model(uxs_sp_t *m)
{
    int32_t fb = dly_fb(song.g[G_DFDBK]), mix = song.g[G_DMIX] * 258, l, step;
    uint32_t k, end, i;
    if (song.g[G_DTYPE] == 1 && fb > 32767)
        fb = 32767;                                     /* (DIGITAL: 100 % at most) */
    m->T = uxs_dly_smp();
    m->pre = uxs_rev_pre();
    m->t60 = uxs_tail_ms();
    l = mix > 0 ? uxs_db10((uint32_t)mix, 32768u) : -999;
    step = fb > 0 ? uxs_db10((uint32_t)fb, 32768u) : -999;
    for (k = 0; k < 24u && l > -600; k++) {
        m->lv[k] = (int16_t)(l > 0 ? 0 : l);
        l += step;
        if (l > 0)
            l = 0;                                      /* (grown to the top: held, fx.c dly_sat) */
    }
    m->n = (uint8_t)k;
    end = uxs_smp_ms(m->pre) + (m->t60 >= 8000u ? 8000u : m->t60);
    if (m->n && uxs_smp_ms(m->T) * m->n > end)
        end = uxs_smp_ms(m->T) * m->n;
    end += end / 12u;
    for (i = 0; i + 1u < NELEM(UXS_SWIN) && UXS_SWIN[i] < end; i++)
        ;
    m->win = UXS_SWIN[i];
}
static int32_t uxs_sx(const uxs_sp_t *m, uint32_t smp)  /* sample smp's column (past the window: off it) */
{
    uint64_t c = uxs_div64((uint64_t)smp * UXS_SW, uxs_ms_smp(m->win));
    return c > (uint64_t)UXS_SW ? UXS_SW + 1 : (int32_t)c;
}
static int32_t uxs_sy(int32_t d10) { return UXS_SY0 + clamp(-d10, 0, 600) * (UXS_SY1 - UXS_SY0) / 600; }   /* dB x 10's row */
static const char *const UXS_SP_SAY[] = {"NO DELAY OR REVERB SEND", "NO DELAY SEND", "NO REVERB SEND", "ECHO ", "TAIL ",
                                         "HOLDS"};
static void uxs_space_draw(const track_t *t, const char *name)
{
    uxs_sp_t m;
    uint32_t k, beat = beat_samples(), nb;
    int32_t x, xe = 0, py = 0, sd = t->p[P_DLY] > 0, sr = t->p[P_REV] > 0;
    uint16_t dcol = sd ? UX_ACID : ux_mix(UX_BG, UX_ACID, 40), fcol = sd ? UX_SLIDE : ux_mix(UX_BG, UX_SLIDE, 40);
    char b[32], c[16];
    uxs_space_model(&m);
    cv_rect(UXS_SX0, UXS_SY1 + 1, UXS_SW, 1, UX_LINE);
    nb = beat ? uxs_ms_smp(m.win) / beat : 0u;
    for (k = 1; nb <= 32u && k <= nb; k++) {            /* the beats (the bars taller) */
        x = UXS_SX0 + uxs_sx(&m, k * beat);
        cv_rect(x, UXS_SY1 + 2, 1, k % 4u ? 2 : 4, UX_LINE);
    }
    {   /* the reverb's tail: from its pre-delay at the top, 60 dB down in t60 (past the window: off its edge), its fill in REV
         * TYPE's colour, its line in REV SIZE's */
        uint16_t fill = ux_mix(UX_BG, UX_VIOLET, sr ? 34 : 14), lc = sr ? UX_ACCENT : ux_mix(UX_BG, UX_ACCENT, 40);
        uint32_t len = uxs_ms_smp(m.t60 >= 8000u ? 8000u : m.t60), span = uxs_ms_smp(m.win);
        int32_t x0 = UXS_SX0 + uxs_sx(&m, m.pre), hold = m.t60 == UXS_TAIL_LONG;
        for (x = x0; x < UXS_SX0 + UXS_SW; x++) {
            uint32_t smp = (uint32_t)uxs_div64((uint64_t)(uint32_t)(x - UXS_SX0) * span, UXS_SW);
            int32_t d10, y;
            smp = smp > m.pre ? smp - m.pre : 0u;
            d10 = hold ? -(int32_t)uxs_div64((uint64_t)smp * 60u, span) : -(int32_t)uxs_div64((uint64_t)smp * 600u, len ? len : 1u);
            if (d10 < -600)
                break;
            y = uxs_sy(d10);
            cv_rect(x, y, 1, UXS_SY1 + 1 - y, fill);
            if (x > x0)
                cv_line_t(x - 1, py, x, y, lc, 2);
            py = y;
        }
    }
    cv_rect(UXS_SX0, UXS_SY0, 2, UXS_SY1 + 1 - UXS_SY0, UX_TEXT);   /* the note */
    for (k = 0; k < m.n; k++) {                         /* the taps, a line through their tops (the FEEDBACK) */
        int32_t y = uxs_sy(m.lv[k]);
        x = UXS_SX0 + uxs_sx(&m, (k + 1u) * m.T);
        if (x >= UXS_SX0 + UXS_SW - 2)
            break;
        cv_rect(x, y, 3, UXS_SY1 + 1 - y, dcol);
        if (k)
            cv_line_t(xe + 1, py, x + 1, y, fcol, 1);
        xe = x;
        py = y;
    }
    uxs_pic_head(t, name);
    if (!sd || !sr) {                                   /* the top right: what this track does not send */
        b[0] = 'T'; b[1] = (char)('1' + song.sel % NTRK); b[2] = ':'; b[3] = ' '; b[4] = 0;
        uxs_cat(b, sizeof b, UXS_SP_SAY[!sd && !sr ? 0 : !sd ? 1 : 2]);
        x = 228 - text_w(&UF_LBL, b);
        GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "SPACE's note");
        cv_text_on(x, 8, &UF_LBL, b, UX_MID, ux_under(x, 12));
    }
    /* the foot: the echo's time, the window, the tail's length */
    str_cpy(b, UXS_SP_SAY[3], sizeof b);
    uxs_mstxt(c, sizeof c, uxs_smp_ms(m.T));
    uxs_cat(b, sizeof b, m.n ? c : N_ONOFF[0]);
    GFX_HOOK_ALIGN(0, 128 + UF_LBL_CAP_Y, 0, 128 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "SPACE's foot");
    cv_text_on(UXS_SX0, 128, &UF_LBL, b, sd ? UX_ACID : UX_MID, ux_under(UXS_SX0, 132));
    uxs_mstxt(c, sizeof c, m.win);
    x = (UXS_SX0 + 228) / 2 - text_w(&UF_LBL, c) / 2;
    cv_text_on(x, 128, &UF_LBL, c, UX_MID, ux_under(x, 132));
    str_cpy(b, UXS_SP_SAY[4], sizeof b);
    if (m.t60 == UXS_TAIL_LONG)
        str_cpy(c, UXS_SP_SAY[5], sizeof c);
    else
        uxs_mstxt(c, sizeof c, m.t60);
    uxs_cat(b, sizeof b, c);
    x = 228 - text_w(&UF_LBL, b);
    cv_text_on(x, 128, &UF_LBL, b, sr ? UX_VIOLET : UX_MID, ux_under(x, 132));
}
/* MASTER: the output as a level meter (OUT: the peak of audio.c's scope, the music after the master, dBFS, -48 .. 0), the clip
 * point marked (the limiter's ceiling, fx.c master_out: LIM_T into the soft clip, ~-6 dB; above it in ACCENT); CLIP's curve (fx.c
 * clip_block: the mix in, what goes on out; the straight line without it); after a kick (a DRUM-X hit) the drums' gain (eng_drum.c
 * punch_gain, PUNCH) and the others' (fx.c duck_block, DUCK, back at DUCK TIME's rate) over 0.8 s, +6 .. -30 dB, block by block as
 * the engine computes them (worked out once a change: uxs_kick). The meter moves as the sound does, paced as HOME's ridges
 * (ux_section_draw); the rest a turn redraws */
#define UXS_MX0 14               /* the meter: its left, width, row, height */
#define UXS_MW 212
#define UXS_MY 26
#define UXS_MH 10
#define UXS_MDB 48               /* its range, dB */
static int32_t uxs_out_db10(void)                       /* the output's peak now, dB x 10 (-UXS_MDB x 10: silence) */
{
    uint32_t w = scope_w, i;
    int32_t pk = 0, v;
    for (i = 1; i <= SCOPE_N; i++) {
        v = scope_buf[(w - i) & (SCOPE_N - 1u)];
        if (v < 0)
            v = -v;
        if (v > pk)
            pk = v;
    }
    return pk > 0 ? clamp(uxs_db10((uint32_t)pk, 32767u), -UXS_MDB * 10, 0) : -UXS_MDB * 10;
}
static int32_t uxs_meter_px(int32_t d10) { return clamp((d10 + UXS_MDB * 10) * UXS_MW / (UXS_MDB * 10), 0, UXS_MW); }
static void uxs_out_txt(int32_t lvl, char *b, uint32_t nb)   /* the meter's lvl (px) in words: "OUT -12DB", whole dB ("OUT OFF") */
{
    str_cpy(b, "OUT ", nb);
    if (lvl <= 0) {
        uxs_cat(b, nb, N_ONOFF[0]);
        return;
    }
    fmt_int(b + str_len(b), (lvl * UXS_MDB * 2 + UXS_MW) / (2 * UXS_MW) - UXS_MDB);   /* (rounded) */
    uxs_cat(b, nb, "DB");
}
static int32_t uxs_clip_out(int32_t x)                  /* fx.c clip_block's sample out of the mix x (CLIP 0: x, not in it) */
{
    int32_t a = song.g[G_CLIP], g = a > 0 ? 4096 + a * 12288 / 100 : 0, mk;
    if (!g)
        return x;
    mk = (int32_t)((6000u << 15) / (uint32_t)softclip((6000 * g) >> 12));
    return ((softclip(((x + 8) >> 4) * g >> 10) * mk + 16384) >> 15) << 2;
}
#define UXS_KMS 800u             /* "after a kick": the window, ms */
#define UXS_KW 136               /* .. its columns */
/* (fx.c duck_block, block by block from a kick: its attack halving to the top, ~25 ms held, back at DUCK TIME's rate) */
typedef struct { int32_t env; uint8_t att, hold; } uxs_dk_t;
static void uxs_duck_hit(uxs_dk_t *d) { d->env = 0; d->att = 1; d->hold = (uint8_t)(1103u / CTL); }
static int32_t uxs_duck_step(uxs_dk_t *d, int32_t dep, uint32_t kb)   /* one block: the others' gain after it, Q15 */
{
    int32_t e;
    if (d->att) {
        d->env += (32767 - d->env) >> 1;
        if (d->env > 31000)
            d->att = 0;
    } else if (d->hold)
        d->hold--;
    else if (d->env)
        d->env = (int32_t)(((uint32_t)d->env * kb) >> 16);
    e = (d->env * (65536 - d->env)) >> 15;
    return 32767 - mulq15(dep, e > 32767 ? 32767 : e);
}
#define UXS_KY 64                /* .. its top row, its height (+6 dB at the top, -30 at the foot) */
#define UXS_KH 56
static int32_t uxs_gain_y(int32_t d10) { return clamp((60 - d10) * UXS_KH / 360, 0, UXS_KH - 1); }   /* a gain's row in it */
/* the kick's two curves (a row a column), worked out when PUNCH, DUCK or DUCK TIME change (not each meter frame) */
static struct { uint32_t key; uint8_t duck[UXS_KW], punch[UXS_KW]; } uxs_kick;
static void uxs_kick_calc(void)
{
    uint32_t key = (uint32_t)song.g[G_PUNCH] * 65537u ^ (uint32_t)song.g[G_DUCK] * 131u ^ (uint32_t)song.g[G_DREL] * 7919u ^ 1u, blk = 0,
             kb = dv_kb(40000u + (uint32_t)clamp(song.g[G_DREL], 0, 100) * 5600u), nblk = uxs_ms_smp(UXS_KMS) / CTL, x;
    int32_t dep = clamp(song.g[G_DUCK], 0, 100) * 31731 / 100, pu = clamp(song.g[G_PUNCH], 0, 100), g = 32767;
    uxs_dk_t dk;
    if (key == uxs_kick.key)
        return;
    uxs_kick.key = key;
    uxs_duck_hit(&dk);
    for (x = 0; x < (uint32_t)UXS_KW; x++) {
        uint32_t to = (x + 1u) * nblk / (uint32_t)UXS_KW;
        for (; blk < to; blk++)
            g = uxs_duck_step(&dk, dep, kb);
        uxs_kick.duck[x] = (uint8_t)uxs_gain_y(uxs_db10((uint32_t)(g > 1 ? g : 1), 32767u));
        uxs_kick.punch[x] = (uint8_t)uxs_gain_y(uxs_db10((uint32_t)punch_gain(to, pu), 32768u));
    }
}
static const char *const UXS_MS_SAY[] = {"", "CLIPS", "CLIP", "AFTER A KICK", "DRUMS", "OTHERS"};
static void uxs_master_draw(const track_t *t, const char *name, int32_t lvl)
{
    int32_t lim = uxs_meter_px(uxs_db10((uint32_t)softclip(LIM_T), 32767u)), i, x, y, py = 0;
    char b[16], c[16];
    uxs_pic_head(t, name);
    {   /* OUT: the meter, the clip point */
        cv_rect(UXS_MX0, UXS_MY, UXS_MW, UXS_MH, UX_EMPTY);
        if (lvl > 0)
            cv_rect(UXS_MX0, UXS_MY, lvl < lim ? lvl : lim, UXS_MH, UX_ACID);
        if (lvl > lim)
            cv_rect(UXS_MX0 + lim, UXS_MY, lvl - lim, UXS_MH, UX_ACCENT);
        for (i = 1; i < UXS_MDB / 12; i++)              /* (-36 -24 -12) */
            cv_rect(UXS_MX0 + i * UXS_MW * 12 / UXS_MDB, UXS_MY + UXS_MH, 1, 3, UX_LINE);
        cv_rect(UXS_MX0 + lim, UXS_MY - 3, 2, UXS_MH + 6, UX_TEXT);
        x = UXS_MX0 + lim - text_w(&UF_LBL, UXS_MS_SAY[1]) / 2;
        GFX_HOOK_ALIGN(0, UXS_MY + UXS_MH + 4 + UF_LBL_CAP_Y, 0, UXS_MY + UXS_MH + 4 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS,
                       "MASTER's clip point");
        cv_text_on(x, UXS_MY + UXS_MH + 4, &UF_LBL, UXS_MS_SAY[1], UX_MID, ux_under(x, UXS_MY + UXS_MH + 8));
        uxs_out_txt(lvl, b, sizeof b);
        x = 228 - text_w(&UF_LBL, b);
        GFX_HOOK_ALIGN(0, 8 + UF_LBL_CAP_Y, 0, 8 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "MASTER's level");
        cv_text_on(x, 8, &UF_LBL, b, UX_TEXT, ux_under(x, 12));
    }
    {   /* CLIP: the mix in (0 .. full scale) -> out, its box */
        int32_t bx = 14, by = 58, s = 60;
        cv_rect(bx, by + s, s, 1, UX_LINE);
        cv_rect(bx, by, 1, s, UX_LINE);
        for (i = 0; i < s; i++)                         /* (without CLIP) */
            cv_rect(bx + i, by + s - 1 - i, 1, 1, ux_mix(UX_BG, UX_LINE, 80));
        for (i = 0; i < s; i++) {
            int32_t o = uxs_clip_out((i + 1) * 32768 / s);
            y = by + s - 1 - clamp(o * s / 32768, 0, s - 1);
            if (i)
                cv_line_t(bx + i - 1, py, bx + i, y, song.g[G_CLIP] ? UX_ACID : ux_mix(UX_BG, UX_ACID, 40), 2);
            py = y;
        }
        GFX_HOOK_ALIGN(0, by + s + 4 + UF_LBL_CAP_Y, 0, by + s + 4 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "MASTER's CLIP");
        cv_text_on(bx, by + s + 4, &UF_LBL, UXS_MS_SAY[2], UX_MID, ux_under(bx, by + s + 8));
    }
    {   /* after a kick: the drums' gain (PUNCH) up, the others' (DUCK) down, +6 .. -30 dB */
        int32_t gx = 90, gy = UXS_KY, gh = UXS_KH, y0 = gy + gh * 6 / 36, pp = 0, dp = 0, du = song.g[G_DUCK] > 0, pu = song.g[G_PUNCH] > 0;
        uxs_kick_calc();
        cv_rect(gx, y0, UXS_KW, 1, UX_LINE);            /* (0 dB) */
        cv_rect(gx, gy, 1, gh, UX_LINE);
        for (x = 0; x < UXS_KW; x++) {
            int32_t yd = gy + uxs_kick.duck[x], yp = gy + uxs_kick.punch[x];
            if (x) {
                cv_line_t(gx + x - 1, pp, gx + x, yp, pu ? UX_SLIDE : ux_mix(UX_BG, UX_SLIDE, 40), 2);
                cv_line_t(gx + x - 1, dp, gx + x, yd, du ? UX_VIOLET : ux_mix(UX_BG, UX_VIOLET, 40), 2);
            }
            pp = yp;
            dp = yd;
        }
        GFX_HOOK_ALIGN(0, gy + gh + 4 + UF_LBL_CAP_Y, 0, gy + gh + 4 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "MASTER's kick");
        cv_text_on(gx, gy + gh + 4, &UF_LBL, UXS_MS_SAY[3], UX_MID, ux_under(gx, gy + gh + 8));
        uxs_mstxt(c, sizeof c, UXS_KMS);
        x = gx + UXS_KW - text_w(&UF_LBL, c);
        cv_text_on(x, gy + gh + 4, &UF_LBL, c, UX_MID, ux_under(x, gy + gh + 8));
        GFX_HOOK_ALIGN(0, gy - 10 + UF_LBL_CAP_Y, 0, gy - 10 + UF_LBL_CAP_Y + UF_LBL_CAP_H, AL_V | AL_PASS, "MASTER's kick");
        cv_text_on(gx + 4, gy - 10, &UF_LBL, UXS_MS_SAY[4], pu ? UX_SLIDE : UX_MID, ux_under(gx + 4, gy - 6));
        x = gx + UXS_KW - text_w(&UF_LBL, UXS_MS_SAY[5]);
        cv_text_on(x, gy - 10, &UF_LBL, UXS_MS_SAY[5], du ? UX_VIOLET : UX_MID, ux_under(x, gy - 6));
    }
}

/* the page's picture: MORE its list; EDIT's its engine's, ENV's, ARP's, SCL's, LFO's, FX's (above). (3b task 8: the fallback,
 * the title over the four values as bars, gone with the last section to come) */
static void uxs_picture(const page_t *pg)
{
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
    if (pg->fam == FAM_LFO) {                           /* (3b task 7) LFO: its wave, DEST's rows, MOD MATRIX's slots */
        if (pg->graph == GR_MOD)
            uxs_mod_draw(TSEL, nav_page_name());
        else if (pg->id[0] == P_LD_PIT)
            uxs_lfo_dest_draw(TSEL, pg, nav_page_name());
        else
            uxs_lfo_draw(TSEL, nav_page_name());
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
    if (pg->fam == FAM_FX) {                            /* (3b task 8) FX: SENDS' bars, SPACE's taps and tail, MASTER's meter */
        if (pg->graph == GR_FX)
            uxs_sends_draw(TSEL, pg, nav_page_name());
        else if (pg->id[0] == G_DTIME)
            uxs_space_draw(TSEL, nav_page_name());
        else
            uxs_master_draw(TSEL, nav_page_name(), uxs_mtr_px);
        return;
    }
    if (pg->fam == FAM_EDIT && !edit_kind()) {          /* (a synth: its wave; the filter's page its response) */
        if (pg->id[0] == P_FTYPE)
            uxs_filter_draw(TSEL, pg, nav_page_name());
        else
            uxs_wave_draw(TSEL, nav_page_name(), trk_engp(TSEL) == &ENG_ANALOG && pg->id[0] == P_E0);   /* (ANALOG's
                                                         * SOUND: its oscillators) */
    }
}

static void ux_section_draw(void)
{
    static uint32_t head_sig, panel_sig, foot_sig;
    const page_t *pg = cur_page();
    uint32_t sec = nav_sec(), sig, all, c, scr = uxs_lfo_sc, win = uxs_rh.win, spw = uxs_sp_win;
    int32_t mtr = uxs_mtr_px;
    int due = 0;
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
    if (pg->fam == FAM_LFO && pg->graph != GR_MORE) {   /* (3b task 7: the settings LFO's pictures are drawn from, the slot
                                                         * picked, the engine (the E1..E8 names). Not the tempo: below) */
        for (c = P_LRATE; c <= P_LD_AMP; c++)
            sig = sig * 31u + (uint16_t)TSEL->p[c];
        for (c = P_M1SRC; c <= P_M4AMT; c++)
            sig = sig * 31u + (uint16_t)TSEL->p[c];
        sig = sig * 31u + mod_ui_slot * 7u + TSEL->eng_req * 8191u;
    }
    if (pg->fam == FAM_LFO && pg->graph != GR_MORE && pg->graph != GR_MOD) {   /* (LFO and DEST: their window is the tempo's
                                                         * (3b task 7 fix: an outside clock moves it 4 times a beat), LFO's wave
                                                         * scrolls: redrawn when either moved, as the load allows (ux_pace_at: no
                                                         * faster than HOME's ridges, frozen while the guard sheds or gov_lite);
                                                         * MENU > ANIM OFF: no scrolling, the window still) */
        uxs_lw_t w;
        uxs_lfo_win(TSEL, &w);
        if (pg->graph == GR_LFO)
            uxs_lfo_poll(TSEL, &w);                     /* (RANDOM's cycles as seen) */
        win = w.inc * 2654435761u ^ w.wb;
        scr = pg->graph == GR_LFO && !(ui_prefs & PREF_ANIM_OFF) ? uxs_lfo_scroll(TSEL, &w) : uxs_lfo_sc;
        due = (scr != uxs_lfo_sc || win != uxs_rh.win) && ux_pace_at(fm1_ms, uxs_lfo_ms, load_tight, ux_since_shed(), gov_lite, sys_load, 0, UX_TRK_MS,
                                              UX_TRK_SLOW_MS).draw;
    }
    if (pg->fam == FAM_FX && pg->graph != GR_MORE) {    /* (3b task 8: what FX's pictures are drawn from: the track's sends and
                                                         * DISTORT's TYPE, the globals (the buses', the master's), the LOAD. Not
                                                         * the tempo, the output: below) */
        static const uint8_t F[] = {P_DIST, P_CHOR, P_DLY, P_REV, P_DTYPE};
        static const uint8_t G[] = {G_DTIME, G_DFDBK, G_DMIX, G_DTYPE, G_DPING, G_DPIT, G_DSPRY, G_RTYPE, G_RSIZE, G_RDAMP, G_RMOD,
                                    G_RPRE, G_CLIP, G_PUNCH, G_DUCK, G_DREL};   /* (fix: only the globals the pictures read: not
                                                         * the DJ FILTER, which recorded automation writes every step) */
        for (c = 0; c < sizeof F; c++)
            sig = sig * 31u + (uint16_t)TSEL->p[F[c]];
        for (c = 0; c < sizeof G; c++)
            sig = sig * 31u + (uint16_t)song.g[G[c]];
        sig = sig * 31u + sys_load * 7u + (uint32_t)LOAD_EFF() * 13u;
        if (pg->id[0] == G_DTIME) {                     /* SPACE: the delay's time at the tempo (an outside clock moves it 4
                                                         * times a beat): redrawn as the load allows, as LFO's window */
            spw = uxs_dly_smp() * 2654435761u ^ beat_samples();
            due = spw != uxs_sp_win && ux_pace_at(fm1_ms, uxs_fx_ms, load_tight, ux_since_shed(), gov_lite, sys_load, 0, UX_TRK_MS,
                                                  UX_TRK_SLOW_MS).draw;
        } else if (pg->id[0] == G_CLIP) {               /* MASTER: the output's peak, falling 20 dB a second, redrawn as it moves
                                                         * at HOME's ridges' pace (frozen while the guard sheds or gov_lite; still
                                                         * with ANIM OFF) */
            uint32_t el = fm1_ms - uxs_fx_ms;
            int32_t fall = (int32_t)((el > 2000u ? 2000u : el) * UXS_MW * 20u / (UXS_MDB * 1000u));
            mtr = uxs_meter_px(uxs_out_db10());
            if (mtr < uxs_mtr_px - fall)
                mtr = uxs_mtr_px - fall;
            due = mtr != uxs_mtr_px && !(ui_prefs & PREF_ANIM_OFF) &&
                  ux_pace_at(fm1_ms, uxs_fx_ms, load_tight, ux_since_shed(), gov_lite, sys_load, 0, UX_TRK_MS, UX_TRK_SLOW_MS).draw;
        }
    }
    if (!all && sig != panel_sig && !due && pg == uxs_heavy_pg && pg->fam == FAM_EDIT && pg->graph != GR_MORE &&
        (edit_kind() == 2u || (!edit_kind() && trk_eng(TSEL) == ENGI_NOISE)) &&
        !ux_pace(fm1_ms, uxs_heavy_ms, load_tight, ux_since_shed(), gov_lite, sys_load, 0).draw)
        sig = panel_sig;                                /* (3b task 10) NOISE's and DRUM-X's pictures render up to 8,192 samples
                                                         * (DUST, a hit): a change redrawn at most every UX_VIS_MS (ux_pace: 200
                                                         * ms under load, held while the guard sheds), the knob row's value at
                                                         * once; the last change drawn when its time comes (panel_sig kept) */
    if (all || sig != panel_sig || due) {
        panel_sig = sig;
        uxs_panels++;
        uxs_heavy_pg = pg;
        uxs_heavy_ms = fm1_ms;
        if (pg->fam == FAM_LFO) {
            uxs_lfo_sc = scr;
            uxs_lfo_ms = fm1_ms;
            uxs_rh.win = win;
        }
        if (pg->fam == FAM_FX) {                        /* (3b task 8: the paced pictures' state, drawn from now) */
            uxs_mtr_px = mtr;
            uxs_sp_win = spw;
            uxs_fx_ms = fm1_ms;
        }
        ux_panel_begin(1);
        uxs_picture(pg);
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
