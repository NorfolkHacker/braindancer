/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker
 * JIANT (FELUCCA TONIC): a drum of the DRUM-X kit edited (docs/TONIC-DRUMX.md). */
/* (ACID FM 3b task 5) On the ux pages (ux_section.c; params.c's views ID_DX_*), a DRUM track's EDIT: SOUND (DRUM LEVEL PITCH
 * DECAY), SOUND 2 (DRUM NOISE COLOR WAVE) and MORE's kit and drum rows. They edit the section's kit (drumx_voice.c dx_kit,
 * dx_lay, dx_tone, dx_hands; eng_drum.c dx_snd, dx_mot: saved with the section, project.c FUNA), as JIANT's SOUND 1..4,
 * KIT 3 and X-MOD pages did (their arithmetic kept, a detent each):
 *   DRUM (KNOB 1 on SOUND and SOUND 2: one pick a track, dx_ui_lane; a key struck on the track picks its drum too, and a
 *   step held on the grid its lane's: dx_follow)
 *   the drum's side's PITCH, DECAY, NOISE, COLOR, PITCH MOD (SIDE: A or B; KIT's MORPH plays between the two)
 *   its mode byte as three: WAVE (SINE FM METAL BELL), NOISE TYPE (LP BP HP), SNAP (the noise's tail a quarter: JIANT's one
 *   MODE knob of 24 "FM BP+"); MOD TYPE (the pitch modulation: DECAY LONG NOISE SINE), DRIVE (OFF / ON)
 *   CLANG (JIANT's LAYER: a second oscillator, a metal edge .. a clang), FEEDBACK (the drum's own, besides the kit's: gen.c
 *   sets the snare's), SEND TO (OFF CHO DLY REV) and SEND (how much: JIANT's one SEND knob of 49; OFF while SEND TO is)
 *   the kit's: METAL (TONE's BP, a band-pass on the hats, rim and bell), FEEDBACK, HANDS (the clap's); MORPH LFO (X-MOD:
 *   RATE DEPTH SHAPE RANDOM) */
static uint32_t dx_kb_seen;                          /* seq.c kb_tog at the last look: a bit that flipped since, a key struck */
static uint8_t dx_kb_looked;                         /* (3b task 10 fix) dx_follow ran this UI pass */
static uint32_t dx_lane_picked(void) { return dx_ui_lane & 7u; }   /* (params.c page_id) */

/* (3b task 5 fix) each UI pass (ui_input.c): the drum picked follows the user's own hands only, never MIDI IN or the
 * sequencer playing (seq.c last_note follows both; and no 8,192-sample redraw a note in): a key struck on a DRUM track
 * picks that track's drum (any page; seq.c keyboard_block's keys, kb_tog / kb_note / kb_trk, read here; 3b task 10: kb_tog
 * latches the press, so one shorter than a UI frame - down and up between two passes - is seen too);
 * a step held on the DRUM grid picks its lane's (ui.lane: the hit it got, the LEVEL SOUND's KNOB 2 locks) */
static void dx_follow(void)
{
    uint32_t cur = kb_tog, up = cur ^ dx_kb_seen, k;
    dx_kb_seen = cur;
    dx_kb_looked = 1;
    for (k = 0; up && k < 27u; k++)
        if (((up >> k) & 1u) && kb_note[k] != KB_SILENT && drum_track(&trk[kb_trk[k] % NTRK]))
            dx_ui_lane_tr[kb_trk[k] % NTRK] = (uint8_t)drum_lane(kb_note[k]);
    if (grid_on() && drum_track(TSEL) && lock_held())
        dx_ui_lane = (uint8_t)(ui.lane % NLANE);
}
/* (3b task 10 fix) the end of each UI pass (ui_input.c ui_input): a pass that returned before dx_follow (MENU, a dialog,
 * NAME, SCREEN OFF's swallowed keys) took the keys struck meanwhile as its own: they pick no drum on the way back */
