/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM 3b (spec 6): the coverage test. Every setting JIANT's pages of EDIT, ENV, LFO, FX, ARP and SCL reached (UXC_JIANT,
 * a reviewed list taken from the tables as JIANT left them, a5ae3f7) is still reached in its chain: on a page of that chain
 * (a PAGES row its CHAINS entries name) or a row of one of its MORE lists, or it is in UXC_HIDDEN, the reviewed list of the
 * settings left off on purpose, each with its reason. A hidden setting keeps its stored value (projects, presets, the web
 * editor and MIDI still read and write it): only the pages stop showing it. Tasks 4..9 simplify the sections: a row they
 * drop from every page and MORE list goes into UXC_HIDDEN with why, or this test fails.
 *   cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/ux_cover_test tests/ux_cover_test.c -lm   (-O1: as run_tests.sh)
 *   build/host/ux_cover_test          the test
 *   build/host/ux_cover_test --dump   the reach of today's tables, as UXC_JIANT's rows (MORE lists left out) */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

/* a chain: ui_lens.c CHAINS' index (nav_key): EDIT's synth chain is SEC_EDIT's, FM6 DRUM DIGITAL after the sections */
enum { UXC_SYN = SEC_EDIT, UXC_FM6 = SEC_N, UXC_DRM, UXC_DIG };
typedef struct { uint8_t key, glo, id; } uxc_id_t;      /* a setting reached in chain key: a global (song.g) or a track's */
#define TRK 0
#define GLO 1

/* the settings JIANT's pages reached, chain by chain (tests/ux_cover_test.c --dump at a5ae3f7 / 06ccb6f: every PAGES row its
 * chain names, every knob, MOD's four slots, MSEQ's and SL PITCH's sixteen steps; 3b task 7: MSEQ's steps are LFO's MORE rows,
 * reached there; task 8: SL PITCH's and the other thirteen FX pages' settings FX's MORE rows). Reviewed: the special pages (DRUM's
 * SOUND 1..4, X-MOD, KIT 3, FM6's OPERATOR / OP ENV / OPERATOR 2, FOCUS, STAGE's stages) turn kit and patch fields or other
 * pages' settings, not ids of their own: Task 5 covers the kit and patch fields; the held layers are Task 9's, but for SCL's
 * (3b task 6: its knobs the section's own, spec 2.5) */
