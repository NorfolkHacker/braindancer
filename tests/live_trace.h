/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* (ACID FM 3c-1 task 6) GLO > LIVE against the inherited MANO / RIELES pages, input for input: one sequence of keys, knobs and
 * OCT gestures (LIVE's own, then the RAILS rows that were RIELES' knobs), the live state hashed after each stage. The hashes
 * LT_WANT were taken from the inherited pages (1b2ac25, MANO and RIELES' knobs: LT_OLD); the same input must leave the same
 * state. Included by tests/live_test.c (after tests/ui_test.c); the page reached by lt_live() / lt_rails() */
static uint32_t lt_h;
static void lt_mix(const void *p, uint32_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    while (n--)
        lt_h = (lt_h ^ *b++) * 16777619u;
}
/* what the main loop keeps (not the ISR's side, not the ms OCT- went down), the tracks' settings a COMMIT writes, the DJ FILTER */
static uint32_t lt_state(void)
{
    uint32_t i;
    lt_h = 2166136261u;
    lt_mix(lv.rate, sizeof lv.rate); lt_mix(lv.dn, sizeof lv.dn); lt_mix(lv.rot, sizeof lv.rot); lt_mix(lv.cut, sizeof lv.cut);
    lt_mix(&lv.here, 1); lt_mix(&lv.rnd, 1); lt_mix(lv.tk, sizeof lv.tk); lt_mix(lv.lt, sizeof lv.lt);
    lt_mix(&lv.latch, 1); lt_mix(&lv.tgt, 1); lt_mix(&lv.byp, 1); lt_mix(lv.want, sizeof lv.want); lt_mix(&lv.busy, 1);
    lt_mix(lv.rq, sizeof lv.rq); lt_mix(lv.rk, sizeof lv.rk);
    for (i = 0; i < NTRK; i++) {
        lt_mix(&trk[i].p[P_SDIV], 2); lt_mix(&trk[i].p[P_SLEN], 2); lt_mix(&trk[i].p[P_SOFS], 2); lt_mix(&trk[i].p[P_TCHAN], 2);
    }
    lt_mix(&song.g[G_DJF], 2); lt_mix(&song.octave, 1); lt_mix(&ui.confirm, sizeof ui.confirm);
    return lt_h;
}
static void lt_key(int black, uint32_t p, int down)
{
    uint32_t k = key_at(black, p);
    if (down) { fm1_in.notes |= 1u << k; host_notes |= 1u << k; }
    else fm1_in.notes &= ~(1u << k);
    keyboard_block();
    frame();
}
static void lt_tap(int black, uint32_t p) { lt_key(black, p, 1); lt_key(black, p, 0); }
static void lt_oct_hold(uint32_t ms)                    /* OCT- held ms, let go */
{
    fm1_in.buttons |= 1u << panel.btn[B_OCTDN];
    host_pressed |= 1u << panel.btn[B_OCTDN];
    frames(ms);
    fm1_in.buttons &= ~(1u << panel.btn[B_OCTDN]);
    frame();
    frame();
}
#ifdef LT_OLD
static void lt_live(void) { nav_put(SEC_GLO, "MANO"); frame(); }
static void lt_rails(uint32_t row, int32_t s) { nav_put(SEC_GLO, "RIELES"); frame(); turn(EN_K1 + row, s); }
#else
static void lt_live(void) { nav_put(SEC_GLO, "LIVE"); frame(); }
static void lt_rails(uint32_t row, int32_t s) { uxg_t_rails(row, s); }   /* (tests/ui_test.c: the row picked, KNOB 2) */
#endif
#define LT_N 9
static void lt_run(uint32_t got[LT_N])
{
    ui_power_on();
    memset(&lv, 0, sizeof lv);
    stop_transport();
    frame();
    lt_live();
    lt_key(1, 0, 1); lt_key(1, 2, 1); lt_tap(0, 0); lt_key(1, 0, 0); lt_key(1, 2, 0);   /* black 1 + 3 held, F3 (/2) tapped */
    lt_key(0, 1, 1);                                    /* G3 (x2) held on all */
    got[0] = lt_state();
    lt_key(0, 1, 0);
    lt_tap(1, 6); lt_tap(0, 5); lt_tap(0, 8);           /* LATCH, C4 (LOOP 4), G4 (REV) latched */
    lt_key(1, 1, 1); lt_tap(1, 8); lt_tap(1, 10); lt_key(1, 1, 0);   /* black 2 held: C#5 (TURN back), F#5 (RANDOM) */
    got[1] = lt_state();
    turn(EN_K1, 2); turn(EN_K2, 3); turn(EN_K3, -4); turn(EN_K4, 5);   /* RATE, LOOP, THIN, DJ FILTER on all */
    lt_key(1, 4, 1); turn(EN_K1, -1); turn(EN_K3, 2); lt_key(1, 4, 0);   /* black 5 held: RATE, THIN on T5 */
    got[2] = lt_state();
    lt_tap(1, 6);                                       /* LATCH off; OCT+ (COMMIT asks), OCT+ (writes) */
    press(B_OCTUP); frame();
    got[3] = lt_state();
    press(B_OCTUP); frame();
    got[4] = lt_state();
    turn(EN_K1, 1); lt_oct_hold(480);                   /* OCT- held: BYPASS, let go */
    press(B_OCTDN); frames(64);                         /* OCT- tapped: RETURN */
    got[5] = lt_state();
    lt_rails(0, 3); lt_rails(1, 2); lt_rails(2, 5); lt_rails(3, -3);   /* T3: RATE x2, CUT 8, DENSITY 70 % */
    got[6] = lt_state();
    lt_rails(0, 4); lt_rails(3, 2); lt_rails(1, -1); lt_rails(2, 9);   /* SYNTHS: DENSITY RATCH x3, RATE, CUT */
    got[7] = lt_state();
    press(B_OCTDN); frames(64);                         /* RETURN on the tracks the rows pick; OCT+ asks */
    lt_rails(0, -7); lt_rails(1, 1);
    frame();
    press(B_OCTUP); frame();
    got[8] = lt_state();
    press(B_OCTDN); frame();                            /* (the dialog's OCT-: no) */
}
