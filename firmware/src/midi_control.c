/* SPDX-License-Identifier: GPL-3.0-only
 * Adapted from MIDI control contribution by ChanceTheMaker (2026).
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* Shared USB/TRS channel controls. Included by seq.c after its input helpers.
 * USB and TRS intentionally share channel state, matching the existing routing.
 * A synth part has one live bend/wheel state; channels assigned to the same part
 * share it (last controller wins). Drum hits ignore bend and sustain. CC1 keeps the existing matrix routing. */
typedef struct {
    int16_t bend;                           /* signed 14-bit value, zero = centre */
    uint8_t wheel, pedal, targets;
    uint8_t owned[NTRK];                    /* held/pedal notes per track, at most 128 per channel */
    uint8_t ready, semis, cents;
    uint8_t rpn_msb, rpn_lsb;
} midi_channel_t;
static midi_channel_t midi_ch[16];
/* A nibble per note (JIANT: two to a byte, 1 KB): low bits track + 1, bit 3 key released, held by its channel's pedal.
 * (ACID FM 0.5) MN_MULTI: the note plays on every track the channel's tracks (midi_claims: two or more); a change of
 * the routing lets go of the notes of every channel it changes (midi_reroute), so the claims are still the note-on's */
static uint8_t midi_sel_on[16][64];
static uint32_t mn_get(uint32_t ch, uint32_t note) { return (midi_sel_on[ch & 15u][(note & 127u) >> 1] >> ((note & 1u) * 4u)) & 15u; }
static void mn_set(uint32_t ch, uint32_t note, uint32_t v)
{
    uint8_t *b = &midi_sel_on[ch & 15u][(note & 127u) >> 1];
    uint32_t sh = (note & 1u) * 4u;
    *b = (uint8_t)((*b & ~(15u << sh)) | (v & 15u) << sh);
}
static uint16_t midi_owners[NTRK];           /* avoids rescanning all 2048 entries for CC123 */
#define MIDI_PEDAL_NOTE 0x8u
#define MN_MULTI 0x7u

static midi_channel_t *midi_channel(uint32_t ch)
{
    midi_channel_t *c = &midi_ch[ch];
    if (!c->ready) {
        c->semis = 2;
        c->rpn_msb = c->rpn_lsb = 127;
        c->ready = 1;
    }
    return c;
}

static uint32_t midi_targets(uint32_t ch)
{
    return midi_channel(ch)->targets | midi_in_mask(ch);
}

static void midi_expression(track_t *t, const midi_channel_t *c)
{
    int32_t range = ((int32_t)c->semis * 100 + c->cents) * 256 / 100;
    if (drum_track(t))
        return;
    midi_bend_target[trk_index(t)] = (int32_t)c->bend * range / (c->bend < 0 ? 8192 : 8191);
}

static void midi_expression_channel(uint32_t ch)
{
    uint32_t i, mask = midi_targets(ch);
    for (i = 0; i < NPART; i++)
        if (mask & (1u << i))
            midi_expression(&trk[i], midi_channel(ch));
}

/* a MIDI note of track t sounds note */
static int midi_note_held(const track_t *t, uint32_t note)
{
    uint32_t ch, id = trk_index(t) + 1u, v;
    for (ch = 0; ch < 16u; ch++)
        if ((v = mn_get(ch, note) & 0x7u) == id ||     /* (MN_MULTI: not on a track forgotten since, midi_forget_track) */
            (v == MN_MULTI && ((midi_claims(ch) >> (id - 1u)) & 1u) && midi_ch[ch].owned[id - 1u]))
            return 1;
    return 0;
}

/* a key held on track t sounds note */
static int midi_local_held(const track_t *t, uint32_t note)
{
    uint32_t k;
    for (k = 0; k < 27u; k++)
        if (kb_on[k] && kb_trk[k] == trk_index(t)) {   /* (0.6.81: a key's chord, the notes started) */
            uint32_t i;
            for (i = 0; i < kb_on[k]; i++)
                if (kb_snd[k][i] == note)
                    return 1;
        }
    return 0;
}