static const uxc_id_t UXC_JIANT[] = {
    {UXC_SYN, TRK, P_DIST}, {UXC_SYN, TRK, P_VOICE}, {UXC_SYN, TRK, P_GLIDE}, {UXC_SYN, TRK, P_GLMODE},
    {UXC_SYN, TRK, P_PRIO}, {UXC_SYN, TRK, P_ALLOC}, {UXC_SYN, TRK, P_DETUNE}, {UXC_SYN, TRK, P_LRTRG},
    {UXC_SYN, TRK, P_MORPH}, {UXC_SYN, TRK, P_DRIFT}, {UXC_SYN, TRK, P_VLYR}, {UXC_SYN, TRK, P_WDUST},
    {UXC_SYN, TRK, P_WRDEP}, {UXC_SYN, TRK, P_FTYPE}, {UXC_SYN, TRK, P_FCUT}, {UXC_SYN, TRK, P_FRES},
    {UXC_SYN, TRK, P_E0}, {UXC_SYN, TRK, P_E1}, {UXC_SYN, TRK, P_E2}, {UXC_SYN, TRK, P_E3}, {UXC_SYN, TRK, P_E4},
    {UXC_SYN, TRK, P_E5}, {UXC_SYN, TRK, P_E6}, {UXC_SYN, TRK, P_E7},
    {UXC_FM6, TRK, P_DIST}, {UXC_FM6, TRK, P_VOICE}, {UXC_FM6, TRK, P_GLIDE}, {UXC_FM6, TRK, P_GLMODE},
    {UXC_FM6, TRK, P_PRIO}, {UXC_FM6, TRK, P_ALLOC}, {UXC_FM6, TRK, P_DETUNE}, {UXC_FM6, TRK, P_FTYPE},
    {UXC_FM6, TRK, P_FCUT}, {UXC_FM6, TRK, P_FRES}, {UXC_FM6, TRK, P_E0}, {UXC_FM6, TRK, P_E1}, {UXC_FM6, TRK, P_E2},
    {UXC_FM6, TRK, P_E3}, {UXC_FM6, TRK, P_E4}, {UXC_FM6, TRK, P_E5}, {UXC_FM6, TRK, P_E6}, {UXC_FM6, TRK, P_E7},
    {UXC_DRM, TRK, P_DIST}, {UXC_DRM, TRK, P_VOICE}, {UXC_DRM, TRK, P_GLIDE}, {UXC_DRM, TRK, P_GLMODE},
    {UXC_DRM, TRK, P_PRIO}, {UXC_DRM, TRK, P_DXIMG}, {UXC_DRM, TRK, P_LN0}, {UXC_DRM, TRK, P_LN1}, {UXC_DRM, TRK, P_LN2},
    {UXC_DRM, TRK, P_LN3}, {UXC_DRM, TRK, P_LN4}, {UXC_DRM, TRK, P_LN5}, {UXC_DRM, TRK, P_LN6}, {UXC_DRM, TRK, P_LN7},
    {UXC_DRM, TRK, P_FTYPE}, {UXC_DRM, TRK, P_FCUT}, {UXC_DRM, TRK, P_FRES}, {UXC_DRM, TRK, P_E0}, {UXC_DRM, TRK, P_E1},
    {UXC_DRM, TRK, P_E2}, {UXC_DRM, TRK, P_E3}, {UXC_DRM, TRK, P_E4}, {UXC_DRM, TRK, P_E5}, {UXC_DRM, TRK, P_E6},
    {UXC_DRM, TRK, P_E7},
    {UXC_DIG, TRK, P_DIST}, {UXC_DIG, TRK, P_VOICE}, {UXC_DIG, TRK, P_GLIDE}, {UXC_DIG, TRK, P_GLMODE},
    {UXC_DIG, TRK, P_PRIO}, {UXC_DIG, TRK, P_ALLOC}, {UXC_DIG, TRK, P_DETUNE}, {UXC_DIG, TRK, P_FM1_ATK},
    {UXC_DIG, TRK, P_FM1_DEC}, {UXC_DIG, TRK, P_FM1_SUS}, {UXC_DIG, TRK, P_FM1_REL}, {UXC_DIG, TRK, P_FM1_LEVEL},
    {UXC_DIG, TRK, P_FM2_ATK}, {UXC_DIG, TRK, P_FM2_DEC}, {UXC_DIG, TRK, P_FM2_SUS}, {UXC_DIG, TRK, P_FM2_REL},
    {UXC_DIG, TRK, P_FM2_LEVEL}, {UXC_DIG, TRK, P_FM3_ATK}, {UXC_DIG, TRK, P_FM3_DEC}, {UXC_DIG, TRK, P_FM3_SUS},
    {UXC_DIG, TRK, P_FM3_REL}, {UXC_DIG, TRK, P_FM3_LEVEL}, {UXC_DIG, TRK, P_FM4_ATK}, {UXC_DIG, TRK, P_FM4_DEC},
    {UXC_DIG, TRK, P_FM4_SUS}, {UXC_DIG, TRK, P_FM4_REL}, {UXC_DIG, TRK, P_FM4_LEVEL}, {UXC_DIG, TRK, P_FTYPE},
    {UXC_DIG, TRK, P_FCUT}, {UXC_DIG, TRK, P_FRES}, {UXC_DIG, TRK, P_E0}, {UXC_DIG, TRK, P_E1}, {UXC_DIG, TRK, P_E2},
    {UXC_DIG, TRK, P_E3}, {UXC_DIG, TRK, P_E4}, {UXC_DIG, TRK, P_E5}, {UXC_DIG, TRK, P_E6}, {UXC_DIG, TRK, P_E7},
    {SEC_ENV, TRK, P_ATK}, {SEC_ENV, TRK, P_DEC}, {SEC_ENV, TRK, P_SUS}, {SEC_ENV, TRK, P_REL}, {SEC_ENV, TRK, P_ED_FLT},
    {SEC_ENV, TRK, P_ED_PIT}, {SEC_ENV, TRK, P_ED_SHP}, {SEC_ENV, TRK, P_ELOOP},
    {SEC_LFO, TRK, P_LRATE}, {SEC_LFO, TRK, P_LWAVE}, {SEC_LFO, TRK, P_LFADE}, {SEC_LFO, TRK, P_LD_PIT},
    {SEC_LFO, TRK, P_LD_FLT}, {SEC_LFO, TRK, P_LD_SHP}, {SEC_LFO, TRK, P_LD_AMP}, {SEC_LFO, TRK, P_M1SRC},
    {SEC_LFO, TRK, P_M1DST}, {SEC_LFO, TRK, P_M1AMT}, {SEC_LFO, TRK, P_M2SRC}, {SEC_LFO, TRK, P_M2DST},
    {SEC_LFO, TRK, P_M2AMT}, {SEC_LFO, TRK, P_M3SRC}, {SEC_LFO, TRK, P_M3DST}, {SEC_LFO, TRK, P_M3AMT},
    {SEC_LFO, TRK, P_M4SRC}, {SEC_LFO, TRK, P_M4DST}, {SEC_LFO, TRK, P_M4AMT}, {SEC_LFO, TRK, P_MSLEN},
    {SEC_LFO, TRK, P_MSDIV}, {SEC_LFO, TRK, P_MSSLW}, {SEC_LFO, TRK, P_MS0}, {SEC_LFO, TRK, P_MS0 + 1},
    {SEC_LFO, TRK, P_MS0 + 2}, {SEC_LFO, TRK, P_MS0 + 3}, {SEC_LFO, TRK, P_MS0 + 4}, {SEC_LFO, TRK, P_MS0 + 5},
    {SEC_LFO, TRK, P_MS0 + 6}, {SEC_LFO, TRK, P_MS0 + 7}, {SEC_LFO, TRK, P_MS0 + 8}, {SEC_LFO, TRK, P_MS0 + 9},
    {SEC_LFO, TRK, P_MS0 + 10}, {SEC_LFO, TRK, P_MS0 + 11}, {SEC_LFO, TRK, P_MS0 + 12}, {SEC_LFO, TRK, P_MS0 + 13},
    {SEC_LFO, TRK, P_MS0 + 14}, {SEC_LFO, TRK, P_MS0 + 15}, {SEC_LFO, TRK, ID_LPHS}, {SEC_LFO, TRK, ID_LMODE},
    {SEC_FX, TRK, P_DIST}, {SEC_FX, TRK, P_CHOR}, {SEC_FX, TRK, P_DLY}, {SEC_FX, TRK, P_REV}, {SEC_FX, TRK, P_APDEP},
    {SEC_FX, TRK, P_APRATE}, {SEC_FX, TRK, P_APWAVE}, {SEC_FX, TRK, P_APSYNC}, {SEC_FX, TRK, P_DTYPE},
    {SEC_FX, TRK, P_DTONE}, {SEC_FX, GLO, G_DTIME}, {SEC_FX, GLO, G_DFDBK}, {SEC_FX, GLO, G_DCOLOR}, {SEC_FX, GLO, G_DMIX},
    {SEC_FX, GLO, G_RSIZE}, {SEC_FX, GLO, G_RDAMP}, {SEC_FX, GLO, G_CRATE}, {SEC_FX, GLO, G_CDEPTH}, {SEC_FX, GLO, G_DREL},
    {SEC_FX, GLO, G_CLIP}, {SEC_FX, GLO, G_RTYPE}, {SEC_FX, GLO, G_PUNCH}, {SEC_FX, GLO, G_DUCK},
    {SEC_FX, GLO, G_CSTER}, {SEC_FX, GLO, G_WIDTH}, {SEC_FX, GLO, G_RMOD}, {SEC_FX, GLO, G_RRATE},
    {SEC_FX, GLO, G_RPRE}, {SEC_FX, GLO, G_RFILT}, {SEC_FX, GLO, G_RWIDE}, {SEC_FX, GLO, G_DPIT}, {SEC_FX, GLO, G_DSPRY},
    {SEC_FX, GLO, G_SLMODE}, {SEC_FX, GLO, G_SLPAT}, {SEC_FX, GLO, G_SLRATE}, {SEC_FX, GLO, G_SLDEP}, {SEC_FX, GLO, G_SLT1},
    {SEC_FX, GLO, G_SLT1 + 1}, {SEC_FX, GLO, G_SLT1 + 2}, {SEC_FX, GLO, G_SLT1 + 3}, {SEC_FX, GLO, G_SLT1 + 4},
    {SEC_FX, GLO, G_SLT6}, {SEC_FX, GLO, G_SLFX}, {SEC_FX, GLO, G_SLPLEN}, {SEC_FX, GLO, G_SLPQ}, {SEC_FX, GLO, G_SLP0},
    {SEC_FX, GLO, G_SLP0 + 1}, {SEC_FX, GLO, G_SLP0 + 2}, {SEC_FX, GLO, G_SLP0 + 3}, {SEC_FX, GLO, G_SLP0 + 4},
    {SEC_FX, GLO, G_SLP0 + 5}, {SEC_FX, GLO, G_SLP0 + 6}, {SEC_FX, GLO, G_SLP0 + 7}, {SEC_FX, GLO, G_SLP0 + 8},
    {SEC_FX, GLO, G_SLP0 + 9}, {SEC_FX, GLO, G_SLP0 + 10}, {SEC_FX, GLO, G_SLP0 + 11}, {SEC_FX, GLO, G_SLP0 + 12},
    {SEC_FX, GLO, G_SLP0 + 13}, {SEC_FX, GLO, G_SLP0 + 14}, {SEC_FX, GLO, G_SLP0 + 15}, {SEC_FX, GLO, G_DMOD},
    {SEC_FX, GLO, G_DTYPE}, {SEC_FX, GLO, G_DPING}, {SEC_FX, GLO, G_PFXLEN}, {SEC_FX, GLO, G_PFXRES},
    {SEC_ARP, TRK, P_AMODE}, {SEC_ARP, TRK, P_ARATE}, {SEC_ARP, TRK, P_AOCT}, {SEC_ARP, TRK, P_AGATE},
    {SEC_ARP, TRK, P_ASWING}, {SEC_ARP, TRK, P_APROB}, {SEC_ARP, TRK, P_AHOLD}, {SEC_ARP, TRK, P_AORDER},
    {SEC_SCL, TRK, P_ROOT}, {SEC_SCL, TRK, P_SCALE}, {SEC_SCL, TRK, P_QUANT}, {SEC_SCL, TRK, P_CHRD},
    {SEC_SCL, TRK, P_CVOI}, {SEC_SCL, TRK, P_CSTR}, {SEC_SCL, TRK, ID_ALLTR},
    /* (3b task 6) the SCL layer's (ui_layer.c LY_SCL: ROOT SCALE ROTATE PITCH): spec 2.5 puts ROTATE and PITCH OFFSET (SEQ
     * PITCH) in the section too, reached without the layer (spec 0 rule 2) */
    {SEC_SCL, TRK, P_SOFS}, {SEC_SCL, TRK, P_POFS},
    /* (3b task 5) the patch's and the kit's fields JIANT's special pages turned, reviewed from ui_fm6op.c and ui_dx.c as of
     * d96b47f (params.c's views name them): FM6's OPERATOR (OP, RATIO, FINE, LEVEL), OP ENV (OP, STAGE, RATE, LEVEL),
     * OPERATOR 2 (OP, MODE, DETUNE, VEL); DRUM-X's SOUND (LANE, SIDE, PITCH, PMOD), SOUND 2 (LANE, DECAY, NOISE, COLOR), SOUND 3
     * (LANE, MODE: its wave, noise filter and SNAP, PM.TYPE, DRIVE), SOUND 4 (LANE, LAYER, FEEDBK, SEND: its bus and amount),
     * KIT 3 (BP, FEEDBK, HANDS), X-MOD (RATE, DEPTH, SHAPE, RAND) */
    {UXC_FM6, TRK, ID_FO_OP}, {UXC_FM6, TRK, ID_FO_CRS}, {UXC_FM6, TRK, ID_FO_FINE}, {UXC_FM6, TRK, ID_FO_LVL},
    {UXC_FM6, TRK, ID_FO_STG}, {UXC_FM6, TRK, ID_FO_RATE}, {UXC_FM6, TRK, ID_FO_ELVL}, {UXC_FM6, TRK, ID_FO_MODE},
    {UXC_FM6, TRK, ID_FO_DTUN}, {UXC_FM6, TRK, ID_FO_VEL},
    {UXC_DRM, TRK, ID_DX_LANE}, {UXC_DRM, TRK, ID_DX_SIDE}, {UXC_DRM, TRK, ID_DX_PITCH}, {UXC_DRM, TRK, ID_DX_PMOD},
    {UXC_DRM, TRK, ID_DX_DECAY}, {UXC_DRM, TRK, ID_DX_NOISE}, {UXC_DRM, TRK, ID_DX_COLOR}, {UXC_DRM, TRK, ID_DX_WAVE},
    {UXC_DRM, TRK, ID_DX_NTYPE}, {UXC_DRM, TRK, ID_DX_SNAP}, {UXC_DRM, TRK, ID_DX_PMT}, {UXC_DRM, TRK, ID_DX_DRV},
    {UXC_DRM, TRK, ID_DX_CLANG}, {UXC_DRM, TRK, ID_DX_LFB}, {UXC_DRM, TRK, ID_DX_SENDTO}, {UXC_DRM, TRK, ID_DX_SEND},
    {UXC_DRM, TRK, ID_DX_BP}, {UXC_DRM, TRK, ID_DX_KFB}, {UXC_DRM, TRK, ID_DX_HANDS}, {UXC_DRM, TRK, ID_DX_XRATE},
    {UXC_DRM, TRK, ID_DX_XDEP}, {UXC_DRM, TRK, ID_DX_XSHP}, {UXC_DRM, TRK, ID_DX_XRND},
};

