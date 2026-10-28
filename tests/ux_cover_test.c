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
     * PITCH) in the section too, reached without the layer (spec 0 rule 2); (3c-1 task 4) SEQ's now, 3c-1 spec 2.4 (below) */
    /* (ACID FM 3c-1 task 4) SEQ: what JIANT's SEQ pages reached (as 9869027 left them: PATTERN, TRACK, MUTATE, STRETCH's ROTATE,
     * then the controls that are no setting of their own, params.c's ID_SQ_* views: STRETCH's STRETCH REPEAT NUDGE, PLAYHEADS'
     * four, COMP's and COMP 2's TYPE and values, EVERY, NOTE's FOCUS PITCH MOVE, AUTOMATION's PLAY and counts, AUTO LIST's four).
     * STEPS' and STEP EXTRAS' knobs are a step's fields (tests/ui_test.c test_t32_*, test_t33_*); AUTOMATION's CLEAR an action
     * (main: checked below) */
    {SEC_SEQ, TRK, P_SLEN}, {SEC_SEQ, TRK, P_SDIV}, {SEC_SEQ, TRK, P_SSWING}, {SEC_SEQ, TRK, P_SGATE},
    {SEC_SEQ, TRK, P_POFS}, {SEC_SEQ, TRK, P_TCHAN}, {SEC_SEQ, TRK, P_HVEL}, {SEC_SEQ, TRK, P_HRQ},
    {SEC_SEQ, TRK, P_MSND}, {SEC_SEQ, TRK, P_MPAT}, {SEC_SEQ, TRK, P_MAMT}, {SEC_SEQ, TRK, P_MMODE}, {SEC_SEQ, TRK, P_SOFS},
    {SEC_SEQ, TRK, ID_SQ_STRETCH}, {SEC_SEQ, TRK, ID_SQ_REPEAT}, {SEC_SEQ, TRK, ID_SQ_NUDGE}, {SEC_SEQ, TRK, ID_SQ_HEAD},
    {SEC_SEQ, TRK, ID_SQ_HRATE}, {SEC_SEQ, TRK, ID_SQ_HDIR}, {SEC_SEQ, TRK, ID_SQ_HIVL}, {SEC_SEQ, TRK, ID_SQ_TYPE},
    {SEC_SEQ, TRK, ID_SQ_VAL}, {SEC_SEQ, TRK, ID_SQ_VAL2}, {SEC_SEQ, TRK, ID_SQ_TYPE2}, {SEC_SEQ, TRK, ID_SQ_VALB},
    {SEC_SEQ, TRK, ID_SQ_VAL2B}, {SEC_SEQ, TRK, ID_SQ_EVERY}, {SEC_SEQ, TRK, ID_SQ_FOCUS}, {SEC_SEQ, TRK, ID_SQ_PITCH},
    {SEC_SEQ, TRK, ID_SQ_MOVE}, {SEC_SEQ, TRK, ID_SQ_PLAY}, {SEC_SEQ, TRK, ID_SQ_EVENTS}, {SEC_SEQ, TRK, ID_SQ_LOCKS},
    {SEC_SEQ, TRK, ID_SQ_ROW}, {SEC_SEQ, TRK, ID_SQ_ESTEP}, {SEC_SEQ, TRK, ID_SQ_EPARAM}, {SEC_SEQ, TRK, ID_SQ_EVAL},
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
    /* (ACID FM 3c-1 task 7) GLO: what the inherited GLO pages reached (as 0db7884 left them): MIXER (the selected track's LEVEL PAN
     * REV, the DJ FILTER: SC_TRK, ui_input.c tracks_edit), SENDS (the track's DIST CHORUS DLY REV), GLOBAL (BPM SWING CLOCK TUNE),
     * SYSTEM (TRACKS LOAD ROUTE CPU), LIVE's second page (task 6: the RAILS, GLO > MORE's views) */
    {SEC_GLO, TRK, P_LEVEL}, {SEC_GLO, TRK, P_PAN}, {SEC_GLO, TRK, P_REV}, {SEC_GLO, GLO, G_DJF}, {SEC_GLO, TRK, P_DIST},
    {SEC_GLO, TRK, P_CHOR}, {SEC_GLO, TRK, P_DLY}, {SEC_GLO, GLO, G_BPM}, {SEC_GLO, GLO, G_SWING}, {SEC_GLO, GLO, G_CLOCK},
    {SEC_GLO, GLO, G_TUNE}, {SEC_GLO, GLO, G_TRKS}, {SEC_GLO, GLO, G_LOADP}, {SEC_GLO, GLO, G_ROUTE}, {SEC_GLO, GLO, G_INFO},
    {SEC_GLO, GLO, ID_LV_TGT}, {SEC_GLO, GLO, ID_LV_RATE}, {SEC_GLO, GLO, ID_LV_CUT}, {SEC_GLO, GLO, ID_LV_DENS},
};

/* (ACID FM 3c-2 task 2) SAVE: what the inherited USER and PRESETS pages reached (their knobs: controls, not settings of their own:
 * USER's SLOT LOAD ERASE SAVE, PRESETS' No. ENG FAV LIST) and where each is now: a SAVE > SOUND knob (its column and name) or a SAVE
 * > MORE row (kind, id). The first rows: task 4 adds the inherited SECTIONS, ARRANGE, PROJECTS and TOOLS pages' */