/* (ch, note) lets go on the tracks of its nibble (MN_MULTI: of mm, the channel's tracks at its note-on). A track that
 * no longer owns a note there (forgotten: midi_forget_track) is left alone */
static void midi_release_m(uint32_t ch, uint32_t note, uint32_t mm)
{
    uint32_t id = mn_get(ch, note) & 0x7u, m, k;
    mn_set(ch, note, 0);
    if (!id)
        return;
    m = id == MN_MULTI ? mm : 1u << (id - 1u);
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        if (!((m >> k) & 1u) || !midi_ch[ch].owned[k])
            continue;
        if (midi_owners[k])
            midi_owners[k]--;
        if (!--midi_ch[ch].owned[k])
            midi_ch[ch].targets &= (uint8_t)~(1u << k);
        if (drum_track(t) && drum_oct_of(note)) {   /* (0.8.5) a band of the kit's octaves */
            dxo_off(t, DXO_MIDI + (ch << 7) + note);
            if (!midi_local_held(t, drum_oct_base(note)))
                input_off(t, drum_oct_base(note));
        } else if (!midi_local_held(t, note))
            input_off(t, note);                     /* input_off also checks other MIDI owners */
    }
}
static void midi_release(uint32_t ch, uint32_t note) { midi_release_m(ch, note, midi_claims(ch)); }

/* a MIDI note-on (ch, note) of track t. A note another key or MIDI note holds already sounds: not started again */
static void midi_play(track_t *t, uint32_t ch, uint32_t note, uint32_t vel)
{
    if (drum_track(t) && drum_oct_of(note)) {          /* (0.8.5) the kit in another octave: its sound, ratcheted /
                                                         * damped (seq.c dxo_on) */
        dxo_on(t, drum_oct_base(note), drum_oct_of(note), vel, DXO_MIDI + (ch << 7) + note);
        return;
    }
    if (!midi_note_held(t, note) && !midi_local_held(t, note))
        input_on(t, note, vel);
}

/* (ACID FM 0.5) a note on channel ch plays on every track receiving ch (midi_in_mask) */
static void midi_note_event(uint32_t ch, uint32_t note, uint32_t vel)
{
    midi_channel_t *c = midi_channel(ch);
    uint32_t id = mn_get(ch, note) & 0x7u, m, k, n = 0, first = NTRK;
    if (vel) {
        m = midi_in_mask(ch);
        /* Repeated notes replace the previous press, including a pedal-held one. */
        if (id)
            midi_release(ch, note);
        for (k = 0; k < NTRK; k++) {
            track_t *t = &trk[k];
            if (!((m >> k) & 1u))
                continue;
            midi_expression(t, c);
            c->targets |= (uint8_t)(1u << k);
            midi_play(t, ch, note, vel);
            midi_owners[k]++;
            c->owned[k]++;
            first = n++ ? first : k;
        }
        if (!n)
            return;
        if (!((m >> song.sel) & 1u))
            midi_hint = (uint8_t)(first + 1u);
        mn_set(ch, note, n > 1u ? MN_MULTI : first + 1u);
    } else if (id) {
        int synth = 0;
        m = id == MN_MULTI ? midi_claims(ch) : 1u << (id - 1u);
        for (k = 0; k < NTRK; k++)
            synth |= ((m >> k) & 1u) && !drum_track(&trk[k]);
        if (c->pedal && synth)
            mn_set(ch, note, mn_get(ch, note) | MIDI_PEDAL_NOTE);
        else
            midi_release(ch, note);
    }
}

static void midi_pedal_up(uint32_t ch)
{
    uint32_t note;
    midi_channel(ch)->pedal = 0;
    for (note = 0; note < 128u; note++)
        if (mn_get(ch, note) & MIDI_PEDAL_NOTE)
            midi_release(ch, note);
}

