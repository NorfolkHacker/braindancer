/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 NorfolkHacker
 * (JIANT 0.6.10: from Felucca 1.5.1, without its undo: JIANT has none since TONIC) */
/* FM6's operators (Discussion #76): the six operators of the track's own patch (eng_fm6.c fm6_patch) edited on the device.
 * (ACID FM 3b task 5) On the ux pages (ux_section.c; params.c's views ID_FO_*): EDIT > OPERATOR (OP RATIO FINE LEVEL), OP ENV
 * (OP STAGE RATE LEVEL), MORE's operator rows (FREQ MODE, OP DETUNE, VELOCITY); KNOB 1 the operator on both pages (one pick,
 * fop_op, kept while one goes round them and in MORE), OP ENV's stage the RATE and LEVEL it edits (fop_stg):
 *   RATIO   the coarse frequency: 0.50, 1..31 (FIXED: 1 10 100 1000 HZ, shown as FREQ)   FINE 0..99   LEVEL 0..99
 *   STAGE   1 ATK, 2 DEC, 3 SUS, 4 REL: the envelope's stage RATE / LEVEL edit             RATE 0..99   LEVEL 0..99
 *   FREQ MODE  RATIO / FIXED      OP DETUNE  -7..+7      VELOCITY  the velocity sensitivity, 0..7
 * A detent writes one byte of the patch (fm6_set_patch: the audio ISR takes it at its next block) and makes the patch the
 * track's own: SLOT OWN (as a patch from the editor; a factory F n edited is OWN from then on). (JIANT: no undo; the
 * output trim, eng_fm6.c fm6_trim, is kept: an edit is not a new patch.) The patch is saved with projects and user presets
 * as before (no new format, no new parameter ids: nothing here is automatable or lockable). The values shown are the
 * patch's own: EDIT 1's macros still apply on top of it as before (MOD LEVEL moves the modulators' LEVEL, MOD RATIO their
 * RATIO, MOD TIME their RATEs, MOD VEL their VELOCITY, DETUNE the carriers' pitch, FEEDBACK the feedback, ALGORITHM
 * another algorithm: the picture and which operators are carriers follow ALGORITHM). Included by ui_graph.c. */
static int32_t accel(uint32_t role, int32_t s, int32_t range);   /* (ui_input.c) */
static uint8_t fop_op;                               /* the operator shown: 0..5 = OP 1..6 (the patch holds OP 6 first) */
static uint8_t fop_stg;                              /* OP ENV: the stage KNOB 3 / 4 edit, 0..3 */

/* per view (params.c ID_FO_CRS ..): its byte in an operator's 21 (RATE / LEVEL: + the stage) */
static const uint8_t FO_OFF[ID_FO_END - ID_FO_OP] = {0, FP_FC, FP_FF, FP_OL, 0, FP_R1, FP_L1, FP_MODE, FP_DET, FP_KVS};

static int fop_ok(void) { return trk_eng(TSEL) == ENGI_FM6; }
/* the selected operator's 21 bytes in track tr's patch */
static const uint8_t *fop_bytes(uint32_t tr) { return fm6_patch[tr % NTRK] + (5u - fop_op % 6u) * FP_OP; }
/* view id's byte in the 155-byte voice (the selected operator, the stage) */
static uint32_t fop_byte(uint32_t id)
{
    return (5u - fop_op % 6u) * FP_OP + FO_OFF[(id - ID_FO_OP) % (ID_FO_END - ID_FO_OP)] +
           (id == ID_FO_RATE || id == ID_FO_ELVL ? fop_stg & 3u : 0u);
}

/* byte b of the selected track's patch = v: the track's own patch now (SLOT OWN) */
static void fop_write(uint32_t b, uint32_t v)
{
    uint32_t tr = song.sel % NTRK;
    track_t *t = TSEL;
    uint8_t p[FP_SIZE + 1u];
    if (fm6_patch[tr][b] == v)
        return;
    memcpy(p, fm6_patch[tr], FP_SIZE);
    p[b] = (uint8_t)v;
    fm6_trim_keep |= (uint8_t)(1u << tr);            /* (JIANT: an edit keeps the patch's trim) */
    fm6_set_patch(tr, p);
    fm6_own_ok &= (uint8_t)~(1u << tr);              /* (an own patch kept aside for SLOT: this one replaces it) */
    fm6_slot[tr] = FM6_OWN;                          /* (fm6_poll: nothing reloads over it) */
    t->p[P_E7] = FM6_OWN;
}