/* the settings left off every page and MORE list on purpose, each with why. They keep their stored value */
typedef struct { uint8_t key, glo, id; const char *why; } uxc_hid_t;
static const uxc_hid_t UXC_HIDDEN[] = {
    {UXC_SYN, TRK, P_WRDEP, "ROTOR's DEPTH: WHEEL retired in ACID FM 0.2 (ui.c page_visible never shows ROTOR)"},
    {UXC_SYN, TRK, P_WDUST, "ROTOR's DUST: as DEPTH"},
    {UXC_SYN, TRK, P_ALLOC, "VOICE 2's ALLOC (ROT / REUSE, how a new note picks a voice): an expert's choice, its default suits "
                            "every sound (3b task 4, the user's 'simplify')"},
    {UXC_FM6, TRK, P_ALLOC, "ALLOC on FM6: as on the synths (3b task 5)"},
    {UXC_DRM, TRK, P_VOICE, "VOICE (POLY / MONO ..) on a kit: no effect, DRUM-X plays its drums as hits (voice.c trk_vmode)"},
    {UXC_DRM, TRK, P_GLIDE, "GLIDE on a kit: no effect, a hit never glides (voice.c trk_note_on: oneshot)"},
    {UXC_DRM, TRK, P_GLMODE, "GLIDE MODE on a kit: no effect, as GLIDE"},
    {UXC_DRM, TRK, P_PRIO, "PRIORITY on a kit: no effect, it picks the note of a MONO voice (voice.c mono_stack)"},
    {UXC_DRM, TRK, P_DIST, "the FILTER page's DRV on a kit: the same setting as FX > SENDS DISTORT (P_DIST); the kit has its own "
                           "DRIVE (KIT 2), three drives were one too many"},
};

