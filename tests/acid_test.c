/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker */
/* (Braindancer 0.8) the 303 acid response on ANALOG in MONO / LEGATO (src/voice.c an_acid / acc_step, the per-voice
 * cutoff, trk_note_on's slide; src/seq.c's acc_step call and the stop's reset), through the real sources (ui_test.c's).
 *   build/host/acid_test
 * 1. climb: T1 ANALOG ACID MONO, 16 steps of C2, steps 0-3 and 8 accented, 120 BPM: the sweep at each step start 6144,
 *    12288, 16384, 16384; after step 4 (no accent) under 8192 within 60 ms and under 256 within 400 ms; step 8 climbs
 *    6144 from where it was; the sound brighter step by step over steps 0, 1, 2 (first-difference RMS / RMS).
 * 2. slide: MONO and LEGATO, step 0 C2 SLIDE, step 1 C3: no new attack, the envelope kept, C3 reached 55-62 ms after
 *    step 1 starts (glide 2: 80 blocks) with GLIDE 0 and GLIDE 100 alike.
 * 3. unchanged: POLY / UNISON ANALOG with accents: no sweep, the render bit for bit the one without the acid response
 *    (ACID_OFF); FM6 / PHASE MONO / LEGATO with accents and slides: their renders' hashes as before the change.
 * 4. the stop zeroes the sweep. Listen-check WAVs of a four-track song (song/4track_mix's parts and patterns) without
 *    and with the response: build/acid_demo/. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
static int acid_off;                                    /* (1: an_acid says no: the sound before the change) */
#define ACID_OFF() acid_off
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static void ck(const char *what, int ok)
{
    printf("acid: %-110s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}

#define NB_MAX 12000u                                   /* blocks at most (~8.7 s) */
static int32_t rbuf[NB_MAX * CTL];                      /* the track's render */
static int32_t sw_at[NB_MAX];                           /* acc_sw after each block */
static uint32_t st_blk[64];                             /* the block each step started in */
static int32_t st_sw[64];                               /* acc_sw at each step's start (after its seq_tick) */
static uint32_t nst;

/* track 1 alone: engine eng, preset pi, voice mode, 16 steps all REST, 120 BPM, no swing, no HUMAN */
static track_t *acid_trk(uint32_t eng, uint32_t pi, uint32_t mode)
{
    track_t *t = &trk[0];
    uint32_t i;
    ui_power_on();
    trk_live = NTRK;
    song.g[G_BPM] = 120;
    song.g[G_SWING] = 0;
    set_engine_of(t, eng);
    t->engine = t->eng_req;
    apply_preset_to(t, pi);
    t->p[P_VOICE] = (int16_t)mode;
    t->p[P_AMODE] = 0;
    t->p[P_MUTE] = 0;
    t->p[P_SLEN] = 16;
    t->p[P_SSWING] = 0;
    t->p[P_HVEL] = 127;
    for (i = 0; i < NSTEP; i++)
        t->step[i] = (step_t){{0}, 0, ST_REST};
    return t;
}
static void put(track_t *t, uint32_t i, uint32_t note, uint32_t flags)
{
    t->step[i] = (step_t){{(uint8_t)note}, 1, ST_NOTE, (uint8_t)flags, 100};
}
/* the transport on, nb blocks of seq_tick + track_render on track 1: rbuf, sw_at, the steps' starts; the hash */
static uint32_t render(track_t *t, uint32_t nb)
{
    uint32_t b, i, h = 2166136261u;
    uint8_t si;
    seq_start();
    si = t->step_inst;
    nst = 0;
    for (b = 0; b < nb && b < NB_MAX; b++) {
        int32_t *o = &rbuf[b * CTL];
        seq_tick(t, CTL);
        if (t->step_inst != si) {
            si = t->step_inst;
            if (nst < 64u) {
                st_blk[nst] = b;
                st_sw[nst++] = t->acc_sw;
            }
        }
        track_render(t, o, CTL);
        sw_at[b] = t->acc_sw;
        for (i = 0; i < CTL; i++)
            h = (h ^ (uint32_t)o[i]) * 16777619u;
    }
    return h;
}
#define MS_BLK(ms) ((uint32_t)((ms) * (double)FS / 1000.0 / CTL))
static double bright(uint32_t b0)                       /* first-difference RMS / RMS over 40 ms from block b0 */
{
    uint32_t i, s0 = b0 * CTL, n = MS_BLK(40) * CTL;
    double e = 0, d = 0;
    for (i = 1; i < n; i++) {
        double x = rbuf[s0 + i], y = x - rbuf[s0 + i - 1u];
        e += x * x;
        d += y * y;
    }
    return e > 0 ? sqrt(d / e) : 0;
}

/* ---- 1. climb ---- */
static void test_climb(void)
{
    track_t *t = acid_trk(ENGI_ANALOG, 4, V_MONO);
    uint32_t i, b4;
    double br[3], bo[3];
    char what[160];
    for (i = 0; i < 16u; i++)
        put(t, i, 36, i < 4u || i == 8u ? SF_ACCENT : 0u);
    render(t, MS_BLK(2000));
    ck("climb: 16 steps started", nst >= 16u);
    if (nst < 16u) return;
    snprintf(what, sizeof what, "climb: the sweep at steps 0-3 6144 12288 16384 16384 (%d %d %d %d)", st_sw[0], st_sw[1],
             st_sw[2], st_sw[3]);
    ck(what, st_sw[0] == 6144 && st_sw[1] == 12288 && st_sw[2] == 16384 && st_sw[3] == 16384);
    b4 = st_blk[4];
    snprintf(what, sizeof what, "climb: after step 4 (no accent) under 8192 within 60 ms (%d), under 256 within 400 ms (%d)",
             sw_at[b4 + MS_BLK(60)], sw_at[b4 + MS_BLK(400)]);
    ck(what, sw_at[b4 + MS_BLK(60)] < 8192 && sw_at[b4 + MS_BLK(400)] < 256);
    snprintf(what, sizeof what, "climb: step 8 climbs 6144 from just before (%d -> %d)", sw_at[st_blk[8] - 1u], st_sw[8]);
    ck(what, st_sw[8] - sw_at[st_blk[8] - 1u] == 6144);
    for (i = 0; i < 3u; i++)
        br[i] = bright(st_blk[i]);
    snprintf(what, sizeof what, "climb: brighter step by step over steps 0, 1, 2 (%.4f %.4f %.4f)", br[0], br[1], br[2]);
    ck(what, br[0] < br[1] && br[1] < br[2]);
    seq_stop();
    ck("the stop: the sweep 0, not relaxing", t->acc_sw == 0 && t->acc_relax == 0);
    t = acid_trk(ENGI_ANALOG, 4, V_MONO);              /* the same without the response */
    for (i = 0; i < 16u; i++)
        put(t, i, 36, i < 4u || i == 8u ? SF_ACCENT : 0u);
    acid_off = 1;
    render(t, MS_BLK(2000));
    acid_off = 0;
    seq_stop();
    if (nst < 3u) return;
    for (i = 0; i < 3u; i++)
        bo[i] = bright(st_blk[i]);
    snprintf(what, sizeof what, "climb: steps 0, 1, 2 brighter than without the response (x %.3f %.3f %.3f)",
             br[0] / bo[0], br[1] / bo[1], br[2] / bo[2]);
    ck(what, bo[0] < br[0] && bo[1] < br[1] && bo[2] < br[2]);
}

/* ---- 2. slide ---- */
static void test_slide(void)
{
    static const uint8_t MODES[2] = {V_MONO, V_LEGATO};
    static const int16_t GL[2] = {0, 100};
    uint32_t m, g;
    for (m = 0; m < 2u; m++)
        for (g = 0; g < 2u; g++) {
            track_t *t = acid_trk(ENGI_ANALOG, 4, MODES[m]);
            voice_t *v = &t->v[0];
            uint32_t b, nb = MS_BLK(500), b1 = 0, reach = 0, att = 0, started = 0;
            int32_t e0 = 0, e1 = 0;
            uint8_t si;
            char what[200];
            t->p[P_GLIDE] = GL[g];
            put(t, 0, 36, SF_SLIDE);
            put(t, 1, 48, 0);
            seq_start();
            si = t->step_inst;
            for (b = 0; b < nb; b++) {
                int32_t o[CTL];
                int32_t eb = v->env;
                seq_tick(t, CTL);
                if (t->step_inst != si) {
                    si = t->step_inst;
                    if (started++ == 1u) {   /* step 1 */
                        b1 = b;
                        e0 = eb;
                    }
                }
                if (started >= 2u && b >= b1 && b < b1 + MS_BLK(100) && v->stage == 1u)
                    att = 1;
                track_render(t, o, CTL);
                if (started >= 2u && b == b1)
                    e1 = v->env;
                if (started >= 2u && !reach && v->pitch_cur == v->pitch16 && v->note == 48u)
                    reach = b - b1 + 1u;
            }
            {
                double ms = reach * (double)CTL * 1000.0 / FS;
                snprintf(what, sizeof what, "slide %s GLIDE %d: no new attack (%u), env kept (%d -> %d), C3 reached in %.1f ms "
                         "(55-62)", m ? "LEGATO" : "MONO", GL[g], att, e0, e1, ms);
                ck(what, started >= 2u && !att && (int64_t)e1 * 10 >= (int64_t)e0 * 9 && ms >= 55.0 && ms <= 62.0);
            }
        }
}

/* ---- 3. unchanged ---- */
/* the hashes of FM6 / PHASE MONO / LEGATO with accents and slides, from the code before the change (8356b61) */
static const uint32_t OLD_H[4] = {0x0808a22au, 0x792c6a80u, 0xad9d3c1du, 0x35df2a6du};
static void pattern_mixed(track_t *t)
{
    static const uint8_t N[16] = {36, 36, 48, 36, 0, 39, 36, 46, 36, 0, 48, 43, 36, 39, 0, 41};
    static const uint8_t F[16] = {1, 0, 2, 1, 0, 0, 3, 2, 1, 0, 1, 0, 0, 2, 0, 1};
    uint32_t i;
    for (i = 0; i < 16u; i++)
        if (N[i]) put(t, i, N[i], F[i]);
}
/* a scene in a child of this process (each from the same state: UNISON's random phases, the counters): its hash, and
 * whether the sweep stayed 0. arg: engine | mode << 8 | ACID_OFF << 16 | KEYS (the pattern's notes on the keys, the
 * transport stopped, accents at velocity 127) */
#define KEYS (1u << 24)
static uint32_t sc_mixed(uint32_t arg, int *z)
{
    track_t *t = acid_trk(arg & 255u, (arg & 255u) == ENGI_ANALOG ? 4u : 0u, (arg >> 8) & 255u);
    uint32_t b, h = 2166136261u, i;
    pattern_mixed(t);
    acid_off = (int)((arg >> 16) & 1u);
    if (arg & KEYS) {                                   /* a step's notes on the keys, a step 125 ms, held 60 ms */
        uint32_t len = MS_BLK(125), on = 0;
        for (b = 0; b < MS_BLK(3000); b++) {
            int32_t *o = &rbuf[b * CTL];
            const step_t *s = &t->step[(b / len) % 16u];
            if (b % len == 0u && s->time == ST_NOTE) {
                trk_note_on(t, s->note[0], s->flags & SF_ACCENT ? 127u : 100u);
                on = s->note[0];
            }
            if (b % len == MS_BLK(60) && on) {
                trk_note_off(t, on);
                on = 0;
            }
            track_render(t, o, CTL);
            sw_at[b] = t->acc_sw;
            for (i = 0; i < CTL; i++)
                h = (h ^ (uint32_t)o[i]) * 16777619u;
        }
    } else
        h = render(t, MS_BLK(3000));
    for (*z = 1, b = 0; b < MS_BLK(3000); b++)
        *z &= sw_at[b] == 0;
    return h;
}
static int in_child(uint32_t arg, uint32_t *h, int *z)
{
    int fd[2], st = 0;
    uint32_t r[2] = {0, 0};
    ssize_t got;
    pid_t pid;
    if (pipe(fd)) return 0;
    fflush(stdout);
    pid = fork();
    if (pid < 0) return 0;
    if (!pid) {
        int zz = 0;
        close(fd[0]);
        r[0] = sc_mixed(arg, &zz);
        r[1] = (uint32_t)zz;
        if (write(fd[1], r, sizeof r) != (ssize_t)sizeof r) _exit(1);
        _exit(0);
    }
    close(fd[1]);
    got = read(fd[0], r, sizeof r);
    close(fd[0]);
    waitpid(pid, &st, 0);
    *h = r[0];
    *z = (int)r[1];
    return got == (ssize_t)sizeof r && WIFEXITED(st) && !WEXITSTATUS(st);
}
static void test_unchanged(void)
{
    static const uint8_t EN[2] = {ENGI_FM6, ENGI_PHASE};
    static const uint8_t MODES[2] = {V_MONO, V_LEGATO};
    uint32_t k, h0, h1;
    int z0, z1, ok;
    char what[160];
    for (k = 0; k < 2u; k++) {                          /* ANALOG POLY / UNISON */
        uint32_t mode = k ? V_UNISON : V_POLY;
        ok = in_child(ENGI_ANALOG | mode << 8, &h0, &z0);
        ok &= in_child(ENGI_ANALOG | mode << 8 | 1u << 16, &h1, &z1);
        snprintf(what, sizeof what, "ANALOG %s with accents and slides: no sweep, the render as without the response "
                 "(%08x %08x)", k ? "UNISON" : "POLY", h0, h1);
        ck(what, ok && z0 && h0 == h1);
    }
    for (k = 0; k < 2u; k++) {                          /* ANALOG MONO / LEGATO on the keys: the accent as before */
        uint32_t mode = k ? V_LEGATO : V_MONO;
        ok = in_child(ENGI_ANALOG | mode << 8 | KEYS, &h0, &z0);
        ok &= in_child(ENGI_ANALOG | mode << 8 | 1u << 16 | KEYS, &h1, &z1);
        snprintf(what, sizeof what, "ANALOG %s on the keys (velocity 127, stopped): no sweep, the render as before "
                 "(%08x %08x)", k ? "LEGATO" : "MONO", h0, h1);
        ck(what, ok && z0 && h0 == h1);
    }
    for (k = 0; k < 4u; k++) {                          /* FM6 / PHASE MONO / LEGATO */
        ok = in_child(EN[k / 2u] | (uint32_t)MODES[k % 2u] << 8, &h0, &z0);
        snprintf(what, sizeof what, "%s %s with accents and slides: no sweep, the render as before (%08x, was %08x)",
                 k / 2u ? "PHASE" : "FM6", k % 2u ? "LEGATO" : "MONO", h0, OLD_H[k]);
        ck(what, ok && z0 && h0 == OLD_H[k]);
    }
}

/* ---- 4. listen-check: song/4track_mix's parts and patterns (tests/regress.c job_song) without / with ---- */
static int32_t wl[NB_MAX * CTL], wr[NB_MAX * CTL];
static void wav(const char *path, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    uint32_t i, v;
    uint16_t h[2];
    if (!f) return;
    fwrite("RIFF", 1, 4, f); v = 36 + n * 4; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f);
    h[0] = 1; h[1] = 2; fwrite(h, 2, 2, f);
    v = FS; fwrite(&v, 4, 1, f); v = FS * 4; fwrite(&v, 4, 1, f);
    h[0] = 4; h[1] = 16; fwrite(h, 2, 2, f);
    fwrite("data", 1, 4, f); v = n * 4; fwrite(&v, 4, 1, f);
    for (i = 0; i < n; i++) {
        int32_t a = wl[i], b = wr[i];                   /* (mix_block: 16-bit scale) */
        int16_t s[2] = {(int16_t)(a > 32767 ? 32767 : a < -32768 ? -32768 : a),
                        (int16_t)(b > 32767 ? 32767 : b < -32768 ? -32768 : b)};
        fwrite(s, 2, 2, f);
    }
    fclose(f);
}
static uint32_t song_render(int off, const char *path)
{
    static const uint8_t ACID[16] = {45, 45, 57, 45, 0, 48, 45, 55, 45, 0, 57, 52, 45, 48, 0, 50};
    static const uint8_t ACIDF[16] = {1, 0, 2, 0, 0, 0, 1, 2, 0, 0, 1, 0, 0, 2, 0, 1};
    static const uint8_t AM[4] = {57, 60, 64, 67}, FMI[4] = {53, 57, 60, 64};
    static const uint8_t LEAD[12] = {76, 0, 0, 79, 0, 0, 81, 0, 79, 0, 76, 0};
    static int32_t o[2u * CTL];
    track_t *t1 = &trk[0], *t2 = &trk[1], *t3 = &trk[2], *td = &trk[3];
    uint32_t i, k, b, nb = MS_BLK(8000), h = 2166136261u;
    acid_off = off;
    ui_power_on();
    song.master_q12 = 4096;
    trk_live = NTRK;
    song.g[G_BPM] = 120;
    song.g[G_SWING] = 0;
    set_engine_of(t3, 3);
    t3->engine = t3->eng_req;
    apply_preset_to(t3, 0);
    for (k = 0; k < 4u; k++) {
        trk[k].p[P_MUTE] = 0;
        trk[k].p[P_SLEN] = 16;
        for (i = 0; i < NSTEP; i++)
            trk[k].step[i] = (step_t){{0}, 0, ST_REST};
    }
    for (i = 0; i < 16u; i++)
        if (ACID[i]) put(t1, i, ACID[i], ACIDF[i]);
    t2->p[P_SLEN] = 32;
    t2->p[P_SGATE] = 120;
    for (i = 0; i < 32u; i++) {
        step_t *s = &t2->step[i];
        const uint8_t *c = i < 16u ? AM : FMI;
        if (i % 16u == 0u) *s = (step_t){{c[0], c[1], c[2], c[3]}, 4, ST_NOTE, 0, 100};
        else if (i % 16u < 14u) *s = (step_t){{0}, 0, ST_TIE};
    }
    t3->p[P_SLEN] = 12;
    for (i = 0; i < 12u; i++)
        if (LEAD[i]) put(t3, i, LEAD[i], i == 0u ? SF_ACCENT : 0u);
    for (i = 0; i < 16u; i++) {
        step_t *s = &td->step[i];
        uint32_t n = 0;
        *s = (step_t){{0}, 0, ST_REST};
        if (i % 4u == 0u) s->note[n++] = 36;
        if (i == 4u || i == 12u) s->note[n++] = 38;
        if (i % 2u == 0u) s->note[n++] = i == 14u ? 46 : 42;
        if (n) { s->n = (uint8_t)n; s->time = ST_NOTE; s->flags = i % 4u == 0u ? SF_ACCENT : 0u; s->vel = 100; }
    }
    transport_req = 1;
    ui_input();
    for (b = 0; b < nb && b < NB_MAX; b++) {
        memset(o, 0, sizeof o);
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++) {
            wl[b * CTL + i] = o[2u * i];
            wr[b * CTL + i] = o[2u * i + 1u];
            h = ((h ^ (uint32_t)o[2u * i]) * 16777619u ^ (uint32_t)o[2u * i + 1u]) * 16777619u;
        }
    }
    transport_req = 2;
    ui_input();
    acid_off = 0;
    wav(path, b * CTL);
    return h;
}
static void test_demo(void)
{
    uint32_t a, b;
    char what[160];
    mkdir("build", 0755);
    mkdir("build/acid_demo", 0755);
    a = song_render(1, "build/acid_demo/4track_mix_before.wav");
    b = song_render(0, "build/acid_demo/4track_mix_after.wav");
    snprintf(what, sizeof what, "the song's acid line (LEGATO ACID, accents, slides) sounds different (%08x -> %08x)", a, b);
    ck(what, a != b);
}

int main(void)
{
    test_unchanged();                                   /* (first: its scenes from the process's first state) */
    test_climb();
    test_slide();
    test_demo();
    if (bad) {
        printf("acid: %d FAILED\n", bad);
        return 1;
    }
    printf("acid: all ok\n");
    return 0;
}
