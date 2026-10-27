/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* GLO > LIVE (0.9.11): the six sequences side by side and their live tricks (seq_live.c). (ACID FM 3c-1 task 6) the page is
 * ux_glo.c's (its picture, knob row, status line); here what it does, as before. Inherited from the mock-up secuencias.html's
 * two pages (MANO and RIELES, their first names): the first is LIVE, the second's knobs GLO > MORE's RAILS rows (params.c
 * UXM_GLO: live_rail).
 *   LIVE   the keys are a board: the black keys 1..6 (F#3 .. F#4) pick the tracks while held (several at once; none, or
 *          A#4: ALL), the white keys are the tricks while held: F3 /2, G3 x2, A3 TR (3/2), B3 DT (2/3), C4 .. F4 LOOP
 *          8 4 2 1 (from where the playhead is; the shortest held wins), G4 REV, A4 PING, B4 C5 RATCH x2 x3, D5 THIN 50 %,
 *          E5 F5 OCT+ OCT-, G5 RETURN. G#4 LATCH: a white key then turns its trick on / off (for the tracks held then).
 *          C#5 D#5 TURN -1 / +1 (a step back / on), F#5 RANDOM (the steps in any order) on / off. The keys play no note here
 *          (seq.c keyboard_block, song.grid 3); off the page they play again (a key still held keeps its trick until
 *          it is let go). KNOB 1 RATE, 2 LOOP, 3 THIN on the tracks held (none: all), 4 the DJ FILTER (MIXER's KNOB 4).
 *   RAILS  (GLO > MORE, the rows under LIVE) TRACKS: ALL, T1 .. T6, SYNTHS (the synths), DRUMS (the DRUM tracks); RATE
 *          /4 .. x4; CUT (the first N steps); DENSITY (left THIN 90 .. 10 %, the middle all, right RATCH x2 .. x4).
 * How they combine: one state per track. LIVE's knobs and the RAILS rows are its LATCHED layer (RATE is the same value on both;
 * LIVE's LOOP and RAILS' CUT are the one window, LOOP from the playhead, CUT from step 1, the last turned wins; THIN and DENSITY
 * are the one density); LIVE's keys sit on top while held or latched: a rate key replaces the rate, a LOOP key shortens the
 * window (the shortest wins), THIN takes the thinner, RATCH the larger, REV / PING / OCT are the keys' alone. On LIVE and on
 * a RAILS row OCT- tapped is RETURN (the tracks picked: everything back, in phase, on the next beat), OCT- held BYPASS (the
 * original while held, every track), OCT+ KEEP (asks; OCT+ again writes RATE into DIV, else STRETCH; CUT / LOOP into LEN, a
 * LOOP's steps to the start; TURN into ROTATE; THIN into CHANCE; REV, PING, RANDOM, RATCH, OCT and a rate with no DIV or STRETCH
 * stay live; UNDO: GLO + G4). Never saved; STOP keeps them. Included by ui_layer.c. */
static uint32_t tl_len(const track_t *t);
static void tl_take(const track_t *t);
static uint32_t tl_stretch(track_t *t, int32_t k);
static void tracks_edit(uint32_t slot, int32_t steps);
#define LV_ALL ((1u << NTRK) - 1u)
static const int8_t LV_KRATE[4] = {-2, 2, 1, -1};       /* F3 .. B3: /2 x2 3/2 2/3 */

/* LIVE shown, or (3c-1 task 6) GLO > MORE with a RAILS row picked (ux_glo.c): OCT- RETURN / BYPASS and OCT+ KEEP there */
static int live_page(void) { return !ui.home && !ui.menu && (cur_page()->graph == GR_LIVE || uxg_more_live()); }
static int lv_isman(void) { return cur_page()->graph == GR_LIVE; }   /* (LIVE itself: the keys its board) */
static int live_mano(void) { return live_page() && !ui.confirm && lv_isman(); }
/* the tracks LIVE's knobs act on: the black keys held (none, or A#4: all) */
static uint32_t lv_key_tracks(void)
{
    uint32_t m = 0, k, i;
    for (k = 0; k < 27u; k++)
        if (((lv.kb >> k) & 1u) && key_black(k) && (i = key_place(k)) <= 7u)
            m |= i == 7u ? LV_ALL : i < NTRK ? 1u << i : 0u;
    return m ? m : LV_ALL;
}
/* .. the RAILS rows: their TRACKS (ALL, T1 .. T6, SYNTHS, DRUMS) */
static uint32_t lv_rail_tracks(void)
{
    uint32_t m = 0, i;
    if (lv.tgt - 1u < NTRK)
        return 1u << (lv.tgt - 1u);
    for (i = 0; i < NTRK && lv.tgt; i++)
        m |= (uint32_t)(drum_track(&trk[i]) == (lv.tgt == 8u)) << i;
    return lv.tgt ? m : LV_ALL;
}
/* the tracks the knobs, RETURN and KEEP act on: on LIVE the keys', elsewhere (a RAILS row) the TRACKS row's */
static uint32_t lv_tracks(void) { return lv_isman() ? lv_key_tracks() : lv_rail_tracks(); }
static void lv_req(uint32_t m, uint32_t kind)           /* a request to the ISR for tracks m (seq_live.c live_q) */
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        if ((m >> i) & 1u) {
            lv.rk[i] = (uint8_t)kind;
            lv.rq[i]++;
        }
}
static void lv_keys_off(uint32_t m, uint32_t from)      /* LIVE's white keys from `from` on: not on tracks m (PING kept) */
{
    for (; from < 15u; from++)
        if (from != 9u) {
            lv.tk[from] &= (uint8_t)~m;
            lv.lt[from] &= (uint8_t)~m;
        }
}
/* what each track should have now (the latched layer, the keys over it) for the ISR */
static void live_publish(void)
{
    uint32_t i, k, busy = 0, rk = 0;
    for (i = 0; i < NTRK; i++) {
        lv_mod_t m = {0};
        uint32_t bit = 1u << i, d;
        int32_t dn = lv.dn[i];
        if (!lv.byp) {
            m.rate = lv.rate[i];
            m.cut = LV_CUT[lv.cut[i] % LV_NCUT];
            m.fl = (uint8_t)(((lv.here >> i) & 1u) * LF_HERE | ((lv.rnd >> i) & 1u) * (LD_RND << 1));
            m.drop = (uint8_t)(dn < 0 ? -10 * dn : 0);
            m.rat = (uint8_t)(dn > 0 ? 1 + dn : 0);
            m.rot = lv.rot[i];
            for (k = 0; k < 15u; k++) {                 /* the keys over it (a rate: the last on the map) */
                if (!(((lv.tk[k] | lv.lt[k]) >> i) & 1u))
                    continue;
                if (k < 4u) {
                    m.rate = LV_KRATE[k];
                    rk |= bit;
                } else if (k < 8u) {                    /* LOOP 8 4 2 1: the shortest */
                    d = 8u >> (k - 4u);
                    m.cut = (uint8_t)(m.cut && (m.fl & LF_HERE) && m.cut < d ? m.cut : d);
                    m.fl |= LF_HERE;
                } else if (k < 10u) {
                    m.fl = (uint8_t)((m.fl & ~6u) | (k - 7u) << 1);   /* REV, PING */
                } else if (k < 12u) {
                    if (m.rat < k - 8u)
                        m.rat = (uint8_t)(k - 8u);      /* RATCH x2, x3 */
                } else if (k == 12u) {
                    if (m.drop < 50u)
                        m.drop = 50;
                } else {
                    m.fl |= k == 13u ? LF_UP : LF_DN;
                }
            }
            if (perf_m[0]) {                            /* (0.9.14) the FX layer's STEPS: 1/2 1/4 1/8 of it, 1 step */
                k = (perf_m[0] + 2u) / 3u;
                d = k < 4u ? (uint32_t)clamp(trk[i].p[P_SLEN], 1, NSTEP) >> k : 1u;
                m.cut = (uint8_t)(d ? d : 1u);          /* (over LIVE's own window) */
                m.fl |= LF_HERE;
            }
            if (m.cut && (m.fl & LF_HERE)) {            /* a LOOP: the N steps the playhead is in when it is taken */
                d = (uint32_t)clamp(trk[i].p[P_SLEN], 1, NSTEP);
                m.ws = lv.want[i].ws;
                if (m.cut != lv.want[i].cut || !(lv.want[i].fl & LF_HERE)) {
                    k = trk[i].seq_idx % d;
                    k -= k % m.cut;
                    m.ws = (uint8_t)(m.cut >= d ? 0u : k + m.cut > d ? d - m.cut : k);
                }
            }
            if (m.rat > 1u) {                           /* RATCH: no part shorter than LV_MINPART (~1.4 ms; the step's
                                                         * shortest swing: 0.6) */
                d = div_samples((uint32_t)trk[i].p[P_SDIV]) * LV_RD[m.rate + LV_X1] / LV_RN[m.rate + LV_X1] * 3u /
                    (5u * LV_MINPART);
                if (m.rat > d)
                    m.rat = (uint8_t)(d > 1u ? d : 0u);
            }
        }
        lv.want[i] = m;
        busy |= !lv_neutral(&m) || !lv_neutral(&lv.cur[i]) || lv.rq[i] != lv.rs[i];
    }
    lv.r16 = (uint8_t)(rk | lv.rkey);                    /* (a rate key pressed or let go: on the 1/16) */
    lv.rkey = (uint8_t)rk;
    lv.busy = (uint8_t)busy;
}
/* RETURN: tracks m back to nothing (both pages' knobs, the keys on them), on the next beat */
static void live_return(uint32_t m)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        if ((m >> i) & 1u)
            lv.rate[i] = lv.dn[i] = lv.rot[i] = (int8_t)(lv.cut[i] = 0);
    lv.here &= (uint8_t)~m;
    lv.rnd &= (uint8_t)~m;
    lv_keys_off(m, 0);
    lv.tk[9] &= (uint8_t)~m;
    lv.lt[9] &= (uint8_t)~m;
    live_publish();
    lv_req(m, LQ_BEAT);
}
/* COMMIT (the dialog's OCT+, UNDO marked): what tracks m play now into their own parameters, at once; then, the
 * interrupts held off, the playhead where the written pattern plays what sounded (a LOOP's steps moved to the start,
 * ROTATE moved, a step's samples at the new DIV), its ghost there too */