static const uint8_t UXC_SECS[] = {UXC_SYN, UXC_FM6, UXC_DRM, UXC_DIG, SEC_ENV, SEC_LFO, SEC_FX, SEC_ARP, SEC_SCL};
static const char *uxc_key_name(uint32_t key)
{
    return key == UXC_FM6 ? "UXC_FM6" : key == UXC_DRM ? "UXC_DRM" : key == UXC_DIG ? "UXC_DIG" : key == UXC_SYN ? "UXC_SYN" :
           key == SEC_ENV ? "SEC_ENV" : key == SEC_LFO ? "SEC_LFO" : key == SEC_FX ? "SEC_FX" : key == SEC_ARP ? "SEC_ARP" :
           key == SEC_SCL ? "SEC_SCL" : "?";
}

/* the settings chain key reaches now, a bit each ([glo][id]): its pages' knobs (MOD's slots, the drum picked's: all), and
 * (more) its MORE lists' settings */
static void uxc_reach(uint32_t key, int more, uint8_t got[2][256])
{
    const ch_t *c = &CHAINS[key];
    uint32_t sec = key < SEC_N ? key : SEC_EDIT, e, i, sl, v, r;
    uint8_t mo = mod_ui_slot, dl = dx_ui_lane;
    memset(got, 0, 2 * 256);
    for (e = 0; e < c->n; e++) {
        if (!c->e[e].t)
            continue;
        for (i = 0; i < NPAGES; i++) {
            const page_t *pg = &PAGES[i];
            if (pg->fam != SEC_FAM[sec] || !str_eq(pg->title, c->e[e].t))
                continue;
            if (pg->graph == GR_MORE) {
                if (more && pg->more)
                    for (r = 0; r < pg->more->n; r++) {
                        const ux_row_t *rw = &pg->more->rows[r];
                        if ((rw->kind == UXR_TRACK || rw->kind == UXR_GLOBAL) && rw->id < 256u)
                            got[rw->kind == UXR_GLOBAL][rw->id] = 1;
                    }
                continue;
            }
            for (sl = 0; sl < 4u; sl++)
                for (v = 0; v < 16u; v++) {             /* (the slot / the drum shown: every one) */
                    uint32_t id;
                    mod_ui_slot = (uint8_t)(v & 3u);
                    dx_ui_lane = (uint8_t)(v & 7u);     /* (3b task 5: DRUM-X's SOUND, the drum picked's LEVEL) */
                    id = page_id(pg, sl);
                    if (id < 256u)
                        got[pg->scope == SC_GLOBAL][id] = 1;
                }
        }
    }
    mod_ui_slot = mo; dx_ui_lane = dl;
}