/* view id (params.c ID_FO_*) turned steps detents of KNOB knob (0..3): OP and STAGE pick, the others write their byte */
static void fop_turn(uint32_t id, int32_t steps, uint32_t knob)
{
    uint32_t b;
    int32_t max;
    if (!fop_ok() || !steps)
        return;
    if (id == ID_FO_OP) {
        fop_op = (uint8_t)clamp((int32_t)fop_op + steps, 0, 5);
        return;
    }
    if (id == ID_FO_STG) {
        fop_stg = (uint8_t)clamp((int32_t)fop_stg + steps, 0, 3);
        return;
    }
    b = fop_byte(id);
    max = (int32_t)fm6_max(b);
    fop_write(b, (uint32_t)clamp((int32_t)fm6_patch[song.sel % NTRK][b] + accel(EN_K1 + (knob & 3u), steps, max), 0, max));
}
/* .. its descriptor (params.c FLD_DESC; RATIO of a FIXED operator: FREQ, its decade) and value */
static const param_desc_t *fop_ref(uint32_t id, int16_t **valp)
{
    static int16_t v;
    const uint8_t *op = fop_bytes(song.sel);
    const param_desc_t *d = &FLD_DESC[(id - ID_FO_OP) % (ID_FO_END - ID_FO_OP)];
    *valp = &v;
    if (id == ID_FO_OP)
        v = (int16_t)(fop_op % 6u);
    else if (id == ID_FO_STG)
        v = (int16_t)(fop_stg & 3u);
    else
        v = (int16_t)fm6_patch[song.sel % NTRK][fop_byte(id)];
    if (id == ID_FO_CRS && op[FP_MODE]) {
        v &= 3;
        return &FLD_FO_FIXED;
    }
    return d;
}

/* "w.ff" (frac: digits places) */
static void fop_dec(char *s, uint32_t w, uint32_t frac, uint32_t digits)
{
    fmt_int(s, (int32_t)w);
    s += str_len(s);
    *s++ = '.';
    if (digits > 1u)
        *s++ = (char)('0' + frac / 10u % 10u);
    *s++ = (char)('0' + frac % 10u);
    *s = 0;
}
/* the operator's frequency (op: its 21 bytes): "x1.50" (ratio: COARSE, 0 = 0.50, x (1 + FINE / 100)) or "440 HZ"
 * (fixed: 10 ^ (COARSE & 3 + FINE / 100) Hz, as fm6_core.c fm6_op_freq); s holds 12 */
static void fop_freq(const uint8_t *op, char *s)
{
    uint32_t c = op[FP_FC] & 31u, f = op[FP_FF] % 100u, m = 1000u, k;
    if (!op[FP_MODE]) {
        uint32_t r = (c ? c * 100u : 50u) * (100u + f) / 100u;   /* hundredths */
        s[0] = 'x';
        fop_dec(s + 1, r / 100u, r % 100u, 2u);
        return;
    }
    for (k = 0; k < f; k++)
        m = (m * 67063u + 32768u) >> 16;            /* x 10 ^ 0.01 (Q16): m = 1000 .. 9772 */
    for (k = 0; k < (c & 3u); k++)
        m *= 10u;                                    /* mHz */
    if (m < 10000u)
        fop_dec(s, m / 1000u, m % 1000u / 10u, 2u);
    else if (m < 100000u)
        fop_dec(s, m / 1000u, m % 1000u / 100u, 1u);
    else if (m < 1000000u)
        fmt_int(s, (int32_t)(m / 1000u));
    else
        fop_dec(s, m / 1000000u, m % 1000000u / 10000u, 2u);
    str_cpy(s + str_len(s), m < 1000000u ? " HZ" : " KHZ", 5);
}