static __attribute__((noinline)) void live_commit(uint32_t m)
{
    static const uint16_t DL[10] = {48, 24, 12, 6, 16, 8, 96, 192, 384, 768};   /* N_DIV's steps, 48ths of a beat */
    uint32_t i, k, f;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        lv_mod_t w = lv.want[i];
        uint32_t len = tl_len(t), idx = t->seq_idx, pos = t->seq_pos, ws = 0, rn = LV_RN[w.rate + LV_X1];
        uint32_t rd = LV_RD[w.rate + LV_X1];
        if (!((m >> i) & 1u))
            continue;
        if (w.rot) {
            t->p[P_SOFS] = (int16_t)clamp((t->p[P_SOFS] - w.rot) % (int32_t)len, -32, 32);
            idx = (uint32_t)((int32_t)(idx % len) - w.rot % (int32_t)len + 2 * (int32_t)len);   /* (mod len below) */
        }
        if (w.cut && w.cut < len) {
            ws = (w.fl & LF_HERE) ? w.ws : 0u;
            if (ws + w.cut > len)
                ws = len - w.cut;
            for (k = 0; k < w.cut; k++)                 /* (the LOOP's steps to the start) */
                t->step[k] = t->step[k + ws];
            t->p[P_SLEN] = (int16_t)w.cut;
        }
        if (w.rate) {                                   /* DIV, else STRETCH, else it stays live */
            uint32_t to = DL[t->p[P_SDIV] % 10] * rd;
            for (k = 0; k < 10u && (to % rn || DL[k] != to / rn); k++) {}
            if (k < 10u)
                t->p[P_SDIV] = (int16_t)k;
            else if (rn == 1u || rd == 1u) {
                tl_take(t);
                tl_stretch(t, rn == 1u ? (int32_t)rd : -(int32_t)rn);
            }
            if (k < 10u || rn == 1u || rd == 1u) {
                pos = pos * rd / rn;
                lv.rate[i] = 0;
                lv_keys_off(1u << i, 0);
            }
        }
        if (w.drop)
            t->p[P_TCHAN] = (int16_t)(127 - (127 - clamp(t->p[P_TCHAN], 0, 127)) * (100 - w.drop) / 100);
        lv.cut[i] = lv.dn[i] = lv.rot[i] = 0;
        lv.here &= (uint8_t)~(1u << i);
        lv_keys_off(1u << i, 4);
        live_publish();
        f = motion_guard();
        if (LV_ON(i) && song.playing && t->seq_pos < 0x7FFFFFFFu) {
            len = tl_len(t);
            lv.gidx[i] = t->seq_idx = (uint16_t)((idx - (idx >= ws ? ws : 0u)) % len);
            lv.gpos[i] = t->seq_pos = pos;
            t->jump_to = 0;
        }
        lv.cur[i] = lv.want[i];
        lv.rj[i] = 0;
        if (lv_neutral(&lv.cur[i]))
            lv.act &= (uint8_t)~(1u << i);
        motion_unguard(f);
    }
    ui_message("WRITTEN: GLO+G4 UNDO");
}
/* each frame: LIVE's keys (seq.c keyboard_block holds them in lv.kb), OCT- / OCT+ on LIVE or a RAILS row (OCT- held BYPASS,
 * tapped RETURN; OCT+ the COMMIT dialog), then what goes to the ISR */