/* Preset/project panic and CC120 must discard ownership so a later pedal-up
 * or note-off cannot release notes subsequently started on another patch. */
static void __attribute__((noinline)) midi_forget_track(uint32_t track)
{
    uint32_t ch, note;
    for (ch = 0; ch < 16u; ch++) {
        if (!(midi_ch[ch].targets & (1u << track)))
            continue;
        for (note = 0; note < 128u; note++)
            if ((mn_get(ch, note) & 0x7u) == track + 1u)
                mn_set(ch, note, 0);
        midi_ch[ch].targets &= (uint8_t)~(1u << track);
        midi_ch[ch].owned[track] = 0;
    }
    midi_owners[track] = 0;
    midi_bend_q8[track] = midi_bend_target[track] = 0;
}

static void midi_silence_track(uint32_t track)
{
    track_t *t = &trk[track];
    uint32_t i;
    trk_all_off(t);
    if (track < NPART)
        dxo_n[track] = 0;                     /* (0.8.5) a held ratchet stops */
    t->nheld = t->arp_phys = t->rh_n = 0;
    arp_forget(t);
    t->seq_n = t->seq_hold = t->slide_glide = 0;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active)
            voice_kill(&t->v[i]);             /* one-block fade, regardless of RELEASE */
    slicer_drop();                            /* do not keep replaying captured sound (the SLICER's bus) */
    midi_forget_track(track);
}

static int midi_track_held(uint32_t track)
{
    uint32_t k;
    if (midi_owners[track])
        return 1;
    for (k = 0; k < 27u; k++)
        if ((fm1_in.notes & (1u << k)) && kb_trk[k] == track)
            return 1;
    return 0;
}

/* The standard CC map (#103): always on, these CCs only, on the track the channel plays (ROUT as notes). A CC
 * sets its parameter as a knob would, 0..127 over the parameter's range (64: the middle of a bipolar one), and
 * AUTOMATION records it as a knob move (motion_capture: the selected track, armed and playing). CC74 / CC71 are
 * the engine's own brightness / resonance (MIDI_CC_ENG); an engine without one ignores them. CC121 resets none
 * of them (RP-015: they are the sound's) */