enum { UXC_SV_ROW = 4u };                               /* (knob: 0..3 SOUND's, UXC_SV_ROW a MORE row) */
typedef struct { const char *was; uint8_t knob, kind, id; const char *now; } uxc_sv_t;
static const uxc_sv_t UXC_SAVE[] = {
    {"USER SLOT", 0, 0, 0, "SLOT"}, {"USER LOAD", UXC_SV_ROW, UXR_ACTION, UXM_A_ULOAD, "LOAD SLOT"},
    {"USER ERASE", UXC_SV_ROW, UXR_ACTION, UXM_A_ERASE, "ERASE USER SLOT"}, {"USER SAVE", 3, 0, 0, "SAVE"},
    {"PRESETS PRESET", 1, 0, 0, "PRESET"}, {"PRESETS ENGINE", UXC_SV_ROW, UXR_GLOBAL, ID_SV_ENGINE, "ENGINE"},
    {"PRESETS FAVORITE", UXC_SV_ROW, UXR_GLOBAL, ID_SV_FAV, "FAVORITE"}, {"PRESETS LIST", UXC_SV_ROW, UXR_GLOBAL, ID_SV_LIST, "LIST"},
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
    {SEC_GLO, GLO, G_ROUTE, "SYSTEM's ROUTE (MIDI IN CH1-6 / SEL): inert since ACID FM 0.5 task 1, per-track MIDI routing replaced it; "
                            "read only when a project without its 'MR' block loads (project.c proj_import)"},
};

/* (ACID FM 3c-1 task 7) settings the inherited GLO pages reached that GLO's chain does not hold as a page's id or a MORE row, each
 * with where it is now, checked: on FX (FX's chain reaches it: SENDS' four, the REVERB send) or on a knob of a GLO page drawn by
 * ux_glo.c (MIX's LEVEL and PAN, LIVE's DJ FILTER: no page id; a turn of that knob there writes it) */
enum { UXC_ON_FX, UXC_ON_KNOB };
typedef struct { uint8_t glo, id, on, slot; const char *page, *why; } uxc_else_t;
static const uxc_else_t UXC_GLO_ELSE[] = {
    {TRK, P_LEVEL, UXC_ON_KNOB, 0, "MIX", "GLO > MIX KNOB 1 (ux_glo.c: the inherited MIXER's write, tracks_edit)"},
    {TRK, P_PAN, UXC_ON_KNOB, 1, "MIX", "GLO > MIX KNOB 2 (as MIXER's)"},
    {GLO, G_DJF, UXC_ON_KNOB, 3, "LIVE", "GLO > LIVE KNOB 4 (the DJ FILTER, as MIXER's KNOB 4; 3c-1 spec 3.2 says FX: FX has no "
                                         "DJ FILTER, LIVE's KNOB 4 is where it is)"},
    {TRK, P_REV, UXC_ON_FX, 0, 0, "FX > SENDS REVERB (3c-1 spec 3.2: SENDS not repeated)"},
    {TRK, P_DIST, UXC_ON_FX, 0, 0, "FX > SENDS DISTORT"},
    {TRK, P_CHOR, UXC_ON_FX, 0, 0, "FX > SENDS CHORUS"},
    {TRK, P_DLY, UXC_ON_FX, 0, 0, "FX > SENDS DELAY"},
};
static const uxc_else_t *uxc_glo_else(uint32_t glo, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < NELEM(UXC_GLO_ELSE); i++)
        if (UXC_GLO_ELSE[i].glo == glo && UXC_GLO_ELSE[i].id == id)
            return &UXC_GLO_ELSE[i];
    return 0;
}
static void uxc_reach(uint32_t key, int more, uint8_t got[2][256]);
/* e holds: FX's chain reaches it, or a turn of its GLO page's knob writes it */
static int uxc_else_ok(const uxc_else_t *e)
{
    static uint8_t fx[2][256];
    int16_t *vp, was;
    int ok;
    if (e->on == UXC_ON_FX) {
        uxc_reach(SEC_FX, 1, fx);
        return fx[e->glo][e->id];
    }
    nav_put(SEC_GLO, e->page);
    vp = e->glo ? &song.g[e->id] : &TSEL->p[e->id];
    was = *vp;
    *vp = 0;
    edit_param(e->slot, 3);
    ok = *vp != 0 && str_eq(cur_page()->title, e->page);
    *vp = was;
    return ok;
}

static const uint8_t UXC_SECS[] = {UXC_SYN, UXC_FM6, UXC_DRM, UXC_DIG, SEC_ENV, SEC_LFO, SEC_FX, SEC_ARP, SEC_SCL, SEC_SEQ,
                                   SEC_GLO};
static const char *uxc_key_name(uint32_t key)
{
    return key == UXC_FM6 ? "UXC_FM6" : key == UXC_DRM ? "UXC_DRM" : key == UXC_DIG ? "UXC_DIG" : key == UXC_SYN ? "UXC_SYN" :
           key == SEC_ENV ? "SEC_ENV" : key == SEC_LFO ? "SEC_LFO" : key == SEC_FX ? "SEC_FX" : key == SEC_ARP ? "SEC_ARP" :
           key == SEC_SCL ? "SEC_SCL" : key == SEC_SEQ ? "SEC_SEQ" : key == SEC_GLO ? "SEC_GLO" : "?";
}

/* the settings chain key reaches now, a bit each ([glo][id]): its pages' knobs (MOD's slots, the drum picked's: all), and
 * (more) its MORE lists' settings */
