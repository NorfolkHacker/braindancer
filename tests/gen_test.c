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
/* (0.9 spec 3, pre-flight 20) the model's x1.00 on two cores for every piece: a style's 41 / 63 x 1.18 (0.77) and WILD's
 * 41 / 63 x 1.55 (1.00), both 1.00 at the floor; the budget 1030. Where a check measures music, not the budget: every
 * piece is fitted now (TAPE counted), and on one core no style's piece fits at all (gen.c gc_budget: 594 at most) */
#define GT_X1 41u
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
/* (0.9) a piece's parts as played, and with TAPE: the steps and engines, the delay, the reverb, the grain, the parts
 * played, the PLAYHEADs; piece_sig adds the swing and TAPE's six */
static uint32_t parts_sig(void)
{
    uint32_t h = all_sig() ^ (uint32_t)song.g[G_DTIME] * 0x9e3779b9u ^ (uint32_t)song.g[G_RTYPE] << 20 ^
                 (uint32_t)(song.g[G_DPIT] != 0) << 27 ^ (uint32_t)(song.g[G_DSPRY] != 0) << 28, k, q;
    for (q = 0; q < 6u; q++) h = h * 31u + (gs.act[q] != 0);
    for (k = 0; k < NTRK; k++) for (q = 0; q < FG_HEADS; q++) h = h * 3u + (fg_set[k][q].rate != 0);
    return h;
}
static uint32_t piece_sig(void)
{
    uint32_t h = parts_sig(), k;
    h = (h ^ (uint32_t)(uint16_t)song.g[G_SWING]) * 16777619u;
    for (k = G_TAGE; k <= G_TTONE; k++) h = (h ^ (uint32_t)(uint16_t)song.g[k]) * 16777619u;
    return h;
}
static uint32_t gt_style(const char *name)              /* a shown style's GENRE index by its screen name */
{
    uint32_t i;
    for (i = 0; i < GEN_NSHOWN; i++) if (!strcmp(GEN_SHOWN_NAME[i], name)) return GEN_SHOWN[i];
    printf("gen: no style %s\n", name);
    return GID_SOLO;
}
static void gid_set(genid_t *id, uint32_t main, uint32_t part, uint32_t seed)
{
    gid_now(id);
    id->main = (uint8_t)main; id->part = (uint8_t)part; id->seed = (uint16_t)(seed % 10000u);
}
/* spec 2's table, by name: main, then its partners */
static const char *const PT[GEN_NSHOWN][7] = {
    {"IDM", "GLITCH HOP", "MICRO", "RETROBOC", "AMBIENT", "DRILL'N'BASS", "ELECTRO"},
    {"GLITCH HOP", "IDM", "BREAKS", "MICRO", "ELECTRO"}, {"BREAKS", "GLITCH HOP", "DRILL'N'BASS", "ACID", "ELECTRO"},
    {"DRILL'N'BASS", "IDM", "BREAKS"}, {"ACID", "BREAKS", "ELECTRO", "AMB TECHNO"},
    {"ELECTRO", "IDM", "GLITCH HOP", "BREAKS", "ACID"}, {"MICRO", "IDM", "GLITCH HOP", "AMB TECHNO"},
    {"RETROBOC", "IDM", "AMBIENT"}, {"AMBIENT", "IDM", "RETROBOC", "AMB TECHNO"}, {"AMB TECHNO", "ACID", "MICRO", "AMBIENT"}};
static int pt_ok(uint32_t main, uint32_t part)          /* part one of main's partners in spec 2's table */
{
    uint32_t i, j;
    for (i = 0; i < GEN_NSHOWN; i++)
        if (gt_style(PT[i][0]) == main)
            for (j = 1; j < 7u && PT[i][j]; j++) if (gt_style(PT[i][j]) == part) return 1;
    return 0;
}
/* (0.9 review I1) the state a piece leaves, whole: every track's parameters, sound and steps; the project as stored
 * (motion, the punch-in lane, the song's globals); ARRANGE's rows; LOOP / SONG; the section; the messages */
typedef struct {
    uint32_t trk_h, step_h, proj_h, chain_h, mode_h, msg_h;
} snap_t;
static uint32_t snap_bytes(uint32_t h, const void *p, uint32_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    while (n--) h = (h ^ *b++) * 16777619u;
    return h;
}
static void snap_take(snap_t *s)
{
    uint32_t k;
    memset(s, 0, sizeof *s);
    s->trk_h = s->step_h = s->proj_h = s->chain_h = s->mode_h = s->msg_h = 2166136261u;
    for (k = 0; k < NTRK; k++) {
        s->trk_h = snap_bytes(s->trk_h, trk[k].p, sizeof trk[k].p);
        s->trk_h = snap_bytes(s->trk_h, &trk[k].eng_req, 1); s->trk_h = snap_bytes(s->trk_h, &trk[k].preset, 1);
        s->trk_h = snap_bytes(s->trk_h, &trk[k].user, 1);
        s->step_h = snap_bytes(s->step_h, trk[k].step, sizeof trk[k].step);
    }
    s->trk_h = snap_bytes(s->trk_h, fg_set, sizeof fg_set);
    s->trk_h = snap_bytes(s->trk_h, song.g, sizeof song.g);
    project_capture(&proj_scratch);
    memset(&proj_wire, 0, sizeof proj_wire);
    if (proj_pack(&proj_wire, &proj_scratch)) s->proj_h = snap_bytes(s->proj_h, &proj_wire, sizeof proj_wire);
    s->chain_h = snap_bytes(s->chain_h, &chain_config, sizeof chain_config);
    s->mode_h = snap_bytes(s->mode_h, (const void *)&live.mode, 1);
    s->mode_h = snap_bytes(s->mode_h, (const void *)&live.cur, 1);
    s->mode_h = snap_bytes(s->mode_h, &ui.song_row, sizeof ui.song_row);
    s->msg_h = snap_bytes(s->msg_h, ui.msg, str_len(ui.msg));
    s->msg_h = snap_bytes(s->msg_h, ui.msg2, str_len(ui.msg2));
    s->msg_h = snap_bytes(s->msg_h, gp.last, str_len(gp.last));
}
static uint32_t snap_diff(const snap_t *a, const snap_t *b)
{
    return (uint32_t)(a->trk_h != b->trk_h) | (uint32_t)(a->step_h != b->step_h) << 1 | (uint32_t)(a->proj_h != b->proj_h) << 2 |
           (uint32_t)(a->chain_h != b->chain_h) << 3 | (uint32_t)(a->mode_h != b->mode_h) << 4 | (uint32_t)(a->msg_h != b->msg_h) << 5;
}
static const char *snap_what(const snap_t *a, const snap_t *b)
{
    static char w[64];
    uint32_t d = snap_diff(a, b);
    w[0] = 0;
    if (d & 1u) strcat(w, "tracks ");
    if (d & 2u) strcat(w, "steps ");
    if (d & 4u) strcat(w, "project ");
    if (d & 8u) strcat(w, "ARRANGE ");
    if (d & 16u) strcat(w, "LOOP/SONG ");
    if (d & 32u) strcat(w, "messages");
    return w;
}
static int gid_eq_t(const genid_t *a, const genid_t *b) { return a->seed == b->seed && a->main == b->main && a->part == b->part && a->energy == b->energy && a->complex == b->complex && a->form == b->form && a->tape == b->tape && a->dev == b->dev && a->two == b->two; }
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

#define GEN_HOUSE_HASH 153064764u    /* (all_sig of HOUSE 0421, energy 50, one core, CPU TEST unset; 0.9: re-recorded at
                                     * 7568d5c: every piece fitted, spec 6: the base's piece with its chords at two notes,
                                     * the arp, the lead and the texture left out, the texture's REL 84, a chord's ARP
                                     * NOTES cleared) */