#define MCC_CUT 0xFEu
#define MCC_RES 0xFFu
static const uint8_t MIDI_CC_MAP[][2] = {
    {5, P_GLIDE}, {7, P_LEVEL}, {10, P_PAN}, {71, MCC_RES}, {72, P_REL}, {73, P_ATK}, {74, MCC_CUT}, {75, P_DEC},
    {91, P_REV}, {93, P_CHOR}, {94, P_DLY},
    /* (0.6.96) the engine's own eight values (EDIT 1 / EDIT 2's knobs, whatever the engine: CC16..23 = E1..E8), the
     * post filter, drive and sustain (CC102..105) */
    {16, P_E0}, {17, P_E1}, {18, P_E2}, {19, P_E3}, {20, P_E4}, {21, P_E5}, {22, P_E6}, {23, P_E7},
    {102, P_FCUT}, {103, P_FRES}, {104, P_DIST}, {105, P_SUS},
};
/* per engine (ENGINES[] order): CC74's E index + 1 in the high nibble, CC71's in the low one, 0 = none */
static const uint8_t MIDI_CC_ENG[NENGINES] = {
    0x56, 0x00, 0x30, 0x80, 0x50, 0x07, 0x67, 0x00,   /* ANALOG CUT RES, -, PHASE DCW, LOFI TONE, SAMPLE CUT, VOICE Q,
                                                       * TRIO CUT RES, WHEEL - (retired) */
    0x80, 0x30, 0x30, 0x34, 0x30,                     /* GRAIN TONE, PHYS BRIT, DRUM TONE, NOISE FREQ RES, FM6 MLVL */
    0x80,                                             /* SLICE TONE (retired: ANALOG's played) */
    0x00,                                             /* ANIMAL (retired in ACID FM 0.2: ANALOG's played) */
};
/* a CC sets parameter id of track t as a knob would: 0..127 over its range (the map's and MIDI LEARN's) */
static void midi_cc_set(track_t *t, uint32_t id, uint32_t value)
{
    const param_desc_t *d = param_desc_of(eng_idx(t->eng_req), id);
    int32_t v = d->min + ((int32_t)value * (d->max - d->min) + 63) / 127;
    if (d->max <= d->min || t->p[id] == v)
        return;
    t->p[id] = (int16_t)v;
    (void)motion_capture(t, id, (int16_t)v);
}
/* MIDI LEARN (JIANT 0.9.12, from Felucca 1.5 by Leo Kuroshita, Discussion #170; midi_learn.c the UI): up to ML_N CCs,
 * each set to one parameter of one track, whatever channel it comes on (a channel ROUT ignores stays ignored) and over
 * USB or the TRS jack alike (one queue: seq.c events_block). Kept with the settings in a favourites row no engine
 * uses (DIGITAL's, engine 1: retired, its byte 0 the old stars' migration, settings_persist.c; bytes 2..31 0 in every
 * older setting = nothing learned): an entry three bytes, the CC | 0x80 (0: empty), the track, the parameter's code: a
 * common parameter its P_* id + 1 (1 .. ML_E0 - 1: their ids never move, new ones go before P_E0), an engine
 * parameter ML_E0 + k for P_E0 + k (P_E0 moves when common ones are added). The CCs with a meaning of their own are
 * never learned (ml_free_cc): CC0 / 32 bank select, CC1 the wheel, CC6 / 38 / 96..101 data entry and RPN, CC11 EXPR,
 * CC64 the pedal, CC120..127 the channel modes, and the standard map's (MIDI_CC_MAP): so a learned CC and the map never
 * meet. The UI writes the entries with the interrupts off; ml.arm / ml.heard hand it a CC heard while learning. Its
 * state in the pool (not among the globals the audio ISR reaches) */
#define ML_N 10u
#define ML_E0 120u
typedef char ml_ids_fit[P_E0 < ML_E0 && P_COUNT - P_E0 == 8 && NTRK <= 8 ? 1 : -1];
static uint8_t *const ml_tab;               /* (midi_learn.c: favorites.factory[ENGI_DIGITAL] + 2; a build without the UI none) */
static struct {
    uint8_t on, pick, trk, id;               /* LEARN on; a parameter picked (a knob turned): track, P_* id */
    volatile uint8_t arm, heard;             /* the UI waits for a CC; the CC that came meanwhile + 1 (0 none) */
} ml __attribute__((section(".pool")));
static uint32_t ml_id(uint32_t code)        /* an entry's code -> P_* id, P_COUNT: none this firmware knows */
{
    return code >= ML_E0 ? P_E0 + code - ML_E0 : code && code <= P_E0 ? code - 1u : P_COUNT;
}
static int ml_free_cc(uint32_t cc)
{
    uint32_t i;
    if (cc < 2u || cc == 6u || cc == 11u || cc == 32u || cc == 38u || cc == 64u || (cc >= 96u && cc <= 101u) || cc >= 120u)
        return 0;
    for (i = 0; i < NELEM(MIDI_CC_MAP); i++)
        if (MIDI_CC_MAP[i][0] == cc)
            return 0;
    return 1;
}
/* 1: cc is learned (set here) or taken by MIDI LEARN waiting for one (a CC never learned still does its own) */
static int __attribute__((noinline)) midi_learned(uint32_t cc, uint32_t value)
{
    const uint8_t *e;
    uint32_t i, id, hit = 0;
    if (!ml_tab)
        return 0;
    if (ml.arm) {
        if (!ml.heard)
            ml.heard = (uint8_t)(cc + 1u);
        return ml_free_cc(cc);
    }
    for (i = 0, e = ml_tab; i < ML_N; i++, e += 3)
        if (e[0] == (0x80u | cc) && e[1] < NTRK && (id = ml_id(e[2])) < P_COUNT) {
            midi_cc_set(&trk[e[1]], id, value);
            hit = 1;
        }
    return hit;
}