static void uxc_reach(uint32_t key, int more, uint8_t got[2][256])
{
    const ch_t *c = &CHAINS[key];
    uint32_t sec = key < SEC_N ? key : SEC_EDIT, e, i, sl, v, r;
    uint8_t mo = mod_ui_slot, dl = dx_ui_lane, sel0 = song.sel;
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
                    if (pg->graph == GR_MIDI)           /* (3c-1 task 8: GLO > MIDI's IN OUT CHANNEL, the track's) */
                        song.sel = (uint8_t)(v % NTRK);
                    id = page_id(pg, sl);
                    song.sel = sel0;
                    if (id < 256u)
                        got[pg->scope == SC_GLOBAL][id] = 1;
                }
        }
    }
    mod_ui_slot = mo; dx_ui_lane = dl; song.sel = sel0;
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
                                       "MORE DIST TYPE", "MORE DLY TYPE", "MORE SLICE MODE",   /* (fix round 1, M2: MORE's
                                                         * KNOB 2, JIANT's words kept: RECTIFY, ANALOG, DIGITAL, STUTTER) */
                                       "EXTRAS CONDITION",
                                       "MORE RATE", "MORE ORDER", "MORE MODE", "MORE TYPE", "MORE PULSE", "MORE CONDITION",
                                       "MORE SHIFT", "MORE DIRECTION", "MORE RANGE", "MORE FOCUS", "MORE BEND", "MORE LAP",
                                       "MORE INTERVAL", "MORE RATCHET", "MORE EVERY",
                                       "LIVE DJ FILTER", "MORE CUT", "MORE DENSITY", "MORE TRACKS",
                                       "MORE NOTES OUT",
                                       "MORE ENGINE"};   /* (3c-2 task 2: SAVE > MORE's ENGINE, the engine's name whole: ANALOG) */   /* (3c-1 task 8: KEYS+SEQ, the ruling's KEYS + SEQUENCE short;
                                                         * the list spells it in UF_HDR, the status line in full) */   /* (3c-1 task 6: the DJ FILTER's cutoff and
                                                         * its side, "5.4K HP"; the RAILS rows' steps, RATCHET X3, SYNTHS, spelled in
                                                         * UF_HDR on the list) */   /* (3c-1 task 4: SEQ > MORE's step options
                                                         * and heads in words with their unit, "UP 5 DEG", "1ST OF 4", "PING-PONG":
                                                         * the list's row spells them in UF_HDR, KNOB 2 a smaller face) */   /* (ACID FM 3c-1 task 3: the conditions in plain words, 3c-1 spec
                                                         * 2.2: EVERY 2ND, 1ST OF 4 .., none fits UF_VAL's 48 px) */
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
    static const uint8_t KEYS[] = {SEC_EDIT, SEC_ENV, SEC_LFO, SEC_FX, SEC_ARP, SEC_SCL, SEC_GLO};   /* (3c-1 task 7: GLO's MIX
                                                         * SWING TUNE, its MORE's SYSTEM and TEMPO rows) */
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
    {   /* (ACID FM 3c-1 task 2) SEQ > STEPS: STEP n/LEN (every LEN), NOTE (every note, chords), ACC/SLD, GATE (NOTE TIE REST, 2 ..
         * 16 steps); a DRUM track's LANE (each), HIT (each mode), ACC (ux_seq.c uxq_knobs) */
        track_t keep = *TSEL;
        ux_knob_t k[4];
        uint32_t len, st, nt, f, g, l, h;
        const page_t *stp = &PAGES[0];
        for (i = 0; i < NPAGES; i++)                    /* (task 3: uxq_knobs by the page given: STEPS') */
            if (PAGES[i].fam == FAM_SEQ && str_eq(PAGES[i].title, "STEP"))
                stp = &PAGES[i];
        set_engine_of(TSEL, ENGI_ANALOG); TSEL->engine = TSEL->eng_req;
        for (len = 1; len <= NSTEP; len++) {
            TSEL->p[P_SLEN] = (int16_t)len;
            for (st = 0; st < len; st++) {
                ui.cursor = (uint8_t)st;
                uxq_knobs(stp, k);
                uxc_wfit("STEPS STEP", k[0].value);
            }
        }
        TSEL->p[P_SLEN] = 16; ui.cursor = 0;
        for (i = 0; i < NSTEP; i++) step_clear(&TSEL->step[i]);
        for (nt = 0; nt < 128u; nt++)
            for (g = 1; g <= 4u; g++)                   /* (a note, a chord of 2 .. 4) */
                for (f = 0; f < 4u; f++) {
                    TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = (uint8_t)g; TSEL->step[0].note[0] = (uint8_t)nt;
                    TSEL->step[0].flags = (uint8_t)((f & 1u ? SF_ACCENT : 0u) | (f & 2u ? SF_SLIDE : 0u));
                    for (l = 0; l < 4u; l++) {
                        ux_knob_t kk[4];
                        uint32_t c;
                        for (c = 0; c < 4u; c++) { kk[c].label = ""; kk[c].value[0] = 0; kk[c].bar = 0; kk[c].off = 0; }
                        uxq_knobs(stp, kk);
                        uxc_wfit(l == 0u ? "STEPS NOTE" : l == 1u ? "STEPS ACC/SLD" : "STEPS GATE", kk[l == 0u ? 1u : l == 1u ? 2u : 3u].value);
                    }
                }
        for (g = 0; g < 16u; g++) {                     /* GATE: a NOTE g + 1 steps long; a TIE; a REST */
            for (i = 1; i < 16u; i++) TSEL->step[i].time = i <= g ? ST_TIE : ST_REST;
            uxq_knobs(stp, k);
            uxc_wfit("STEPS GATE", k[3].value);
        }
        TSEL->step[0].time = ST_TIE; uxq_knobs(stp, k); uxc_wfit("STEPS GATE", k[3].value);
        TSEL->step[0].time = ST_REST; uxq_knobs(stp, k); uxc_wfit("STEPS GATE", k[3].value); uxc_wfit("STEPS NOTE", k[1].value);
        set_engine_of(TSEL, ENGI_DRUM); TSEL->engine = TSEL->eng_req;
        for (i = 0; i < NSTEP; i++) step_clear(&TSEL->step[i]);
        for (l = 0; l < NLANE; l++)
            for (h = 0; h <= LM_N; h++) {
                ui.lane = (uint8_t)l;
                grid_hit_set(TSEL, 0, l, h);
                grid_acc(TSEL, 0, l, (int)(h & 1u));
                uxq_knobs(stp, k);
                uxc_wfit("STEPS LANE", k[1].value);
                uxc_wfit("STEPS HIT", k[2].value);
                uxc_wfit("STEPS ACC", k[3].value);
            }
        *TSEL = keep;
        ui.lane = 0; ui.cursor = 0;
    }
    {   /* (ACID FM 3c-1 task 3) SEQ > STEP EXTRAS: RATCHET (X1 .. X4), CHANCE (0 .. 100%), CONDITION (each, FULL), MICRO (-32 .. +31, GRID,
         * --); PATTERN: LENGTH, DIVISION, SWING, GATE over their ranges (ux_seq.c uxq_knobs, ux_section.c uxs_knobs) */
        track_t keep = *TSEL;
        ux_knob_t k[4];
        const page_t *ex = 0, *pa = 0;
        int32_t v;
        for (i = 0; i < NPAGES; i++) {
            if (str_eq(PAGES[i].title, "STEP EXTRAS")) ex = &PAGES[i];
            if (PAGES[i].fam == FAM_SEQ && str_eq(PAGES[i].title, "PATTERN")) pa = &PAGES[i];
        }
        set_engine_of(TSEL, ENGI_ANALOG); TSEL->engine = TSEL->eng_req;
        ui.cursor = 0;
        step_clear(&TSEL->step[0]);
        TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 1; TSEL->step[0].note[0] = 48;
        for (v = 0; ex && v <= 100; v++) {
            step_set_ratchet(&TSEL->step[0], 1u + (uint32_t)v % 4u);
            step_set_chance(&TSEL->step[0], (uint32_t)v);
            TSEL->step[0].comp = (uint8_t)(v % 14 < 12 ? CP_TRIG : v % 14 == 12 ? CP_NONE : CP_RAND);   /* (CONDITION: each TRIG,
                                                         * ALWAYS, FULL) */
            TSEL->step[0].carg = (uint8_t)(v % 12);
            TSEL->step[0].comp2 = (uint8_t)(v % 14 == 13 ? CP_ARPN : CP_NONE);
            ST_MICRO_SET(&TSEL->step[0], MICRO_MIN + v % (MICRO_MAX - MICRO_MIN + 1));
            uxs_knobs(ex, k);
            uxc_wfit("EXTRAS RATCHET", k[0].value);
            uxc_wfit("EXTRAS CHANCE", k[1].value);
            uxc_wfit("EXTRAS CONDITION", k[2].value);
            uxc_wfit("EXTRAS MICRO", k[3].value);
        }
        ST_MICRO_SET(&TSEL->step[0], MICRO_MAX);
        uxs_knobs(ex, k); uxc_wfit("EXTRAS MICRO", k[3].value);
        TSEL->step[0].time = ST_REST;
        uxs_knobs(ex, k); uxc_wfit("EXTRAS MICRO", k[3].value);
        for (v = 0; pa && v <= 127; v++) {
            TSEL->p[P_SLEN] = (int16_t)clamp(v, 1, NSTEP);
            TSEL->p[P_SDIV] = (int16_t)(v % 10);
            TSEL->p[P_SSWING] = (int16_t)clamp(v, 0, 100);
            TSEL->p[P_SGATE] = (int16_t)clamp(v, 1, 127);
            uxs_knobs(pa, k);
            for (i = 0; i < 4u; i++) uxc_wfit("PATTERN", k[i].value);
        }
        if (!ex || !pa) {
            printf("  STEP EXTRAS / PATTERN: no page\n");
            uxc_wbad++;
        }
        *TSEL = keep;
        ui.cursor = 0;
    }
    {   /* (ACID FM 3c-1 task 4) SEQ > MORE's views (ux_seq.c uxq_vtext) as KNOB 2 shows them: every TYPE of both components with
         * every value (ECHO's shapes and pitch steps, DIV), EVERY, STRETCH REPEAT, the heads, FOCUS / PITCH, the list's; their names
         * as KNOB 2's label */
        track_t keep = *TSEL;
        uint32_t c, a, r, id;
        set_engine_of(TSEL, ENGI_ANALOG); TSEL->engine = TSEL->eng_req;
        ui.cursor = 0;
        step_clear(&TSEL->step[0]);
        TSEL->step[0].time = ST_NOTE; TSEL->step[0].n = 3; TSEL->step[0].note[0] = 0; TSEL->step[0].note[1] = 127;
        for (c = 0; c < COMP_N; c++)
            for (a = 0; a < 256u; a += (c == COMP_RATCH || c == COMP_ARPN || c == COMP_ARPQ || c == COMP_WALK || c == COMP_OCT) ? 1u : 3u) {
                TSEL->step[0].comp = TSEL->step[0].comp2 = COMP_CP[c];
                TSEL->step[0].carg = TSEL->step[0].carg2 = (uint8_t)a;
                TSEL->step[0].ctime = (uint8_t)(a & 7u);
                ST_EVERY_SET(&TSEL->step[0], a % EV_N);
                ui.comp_at = 0; ui.comp_sel = (uint8_t)c;
                for (id = ID_SQ_TYPE; id < ID_SQ_END; id = id == ID_SQ_EVERY ? ID_SQ_RNG : id + 1u) {
                    char v[24], w[48];
                    uint8_t bar;
                    const char *nm = uxq_vtext(id, v, sizeof v, &bar);
                    str_cpy(w, "MORE ", sizeof w);
                    str_cpy(w + str_len(w), nm, sizeof w - str_len(w));
                    if (nm[0]) uxc_wfit(w, v);
                    if (nm[0] && text_w(&UF_LBL, nm) > UX_COL_W - 4) { printf("  %s: its name too wide\n", w); uxc_wbad++; }
                    uxc_wmore++;
                }
            }
        for (r = 0; r < 15u * 9u; r++) {               /* STRETCH, REPEAT; the heads; FOCUS on each note; the counts and list */
            char v[24];
            uint8_t bar;
            shp_i = (uint8_t)(r % 15u); shp_r = (uint8_t)(r % 9u);
            fg_ui = (uint8_t)(r % FG_HEADS);
            fg_set[song.sel % NTRK][fg_ui].rate = (uint8_t)(r % FG_NRATE);
            fg_set[song.sel % NTRK][fg_ui].dir = (uint8_t)(r & 3u);
            fg_set[song.sel % NTRK][fg_ui].ivl = (int8_t)((int32_t)(r % 29u) - 14);
            ui.pr_focus = (uint8_t)(r % 4u);
            for (id = ID_SQ_STRETCH; id < ID_SQ_END; id++) {
                char w[48];
                const char *nm;
                if ((id >= ID_SQ_TYPE && id <= ID_SQ_EVERY) || id >= ID_SQ_RNG) continue;
                nm = uxq_vtext(id, v, sizeof v, &bar);
                str_cpy(w, "MORE ", sizeof w);
                str_cpy(w + str_len(w), nm, sizeof w - str_len(w));
                uxc_wfit(w, v);
                if (text_w(&UF_LBL, nm) > UX_COL_W - 4) { printf("  %s: its name too wide\n", w); uxc_wbad++; }
            }
        }
        *TSEL = keep;
        ui.pr_focus = 0; shp_i = 7u; shp_r = 0u; fg_ui = 0; ui.comp_at = 0;
        memset(&fg_set[song.sel % NTRK], 0, sizeof fg_set[0]);
    }
    {   /* (ACID FM 3c-1 task 6) GLO > LIVE's knobs (ux_glo.c uxg_knobs: RATE LOOP THIN of every value, the DJ FILTER's every
         * value) and GLO > MORE's RAILS rows as KNOB 2 shows them (TRACKS RATE CUT DENSITY, uxg_vtext) */
        static const uint8_t RAILS[4] = {ID_LV_TGT, ID_LV_RATE, ID_LV_CUT, ID_LV_DENS};
        int16_t djf = song.g[G_DJF];
        ux_knob_t k[4];
        uint8_t keep[sizeof lv];
        memcpy(keep, &lv, sizeof lv);
        memset(&lv, 0, sizeof lv);
        nav_put(SEC_GLO, "LIVE");
        for (x = -9; x <= 12; x++) {
            uint32_t c;
            for (c = 0; c < NTRK; c++) {
                lv.rate[c] = (int8_t)clamp(x, -4, 4); lv.cut[c] = (uint8_t)((x + 9) % LV_NCUT); lv.dn[c] = (int8_t)clamp(x, -9, 3);
            }
            lv.tgt = (uint8_t)((x + 9) % 9);
            for (song.g[G_DJF] = -64; song.g[G_DJF] <= 63; song.g[G_DJF] = (int16_t)(song.g[G_DJF] + (x == 0 ? 1 : 64))) {
                uxg_knobs(k);
                for (c = 0; c < 4u; c++) {
                    char w[32];
                    str_cpy(w, "LIVE ", sizeof w);
                    str_cpy(w + str_len(w), k[c].label, sizeof w - str_len(w));
                    uxc_wfit(w, k[c].value);
                    if (text_w(&UF_LBL, k[c].label) > UX_COL_W - 4) { printf("  %s: its name too wide\n", w); uxc_wbad++; }
                }
            }
            for (c = 0; c < 4u; c++) {
                char v[24], w[32];
                uint8_t bar;
                const char *nm = uxg_vtext(RAILS[c], v, sizeof v, &bar);
                str_cpy(w, "MORE ", sizeof w);
                str_cpy(w + str_len(w), nm, sizeof w - str_len(w));
                uxc_wfit(w, v);
                uxc_wmore++;
            }
        }
        memcpy(&lv, keep, sizeof lv);
        song.g[G_DJF] = djf;
    }
    {   /* (ACID FM 3c-1 task 7) GLO > MIX's LEVEL and PAN (ux_glo.c uxg_mix_knobs: every value of the selected track's) */
        int16_t l0 = TSEL->p[P_LEVEL], p0 = TSEL->p[P_PAN];
        ux_knob_t k[4];
        nav_put(SEC_GLO, "MIX");
        for (x = -64; x <= 127; x++) {
            uint32_t c;
            TSEL->p[P_LEVEL] = (int16_t)clamp(x, 0, 127);
            TSEL->p[P_PAN] = (int16_t)clamp(x, -64, 63);
            uxs_knobs(cur_page(), k);
            for (c = 0; c < 2u; c++) {
                char w[32];
                str_cpy(w, "MIX ", sizeof w);
                str_cpy(w + str_len(w), k[c].label, sizeof w - str_len(w));
                uxc_wfit(w, k[c].value);
                if (text_w(&UF_LBL, k[c].label) > UX_COL_W - 4) { printf("  %s: its name too wide\n", w); uxc_wbad++; }
            }
        }
        TSEL->p[P_LEVEL] = l0; TSEL->p[P_PAN] = p0;
    }
    {   /* (3c-1 task 8) GLO > MORE's MIDI views each value (param_ref reads them from the device: set, not *vp) and their names, GLO
         * > MIDI's labels */
        static const uint8_t MV[] = {ID_MO_NOTES, ID_MO_CLOCK, ID_ML_LEARN};
        uint32_t n0 = mout_notes(), c0 = mout_clock(), r, k;
        ux_row_t rw = {0, UXR_GLOBAL, 0};
        ux_knob_t kn[4];
        for (r = 0; r < NELEM(MV); r++)
            for (k = 0; k < 3u; k++) {
                char v[24], w[32];
                uint8_t bar;
                const char *nm;
                rw.id = MV[r];
                if (MV[r] == ID_MO_NOTES) mout_put(k, 0); else if (MV[r] == ID_MO_CLOCK) mout_put(MOUT_KEYS, k & 1u); else ml.on = (uint8_t)(k & 1u);
                nm = uxm_row_text(&rw, v, sizeof v, &bar);
                str_cpy(w, "MORE ", sizeof w);
                str_cpy(w + str_len(w), nm, sizeof w - str_len(w));
                uxc_wfit(w, v);
                uxc_wmore++;
                if (text_w(&UF_LBL, nm) > UX_COL_W - 4) { printf("  %s: its name too wide\n", w); uxc_wbad++; }
            }
        ml.on = 0;
        mout_put(n0, c0);
        rw.id = G_MRSEL;
        {
            char v[24];
            uint8_t bar;
            const char *nm = uxm_row_text(&rw, v, sizeof v, &bar);
            if (text_w(&UF_LBL, nm) > UX_COL_W - 4) { printf("  MORE %s: its name too wide\n", nm); uxc_wbad++; }
        }
        rw.kind = UXR_ACTION; rw.id = UXM_A_LCLEAR;
        {
            char v[24];
            uint8_t bar;
            const char *nm = uxm_row_text(&rw, v, sizeof v, &bar);
            if (text_w(&UF_LBL, nm) > UX_COL_W - 4) { printf("  MORE %s: its name too wide\n", nm); uxc_wbad++; }
        }
        nav_put(SEC_GLO, "MIDI");
        uxs_knobs(cur_page(), kn);
        for (k = 0; k < 4u; k++)
            if (text_w(&UF_LBL, kn[k].label) > UX_COL_W - 4) { printf("  MIDI %s: its name too wide\n", kn[k].label); uxc_wbad++; }
    }
    {   /* (ACID FM 3c-2 task 2) SAVE > SOUND's knobs (ux_save.c uxv_knobs: SLOT every slot, PRESET every place in every engine's
         * list, NAME, SAVE armed or not) and SAVE > MORE's views (param_ref reads them from the state: set, not *vp), their names */
        static const uint8_t SV[] = {ID_SV_ENGINE, ID_SV_FAV, ID_SV_LIST};
        track_t keep = *TSEL;
        uint32_t k0 = ui.uslot, a0 = ui.act, l0 = list_get(), r, k, c;
        ux_row_t rw = {0, UXR_GLOBAL, 0};
        ux_knob_t kn[4];
        nav_put(SEC_SAVE, "SOUND");
        up_store(2, "A LONG NAME!");
        for (k = 0; k < UP_SLOTS; k++)
            for (c = 0; c < 2u; c++) {
                ui.uslot = (uint8_t)k;
                ui.act = (uint8_t)(c ? 4u : 0u);
                uxs_knobs(cur_page(), kn);
                for (r = 0; r < 4u; r++) {
                    char w[32];
                    str_cpy(w, "SOUND ", sizeof w);
                    str_cpy(w + str_len(w), kn[r].label, sizeof w - str_len(w));
                    uxc_wfit(w, kn[r].value);
                    if (text_w(&UF_LBL, kn[r].label) > UX_COL_W - 4) { printf("  %s: its name too wide\n", w); uxc_wbad++; }
                }
            }
        for (e = 0; e < NENGINES; e++) {                /* (PRESET: every sound of every engine, ALL and FAV) */
            uint32_t total, p;
            if (!eng_ok(e))
                continue;
            set_engine_of(TSEL, e);
            TSEL->engine = TSEL->eng_req;
            for (c = 0; c < 2u; c++) {
                list_put(c);
                (void)preset_pos(&total);
                for (p = 0; p < ENGINES[e]->npresets; p++) {
                    TSEL->preset = (uint8_t)p;
                    uxs_knobs(cur_page(), kn);
                    uxc_wfit("SOUND PRESET", kn[1].value);
                }
            }
            for (r = 0; r < NELEM(SV); r++)             /* (MORE: ENGINE each, FAVORITE on and off, LIST each) */
                for (k = 0; k <= PT_N; k++) {
                    char v[24], w[32];
                    uint8_t bar;
                    const char *nm;
                    if ((SV[r] == ID_SV_FAV && k > 1u) || (SV[r] == ID_SV_ENGINE && k))
                        continue;
                    if (SV[r] == ID_SV_FAV) preset_mark((int)k);
                    if (SV[r] == ID_SV_LIST) list_put(k);
                    rw.id = SV[r];
                    nm = uxm_row_text(&rw, v, sizeof v, &bar);
                    str_cpy(w, "MORE ", sizeof w);
                    str_cpy(w + str_len(w), nm, sizeof w - str_len(w));
                    uxc_wfit(w, v);
                    uxc_wmore++;
                    if (text_w(&UF_LBL, nm) > UX_COL_W - 4) { printf("  %s: its name too wide\n", w); uxc_wbad++; }
                }
            preset_mark(0);
        }
        rw.kind = UXR_ACTION;
        for (r = 0; r < 2u; r++) {                      /* (MORE: its actions' names on KNOB 2's column) */
            char v[24];
            uint8_t bar;
            const char *nm;
            rw.id = r ? UXM_A_ERASE : UXM_A_ULOAD;
            nm = uxm_row_text(&rw, v, sizeof v, &bar);
            if (text_w(&UF_LBL, nm) > UX_COL_W - 4) { printf("  MORE %s: its name too wide\n", nm); uxc_wbad++; }
        }
        (void)up_put(2, 0);
        list_put(l0);
        *TSEL = keep;
        ui.uslot = (uint8_t)k0; ui.act = (uint8_t)a0;
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
    uint32_t i, s, n = 0, nh = 0, ne = 0;
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
            if (key == SEC_GLO && uxc_glo_else(j->glo, j->id)) {   /* (3c-1 task 7: on FX, or a GLO page's own knob: checked) */
                const uxc_else_t *e = uxc_glo_else(j->glo, j->id);
                if (uxc_else_ok(e)) {
                    ne++;
                    continue;
                }
                printf("  SEC_GLO: %s %u (%s) said to be at \"%s\", not reached there\n", j->glo ? "global" : "track setting",
                       (unsigned)j->id, uxc_label(j->glo, j->id), e->why);
                ok = 0;
                continue;
            }
            printf("  %s: %s %u (%s) is on no page or MORE row, and not in UXC_HIDDEN\n", uxc_key_name(key),
                   j->glo ? "global" : "track setting", (unsigned)j->id, uxc_label(j->glo, j->id));
            ok = 0;
        }
    }
    bad += check("ux cover: every setting JIANT's six sections (3c-1 task 4: and SEQ; task 7: and GLO) reached is on a page or a MORE "
                 "row, or hidden on purpose", ok && n > 210u && ne == NELEM(UXC_GLO_ELSE));
    ok = 1;                                             /* (3c-1 task 7) the GLO list: each one the inherited GLO pages reached */
    for (i = 0; i < NELEM(UXC_GLO_ELSE); i++)
        ok &= uxc_in_jiant(SEC_GLO, UXC_GLO_ELSE[i].glo, UXC_GLO_ELSE[i].id) && UXC_GLO_ELSE[i].why && str_len(UXC_GLO_ELSE[i].why) > 8u;
    uxc_reach(SEC_GLO, 1, got);                         /* (MIX's SWING TUNE on its page; MORE's rows; the RAILS views) */
    ok &= got[GLO][G_SWING] && got[GLO][G_TUNE] && got[GLO][G_BPM] && got[GLO][G_CLOCK] && got[GLO][G_TRKS] &&
          got[GLO][G_LOADP] && got[GLO][G_INFO] && got[GLO][ID_LV_DENS] && !got[GLO][G_ROUTE];
    uxc_reach(SEC_GLO, 0, got);
    ok &= got[GLO][G_SWING] && got[GLO][G_TUNE] && !got[GLO][G_BPM] && !got[GLO][G_TRKS];
    {   /* (3c-1 task 8) GLO > MIDI: every track's IN OUT CHANNEL (the selected one's) and CLOCK on its page; GLO > MORE's MIDI rows:
         * KEYBOARD PLAYS SELECTED, and what MENU > MIDI held (MIDI OUT as NOTES OUT and CLOCK OUT, LEARN; LEARN CLEAR an action) */
        uint32_t k, r, lc = 0, ck = 0;
        for (k = 0; k < NTRK; k++)
            ok &= got[GLO][G_MRIN1 + k] && got[GLO][G_MROUT1 + k] && got[GLO][G_MRCH1 + k];
        ok &= got[GLO][G_CLOCK] && !got[GLO][G_MRSEL] && !got[GLO][ID_MO_NOTES];
        uxc_reach(SEC_GLO, 1, got);
        ok &= got[GLO][G_MRSEL] && got[GLO][ID_MO_NOTES] && got[GLO][ID_MO_CLOCK] && got[GLO][ID_ML_LEARN];
        for (r = 0; r < UXM_GLO.n; r++)
            lc += UXM_GLO.rows[r].kind == UXR_ACTION && UXM_GLO.rows[r].id == UXM_A_LCLEAR,
            ck += UXM_GLO.rows[r].kind == UXR_GLOBAL && UXM_GLO.rows[r].id == G_CLOCK;
        ok &= lc == 1u && ck == 0u;                     /* (CLOCK: the MIDI page's, no MORE row) */
    }
    bad += check("ux cover: GLO (3c-1 task 7): MIX's SWING TUNE, MORE's SYSTEM and TEMPO; LEVEL PAN DJ FILTER on its knobs, the sends on "
                 "FX, ROUTE hidden; (task 8) MIDI's IN OUT CHANNEL of every track and CLOCK, MORE's MIDI rows (MENU's MIDI OUT, LEARN, "
                 "LEARN CLEAR)", ok);
    ok = 1;                                             /* the hidden list: reviewed rows only */
    for (i = 0; i < NELEM(UXC_HIDDEN); i++) {
        const uxc_hid_t *h = &UXC_HIDDEN[i];
        int real = h->glo ? h->id < G_COUNT : h->id < P_COUNT || fld_is(h->id) || sqv_is(h->id);   /* (a stored setting: no view, kept as it
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
    {   /* (3b task 6) the SCL layer's four (ui_layer.c LY_SCL): ROOT SCALE in SCL's chain; (3c-1 task 4) ROTATE and SEQ PITCH in
         * SEQ's MORE alone (3c-1 spec 2.4: one place a setting); ENV's eight on its two pages, ARP's eight on its two */
        uint32_t k;
        uxc_reach(SEC_SCL, 1, got);
        for (k = 0; k < 2u; k++)
            ok &= got[LY_SCL.scope == SC_GLOBAL][LY_SCL.id[k]];
        ok &= got[TRK][P_TRANS] && !got[TRK][P_SOFS] && !got[TRK][P_POFS];
        uxc_reach(SEC_SEQ, 1, got);
        for (k = 2; k < 4u; k++)
            ok &= got[LY_SCL.scope == SC_GLOBAL][LY_SCL.id[k]];
        ok &= got[TRK][ID_SQ_TYPE2] && got[TRK][ID_SQ_EVAL] && got[TRK][P_HRQ];
        uxc_reach(SEC_SEQ, 0, got);
        ok &= got[TRK][P_SLEN] && !got[TRK][P_SOFS] && !got[TRK][ID_SQ_STRETCH];
        {                                               /* AUTOMATION's CLEAR: SEQ's MORE, its dialog JIANT's */
            uint32_t r;
            for (r = 0; r < UXM_SEQ.n && !(UXM_SEQ.rows[r].kind == UXR_ACTION && UXM_SEQ.rows[r].id == UXM_A_CLRAUTO); r++)
                ;
            ok &= r < UXM_SEQ.n && UXM_ACT[UXM_A_CLRAUTO].ask == CF_CLEAR_MOTION;
        }
        uxc_reach(SEC_ENV, 0, got);
        ok &= got[TRK][P_ATK] && got[TRK][P_ELOOP] && got[TRK][P_ED_SHP];
        uxc_reach(SEC_ARP, 0, got);
        ok &= got[TRK][P_AMODE] && got[TRK][P_AORDER];
    }
    bad += check("ux cover: the reach counts the pages' knobs (every step and slot) and the MORE rows, and sees a gap", ok);
    {   /* (ACID FM 3c-2 task 2) SAVE: each control the inherited USER and PRESETS pages had is a SOUND knob or a SAVE > MORE row */
        ux_knob_t kn[4];
        uint64_t m;
        ok = 1;
        nav_put(SEC_SAVE, "SOUND");
        uxs_knobs(cur_page(), kn);
        m = uxm_mask(&UXM_SAVE);
        for (i = 0; i < NELEM(UXC_SAVE); i++) {
            const uxc_sv_t *v = &UXC_SAVE[i];
            int got = 0;
            if (v->knob < UXC_SV_ROW)
                got = str_eq(cur_page()->title, "SAVE SOUND") && str_eq(kn[v->knob].label, v->now);
            else
                for (s = 0; s < UXM_SAVE.n; s++)
                    if (UXM_SAVE.rows[s].kind == v->kind && UXM_SAVE.rows[s].id == v->id && uxm_pick_ok(&UXM_SAVE, m, s)) {
                        char val[24];
                        uint8_t bar;
                        got = str_eq(uxm_list_name(&UXM_SAVE.rows[s], uxm_row_text(&UXM_SAVE.rows[s], val, sizeof val, &bar)),
                                     v->now);
                    }
            if (!got)
                printf("  SAVE: the inherited %s is not at %s\n", v->was, v->now);
            ok &= got;
        }
        bad += check("ux cover: SAVE (3c-2 task 2): every control USER and PRESETS had is a SOUND knob or a SAVE > MORE row", ok &&
                     NELEM(UXC_SAVE) == 8u);
    }
    bad += uxc_widths();
    printf("ux cover: %u settings JIANT reached, %u hidden on purpose, %u GLO's elsewhere (checked)\n", (unsigned)n, (unsigned)nh,
           (unsigned)ne);
    if (bad)
        printf("ux cover: %d FAILED\n", bad);
    return bad ? 1 : 0;
}