static const uxc_hid_t *uxc_hidden(uint32_t key, uint32_t glo, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < NELEM(UXC_HIDDEN); i++)
        if (UXC_HIDDEN[i].key == key && UXC_HIDDEN[i].glo == glo && UXC_HIDDEN[i].id == id)
            return &UXC_HIDDEN[i];
    return 0;
}
static int uxc_in_jiant(uint32_t key, uint32_t glo, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < NELEM(UXC_JIANT); i++)
        if (UXC_JIANT[i].key == key && UXC_JIANT[i].glo == glo && UXC_JIANT[i].id == id)
            return 1;
    return 0;
}
static const char *uxc_label(uint32_t glo, uint32_t id)
{
    int16_t *vp;
    const param_desc_t *d = param_ref(glo ? 0 : TSEL, id, &vp);
    return d && d->label ? d->label : "?";
}

static void uxc_dump(void)
{
    static uint8_t got[2][256];
    uint32_t s, g, id;
    for (s = 0; s < NELEM(UXC_SECS); s++) {
        uxc_reach(UXC_SECS[s], 0, got);
        for (g = 0; g < 2u; g++)
            for (id = 0; id < 256u; id++)
                if (got[g][id])
                    printf("    {%s, %s, %u},   /* %s */\n", uxc_key_name(UXC_SECS[s]), g ? "GLO" : "TRK", (unsigned)id,
                           uxc_label(g, id));
    }
}

/* (3b task 10; spec 5's backlog) every value a 3b knob can show fits its knob column in UF_VAL (the knob row's big face):
 * each section page's four knobs (EDIT on every engine one can pick, ENV LFO FX ARP SCL; MOD's four slots and DRUM-X's eight
 * drums too), each id's whole range through the knob row's form (ux_section.c uxs_vtext), HOME's (LEVEL, BPM, MACRO) and the
 * held layers' own (FX's glitchers and macros, MACRO's; EDIT's and SCL's are their sections' pages). UXC_WIDE: the knobs
 * whose names were kept whole on purpose (3b task 7 fix: a MOD source or destination reads as the knob it moves, "CUTOFF";
 * task 8: REVERB TYPE's SHIMMER): those fall back to UF_HDR, then UF_LBL (ux_widgets.c ux_val_face: NOTE RAND, CHO DEPTH)
 * and must fit their column there. Any other value wider than its column in UF_VAL fails, printed once a knob */
/* (fix round 1, M2) MORE pages too: KNOB 1's "n/m" (every count of rows one can pick, each engine) and KNOB 2's value (each
 * row's whole range through uxm_row_text, "GO" for an action), as ux_section.c's knob row draws them */
static const char *const UXC_WIDE[] = {"MOD SRC", "MOD DEST", "FX SPACE TYPE",   /* (page title, then the label's start) */
                                       "MORE DIST TYPE", "MORE DLY TYPE", "MORE SLICE MODE"};   /* (fix round 1, M2: MORE's
                                                         * KNOB 2, JIANT's words kept: RECTIFY, ANALOG, DIGITAL, STUTTER) */