static __attribute__((noinline)) void live_frame(void)
{
    uint32_t kb = lv.kb, ch = kb ^ lv.kprev, tg = lv_tracks(), k, i, any = 0;
    uint32_t ob = ((fm1_in.buttons >> panel.btn[B_OCTDN]) & 1u) | ((fm1_in.buttons >> panel.btn[B_OCTUP]) & 1u) << 1;
    lv.kprev = kb;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k), dn = (kb >> k) & 1u;
        if (!((ch >> k) & 1u))
            continue;
        if (key_black(k)) {
            if (!dn)
                continue;
            if (p == 6u)
                lv.latch ^= 1u;
            else if (p == 8u || p == 9u) {
                for (i = 0; i < NTRK; i++)
                    if ((tg >> i) & 1u)
                        lv.rot[i] = (int8_t)clamp(lv.rot[i] + (p == 8u ? -1 : 1), -16, 16);
            } else if (p == 10u)
                lv.rnd ^= (uint8_t)tg;
        } else if (p == 15u) {
            if (dn)
                live_return(tg);
        } else if (!dn) {
            lv.tk[p] = 0;
        } else if (lv.latch) {
            lv.lt[p] = (uint8_t)(lv.lt[p] ? 0u : tg);
        } else {
            lv.tk[p] = (uint8_t)tg;
        }
    }
    if (live_page() && !ui.confirm && ob != 3u) {
        if ((ob & ~lv.ob) & 1u)
            lv.oms = fm1_ms | 1u;
        if ((ob & ~lv.ob) & 2u) {
            for (i = 0; i < NTRK; i++)
                any |= ((tg >> i) & 1u) && !lv_neutral(&lv.want[i]);
            if (!any || chain_busy())
                ui_message(any ? "STOP TO EDIT" : "NOTHING TO KEEP");
            else
                confirm_open(CF_LIVE_COMMIT, tg);
        }
    }
    lv.ob = (uint8_t)ob;
    if (lv.oms && (ob & 1u) && !lv.byp && (int32_t)(fm1_ms - lv.oms) > 350) {
        lv.byp = 1;
        lv_req(LV_ALL, LQ_NOW);
        ui_message("BYPASS: THE ORIGINAL");
    } else if (lv.oms && !(ob & 1u)) {
        if (lv.byp)
            lv_req(LV_ALL, LQ_NOW);
        else if (live_page())
            live_return(tg);
        lv.byp = 0;
        lv.oms = 0;
    }
    live_publish();
}
/* the latched layer's setting slot (0 RATE, 1 LOOP / CUT, 2 THIN / DENSITY) turned steps detents on tracks tg, the first one's
 * value to every one: live (LIVE's knobs: a LOOP from the playhead, THIN alone) or not (the RAILS rows: CUT from step 1, DENSITY
 * THIN .. RATCH) */