static void dx_pass_end(void)
{
    if (!dx_kb_looked)
        dx_kb_seen = kb_tog;
    dx_kb_looked = 0;
}
static int32_t dx_step(int32_t s) { return s > 0 ? 1 : -1; }   /* (JIANT: a detent a step on the small ranges) */
/* the drum picked's side's values (PITCH .. COLOR: drumx_voice.c DXP_*) */
static uint8_t *dx_side(void) { dx_lane_t *L = &dx_kit[dx_ui_lane & 7u]; return dx_ui_side ? L->b : L->a; }
static uint32_t dx_vi(uint32_t id)
{
    return id == ID_DX_PITCH ? DXP_PITCH : id == ID_DX_DECAY ? DXP_DECAY : id == ID_DX_NOISE ? DXP_NOISE :
           id == ID_DX_COLOR ? DXP_COLOR : DXP_PMOD;
}
/* view id (params.c ID_DX_*) turned steps detents of KNOB knob (0..3) */
static void dx_turn(uint32_t id, int32_t s, uint32_t knob)
{
    dx_lane_t *L = &dx_kit[dx_ui_lane & 7u];
    uint32_t ln = dx_ui_lane & 7u;
    if (!s)
        return;
    switch (id) {
    case ID_DX_LANE:
        dx_ui_lane = (uint8_t)clamp((int32_t)dx_ui_lane + dx_step(s), 0, 7);
        return;
    case ID_DX_SIDE:
        dx_ui_side = s > 0;
        return;
    case ID_DX_PITCH: case ID_DX_DECAY: case ID_DX_NOISE: case ID_DX_COLOR: case ID_DX_PMOD: {
        uint8_t *v = dx_side();
        uint32_t i = dx_vi(id);
        v[i] = (uint8_t)clamp(v[i] + accel(EN_K1 + (knob & 3u), s, 127), 0, 127);
        return;
    }
    case ID_DX_WAVE:                                    /* (the mode's bits 0..1, 2..3, 4: drumx_voice.c DX_MODE) */
        L->mode = (uint8_t)((L->mode & ~3u) | (uint32_t)clamp((int32_t)(L->mode & 3u) + dx_step(s), 0, 3));
        return;
    case ID_DX_NTYPE:
        L->mode = (uint8_t)((L->mode & ~0x0Cu) | (uint32_t)clamp((int32_t)((L->mode >> 2) & 3u) % 3 + dx_step(s), 0, 2) << 2);
        return;
    case ID_DX_SNAP:
        L->mode = (uint8_t)(s > 0 ? L->mode | 0x10u : L->mode & ~0x10u);
        return;
    case ID_DX_PMT:
        L->mode = (uint8_t)((L->mode & ~0x60u) | (uint32_t)clamp((int32_t)((L->mode >> 5) & 3u) + dx_step(s), 0, 3) << 5);
        return;
    case ID_DX_DRV:
        L->mode = (uint8_t)(s > 0 ? L->mode | DX_DRIVE : L->mode & ~DX_DRIVE);
        return;
    case ID_DX_CLANG: case ID_DX_LFB: {                 /* (JIANT's SOUND 4: LAYER, FEEDBK) */
        int32_t la = (int32_t)DX_LAY_L(dx_lay[ln]), fb = (int32_t)DX_LAY_FB(dx_lay[ln]);
        if (id == ID_DX_CLANG) la = clamp(la + dx_step(s), 0, 15);
        else fb = clamp(fb + dx_step(s), 0, 15);
        dx_lay[ln] = (uint8_t)(la | fb << 4);
        return;
    }
    case ID_DX_SENDTO: {                                /* (the bus: from OFF, the amount JIANT's first, 1 of 16; back */
        uint32_t b = (uint32_t)clamp((int32_t)DXS_BUS(dx_snd[ln]) + dx_step(s), 0, 3);   /* to OFF: 0, JIANT's OFF) */
        int32_t a = DXS_AMT(dx_snd[ln]);
        dx_snd[ln] = (uint8_t)(b ? b << 6 | (uint32_t)(a < 3 ? 3 : a) : 0u);
        return;
    }
    case ID_DX_SEND: {                                  /* (16 amounts, as JIANT's SEND: 4 n - 1; the bus OFF: nothing, */
        int32_t a = clamp((DXS_AMT(dx_snd[ln]) + 1) / 4, 1, 16);   /* said why, spec 0 rule 6) */
        if (!DXS_BUS(dx_snd[ln])) {
            ui_message("SEND TO IS OFF: PICK A BUS");
            return;
        }
        a = clamp(a + dx_step(s), 1, 16);
        dx_snd[ln] = (uint8_t)(DXS_BUS(dx_snd[ln]) << 6 | (uint32_t)(a * 4 - 1));
        return;
    }
    case ID_DX_BP: case ID_DX_KFB: case ID_DX_HANDS: {  /* (JIANT's KIT 3) */
        uint32_t bp = DX_TONE_BP(dx_tone), fb = DX_TONE_FB(dx_tone);
        if (id == ID_DX_BP) bp = (uint32_t)clamp((int32_t)bp + dx_step(s), 0, 15);
        else if (id == ID_DX_KFB) fb = (uint32_t)clamp((int32_t)fb + dx_step(s), 0, 15);
        else dx_hands = (uint8_t)clamp((int32_t)dx_hands + dx_step(s), 0, 15);
        dx_tone = (uint8_t)(bp | fb << 4);
        return;
    }
    default: {                                          /* (JIANT 0.5.1) X-MOD: RATE DEPTH SHAPE RANDOM, the kit's */
        static const uint8_t MX[4] = {8, 127, 4, 127};
        uint32_t k = (id - ID_DX_XRATE) & 3u;
        if (id < ID_DX_XRATE || id > ID_DX_XRND)
            return;
        dx_mot[k] = (uint8_t)clamp((int32_t)dx_mot[k] + (MX[k] > 8 ? accel(EN_K1 + (knob & 3u), s, 127) : dx_step(s)), 0,
                                   MX[k]);
        return;
    }
    }
}
/* .. its descriptor (params.c FLD_DX_DESC; METAL off: OFF) and value */
static const param_desc_t *dx_ref(uint32_t id, int16_t **valp)
{
    static int16_t v;
    const dx_lane_t *L = &dx_kit[dx_ui_lane & 7u];
    uint32_t ln = dx_ui_lane & 7u, m = L->mode;
    *valp = &v;
    switch (id) {
    case ID_DX_LANE: v = (int16_t)ln; break;
    case ID_DX_SIDE: v = (int16_t)(dx_ui_side != 0); break;
    case ID_DX_PITCH: case ID_DX_DECAY: case ID_DX_NOISE: case ID_DX_COLOR: case ID_DX_PMOD: v = dx_side()[dx_vi(id)]; break;
    case ID_DX_WAVE: v = (int16_t)(m & 3u); break;
    case ID_DX_NTYPE: v = (int16_t)(((m >> 2) & 3u) % 3u); break;
    case ID_DX_SNAP: v = (int16_t)((m >> 4) & 1u); break;
    case ID_DX_PMT: v = (int16_t)((m >> 5) & 3u); break;
    case ID_DX_DRV: v = (int16_t)((m & DX_DRIVE) != 0u); break;
    case ID_DX_CLANG: v = (int16_t)DX_LAY_L(dx_lay[ln]); break;
    case ID_DX_LFB: v = (int16_t)DX_LAY_FB(dx_lay[ln]); break;
    case ID_DX_SENDTO: v = (int16_t)DXS_BUS(dx_snd[ln]); break;
    case ID_DX_SEND:
        if (!DXS_BUS(dx_snd[ln])) {                     /* (no bus: OFF, no bar) */
            v = 0;
            return &FLD_DX_SENDOFF;
        }
        v = (int16_t)(clamp((DXS_AMT(dx_snd[ln]) + 1) / 4, 1, 16) * 100 / 16);
        break;
    case ID_DX_BP:
        if (!DX_TONE_BP(dx_tone)) {
            v = 0;
            return &FLD_DX_BPOFF;
        }
        v = (int16_t)(78u + DX_TONE_BP(dx_tone) * 2u);   /* (its centre, as the filter sets it: a CUTOFF_HZ index) */
        break;
    case ID_DX_KFB: v = (int16_t)DX_TONE_FB(dx_tone); break;
    case ID_DX_HANDS: v = (int16_t)dx_hands; break;
    default: v = (int16_t)dx_mot[(id - ID_DX_XRATE) & 3u]; break;
    }
    return &FLD_DX_DESC[(id - ID_DX_LANE) % (ID_DX_END - ID_DX_LANE)];
}

/* params.c's views: FM6's operator (ui_fm6op.c), DRUM-X's kit (above) */
static const param_desc_t *fld_ref(uint32_t id, int16_t **valp)
{
    if (!fld_is(id)) {
        *valp = 0;
        return 0;
    }
    return id >= ID_DX_LANE ? dx_ref(id, valp) : fop_ref(id, valp);
}
static void fld_turn(uint32_t id, int32_t steps, uint32_t knob)
{
    if (id >= ID_DX_LANE && id < ID_DX_END)
        dx_turn(id, steps, knob);
    else if (id >= ID_FO_OP && id < ID_FO_END)
        fop_turn(id, steps, knob);
}