static uint32_t uxc_wn, uxc_wbad, uxc_whdr, uxc_wmore;
static void uxc_wfit(const char *where, const char *v)
{
    static uint32_t seen[256];
    uint32_t h = str_hash(str_hash(7u, where), v), i, n = 0;
    uxc_wn++;
    if (str_len(v) + 1u < sizeof ((ux_knob_t *)0)->value) {
        if (text_w(&UF_VAL, v) <= UX_COL_W - 4)
            return;
        for (i = 0; i < NELEM(UXC_WIDE); i++)
            if (!strncmp(where, UXC_WIDE[i], str_len(UXC_WIDE[i])) && text_w(&UF_LBL, v) <= UX_COL_W - 4) {
                uxc_whdr++;
                return;
            }
    }
    for (i = 0; i < uxc_wbad && i < NELEM(seen); i++)  /* (each wide value once a knob: MOD's four slots, the engines) */
        n |= seen[i] == h;
    if (uxc_wbad < NELEM(seen))
        seen[uxc_wbad] = h;
    uxc_wbad++;
    if (!n)
        printf("  %s: \"%s\" is %d px in UF_VAL, its column %d\n", where, v, (int)text_w(&UF_VAL, v), UX_COL_W - 4);
}
static int uxc_widths(void)
{
    static const uint8_t KEYS[] = {SEC_EDIT, SEC_ENV, SEC_LFO, SEC_FX, SEC_ARP, SEC_SCL};
    uint8_t mo = mod_ui_slot, dl = dx_ui_lane, eng0 = TSEL->engine, req0 = TSEL->eng_req;
    uint32_t e, s, i, sl, v;
    int32_t x;
    char b[32];
    uxc_wn = uxc_wbad = uxc_whdr = uxc_wmore = 0;
    for (e = 0; e < NENGINES; e++) {
        if (!eng_ok(e))
            continue;
        set_engine_of(TSEL, e);
        TSEL->engine = TSEL->eng_req;
        for (s = 0; s < NELEM(KEYS); s++)
            for (i = 0; i < NPAGES; i++) {
                const page_t *pg = &PAGES[i];
                if (pg->fam != SEC_FAM[KEYS[s]] || pg->graph == GR_MORE || (KEYS[s] != SEC_EDIT && e))
                    continue;                           /* (the other sections the same on every engine: once) */
                for (sl = 0; sl < 4u; sl++)
                    for (v = 0; v < 8u; v++) {          /* (MOD's slot, DRUM-X's drum: every one) */
                        int16_t *vp;
                        const param_desc_t *d;
                        mod_ui_slot = (uint8_t)(v & 3u);
                        dx_ui_lane = (uint8_t)v;
                        d = page_desc(pg, sl, &vp);
                        if (!d || !vp || !d->label || d->label[0] == '-')
                            continue;
                        for (x = d->min; x <= d->max; x++) {
                            char w[48];
                            uxs_vtext(d, x, pg->scope == SC_GLOBAL, page_id(pg, sl), b, sizeof ((ux_knob_t *)0)->value);
                            str_cpy(w, pg->title, sizeof w);
                            str_cpy(w + str_len(w), " ", sizeof w - str_len(w));
                            str_cpy(w + str_len(w), d->label, sizeof w - str_len(w));
                            uxc_wfit(w, b);
                        }
                    }
            }
    }
    for (e = 0; e < NENGINES; e++) {                   /* (fix round 1, M2) MORE pages: KNOB 1 "n/m", KNOB 2 the row's value */
        if (!eng_ok(e))                                 /* (ux_section.c uxs_knobs: uxm_row_text, "GO" for an action) */
            continue;
        set_engine_of(TSEL, e);
        TSEL->engine = TSEL->eng_req;
        for (s = 0; s < NELEM(KEYS); s++)
            for (i = 0; i < NPAGES; i++) {
                const page_t *pg = &PAGES[i];
                uint64_t m;
                uint32_t r, n, a, cnt;
                if (pg->fam != SEC_FAM[KEYS[s]] || pg->graph != GR_MORE || !pg->more || (KEYS[s] != SEC_EDIT && e))
                    continue;
                m = uxm_mask(pg->more);
                n = uxm_n(pg->more);
                for (r = cnt = 0; r < n; r++)
                    cnt += uxm_pick_ok(pg->more, m, r) != 0;
                for (a = 1; a <= cnt; a++) {
                    fmt_int(b, (int32_t)a);
                    str_cpy(b + str_len(b), "/", sizeof b - str_len(b));
                    fmt_int(b + str_len(b), (int32_t)cnt);
                    uxc_wfit("MORE PICK", b);
                    uxc_wmore++;
                }
                for (r = 0; r < n; r++) {
                    const ux_row_t *rw = &pg->more->rows[r];
                    char v[24], w[48];
                    uint8_t bar;
                    const char *nm;
                    if (!uxm_pick_ok(pg->more, m, r))
                        continue;
                    if (rw->kind == UXR_ACTION) {
                        uxc_wfit("MORE GO", "GO");
                        uxc_wmore++;
                        continue;
                    }
                    for (v[0] = 0, sl = 0; sl < 8u; sl++) {   /* (MOD's slot, DRUM-X's drum: every one) */
                        int16_t *vp, keep;
                        const param_desc_t *d;
                        mod_ui_slot = (uint8_t)(sl & 3u);
                        dx_ui_lane = (uint8_t)sl;
                        d = param_ref(rw->kind == UXR_GLOBAL ? 0 : TSEL, rw->id, &vp);
                        if (!d || !vp || (rw->kind != UXR_TRACK && rw->kind != UXR_GLOBAL))
                            continue;
                        keep = *vp;
                        for (x = d->min; x <= d->max; x++) {
                            *vp = (int16_t)x;
                            nm = uxm_row_text(rw, v, sizeof v, &bar);
                            str_cpy(w, "MORE ", sizeof w);
                            str_cpy(w + str_len(w), nm ? nm : "?", sizeof w - str_len(w));
                            uxc_wfit(w, v);
                            uxc_wmore++;
                        }
                        *vp = keep;
                    }
                }
            }
    }
    TSEL->eng_req = req0; TSEL->engine = eng0;
    mod_ui_slot = mo; dx_ui_lane = dl;
    for (x = TP[P_LEVEL].min; x <= TP[P_LEVEL].max; x++) {   /* HOME: TRACKS' LEVELs (the number), its BPM; VISUAL's MACROs */
        const char *u;
        param_format(&TP[P_LEVEL], x, b, &u);
        uxc_wfit("HOME LEVEL", b);
    }
    for (x = GP[G_BPM].min; x <= GP[G_BPM].max; x++) {
        fmt_int(b, x);
        uxc_wfit("HOME BPM", b);
    }
    for (x = 0; x <= 127; x++) {                        /* (and the MACRO layer's: the same) */
        fmt_int(b, x * 100 / 127);
        str_cpy(b + str_len(b), "%", sizeof b - str_len(b));
        uxc_wfit("MACRO", b);
    }
    for (i = 0; i < 4u; i++)                            /* the FX layer: a glitcher's four, the macros */
        for (sl = 0; sl < 4u; sl++) {
            param_desc_t d = gx_desc(i, sl);
            if (!d.label[0] || !d.max)
                continue;
            for (x = d.min; x <= d.max; x++) {
                uxs_val(&d, x, b, sizeof ((ux_knob_t *)0)->value);
                uxc_wfit(UXL_GX[i * 4u + sl], b);
            }
        }
    for (i = 0; i < NELEM(UXL_LOOP); i++) uxc_wfit("FX LOOP", UXL_LOOP[i]);
    for (i = 0; i < NELEM(UXL_IVL); i++) uxc_wfit("FX INTERVAL", UXL_IVL[i]);
    printf("ux widths: %u values a 3b knob can show (%u on MORE pages' knob rows), %u names kept whole in a smaller face "
           "(UXC_WIDE), %u too wide\n", (unsigned)uxc_wn, (unsigned)uxc_wmore, (unsigned)uxc_whdr, (unsigned)uxc_wbad);
    i = uxc_wbad;                                       /* (the check sees one: a value as wide as MOD's, on a knob not listed) */
    uxc_wfit("(self-check, not a knob)", "CHO DEPTH");
    s = uxc_wbad - i;
    uxc_wbad = i;
    return check("ux widths: every value a 3b knob can show (each id's range) fits its knob column in UF_VAL",
                 uxc_wn > 5000u && uxc_wmore > 1000u && !uxc_wbad && s == 1u);
}

