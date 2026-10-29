/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker */
/* (ACID FM 0.6, 3c-2 task 6) PLAYS, a track-wide loop condition (core.h G_PLAYS1 .. G_PLAYS6, seq.c trk_plays_on), on the
 * real sequencer, project and UI sources (with the stubs of tests/ui_test.c):
 *   LOOPS      a one-step loop of T1: ALWAYS a note every pass; 1ST OF 2 on loops 0 2 4; EVERY 2ND on 1 3; 1ST ONLY on 0;
 *              the step's own CONDITION on top of NOT 1ST; a DRUM-X hit silent on the loops PLAYS leaves out.
 *   AS MUTED   a lock on the step still applies on a loop PLAYS leaves out (motion_step runs); MIDI OUT sends no note.
 *   STORE      PLAYS ALWAYS everywhere: no 'P' 'L' block, the size and bytes as before; T5 1ST OF 2: the block the last,
 *              after 'M' 'R', the round trip equal; a project without it loads ALWAYS; NEW PROJECT resets it, UNDO brings
 *              it back, the sequence dice (and their OCT-) leave it alone.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static const uint8_t WANT_1OF2[5] = {1, 0, 1, 0, 1};      /* PLAYS 1ST OF 2 (1): loops 0, 2, 4 */
static const uint8_t WANT_MIX[5] = {0, 0, 1, 0, 1};       /* PLAYS NOT 1ST (12) + the step's CONDITION 1ST OF 2 */

/* T1 alone, a loop of len steps: step 0 NOTE 60, the rest REST, GATE 64, no swing; PLAYS plays; the transport started */
static track_t *pl_track(uint32_t len, int16_t plays)
{
    track_t *t = &trk[0];
    uint32_t i;
    track_defaults_steps(t);
    t->p[P_SLEN] = (int16_t)len;
    t->p[P_SGATE] = 64;
    t->p[P_SSWING] = 0;
    song.g[G_SWING] = 0;
    t->step[0] = (step_t){{60}, 1, ST_NOTE, 0, 100};
    for (i = 1; i < len; i++)
        t->step[i] = (step_t){{0}, 0, ST_REST};
    song.g[G_PLAYS1] = plays;
    seq_start();
    return t;
}
static uint32_t pl_len(const track_t *t) { return step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), 0); }
/* note-ons (voice starts) in each of n passes of a one-step loop */
static void pl_run(track_t *t, uint32_t n, uint8_t *got)
{
    uint32_t len = pl_len(t), b, blocks = n * len / CTL - 1u;
    memset(got, 0, n);
    for (b = 0; b < blocks; b++) {
        uint32_t v0 = vage, k = ((b + 1u) * CTL - 1u) / len;
        seq_tick(t, CTL);
        if (k < n)
            got[k] = (uint8_t)(got[k] + (vage - v0));
    }
}
static int pl_is(const char *what, const uint8_t *got, const uint8_t *want)
{
    if (!memcmp(got, want, 5))
        return 1;
    printf("  %s: %u %u %u %u %u, wanted %u %u %u %u %u\n", what, got[0], got[1], got[2], got[3], got[4], want[0], want[1],
           want[2], want[3], want[4]);
    return 0;
}