static void __attribute__((noinline)) midi_cc_map(track_t *t, uint32_t cc, uint32_t value)
{
    uint32_t i, id = 0xFFFFu, e;
    for (i = 0; i < NELEM(MIDI_CC_MAP); i++)
        if (MIDI_CC_MAP[i][0] == cc)
            id = MIDI_CC_MAP[i][1];
    if (id >= MCC_CUT) {
        if (id == 0xFFFFu)
            return;
        e = MIDI_CC_ENG[eng_idx(t->eng_req)] >> (id == MCC_CUT ? 4 : 0) & 15u;
        if (!e)
            return;
        id = P_E0 + e - 1u;
    }
    midi_cc_set(t, id, value);
}

static void midi_control(uint32_t ch, uint32_t cc, uint32_t value)
{
    midi_channel_t *c = midi_channel(ch);
    uint32_t i, mask;
    if (midi_learned(cc, value))                   /* (0.9.12) MIDI LEARN's (a learned CC is the user's) */
        return;
    switch (cc) {
    case 1:
        c->wheel = (uint8_t)value;
        break;
    case 120:                                      /* All Sound Off: ignores the pedal */
        mask = midi_targets(ch);
        for (i = 0; i < NTRK; i++)
            if (mask & (1u << i))
                midi_silence_track(i);
        break;
    case 123:                                      /* All Notes Off: normal releases, honours pedal */
        mask = midi_targets(ch);
        for (i = 0; i < 128u; i++)
            if (mn_get(ch, i))
                midi_note_event(ch, i, 0);
        for (i = 0; i < NTRK; i++)
            if ((mask & (1u << i)) && !midi_track_held(i)) {
                trk_all_off(&trk[i]);
                trk[i].nheld = trk[i].arp_phys = trk[i].rh_n = 0;
                arp_forget(&trk[i]);
            }
        break;
    case 121:                                      /* Reset All Controllers, keep bend sensitivity */
        c->bend = 0;
        c->wheel = 0;
        c->rpn_msb = c->rpn_lsb = 127;
        midi_expression_channel(ch);
        midi_pedal_up(ch);
        break;
    case 64:
        if (value >= 64u)
            c->pedal = 1;
        else
            midi_pedal_up(ch);
        break;
    case 101: c->rpn_msb = (uint8_t)value; break;
    case 100: c->rpn_lsb = (uint8_t)value; break;
    case 99: case 98:                              /* NRPN selection cancels RPN data entry */
        c->rpn_msb = c->rpn_lsb = 127;
        break;
    case 6: case 38:
        if (!c->rpn_msb && !c->rpn_lsb) {           /* RPN 0: +/-0..24 semitones, 0..99 cents */
            if (cc == 6u)
                c->semis = (uint8_t)(value > 24u ? 24u : value);
            else
                c->cents = (uint8_t)(value > 99u ? 99u : value);
            midi_expression_channel(ch);
        }
        break;
    default:
        mask = midi_in_mask(ch);
        for (i = 0; i < NTRK; i++)
            if ((mask >> i) & 1u)
                midi_cc_map(&trk[i], cc, value);
        break;
    }
}

/* (ACID FM 0.5) the routing changed (events_block). A channel no longer heard lets go of what it holds: its notes
 * released (also pedal-held ones), its pedal, bend, wheel and RPN selection reset, so no note hangs on a channel no
 * longer heard (was midi_route_ch14, on ROUT's change to CH1-4). One heard by other tracks lets go of its notes on two
 * tracks or more (on the tracks of before: MN_MULTI); a note of one track follows its note-on's track, as across a
 * change of the selected track. Each track receiving then follows its own channel's bend; one receiving none, none */