int main(int argc, char **argv)
{
    static uint8_t got[2][256];
    uint32_t i, s, n = 0, nh = 0;
    int bad = 0, ok = 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    ui_power_on();
    if (argc > 1 && str_eq(argv[1], "--dump")) {
        uxc_dump();
        return 0;
    }
    for (s = 0; s < NELEM(UXC_SECS); s++) {             /* every setting JIANT reached: reached still, or hidden on purpose */
        uint32_t key = UXC_SECS[s];
        uxc_reach(key, 1, got);
        for (i = 0; i < NELEM(UXC_JIANT); i++) {
            const uxc_id_t *j = &UXC_JIANT[i];
            if (j->key != key)
                continue;
            n++;
            if (got[j->glo][j->id])
                continue;
            if (uxc_hidden(j->key, j->glo, j->id)) {
                nh++;
                continue;
            }
            printf("  %s: %s %u (%s) is on no page or MORE row, and not in UXC_HIDDEN\n", uxc_key_name(key),
                   j->glo ? "global" : "track setting", (unsigned)j->id, uxc_label(j->glo, j->id));
            ok = 0;
        }
    }
    bad += check("ux cover: every setting JIANT's six sections reached is on a page or a MORE row, or hidden on purpose",
                 ok && n > 150u);
    ok = 1;                                             /* the hidden list: reviewed rows only */
    for (i = 0; i < NELEM(UXC_HIDDEN); i++) {
        const uxc_hid_t *h = &UXC_HIDDEN[i];
        int real = h->glo ? h->id < G_COUNT : h->id < P_COUNT || fld_is(h->id);   /* (a stored setting: no view, kept as it
                                                         * is; 3b task 5: a patch's or kit's byte, stored with them) */
        if (!uxc_in_jiant(h->key, h->glo, h->id) || !real || !h->why || str_len(h->why) < 8u ||
            uxc_hidden(h->key, h->glo, h->id) != h) {
            printf("  UXC_HIDDEN[%u]: %s %u: not one JIANT reached there, not a stored setting, no reason, or twice\n",
                   (unsigned)i, uxc_key_name(h->key), (unsigned)h->id);
            ok = 0;
        }
    }
    bad += check("ux cover: each hidden-on-purpose setting is one JIANT reached there, a stored setting, with its reason", ok);
    ok = 1;                                             /* the check sees what is missing: SCL never reached the filter */
    uxc_reach(SEC_SCL, 1, got);
    ok &= !got[TRK][P_FCUT] && got[TRK][P_ROOT] && !uxc_hidden(SEC_SCL, TRK, P_FCUT);
    uxc_reach(UXC_SYN, 1, got);                         /* (.. and a MORE row counts: a synth's MORPH, on EDIT's MORE only) */
    ok &= got[TRK][P_MORPH] && got[TRK][P_VLYR] && got[TRK][P_E0];
    uxc_reach(UXC_SYN, 0, got);
    ok &= !got[TRK][P_MORPH] && got[TRK][P_FCUT];
    uxc_reach(SEC_LFO, 0, got);                         /* (MOD MATRIX's four slots on its page; 3b task 7: the STEPS' sixteen
                                                         * and PHASE MORE's rows, on no page) */
    ok &= got[TRK][P_M1SRC] && got[TRK][P_M4AMT] && got[TRK][P_LRATE] && got[TRK][ID_LMODE] && got[TRK][P_LD_AMP] &&
          !got[TRK][P_MS0] && !got[TRK][P_MSDIV] && !got[TRK][ID_LPHS];
    uxc_reach(SEC_LFO, 1, got);
    ok &= got[TRK][P_MS0] && got[TRK][P_MS15] && got[TRK][P_MSLEN] && got[TRK][P_MSDIV] && got[TRK][P_MSSLW] && got[TRK][ID_LPHS];
    uxc_reach(SEC_FX, 0, got);                          /* (3b task 8: SENDS SPACE MASTER on pages; JIANT's other thirteen FX
                                                         * pages' settings, the SLICER's steps too, in MORE only) */
    ok &= got[TRK][P_DIST] && got[TRK][P_REV] && got[GLO][G_DTIME] && got[GLO][G_RSIZE] && got[GLO][G_CLIP] && got[GLO][G_DREL] &&
          !got[GLO][G_SLP0] && !got[GLO][G_SLMODE] && !got[TRK][P_DTYPE] && !got[GLO][G_DMIX] && !got[GLO][G_PFXLEN];
    uxc_reach(SEC_FX, 1, got);
    ok &= got[GLO][G_SLP0] && got[GLO][G_SLP15] && got[GLO][G_SLT6] && got[TRK][P_APWAVE] && got[GLO][G_DPING] &&
          got[GLO][G_RWIDE] && got[GLO][G_CSTER] && got[GLO][G_PFXRES] && !got[GLO][G_DJF];
    uxc_reach(UXC_DRM, 0, got);                         /* (3b task 5: SOUND's LEVEL, every drum's; the kit's fields) */
    ok &= got[TRK][P_LN0] && got[TRK][P_LN7] && got[TRK][ID_DX_LANE] && !got[TRK][ID_DX_XRND];
    uxc_reach(UXC_DRM, 1, got);
    ok &= got[TRK][ID_DX_XRND] && got[TRK][ID_DX_SEND] && got[TRK][ID_DX_LFB] && !got[TRK][P_VOICE];   /* (task 5 fix: a
                                                         * drum's FEEDBACK shown, gen.c sets the snare's) */
    uxc_reach(UXC_FM6, 1, got);
    ok &= got[TRK][ID_FO_DTUN] && got[TRK][ID_FO_ELVL] && got[TRK][P_E7] && !got[TRK][P_ALLOC];
    {   /* (3b task 6) the SCL layer's four (ui_layer.c LY_SCL) all in SCL's chain: ROTATE on a page, PITCH (SEQ PITCH) in MORE;
         * ENV's eight on its two pages (STAGE gone, none in a MORE), ARP's eight on its two */
        uint32_t k;
        uxc_reach(SEC_SCL, 1, got);
        for (k = 0; k < 4u; k++)
            ok &= got[LY_SCL.scope == SC_GLOBAL][LY_SCL.id[k]];
        ok &= got[TRK][P_TRANS];
        uxc_reach(SEC_SCL, 0, got);
        ok &= got[TRK][P_SOFS] && !got[TRK][P_POFS];
        uxc_reach(SEC_ENV, 0, got);
        ok &= got[TRK][P_ATK] && got[TRK][P_ELOOP] && got[TRK][P_ED_SHP];
        uxc_reach(SEC_ARP, 0, got);
        ok &= got[TRK][P_AMODE] && got[TRK][P_AORDER];
    }
    bad += check("ux cover: the reach counts the pages' knobs (every step and slot) and the MORE rows, and sees a gap", ok);
    bad += uxc_widths();
    printf("ux cover: %u settings JIANT reached, %u hidden on purpose\n", (unsigned)n, (unsigned)nh);
    if (bad)
        printf("ux cover: %d FAILED\n", bad);
    return bad ? 1 : 0;
}