static int test_plays_loops(void)
{
    static const uint8_t ALL[5] = {1, 1, 1, 1, 1}, EV2[5] = {0, 1, 0, 1, 0}, FIRST[5] = {1, 0, 0, 0, 0};
    uint8_t got[5];
    int ok;
    ui_power_on();
    pl_run(pl_track(1, 0), 5, got);
    ok = pl_is("ALWAYS", got, ALL);
    pl_run(pl_track(1, 1), 5, got);
    ok &= pl_is("1ST OF 2", got, WANT_1OF2);
    pl_run(pl_track(1, 2), 5, got);
    ok &= pl_is("EVERY 2ND", got, EV2);
    pl_run(pl_track(1, 11), 5, got);
    ok &= pl_is("1ST ONLY", got, FIRST);
    return check("PLAYS: ALWAYS every loop; 1ST OF 2 loops 0 2 4; EVERY 2ND 1 3; 1ST ONLY loop 0 (the track's loops)", ok);
}
static int test_plays_cond(void)
{
    uint8_t got[5];
    track_t *t;
    int ok;
    ui_power_on();
    t = pl_track(1, 12);
    uxq_cond_turn(&t->step[0], 1);                      /* (STEP EXTRAS' CONDITION: 1ST OF 2) */
    ok = t->step[0].comp == CP_TRIG && t->step[0].carg == 0u;
    seq_start();
    pl_run(t, 5, got);
    ok &= pl_is("NOT 1ST + CONDITION 1ST OF 2", got, WANT_MIX);
    return check("PLAYS NOT 1ST and the step's CONDITION 1ST OF 2: the step on loops 2 and 4 (its CONDITION on the loops PLAYS "
                 "lets through)", ok);
}
static int test_plays_drum(void)
{
    uint8_t got[5];
    track_t *t;
    int ok;
    ui_power_on();
    t = pl_track(1, 0);
    set_engine_of(t, ENGI_DRUM);
    t->engine = t->eng_req;
    t->step[0] = (step_t){{0}, 0, ST_NOTE, 0, 100, 1u << DV_KICK};
    seq_start();
    pl_run(t, 5, got);
    ok = got[0] == 1u && got[1] == 1u;
    song.g[G_PLAYS1] = 1;
    seq_start();
    pl_run(t, 5, got);
    ok &= pl_is("DRUM-X 1ST OF 2", got, WANT_1OF2);
    return check("PLAYS 1ST OF 2 on a DRUM-X track: the kick's hit on loops 0 2 4 only", ok);
}
static int test_plays_locks(void)
{
    track_t *t;
    uint32_t len, b, blocks;
    int ok = 1, seen[2] = {0, 0}, n = 0, ok_mute;
    ui_power_on();
    t = pl_track(2, 1);                                 /* (step 0 NOTE, step 1 REST; loop 1 left out) */
    t->p[P_FCUT] = 100;
    ok &= !motion_set_lock(t, 0, P_FCUT, 10);
    seq_start();
    len = pl_len(t);
    blocks = 4u * len / CTL;
    for (b = 0; b < blocks; b++) {                      /* (loops 0 and 1: step 0 at 10 both times, step 1 at 100) */
        uint32_t v0 = vage, pos = b * CTL + CTL - 1u, step = (pos / len) % 2u, loop = pos / len / 2u;
        seq_tick(t, CTL);
        n += (int)(vage - v0);
        if (pos % len < CTL || pos % len > len - CTL)
            continue;                                   /* (a block across a step's edge: either) */
        if (step == 0u && loop < 2u)
            seen[loop] |= t->p[P_FCUT] == 10;
        if (step == 1u)
            ok &= t->p[P_FCUT] == 100;
    }
    ok &= seen[0] && seen[1] && n == 1;
    t = pl_track(2, 0);                                 /* (as P_MUTE does it; the steps cleared: the lock again) */
    t->p[P_FCUT] = 100;
    ok &= !motion_set_lock(t, 0, P_FCUT, 10);
    t->p[P_MUTE] = 1;
    seq_start();
    ok_mute = 0;
    for (b = 0; b < len / CTL - 2u; b++) {
        seq_tick(t, CTL);
        ok_mute |= t->p[P_FCUT] == 10;
    }
    t->p[P_MUTE] = 0;
    return check("PLAYS: a loop it leaves out still applies the step's lock (P_FCUT 10 on loop 1, back to 100 after), no note: as "
                 "P_MUTE", ok && ok_mute);
}
static int test_plays_midi_out(void)
{
    track_t *t;
    uint32_t b, len, on[2] = {0, 0};
    int ok;
    ui_power_on();
    usb.config = 1;
    menu_put(MI_MIDIOUT, 2); frame();                   /* (KEYS+SEQ: the sequencer's notes out) */
    t = pl_track(1, 1);
    len = pl_len(t);
    mo_block(0);                                        /* (as events_block: the tracks' channels and OUT, mo_tch) */
    mo_r = mo_w;
    for (b = 0; b < 2u * len / CTL - 1u; b++) {
        uint32_t k = ((b + 1u) * CTL - 1u) / len;
        seq_tick(t, CTL);
        while (mo_r != mo_w) {
            uint32_t p = midi_out_q[mo_r++ % MQ], st = (p >> 8) & 0xFFu;
            if (st == 0x90u && k < 2u)
                on[k]++;
        }
    }
    ok = on[0] == 1u && on[1] == 0u;
    if (!ok) printf("  midi out: note-ons %u %u\n", on[0], on[1]);
    menu_put(MI_MIDIOUT, 0); frame();
    usb.config = 0;
    return check("PLAYS 1ST OF 2: MIDI OUT sends T1's note on loop 0, none on loop 1", ok);
}
static int test_plays_store(void)
{
    static project_store_t s0, s1, s2;
    project_t a, c;
    uint32_t n0, n1, i, k;
    int ok, bad = 0;
    const uint8_t *b0 = s0.raw, *b1 = s1.raw;
    ui_power_on();
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 100};
    song.g[G_MRSEL] = 1;                                /* (an 'M' 'R' block: PLAYS's after it) */
    project_capture(&a);
    n0 = proj_packed_size(&a);
    ok = proj_pack(&s0, &a);
    for (i = n0; i < PROJC_PFX_OFF; i++)
        ok &= b0[i] == 0u;                              /* (nothing after the last block: no 'P' 'L') */
    ok &= b0[n0 - 2u - PROJ_MR_N] == 'M' && b0[n0 - 1u - PROJ_MR_N] == 'R';
    for (k = 0; k < NTRK; k++)
        ok &= !a.g[G_PLAYS1 + k];
    bad += check("PLAYS ALWAYS everywhere: packed as before (no 'P' 'L', 'M' 'R' the last block, the size)", ok);
    song.g[G_PLAYS1 + 4] = 1;
    project_capture(&a);
    n1 = proj_packed_size(&a);
    ok = proj_pack(&s1, &a) && n1 == n0 + 2u + PROJ_PL_N && !memcmp(b0, b1, n0) && b1[n0] == 'P' && b1[n0 + 1u] == 'L';
    for (k = 0; k < NTRK; k++)
        ok &= b1[n0 + 2u + k] == (k == 4u ? 1u : 0u);
    for (i = n1; i < PROJC_PFX_OFF; i++)
        ok &= b1[i] == 0u;
    ok &= proj_import(&c, &s1, sizeof s1) && c.g[G_PLAYS1 + 4] == 1 && proj_pack(&s2, &c) && !memcmp(&s1, &s2, sizeof s1);
    bad += check("T5 PLAYS 1ST OF 2: a 'P' 'L' block the last (after 'M' 'R'), one byte a track; the round trip byte for byte", ok);
    song.g[G_PLAYS1 + 2] = 7;
    ok = proj_import(&c, &s0, sizeof s0);
    for (k = 0; k < NTRK; k++)
        ok &= !c.g[G_PLAYS1 + k];
    ok &= !project_restore_runtime(&c);
    for (k = 0; k < NTRK; k++)
        ok &= !song.g[G_PLAYS1 + k];
    bad += check("a project without 'P' 'L' (before 0.6): every PLAYS ALWAYS, whatever the song held", ok);
    for (k = 0; k < NTRK; k++)
        song.g[G_PLAYS1 + k] = (int16_t)(k + 1u);
    system_init();
    for (ok = 1, k = 0; k < NTRK; k++)
        ok &= !song.g[G_PLAYS1 + k];
    bad += check("NEW PROJECT (system_init): every PLAYS ALWAYS", ok);
    song.g[G_PLAYS1 + 4] = 3;
    undo_mark();
    song.g[G_PLAYS1 + 4] = 0;
    ok = !undo_do() && song.g[G_PLAYS1 + 4] == 3;
    undo_mark();
    song.g[G_PLAYS1 + 4] = 9;
    ok &= !undo_do() && song.g[G_PLAYS1 + 4] == 3;
    bad += check("UNDO brings PLAYS back (3 after 0, 3 after 9)", ok);
    ui_power_on();
    track_select(4); frame();
    for (k = 0; k < NTRK; k++)
        song.g[G_PLAYS1 + k] = (int16_t)(12u - k);
    part_dice(CF_DICE_SEQ); frame();
    ok = ui.confirm == CF_DICE_SEQ;
    for (k = 0; k < NTRK; k++)
        ok &= song.g[G_PLAYS1 + k] == (int16_t)(12u - k);
    press(B_OCTDN); frames(64);
    ok &= ui.confirm == CF_NONE;
    for (k = 0; k < NTRK; k++)
        ok &= song.g[G_PLAYS1 + k] == (int16_t)(12u - k);
    bad += check("the sequence dice and their OCT- leave every PLAYS alone", ok);
    ui_power_on();
    return bad;
}

int main(void)
{
    int bad = test_plays_loops() + test_plays_cond() + test_plays_drum() + test_plays_locks() + test_plays_midi_out() +
              test_plays_store();
    printf("%s\n", bad ? "PLAYS TEST FAILED" : "plays tests passed");
    return bad != 0;
}
