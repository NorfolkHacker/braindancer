/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker
 * JIANT (0.6.91) the song generator (firmware/src/gen.c) measured: every genre at several seeds and settings, on the
 * host build of the UI tests. What a piece must always have: the same piece from the same settings (deterministic);
 * one scale on every melodic track, every note in it (the bass's chromatic approaches excepted) and in its register;
 * drums on one track with the kick anchored (four-on-the-floor genres: step 1), its density growing with DRUMS;
 * the bass's first note at a chord change the root (most of the time: FiloBass' 68 %); the lead's strong steps on chord
 * tones more often than not; chords of three or four, voiced in C3..G5, led closely; a storable project; a form in
 * ARRANGE when asked; the parts' knobs only touching their own part. (0.8.6) EXPLORE, the default: no genre, every
 * rule broken, every piece still playing in its key, very different from the next.
 *   cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/gen_test tests/gen_test.c -lm */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int gcheck(const char *what, int ok)
{
    printf("gen: %-74s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}
static uint8_t gt_cpt;                                   /* (0.9.14) the CPU TEST result the pieces are made for (gen.c gc_dev) */
static uint32_t gt_dly_stage(void)                     /* (ACID FM 0.5.2) the rate the piece's delay line runs at (fx.c
                                                         * fx_buses, dly_rate): 0 full, 1 half, 2 a quarter; the full
                                                         * rate whatever the line ran at before (dly_stage's hysteresis) */
{
    uint32_t g = (uint32_t)song.g[G_DTIME], wd = (uint32_t)song.g[G_WIDTH], dl;
    int png = song.g[G_DPING] == 1 && !song.g[G_DPIT] && !song.g[G_DSPRY];
    dl = g < DT_FREE ? div_samples(g) : (uint32_t)DTIME_MS[(g - DT_FREE) % NELEM(DTIME_MS)] * 441u / 10u;
    dl = dl < 16u ? 16u : dl > DLY_MAX ? DLY_MAX : dl;
    return dly_stage(png ? dl * 2u : dl + (wd && !png ? div_samples(3) * wd / 127u : 0u), 2u);
}
static int form_drop_longest(void)                      /* (0.9.14) the drop (A) rows longer than every other row */
{
    uint32_t k, a = 0, o = 0;
    for (k = 0; k < chain_config.count; k++) {
        uint32_t b = chain_config.row[k].bars;
        if (chain_config.row[k].slot == 0u) { if (b > a) a = b; } else if (b > o) o = b;
    }
    return a > o;
}
static uint32_t form_sig(void)                          /* the ARRANGE's rows: slots and bars */
{
    uint32_t k, h = chain_config.count;
    for (k = 0; k < chain_config.count; k++) h = h * 131u + chain_config.row[k].slot * 7u + chain_config.row[k].bars;
    return h;
}
static void gen_reset(void)
{
    ui_power_on();
    ui_rec_prefs |= PREF_SIX >> 8; rp_apply();          /* TRACKS 6 */
    cpt_res = gt_cpt; cpt_cores = gt_cpt ? 2u : 0u;
    settings.palette = UI_ACID_INDEX; palette_set(UI_ACID_INDEX);
}
static void gen_with(uint32_t genre, uint32_t seed, uint32_t energy, uint32_t form)
{
    uint32_t k;
    gp.genre = (uint8_t)genre; gp.seed = (uint16_t)seed; gp.energy = (uint8_t)energy; gp.form = (uint8_t)form;
    for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
    gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
    gen_go();
}
static uint32_t all_sig(void)
{
    uint32_t k, h = 2166136261u ^ (uint32_t)song.g[G_BPM];
    for (k = 0; k < NTRK; k++) h = (h ^ steps_sig(&trk[k]) ^ trk[k].eng_req ^ (uint32_t)trk[k].p[P_E4] * 7u) * 16777619u;
    return h;
}
static int in_scale(const track_t *t, int32_t n)
{
    uint32_t mask = scale_mask(t);
    return (mask >> (uint32_t)((n - t->p[P_ROOT] + 120) % 12)) & 1u;
}
static uint32_t hits_of(const track_t *t, uint32_t lane)
{
    uint32_t i, c = 0;
    for (i = 0; i < (uint32_t)t->p[P_SLEN]; i++) c += t->step[i].time == ST_NOTE && ((t->step[i].hit >> lane) & 1u);
    return c;
}
/* (0.8.6) the piece heard for a while: its RMS and peak (the output before the DAC's clamp), the blocks at full scale,
 * the most voices sounding at once over the synth parts */
static void gx_listen(uint32_t blocks, double *rms, int32_t *peak, uint32_t *clip, uint32_t *vmax)
{
    uint32_t b, i, p;
    double acc = 0;
    int32_t pk = 0;
    uint32_t cl = 0, vm = 0;
    song.master_q12 = 4096;
    transport_req = 1;
    for (b = 0; b < blocks; b++) {
        int32_t o[2 * CTL];
        uint32_t nv = 0;
        if (b % 32u == 0u) { ui_input(); song_poll(); host_ticks += 20000u; fm1_ms += 20u; }
        memset(o, 0, sizeof o);
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++) {
            int32_t v = o[i] < 0 ? -o[i] : o[i];
            if (v > pk) pk = v;
            cl += v >= 32767;
            acc += (double)o[i] * (double)o[i];
        }
        for (p = 0; p < NPART; p++)                     /* (as voice.c voices_busy counts them: not the fading) */
            for (i = 0; i < NVOICE; i++) nv += trk[p].v[i].active && trk[p].v[i].stage != 4u;
        if (nv > vm) vm = nv;
    }
    transport_req = 2;
    { int32_t o[2 * CTL]; ui_input(); mix_block(o, CTL); }
    *rms = sqrt(acc / (double)(blocks * 2u * CTL)); *peak = pk; *clip = cl; *vmax = vm;
}
static uint32_t hits_all(const track_t *t)
{
    uint32_t i, c = 0, l;
    for (i = 0; i < (uint32_t)t->p[P_SLEN]; i++)
        if (t->step[i].time == ST_NOTE) for (l = 0; l < 8u; l++) c += (t->step[i].hit >> l) & 1u;
    return c;
}

#define GEN_HOUSE_HASH 2027450874u   /* (all_sig of HOUSE 0421, energy 50, captured on the base commit a4fcac7) */
int main(void)
{
    int bad = 0;
    uint32_t g, s, k, i;
    uint32_t notes_out = 0, notes_all = 0, chords_bad = 0, lead_strong = 0, lead_ct = 0, bass_changes = 0, bass_root = 0;
    uint32_t drums_many = 0, pieces = 0, kick_anchor_bad = 0, store_bad = 0, range_bad = 0, motion_bad = 0;
    uint32_t genre0 = gp.genre;                         /* (0.8.6: the power-on's GENRE, before any test turns it) */
    gen_reset();
    /* ---- deterministic: the same settings twice, the same piece; another seed, another piece */
    {
        uint32_t h1, h2, h3;
        gen_with(1, 421, 50, 0); h1 = all_sig();
        gen_with(5, 7, 80, 0);
        gen_with(1, 421, 50, 0); h2 = all_sig();
        gen_with(1, 422, 50, 0); h3 = all_sig();
        bad += gcheck("deterministic: HOUSE 0421 twice the same piece, 0422 another", h1 == h2 && h1 != h3);
    }
    /* ---- every genre, three seeds, three energies: the guard rails */
    for (g = 0; g < NGENRE; g++) {
        for (s = 0; s < 3u; s++) {
            uint32_t dr = NTRK, bs = NTRK, ld = NTRK, pd = NTRK, drums = 0;
            gen_with(g + 1u, 100u * g + 17u * s + 3u, s == 0u ? 20u : s == 1u ? 50u : 85u, 0);
            pieces++;
            for (k = 0; k < NTRK; k++) {
                if (drum_track(&trk[k])) { drums++; dr = k; }
                else if (gs.role[k] == RL_BASS && bs == NTRK) bs = k;
                else if (gs.role[k] == RL_LEAD && ld == NTRK) ld = k;
                else if (gs.role[k] == RL_PAD && pd == NTRK) pd = k;
            }
            if (drums != 1u) drums_many++;
            for (k = 0; k < NTRK; k++) {
                const track_t *t = &trk[k];
                uint32_t len = (uint32_t)t->p[P_SLEN];
                if (k == dr || len > NSTEP) continue;
                if (t->p[P_ROOT] != trk[0].p[P_ROOT] || t->p[P_SCALE] != trk[0].p[P_SCALE]) range_bad++;
                for (i = 0; i < len; i++) {
                    const step_t *st = &t->step[i];
                    uint32_t j;
                    if (st->time != ST_NOTE) continue;
                    if (st->n < 1u || st->n > 4u) chords_bad++;
                    for (j = 0; j < st->n; j++) {
                        notes_all++;
                        if (!in_scale(t, st->note[j])) notes_out++;
                        if (st->note[j] > 84u || st->note[j] < 20u) { range_bad++; printf("  range: %s T%u role %u step %u note %u\n", gs.g->name, k + 1u, gs.role[k], i, st->note[j]); }
                    }
                    if (k == pd && st->n >= 3u && (st->note[0] < 46u || st->note[st->n - 1u] > 81u)) { chords_bad++; printf("  chord range: %s seed %u step %u %u..%u\n", gs.g->name, s, i, st->note[0], st->note[st->n - 1u]); }
                }
            }
            if (dr < NTRK && gs.act[RL_DRUM]) {         /* the kick anchored on the one where the genre has it */
                uint32_t bd = DRUM_MAP_I[gs.g->drum][0];   /* (0.6.95: the map as this throw plays it; 0.9.7 packed) */
                if (bd && gm_level(bd, 0) == 255u && !(trk[dr].step[0].time == ST_NOTE && (trk[dr].step[0].hit & 1u))) { kick_anchor_bad++; printf("  kick anchor: %s seed %u map %u\n", gs.g->name, s, gs.g->drum); }
            }
            if (bs < NTRK && gs.act[RL_BASS] && gs.ba != BA_ACID) {   /* the root at the chord changes */
                const track_t *b = &trk[bs];
                uint32_t len = (uint32_t)b->p[P_SLEN], at;
                for (at = 0; at < len; at++) {
                    uint32_t j;
                    int32_t root = (int32_t)gchord_root_pc(at);
                    if (!gchord_starts(at)) continue;
                    for (j = at; j < at + 4u && j < len; j++)
                        if (b->step[j].time == ST_NOTE) {
                            bass_changes++;
                            if (b->step[j].note[0] % 12u == (uint32_t)root) bass_root++;
                            break;
                        }
                }
            }
            if (ld < NTRK && gs.act[RL_LEAD]) {         /* the lead's strong steps on chord tones */
                const track_t *l = &trk[ld];
                uint32_t len = (uint32_t)l->p[P_SLEN];
                for (i = 0; i < len; i += 4u) {
                    const step_t *st = &l->step[i];
                    if (st->time != ST_NOTE) continue;
                    lead_strong++;
                    if ((gchord_pcs(i) >> (st->note[0] % 12u)) & 1u) lead_ct++;
                }
            }
            project_capture(&proj_scratch);
            if (!proj_pack(&proj_wire, &proj_scratch)) store_bad++;
            if (motion.count > MOTION_MAX) motion_bad++;
            for (i = 0; i < 16u; i++) events_block(CTL);   /* (it plays: no crash) */
        }
    }
    printf("gen: %u pieces, %u notes, %u off the scale (%u %%), bass root at %u of %u changes, lead chord tones %u of %u strong\n",
           pieces, notes_all, notes_out, notes_all ? notes_out * 100u / notes_all : 0u, bass_root, bass_changes, lead_ct, lead_strong);
    bad += gcheck("every genre x 3: one DRUM track, one scale, every note in it but the approaches (< 4 %), in its register",
                  !drums_many && !range_bad && notes_out * 100u < notes_all * 4u);
    bad += gcheck("  chords of 1..4 notes, the pads' in C3..G5; the kick on the one where the map anchors it", !chords_bad && !kick_anchor_bad);
    bad += gcheck("  the bass's first note at a chord change the root, 70 % at least (FiloBass: 68 %)", bass_root * 10u >= bass_changes * 7u);
    bad += gcheck("  the lead's strong steps on chord tones, 55 % at least", lead_ct * 100u >= lead_strong * 55u);
    bad += gcheck("  every piece storable, the automation inside its 64", !store_bad && !motion_bad);
    /* ---- (0.8.3) the music: twelve seeds of every genre at varied ENERGY and COMPLEX, measured as a musician would
     * listen: every note in the key the SCALE page shows, but the chromaticism the rules allow (a semitone approach off
     * beats 1 and 3 that resolves, a borrowed / secondary / raised chord's own tones where the genre colours) and few
     * of those; the SCALE page a mode of seven, QUANT on, the CHORD page on the chord track (an in-scale chord type,
     * VOICING LEAD) and every chord it builds in the key; the lead's beats on chord tones, its motion mostly by steps,
     * never past an octave, a leap past a fourth turned back by a step, its phrases closing on a stable degree, its
     * first bar's rhythm coming back; the chords led closely; the bass on the root at every chord change */
    {
        uint32_t nn = 0, out_appr = 0, out_ct = 0, out_bad = 0, page_bad = 0, chord_bad = 0;
        uint32_t ls = 0, lct = 0, liv = 0, lrep = 0, lstep = 0, loct = 0, l5 = 0, l5r = 0, lend = 0, lend_ok = 0, lbars = 0, lbars_r = 0;
        uint32_t pmov = 0, pmot = 0, bch = 0, broot = 0;
        for (g = 0; g < NGENRE; g++)
            for (s = 0; s < 12u; s++) {
                uint32_t pd;
                gp.genre = (uint8_t)(g + 1u); gp.seed = (uint16_t)(2000u + 41u * s + g); gp.energy = (uint8_t)(15u + (s * 23u) % 80u); gp.form = 0;
                for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
                gp.complex = (uint8_t)(10u + (s * 31u) % 85u); gp.swing = GA_AUTO; gp.human = 30;
                gen_go();
                for (k = 0; k < NTRK; k++) {            /* the SCALE page */
                    const track_t *t = &trk[k];
                    uint32_t c = 0, b;
                    for (b = 0; b < 12u; b++) c += (scale_mask(t) >> b) & 1u;
                    if (t->p[P_ROOT] != gs.root || t->p[P_SCALE] != gs.scale || c != 7u || t->p[P_QUANT] == QN_OFF) page_bad++;
                }
                pd = gs.trk_of[RL_PAD];
                if (pd < NTRK && gs.act[RL_PAD]) {      /* the CHORD page: an in-scale type, LEAD, every chord in the key */
                    track_t *t = &trk[pd];
                    uint8_t o[KB_CH];
                    uint32_t key, c, j;
                    if (t->p[P_CHRD] < CD_TRI || t->p[P_CHRD] > CD_SUS4 || t->p[P_CVOI] != CV_LEAD) page_bad++;
                    for (key = 53; key < 80u; key++)
                        for (c = chord_build(t, key, o), j = 0; j < c; j++) if (!in_scale(t, o[j])) chord_bad++;
                }
                for (k = 0; k < NTRK; k++) {
                    const track_t *t = &trk[k];
                    uint32_t r = gs.role[k], len = dlen(t);
                    if (drum_track(t) || !gs.act[r]) continue;
                    for (i = 0; i < len; i++) {         /* every note in the key, or chromatic by a rule */
                        const step_t *st = &t->step[i];
                        uint32_t j;
                        if (st->time != ST_NOTE) continue;
                        for (j = 0; j < st->n; j++) {
                            int32_t n = st->note[j], nx = -1;
                            uint32_t q;
                            const gch_t *c = gchord(i);
                            nn++;
                            if (in_scale(t, n)) continue;
                            for (q = 1; q <= len && nx < 0; q++) if (t->step[(i + q) % len].time == ST_NOTE) nx = t->step[(i + q) % len].note[0];
                            if (c->alt && ((gchord_pcs(i) >> (uint32_t)(n % 12)) & 1u) &&
                                (gs.hx & (HX_RAISE | HX_SEC | HX_BOR) || HXF(gs.hx) == HF_JAZZ)) out_ct++;
                            else if (st->n == 1u && i % 8u && nx >= 0 && (nx - n == 1 || n - nx == 1)) out_appr++;
                            else { out_bad++; if (out_bad < 6u) printf("  off the key: %s seed %u T%u step %u note %d\n", gs.g->name, gp.seed, k + 1u, i, n); }
                        }
                    }
                    if (r == RL_LEAD) {
                        int32_t prev = -1, piv = 0, last = -1;
                        uint32_t pend = 0;
                        uint16_t bar0 = 0;
                        for (i = 0; i < len; i++) {
                            const step_t *st = &t->step[i];
                            int32_t n, iv, a2;
                            if (st->time != ST_NOTE || !st->n) continue;
                            n = st->note[0];
                            if (i % 4u == 0u) { ls++; lct += (gchord_pcs(i) >> (uint32_t)(n % 12)) & 1u; }
                            if (i < 16u) bar0 |= (uint16_t)(1u << i);
                            if (prev >= 0) {
                                iv = n - prev; a2 = iv < 0 ? -iv : iv;
                                liv++; lrep += !a2; lstep += a2 && a2 <= 2; loct += a2 > 12;
                                if (pend) { l5++; l5r += a2 && a2 <= 2 && (iv > 0) != (piv > 0); pend = 0; }
                                if (a2 > 5) { pend = 1; piv = iv; }
                            }
                            prev = n; last = (int32_t)i;
                        }
                        if (last >= 0 && gs.g->lead != LD_OSTINATO && gs.g->lead != LD_MECH) {   /* (the phrase's close) */
                            int32_t n = t->step[last].note[0], d = ((n - gs.root) % 12 + 12) % 12;
                            lend++;
                            lend_ok += ((gchord_pcs((uint32_t)last) >> (uint32_t)(n % 12)) & 1u) && (d == 0 || d == 3 || d == 4 || d == 7);
                        }
                        if (gs.g->lead == LD_MOTIF || gs.g->lead == LD_HOOK)
                            for (i = 16; i + 16u <= len; i += 16u) {   /* (the motif's rhythm back: two onsets moved at most) */
                                uint16_t bb = 0;
                                uint32_t q, d = 0;
                                for (q = 0; q < 16u; q++) if (t->step[i + q].time == ST_NOTE && t->step[i + q].n) bb |= (uint16_t)(1u << q);
                                if (!bb) continue;
                                for (q = 0; q < 16u; q++) d += ((bb ^ bar0) >> q) & 1u;
                                lbars++; lbars_r += d <= 2u;
                            }
                    }
                    if (r == RL_PAD && gs.vm != VM_PLANE) {   /* (the chords led: semitones moved, voice by voice) */
                        int32_t pv[4];
                        uint32_t pn = 0, c, j;
                        for (i = 0; i < len; i++) {
                            const step_t *st = &t->step[i];
                            int same = 1, mv = 0;
                            if (st->time != ST_NOTE || st->n < 2u) continue;
                            c = st->n;
                            for (j = 0; j < c && j < pn; j++) if (st->note[j] != pv[j]) same = 0;
                            if (pn && (!same || c != pn)) {
                                for (j = 0; j < c && j < pn; j++) mv += st->note[j] > pv[j] ? st->note[j] - pv[j] : pv[j] - st->note[j];
                                pmov++; pmot += (uint32_t)mv;
                            }
                            for (j = 0; j < c; j++) pv[j] = st->note[j];
                            pn = c;
                        }
                    }
                    if (r == RL_BASS)                   /* (the root at each chord change, within its beat) */
                        for (i = 0; i < len; i++) {
                            uint32_t j;
                            if (!gchord_starts(i)) continue;
                            for (j = i; j < i + 4u && j < len; j++)
                                if (t->step[j].time == ST_NOTE && t->step[j].n) {
                                    bch++; broot += t->step[j].note[0] % 12u == gchord_root_pc(i);
                                    break;
                                }
                        }
                }
            }
        printf("gen: music: %u notes: %u approaches, %u borrowed/secondary/raised chord tones, %u off the rules\n", nn, out_appr, out_ct, out_bad);
        printf("gen: music: lead beats on chord tones %u of %u, steps %u of %u moving intervals, %u past an octave, leaps back %u of %u\n",
               lct, ls, lstep, liv - lrep, loct, l5r, l5);
        printf("gen: music: phrases closing stable %u of %u, the motif's rhythm back in %u of %u bars, chords moved %u.%u semitones, bass roots %u of %u\n",
               lend_ok, lend, lbars_r, lbars, pmov ? pmot / pmov : 0u, pmov ? pmot * 10u / pmov % 10u : 0u, broot, bch);
        bad += gcheck("music: every note in the key but the rules' chromaticism (approaches < 2 %, coloured chords' tones < 3 %)",
                      !out_bad && out_appr * 50u < nn && out_ct * 33u < nn);
        bad += gcheck("  the SCALE page a mode of seven on every track, QUANT on; the CHORD page in-scale, LEAD, its chords in the key", !page_bad && !chord_bad);
        bad += gcheck("  the lead's beats on chord tones (75 % at least)", lct * 100u >= ls * 75u);
        bad += gcheck("  the lead moves mostly by steps (over half its moves), never past an octave", lstep * 2u > liv - lrep && !loct);
        bad += gcheck("  a leap past a fourth turned back by a step (70 % at least)", l5r * 100u >= l5 * 70u);
        bad += gcheck("  phrases close on a stable chord tone (70 %), the motif's rhythm comes back (70 % of its bars)",
                      lend_ok * 100u >= lend * 70u && lbars_r * 100u >= lbars * 70u);
        bad += gcheck("  the chords led closely (5.2 semitones a change at most, voice by voice); the bass's root at every change (95 %)",
                      pmot * 10u <= pmov * 52u && broot * 100u >= bch * 95u);
    }
    /* ---- (0.8.3) the sections relate: a pop genre's B on its predominant (the chords turned), its last section a whole
     * step up (a real modulation: the section's SCALE page moved with it, its notes in that key); a loop genre's
     * sections in the one key */
    {
        static project_t pa, pb, pc;
        uint32_t seeds, lift_ok = 0, rot_ok = 0, loop_ok = 0, keyc_bad = 0, n_c = 0;
        for (seeds = 1; seeds <= 6u; seeds++) {
            uint32_t bs, a0, b0;
            gen_reset();
            gen_with(15, seeds, 50, 1);                 /* SYNTH POP, PARTS: A B A C */
            bs = gs.trk_of[RL_BASS];
            if (!proj_import(&pa, &proj_slot[0], sizeof(project_store_t)) || !proj_import(&pb, &proj_slot[1], sizeof(project_store_t)) ||
                !proj_import(&pc, &proj_slot[2], sizeof(project_store_t))) continue;
            lift_ok += pc.t[bs].p[P_ROOT] == (pa.t[bs].p[P_ROOT] + 2) % 12 && pc.t[0].p[P_ROOT] == pc.t[bs].p[P_ROOT];
            for (k = 0; k < NTRK; k++) {                /* C's notes in C's key */
                track_t tt;
                if (gs.role[k] == RL_DRUM) continue;
                memcpy(tt.p, pc.t[k].p, sizeof tt.p);
                memcpy(tt.step, pc.t[k].step, sizeof tt.step);
                proj_steps(tt.step, pc.t[k].engine);
                for (i = 0; i < NSTEP; i++) {
                    uint32_t j;
                    if (tt.step[i].time != ST_NOTE) continue;
                    for (j = 0; j < tt.step[i].n; j++) { n_c++; keyc_bad += !in_scale(&tt, tt.step[i].note[j]); }
                }
            }
            {   /* B: the chords turned (its first bass note another root than A's) */
                step_t sa[NSTEP], sb[NSTEP];
                memcpy(sa, pa.t[bs].step, sizeof sa); proj_steps(sa, pa.t[bs].engine);
                memcpy(sb, pb.t[bs].step, sizeof sb); proj_steps(sb, pb.t[bs].engine);
                for (i = 0, a0 = b0 = 0; i < NSTEP && (!a0 || !b0); i++) {
                    if (!a0 && sa[i].time == ST_NOTE) a0 = sa[i].note[0] % 12u + 1u;
                    if (!b0 && sb[i].time == ST_NOTE) b0 = sb[i].note[0] % 12u + 1u;
                }
                rot_ok += a0 && b0 && a0 != b0;
            }
            gen_reset();
            gen_with(6, seeds, 50, 1);                  /* TECHNO: one key, the loop kept */
            if (proj_import(&pa, &proj_slot[0], sizeof(project_store_t)) && proj_import(&pc, &proj_slot[2], sizeof(project_store_t)))
                loop_ok += pa.t[gs.trk_of[RL_BASS]].p[P_ROOT] == pc.t[gs.trk_of[RL_BASS]].p[P_ROOT];
        }
        printf("gen: sections: SYNTH POP lifted %u of 6, its C notes off C's key %u of %u, B turned %u of 6; TECHNO one key %u of 6\n",
               lift_ok, keyc_bad, n_c, rot_ok, loop_ok);
        bad += gcheck("FORM PARTS (0.8.3): SYNTH POP's C a whole step up (its SCALE page with it), B on another chord; TECHNO's one key",
                      lift_ok == 6u && keyc_bad * 33u < n_c && rot_ok >= 5u && loop_ok == 6u);
    }
    /* ---- the DRUMS knob: the density grows with it, the kick stays */
    {
        uint32_t h0, h1, h2, k0, k2;
        gen_reset();
        gp.genre = 1; gp.seed = 77; gp.energy = 50; gp.form = 0; gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
        for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
        gp.amt[GP_DRUMS] = 20; gen_go(); h0 = hits_all(&trk[gs.trk_of[RL_DRUM]]); k0 = hits_of(&trk[gs.trk_of[RL_DRUM]], 0);
        gp.amt[GP_DRUMS] = 60; gen_go(); h1 = hits_all(&trk[gs.trk_of[RL_DRUM]]);
        gp.amt[GP_DRUMS] = 100; gen_go(); h2 = hits_all(&trk[gs.trk_of[RL_DRUM]]); k2 = hits_of(&trk[gs.trk_of[RL_DRUM]], 0);
        printf("gen: HOUSE drums at 20 / 60 / 100 %%: %u %u %u hits, the kick %u -> %u\n", h0, h1, h2, k0, k2);
        bad += gcheck("DRUMS 20 < 60 < 100 %: more hits each time, the kick's anchors from the start", h0 < h1 && h1 < h2 && k0 >= 4u && k2 >= k0);
    }
    /* ---- a part's knob touches its part only: BASS turned, the drums and the lead the same */
    {
        uint32_t d0, l0, b0, d1, l1, b1;
        gen_reset();
        gen_with(7, 500, 50, 0);
        d0 = steps_sig(&trk[gs.trk_of[RL_DRUM]]); l0 = steps_sig(&trk[gs.trk_of[RL_LEAD]]); b0 = steps_sig(&trk[gs.trk_of[RL_BASS]]);
        gp.amt[GP_BASS] = 30; gen_go();
        d1 = steps_sig(&trk[gs.trk_of[RL_DRUM]]); l1 = steps_sig(&trk[gs.trk_of[RL_LEAD]]); b1 = steps_sig(&trk[gs.trk_of[RL_BASS]]);
        bad += gcheck("BASS turned: the bass changes, the drums and the lead stay (a stream a part)", d0 == d1 && l0 == l1 && b0 != b1);
    }
    /* ---- a part OFF: its track silent; FORM: PARTS writes sections and ARRANGE */
    {
        gen_reset();
        gp.genre = 2; gp.seed = 9; gp.energy = 50; gp.form = 0;
        for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
        gp.amt[GP_LEAD] = 0; gp.amt[GP_ARP] = 0; gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
        gen_go();
        {
            uint32_t on = 0;
            for (k = 0; k < NTRK; k++)
                if (gs.role[k] == RL_LEAD || gs.role[k] == RL_ARP)
                    for (i = 0; i < (uint32_t)trk[k].p[P_SLEN]; i++) on += trk[k].step[i].time == ST_NOTE;
            bad += gcheck("LEAD and ARP OFF: their tracks silent (the sound kept)", on == 0u);
        }
        gen_reset();
        gen_with(1, 31, 60, 1);
        bad += gcheck("FORM PARTS: sections written, ARRANGE filled, PLAY the song",
                      live.mode == 1 && chain_config.count >= 2u && (dice_secs & 3u) == 3u && SONG_END(&chain_config) == SONG_FOREVER);
        for (k = 0; k < chain_config.count; k++) if (chain_config.row[k].slot > 3u || !chain_config.row[k].bars) bad += gcheck("  a row's section and bars", 0);
        {   /* (0.9.14) the seed changes the ARRANGE: the order and the rows' lengths (before: always the same) */
            uint32_t sd, f, sigs[12], diff = 0;
            for (f = 1; f <= 2u; f++)
                for (sd = 0; sd < 6u; sd++) { gen_reset(); gen_with(1, 31 + sd * 977u, 60, f); sigs[(f - 1u) * 6u + sd] = form_sig(); }
            for (sd = 1; sd < 6u; sd++) diff += sigs[sd] != sigs[0];
            for (sd = 7; sd < 12u; sd++) diff += sigs[sd] != sigs[6];
            bad += gcheck("FORM PARTS / SONG: other seeds, other arrangements (order and lengths)", diff >= 6u);
        }
        gen_reset();
        gen_with(1, 31, 60, 2);
        bad += gcheck("FORM SONG (HOUSE): four sections (intro, build, drop, break), five rows or more, the drop longest",
                      live.mode == 1 && chain_config.count >= 5u && (dice_secs & 15u) == 15u && form_drop_longest());
        {   /* the BUILD section holds automation on the chords (the filter opening), the BREAK no drums */
            project_t p;
            int ok = proj_import(&p, &proj_slot[2], sizeof(project_store_t)) && p.motion.count >= 4u;
            ok &= proj_import(&p, &proj_slot[3], sizeof(project_store_t)) && p.t[gs.trk_of[RL_DRUM]].p[P_MUTE] == 1;
            bad += gcheck("  the BUILD carries the filter's automation, the BREAK mutes the drums", ok);
        }
        {   /* the live music after: section A, nothing muted */
            uint32_t m = 0;
            for (k = 0; k < NTRK; k++) m += trk[k].p[P_MUTE] != 0;
            bad += gcheck("  after: the whole piece live (A), nothing muted", m == 0u && live.cur == 0);
        }
    }
    /* ---- (0.6.97) the kit's space, the top end's variety, Hook's register, the lead's envelope */
    {
        uint32_t seeds, rows = 0, bs, lo = 127, hi = 0, i2, sus_lt = 0, leads = 0;
        uint32_t seen[64]; uint32_t nseen = 0;
        gen_reset();
        for (seeds = 1; seeds <= 8u; seeds++) {         /* HOUSE: the closed hats' row over eight seeds */
            uint32_t h = 2166136261u, d;
            gen_with(1, seeds, 50, 0);
            d = gs.trk_of[RL_DRUM];
            for (i2 = 0; i2 < 16u; i2++) h = (h ^ (uint32_t)(trk[d].step[i2].time == ST_NOTE ? (trk[d].step[i2].hit >> 3) & 3u : 0u)) * 16777619u;
            for (i2 = 0; i2 < nseen && seen[i2] != h; i2++) {}
            if (i2 == nseen) seen[nseen++] = h;
        }
        rows = nseen;
        bad += gcheck("HOUSE x 8 seeds: three hat rows at least (the pools)", rows >= 3u);
        gen_with(1, 3, 50, 0);
        bad += gcheck("the kit's snare and clap into the reverb (SEND bus 3, 6..63)", DXS_BUS(dx_snd[DV_SNARE]) == 3u && DXS_AMT(dx_snd[DV_SNARE]) >= 6 &&
                      DXS_BUS(dx_snd[DV_CLAP]) == 3u);
        for (seeds = 1; seeds <= 4u; seeds++) {         /* POST PUNK: the bass high on the neck, never below E2 nor above A3 */
            gen_with(17, seeds, 50, 0);
            bs = gs.trk_of[RL_BASS];
            for (i2 = 0; i2 < dlen(&trk[bs]); i2++)
                if (trk[bs].step[i2].time == ST_NOTE) { if (trk[bs].step[i2].note[0] < lo) lo = trk[bs].step[i2].note[0]; if (trk[bs].step[i2].note[0] > hi) hi = trk[bs].step[i2].note[0]; }
            if (gs.act[RL_LEAD]) { leads++; sus_lt += trk[gs.trk_of[RL_LEAD]].p[P_SUS] < 95; }
        }
        if (lo < 40u || hi > 70u) printf("  post punk bass %u..%u\n", lo, hi);
        bad += gcheck("POST PUNK x 4: the bass between E2 and A#4 (Hook's neck), its tempo 118..148", lo >= 40u && hi <= 70u && song.g[G_BPM] >= 118 && song.g[G_BPM] <= 148);
        bad += gcheck("  the leads not all at full sustain", !leads || sus_lt > 0u);
    }
    /* ---- (0.6.96) a new piece: MUTATE off on every track (a reroll kept the lanes), the anchor the piece */
    {
        uint32_t on = 0;
        gen_reset();
        gen_with(1, 5, 50, 0);
        trk[3].p[P_MSND] = 3; trk[0].p[P_MPAT] = 2;
        gen_go();
        for (k = 0; k < NTRK; k++) on += trk[k].p[P_MSND] || trk[k].p[P_MPAT];
        bad += gcheck("a new piece: MUTATE SOUND / PATTERN OFF on every track, the anchor kept", !on && mut.ok);
    }
    /* ---- (0.6.95) a throw its own: HOUSE over twelve seeds takes more than one drum map and more than one chord
     * play; RANDOM over forty seeds throws mutants ("A+B") a third of the time or so, each playable and storable */
    {
        uint32_t seeds, maps = 0, plays = 0, mut = 0, bad_mut = 0;
        uint8_t seen_m[64] = {0}, seen_p[16] = {0};
        gen_reset();
        for (seeds = 1; seeds <= 12u; seeds++) {
            gen_with(1, seeds, 50, 0);
            if (!seen_m[gs.g->drum]) { seen_m[gs.g->drum] = 1; maps++; }
            if (!seen_p[gs.g->pads]) { seen_p[gs.g->pads] = 1; plays++; }
        }
        bad += gcheck("HOUSE x 12 seeds: two drum maps at least, two chord plays at least", maps >= 2u && plays >= 2u);
        for (seeds = 1; seeds <= 40u; seeds++) {
            gen_with(0, seeds, 50, 0);
            if (gs.mutant) {
                mut++;
                if (!strchr(gs.g->name, '+') || gs.g->bpm_hi < gs.g->bpm_lo || strncmp(ui.msg, "GEN: ", 5)) bad_mut++;
                project_capture(&proj_scratch);
                if (!proj_pack(&proj_wire, &proj_scratch)) bad_mut++;
                for (i = 0; i < 16u; i++) events_block(CTL);
            }
        }
        if (mut < 8u || mut > 28u || bad_mut) printf("  mutants: %u of 40, %u bad\n", mut, bad_mut);
        bad += gcheck("RANDOM x 40 seeds: 8..28 mutants, named A+B, a tempo range, storable, playing", mut >= 8u && mut <= 28u && !bad_mut);
    }
    /* ---- (0.6.94) the snare a snare: mostly noise, shorter than the factory kit's, its own FEEDBK */
    {
        gen_reset();
        gen_with(1, 5, 50, 0);                          /* HOUSE (a drum track) */
        if (dx_kit[DV_SNARE].a[DXP_NOISE] < 100u) printf("  snare: noise %u / %u, decay %u, lay %02x\n", dx_kit[DV_SNARE].a[DXP_NOISE], dx_kit[DV_SNARE].b[DXP_NOISE], dx_kit[DV_SNARE].a[DXP_DECAY], dx_lay[DV_SNARE]);
        bad += gcheck("the generated snare: NOISE >= 100 both ends, DECAY under the factory kit's, FEEDBK 3..6, LAYER kept",
                      dx_kit[DV_SNARE].a[DXP_NOISE] >= 100u && dx_kit[DV_SNARE].b[DXP_NOISE] >= 100u &&
                      dx_kit[DV_SNARE].a[DXP_DECAY] < DX_KIT_DEF[DV_SNARE].a[DXP_DECAY] && DX_LAY_FB(dx_lay[DV_SNARE]) >= 3u &&
                      DX_LAY_FB(dx_lay[DV_SNARE]) <= 6u && DX_LAY_L(dx_lay[DV_SNARE]) == DX_LAY_L(DX_LAY_DEF[DV_SNARE]));
        bad += gcheck("the generated hats: a band-pass (not the factory HP), COLOR >= 120, NOISE >= 112, the kick and clap as the kit's",
                      ((dx_kit[DV_HATC].mode >> 2) & 3u) == DXF_BP && ((dx_kit[DV_HATO].mode >> 2) & 3u) == DXF_BP &&
                      dx_kit[DV_HATC].a[DXP_COLOR] >= 120u && dx_kit[DV_HATO].b[DXP_COLOR] >= 120u &&
                      dx_kit[DV_HATC].a[DXP_NOISE] >= 112u && dx_kit[DV_HATO].a[DXP_NOISE] >= 112u &&
                      dx_kit[DV_KICK].mode == DX_KIT_DEF[DV_KICK].mode && dx_kit[DV_CLAP].mode == DX_KIT_DEF[DV_CLAP].mode);
    }
    /* ---- (0.6.93) the ARP amount is its density: a fifth of the steps at 20 %, most of them at 100 % */
    {
        uint32_t n20 = 0, n100 = 0, i, ar;
        gen_reset();
        gen_with(1, 5, 50, 0);                          /* HOUSE */
        ar = gs.trk_of[RL_ARP];
        gp.amt[GP_ARP] = 20; gen_go();
        for (i = 0; i < dlen(&trk[ar]); i++) n20 += trk[ar].step[i].time == ST_NOTE;
        gp.amt[GP_ARP] = 100; gen_go();
        for (i = 0; i < dlen(&trk[ar]); i++) n100 += trk[ar].step[i].time == ST_NOTE;
        if (n20 * 2u > dlen(&trk[ar]) || n100 < n20 * 2u) printf("  arp: %u notes at 20 %%, %u at 100 %% (LEN %u)\n", n20, n100, dlen(&trk[ar]));
        bad += gcheck("ARP 20 %: eighths, under half the steps; 100 %: at least twice as many notes", ar < NTRK && n20 * 2u <= dlen(&trk[ar]) && n100 >= n20 * 2u);
    }
    /* ---- (0.8.6) EXPLORE, the dice's default: at power-on, on NEW PROJECT, where the screen opens; sixty seeds, each
     * a piece that plays (two melodic parts at least, sound at a safe level, the voice and engine budgets), its SCALE and
     * CHORD pages the key it plays in (every note in it but the coloured chords' and the approaches), storable; and
     * very different from the next: the tempos, the keys, the loops' lengths and DIVs, the drum maps, the engines
     * (VOICE among them; ACID FM 0.2: never a retired one), the components, PLAYHEADS. A genre's piece the same before and after one */
    {
        static const uint32_t SEEDS = 60u;
        uint32_t sd, nmel_bad = 0, page_bad = 0, chord_bad = 0, key_bad = 0, key_col = 0, key_appr = 0, nn = 0;
        uint32_t store_bad = 0, budget_bad = 0, quiet = 0, loud = 0, odd = 0, poly = 0, divs = 0, comps = 0, lms = 0, heads = 0, words = 0;
        uint32_t nbpm = 0, nsc = 0, nlen = 0, ndm = 0, neng = 0, same = 0, bpm_lo = 999, bpm_hi = 0, name_bad = 0, cpu_bad = 0;
        uint8_t seen_bpm[256] = {0}, seen_sc[16] = {0}, seen_len[NSTEP + 1] = {0}, seen_dm[64] = {0}, seen_eng[NENGINES] = {0};
        uint32_t sig[60];
        double rms_min = 1e9;
        int32_t peak_max = 0;
        uint32_t vmax_all = 0;
        uint32_t vref = 0;
        bad += gcheck("EXPLORE (0.8.6): GENRE EXPLORE at power-on", genre0 == GEN_EXPLORE);
        gen_reset();
        gt_cpt = 63;                                    /* (0.9.14) the device as the model holds it (gen.c gc_dev x1.00): the
                                                         * pieces 0.9.13 made; the default's (x1.60) after these */
        cpt_res = 63; cpt_cores = 2;
        mc_host_v12 = 1;                                /* (0.9.13: two cores, the FM-1's default: the CPU budget's, 12 voices) */
        for (g = 0; g < 3u * NGENRE; g++) {             /* (the voices the genres' pieces reach at once, the budget's
                                                         * own overshoot included: the reference) */
            double rms;
            int32_t peak;
            uint32_t clip, vmax;
            gen_with(g % NGENRE + 1u, g * 31u + 5u, 20u + (g / NGENRE) * 30u, 0);
            gx_listen(1200u, &rms, &peak, &clip, &vmax);
            if (vmax > vref) vref = vmax;
        }
        for (sd = 0; sd < SEEDS; sd++) {
            uint32_t mel = 0, fm = 0, an = 0, vo = 0, dk = 0, l0 = 0, lens_differ = 0, clip, vmax, kk;
            double rms;
            int32_t peak;
            uint32_t h = 2166136261u;
            if (sd % 12u == 0u) gen_reset();
            gp.genre = (uint8_t)GEN_EXPLORE; gp.seed = (uint16_t)(sd * 163u + 7u); gp.energy = (uint8_t)(10u + (sd * 37u) % 85u);
            gp.form = 0;
            for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
            gp.complex = (uint8_t)((sd * 29u) % 100u); gp.swing = GA_AUTO; gp.human = 30;
            gen_go();
            if (strncmp(ui.msg, "GEN: EXPLORE ", 13) || strcmp(gs.g->name, "EXPLORE")) name_bad++;
            for (k = 0; k < NTRK; k++) {
                const track_t *t = &trk[k];
                uint32_t r = gs.role[k], len = dlen(t), on = 0, c = 0, b;
                if (drum_track(t)) { dk++; if (gs.act[RL_DRUM]) for (i = 0; i < len; i++) lms += step_lm_any(&t->step[i]) != 0; continue; }
                fm += t->eng_req == ENGI_FM6; an += eng_gone(t->eng_req % NENGINES); vo += t->eng_req == EG_VO;
                if (!seen_eng[t->eng_req % NENGINES]) { seen_eng[t->eng_req % NENGINES] = 1; neng++; }
                words += t->eng_req == EG_VO && t->p[P_VLANG] > 0;
                for (b = 0; b < 12u; b++) c += (scale_mask(t) >> b) & 1u;
                if (t->p[P_ROOT] != gs.root || t->p[P_SCALE] != gs.scale || c != 7u || t->p[P_QUANT] == QN_OFF) page_bad++;
                if (!gs.act[r] || (r == RL_TEX && k != gs.trk_of[RL_TEX])) continue;
                if (!l0) l0 = len; else if (len != l0) lens_differ = 1;
                if (!seen_len[len]) { seen_len[len] = 1; nlen++; }
                odd += len % 4u != 0u;
                divs += t->p[P_SDIV] != 2;
                for (i = 0; i < len; i++) {
                    const step_t *st = &t->step[i];
                    uint32_t j;
                    if (st->time != ST_NOTE) continue;
                    on++;
                    comps += st->comp != 0;
                    for (j = 0; j < st->n; j++) {
                        int32_t n = st->note[j], nx = -1;
                        uint32_t q;
                        nn++;
                        if (n < 20 || n > 96) { key_bad++; printf("  EXPLORE out of range: seed %u T%u step %u note %d\n", gp.seed, k + 1u, i, n); }
                        if (in_scale(t, n)) continue;
                        for (q = 1; q <= len && nx < 0; q++) if (t->step[(i + q) % len].time == ST_NOTE) nx = t->step[(i + q) % len].note[0];
                        if (gchord(i)->alt && ((gchord_pcs(i) >> (uint32_t)(n % 12)) & 1u)) key_col++;
                        else if (st->n == 1u && nx >= 0 && (nx - n == 1 || n - nx == 1)) key_appr++;
                        else { key_bad++; if (key_bad < 6u) printf("  EXPLORE off the key: seed %u T%u step %u note %d\n", gp.seed, k + 1u, i, n); }
                    }
                }
                mel += on > 0u;
                for (kk = 0; kk < FG_HEADS; kk++) heads += fg_set[k][kk].rate != 0;
            }
            poly += lens_differ;
            if (mel < 2u) { nmel_bad++; printf("  EXPLORE seed %u: %u melodic parts playing\n", gp.seed, mel); }
            if (fm > 2u || an || vo > 2u || dk > 1u) budget_bad++;   /* (an: a retired engine, ACID FM 0.2) */
            {   /* the CHORD page: an in-scale type on the chord track, its chords in the key */
                uint32_t pd = gs.trk_of[RL_PAD];
                if (pd < NTRK && gs.act[RL_PAD]) {
                    track_t *t = &trk[pd];
                    uint8_t o[KB_CH];
                    uint32_t key, c, j;
                    if (t->p[P_CHRD] < CD_TRI || t->p[P_CHRD] > CD_SUS4 || t->p[P_CVOI] != CV_LEAD) page_bad++;
                    for (key = 53; key < 80u; key++)
                        for (c = chord_build(t, key, o), j = 0; j < c; j++) if (!in_scale(t, o[j])) chord_bad++;
                }
            }
            if (gc_cost(1) > gc_budget()) { cpu_bad++; printf("  EXPLORE seed %u: CPU %u over the budget %u\n", gp.seed, gc_cost(1), gc_budget()); }
            if (!seen_bpm[song.g[G_BPM] & 255]) { seen_bpm[song.g[G_BPM] & 255] = 1; nbpm++; }
            if ((uint32_t)song.g[G_BPM] < bpm_lo) bpm_lo = (uint32_t)song.g[G_BPM];
            if ((uint32_t)song.g[G_BPM] > bpm_hi) bpm_hi = (uint32_t)song.g[G_BPM];
            if (!seen_sc[gs.scale & 15]) { seen_sc[gs.scale & 15] = 1; nsc++; }
            if (!seen_dm[gs.g->drum & 63]) { seen_dm[gs.g->drum & 63] = 1; ndm++; }
            project_capture(&proj_scratch);
            if (!proj_pack(&proj_wire, &proj_scratch) || motion.count > MOTION_MAX) store_bad++;
            h = all_sig();
            for (i = 0; i < sd; i++) same += sig[i] == h;
            sig[sd] = h;
            gx_listen(sd < 20u ? 2400u : 800u, &rms, &peak, &clip, &vmax);   /* (~1.6 s, then ~0.5 s) */
            if (rms < rms_min) rms_min = rms;
            if (peak > peak_max) peak_max = peak;
            if (vmax > vmax_all) vmax_all = vmax;
            if (rms < 150.0) { quiet++; printf("  EXPLORE seed %u: RMS %.0f\n", gp.seed, rms); }
            if (clip * 100u > 800u * 2u * CTL) { loud++; printf("  EXPLORE seed %u: %u samples at full scale\n", gp.seed, clip); }
        }
        printf("gen: EXPLORE x %u: %u tempos (%u..%u BPM), %u keys, %u loop lengths (%u odd), %u drum maps, %u engines, polyrhythm in %u,\n"
               "gen:   %u DIVs off 1/16, %u components, %u DRUM-X octave steps, %u playheads, %u singing VOICEs; %u notes: %u coloured, %u approaches;\n"
               "gen:   RMS %.0f at least, peak %d, %u voices at most (the genres: %u)\n",
               SEEDS, nbpm, bpm_lo, bpm_hi, nsc, nlen, odd, ndm, neng, poly, divs, comps, lms, heads, words, nn, key_col, key_appr, rms_min,
               peak_max, vmax_all, vref);
        bad += gcheck("  60 seeds: each named EXPLORE nnnn, each another piece (no two the same)", !name_bad && !same);
        bad += gcheck("  each plays: two melodic parts at least, sound (RMS), never stuck at full scale", !nmel_bad && !quiet && !loud);
        bad += gcheck("  the budgets: FM6 / VOICE on two tracks at most, no retired engine, one DRUM track, no more voices than a genre's",
                      !budget_bad && vmax_all <= vref);
        bad += gcheck("  the SCALE page its key on every track (a mode of seven, QUANT on); the CHORD page in-scale, its chords in the key",
                      !page_bad && !chord_bad);
        bad += gcheck("  every note in the key but the coloured chords' tones and the approaches (< 12 %), in range",
                      !key_bad && (key_col + key_appr) * 100u < nn * 12u);
        bad += gcheck("  varied: 20 tempos from under 90 to over 150 BPM, 6 keys, 12 loop lengths (odd ones), 12 drum maps",
                      nbpm >= 20u && bpm_lo < 90u && bpm_hi > 150u && nsc >= 6u && nlen >= 12u && odd > 0u && ndm >= 12u);
        bad += gcheck("  the rules broken: polyrhythm (a third of the pieces), DIVs, components, DRUM-X octaves, PLAYHEADS",
                      poly * 3u >= SEEDS && divs && comps && lms && heads);
        bad += gcheck("  any engine: six at least, VOICE among them, VOICE singing words; never WHEEL or ANIMAL (retired)",
                      neng >= 6u && !seen_eng[ENGI_ANIMAL] && !seen_eng[ENGI_WHEEL] && seen_eng[EG_VO] && words);
        bad += gcheck("  every piece storable, the automation inside its 64", !store_bad);
        bad += gcheck("  (0.9.13) every piece inside the CPU budget of two cores (gc_cost: 65 % of a half)", !cpu_bad);
        mc_host_v12 = 0;
        {   /* (0.9.13) COMPLEX sets how wild: 200 seeds at 0, 50 and 100, the tracks in a loop or a DIV of their own, the
             * BPM's spread, the PLAYHEADS; one core: the budget made every piece lighter (the estimate down, never up) */
            uint32_t cx, own[3] = {0}, hd[3] = {0}, lo[3] = {999, 999, 999}, hi[3] = {0}, est[2] = {0}, over1 = 0;
            mc_host_v12 = 1;
            for (cx = 0; cx < 3u; cx++)
                for (sd = 0; sd < 200u; sd++) {
                    uint32_t bpm;
                    gen_reset();
                    gp.genre = (uint8_t)GEN_EXPLORE; gp.seed = (uint16_t)(sd * 41u + 3u); gp.energy = 50; gp.form = 0;
                    for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
                    gp.complex = (uint8_t)(cx * 50u); gp.swing = GA_AUTO; gp.human = 30;
                    gen_go();
                    for (k = 0; k < NTRK; k++) {
                        uint32_t h;
                        own[cx] += !drum_track(&trk[k]) && gs.act[gs.role[k]] && (trk[k].p[P_SDIV] != 2 || (uint32_t)trk[k].p[P_SLEN] != gs.len);
                        for (h = 0; h < FG_HEADS; h++) hd[cx] += fg_set[k][h].rate != 0;
                    }
                    bpm = (uint32_t)song.g[G_BPM];
                    if (bpm < lo[cx]) lo[cx] = bpm;
                    if (bpm > hi[cx]) hi[cx] = bpm;
                    if (cx == 1u) {
                        est[0] += gc_cost(1);
                        mc_host_v12 = 0;
                        gen_reset();
                        gen_go();
                        est[1] += gc_cost(1);
                        over1 += gc_cost(1) > gc_budget();
                        mc_host_v12 = 1;
                    }
                }
            mc_host_v12 = 0;
            printf("gen: EXPLORE COMPLEX 0 / 50 / 100 x 200: tracks of their own loop or DIV %u %u %u, PLAYHEADS %u %u %u, BPM %u..%u %u..%u %u..%u;\n"
                   "gen:   CPU (thousandths of CPU TEST) at 50: two cores %u, one core %u on average, %u of 200 at one core's floor (the drums,\n"
                   "gen:   the bass, two-note chords: lighter it cannot be)\n",
                   own[0], own[1], own[2], hd[0], hd[1], hd[2], lo[0], hi[0], lo[1], hi[1], lo[2], hi[2], est[0] / 200u, est[1] / 200u, over1);
            bad += gcheck("  (0.9.13) COMPLEX: wilder as it goes up (own loops / DIVs, PLAYHEADS, the tempo's spread); one core lighter",
                          own[0] < own[1] && own[1] < own[2] && hd[0] <= hd[1] && hd[1] < hd[2] && hi[0] - lo[0] < hi[2] - lo[2] &&
                          est[1] < est[0]);
        }
        {   /* (0.9.14) the device factor (gen.c gc_dev): no CPU TEST measured, x1.60: the budget 1/1.6 of the model's (two
             * cores 1030 / 1.6 - 100 thousandths of CPU TEST), each piece inside it or at the floor (drums, bass, two-note
             * chords), lighter than at x1.00 on average, still two melodic parts and sound; a CPU TEST measured at 63 %
             * gives x1.00 again, at 126 % x2.00, never under x1.00. (ACID FM 0.5.2) Two cores: x1.90 unset (the budget
             * 1030 / 1.9, 542: 0.5.1's 543), a CPU TEST x1.50 its own ratio (GC_GEN_X100, measured on an FM-1): x1.00 at
             * 42 %, the pieces compared against it; one core as before */
            uint32_t est[2] = {0}, over = 0, mel_bad = 0, quiet2 = 0, d0, d1, d2, d3, d4;
            cpt_res = 0;
            mc_host_v12 = 1;
            d0 = gc_dev();
            for (sd = 0; sd < 60u; sd++) {
                double rms;
                int32_t peak;
                uint32_t clip, vmax, mel = 0, f;
                for (f = 0; f < 2u; f++) {
                    gt_cpt = f ? 0u : 42u;
                    gen_reset();
                    gp.genre = (uint8_t)GEN_EXPLORE; gp.seed = (uint16_t)(sd * 53u + 11u); gp.energy = 50; gp.form = 0;
                    for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
                    gp.complex = 50; gp.swing = GA_AUTO; gp.human = 30;
                    gen_go();
                    est[f] += gc_cost(1);
                }
                for (k = 0; k < NTRK; k++) {
                    const track_t *t = &trk[k];
                    uint32_t r = gs.role[k], on = 0;
                    if (drum_track(t) || !gs.act[r] || (r == RL_TEX && k != gs.trk_of[RL_TEX])) continue;
                    for (i = 0; i < dlen(t); i++) on += t->step[i].time == ST_NOTE;
                    mel += on > 0u;
                }
                over += gc_cost(1) > gc_budget();
                mel_bad += mel < 2u;
                gx_listen(400u, &rms, &peak, &clip, &vmax);
                quiet2 += rms < 150.0;
            }
            cpt_res = 42; cpt_cores = 2; d4 = gc_dev();
            mc_host_v12 = 0;
            cpt_res = 63; d1 = gc_dev(); cpt_res = 126; d2 = gc_dev(); cpt_res = 30; d3 = gc_dev();
            cpt_res = 0; cpt_cores = 0; gt_cpt = 63;
            printf("gen: (0.9.14) EXPLORE x 60 at COMPLEX 50, two cores: the device x%u.%02u by default: CPU %u thousandths of CPU TEST\n"
                   "gen:   on average (x1.00: %u), %u at the floor over the budget %u; x%u.%02u at 63 %%, x%u.%02u at 126 %%, x%u.%02u at 30 %%\n",
                   d0 / 100u, d0 % 100u, est[1] / 60u, est[0] / 60u, over, (uint32_t)(1030u * 100u / d0), d1 / 100u, d1 % 100u,
                   d2 / 100u, d2 % 100u, d3 / 100u, d3 % 100u);
            bad += gcheck("  (0.9.14) the device factor: x1.90 without a CPU TEST (two cores; one: x1.00 at 63 %, x2.00 at 126 %), "
                          "lighter pieces, still two melodic parts and sound",
                          d0 == 190u && d4 == 100u && d1 == 100u && d2 == 200u && d3 == 100u && est[1] < est[0] * 3u / 4u && !mel_bad && !quiet2);
            cpt_res = 63; cpt_cores = 2;
        }
        gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* EXPLORE deterministic; its FORMs; a genre's piece the same before and after it (DIV 1/16, no PLAYHEADS) */
        uint32_t h1, h2, h3, h4, divs = 0, fg = 0, sec_bad = 0, f;
        static project_t ps;
        gen_reset();
        gen_with(GEN_EXPLORE, 77, 50, 0); h1 = all_sig();
        gen_with(GEN_EXPLORE, 77, 50, 0); h2 = all_sig();
        bad += gcheck("EXPLORE deterministic: the same seed twice the same piece", h1 == h2);
        gen_reset();
        gen_with(1, 421, 50, 0); h3 = all_sig();
        for (f = 0; f < 6u; f++) gen_with(GEN_EXPLORE, 300u + f, 50, f % 3u);
        gen_with(1, 421, 50, 0); h4 = all_sig();
        for (k = 0; k < NTRK; k++) { divs += trk[k].p[P_SDIV] != 2; for (i = 0; i < FG_HEADS; i++) fg += fg_set[k][i].rate != 0; }
        bad += gcheck("  HOUSE 0421 the same piece before and after six EXPLORE pieces (DIV 1/16, no PLAYHEADS)", h3 == h4 && !divs && !fg);
        for (f = 1; f <= 12u; f++) {                    /* PARTS and SONG: each section in its own key, playing, storable */
            uint32_t s2;
            gen_reset();
            gen_with(GEN_EXPLORE, 900u + f, 60, 1u + f % 2u);
            if (!live.mode || chain_config.count < 2u) { sec_bad++; continue; }
            for (s2 = 0; s2 < 4u; s2++) {
                if (!((dice_secs >> s2) & 1u)) continue;
                if (!proj_import(&ps, &proj_slot[s2], sizeof(project_store_t))) { sec_bad++; continue; }
                for (k = 0; k < NTRK; k++) {
                    track_t tt;
                    uint32_t j;
                    if (gs.role[k] == RL_DRUM) continue;
                    memcpy(tt.p, ps.t[k].p, sizeof tt.p);
                    memcpy(tt.step, ps.t[k].step, sizeof tt.step);
                    proj_steps(tt.step, ps.t[k].engine);
                    if (tt.p[P_SCALE] != gs.scale) sec_bad++;
                    for (i = 0; i < NSTEP; i++) {
                        if (tt.step[i].time != ST_NOTE) continue;
                        for (j = 0; j < tt.step[i].n; j++) if (tt.step[i].note[j] < 12u || tt.step[i].note[j] > 108u) sec_bad++;
                    }
                }
            }
            for (i = 0; i < 16u; i++) events_block(CTL);
        }
        bad += gcheck("  EXPLORE PARTS / SONG x 12: sections stored, ARRANGE filled, each on the piece's SCALE", !sec_bad);
    }
    {   /* (0.9.13) the sounds' movement: every genre, RANDOM and EXPLORE x 20 seeds. A pitch LFO only as a soft vibrato
         * (at most a third of a semitone, ~5 Hz, fading in: never the old 2-semitone siren on the lead, track 5), the
         * filter moving on most melodic parts (its envelope or a slow LFO), a release in its role's range (never dry),
         * a progression of two chords or more on most pieces; (ACID FM 0.2) no part on a retired engine (WHEEL, ANIMAL: their
         * genres' recipes and EXPLORE's weights on FM6 and NOISE) */
        uint32_t gg, sd, n = 0, pit = 0, pit_bad = 0, mel = 0, fmod = 0, rel_bad = 0, prog = 0, xp = 0, anim = 0, xtr = 0;
        static const uint8_t RL_LO[RL_N] = {0, 40, 70, 55, 50, 70};   /* (EXPLORE's CPU budget may shorten a long one to 84) */
        gen_reset();
        for (gg = 0; gg <= GEN_EXPLORE; gg++)
            for (sd = 0; sd < 20u; sd++) {
                uint32_t pd, ch = 0;
                gen_with(gg, sd * 131u + 7u, 50, 0);
                n++;
                for (k = 0; k < NTRK; k++) {
                    const track_t *t = &trk[k];
                    uint32_t r = gs.role[k];
                    int32_t d = t->p[P_LD_PIT] < 0 ? -t->p[P_LD_PIT] : t->p[P_LD_PIT];
                    if (drum_track(t) || !gs.act[r] || k >= trk_live || (r == RL_TEX && k != gs.trk_of[RL_TEX])) continue;
                    mel++;
                    fmod += t->p[P_ED_FLT] != 0 || t->p[P_LD_FLT] != 0;
                    if (d) { pit++; if (d > 2 || t->p[P_LFADE] < 60 || t->p[P_LRATE] < 80) { pit_bad++; printf("  pitch LFO %d rate %d fade %d: %s %u T%u\n", t->p[P_LD_PIT], t->p[P_LRATE], t->p[P_LFADE], gs.g->name, gp.seed, k + 1u); } }
                    if (t->p[P_REL] < RL_LO[r]) { rel_bad++; printf("  release %d: %s %u T%u role %u\n", t->p[P_REL], gs.g->name, gp.seed, k + 1u, r); }
                    xtr++; anim += eng_gone(t->eng_req % NENGINES);
                }
                pd = gs.trk_of[RL_PAD];
                if (pd < NTRK && gs.act[RL_PAD]) for (i = 0; i < NSTEP; i++) ch += trk[pd].step[i].time == ST_NOTE && trk[pd].step[i].n >= 2u;
                prog += ch && gs.pn >= 2u;
                xp += gg == GEN_EXPLORE;
            }
        printf("gen: sounds x %u pieces: a soft vibrato on %u of %u melodic parts, the filter moving on %u; a progression in %u;"
               " a retired engine on %u of %u parts\n", n, pit, mel, fmod, prog, anim, xtr);
        bad += gcheck("(0.9.13) pitch LFO a soft vibrato only (<= 1/3 semitone, ~5 Hz, fading in), on few parts", !pit_bad && pit * 4u < mel);
        bad += gcheck("(0.9.13) the filter moving on 90 % of the melodic parts; releases in their role's range", fmod * 10u >= mel * 9u && !rel_bad);
        bad += gcheck("(0.9.13) a chord progression (two chords or more) in 90 % of the pieces; no retired engine on any genre's part",
                      prog * 10u >= n * 9u && !anim && xtr);
    }
    {   /* (ACID FM 0.2) NOISE never a bass, a lead or an arp (unpitched: gen_tables.c GX_ENG_W, ANIMAL's retired column
         * NOISE's on chords and texture only): every genre and RANDOM x 10 seeds, EXPLORE x 1200 at COMPLEX 0 / 50 / 100;
         * two cores (the FM-1's default: one core's CPU budget swaps most engines for ANALOG / LOFI) */
        uint32_t gg, sd, pieces = 0, parts = 0, nz = 0;
        gen_reset();
        mc_host_v12 = 1;
        for (gg = 0; gg <= GEN_EXPLORE; gg++)
            for (sd = 0; sd < (gg == GEN_EXPLORE ? 1200u : 10u); sd++) {
                gp.genre = (uint8_t)gg; gp.seed = (uint16_t)(sd * 97u + 5u); gp.energy = (uint8_t)(20u + (sd * 29u) % 70u);
                gp.form = 0;
                for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
                gp.complex = (uint8_t)(gg == GEN_EXPLORE ? (sd % 3u) * 50u : 50u); gp.swing = GA_AUTO; gp.human = 30;
                gen_go();
                pieces++;
                for (k = 0; k < NTRK && k < trk_live; k++) {
                    uint32_t r = gs.role[k];
                    if (drum_track(&trk[k]) || !gs.act[r] || (r != RL_BASS && r != RL_LEAD && r != RL_ARP)) continue;
                    parts++;
                    if (trk[k].eng_req == ENGI_NOISE) {
                        nz++;
                        if (nz <= 5u) printf("  NOISE on role %u: %s seed %u T%u\n", r, gs.g->name, gp.seed, k + 1u);
                    }
                }
            }
        mc_host_v12 = 0;
        printf("gen: NOISE on %u of %u bass / lead / arp parts (%u pieces)\n", nz, parts, pieces);
        bad += gcheck("(ACID FM 0.2) NOISE never a bass, a lead or an arp (every genre, RANDOM, EXPLORE x 1200)", !nz && parts > 1500u);
    }
    {   /* (0.9.17) the dice's variety and its melodies (tests/dice_metrics.c measures them at length): OCT+ pressed 24
         * times a genre (MICRO HOUSE among them: the owner's), a part the same as the press before (90 % of its steps,
         * transposed to the key) seldom, the sounds of a part many, the press before's seldom; the power-on's first
         * piece not the same every session; SEQ + SELECT on the piece's harmony, never the same twice running; EDIT +
         * SELECT never the same preset twice running; the lead in the key, by steps, few leaps, a few syncopations; its
         * envelope seldom held at the top */
        static const uint8_t GG[6] = {3, 1, 6, 14, 27, 40};   /* (MICRO HOUSE, HOUSE, TECHNO, SYNTH POP, BOOM BAP, AMBIENT) */
        static int16_t prev[NTRK][NSTEP];
        uint32_t gi, pr, near = 0, parts = 0, sndrep = 0, snd = 0, ml_pre[24], nml = 0, liv = 0, lstep = 0, lleap = 0, lon = 0,
                 loff = 0, lflat = 0, lfull = 0, nl = 0, seq_ct = 0, seq_st = 0, seq_same = 0, sd_rep = 0, po_same = 0;
        uint16_t lastsnd[NTRK];
        for (gi = 0; gi < 6u; gi++) {
            gen_reset();
            gen_with(GG[gi], 4321u + gi, 50, 0);
            memset(prev, 0, sizeof prev);
            for (k = 0; k < NTRK; k++) lastsnd[k] = 0xFFFF;
            for (pr = 0; pr < 24u; pr++) {
                host_ticks += 1300000u + pr * 777u;
                gen_press();
                for (k = 0; k < NTRK; k++) {
                    const track_t *t = &trk[k];
                    uint32_t r = gs.role[k], len = dlen(t), eq = 0, any = 0;
                    uint16_t id = (uint16_t)(t->eng_req << 8 | t->preset);
                    int32_t pv = -1;
                    if (!gs.act[r] || (r == RL_TEX && k != gs.trk_of[RL_TEX])) { prev[k][0] = 0x7FFF; continue; }
                    for (i = 0; i < len; i++) {
                        const step_t *st = &t->step[i];
                        int16_t v = (int16_t)(r == RL_DRUM ? (st->time == ST_NOTE ? st->hit : 0) :
                                              st->time == ST_NOTE && st->n ? 200 + st->note[0] - gs.root : st->time == ST_TIE);
                        eq += v == prev[k][i]; any |= v; prev[k][i] = v;
                    }
                    for (; i < NSTEP; i++) prev[k][i] = 0x7FFF;
                    if (pr && any) { parts++; near += eq * 10u >= len * 9u; }
                    if (r != RL_DRUM) { snd++; sndrep += pr && id == lastsnd[k]; lastsnd[k] = id; }
                    if (gi == 0u && r == RL_LEAD) {
                        uint32_t j;
                        for (j = 0; j < nml && ml_pre[j] != id; j++) {}
                        if (j == nml && nml < 24u) ml_pre[nml++] = id;
                    }
                    if (r == RL_LEAD) {
                        nl++; lflat += t->p[P_SUS] >= 120 && t->p[P_REL] <= 12; lfull += t->p[P_SUS] >= 100;
                        for (i = 0; i < len; i++) {
                            const step_t *st = &t->step[i];
                            int32_t a;
                            if (st->time != ST_NOTE || !st->n) continue;
                            lon++; loff += i & 1u;
                            if (pv >= 0) { a = st->note[0] - pv; if (a < 0) a = -a; liv++; lstep += a && a <= 2; lleap += a > 7; }
                            pv = st->note[0];
                        }
                    }
                }
            }
        }
        printf("gen: (0.9.17) 6 genres x 24 presses: a part near the press before's in %u of %u, the press before's sound in %u of %u;"
               " MICRO HOUSE's lead %u sounds; the lead: steps %u %%, leaps past a fifth %u %%, off the eighths %u %%, held at the top %u of %u\n",
               near, parts, sndrep, snd, nml, liv ? lstep * 100u / liv : 0u, liv ? lleap * 100u / liv : 0u, lon ? loff * 100u / lon : 0u, lfull, nl);
        bad += gcheck("(0.9.17) DICE: a part near the press before's under 8 %, its sound the press before's under 25 %", near * 100u < parts * 8u && sndrep * 4u < snd);
        bad += gcheck("  MICRO HOUSE's lead: 8 sounds at least in 24 presses (0.9.17: 7 in 40)", nml >= 8u);
        bad += gcheck("  the lead by steps (40 % at least), leaps past a fifth under 5 %, 12 % at least off the eighths", lstep * 100u >= liv * 40u && lleap * 100u < liv * 5u && loff * 100u >= lon * 12u);
        bad += gcheck("  the lead's envelope: no release at full sustain never, held at 79 % or more under 15 %", !lflat && lfull * 100u < nl * 15u);
        {   /* the power-on: the first OCT+ of two sessions */
            uint32_t h0;
            gen_reset(); session_dice(111u); gp.genre = GEN_EXPLORE; gp.sig = 0; gen_press(); h0 = dice_print();
            gen_reset(); session_dice(222u); gp.genre = GEN_EXPLORE; gp.sig = 0; gen_press(); po_same = dice_print() == h0;
            bad += gcheck("  the power-on's SEED its session's: the first OCT+ another piece every session (0.9.17: SEED 0001)", !po_same);
        }
        {   /* SEQ + SELECT on a HOUSE piece's lead, 20 times */
            uint32_t ld, last = 0;
            gen_reset(); gen_with(1, 77, 50, 0);
            ld = gs.trk_of[RL_LEAD];
            for (pr = 0; pr < 20u && ld < NTRK; pr++) {
                uint32_t sg;
                host_ticks += 990000u;
                dice_seq(&trk[ld]);
                sg = steps_sig(&trk[ld]);
                seq_same += pr && sg == last; last = sg;
                for (i = 0; i < dlen(&trk[ld]); i += 4u)
                    if (trk[ld].step[i].time == ST_NOTE && trk[ld].step[i].n) { seq_st++; seq_ct += (gchord_pcs(i) >> (trk[ld].step[i].note[0] % 12u)) & 1u; }
            }
            printf("gen: (0.9.17) SEQ + SELECT x 20 on HOUSE 0077's lead: strong steps on the piece's chord tones %u of %u, the same twice running %u\n", seq_ct, seq_st, seq_same);
            bad += gcheck("  SEQ + SELECT: on the piece's chords (75 % of the strong steps; 0.9.17: 58 %), never the same twice running", ld < NTRK && seq_ct * 4u >= seq_st * 3u && !seq_same);
        }
        {   /* EDIT + SELECT x 30, on FM6 (15 presets) */
            uint32_t last = 0xFFFF;
            gen_reset();
            set_engine_of(&trk[1], ENGI_FM6);
            for (pr = 0; pr < 30u; pr++) { host_ticks += 700000u; sound_dice_of(&trk[1], 1); sd_rep += trk[1].preset == last; last = trk[1].preset; }
            bad += gcheck("  EDIT + SELECT: never the same preset twice running (FM6 x 30)", !sd_rep);
        }
    }
    {   /* NEW PROJECT (SAVE > TOOLS 5): GENRE back to EXPLORE */
        gen_reset();
        gp.genre = 5;
        tools_run(5); press(B_OCTUP);
        bad += gcheck("NEW PROJECT: GENRE back to EXPLORE", gp.genre == GEN_EXPLORE && msg_is("NEW PROJECT"));
    }
    /* ---- the screen: HOME + SELECT opens GEN, the knobs set, SELECT pages, OCT+ makes, OCT- is done */
    {
        gen_reset();
        gp.genre = 7; gp.energy = 50; gp.page = 0; gp.form = 0;   /* (0.6.93) a genre left from before: the screen opens
                                                         * on RANDOM; (0.8.6) on EXPLORE */
        for (k = 0; k < GP_N; k++) gp.amt[k] = GA_AUTO;
        go_home(); frames(16);
        fm1_ms += 1200u;
        held_turn(B_HOME, 1);
        bad += gcheck("HOME + SELECT: the GEN screen (ui.confirm CF_DICE_ALL), page 1, GENRE EXPLORE (0.8.6)", ui.confirm == CF_DICE_ALL && gp.page == 0u && gp.genre == GEN_EXPLORE);
        {   /* (0.8.6) the knob's order: EXPLORE first, RANDOM, the genres; back from EXPLORE the last genre */
            char v[16];
            uint8_t g0 = gp.genre;
            int ok = 1;
            static const char *const WALK[12] = {"EXPLORE", "RANDOM", "IDM", "GLITCH", "BREAKS", "D'N'BASS", "ACID",
                                                 "ELECTRO", "MICRO", "RETRO", "AMBIENT", "AMBTECH"};
            static const char *const FULL[10] = {"IDM", "GLITCH HOP", "BREAKS", "DRILL'N'BASS", "ACID", "ELECTRO", "MICRO",
                                                 "RETROBOC", "AMBIENT", "AMB TECHNO"};
            uint32_t w;
            gp.genre = GEN_EXPLORE;
            for (w = 0; w < 24u; w++) {                 /* (3b) ten genres shown: forward twice round, then back twice */
                gen_value(0, 0, v); ok &= !strcmp(v, WALK[w % 12u]);   /* (the card's names) */
                if (w % 12u > 1u) ok &= !strcmp(GEN_SHOWN_NAME[w % 12u - 2u], FULL[w % 12u - 2u]);
                ok &= w % 12u < 2u ? gp.genre == (w % 12u ? 0u : GEN_EXPLORE) : gp.genre == 1u + GEN_SHOWN[w % 12u - 2u];
                gen_knob(0, 1);
            }
            ok &= gp.genre == GEN_EXPLORE;
            for (w = 0; w < 24u; w++) {
                gen_knob(0, -1);
                gen_value(0, 0, v); ok &= !strcmp(v, WALK[(36u - 1u - w) % 12u]);
            }
            ok &= gp.genre == GEN_EXPLORE;
            gp.genre = g0; gp.hot = 0xFF;
            bad += gcheck("  GENRE's order: EXPLORE, RANDOM, the ten (IDM .. DRIFT), wrapping both ways", ok);
        }
        {   /* (3b) the ten shown genres: no random draw reaches a hidden one; a stored hidden genre generates as before */
            uint32_t sd, hid = 0, hh;
            char v[16];
            for (sd = 0; sd < 2500u; sd++) {
                hid += gen_shown_index(gen_random_genre()) == 255u;
                gen_with(0, sd, 50, 0);
                hid += gen_shown_index(gs.gi + 1u) == 255u;
            }
            bad += gcheck("  (3b) RANDOM / mutant / dice draws over 2,500 seeds: only the ten genres", hid == 0u);
            bad += gcheck("  (3b) gen_shown_index: the ten in order, hidden / RANDOM / EXPLORE 255", gen_shown_index(1u + GEN_SHOWN[0]) == 0u &&
                          gen_shown_index(1u + GEN_SHOWN[9]) == 9u && gen_shown_index(1u) == 255u && gen_shown_index(0u) == 255u &&
                          gen_shown_index(GEN_EXPLORE) == 255u);
            gen_with(1, 421, 50, 0); hh = all_sig();
            bad += gcheck("  (3b) a stored hidden genre (HOUSE 0421): the piece as at the base commit", hh == GEN_HOUSE_HASH);
            gp.genre = 1; gen_value(0, 0, v);
            bad += gcheck("  (3b) ... and its name shows as JIANT's", !strcmp(v, "HOUSE"));
            gp.genre = (uint8_t)GEN_EXPLORE; gp.hot = 0xFF;
        }
        {   /* (0.5.1) each shown genre plays its own tempo range and swing (docs/braindancer/genre-research.md): RETROBOC 68..100,
             * DRILL'N'BASS 155..172, ELECTRO 124..138 ...; a hidden genre keeps its template's */
            uint32_t i, sd, ok = 1, sw = 1;
            for (i = 0; i < GEN_NSHOWN; i++)
                for (sd = 1; sd <= 40u; sd++) {
                    uint32_t lo = GEN_SHOWN_TEMPO[i][0], hi = GEN_SHOWN_TEMPO[i][1];
                    gen_with(1u + GEN_SHOWN[i], sd, (sd * 37u) % 101u, 0);
                    ok &= song.g[G_BPM] >= (int32_t)lo - 3 && song.g[G_BPM] <= (int32_t)hi + 3;
                    sw &= gp.swing != GA_AUTO || song.g[G_SWING] == GEN_SHOWN_TEMPO[i][2];
                }
            bad += gcheck("  (0.5.1) every shown genre's tempo within its own range (+-3, the energy's nudge)", ok);
            bad += gcheck("  (0.5.1) ... and its own swing", sw);
            bad += gcheck("  (0.5.1) the names: GLITCH HOP, AMB TECHNO; RETROBOC kept", !strcmp(GEN_SHOWN_NAME[1], "GLITCH HOP") &&
                          !strcmp(GEN_SHOWN_NAME[9], "AMB TECHNO") && !strcmp(GEN_SHOWN_NAME[7], "RETROBOC"));
            gen_with(1, 421, 50, 0);
            bad += gcheck("  (0.5.1) a hidden genre (HOUSE) keeps its template's tempo", song.g[G_BPM] >= 119 && song.g[G_BPM] <= 130);
        }
        {   /* (3b final review I2) RANDOM's mutants are named from the shown names only ("ACID+D'N'BASS 0421"), never JIANT's
             * hidden ones; the name fits the GEN panel's LAST line and the status line */
            uint32_t sd, nm = 0, badn = 0, wide = 0, j;
            char v[48];
            for (sd = 0; sd < 3000u; sd++) {
                char a[24], *pl;
                gen_with(0, sd, 50, 0);
                if (!gs.mutant) continue;
                nm++;
                str_cpy(a, gp.last, sizeof a);
                pl = strchr(a, ' ');                   /* the name before the seed (no shown card name has a space) */
                if (pl) *pl = 0;
                pl = strchr(a, '+');
                if (!pl) { badn++; continue; }
                *pl++ = 0;
                {
                    int ka = 0, kb = 0;
                    for (j = 0; j < GEN_NSHOWN; j++) {
                        ka |= !strcmp(a, GEN_SHOWN_CARD[j]);
                        kb |= !strcmp(pl, GEN_SHOWN_CARD[j]);
                    }
                    if (!ka || !kb) { if (badn < 3u) printf("    mutant name \"%s\"\n", gp.last); badn++; }
                }
                gen_last_line(v, sizeof v);
                wide += text_w(&AF_X, v) > GEN_LAST_W || !strstr(v, gp.last) || text_w(&UF_HDR, ui.msg) > 232 || str_len(gp.last) + 1u >= sizeof gp.last ||
                        strncmp(ui.msg, "GEN: ", 5) || strcmp(ui.msg + 5, gp.last);
            }
            bad += gcheck("  (3b) RANDOM's mutants over 3,000 seeds: named from the shown card names only", nm > 500u && badn == 0u);
            bad += gcheck("  (3b) ... the name and seed whole, fitting the GEN panel (150 px) and the status line", nm > 500u && wide == 0u);
            gp.genre = (uint8_t)GEN_EXPLORE; gp.hot = 0xFF; ui.msg_t = 0;
        }
        frames(800);
        press(B_PLAY); frames(4);
        bad += gcheck("  (0.6.97) PLAY on the GEN screen: the transport runs, the screen stays", (song.playing || transport_req == 1) && ui.confirm == CF_DICE_ALL);
        press(B_PLAY); frames(4);
        bad += gcheck("  PLAY again: stopped, the screen stays", (!song.playing || transport_req == 2) && ui.confirm == CF_DICE_ALL);
        frames(8);
        frames(800);                                    /* (the knobs quiet a moment after a button: ui_layer.c) */
        turn(EN_K1, 1); turn(EN_K1, 1); turn(EN_K3, 2);
        bad += gcheck("  KNOB 1 the genre (EXPLORE -> RANDOM -> IDM), KNOB 3 the energy (+10)", gp.genre == 1u + GEN_SHOWN[0] && gp.energy == 60u);
        turn(EN_SELECT, 1); frames(800);                /* (0.9.2: SELECT GEN's next page, GEN 2; a HOME tap: on to ECO) */
        turn(EN_K1, -1); turn(EN_K2, 1);
        bad += gcheck("  SELECT: page 2 (PARTS); DRUMS AUTO -> 100 %, BASS AUTO stays AUTO above 100", gp.page == 1u && gp.amt[GP_DRUMS] == 100u && gp.amt[GP_BASS] == GA_AUTO);
        {
            uint32_t sig0 = steps_sig(&trk[0]), sig1;
            uint16_t seed0;
            press(B_OCTUP); frames(4);
            seed0 = gp.seed;
            sig1 = steps_sig(&trk[0]);
            bad += gcheck("  OCT+: a piece (GEN: IDM nnnn), the screen stays", ui.confirm == CF_DICE_ALL && !strncmp(ui.msg, "GEN: IDM ", 9) && sig1 != sig0);
            press(B_OCTUP); frames(4);
            bad += gcheck("  OCT+ again with the same settings: a new seed, another piece", gp.seed != seed0 && steps_sig(&trk[0]) != sig1);
            press(B_OCTDN);
            bad += gcheck("  OCT-: done (0.9.2: HOME's ECO), the piece stays", !ui.confirm && ui.home && home_level() == 0u &&
                          steps_sig(&trk[0]) != sig0);
        }
    }
    {   /* (0.9.18) the drums' rolls on their lanes (x2 / x4), never the step's RATCH: a kick under a rolled hat, a hit
         * recorded into the step, never roll with it */
        uint32_t g, sd, k, i, stepr = 0, lrolls = 0;
        for (g = 0; g < NGENRE; g++)
            for (sd = 0; sd < 4u; sd++) {
                gen_with(g + 1u, sd * 977u + g * 13u + 3u, 40u + sd * 20u, 0);
                for (k = 0; k < NTRK; k++) {
                    const track_t *t = &trk[k];
                    if (!drum_track(t)) continue;
                    for (i = 0; i < NSTEP; i++) {
                        stepr += t->step[i].time == ST_NOTE && step_ratchet(&t->step[i]) > 1u;
                        for (uint32_t l = 0; l < NLANE; l++) lrolls += lm_hits(step_lmode(&t->step[i], l)) > 1u;
                    }
                }
            }
        printf("gen:   drum rolls on their lanes: %u, steps with RATCH on a kit: %u\n", lrolls, stepr);
        bad += gcheck("GEN drums: rolls as the lanes' x2 / x4, no step RATCH on a kit", !stepr && lrolls > 0u);
    }
    {   /* (ACID FM 0.5.2) the CPU budget against an FM-1 (CPU TEST stored at ~74 %, TRACKS 6, two cores). EXPLORE 3423 at
         * the defaults made a 2 s grain delay on the quarter-rate line (fx.c dly_lo), ~10 % of the device's CPU whatever
         * the voices: 2 lost halves a second, none with its TIME short. EXPLORE 9641 (its delay short) lost ~4.5 a second
         * at 75-80 %. Both fitted 0.5.1's budget (780 at 74 %): the device runs GEN pieces x1.45 .. x1.6 heavier against
         * the CPU TEST song than the model says (gen.c GC_GEN_X100). Now: the budget 65 % of the device's half, 588 at 74 %
         * (542 unset, as 0.5.1's 543); a quarter-rate delay only on a piece under it with it, and not on a busy one
         * (5 parts or more) with the CPU TEST unset; delays at the full or half rate never shortened (a slow genre's 1/4
         * echo stays); EXPLORE fitted under the budget. Then ~1000 pieces (EXPLORE, RANDOM, every genre; the CPU TEST
         * unset and 74 %). (review I2) RANDOM and the genres keep their parts: only a quarter-rate delay is budgeted */
        static const uint8_t CPT[2] = {0, 74};
        uint32_t c, g, sd, k, n = 0, over_q = 0, busy_q = 0, cut_bad = 0, cut = 0, kept_q = 0, st3423[2], ov[2][2], bud[2];
        uint32_t xr_n = 0, xr_over = 0, slow_kept = 0, x_over = 0, r_over = 0, r_diff = 0, r_n = 0, r_dly = 0;
        mc_host_v12 = 1;
        for (c = 0; c < 2u; c++) {
            static const uint16_t SEED[2] = {3423, 9641};
            uint32_t j;
            for (j = 0; j < 2u; j++) {
                gt_cpt = CPT[c]; gen_reset();
                gen_with(GEN_EXPLORE, SEED[j], 50, 1);
                if (!j) st3423[c] = gt_dly_stage();
                ov[c][j] = gc_cost(1) > gc_budget(); bud[c] = gc_budget();
                k = (uint32_t)song.g[G_DTIME];
                printf("gen: (0.5.2) EXPLORE %u, CPU TEST %u: TIME %s -> %s (the line at 1/%u rate), estimate %u, budget %u\n",
                       SEED[j], CPT[c], N_DTIME[gs.g->dtime % NELEM(N_DTIME)], N_DTIME[k % NELEM(N_DTIME)], 1u << gt_dly_stage(),
                       gc_cost(1), gc_budget());
            }
        }
        bad += gcheck("(0.5.2) the budget, two cores: 588 at CPU TEST 74 % (65 % of the FM-1's half), 542 unset",
                      bud[1] == 588u && bud[0] == 542u);
        bad += gcheck("  EXPLORE 3423 (CPU TEST unset, 74 %): its delay at the full rate", !st3423[0] && !st3423[1]);
        bad += gcheck("  EXPLORE 3423 and 9641 (CPU TEST unset, 74 %): under the budget", !ov[0][0] && !ov[0][1] && !ov[1][0] && !ov[1][1]);
        for (g = 0; g <= GEN_EXPLORE; g++)
            for (sd = 0; sd < (g == GEN_EXPLORE ? 400u : g == 0u ? 200u : 10u); sd++, n++) {
                uint32_t st, over, dt, busy, st0;
                gt_cpt = CPT[sd & 1u]; gen_reset();
                gen_with(g, sd * 89u + g * 7u + 5u, 30u + (sd * 17u) % 60u, sd % 3u);
                st = gt_dly_stage(); over = gc_cost(1) > gc_budget(); busy = gen_busy();
                over_q += over && st == 2u;
                busy_q += busy && !gt_cpt && st == 2u;
                kept_q += st == 2u;
                if (g == 0u || g == GEN_EXPLORE) { xr_n++; xr_over += over; x_over += over && g; r_over += over && !g;
                    if (over && xr_over < 8u) printf("  %s (%s) seed %u: %u over the budget %u, CPU TEST %u\n", g ? "EXPLORE" : "RANDOM", gs.g->name, gp.seed, gc_cost(1), gc_budget(), gt_cpt); }
                dt = (uint32_t)song.g[G_DTIME];
                song.g[G_DTIME] = gs.g->dtime;
                st0 = gt_dly_stage();
                slow_kept += st0 == 1u && dt == gs.g->dtime;
                if (dt != gs.g->dtime) {                /* shortened: a quarter-rate delay, over the budget with it or busy */
                    uint32_t why = st0 == 2u && (gc_cost(1) > gc_budget() || (busy && !gt_cpt));
                    cut++;
                    if (!why && ++cut_bad < 6u)
                        printf("  %s seed %u: TIME %s shortened, its line at 1/%u rate, %u of %u with it\n", gs.g->name, gp.seed,
                               N_DTIME[gs.g->dtime % NELEM(N_DTIME)], 1u << st0, gc_cost(1), gc_budget());
                }
                song.g[G_DTIME] = (int16_t)dt;
                if (over && st == 2u && over_q < 6u) printf("  %s seed %u: over the budget (%u of %u), the delay at a quarter rate\n", gs.g->name, gp.seed, gc_cost(1), gc_budget());
            }
        printf("gen: (0.5.2) %u pieces: %u delays shortened, %u quarter-rate kept, %u half-rate kept; EXPLORE + RANDOM %u of %u "
               "over the budget (EXPLORE %u, RANDOM %u)\n", n, cut, kept_q, slow_kept, xr_over, xr_n, x_over, r_over);
        bad += gcheck("  ~1000 pieces: none over the budget, nor busy with the CPU TEST unset, with a quarter-rate delay", !over_q && !busy_q);
        bad += gcheck("  .. a delay shortened only from the quarter rate, over the budget or busy; half / full rate ones kept",
                      !cut_bad && slow_kept >= 10u);
        for (g = 0; g < GEN_EXPLORE; g++)              /* (review I2) RANDOM, a genre: the same piece whatever the budget */
            for (sd = 0; sd < (g ? 4u : 100u); sd++) {
                uint32_t sig[2], act[2], rt[2], dt2[2], j, q, hh;
                for (j = 0; j < 2u; j++) {
                    gt_cpt = CPT[j]; gen_reset();
                    gen_with(1, 7, 50, 0);              /* (the press before the same for both: GEN keeps away from it) */
                    gen_with(g, sd * 61u + g * 13u + 3u, 30u + (sd * 23u) % 60u, sd % 3u);
                    sig[j] = all_sig(); dt2[j] = (uint32_t)song.g[G_DTIME]; rt[j] = (uint32_t)song.g[G_RTYPE] << 8 | (uint32_t)(song.g[G_DPIT] != 0);
                    for (act[j] = 0, q = 0; q < 6u; q++) act[j] |= (uint32_t)(gs.act[q] != 0) << q;
                    for (q = 0; q < NTRK; q++) for (hh = 0; hh < FG_HEADS; hh++) act[j] += (fg_set[q][hh].rate != 0) << 8;
                }
                if (dt2[0] != dt2[1]) { r_dly++; continue; }   /* (the delay rule: its TIME, then the rest drawn after it) */
                r_n++;
                if (sig[0] != sig[1] || act[0] != act[1] || rt[0] != rt[1]) {
                    r_diff++;
                    if (r_diff < 4u) printf("  %s seed %u: another piece at CPU TEST 74 %% than unset\n", gs.g->name, gp.seed);
                }
            }
        printf("gen: (0.5.2) RANDOM and the genres at CPU TEST unset and 74 %%: %u pieces with the same TIME, %u of them differ; "
               "%u with a quarter-rate delay shortened at one of them\n", r_n, r_diff, r_dly);
        bad += gcheck("  .. EXPLORE under the budget (4 % over at most: at the floor, the fit's steps run out)", x_over * 25u <= 400u);
        bad += gcheck("  .. (review I2) RANDOM and the genres keep their parts, sounds, reverb, grain and PLAYHEADS whatever the "
                      "budget: only a quarter-rate delay changes", !r_diff);
        mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* (ACID FM 0.5.2 review I1, M1) the parts the safety net may hush (audio.c fx_safety_tick: safe_part) are the GEN
         * piece's texture and arp only while that piece is the music, and only if it plays them: UNDO, NEW PROJECT, a
         * section or project loaded, a part diced on that track: none, and the net's levers off */
        uint32_t sd, m1_bad = 0, armed = 0, ok_undo, ok_new, ok_sec, ok_dice, ok_regen, ok_cpt, k;
        mc_host_v12 = 1; gt_cpt = 74; gen_reset();
        for (sd = 0; sd < 60u; sd++) {
            gen_with(sd % 3u ? GEN_EXPLORE : 1u + sd % 10u, sd * 37u + 11u, 50, sd % 3u);
            m1_bad += (safe_part[0] < NTRK) != (gs.act[RL_TEX] && gs.trk_of[RL_TEX] < trk_live) ||
                      (safe_part[1] < NTRK) != (gs.act[RL_ARP] && gs.trk_of[RL_ARP] < trk_live) ||
                      (safe_part[0] < NTRK && safe_part[0] != gs.trk_of[RL_TEX]) || (safe_part[1] < NTRK && safe_part[1] != gs.trk_of[RL_ARP]);
            armed += safe_part[0] < NTRK || safe_part[1] < NTRK;
        }
        for (sd = 0; sd < 200u; sd++) {                 /* (a piece with both: GENRE 1) */
            gen_with(1, sd * 7u + 1u, 50, 1);
            if (safe_part[0] < NTRK && safe_part[1] < NTRK) break;
        }
        fx_safe = SAFE_DLY | SAFE_TEX | SAFE_ARP;
        ok_regen = safe_part[0] < NTRK;
        ok_undo = !undo_do() && safe_part[0] == NTRK && safe_part[1] == NTRK && !fx_safe;
        gen_with(1, gp.seed, 50, 1);
        fx_safe = SAFE_TEX | SAFE_ARP;
        system_init();
        ok_new = safe_part[0] == NTRK && safe_part[1] == NTRK && !fx_safe;
        gen_with(1, gp.seed, 50, 1);
        ok_regen &= safe_part[0] < NTRK;
        fx_safe = SAFE_TEX;
        ok_sec = !section_load(0) && safe_part[0] == NTRK && safe_part[1] == NTRK && !fx_safe;
        gen_with(1, gp.seed, 50, 1);
        k = safe_part[0];
        (void)dice_seq(&trk[k % NTRK]);
        ok_dice = k < NTRK && safe_part[0] == NTRK;
        {   /* (3c-2 final review 8) CPU TEST after a GEN piece: none of its tracks the net may hush (its figures the real ones) */
            uint32_t a0;
            gen_with(1, gp.seed, 50, 1);
            a0 = safe_part[0] < NTRK || safe_part[1] < NTRK;
            fx_safe = SAFE_TEX | SAFE_ARP;
            cpu_test_song();
            ok_cpt = a0 && safe_part[0] == NTRK && safe_part[1] == NTRK && !fx_safe;
            if (!ok_cpt) printf("gen:   CPU TEST after GEN: armed %u, safe_part %u %u, fx_safe %u\n", a0, safe_part[0], safe_part[1], fx_safe);
        }
        fx_safe = 0;
        printf("gen: (0.5.2) the safety net's parts: %u of 60 pieces with one, %u not the playing texture / arp\n", armed, m1_bad);
        bad += gcheck("(0.5.2 review M1) the net's parts: the piece's texture and arp, only if it plays them", !m1_bad && armed > 10u);
        bad += gcheck("  (review I1) none, the levers off, after UNDO, NEW PROJECT, a section load (a project load), a part diced "
                      "on the track; GEN again: back", ok_undo && ok_new && ok_sec && ok_dice && ok_regen);
        bad += gcheck("  (3c-2 final review 8) none, the levers off, after CPU TEST (its load the real one)", ok_cpt);
        mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* (Braindancer 0.8, review ruling 1) TAPE and GEN: GEN never cuts the music to pay for TAPE (TAPE is the safety
         * net's first rung: it goes before any part). With TAPE on (AGE 50, set before GEN), the same piece as with it off:
         * every part's steps and engine, the parts played (texture, arp, lead ...), the reverb, the delay's TIME and
         * grain, the PLAYHEADs, GEN's estimate; TAPE's six as they were. EXPLORE 3423 and others, RANDOM, a genre; one
         * core and two (CPU TEST 74 %, and unset) */
        static const struct { uint8_t g; uint16_t sd; } PC[] = {{GEN_EXPLORE, 3423}, {GEN_EXPLORE, 9641}, {GEN_EXPLORE, 77},
                                                                 {GEN_EXPLORE, 1234}, {0, 31}, {2, 97}};
        uint32_t c, n, k, q, h, same = 0, total = 0, kept = 1, over = 0;
        for (c = 0; c < 4u; c++)
            for (n = 0; n < sizeof PC / sizeof PC[0]; n++) {
                uint32_t sig[2], est[2], j;
                for (j = 0; j < 2u; j++) {
                    mc_host_v12 = (uint8_t)(c & 1u); mc_off = (uint8_t)!(c & 1u);   /* (one core: CORES 1) */
                    gt_cpt = c & 2u ? 0u : 74u; gen_reset();
                    if (j) tape_age_apply(50);
                    gen_with(PC[n].g, PC[n].sd, 50, 1);
                    sig[j] = all_sig() ^ (uint32_t)song.g[G_DTIME] * 0x9e3779b9u ^ (uint32_t)song.g[G_RTYPE] << 20 ^
                             (uint32_t)(song.g[G_DPIT] != 0) << 27 ^ (uint32_t)(song.g[G_DSPRY] != 0) << 28;
                    for (q = 0; q < 6u; q++) sig[j] = sig[j] * 31u + (gs.act[q] != 0);
                    for (k = 0; k < NTRK; k++) for (h = 0; h < FG_HEADS; h++) sig[j] = sig[j] * 3u + (fg_set[k][h].rate != 0);
                    est[j] = gc_cost(1);
                    if (j) {
                        kept &= song.g[G_TAGE] == 50 && song.g[G_THISS] == 20 && tape_set();
                        over += est[j] > gc_budget();
                        for (k = 0; k < 6u; k++) song.g[G_TAGE + k] = 0;
                    }
                }
                total++;
                same += sig[0] == sig[1] && est[0] == est[1];
                if (sig[0] != sig[1] || est[0] != est[1])
                    printf("gen:   (0.8) TAPE on changed the piece: genre %u seed %u, cores %u, CPU TEST %u: estimate %u -> %u\n",
                           PC[n].g, PC[n].sd, (c & 1u) + 1u, c & 2u ? 0u : 74u, est[0], est[1]);
            }
        printf("gen: (0.8) TAPE on (AGE 50) before GEN: %u of %u pieces the same as with TAPE off (%u over the budget either way: "
               "the net's TAPE rung carries TAPE)\n", same, total, over);
        bad += gcheck("(0.8) GEN doesn't pay for TAPE: with it on, the same piece (parts, delay, reverb, heads), the same estimate",
                      same == total && total == 24u);
        bad += gcheck("  .. TAPE's six as they were (GEN never changes TAPE)", kept);
        mc_host_v12 = 0; mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    printf(bad ? "gen: %d FAILED\n" : "gen: all passed\n", bad);
    return bad ? 1 : 0;
}