#define GEN_HOUSE_HASH_X1 2027450874u   /* (the same at the model's x1.00, two cores: nothing to fit, the piece captured on
                                         * the base commit a4fcac7 unchanged) */
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
    {   /* (0.9 spec 2) GENERATE: the identity makes the piece; the picker; the blends */
        genid_t a, b, d;
        uint32_t h1, h2, h3, i, j, pr, wild = 0, blend = 0, rep = 0, badp = 0, nonw = 0, cnt[GEN_NSHOWN] = {0}, seenp = 0, ok;
        char nm[32];
        gen_reset();
        gid_set(&a, gt_style("RETROBOC"), gt_style("AMBIENT"), 421);
        gen_build(&a); h1 = piece_sig();
        ok = !strcmp(gp.last, "RETROBOC+AMBIENT 0421") && !strcmp(ui.msg, "GEN: RETROBOC+AMBIENT 0421") && gs.mutant;
        gid_set(&b, gt_style("DRILL'N'BASS"), GID_SOLO, 77); gen_build(&b);
        gen_build(&a); h2 = piece_sig();
        d = a; d.part = GID_SOLO; gen_build(&d); h3 = piece_sig();
        bad += gcheck("(0.9) the same identity twice the same piece (RETROBOC+AMBIENT 0421, another between); solo another", h1 == h2 && h1 != h3);
        bad += gcheck("  named on the panel and the status line: RETROBOC+AMBIENT 0421; solo RETROBOC 0421", ok && !strcmp(gp.last, "RETROBOC 0421"));
        d.main = GID_WILD; gen_build(&d);
        bad += gcheck("  WILD: the EXPLORE generator, named WILD 0421", gx.on && !strcmp(gp.last, "WILD 0421"));
        for (ok = 1, i = 0; i < GEN_NSHOWN; i++) {      /* the firmware's table is spec 2's */
            uint32_t n = 0;
            for (j = 1; j < 7u && PT[i][j]; j++) n++;
            ok &= GEN_NPARTNER[gen_shown_index(gt_style(PT[i][0]) + 1u)] == n;
            for (j = 0; j < GEN_NPARTNER[i]; j++) {
                uint32_t q;
                ok &= pt_ok(GEN_SHOWN[i], GEN_SHOWN[GEN_PARTNER[i][j]]);
                for (q = 0; q < j; q++) ok &= GEN_PARTNER[i][q] != GEN_PARTNER[i][j];   /* (review M3: no partner twice) */
            }
        }
        bad += gcheck("  GEN_PARTNER is spec 2's table (every row, every partner, none twice)", ok);
        gen_reset();
        for (pr = 0; pr < 2000u; pr++) {                /* 2,000 presses' draws (the identity only, then as if built) */
            genid_t id;
            uint32_t prev = gp.id.main, si;
            host_ticks += 1300000u + pr * 777u;
            gen_draw(&id);
            if (id.main == GID_WILD) { wild++; badp += id.part != GID_SOLO; gp.id = id; continue; }
            nonw++;
            si = gen_shown_index(id.main + 1u);
            if (si >= GEN_NSHOWN) { badp++; continue; }
            cnt[si]++;
            rep += id.main == prev;
            if (id.part != GID_SOLO) { blend++; badp += !pt_ok(id.main, id.part); seenp |= 1u << gen_shown_index(id.part + 1u); }
            gp.id = id;
        }
        for (ok = 1, i = 0; i < GEN_NSHOWN; i++) ok &= cnt[i] * 100u >= nonw * 7u && cnt[i] * 100u <= nonw * 13u;
        printf("gen: (0.9) 2000 presses: WILD %u, blends %u of %u, the same main twice running %u, outside the table %u\n",
               wild, blend, nonw, rep, badp);
        bad += gcheck("  WILD about 10 % (7..13 %), never blended", wild >= 140u && wild <= 260u);
        bad += gcheck("  blends about 45 % of the rest (40..50 %), partners only from the table, every style a partner", blend * 100u >= nonw * 40u &&
                      blend * 100u <= nonw * 50u && !badp && seenp == (1u << GEN_NSHOWN) - 1u);
        bad += gcheck("  the main evenly (7..13 % each), never the same main as the press before", ok && !rep);
        for (ok = 1, i = 0; i < GEN_NSHOWN; i++)       /* a blend: the main's tempo, swing and drums, the partner's bass */
            for (j = 0; j < GEN_NPARTNER[i]; j++) {
                uint32_t mi = GEN_SHOWN[i], pa = GEN_SHOWN[GEN_PARTNER[i][j]];
                gid_set(&d, mi, pa, 100u * i + j + 3u); gen_build(&d);
                ok &= song.g[G_BPM] >= GEN_SHOWN_TEMPO[i][0] - 3 && song.g[G_BPM] <= GEN_SHOWN_TEMPO[i][1] + 3 &&
                      song.g[G_SWING] == GEN_SHOWN_TEMPO[i][2] && gs.g->kit == GENRE[mi].kit && gs.g->bass_lo == GENRE[pa].bass_lo &&
                      gs.g->kick_mode == GENRE[pa].kick_mode && gs.mutant;
                gid_name(&d, nm, sizeof nm, 1);         /* (the short form: the fallback, pre-flight 17) */
                ok &= text_w(&UF_HDR, ui.msg) <= 232 && str_len(gp.last) + 1u < sizeof gp.last && strchr(gp.last, '+') &&
                      (strstr(gp.last, GEN_SHOWN_NAME[i]) == gp.last || !strcmp(gp.last, nm));
            }
        bad += gcheck("  every blend: the main's tempo, swing and kit, the partner's bass; its name whole and on the status line", ok);
        {   /* the CPU factor is the identity's: rebuilt after CPU TEST changed, the same piece and budget */
            uint32_t b0, s0;
            mc_host_v12 = 1; gt_cpt = 74; gen_reset();
            gid_set(&d, gt_style("IDM"), gt_style("MICRO"), 9641); gen_build(&d); s0 = piece_sig(); b0 = gc_budget();
            cpt_res = 0; cpt_cores = 0;
            gen_build(&d);
            bad += gcheck("  the CPU factor in the identity: CPU TEST forgotten, the same piece and budget rebuilt", piece_sig() == s0 &&
                          gc_budget() == b0 && gc_dev_base() != d.dev);
            {   /* (0.9 review M6) WILD's own factor (x1.55) from the stored ratio: the same */
                uint32_t wb, ws;
                cpt_res = 74; cpt_cores = 2;
                gid_set(&d, GID_WILD, GID_SOLO, 3423); gen_build(&d); ws = piece_sig(); wb = gc_budget();
                cpt_res = 0; cpt_cores = 0;
                gen_build(&d);
                bad += gcheck("  .. WILD (its own x1.55 from the stored CPU TEST): the same piece and budget rebuilt",
                              piece_sig() == ws && gc_budget() == wb && wb == 569u && gc_dev_base() != d.dev);
            }
            mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
        }
        {   /* the tools' way in: gen_go with gp.genre 0 draws from the seed (the same seed, the same draw) */
            uint32_t s1, s2;
            gen_reset();
            gen_with(0, 31, 50, 0); s1 = piece_sig(); a = gp.id;
            gen_with(1, 7, 50, 0);
            gen_with(0, 31, 50, 0); s2 = piece_sig();
            bad += gcheck("  gen_go, genre 0 (the tools'): a draw from the seed, deterministic", s1 == s2 && gid_eq_t(&a, &gp.id));
        }
    }
    {   /* (0.9 spec 2, review I1) a piece depends only on its identity: built fresh (after the power-on) and after
         * other pieces of every kind (drums or none, LOOP / PARTS / SONG, a blend, WILD), the same tracks to the last
         * parameter (AUTO PAN's too), the same steps, the same project as stored, ARRANGE, LOOP / SONG, the messages */
        static const struct { const char *m, *p; uint16_t seed; uint8_t form, wild; } TG[] = {
            {"AMBIENT", "IDM", 3901, 0, 0}, {"RETROBOC", "AMBIENT", 421, 1, 0}, {"IDM", 0, 421, 0, 0}, {"BREAKS", 0, 421, 0, 0},
            {"DRILL'N'BASS", 0, 77, 2, 0}, {"ACID", 0, 5, 2, 0}, {"MICRO", "AMB TECHNO", 1234, 1, 0}, {0, 0, 421, 2, 1},
            {0, 0, 99, 0, 1}, {"AMB TECHNO", 0, 9999, 1, 0}};
        static const struct { const char *m, *p; uint16_t seed; uint8_t form, wild; } PV[] = {
            {"ACID", 0, 5, 1, 0}, {"DRILL'N'BASS", 0, 77, 2, 0}, {0, 0, 99, 2, 1}, {"IDM", 0, 77, 0, 0},
            {"BREAKS", "ELECTRO", 6060, 2, 0}, {"AMBIENT", 0, 31, 1, 0}};
        uint32_t t, v, c, n = 0, diff = 0;
        uint32_t why[8] = {0};
        uint32_t rng0 = rng_state;                      /* (the session's random numbers as they were after: the blocks
                                                         * below measure what they did before this one) */
        for (c = 0; c < 2u; c++)
            for (t = 0; t < NELEM(TG); t++) {
                genid_t id;
                snap_t s0;
                gen_reset(); mc_host_v12 = (uint8_t)c;
                gid_set(&id, TG[t].wild ? GID_WILD : gt_style(TG[t].m), TG[t].p ? gt_style(TG[t].p) : GID_SOLO, TG[t].seed);
                id.form = TG[t].form;
                gen_build(&id); snap_take(&s0);
                for (v = 0; v < NELEM(PV); v++) {
                    genid_t pv;
                    snap_t s1;
                    uint32_t w;
                    char pn[28];
                    gen_reset(); mc_host_v12 = (uint8_t)c;
                    gid_set(&pv, PV[v].wild ? GID_WILD : gt_style(PV[v].m), PV[v].p ? gt_style(PV[v].p) : GID_SOLO, PV[v].seed);
                    pv.form = PV[v].form;
                    gen_build(&pv); str_cpy(pn, gp.last, sizeof pn);
                    gen_build(&id); snap_take(&s1);
                    n++;
                    w = snap_diff(&s0, &s1);
                    if (w) {
                        diff++;
                        if (diff <= 8u) printf("  %s (%s) after %s: differs (%s)\n", gp.last, c ? "two cores" : "one core", pn, snap_what(&s0, &s1));
                        for (; w; w &= w - 1u) why[__builtin_ctz(w)]++;
                    }
                }
            }
        mc_host_v12 = 0; rng_state = rng0;
        printf("gen: (0.9) %u rebuilds after another piece: %u differ (tracks %u, steps %u, project %u, ARRANGE %u, LOOP/SONG %u, messages %u)\n",
               n, diff, why[0], why[1], why[2], why[3], why[4], why[5]);
        bad += gcheck("(0.9) a piece depends only on its identity: the same after any piece before it (every parameter, ARRANGE, LOOP)", !diff && n == 120u);
    }
    {   /* (0.9 re-review N1) the mutes live when GENERATE is pressed: a section's (B playing: the drums and the bass muted)
         * or the user's MUTE on a track. Neither changes the piece nor its fit; no MUTE left on any track. Two cores, CPU
         * TEST 74 %, PARTS pieces, styles and blends */
        static const struct { const char *m, *p; uint16_t seed; } MG[] = {
            {"RETROBOC", 0, 421}, {"IDM", 0, 77}, {"BREAKS", "ACID", 1234}, {"AMBIENT", "IDM", 3901}, {"ACID", 0, 5}, {"MICRO", "AMB TECHNO", 9}};
        uint32_t t, w, n = 0, diff = 0, muted = 0, k;
        uint32_t rng0 = rng_state;
        for (t = 0; t < NELEM(MG); t++) {
            genid_t id, pv;
            snap_t s0, s1;
            mc_host_v12 = 1; gt_cpt = 74; gen_reset();
            gid_set(&id, gt_style(MG[t].m), MG[t].p ? gt_style(MG[t].p) : GID_SOLO, MG[t].seed);
            id.form = 1u; id.energy = 80; id.complex = 70;
            gen_build(&id); snap_take(&s0);
            for (w = 0; w < 2u; w++) {
                gen_reset();
                if (!w) {                               /* (a PARTS piece before, its section B playing: mutes live) */
                    gid_set(&pv, gt_style("IDM"), GID_SOLO, 600u + t); pv.form = 1u; gen_build(&pv);
                    section_load(1);
                    for (k = 0, muted = 0; k < NTRK; k++) muted += trk[k].p[P_MUTE] != 0;
                    if (!muted) printf("  (no mute in section B before %s)\n", MG[t].m);
                } else trk[(t + 1u) % NTRK].p[P_MUTE] = 1;   /* (the user's MUTE) */
                gen_build(&id); snap_take(&s1);
                n++;
                for (k = 0, muted = 0; k < NTRK; k++) muted += trk[k].p[P_MUTE] != 0;
                if (snap_diff(&s0, &s1) || muted) {
                    diff++;
                    if (diff <= 4u) printf("  %s %s: differs (%s), %u tracks muted\n", gp.last, w ? "after a user's MUTE" : "built in section B",
                                          snap_what(&s0, &s1), muted);
                }
            }
        }
        mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0; rng_state = rng0;
        bad += gcheck("(0.9 N1) the mutes live at the press (a section's, the user's): the same piece, no MUTE left", n == 12u && !diff);
    }
    {   /* (0.9 spec 4) recent pieces: 9 presses keep 8, newest first; each brought back exactly (the full snapshot: every
         * parameter, step, the project as stored, ARRANGE, LOOP / SONG, the messages), the oldest each time, moved to the
         * front, never twice, UNDO marked; GENERATE never repeats a recalled piece's main; a restart clears. Once with no
         * CPU TEST stored, once at a stored 74 % that is forgotten before the rebuilds */
        uint32_t c, pr, j, ok, okb, rep = 0, okc = 1;
        uint32_t rng0 = rng_state;
        for (c = 0; c < 2u; c++) {
            genid_t ids[9];
            snap_t sn[9], sr;
            ok = okb = 1;
            mc_host_v12 = (uint8_t)c; gt_cpt = c ? 74u : 0u;
            gen_reset(); session_dice(4242u);
            for (pr = 0; pr < 9u; pr++) {
                host_ticks += 1300000u + pr * 777u;
                gen_generate();
                ids[pr] = gp.id; snap_take(&sn[pr]);
            }
            ok &= gen_nrecent == 8u && gp.at == 0u && (!c || (gc_dev_base() == 117u && gp.id.dev == 117u));   /* (74 % of the model's 63: 117) */
            for (j = 0; j < 8u; j++) ok &= gid_eq(&gen_recent[j], &ids[8u - j]);
            bad += gcheck(c ? "(0.9) recent pieces, CPU TEST 74 %: 9 presses keep the last 8, newest first" :
                              "(0.9) recent pieces: 9 presses keep the last 8, newest first", ok);
            if (c) { cpt_res = 0; cpt_cores = 0; }
            for (j = 0; j < 8u; j++) {
                undo_ok = 0;
                gen_recall(gen_nrecent - 1u);
                snap_take(&sr);
                if (snap_diff(&sn[1u + j], &sr)) {
                    okb = 0;
                    printf("  recall %u (%s): differs (%s)\n", j, gp.last, snap_what(&sn[1u + j], &sr));
                }
                okb &= gid_eq(&gen_recent[0], &ids[1u + j]) && gen_nrecent == 8u && undo_ok;
            }
            for (j = 1; j < 8u; j++) okb &= !gid_eq(&gen_recent[j], &gen_recent[0]);
            bad += gcheck("  each brought back (the oldest each time): the same piece to the last parameter, to the front, never twice, UNDO marked", okb);
            /* a middle one too: recall 3, then the ring's order is that piece first, the rest as they were */
            {
                genid_t was[8], mid;
                for (j = 0; j < 8u; j++) was[j] = gen_recent[j];
                mid = was[3];
                gen_recall(3);
                okc &= gid_eq(&gen_recent[0], &mid) && gen_nrecent == 8u;
                for (j = 1; j < 4u; j++) okc &= gid_eq(&gen_recent[j], &was[j - 1u]);
                for (j = 4; j < 8u; j++) okc &= gid_eq(&gen_recent[j], &was[j]);
            }
        }
        bad += gcheck("  a middle one brought back: first, the newer ones one place down, the older ones where they were", okc);
        for (pr = 0; pr < 50u; pr++) {
            genid_t id;
            gen_recall(3);
            host_ticks += 990000u;
            gen_draw(&id);
            rep += id.main != GID_WILD && id.main == gp.id.main;
        }
        bad += gcheck("  GENERATE after a piece brought back: never its main", !rep);
        gen_recall(99);                                 /* (beyond the ring: nothing happens) */
        bad += gcheck("  a recall beyond the ring does nothing", gen_nrecent == 8u);
        session_dice(77u);
        bad += gcheck("  a restart (the power-on's session_dice) clears them, PRESETS on NEW", gen_nrecent == 0u && gp.at == 0u);
        mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0; rng_state = rng0;
    }
    {   /* (0.9 re-review N2) the SEQ dice on a chord track not capped by the last piece's fit (gx.nmax): after a piece
         * whose chords the fitter left at two notes (one core, CPU TEST unset: the floor), the dice's chords as many as
         * they come; the same where no piece is (the track's key changed) */
        uint32_t pd, i, big = 0, big2 = 0, a2 = 1, tries;
        uint32_t rng0 = rng_state;
        gen_reset();
        gen_with(1, 421, 50, 0);                        /* (HOUSE 0421: its chords two notes, GEN_HOUSE_HASH) */
        pd = gs.trk_of[RL_PAD];
        for (i = 0; pd < NTRK && i < dlen(&trk[pd]); i++) a2 &= trk[pd].step[i].n <= 2u;
        for (tries = 0; pd < NTRK && tries < 6u; tries++) {
            host_ticks += 990000u;
            dice_seq(&trk[pd]);
            for (i = 0; i < dlen(&trk[pd]); i++) big += trk[pd].step[i].time == ST_NOTE && trk[pd].step[i].n > 2u;
        }
        for (tries = 0; pd < NTRK && tries < 6u; tries++) {
            trk[pd].p[P_ROOT] = (int16_t)((gs.root + 5) % 12);   /* (no piece here: its choices made up) */
            host_ticks += 990000u;
            dice_seq(&trk[pd]);
            for (i = 0; i < dlen(&trk[pd]); i++) big2 += trk[pd].step[i].time == ST_NOTE && trk[pd].step[i].n > 2u;
        }
        rng_state = rng0;
        printf("gen: (0.9 N2) SEQ + SELECT x 6 on HOUSE 0421's chords (two notes as fitted): %u steps of 3 notes or more; no piece there: %u\n", big, big2);
        bad += gcheck("(0.9 N2) the SEQ dice's chords not capped by the last piece's fit (and where no piece is)", pd < NTRK && a2 && big && big2);
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
        mc_host_v12 = 1; cpt_res = GT_X1; cpt_cores = 2;   /* (0.9, pre-flight 20: the music measured, not the budget) */
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
        mc_host_v12 = 0; cpt_res = 0; cpt_cores = 0;
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
                if (!strchr(gp.last, '+') || gs.g->bpm_hi < gs.g->bpm_lo || strncmp(ui.msg, "GEN: ", 5)) bad_mut++;
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
        {   /* (Braindancer 0.8) GEN chooses among 0.7's presets only: 40 seeds x every genre */
            uint32_t gg, sd2, j, over = 0, seen = 0;
            for (gg = 0; gg < NGENRE; gg++)
                for (sd2 = 0; sd2 < 40u; sd2++) {
                    gen_with(gg + 1u, sd2 * 17u + gg, 50, 0);
                    for (j = 0; j < gs.nsnd; j++) {
                        uint32_t e2 = gs.snd[j] >> 8, k2 = gs.snd[j] & 255u;
                        seen++;
                        if (e2 >= NENGINES || k2 >= gen_npre(ENGINES[e2])) over++;
                    }
                }
            printf("gen: preset indices: %u picks, %u past 0.7's\n", seen, over);
            bad += gcheck("  (0.8) 40 seeds x every genre: no track's preset index past 0.7's (gen_npre)", seen > 0u && !over);
        }
        bad += gcheck("EXPLORE (0.8.6): GENRE EXPLORE at power-on", genre0 == GEN_EXPLORE);
        gen_reset();
        gt_cpt = GT_X1;                                 /* (0.9.14) the device as the model holds it (gen.c gc_dev x1.00): the
                                                         * pieces 0.9.13 made; the default's (x1.60) after these. (0.9:
                                                         * GT_X1, CPU TEST 63 % is x1.18 / x1.55 on two cores) */
        cpt_res = GT_X1; cpt_cores = 2;
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
            if (strncmp(ui.msg, "GEN: WILD ", 10) || strcmp(gs.g->name, "EXPLORE")) name_bad++;   /* (0.9: WILD on the screen) */
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
        bad += gcheck("  60 seeds: each named WILD nnnn, each another piece (no two the same)", !name_bad && !same);
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
             * 42 %, the pieces compared against it; one core as before. (0.9 spec 3's recalibration) Two cores unset: a
             * style's piece x1.50 (gc_dev), WILD's x1.96 (these pieces); a CPU TEST x1.18 its ratio a style's, x1.55 WILD's:
             * at 42 % a style's x1.00 (0.77, the floor), these WILD pieces' x1.02 (66 x 1.55) */
            uint32_t est[2] = {0}, over = 0, mel_bad = 0, quiet2 = 0, d0, d1, d2, d3, d4, w0;
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
            w0 = gcf.dev;                               /* (the last piece: WILD, two cores, unset) */
            cpt_res = 42; cpt_cores = 2; d4 = gc_dev();
            mc_host_v12 = 0;
            cpt_res = 63; d1 = gc_dev(); cpt_res = 126; d2 = gc_dev(); cpt_res = 30; d3 = gc_dev();
            cpt_res = 0; cpt_cores = 0; gt_cpt = GT_X1;
            printf("gen: (0.9.14) EXPLORE x 60 at COMPLEX 50, two cores: the device x%u.%02u by default: CPU %u thousandths of CPU TEST\n"
                   "gen:   on average (x1.00: %u), %u at the floor over the budget %u; x%u.%02u at 63 %%, x%u.%02u at 126 %%, x%u.%02u at 30 %%\n",
                   d0 / 100u, d0 % 100u, est[1] / 60u, est[0] / 60u, over, (uint32_t)(1030u * 100u / d0), d1 / 100u, d1 % 100u,
                   d2 / 100u, d2 % 100u, d3 / 100u, d3 % 100u);
            bad += gcheck("  (0.9.14) the device factor: x1.50 / WILD x1.96 without a CPU TEST (two cores; one: x1.00 at 63 %, "
                          "x2.00 at 126 %), lighter pieces, still two melodic parts and sound",
                          d0 == 150u && w0 == 196u && d4 == 100u && d1 == 100u && d2 == 200u && d3 == 100u && est[1] < est[0] * 3u / 4u && !mel_bad && !quiet2);
            cpt_res = GT_X1; cpt_cores = 2;
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
                /* (0.9, review M2) the seeds past 9999 (uint16 already wrapped them) fold to seed % 10000 in the identity
                 * (gen.c gid_now): EXPLORE's 1200 are other in-range pieces than 0.8's, a few the same twice */
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
                        if (nz <= 5u) printf("  NOISE on role %u: %s seed %u T%u\n", r, gs.g->name, gp.id.seed, k + 1u);
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
        mc_host_v12 = 1; gt_cpt = GT_X1;                /* (0.9, pre-flight 20: the lead measured, not the budget) */
        for (gi = 0; gi < 6u; gi++) {
            gen_reset();
            gen_with(GG[gi], 4321u + gi, 50, 0);
            memset(prev, 0, sizeof prev);
            for (k = 0; k < NTRK; k++) lastsnd[k] = 0xFFFF;
            for (pr = 0; pr < 24u; pr++) {
                host_ticks += 1300000u + pr * 777u;
                gen_new_seed(); gen_go();               /* (0.9: the style's own press, a new seed: 0.8's OCT+ on a set
                                                         * GENRE; OCT+ on the page is GENERATE now, any style) */
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
        mc_host_v12 = 0; gt_cpt = 0;
        {   /* the power-on: the first OCT+ of two sessions */
            uint32_t h0;
            gen_reset(); session_dice(111u); gen_press(); h0 = dice_print();
            gen_reset(); session_dice(222u); gen_press(); po_same = dice_print() == h0;
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
    {   /* (0.9 spec 1) the GEN page: one page, ENERGY FORM COMPLEX TAPE; OCT+ GENERATE; PRESETS NEW then the recent
         * pieces (OCT+ brings one back); SELECT nothing; OCT- TRACKS; no GENRE, no SEED; NEW PROJECT keeps it */
        static const char *const LBL[4] = {"ENERGY", "FORM", "COMPLEX", "TAPE"};
        static const char *const TV[13] = {"AUTO", "OFF", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", "100%"};
        char v[40], n0[32];
        uint32_t k, ok = 1, s1, s2, e1, f1, c1, t1;
        gen_reset(); session_dice(9u);
        go_home(); frames(16); fm1_ms += 1200u;
        held_turn(B_HOME, 1);
        ok &= ui.confirm == CF_DICE_ALL && gp.at == 0u && !strcmp(gen_oct_label(), "OCT+ GENERATE");
        for (k = 0; k < 4u; k++) ok &= !strcmp(GEN_LBL[k], LBL[k]) && strcmp(GEN_LBL[k], "GENRE") && strcmp(GEN_LBL[k], "SEED");
        bad += gcheck("(0.9) HOME + SELECT: GEN, one page: ENERGY FORM COMPLEX TAPE (no GENRE, no SEED), PRESETS on NEW, OCT+ GENERATE", ok);
        frames(800);
        gp.tape = GT_AUTO;
        for (ok = 1, k = 0; k < 13u; k++) { gen_value(3, v); ok &= !strcmp(v, TV[k]); turn(EN_K4, 1); }
        for (k = 0; k < 13u; k++) turn(EN_K4, -1);
        gen_value(3, v); ok &= !strcmp(v, "AUTO") && gp.tape == GT_AUTO;
        gp.energy = 50; turn(EN_K1, 2); ok &= gp.energy == 60u; gen_value(0, v); ok &= !strcmp(v, "60%");
        gp.form = 0; turn(EN_K2, 1); gen_value(1, v); ok &= !strcmp(v, "PARTS"); turn(EN_K2, 5); gen_value(1, v); ok &= !strcmp(v, "SONG");
        gp.complex = 50; turn(EN_K3, -1); ok &= gp.complex == 45u;
        bad += gcheck("  KNOB 4 TAPE: AUTO, OFF, 10 .. 100 %, stops; KNOB 1 ENERGY +-5, KNOB 2 FORM, KNOB 3 COMPLEX", ok);
        turn(EN_K2, -1); turn(EN_K2, -1); gp.tape = GT_AUTO;   /* (FORM a place a turn: back to LOOP) */
        press(B_OCTUP); frames(4);
        str_cpy(n0, gp.last, sizeof n0); s1 = piece_sig();
        e1 = gp.id.energy; f1 = gp.id.form; c1 = gp.id.complex; t1 = gp.id.tape;
        ok = ui.confirm == CF_DICE_ALL && !strncmp(ui.msg, "GEN: ", 5) && gen_nrecent == 1u;
        press(B_OCTUP); frames(4); s2 = piece_sig();   /* (again, the knobs untouched: the same settings, a new seed) */
        ok &= gen_nrecent == 2u && s2 != s1 && strcmp(gp.last, n0) && gp.id.seed != gen_recent[1].seed &&
              gp.id.energy == e1 && gp.id.form == f1 && gp.id.complex == c1 && gp.id.tape == t1 &&
              gp.energy == e1 && gp.form == f1 && gp.complex == c1 && gp.tape == t1;
        bad += gcheck("  OCT+: GENERATE (GEN: name seed), the screen stays; again, the knobs untouched: a new seed, the same settings", ok);
        gp.energy = 85; gp.form = 2; gp.complex = 15; gp.tape = 7;   /* (the knobs moved on: a recall brings piece 1's back) */
        frames(800);                                    /* (the knobs quiet a moment after a button: ui_layer.c) */
        turn(EN_PRESET, -1);
        gen_panel_line(v, sizeof v);
        ok = gp.at == 1u && !strncmp(v, "RECENT 1: ", 10) && !strcmp(gen_oct_label(), "OCT+ RECALL");
        turn(EN_PRESET, -1); turn(EN_PRESET, -1);
        ok &= gp.at == 2u;                              /* (clamped at the oldest) */
        {   /* (Task 4 review) RECALL while ARRANGE runs: refused as GENERATE is (OCT+ does nothing; the ring as it was) */
            genid_t r0 = gen_recent[0];
            chain.running = 1;
            press(B_OCTUP); frames(4);
            ok &= piece_sig() == s2 && gp.at == 2u && gen_nrecent == 2u && gid_eq(&gen_recent[0], &r0);
            gp.at = 0;
            press(B_OCTUP); frames(4);
            ok &= piece_sig() == s2 && gen_nrecent == 2u && gid_eq(&gen_recent[0], &r0);
            chain.running = 0; gp.at = 2;
        }
        press(B_OCTUP); frames(4);
        ok &= piece_sig() == s1 && gp.at == 1u && gen_nrecent == 2u && !strcmp(gp.last, n0);
        gen_panel_line(v, sizeof v);
        ok &= str_len(v) > 5u && !strcmp(v + str_len(v) - 5u, n0 + str_len(n0) - 5u) && !strcmp(gen_oct_label(), "OCT+ RECALL");
        ok &= gp.energy == e1 && gp.form == f1 && gp.complex == c1 && gp.tape == t1;   /* (the knobs show the piece's) */
        frames(800);
        turn(EN_PRESET, 5);
        ok &= gp.at == 0u && !strcmp(gen_oct_label(), "OCT+ GENERATE");
        bad += gcheck("  PRESETS back: RECENT 1, 2 (stops at the oldest), OCT+ RECALL brings it back (the same piece, its knobs; refused while ARRANGE runs); on: NEW", ok);
        fm1_ms += 1000u;
        k = nav_cur();
        turn(EN_SELECT, 1); frame(); turn(EN_SELECT, -1); frame();
        bad += gcheck("  SELECT: nothing (GEN is one page)", gen_on() && nav_cur() == k && k == UXH_GEN0 && gp.at == 0u);
        press(B_OCTDN);
        bad += gcheck("  OCT-: back to TRACKS, the piece stays", !ui.confirm && ui.home && home_level() == 0u && piece_sig() == s1);
        tools_run(5); press(B_OCTUP);
        bad += gcheck("  NEW PROJECT: GEN's knobs and recent pieces kept", msg_is("NEW PROJECT") && gen_nrecent == 2u && gp.form == 0u);
        {   /* (Task 5 review) a knob turned on a recent piece: PRESETS back to NEW, OCT+ GENERATE with the turned value; a
             * piece made or brought back with every part amount, the swing and the human as AUTO does (gen_ui_defaults) */
            genid_t r2;
            uint32_t e2, sr, d = 1;
            fm1_ms += 1200u; held_turn(B_HOME, 1); frames(800);
            turn(EN_PRESET, -1); turn(EN_PRESET, -1);
            ok = gen_on() && gp.at == 2u;
            r2 = gen_recent[1];
            for (k = 0; k < GP_N; k++) gp.amt[k] = 40;
            gp.swing = 10; gp.human = 90;
            press(B_OCTUP); frames(4);
            for (k = 0; k < GP_N; k++) d &= gp.amt[k] == GA_AUTO;
            d &= gp.swing == GA_AUTO && gp.human == 30u;
            ok &= gp.at == 1u && gid_eq(&gp.id, &r2) && gp.energy == r2.energy;
            sr = piece_sig(); e2 = (uint32_t)gp.energy + (gp.energy <= 95u ? 5u : -5u);
            frames(800);
            turn(EN_K1, gp.energy <= 95u ? 1 : -1);
            gen_panel_line(v, sizeof v);
            ok &= gp.energy == e2 && gp.at == 0u && !strcmp(gen_oct_label(), "OCT+ GENERATE") && strncmp(v, "RECENT", 6);
            for (k = 0; k < GP_N; k++) gp.amt[k] = 40;
            gp.swing = 10; gp.human = 90;
            press(B_OCTUP); frames(4);
            for (k = 0; k < GP_N; k++) d &= gp.amt[k] == GA_AUTO;
            d &= gp.swing == GA_AUTO && gp.human == 30u;
            ok &= gp.at == 0u && gen_nrecent == 3u && gp.id.energy == e2 && gid_eq(&gen_recent[0], &gp.id) &&
                  !gid_eq(&gp.id, &r2) && piece_sig() != sr;
            bad += gcheck("  (review) a knob turned on RECENT: PRESETS back to NEW, OCT+ GENERATE with it (a new piece, in the ring)", ok);
            bad += gcheck("  (review) GENERATE and RECALL: every part amount, the swing and the human as AUTO (gen_ui_defaults)", d);
            press(B_OCTDN);
        }
    }
    {   /* every name fits: the status line (232 px) and the panel's line beside its label, for every style and blend */
        uint32_t i, j, wide = 0;
        genid_t id;
        char v[40];
        gen_reset();
        for (i = 0; i <= GEN_NSHOWN; i++)
            for (j = 0; j <= (i < GEN_NSHOWN ? GEN_NPARTNER[i] : 0u); j++) {
                gid_now(&id); id.seed = 8888;
                id.main = i < GEN_NSHOWN ? GEN_SHOWN[i] : GID_WILD;
                id.part = i < GEN_NSHOWN && j < GEN_NPARTNER[i] ? GEN_SHOWN[GEN_PARTNER[i][j]] : GID_SOLO;
                gen_build(&id);
                gen_panel_line(v, sizeof v);
                wide += text_w(&UF_HDR, ui.msg) > 232 || text_w(&AF_X, v) > 228 - text_w(&AF_X, gen_oct_label()) - 8 || !strstr(v, " 8888");
            }
        bad += gcheck("(0.9) every style, blend and WILD: the name and seed whole on the status line and the panel", !wide);
    }
    /* ---- the screen: HOME + SELECT opens GEN, the knobs set, SELECT pages, OCT+ makes, OCT- is done */
    {
        gen_reset();
        go_home(); frames(16);
        fm1_ms += 1200u;
        held_turn(B_HOME, 1);
        bad += gcheck("HOME + SELECT: the GEN screen (ui.confirm CF_DICE_ALL)", ui.confirm == CF_DICE_ALL);
        {   /* (3b) the ten shown genres: no random draw reaches a hidden one; a stored hidden genre generates as before */
            uint32_t sd, hid = 0, hh;
            for (sd = 0; sd < 2500u; sd++) {
                gen_with(0, sd, 50, 0);
                hid += gp.id.main != GID_WILD && gen_shown_index(gs.gi + 1u) == 255u;
                hid += gp.id.part != GID_SOLO && gen_shown_index(gp.id.part + 1u) == 255u;
            }
            bad += gcheck("  (0.9) seed draws over 2,500 seeds: only the ten", hid == 0u);
            bad += gcheck("  (3b) gen_shown_index: the ten in order, hidden / RANDOM / EXPLORE 255", gen_shown_index(1u + GEN_SHOWN[0]) == 0u &&
                          gen_shown_index(1u + GEN_SHOWN[9]) == 9u && gen_shown_index(1u) == 255u && gen_shown_index(0u) == 255u &&
                          gen_shown_index(GEN_EXPLORE) == 255u);
            gen_with(1, 421, 50, 0); hh = all_sig();
            {   uint32_t m0 = mc_host_v12, c0 = cpt_res, k0 = cpt_cores, hx;
                mc_host_v12 = 1; cpt_res = GT_X1; cpt_cores = 2;
                gen_with(1, 421, 50, 0); hx = all_sig();
                mc_host_v12 = (uint8_t)m0; cpt_res = (uint8_t)c0; cpt_cores = (uint8_t)k0;
                bad += gcheck("  (3b) a stored hidden genre (HOUSE 0421): the piece as at the base commit; fitted (0.9) at the default",
                              hh == GEN_HOUSE_HASH && hx == GEN_HOUSE_HASH_X1);
                if (hh != GEN_HOUSE_HASH || hx != GEN_HOUSE_HASH_X1) printf("  HOUSE 0421: %u, at x1.00 %u\n", hh, hx);
            }
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
        {   /* (3b final review I2; 0.9 pre-flight 17) the seed draws' blends are named from the shown names only
             * ("RETROBOC+AMBIENT 0421", the cards' short names where the status line has no room), never JIANT's hidden
             * ones; the name whole on the status line (the GEN panel's line: Task 5's) */
            uint32_t sd, nm = 0, badn = 0, wide = 0, j;
            for (sd = 0; sd < 3000u; sd++) {
                char a[28], *pl;
                gen_with(0, sd, 50, 0);
                if (gp.id.part == GID_SOLO) continue;
                nm++;
                str_cpy(a, gp.last, sizeof a);
                pl = strrchr(a, ' ');                  /* the name before the seed (the full names have spaces: the last) */
                if (pl) *pl = 0;
                pl = strchr(a, '+');
                if (!pl) { badn++; continue; }
                *pl++ = 0;
                {
                    int ka = 0, kb = 0;
                    for (j = 0; j < GEN_NSHOWN; j++) {
                        ka |= !strcmp(a, GEN_SHOWN_NAME[j]) || !strcmp(a, GEN_SHOWN_CARD[j]);
                        kb |= !strcmp(pl, GEN_SHOWN_NAME[j]) || !strcmp(pl, GEN_SHOWN_CARD[j]);
                    }
                    if (!ka || !kb) { if (badn < 3u) printf("    blend name \"%s\"\n", gp.last); badn++; }
                }
                wide += text_w(&UF_HDR, ui.msg) > 232 || str_len(gp.last) + 1u >= sizeof gp.last ||
                        strncmp(ui.msg, "GEN: ", 5) || strcmp(ui.msg + 5, gp.last);
            }
            bad += gcheck("  (0.9) the seed draws' blends over 3,000 seeds: named from the shown names only", nm > 400u && badn == 0u);
            bad += gcheck("  (0.9) ... the name and seed whole, fitting the status line", nm > 400u && wide == 0u);
            gp.genre = (uint8_t)GEN_EXPLORE; gp.hot = 0xFF; ui.msg_t = 0;
        }
        frames(800);
        press(B_PLAY); frames(4);
        bad += gcheck("  (0.6.97) PLAY on the GEN screen: the transport runs, the screen stays", (song.playing || transport_req == 1) && ui.confirm == CF_DICE_ALL);
        press(B_PLAY); frames(4);
        bad += gcheck("  PLAY again: stopped, the screen stays", (!song.playing || transport_req == 2) && ui.confirm == CF_DICE_ALL);
        frames(8);
        press(B_OCTDN);                                 /* (OCT+ and OCT- on the page: the block before) */
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
         * the CPU TEST song than the model says (gen.c GC_WILD_X100). Now: the budget 65 % of the device's half, 588 at 74 %
         * (542 unset, as 0.5.1's 543; 0.9 recalibration: EXPLORE x1.55, 569 and 525); a quarter-rate delay only on a piece under it with it, and not on a busy one
         * (5 parts or more) with the CPU TEST unset; delays at the full or half rate never shortened (a slow genre's 1/4
         * echo stays); EXPLORE fitted under the budget. Then ~1000 pieces (EXPLORE, RANDOM, every genre; the CPU TEST
         * unset and 74 %). (0.9 spec 3) every piece fitted with the full fitter: 0.8's "RANDOM and the genres keep their
         * parts" withdrawn */
        static const uint8_t CPT[2] = {0, 74};
        uint32_t c, g, sd, k, n = 0, over_q = 0, busy_q = 0, cut_bad = 0, cut = 0, kept_q = 0, st3423[2], ov[2][2], bud[2];
        uint32_t xr_n = 0, xr_over = 0, slow_kept = 0, x_over = 0, r_over = 0;
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
        bad += gcheck("(0.5.2) the budget, two cores, EXPLORE: 569 at CPU TEST 74 % (65 % of the FM-1's half), 525 unset (0.9: WILD x1.55)",
                      bud[1] == 569u && bud[0] == 525u);
        bad += gcheck("  EXPLORE 3423 (CPU TEST unset, 74 %): its delay at the full rate", !st3423[0] && !st3423[1]);
        bad += gcheck("  EXPLORE 3423 and 9641 (CPU TEST unset, 74 %): under the budget", !ov[0][0] && !ov[0][1] && !ov[1][0] && !ov[1][1]);
        for (g = 0; g <= GEN_EXPLORE; g++)
            for (sd = 0; sd < (g == GEN_EXPLORE ? 400u : g == 0u ? 200u : 10u); sd++, n++) {
                uint32_t st, over, dt, busy, st0;
                gt_cpt = CPT[sd & 1u]; gen_reset();
                gen_with(g, g == GEN_EXPLORE ? sd * 25u + 5u : sd * 89u + g * 7u + 5u, 30u + (sd * 17u) % 60u, sd % 3u);   /* (0.9:
                                                         * EXPLORE's 400 seeds evenly over the identity's 0..9999; review
                                                         * M2: RANDOM's, sd * 89 + 5 up to 17,716, fold to seed % 10000
                                                         * past 9999, other draws than 0.8's; the genres' stay in range) */
                st = gt_dly_stage(); over = gc_cost(1) > gc_budget(); busy = gen_busy();
                over_q += over && st == 2u;
                busy_q += busy && !gt_cpt && st == 2u;
                kept_q += st == 2u;
                if (g == 0u || g == GEN_EXPLORE) { xr_n++; xr_over += over; x_over += over && g; r_over += over && !g;
                    if (over && xr_over < 8u) printf("  %s (%s) seed %u: %u over the budget %u, CPU TEST %u\n", g ? "EXPLORE" : "RANDOM", gs.g->name, gp.id.seed, gc_cost(1), gc_budget(), gt_cpt); }   /* (the seed built) */
                dt = (uint32_t)song.g[G_DTIME];
                song.g[G_DTIME] = gs.g->dtime;
                st0 = gt_dly_stage();
                slow_kept += st0 == 1u && dt == gs.g->dtime;
                if (dt != gs.g->dtime) {                /* shortened: a quarter-rate delay, over the budget with it or busy
                                                         * (0.9: AUTO's AGE already given way: the piece's AGE as it is) */
                    uint32_t why = st0 == 2u && (gc_cost(1) > gc_budget() || (busy && !gt_cpt)), stg;
                    {   int16_t gp0 = song.g[G_DPIT];   /* (its line with the grain on; review M5: the grain kept) */
                        song.g[G_DPIT] = 1; stg = gt_dly_stage(); song.g[G_DPIT] = gp0; }
                    /* (0.9 fix round 1, controller ruling B) or to keep the full rate when the fitter turned the grain
                     * off: PING-PONG then doubles the line it had (full with the grain) to the half rate, the fitter's
                     * side effect: straight back to the full rate */
                    why |= gs.grain_off && st0 == 1u && !stg && !st;
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
        bad += gcheck("  .. a delay shortened only from the quarter rate, over the budget or busy, or to keep the full rate when the "
                      "fitter turns the grain off; half / full rate ones kept",
                      !cut_bad && slow_kept >= 10u);
        /* (0.9 ruling) 0.8's own EXPLORE measured 11..21 of 400 over the budget on valid seeds (0..9999); the old 4 %
         * passed only because most of its seeds were out of range (up to 35,824). The threshold is 0.8's measured rate,
         * 6 % (24 of 400). Re-examined in Task 3 with every piece fitted (TAPE counted, the recalibration): EXPLORE
         * 18 of 400 (4.5 %), still past 4 %: the 6 % stays */
        bad += gcheck("  .. EXPLORE under the budget (6 % over at most: at the floor, the fit's steps run out)", x_over <= 24u);
        mc_host_v12 = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* (0.9 spec 3) TAPE on AUTO: the style's AGE; a blend the main's + half the partner's, 70 at most; WILD 0..50 from
         * its seed. On a budget every piece fits: two cores, CPU TEST 63 % (a style's piece x1.18, the budget 872; WILD
         * x1.55, 664). (0.9: the plan's "x1.00 at 63 %, two cores" was 0.8's EXPLORE factor read wrong: x1.50 then) */
        static const struct { const char *m, *p; uint8_t age; } AA[] = {
            {"RETROBOC", 0, 60}, {"AMBIENT", 0, 40}, {"AMB TECHNO", 0, 25}, {"IDM", 0, 20}, {"MICRO", 0, 15},
            {"ACID", 0, 0}, {"ELECTRO", 0, 0}, {"BREAKS", 0, 0}, {"GLITCH HOP", 0, 0}, {"DRILL'N'BASS", 0, 0},
            {"RETROBOC", "AMBIENT", 70}, {"IDM", "RETROBOC", 50}, {"AMBIENT", "IDM", 50}, {"ACID", "AMB TECHNO", 12},
            {"MICRO", "IDM", 25}, {"BREAKS", "ACID", 0}};
        uint8_t seen[GEN_AGE_WILD + 1u] = {0};
        uint32_t n, ok = 1, cut = 0, sd, a, lo = 100, hi = 0, nseen = 0, w1;
        genid_t id;
        int16_t d[5];
        mc_host_v12 = 1; gt_cpt = 63; gen_reset();
        for (n = 0; n < NELEM(AA); n++) {
            gid_set(&id, gt_style(AA[n].m), AA[n].p ? gt_style(AA[n].p) : GID_SOLO, 300u + n);
            gen_build(&id);
            tape_age_map(AA[n].age, d);
            cut += gs.age_cut;
            if (!gs.age_cut && (song.g[G_TAGE] != AA[n].age || memcmp(&song.g[G_TWOW], d, sizeof d))) {
                ok = 0; printf("  %s: AGE %d, want %u\n", gp.last, song.g[G_TAGE], AA[n].age);
            }
        }
        bad += gcheck("(0.9) AUTO AGE: RETROBOC 60, AMBIENT 40, AMB TECHNO 25, IDM 20, MICRO 15, the rest 0; blends main + half, <= 70", ok && !cut);
        cpt_res = GT_X1;                                /* (WILD at x1.00, the budget 1030: its pieces fit with TAPE; at
                                                         * 63 % WILD's x1.55, AUTO gives AGE way on many: fix round 1) */
        for (sd = 0; sd < 200u; sd++) {
            gid_set(&id, GID_WILD, GID_SOLO, sd * 37u + 1u); gen_build(&id);
            a = (uint32_t)song.g[G_TAGE];
            if (gs.age_cut) continue;
            lo = a < lo ? a : lo; hi = a > hi ? a : hi;
            if (a <= GEN_AGE_WILD && !seen[a]) { seen[a] = 1; nseen++; }
        }
        gid_set(&id, GID_WILD, GID_SOLO, 41); gen_build(&id); w1 = (uint32_t)song.g[G_TAGE];
        gid_set(&id, GID_WILD, GID_SOLO, 42); gen_build(&id);
        gid_set(&id, GID_WILD, GID_SOLO, 41); gen_build(&id);
        bad += gcheck("  WILD: AGE 0..50 from its seed (20 values at least over 200 seeds), the same seed the same AGE",
                      hi <= GEN_AGE_WILD && nseen >= 20u && (uint32_t)song.g[G_TAGE] == w1);
        {   /* gc_cost counts TAPE while the piece's AGE is above 0 (two cores: GC_TAPE2; one: GC_TAPE1). Two cores, CPU TEST
             * 63 % (the budget 872): the piece fits with TAPE, the same parts. One core: no style's piece fits with TAPE (the
             * budget 594 at most, x1.00; the lightest of 1200 style pieces 525 + 76), so the cost is compared on the same
             * piece, AGE 50 against 0, and the knob's AGE kept over the budget */
            uint32_t e0, e1, p0, p1, a;
            mc_host_v12 = 1; mc_off = 0; gt_cpt = 63; gen_reset();
            gp.tape = GT_OFF; gid_set(&id, gt_style("ACID"), GID_SOLO, 1234); gen_build(&id); e0 = gc_cost(1); p0 = parts_sig();
            id.tape = 6u; gen_build(&id); e1 = gc_cost(1); p1 = parts_sig();   /* (50) */
            bad += gcheck("  TAPE counted, two cores: AGE 50 costs GC_TAPE2 more, the same parts (under the budget)",
                          p0 == p1 && e1 == e0 + GC_TAPE2 && e1 <= gc_budget() && song.g[G_TAGE] == 50);
            mc_host_v12 = 0; mc_off = 1; gen_reset();
            gid_set(&id, gt_style("ACID"), GID_SOLO, 1234); id.tape = 6u; gen_build(&id); e1 = gc_cost(1);
            a = gs.age; gs.age = 0; e0 = gc_cost(1); gs.age = (uint8_t)a;
            bad += gcheck("  TAPE counted, one core: AGE 50 costs GC_TAPE1 more (the same piece); the knob's AGE kept, over the budget",
                          e1 == e0 + GC_TAPE1 && e1 > gc_budget() && song.g[G_TAGE] == 50 && !gs.age_cut);
            gp.tape = GT_AUTO;
        }
        mc_host_v12 = 0; mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* the TAPE knob: OFF -> AGE 0 on every piece whatever was set (UNDO brings it back); an amount -> that AGE and its
         * five, never lowered, even over the budget (one core, CPU TEST 74 %: the tightest) */
        uint32_t sd, okoff = 1, okset = 1, okundo = 1, over = 0;
        genid_t id;
        int16_t d[5];
        mc_off = 1; gt_cpt = 74; gen_reset();
        tape_age_map(70, d);
        for (sd = 0; sd < 60u; sd++) {
            tape_age_apply(50);
            gp.tape = GT_OFF; gid_set(&id, GEN_SHOWN[sd % GEN_NSHOWN], sd & 1u ? GEN_SHOWN[GEN_PARTNER[sd % GEN_NSHOWN][0]] : GID_SOLO, sd * 53u + 7u);
            gen_build(&id);
            okoff &= !song.g[G_TAGE] && !tape_set() && !gs.age_cut;
            if (sd == 0u) okundo = !undo_do() && song.g[G_TAGE] == 50 && tape_set();
            id.tape = 8u; gen_build(&id);                 /* (70) */
            okset &= song.g[G_TAGE] == 70 && !memcmp(&song.g[G_TWOW], d, sizeof d) && !gs.age_cut;
            over += gc_cost(1) > gc_budget();
        }
        gp.tape = GT_AUTO;
        printf("gen: (0.9) the TAPE knob at 70 on 60 pieces, one core, CPU TEST 74 %%: %u over the budget (the knob's AGE kept)\n", over);
        bad += gcheck("(0.9) TAPE OFF: AGE 0 and its five 0 on every piece (whatever was set); UNDO: TAPE back", okoff && okundo);
        bad += gcheck("  TAPE 70: AGE 70 and its five on every piece, never lowered", okset);
        mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* every style, blends and WILD, CPU TEST unset and 74 %, one core and two, TAPE on AUTO: the estimate with TAPE
         * within the budget (4 % over at most: the fitter's floor, AGE already 0), never over with AGE on (but the tape
         * styles', RETROBOC and AMBIENT: their AGE kept as a knob's, user decision). (0.9 spec 3,
         * "AUTO gives way before the music") AUTO took AGE to 0 before the music was thinned: over with TAPE after the
         * first rung (the chord steps' ECHO / ARP NOTES), AGE and its five 0, the parts those of the same identity with
         * TAPE OFF (that first rung aside: it counted TAPE); with the knob at 100 the AGE kept.
         * (0.9 spec 3's recalibration) The 4 % bound on two cores only: the model is calibrated there (15 pieces on an
         * FM-1, two cores, CPU TEST 74 %). One core is fitted the same way but carries no bound, its figure printed: its
         * budget (gen.c gc_budget, 714 / dev - 120) has no device measurement behind it and no template piece reaches it */
        static const uint8_t CPT[2] = {0, 74};
        uint32_t c, n, pieces = 0, over = 0, over_on = 0, cuts = 0, cut_bad = 0, p2 = 0, over2 = 0, cuts2 = 0, over_ts = 0, over_ts2 = 0;
        for (c = 0; c < 4u; c++)
            for (n = 0; n < 150u; n++) {
                genid_t id;
                uint32_t ps, s = n * 2654435761u + c + 1u;
                mc_host_v12 = (uint8_t)(c & 1u); mc_off = (uint8_t)!(c & 1u); gt_cpt = CPT[c >> 1]; gen_reset();
                gid_now(&id);
                gid_pick(&id, &s, GID_SOLO);
                id.seed = (uint16_t)((n * 89u + c * 7u + 5u) % 10000u); id.energy = (uint8_t)(30u + (n * 17u) % 60u);
                id.form = (uint16_t)(n % 3u); id.tape = GT_AUTO;
                gen_build(&id);
                pieces++;
                over += gc_cost(1) > gc_budget();
                p2 += c & 1u; over2 += (c & 1u) && gc_cost(1) > gc_budget(); cuts2 += (c & 1u) && gs.age_cut;
                over_on += gc_cost(1) > gc_budget() && song.g[G_TAGE] && !gen_keeps_age();
                over_ts += gc_cost(1) > gc_budget() && song.g[G_TAGE] && gen_keeps_age();   /* (the tape styles: AGE kept) */
                over_ts2 += (c & 1u) && gc_cost(1) > gc_budget() && song.g[G_TAGE] && gen_keeps_age();
                if (gs.age_cut) {
                    cuts++;
                    cut_bad += gs.cost_thin <= gc_budget() || song.g[G_TAGE] || tape_set();
                    gen_fit_ccomp(); ps = parts_sig();  /* (the first rung as if over: the same on both) */
                    id.tape = GT_OFF; gen_build(&id); gen_fit_ccomp();
                    cut_bad += parts_sig() != ps;       /* (no part thinned for TAPE) */
                    id.tape = GT_N - 1u; gen_build(&id);   /* (the knob at 100) */
                    cut_bad += song.g[G_TAGE] != 100;
                }
            }
        printf("gen: (0.9) %u pieces (TAPE AUTO): %u over the budget, %u with AGE on; AUTO lowered AGE on %u\n", pieces, over, over_on, cuts);
        printf("gen:   two cores: %u of %u over, AGE lowered on %u; one core (no bound: uncalibrated): %u of %u over, AGE lowered on %u\n",
               over2, p2, cuts2, over - over2, pieces - p2, cuts - cuts2);
        printf("gen:   the tape styles (AGE kept, the music thinned around it): %u over the budget with AGE on, %u of them on two cores\n",
               over_ts, over_ts2);
        bad += gcheck("  (0.9 N3) the tape styles on two cores: none over the budget with their AGE (spec 3's Aim: the net's TAPE rung idle)",
                      !over_ts2);
        bad += gcheck("(0.9) every piece fitted with TAPE counted: two cores over the budget 4 % at most, none with AUTO's AGE on (the "
                      "tape styles' kept: spec 3)",
                      over2 * 25u <= p2 && !over_on);
        bad += gcheck("  AUTO took AGE to 0 before the music was thinned (over with TAPE then; the parts those of TAPE OFF)", cuts && !cut_bad);
        mc_host_v12 = 0; mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* (0.9 spec 3, "every section is fitted"; review I1) a PARTS or SONG piece's sections B..D, made after A was fitted,
         * fitted to the budget A was fitted to: none over it unless A is (the floor), and over by no more than A. Two
         * cores (the calibrated configuration), CPU TEST unset and 74 %, styles, blends and WILD, TAPE on AUTO */
        static const uint8_t CPT[2] = {0, 74};
        uint32_t c, n, pieces = 0, secs = 0, sec_over = 0, sec_bad = 0, worst = 0;
        for (c = 0; c < 2u; c++)
            for (n = 0; n < 100u; n++) {
                genid_t id;
                uint32_t s = n * 2246822519u + c * 7u + 3u, k, ca;
                mc_host_v12 = 1; mc_off = 0; gt_cpt = CPT[c]; gen_reset();
                gid_now(&id);
                gid_pick(&id, &s, GID_SOLO);
                id.seed = (uint16_t)((n * 131u + c * 17u + 9u) % 10000u); id.energy = (uint8_t)(30u + (n * 23u) % 70u);
                id.complex = (uint8_t)(20u + (n * 37u) % 80u); id.form = (uint16_t)(1u + n % 2u); id.tape = GT_AUTO;
                gen_build(&id);
                pieces++;
                ca = gc_cost(1);
                for (k = 1; k < 4u; k++) {
                    uint32_t cs;
                    if (!((dice_secs >> k) & 1u) || section_load(k)) continue;
                    secs++;
                    gs.in_sec = 1; cs = gc_cost(1); gs.in_sec = 0;   /* (as gen_fit_section costs it: its mutes free) */
                    sec_over += cs > gc_budget();
                    if (cs > gc_budget() && cs > ca) {
                        sec_bad++;
                        if (cs - (ca > gc_budget() ? ca : gc_budget()) > worst) worst = cs - (ca > gc_budget() ? ca : gc_budget());
                        if (sec_bad <= 4u) printf("  %s, CPU TEST %u: section %c %u over the budget %u (A %u)\n", gp.last, CPT[c], 'A' + (int)k, cs, gc_budget(), ca);
                    }
                }
                section_load(0);
            }
        printf("gen: (0.9) %u PARTS / SONG pieces, two cores: %u later sections, %u over the budget, %u over it and A (worst +%u)\n",
               pieces, secs, sec_over, sec_bad, worst);
        bad += gcheck("(0.9) every section fitted: no later section over the budget the piece was fitted to (A at the floor aside)",
                      secs > 200u && !sec_bad);
        mc_host_v12 = 0; mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
    }
    {   /* (0.9 spec 3, "except the tape styles": user decision) RETROBOC and AMBIENT, and blends led by them: TAPE is part of
         * their sound, AUTO keeps their AGE and the music is thinned around it (as a knob-set AGE). Two cores, CPU TEST
         * unset and 74 %, 40 pieces a style (solo and blends with each partner) */
        static const uint8_t CPT[2] = {0, 74};
        static const char *const TS[2] = {"RETROBOC", "AMBIENT"};
        uint32_t c, t, n, kept[2][2] = {{0}}, tot[2][2] = {{0}}, ok = 1;
        for (c = 0; c < 2u; c++)
            for (t = 0; t < 2u; t++)
                for (n = 0; n < 40u; n++) {
                    genid_t id;
                    uint32_t m = gt_style(TS[t]), i = gen_shown_index(m + 1u), want;
                    mc_host_v12 = 1; mc_off = 0; gt_cpt = CPT[c]; gen_reset();
                    gid_set(&id, m, n % 3u ? GEN_SHOWN[GEN_PARTNER[i][n % GEN_NPARTNER[i]]] : GID_SOLO, n * 211u + t * 5u + 17u);
                    id.energy = (uint8_t)(30u + (n * 29u) % 60u); id.form = (uint16_t)(n % 3u); id.tape = GT_AUTO;
                    gen_build(&id);
                    want = gen_age();
                    tot[c][t]++;
                    kept[c][t] += song.g[G_TAGE] == (int16_t)want && want;
                    ok &= !gs.age_cut && song.g[G_TAGE] == (int16_t)want && want > 0u;
                }
        printf("gen: (0.9) the tape styles' AUTO AGE kept, two cores: RETROBOC %u / %u unset, %u / %u at 74 %%; AMBIENT %u / %u, %u / %u\n",
               kept[0][0], tot[0][0], kept[1][0], tot[1][0], kept[0][1], tot[0][1], kept[1][1], tot[1][1]);
        bad += gcheck("(0.9) RETROBOC and AMBIENT (and blends they lead): AUTO's AGE kept on every piece, the music thinned around it", ok);
        mc_host_v12 = 0; mc_off = 0; gt_cpt = 0; cpt_res = 0; cpt_cores = 0;
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
    {   /* (0.9 spec 6, pre-flight 1) GEN's page and its recent pieces are the session's: a project and the settings are
         * byte for byte the same whatever they hold (nothing to ignore on load: nothing of GEN was ever stored) */
        static project_t pa, pb;
        static project_store_t sa, sb;
        static persist_t ea, eb;
        genid_t x;
        gen_reset();
        gen_with(1u + GEN_SHOWN[2], 55, 50, 0);
        memset(&sa, 0, sizeof sa); memset(&sb, 0, sizeof sb); memset(&ea, 0, sizeof ea); memset(&eb, 0, sizeof eb);
        project_capture(&pa); proj_pack(&sa, &pa); settings_export(&ea);
        gp.energy = 85; gp.form = 2; gp.complex = 10; gp.tape = 7; gp.at = 0;
        gid_now(&x); x.seed = 999; gen_recent_push(&x);
        project_capture(&pb); proj_pack(&sb, &pb); settings_export(&eb);
        bad += gcheck("(0.9) GEN's page and recent pieces: a project and the settings byte for byte the same",
                      !memcmp(&sa, &sb, sizeof sa) && !memcmp(&ea, &eb, sizeof ea));
    }
    printf(bad ? "gen: %d FAILED\n" : "gen: all passed\n", bad);
    return bad ? 1 : 0;
}