static void lv_set(int live, uint32_t tg, uint32_t slot, int32_t steps)
{
    uint32_t f = tg ? (uint32_t)__builtin_ctz(tg) : 0u, i;
    int32_t d = clamp(steps, -12, 12), v;
    v = slot == 0u ? clamp(lv.rate[f] + d, -4, 4) : slot == 1u ? clamp((int32_t)lv.cut[f] + d, 0, LV_NCUT - 1) :
        live ? clamp((lv.dn[f] < 0 ? lv.dn[f] : 0) - d, -9, 0) : clamp(lv.dn[f] + d, -9, 3);
    for (i = 0; i < NTRK; i++) {
        if (!((tg >> i) & 1u))
            continue;
        if (slot == 0u) {
            lv.rate[i] = (int8_t)v;
        } else if (slot == 1u) {
            lv.cut[i] = (uint8_t)v;
            lv.here = (uint8_t)(live ? lv.here | 1u << i : lv.here & ~(1u << i));
        } else {
            lv.dn[i] = (int8_t)v;
        }
    }
}
/* KNOB slot turned on LIVE (edit_param): RATE, LOOP, THIN on the tracks held, the DJ FILTER as MIXER's KNOB 4 */
static __attribute__((noinline)) void live_knob(uint32_t slot, int32_t steps)
{
    if (slot == 3u)
        tracks_edit(3, steps);
    else
        lv_set(1, lv_key_tracks(), slot, steps);
}
/* (3c-1 task 6) a RAILS row (params.c ID_LV_*) turned steps detents (ui_input.c param_knob): TRACKS, else RATE CUT DENSITY on them */
static __attribute__((noinline)) void live_rail(uint32_t id, int32_t steps)
{
    if (id == ID_LV_TGT)
        lv.tgt = (uint8_t)clamp((int32_t)lv.tgt + clamp(steps, -12, 12), 0, 8);
    else
        lv_set(0, lv_rail_tracks(), id - ID_LV_RATE, steps);
}
/* LIVE's key LEDs: the keys held, the white keys latched, LATCH on */
static __attribute__((noinline)) uint32_t live_leds(void)
{
    uint32_t m = lv.kb, k;
    for (k = 0; k < 27u; k++)
        if (key_black(k) ? key_place(k) == 6u && lv.latch : key_place(k) < 15u && lv.lt[key_place(k)])
            m |= 1u << k;
    return m;
}