static void __attribute__((noinline)) midi_reroute(void)
{
    uint32_t ch, note, i;
    for (ch = 0; ch < 16u; ch++) {
        uint32_t m = midi_claims(ch);
        uint8_t h = (uint8_t)(m ? m : song.g[G_MRSEL] ? 0x80u : 0u), was = midi_heard[ch];
        midi_channel_t *c;
        if (h == was)
            continue;
        midi_heard[ch] = h;
        c = midi_channel(ch);
        for (note = 0; note < 128u; note++)
            if (mn_get(ch, note) && (!h || (mn_get(ch, note) & 0x7u) == MN_MULTI))
                midi_release_m(ch, note, was & 0x3Fu);
        if (!h) {
            c->pedal = c->wheel = 0;
            c->bend = 0;
            c->rpn_msb = c->rpn_lsb = 127;
        }
    }
    for (i = 0; i < NTRK; i++)
        if (song.g[G_MRIN1 + i])
            midi_expression(&trk[i], midi_channel(trk_midi_ch(i)));
        else
            midi_bend_target[i] = 0;
}

/* (JIANT 0.6.72) PROGRAM CHANGE n on the channel a track plays (ACID FM 0.5: every track receiving it): the n + 1-th sound of the PRESETS list (every engine's
 * presets in their order, then the user presets; the favourites filter aside), loaded by the UI between frames (a
 * sound load is the main loop's: ui.c midi_pc_poll). 0 none, else n + 1 */
static volatile uint8_t midi_pc_req[NTRK];
/* (JIANT 0.6.80) PROGRAM CHANGE on channel 16, the song's own (never a track's, whatever the routing: ACID FM 0.5, a
 * track on channel 16 takes its notes and CCs, not its PROGRAM CHANGE): 0..3 the section A..D
 * (in on the next bar, as the song layer's keys), 16..19 the project 1..4 (ACID FM 0.2: four; stopped only, as SAVE > PROJECT). The UI
 * takes it between frames (ui.c midi_pc_poll). 0 none, else n + 1 */
static volatile uint8_t midi_song_req;

/* Keep the occasional controller/panic dispatch outside the hot rendering loop. Channel voice messages only
 * (realtime and clock are handled in events_block, SysEx never reaches here). A channel no track receives (ACID FM
 * 0.5: midi_in_mask; was ROUT CH1-4's channels 5..16) is ignored entirely: notes, bend, CCs (CC1/11/64, RPN, and the
 * CC120/121/123 panic and reset), channel aftertouch, so it stays free for other instruments (a note-off still
 * finds the note it ends, if any) */
static void __attribute__((noinline)) midi_event(uint32_t st, uint32_t ch, uint32_t d1, uint32_t d2)
{
    uint32_t m, k;
    if (st == 0xC0u && ch == 15u) {                    /* (0.6.80) channel 16's PROGRAM CHANGE: a section, a project */
        midi_song_req = (uint8_t)(d1 + 1u);
        return;
    }
    m = midi_in_mask(ch);
    if (st == 0x90u || st == 0x80u)
        midi_note_event(ch, d1, st == 0x90u ? d2 : 0);
    else if (!m)
        return;
    else if (st == 0xE0u) {
        midi_channel(ch)->bend = (int16_t)((int32_t)(d1 | (d2 << 7)) - 8192);
        midi_expression_channel(ch);
    } else if (st == 0xB0u || st == 0xD0u) {
        for (k = 0; k < NTRK; k++)
            if ((m >> k) & 1u)
                mod_midi(&trk[k], st, d1, d2);
        if (st == 0xB0u)
            midi_control(ch, d1, d2);
    } else if (st == 0xC0u) {                      /* (0.6.72) PROGRAM CHANGE: the UI loads it (ui.c midi_pc_poll) */
        for (k = 0; k < NTRK; k++)
            if ((m >> k) & 1u)
                midi_pc_req[k] = (uint8_t)(d1 + 1u);
    }
}
