/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker */
/* (Braindancer 0.8) TAPE, the master effect (src/fx.c tape_begin / tape_block, globals G_TAGE .. G_TTONE) through the
 * real sources (ui_test.c's).
 *   build/host/tape_test
 * 1. off: every TAPE global 0: the sequenced four-part mix of tests/djf_test.c bit for bit 0.7's (DJF_CENTRE_090), and
 *    tape_block never runs (out of the chain).
 * 2. a tone through it (tape_begin + tape_block alone): WOW's drift (cents, its rate), FLUTTER's (its rate, left and
 *    right apart), SAT's level kept and its peaks held, the line's delay exact, TONE's roll-off, HISS under the music
 *    and none over silence.
 * 3. in and out of the chain without a click; out again within its fade, and not run after.
 * 4. at rest after a note: tape_idle, its states exactly 0, the silence path taken; turned off there, out of the chain
 *    at once: the first hit after PLAY bit for bit as never on.
 * 5. stored: no block when all 0 (the bytes as before), 'T' 'P' the last block, the round trip, old projects, 255
 *    clamped, NEW PROJECT, UNDO.
 * 6. AGE's automation (MOT_AGE, as the DJ FILTER's): recorded, played back (the five with it), the six back at a stop,
 *    no lock; its records in 'T' 'P' alone (every byte before as without them), a crafted / oversize block refused;
 *    NEW PROJECT while AGE and DJ FILTER records play: the power-on values, not the old knobs. */
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
static unsigned tape_runs;
static int32_t tape_in_any;                             /* (what tape_block saw: its input not all 0) */
#define TAPE_COUNT()                                                                                       \
    do {                                                                                                   \
        uint32_t z_;                                                                                       \
        tape_runs++;                                                                                       \
        for (z_ = 0; z_ < n; z_++) tape_in_any |= mix_l[z_] | mix_r[z_] | wet[z_];                         \
    } while (0)
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bad;
static void ck(const char *what, int ok)
{
    printf("tape: %-110s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}
#define D0 ((int32_t)(TAPE_D0 >> 8))                    /* the line's centre read, samples */

/* ---- 1. off ---- */
#define DJF_CENTRE_090 0x8b898fe6u
#define SC_NB 12000u                                    /* (blocks at most: the off-again scene's ~8.7 s) */
static uint32_t sc_nb = 2400u;                          /* (the 0.7 hash: 2400) */
static uint32_t sc_mix[SC_NB], sc_out[SC_NB];           /* each block's hash: the mix TAPE gives the master, the output */
static uint32_t sc_on = SC_NB, sc_off = SC_NB;          /* TAPE (WOW 40, TONE 50, HISS 30) in at block sc_on, out at sc_off */
static uint32_t centre_render(void)                     /* (tests/djf_test.c's scene) */
{
    static int32_t o[2u * CTL];
    uint32_t b, i, h = 2166136261u, k;
    ui_power_on();
    song.master_q12 = 4096;
    trk_live = NTRK;
    for (k = 0; k < 4u; k++) {
        track_t *t = &trk[k];
        set_engine_of(t, k == 3u ? ENGI_DRUM : k == 1u ? ENGI_FM6 : ENGI_ANALOG);
        t->engine = t->eng_req;
        apply_preset_to(t, k);
        t->p[P_MUTE] = 0; t->p[P_SLEN] = 16; t->p[P_DLY] = 50; t->p[P_REV] = 60; t->p[P_CHOR] = 30;
        for (i = 0; i < 16u; i++) {
            step_t *s = &t->step[i];
            memset(s, 0, sizeof *s);
            s->time = (i + k) % 3u ? ST_NOTE : ST_REST;
            s->vel = 100;
            if (k == 3u) s->hit = (uint8_t)(1u << (i % 4u));
            else { s->n = 2; s->note[0] = (uint8_t)(40 + 12 * k + i % 7u); s->note[1] = (uint8_t)(s->note[0] + 7u); }
        }
    }
    song.g[G_CLIP] = 30;
    transport_req = 1; ui_input();
    for (b = 0; b < sc_nb; b++) {
        uint32_t hm = 2166136261u, ho = 2166136261u;
        if (b == sc_on) { song.g[G_TWOW] = 40; song.g[G_TTONE] = 50; song.g[G_THISS] = 30; }
        if (b == sc_off) { song.g[G_TWOW] = 0; song.g[G_TTONE] = 0; song.g[G_THISS] = 0; }
        memset(o, 0, sizeof o);
        mix_block(o, CTL);
        for (i = 0; i < 2u * CTL; i++) {
            h = (h ^ (uint32_t)o[i]) * 16777619u;
            ho = (ho ^ (uint32_t)o[i]) * 16777619u;
        }
        for (i = 0; i < CTL; i++)                       /* (mix_l / mix_r: TAPE's output, what the leveler reads) */
            hm = (((hm ^ (uint32_t)mix_l[i]) * 16777619u) ^ (uint32_t)mix_r[i]) * 16777619u;
        sc_mix[b] = hm;
        sc_out[b] = ho;
    }
    transport_req = 2; ui_input();
    return h;
}
static void test_off(void)
{
    uint32_t h, k, z = 1;
    char what[140];
    tape_runs = 0;
    h = centre_render();
    for (k = G_TAGE; k <= G_TTONE; k++)
        z &= song.g[k] == 0;
    snprintf(what, sizeof what, "off (every TAPE global 0): the sequenced four-part mix bit for bit 0.7's (%08x)", h);
    ck(what, z && h == DJF_CENTRE_090);
    ck("off: tape_block never ran, TAPE out of the chain", tape_runs == 0u && !tape.on);
}

static void tape_reset(void);
/* the scene from this process's state as it is (a child of it: both variants start from the same state), TAPE in at
 * block on, out at off: the blocks' hashes into mix / out, tape_block's runs into *runs */
static int scene_fork(uint32_t on, uint32_t off, uint32_t *mix, uint32_t *out, uint32_t *runs)
{
    int fd[2], st = 0;
    pid_t pid;
    size_t got = 0, want = 2u * SC_NB * sizeof *mix + sizeof *runs;
    static uint8_t buf[2u * SC_NB * 4u + 4u];
    if (pipe(fd)) return 0;
    fflush(stdout);
    pid = fork();
    if (pid < 0) return 0;
    if (!pid) {
        close(fd[0]);
        sc_on = on; sc_off = off; sc_nb = SC_NB;
        tape_runs = 0;
        (void)centre_render();
        memcpy(buf, sc_mix, sizeof sc_mix);
        memcpy(buf + sizeof sc_mix, sc_out, sizeof sc_out);
        memcpy(buf + 2u * sizeof sc_mix, &tape_runs, 4);
        if (write(fd[1], buf, want) != (ssize_t)want) _exit(1);
        _exit(tape.on ? 2 : 0);
    }
    close(fd[1]);
    while (got < want) {
        ssize_t r = read(fd[0], buf + got, want - got);
        if (r <= 0) break;
        got += (size_t)r;
    }
    close(fd[0]);
    waitpid(pid, &st, 0);
    memcpy(mix, buf, SC_NB * 4u);
    memcpy(out, buf + SC_NB * 4u, SC_NB * 4u);
    memcpy(runs, buf + 2u * SC_NB * 4u, 4);
    return got == want && WIFEXITED(st) && !WEXITSTATUS(st);
}
/* on and back off: once out of the chain, the mix TAPE hands on is bit for bit what it would be had it never been on */
static void test_off_again(void)
{
    static uint32_t ref_mix[SC_NB], ref_out[SC_NB];
    uint32_t b, from = SC_NB, ofrom = SC_NB, diff = 0, runs = 0, r0 = 1;
    int ok;
    char what[220];
    ok = scene_fork(SC_NB, SC_NB, ref_mix, ref_out, &r0) && !r0;
    ok &= scene_fork(400u, 900u, sc_mix, sc_out, &runs);
    for (b = 400u; b < 900u; b++) diff += sc_mix[b] != ref_mix[b];
    for (b = SC_NB; b-- > 900u;) {
        if (sc_mix[b] != ref_mix[b]) break;
        from = b;
    }
    for (b = SC_NB; b-- > 900u;) {
        if (sc_out[b] != ref_out[b]) break;
        ofrom = b;
    }
    snprintf(what, sizeof what, "on at block 400, off at 900: the mix TAPE gives bit for bit the same again from block %u (<= %u; %u "
             "blocks differed while on); the output (after the leveler's slow states) from %u (<= %u)", from, 900u + TAPE_FADE / CTL + 1u, diff,
             ofrom, 900u + 6u * FS / CTL);
    ck(what, ok && runs > 400u && diff > 400u && from <= 900u + TAPE_FADE / CTL + 1u && ofrom <= 900u + 6u * FS / CTL);
    tape_reset();
}

/* ---- 2. a tone through it ---- */
#define NMAX (44100u * 20u + 64u)
static int32_t in_l[NMAX], out_l[NMAX], out_r[NMAX];
static void tape_reset(void)
{
    uint32_t k;
    for (k = G_TAGE; k <= G_TTONE; k++)
        song.g[k] = 0;
    memset(&tape, 0, sizeof tape);
    memset(tape_buf, 0, sizeof tape_buf);
    fx_safe = 0;
}
static void tape_g(int32_t wow, int32_t flut, int32_t sat, int32_t hiss, int32_t tone)
{
    song.g[G_TWOW] = (int16_t)wow; song.g[G_TFLUT] = (int16_t)flut; song.g[G_TSAT] = (int16_t)sat;
    song.g[G_THISS] = (int16_t)hiss; song.g[G_TTONE] = (int16_t)tone;
}
/* a sine of hz, amplitude amp (the mix's scale: 32768 the output's full scale), for nblk blocks through TAPE as
 * mix_block runs it (tape_begin, tape_block while in the chain), from a fresh state with the globals set; the input in
 * in_l, the output in out_l / out_r; silent_from: the input 0 from that block on */
static void tape_tone_s(double hz, double amp, uint32_t nblk, uint32_t silent_from)
{
    uint32_t b, i;
    double ph = 0;
    memset(&tape, 0, sizeof tape);
    for (b = 0; b < nblk; b++) {
        for (i = 0; i < CTL; i++) {
            int32_t x = b < silent_from ? (int32_t)lrint(amp * sin(ph)) : 0;
            ph += 2.0 * M_PI * hz / FS;
            mix_l[i] = mix_r[i] = in_l[b * CTL + i] = x;
            wet[i] = 0;
        }
        tape_begin();
        if (tape.on)
            tape_block(CTL);
        for (i = 0; i < CTL; i++) {
            out_l[b * CTL + i] = mix_l[i] + wet[i];
            out_r[b * CTL + i] = mix_r[i] + wet[i];
        }
    }
}
static void tape_tone(double hz, double amp, uint32_t nblk) { tape_tone_s(hz, amp, nblk, nblk); }

/* the pitch in cents against hz0, window by window (w samples) from sample s0: the rising zero crossings, interpolated */
static uint32_t cents_of(const int32_t *y, uint32_t n, uint32_t s0, uint32_t w, double hz0, double *c)
{
    uint32_t k = 0, s;
    for (s = s0; s + w <= n; s += w) {
        double t0 = -1, t1 = 0;
        uint32_t i, m = 0;
        for (i = s + 1u; i < s + w; i++)
            if (y[i - 1u] < 0 && y[i] >= 0) {
                double t = (double)(i - 1u) + (double)-y[i - 1u] / (double)(y[i] - y[i - 1u]);
                if (t0 < 0) t0 = t;
                t1 = t;
                m++;
            }
        c[k++] = m > 1u ? 1200.0 * log2((double)(m - 1u) * FS / (t1 - t0) / hz0) : 0;
    }
    return k;
}
static double peak_abs(const double *c, uint32_t n)
{
    double m = 0;
    uint32_t i;
    for (i = 0; i < n; i++)
        if (fabs(c[i]) > m) m = fabs(c[i]);
    return m;
}
/* the deviation's swings: the times between its sign changes (hysteresis h), as rates (1 / the time); slowest, fastest */
static uint32_t swings(const double *c, uint32_t n, double dt, double h, double *slow, double *fast)
{
    int sg = 0;
    uint32_t i, last = 0, k = 0;
    *slow = 1e9; *fast = 0;
    for (i = 0; i < n; i++) {
        int s = c[i] > h ? 1 : c[i] < -h ? -1 : 0;
        if (!s || s == sg) continue;
        if (sg) {
            if (k) {
                double r = 1.0 / ((double)(i - last) * dt);
                if (r < *slow) *slow = r;
                if (r > *fast) *fast = r;
            }
            k++;
            last = i;
        }
        sg = s;
    }
    return k;
}
static double dominant_hz(const double *c, uint32_t n, double dt)   /* the strongest rate 1..40 Hz */
{
    double best = 0, bh = 0, hz, mean = 0;
    uint32_t i;
    for (i = 0; i < n; i++) mean += c[i] / n;
    for (hz = 1.0; hz <= 40.0; hz += 0.05) {
        double re = 0, im = 0;
        for (i = 0; i < n; i++) {
            re += (c[i] - mean) * cos(2.0 * M_PI * hz * i * dt);
            im += (c[i] - mean) * sin(2.0 * M_PI * hz * i * dt);
        }
        if (re * re + im * im > best) { best = re * re + im * im; bh = hz; }
    }
    return bh;
}
static double corr(const double *a, const double *b, uint32_t n)
{
    double ma = 0, mb = 0, sab = 0, saa = 0, sbb = 0;
    uint32_t i;
    for (i = 0; i < n; i++) { ma += a[i] / n; mb += b[i] / n; }
    for (i = 0; i < n; i++) {
        sab += (a[i] - ma) * (b[i] - mb);
        saa += (a[i] - ma) * (a[i] - ma);
        sbb += (b[i] - mb) * (b[i] - mb);
    }
    return sab / sqrt(saa * sbb + 1e-30);
}
static double rms_db(const int32_t *y, uint32_t s0, uint32_t n)
{
    double s = 0;
    uint32_t i;
    for (i = s0; i < n; i++) s += (double)y[i] * y[i];
    return 10.0 * log10(s / (n - s0) + 1e-30);
}
static int32_t peak_of(const int32_t *y, uint32_t s0, uint32_t n)
{
    int32_t m = 0;
    uint32_t i;
    for (i = s0; i < n; i++)
        if (abs(y[i]) > m) m = abs(y[i]);
    return m;
}
static double gain_db(int32_t tone, double hz)          /* TONE's: a sine's level out against in, settled */
{
    uint32_t nb = 1500u, n = nb * CTL, s0 = 300u * CTL;
    tape_reset();
    tape_g(0, 0, 0, 0, tone);
    tape_tone(hz, 8000, nb);
    return rms_db(out_l, s0, n) - rms_db(in_l, s0 - D0, n - D0);
}
#define NBW (44100u * 8u / CTL)                  /* (8 s; WOW's swings: 20 s) */
#define NBW20 (44100u * 20u / CTL)
static double cw[NMAX / 220u], cw2[NMAX / 220u], cr[NMAX / 220u];
static void test_tone_through(void)
{
    char what[180];
    uint32_t nb = NBW, n = nb * CTL, s0 = 11025u, nw, k, i, n20 = NBW20 * CTL;
    double p100, p30, slow, fast, dom, dom1, cc;
    /* WOW */
    tape_reset();
    tape_g(100, 0, 0, 0, 0);
    tape_tone(1000, 8000, NBW20);
    nw = cents_of(out_l, n20, s0, 882u, 1000, cw);
    p100 = peak_abs(cw, nw);
    k = swings(cw, nw, 0.02, 0.3, &slow, &fast);
    tape_reset();
    tape_g(30, 0, 0, 0, 0);
    tape_tone(1000, 8000, NBW20);
    p30 = peak_abs(cw2, cents_of(out_l, n20, s0, 882u, 1000, cw2));
    snprintf(what, sizeof what, "WOW 100 (20 s): peak %.1f cents (3..45), %u swings at %.2f .. %.2f Hz (0.25..2.2); WOW 30: %.1f cents",
             p100, k, slow, fast, p30);
    ck(what, p100 >= 3.0 && p100 <= 45.0 && k >= 8u && slow >= 0.25 && fast <= 2.2 && p30 < p100);
    /* FLUTTER */
    tape_reset();
    tape_g(0, 100, 0, 0, 0);
    tape_tone(1000, 8000, nb);
    nw = cents_of(out_l, n, s0, 221u, 1000, cw);
    (void)cents_of(out_r, n, s0, 221u, 1000, cr);
    dom = dominant_hz(cw, nw, 221.0 / FS);
    p100 = peak_abs(cw, nw);
    cc = corr(cw, cr, nw);
    tape_reset();
    tape_g(0, 1, 0, 0, 0);
    tape_tone(1000, 8000, 50);
    dom1 = (double)tape.flut_inc[0] * FS / 4294967296.0;
    snprintf(what, sizeof what, "FLUTTER 100: at %.2f Hz (13..16), peak %.1f cents (<= 15), left / right correlation %.2f (< 0.95); "
             "FLUTTER 1: %.2f Hz", dom, p100, cc, dom1);
    ck(what, dom >= 13.0 && dom <= 16.0 && p100 > 1.0 && p100 <= 15.0 && cc < 0.95 && dom1 > 5.9 && dom1 < 6.3);
    /* SAT */
    {
        double q = 131072.0 * pow(10.0, -15.0 / 20.0), dl;
        int32_t pd, po;
        tape_reset();
        tape_g(0, 0, 100, 0, 0);
        tape_tone(220, q, 600);
        dl = rms_db(out_l, 200u * CTL, 600u * CTL) - rms_db(in_l, 200u * CTL - D0, 600u * CTL - D0);
        tape_reset();
        tape_g(0, 0, 100, 0, 0);
        tape_tone(220, 131071, 600);
        pd = peak_of(in_l, 200u * CTL, 600u * CTL);
        po = peak_of(out_l, 200u * CTL, 600u * CTL);
        snprintf(what, sizeof what, "SAT 100: a -15 dB sine's level %+.2f dB (within 1.5), a 0 dB sine's peak %d under the dry's %d", dl,
                 po, pd);
        ck(what, fabs(dl) <= 1.5 && po < pd);
    }
    /* the line's delay, exact */
    {
        int32_t m = 0, m1 = 0;
        tape_reset();
        tape_g(0, 0, 0, 1, 0);
        tape_tone(440, 8000, 200);
        for (i = 100u * CTL; i < 200u * CTL; i++) {
            if (abs(out_l[i] - in_l[i - D0]) > m) m = abs(out_l[i] - in_l[i - D0]);
            if (abs(out_l[i] - in_l[i - D0 - 1]) > m1) m1 = abs(out_l[i] - in_l[i - D0 - 1]);
        }
        snprintf(what, sizeof what, "HISS 1 alone: the dry delayed exactly %d samples (%.2f ms; residual %d, a sample off %d)", D0,
                 D0 * 1000.0 / FS, m, m1);
        ck(what, m <= 16 && m1 > 100);
    }
    /* a hot mix (3 x past the old int16 line's +-131068): through the line unclipped, sample for sample */
    {
        int32_t m = 0, pi, po;
        double g1, g0;
        tape_reset();
        tape_g(0, 0, 0, 1, 0);
        tape_tone(440, 400000, 200);
        for (i = 100u * CTL; i < 200u * CTL; i++)
            if (abs(out_l[i] - in_l[i - D0]) > m) m = abs(out_l[i] - in_l[i - D0]);
        pi = peak_of(in_l, 100u * CTL, 200u * CTL);
        po = peak_of(out_l, 100u * CTL, 200u * CTL);
        g0 = gain_db(50, 3000);
        tape_reset();
        tape_g(0, 0, 0, 0, 50);
        tape_tone(3000, 400000, 1500);
        g1 = rms_db(out_l, 300u * CTL, 1500u * CTL) - rms_db(in_l, 300u * CTL - D0, 1500u * CTL - D0);
        snprintf(what, sizeof what, "a hot mix (a 400000 sine, HISS 1): peak %d out of %d in, %d at most from the dry delayed; TONE 50 on it "
                 "%+.2f dB (%+.2f at -12 dB)", po, pi, m, g1, g0);
        ck(what, abs(po - pi) <= 48 && m <= 48 && fabs(g1 - g0) < 0.05);
    }
    /* TONE */
    {
        double g300 = gain_db(100, 300), g10k = gain_db(100, 10000), h50 = gain_db(50, 10000), h0 = gain_db(0, 10000);
        snprintf(what, sizeof what, "TONE 100: 300 Hz %+.2f dB (0.5), 10 kHz %+.1f dB (<= -8); TONE 50 10 kHz %+.1f between TONE 0's %+.1f "
                 "and 100's", g300, g10k, h50, h0);
        ck(what, fabs(g300) <= 0.5 && g10k <= -8.0 && h50 < h0 - 0.5 && h50 > g10k + 0.5);
    }
    /* HISS */
    {
        double r12, r18, t12;
        uint32_t nbh = 600u, a = 200u * CTL, zero = 0, last = 0;
        static int32_t res[600u * CTL];
        tape_reset();
        tape_g(0, 0, 0, 100, 0);
        tape_tone(1000, 32768.0 * pow(10.0, -12.0 / 20.0), nbh);
        for (i = a; i < nbh * CTL; i++) res[i] = out_l[i] - in_l[i - D0];
        r12 = rms_db(res, a, nbh * CTL);
        t12 = rms_db(in_l, a, nbh * CTL);
        tape_reset();
        tape_g(0, 0, 0, 100, 0);
        tape_tone(1000, 32768.0 * pow(10.0, -18.0 / 20.0), nbh);
        for (i = a; i < nbh * CTL; i++) res[i] = out_l[i] - in_l[i - D0];
        r18 = rms_db(res, a, nbh * CTL);
        tape_reset();
        tape_g(0, 0, 0, 100, 0);
        tape_tone_s(1000, 32768.0 * pow(10.0, -12.0 / 20.0), nbh, 300u);   /* (the tone, then silence) */
        for (i = 300u * CTL; i < nbh * CTL; i++)
            if (out_l[i] || out_r[i]) last = i;
        tape_reset();                                   /* (silence only, from the start) */
        tape_g(0, 0, 0, 100, 0);
        tape_tone(1000, 0, 100);
        for (k = 0; k < 100u * CTL; k++) zero |= (uint32_t)(out_l[k] | out_r[k]);
        snprintf(what, sizeof what, "HISS 100: %.1f dB under a -12 dB tone (24..42), %.1f dB lower under a -18 dB one (6 +-1.5); none over "
                 "silence; 0 %u blocks after a tone", t12 - r12, r12 - r18, last / CTL + 1u - 300u);
        ck(what, t12 - r12 >= 24.0 && t12 - r12 <= 42.0 && fabs(r12 - r18 - 6.0) <= 1.5 && !zero &&
           last / CTL + 1u - 300u <= (uint32_t)(D0 + 2300) / CTL + 2u);
    }
    tape_reset();
}

/* ---- 3. in and out ---- */
/* a 220 Hz tone at -12 dB, TAPE in (WOW 40) at block 100, out at 400: the output's largest second difference (*ms: in the
 * chain, away from the edges); *off: the blocks from 400 until out of the chain; *runs: tape_block's count then. hard: in
 * at once, no fade (a click, to see the measure catch it) */
static double in_out_d2(int hard, double *ms, uint32_t *off, uint32_t *runs)
{
    uint32_t b, i;
    double amp = 32768.0 * pow(10.0, -12.0 / 20.0), w = 2.0 * M_PI * 220.0 / FS, m = 0, ph = 0;
    int32_t p1 = 0, p2 = 0;
    tape_reset();
    *ms = 0; *off = 0; *runs = 0;
    for (b = 0; b < 600u; b++) {
        if (b == 100u) tape_g(40, 0, 0, 0, 0);
        if (b == 400u) tape_g(0, 0, 0, 0, 0);
        for (i = 0; i < CTL; i++) {
            mix_l[i] = mix_r[i] = (int32_t)lrint(amp * sin(ph));
            ph += w;
            wet[i] = 0;
        }
        tape_begin();
        if (hard && tape.on && b < 400u)
            tape.g = TAPE_FULL;                         /* (no fade, no hold) */
        if (tape.on)
            tape_block(CTL);
        for (i = 0; i < CTL; i++) {
            int32_t y = mix_l[i] + wet[i], d2 = y - 2 * p1 + p2;
            if (b >= 4u && fabs((double)d2) > m) m = fabs((double)d2);
            if (b >= 200u && b < 380u && fabs((double)d2) > *ms) *ms = fabs((double)d2);
            p2 = p1;
            p1 = y;
        }
        if (b >= 400u && !tape.on && !*off) { *off = b + 1u - 400u; *runs = tape_runs; }
    }
    tape_reset();
    return m;
}
static void test_in_out(void)
{
    char what[200];
    uint32_t off, runs, o2, r2;
    double w = 2.0 * M_PI * 220.0 / FS, tone = 32768.0 * pow(10.0, -12.0 / 20.0) * w * w, ms, ms2, m, h;
    m = in_out_d2(0, &ms, &off, &runs);
    snprintf(what, sizeof what, "in at block 100, out at 400: second difference %.0f (in, between: %.0f; the tone's %.0f, at most x4); "
             "out %u blocks after (<= %u), not run since", m, ms, tone, off, TAPE_FADE / CTL + 1u);
    ck(what, m < 4.0 * tone && off && off <= TAPE_FADE / CTL + 1u && tape_runs == runs);
    h = in_out_d2(1, &ms2, &o2, &r2);
    snprintf(what, sizeof what, "(the measure: TAPE switched in at once is a click: %.0f, past x4)", h);
    ck(what, h > 4.0 * tone);
}

/* (Braindancer 0.8) TAPE the safety net's first rung (audio.c fx_safety_tick: fx_safe SAFE_TAPE): a -12 dBFS 220 Hz tone
 * through TAPE as mix_block runs it (tape_begin, tape_block while in the chain), AGE 50 with HISS 0 (its noise would be
 * the measure's floor); the rung on at block 200, off at
 * 600: TAPE fades out and back in on its own fade, no click at either edge (the second difference around each against
 * the tone's own), AGE's six untouched */
static double rung_d2(int hard, double *e, double *ms, uint32_t *outb, uint32_t *inb, int *kept)
{
    uint32_t b, i;
    double amp = 32768.0 * pow(10.0, -12.0 / 20.0), w = 2.0 * M_PI * 220.0 / FS, ph = 0;
    int32_t p1 = 0, p2 = 0;
    int16_t g6[6];
    tape_reset();
    tape_age_apply(50);
    song.g[G_THISS] = 0;                                /* (HISS off: the tone's own second difference the measure) */
    memcpy(g6, &song.g[G_TAGE], sizeof g6);
    e[0] = e[1] = *ms = 0; *outb = *inb = 0;
    for (b = 0; b < 1000u; b++) {
        if (b == 200u) fx_safe |= SAFE_TAPE;
        if (b == 600u) fx_safe &= (uint8_t)~SAFE_TAPE;
        if (hard && b == 200u) tape.on = 0;             /* (out at once: no fade) */
        for (i = 0; i < CTL; i++) {
            mix_l[i] = mix_r[i] = (int32_t)lrint(amp * sin(ph));
            ph += w;
            wet[i] = 0;
        }
        if (tape.on || tape_set())                      /* (mix_block's order) */
            tape_begin();
        if (tape.on)
            tape_block(CTL);
        for (i = 0; i < CTL; i++) {
            int32_t y = mix_l[i] + wet[i], d2 = y - 2 * p1 + p2;
            if (b >= 196u && b < 220u && fabs((double)d2) > e[0]) e[0] = fabs((double)d2);
            if (b >= 596u && b < 630u && fabs((double)d2) > e[1]) e[1] = fabs((double)d2);
            if (b >= 100u && b < 196u && fabs((double)d2) > *ms) *ms = fabs((double)d2);
            p2 = p1;
            p1 = y;
        }
        if (b >= 200u && b < 600u && !tape.on && !*outb) *outb = b + 1u - 200u;
        if (b >= 600u && tape.on && tape.g == TAPE_FULL && !*inb) *inb = b + 1u - 600u;
    }
    *kept = !memcmp(g6, &song.g[G_TAGE], sizeof g6);  /* (the rung leaves the user's settings) */
    tape_reset();
    return amp * w * w;
}
static void test_rung_click(void)
{
    char what[260];
    uint32_t outb, inb, o2, i2;
    double e[2], h[2], ms, ms2, tone;
    int kept, k2;
    tone = rung_d2(0, e, &ms, &outb, &inb, &kept);
    snprintf(what, sizeof what, "the safety net's rung (AGE 50, HISS 0): out at block 200, in at 600: second difference %.0f / %.0f "
             "(in the chain %.0f; the tone's %.0f, at most x4); out in %u blocks, whole again in %u; the six kept",
             e[0], e[1], ms, tone, outb, inb);
    ck(what, e[0] <= 4.0 * tone && e[1] <= 4.0 * tone && outb && outb <= TAPE_FADE / CTL + 1u && inb &&
       inb <= ((TAPE_D0 >> 8) + 8u + TAPE_FADE) / CTL + 2u && kept);
    (void)rung_d2(1, h, &ms2, &o2, &i2, &k2);
    snprintf(what, sizeof what, "(the measure: TAPE taken out at once on the rung is a click: %.0f, past it)", h[0]);
    ck(what, h[0] > 4.0 * tone);
}
/* a mix far past anything the parts make (a 0.5 G sine, SAT 100): no product wraps (SAT's input clamped) */
static void test_huge(void)
{
    char what[200];
    uint32_t i, bad_n = 0;
    int32_t pk = 0;
    tape_reset();
    tape_g(0, 0, 100, 0, 0);
    tape_tone(440, 5.0e8, 200);
    for (i = 100u * CTL; i < 200u * CTL; i++) {
        int32_t x = in_l[i - D0], y = out_l[i];
        if (abs(y) > pk) pk = abs(y);
        bad_n += x != 0 && (x > 0) != (y > 0);           /* (a wrapped product: the sign flips) */
    }
    snprintf(what, sizeof what, "SAT 100 on a huge mix (a 5e8 sine): its sign kept everywhere (%u flips), the peak %d (SAT's ceiling)",
             bad_n, pk);
    ck(what, !bad_n && pk > 30000 && pk <= 4 * 65536);
    tape_reset();
}

/* ---- 4. at rest ---- */
static void test_rest(void)
{
    static int32_t o[2u * CTL];
    char what[180];
    uint32_t b, i, tail = 0, idle = 0, rest0;
    ui_power_on();
    load_depth++; fm6_init(); power_on_parts(); load_depth--;
    song.master_q12 = 4096;
    mc_host = 1;
    tape_reset();
    tape_g(40, 0, 0, 50, 50);
    for (i = 0; i < 3u; i++) trk_note_on(&trk[4], 48u + 4u * i, 100);
    for (b = 0; b < 600u; b++) mix_block(o, CTL);
    for (i = 0; i < 3u; i++) trk_note_off(&trk[4], 48u + 4u * i);
    rest0 = prf_cur.fxrest;
    for (b = 0; b < 40000u; b++) {
        if (b % 32u == 0u) { ui_input(); song_poll(); host_ticks += 20000u; fm1_ms += 20u; }
        tape_in_any = 0;
        mix_block(o, CTL);
        if (tape_in_any) { tail = b; idle = 0; }
        else if (!idle && tape.on && tape_idle()) idle = b;
        if (idle && prf_cur.fxrest > rest0 + 10u) break;
    }
    snprintf(what, sizeof what, "a note, then silence: TAPE at rest %u blocks after the tail's end (<= %u), its states 0, the silence "
             "path taken (%u blocks)", idle - tail, TAPE_QUIET + 2u, prf_cur.fxrest - rest0);
    ck(what, tape.on && idle && idle - tail <= TAPE_QUIET + 2u && !tape.env && !tape.tlp[0] && !tape.tlp[1] &&
       prf_cur.fxrest > rest0 + 10u);
    tape_reset();
}

/* TAPE turned off while the mix is at rest (the silence path: tape_block not run), then PLAY: the first hit as if TAPE
 * had never been on. A child of this process per variant (both from the same state): the mix TAPE gives the master,
 * a hash a block, for RP_HIT blocks after PLAY, through the pipe; the child's exit: TAPE out of the chain before PLAY
 * and tape_block not run after it */
#define RP_HIT 64u
static uint32_t rp_hit(int was_on, uint32_t *hm, uint32_t *rest)
{
    static int32_t o[2u * CTL];
    uint32_t b, i, r0, before = 0;
    track_t *t = &trk[3];
    ui_power_on();
    load_depth++; fm6_init(); power_on_parts(); load_depth--;
    song.master_q12 = 4096;
    mc_host = 1;
    trk_live = NTRK;
    tape_reset();
    set_engine_of(t, ENGI_DRUM);                        /* T4 DRUM: a kick on step 0 alone */
    t->engine = t->eng_req;
    t->p[P_MUTE] = 0; t->p[P_SLEN] = 16;
    for (i = 0; i < 16u; i++) {
        memset(&t->step[i], 0, sizeof t->step[i]);
        t->step[i].time = i ? ST_REST : ST_NOTE;
        t->step[i].vel = 100;
        t->step[i].hit = i ? 0u : 1u;
    }
    if (was_on) tape_g(40, 0, 0, 50, 50);
    transport_req = 1; ui_input();
    for (b = 0; b < 600u; b++) mix_block(o, CTL);       /* (the kick through TAPE, when on) */
    transport_req = 2; ui_input();
    r0 = prf_cur.fxrest;
    for (b = 0; b < 40000u; b++) {                      /* stopped: to rest (the silence path) */
        if (b % 32u == 0u) { ui_input(); song_poll(); host_ticks += 20000u; fm1_ms += 20u; }
        mix_block(o, CTL);
    }
    *rest = prf_cur.fxrest - r0;
    tape_age_apply(0);                                  /* at rest: AGE 0 (every detail 0) */
    for (b = 0; b < 200u; b++) {
        if (b % 32u == 0u) { ui_input(); song_poll(); host_ticks += 20000u; fm1_ms += 20u; }
        mix_block(o, CTL);
    }
    before = tape.on;
    tape_runs = 0;
    transport_req = 1; ui_input();                      /* PLAY: the first hit */
    for (b = 0; b < RP_HIT; b++) {
        uint32_t h = 2166136261u;
        mix_block(o, CTL);
        for (i = 0; i < CTL; i++)
            h = (((h ^ (uint32_t)mix_l[i]) * 16777619u) ^ (uint32_t)mix_r[i]) * 16777619u;
        hm[b] = h;
    }
    transport_req = 2; ui_input();
    return before || tape_runs;
}
static int rp_fork(int was_on, uint32_t *hm, uint32_t *rest)
{
    int fd[2], st = 0;
    pid_t pid;
    uint32_t buf[RP_HIT + 1u];
    size_t got = 0;
    if (pipe(fd)) return -1;
    fflush(stdout);
    pid = fork();
    if (pid < 0) return -1;
    if (!pid) {
        uint32_t r;
        close(fd[0]);
        r = rp_hit(was_on, buf, &buf[RP_HIT]);
        if (write(fd[1], buf, sizeof buf) != (ssize_t)sizeof buf) _exit(3);
        _exit(r ? 2 : 0);
    }
    close(fd[1]);
    while (got < sizeof buf) {
        ssize_t r = read(fd[0], (uint8_t *)buf + got, sizeof buf - got);
        if (r <= 0) break;
        got += (size_t)r;
    }
    close(fd[0]);
    waitpid(pid, &st, 0);
    memcpy(hm, buf, RP_HIT * sizeof *hm);
    *rest = buf[RP_HIT];
    return got == sizeof buf && WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}
static void test_rest_off_play(void)
{
    uint32_t ref[RP_HIT], got[RP_HIT], r_ref = 0, r_on = 0, b, diff = 0;
    int e_ref, e_on;
    char what[220];
    e_ref = rp_fork(0, ref, &r_ref);
    e_on = rp_fork(1, got, &r_on);
    for (b = 0; b < RP_HIT; b++) diff += got[b] != ref[b];
    snprintf(what, sizeof what, "TAPE on, the mix at rest (%u silent blocks), AGE 0, PLAY: TAPE out of the chain before it (%d) and the "
             "first hit bit for bit as never on (%u of %u blocks differ)", r_on, e_on, diff, RP_HIT);
    ck(what, !e_ref && !e_on && r_on > 100u && r_ref > 100u && !diff);
}

/* ---- 5. stored ---- */
static void test_store(void)
{
    static project_store_t s0, s1, s2;
    project_t a, c;
    uint32_t n0, n1, i, k;
    int ok;
    char what[180];
    ui_power_on();
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 100};
    song.g[G_PLAYS1 + 1] = 2;                           /* (a 'P' 'L' block: TAPE's after it) */
    tape_reset();
    project_capture(&a);
    n0 = proj_packed_size(&a);
    ok = proj_pack(&s0, &a) && !proj_tp_used(&a);
    for (i = n0; i < PROJC_PFX_OFF; i++)
        ok &= s0.raw[i] == 0u;
    ok &= s0.raw[n0 - 2u - PROJ_PL_N] == 'P' && s0.raw[n0 - 1u - PROJ_PL_N] == 'L';
    ck("TAPE all 0: packed as before (no 'T' 'P', 'P' 'L' the last block, the size)", ok);
    song.g[G_TWOW] = 40; song.g[G_TTONE] = 70;
    project_capture(&a);
    n1 = proj_packed_size(&a);
    ok = proj_pack(&s1, &a) && n1 == n0 + 2u + PROJ_TP_N && !memcmp(s0.raw, s1.raw, n0) && s1.raw[n0] == 'T' &&
         s1.raw[n0 + 1u] == 'P';
    ok &= s1.raw[n0 + 2u] == 0u && s1.raw[n0 + 3u] == 40u && s1.raw[n0 + 7u] == 70u && s1.raw[n0 + 8u] == 0u;
    for (i = n1; i < PROJC_PFX_OFF; i++)
        ok &= s1.raw[i] == 0u;
    tape_reset();
    ok &= proj_import(&c, &s1, sizeof s1) && !project_restore_runtime(&c);
    for (k = G_TAGE; k <= G_TTONE; k++)
        ok &= song.g[k] == (k == G_TWOW ? 40 : k == G_TTONE ? 70 : 0);
    ok &= proj_pack(&s2, &c) && !memcmp(&s1, &s2, sizeof s1);
    ck("WOW 40, TONE 70: 'T' 'P' the last block (after 'P' 'L'), six bytes and a count; the round trip byte for byte", ok);
    song.g[G_PLAYS1 + 1] = 0;
    song.g[G_MRSEL] = 1;                                /* (an 'M' 'R' block, no 'P' 'L') */
    song.g[G_TFLUT] = 9;
    project_capture(&a);
    n0 = proj_packed_size(&a);
    ok = proj_pack(&s1, &a) && s1.raw[n0 - 2u - PROJ_TP_N] == 'T' && s1.raw[n0 - 4u - PROJ_TP_N - PROJ_MR_N] == 'M';
    ck("no PLAYS: 'T' 'P' right after 'M' 'R'", ok && s1.raw[n0 - 3u - PROJ_TP_N - PROJ_MR_N] == 'R');
    song.g[G_MRSEL] = 0;
    for (k = G_TAGE; k <= G_TTONE; k++)
        song.g[k] = 33;
    ok = proj_import(&c, &s0, sizeof s0) && !project_restore_runtime(&c);
    for (k = G_TAGE; k <= G_TTONE; k++)
        ok &= c.g[k] == 0 && song.g[k] == 0;
    ck("a project without 'T' 'P' (0.7's): TAPE all 0, whatever the song held", ok);
    {
        uint32_t sum;
        song.g[G_TWOW] = 40; song.g[G_TTONE] = 70;
        project_capture(&a);
        n1 = proj_packed_size(&a);
        ok = proj_pack(&s1, &a) && s1.raw[n1 - 2u - PROJ_TP_N + 2u + 5u] == 70u;
        s1.raw[n1 - 2u] = 255u;                         /* (TONE: a value no firmware wrote) */
        sum = proj_hash(s1.raw, PROJ_STORE_SIZE - 4u);
        memcpy(s1.raw + PROJ_STORE_SIZE - 4u, &sum, 4);
        ok &= proj_import(&c, &s1, sizeof s1) && !project_restore_runtime(&c) && song.g[G_TTONE] == 100 && song.g[G_TWOW] == 40;
        snprintf(what, sizeof what, "a stored 255: clamped (TONE %d)", song.g[G_TTONE]);
        ck(what, ok);
    }
    for (k = G_TAGE; k <= G_TTONE; k++)
        song.g[k] = (int16_t)(k - G_TAGE + 10u);
    system_init();
    for (ok = 1, k = G_TAGE; k <= G_TTONE; k++)
        ok &= song.g[k] == 0;
    ck("NEW PROJECT (system_init): TAPE all 0", ok);
    song.g[G_TSAT] = 60; song.g[G_TAGE] = 50;
    undo_mark();
    song.g[G_TSAT] = 0; song.g[G_TAGE] = 0;
    ok = !undo_do() && song.g[G_TSAT] == 60 && song.g[G_TAGE] == 50;
    ck("UNDO brings TAPE back (SAT 60, AGE 50)", ok);
    ui_power_on();
    tape_reset();
}

/* ---- 6. AGE's automation (as the DJ FILTER's: tests/djf_test.c test_automation): MOT_AGE records, kept in 'T' 'P' ---- */
static void age_steps(track_t *t, uint32_t n, int half)  /* n steps on (a 1/16 at 120), each in two ticks when half */
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        uint32_t s = step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx);
        if (half) { seq_tick(t, s / 2u); seq_tick(t, s - s / 2u); }
        else seq_tick(t, s);
    }
}
static int age_six(const int16_t *g, const int16_t *w) { return !memcmp(g, w, 6u * sizeof g[0]); }
static int age_on_map(int32_t age)                       /* song.g: AGE age and its five on the map */
{
    int16_t d[5];
    tape_age_map(age, d);
    return song.g[G_TAGE] == age && !memcmp(&song.g[G_TWOW], d, sizeof d);
}
static uint32_t age_steps_of(const motion_store_t *m, uint32_t k)   /* the steps of track k's MOT_AGE records (bit) */
{
    uint32_t i, s = 0;
    for (i = 0; i < m->count; i++)
        if (MOTION_ID(&m->event[i]) == MOT_AGE && MOTION_TRK(&m->event[i]) == k) s |= 1u << MOTION_STEP(&m->event[i]);
    return s;
}
static int age_lock_refused(void)                       /* a store holding an AGE lock: invalid; the event itself: valid */
{
    static motion_store_t m;
    memset(&m, 0, sizeof m);
    m.count = 1;
    motion_at(&m.event[0], 0u << 6 | 3u, 10);
    m.event[0].param = (uint8_t)MOT_AGE;
    if (!motion_valid(&m)) return 0;
    m.event[0].param = (uint8_t)(MOT_AGE | MOTION_LOCK);
    return !motion_valid(&m);
}
static void test_age_auto(void)
{
    static project_store_t st, st2, sx;
    static project_t q, q2, r;
    track_t *t;
    int16_t b0[6];
    uint32_t i, k, n, n2, sum, steps;
    int ok, same;
    char what[200];
    ui_power_on();
    tape_reset();
    t = &trk[0];
    song.sel = 0;
    tape_age_apply(20);                                 /* the knob before, and SAT by hand */
    song.g[G_TSAT] = 7;
    memcpy(b0, &song.g[G_TAGE], sizeof b0);
    song.rec = 1;
    seq_start();
    seq_tick(t, CTL);
    motion_age_set(t, 30);                              /* step 0 */
    age_steps(t, 4u, 1);
    nav_put(SEC_FX, "MASTER");                          /* step 4: FX > MASTER KNOB 4 (AGE), recording */
    frames(200);
    turn(EN_K4, 30);
    age_steps(t, 4u, 1);
    motion_age_set(t, 90);                              /* step 8 */
    steps = age_steps_of(&motion, 0);
    ok = steps == (1u << 0 | 1u << 4 | 1u << 8) && age_on_map(90) && proj_age_events(&motion) == 3u;
    snprintf(what, sizeof what, "recording: AGE turned at steps 0, 4, 8 writes three MOT_AGE records on T1 (steps %#x)",
             (unsigned)steps);
    ck(what, ok);
    ck("AUTO LIST lists them, named AGE", ev_id_ok(t, MOT_AGE) && str_eq(track_desc(t, MOT_AGE)->label, "AGE") &&
       motion_desc(t, MOT_AGE) == &GP[G_TAGE]);
    seq_stop();
    ck("stop: the six back to their values before the first record (AGE 20, SAT 7 by hand)", age_six(&song.g[G_TAGE], b0));
    seq_start();
    seq_tick(t, CTL);
    ok = age_on_map(30);
    song.g[G_TWOW] = 3;                                 /* WOW by hand while the records play */
    age_steps(t, 4u, 0);
    ok &= age_on_map(60);
    ck("playback: step 0's record sets AGE 30 and its five; a WOW by hand meanwhile overwritten at step 4's (AGE 60)", ok);
    ck("playback: the base (a save's) is the knob's, not the records'", motion_base_value(t, MOT_AGE) == b0[0]);
    project_capture(&q);
    ok = age_six(&q.g[G_TAGE], b0);
    age_steps(t, 4u, 0);
    ok &= age_on_map(90);
    ck("playback: step 8's (AGE 90); a save keeps the six the knob set", ok);
    seq_stop();
    ck("stop again: the six back", age_six(&song.g[G_TAGE], b0));
    song.rec = 0;
    ck("no lock for it (motion_put .., MOTION_LOCK refused with 1; motion_valid refuses one)",
       motion_put(t, 3, MOT_AGE, 10, MOTION_LOCK) == 1 && motion_param(MOT_AGE) && age_steps_of(&motion, 0) == 0x111u &&
       age_lock_refused());
    (void)motion_set_event(t, 7, P_REV, 90);            /* (a track parameter's record, a DJ FILTER's) */
    (void)motion_set_event(t, 9, MOT_DJF, -10);
    project_capture(&q);
    q2 = q;                                             /* the same project without the AGE records */
    for (i = 0, n = 0; i < q.motion.count; i++)
        if (MOTION_ID(&q.motion.event[i]) != MOT_AGE) q2.motion.event[n++] = q.motion.event[i];
    memset(q2.motion.event + n, 0, (MOTION_MAX - n) * sizeof q2.motion.event[0]);
    q2.motion.count = (uint8_t)n;
    n = proj_packed_size(&q);
    n2 = proj_packed_size(&q2);
    ok = proj_pack(&st, &q) && proj_pack(&st2, &q2) && n == n2 + 3u * sizeof q.motion.event[0] && !memcmp(st.raw, st2.raw, n2 - 1u);
    ok &= st.raw[n2 - 1u] == 3u && st2.raw[n2 - 1u] == 0u && st.raw[n2 - 1u - PROJ_TP_N] == 'P' && st.raw[n2 - 2u - PROJ_TP_N] == 'T';
    for (k = 0; k < 3u; k++)
        ok &= (st.raw[n2 + 4u * k + 1u] & 0x7Fu) == MOT_AGE;
    for (k = 0; k + 3u < n2; k++)                       /* ('M' 'G': the DJ FILTER's one) */
        if (st.raw[k] == 'M' && st.raw[k + 1u] == 'G' && st.raw[k + 2u] == 1u && (st.raw[k + 4u] & 0x7Fu) == MOT_DJF) break;
    ok &= k + 3u < n2;
    ck("FUNC: every byte as the project without AGE records (the main motion, 'M' 'G' the DJ FILTER's alone) up to 'T' 'P''s "
       "count: 3, then the three MOT_AGE records", ok);
    ok = proj_import(&r, &st, sizeof st) && r.motion.count == q.motion.count && age_six(&r.g[G_TAGE], b0) &&
         proj_age_events(&r.motion) == 3u && proj_djf_events(&r.motion) == 1u;
    for (i = 0, same = 1; i < q.motion.count; i++) {
        int found = 0;
        for (k = 0; k < r.motion.count; k++)
            found |= !memcmp(&q.motion.event[i], &r.motion.event[k], sizeof q.motion.event[0]);
        same &= found;
    }
    ck("loaded back: the same records, the knob's six", ok && same);
    ok = proj_import(&r, &st2, sizeof st2) && !proj_age_events(&r.motion) && r.motion.count == q2.motion.count;
    ck("loaded back without AGE records ('T' 'P' count 0, as Task 1 wrote it): none", ok);
    sx = st;                                            /* a record in 'T' 'P' that is not AGE's: refused */
    sx.raw[n2 + 4u + 1u] = (uint8_t)((sx.raw[n2 + 4u + 1u] & 0x80u) | P_REV);
    sum = proj_hash(sx.raw, PROJ_STORE_SIZE - 4u);
    memcpy(sx.raw + PROJ_STORE_SIZE - 4u, &sum, 4);
    ok = !proj_import(&r, &sx, sizeof sx);
    sx = st;                                            /* (as is, rehashed: loads) */
    sum = proj_hash(sx.raw, PROJ_STORE_SIZE - 4u);
    memcpy(sx.raw + PROJ_STORE_SIZE - 4u, &sum, 4);
    ok &= proj_import(&r, &sx, sizeof sx) != 0;
    ck("a crafted 'T' 'P' holding a P_REV record: the load refused (proj_unpack_c 0)", ok);
    ok = 1;
    for (k = 4u; k < 256u; k += k < 8u ? 1u : 37u) {   /* a count past its records (zeros after: not AGE's), past 64 */
        sx = st;
        sx.raw[n2 - 1u] = (uint8_t)k;
        sum = proj_hash(sx.raw, PROJ_STORE_SIZE - 4u);
        memcpy(sx.raw + PROJ_STORE_SIZE - 4u, &sum, 4);
        ok &= !proj_import(&r, &sx, sizeof sx);
    }
    ck("'T' 'P' with a count past its records or past the 64: refused", ok);
    ui_power_on();
    tape_reset();
    project_capture(&q);                                /* (TAPE all 0: no 'T' 'P', as 0.7's) */
    ok = proj_pack(&st, &q) && !proj_tp_used(&q);
    seq_start();
    song.rec = 1;
    motion_age_set(&trk[0], 40);                        /* (records in the song before the load) */
    song.rec = 0;
    seq_stop();
    ok &= proj_age_events(&motion) == 1u && proj_import(&r, &st, sizeof st) && !project_restore_runtime(&r) &&
          !proj_age_events(&r.motion) && !proj_age_events(&motion) && !song.g[G_TAGE];
    ck("a 0.7 project (no 'T' 'P'): loads with no AGE records, TAPE off", ok);
    ui_power_on();
    tape_reset();
}

/* NEW PROJECT (system_init) while AGE and DJ FILTER records play: the power-on values, not the old project's knob
 * values put back by the records' end (motion_age_off / motion_djf_off after the globals' reset) */
static void test_new_project_motion(void)
{
    track_t *t;
    uint32_t k;
    int ok;
    char what[200];
    ui_power_on();
    tape_reset();
    t = &trk[0];
    song.sel = 0;
    tape_age_apply(20);                                 /* the knobs before: AGE 20, DJ FILTER 30 */
    song.g[G_DJF] = 30;
    song.rec = 1;
    seq_start();
    seq_tick(t, CTL);
    motion_age_set(t, 60);                              /* step 0: an AGE record and a DJ FILTER record, playing */
    motion_djf_set(t, -20);
    ok = song.playing && motion_age_on && motion_djf_on;
    transport_req = 2;                                  /* (as CF_INIT_SYS: the stop is asked for, not done yet) */
    system_init();
    for (k = G_TAGE; k <= G_TTONE; k++)
        ok &= song.g[k] == GP[k].def;
    snprintf(what, sizeof what, "NEW PROJECT while AGE and DJ FILTER records play: TAPE all 0 (AGE %d WOW %d TONE %d), DJ FILTER %d "
             "(power-on %d)", song.g[G_TAGE], song.g[G_TWOW], song.g[G_TTONE], song.g[G_DJF], GP[G_DJF].def);
    ck(what, ok && song.g[G_DJF] == GP[G_DJF].def && !motion_age_on && !motion_djf_on);
    song.rec = 0;
    seq_stop();
    transport_req = 0;
    ck("NEW PROJECT then the stop: still TAPE all 0, DJ FILTER the power-on's",
       !song.g[G_TAGE] && !song.g[G_TWOW] && song.g[G_DJF] == GP[G_DJF].def);
    ui_power_on();
    tape_reset();
}

/* ---- 9. (Braindancer 0.9) the AGE retune: AGE 50 a clearly worn tape (spec 5) ----
 * The device (docs/braindancer/device-log.md, 2026-10-31; SINE KEY A3 held, its own vibrato 2.25 cents sd): AGE 0 / 50
 * / 100 gave pitch sd 2.25 / 2.42 / 6.95 cents, non-harmonic noise -53 / -50 / -49 dB. Spec 5 at AGE 50: sd >= 2 x AGE
 * 0's (4.50) and noise >= AGE 0's + 6 dB (-47). A host sine has no vibrato and no floor: the targets are TAPE's own
 * share, AGE 0's taken out (in quadrature / in power):
 *   sd     sqrt(4.50^2 - 2.25^2) = 3.90 cents          -> AGE50_SD_MIN 4.0
 *   noise  10 log10(10^-4.7 - 10^-5.3) = -48.3 dB     -> AGE50_NZ_MIN -48.0
 * The bridge between the two trackers: 0.8's AGE 100 row (WOW 100, FLUTTER 50, HISS 50) had 6.58 cents and -51.2 dB of
 * TAPE's own on the device (its AGE 50 reading, 0.89 cents of its own, is far under what the same WOW gives here: the
 * device's window under-reads a slow wow), so AGE 50 must also wobble at least as that row does here and hiss 3 dB more */
#define AGE50_SD_MIN 4.0
#define AGE50_NZ_MIN (-48.0)
static const uint8_t AGE_PT_08[3][5] = {{0, 0, 0, 0, 0}, {60, 20, 30, 20, 35}, {100, 50, 65, 50, 80}};
static const uint8_t AGE_PT_09[3][5] = {{0, 0, 0, 0, 0}, {100, 55, 45, 78, 50}, {100, 100, 85, 100, 90}};
#define AGE_NB (44100u * 20u / CTL)                     /* 20 s */
#define AGE_FFT 32768u
static double age_re[AGE_FFT], age_im[AGE_FFT], age_c[AGE_NB * CTL / 2205u + 2u];
static void age_fft(double *re, double *im, uint32_t n)   /* in place, radix 2 */
{
    uint32_t i, j = 0, k, m;
    for (i = 1; i < n; i++) {
        uint32_t b = n >> 1;
        for (; j & b; b >>= 1) j ^= b;
        j ^= b;
        if (i < j) { double t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (m = 2; m <= n; m <<= 1) {
        double a = -2.0 * M_PI / (double)m;
        for (k = 0; k < n; k += m)
            for (i = 0; i < m / 2u; i++) {
                uint32_t p = k + i, q = p + m / 2u;
                double wr = cos(a * i), wi = sin(a * i);
                double xr = re[q] * wr - im[q] * wi, xi = re[q] * wi + im[q] * wr;
                re[q] = re[p] - xr; im[q] = im[p] - xi;
                re[p] += xr; im[p] += xi;
            }
    }
}
/* TAPE's own wobble and noise on a held A3 (220 Hz, -12 dBFS), the five details d: the pitch's sd in cents over 50 ms
 * windows (20 s, the first 0.5 s left out, its mean removed); the non-harmonic share (dB of the whole, 50 Hz .. 16 kHz:
 * the bins further than 40 Hz from every harmonic; a Hann window of 32768 from 1 s); the share above 3 kHz */
static void age_measure(const uint8_t *d, double *sd, double *nz, double *hi)
{
    uint32_t n = AGE_NB * CTL, nw, i;
    double m = 0, v = 0, tot = 0, non = 0, up = 0;
    tape_reset();
    tape_g(d[0], d[1], d[2], d[3], d[4]);
    tape_tone(220, 8000, AGE_NB);
    nw = cents_of(out_l, n, 22050u, 2205u, 220, age_c);
    for (i = 0; i < nw; i++) m += age_c[i];
    m /= nw;
    for (i = 0; i < nw; i++) v += (age_c[i] - m) * (age_c[i] - m);
    *sd = sqrt(v / nw);
    for (i = 0; i < AGE_FFT; i++) {
        age_re[i] = out_l[44100u + i] * (0.5 - 0.5 * cos(2.0 * M_PI * i / AGE_FFT));
        age_im[i] = 0;
    }
    age_fft(age_re, age_im, AGE_FFT);
    for (i = 1; i < AGE_FFT / 2u; i++) {
        double hz = (double)i * FS / AGE_FFT, p = age_re[i] * age_re[i] + age_im[i] * age_im[i], h = fmod(hz, 220.0);
        if (hz < 50.0 || hz > 16000.0) continue;
        tot += p;
        if (h > 40.0 && h < 180.0) non += p;
        if (hz > 3000.0) up += p;
    }
    *nz = 10.0 * log10(non / tot + 1e-30);
    *hi = 10.0 * log10(up / tot + 1e-30);
}
static void test_age_09(void)
{
    char what[260];
    uint8_t d50[5], d100[5];
    int16_t m[5];
    double sdr, nzr, hir, sd5, nz5, hi5, sd50, nz50, hi50, sd100, nz100, hi100;
    uint32_t k;
    age_measure(AGE_PT_08[1], &sd5, &nz5, &hi5);       /* (0.8's AGE 50, for the record) */
    age_measure(AGE_PT_08[2], &sdr, &nzr, &hir);       /* (0.8's AGE 100: the bridge) */
    tape_age_map(50, m); for (k = 0; k < 5u; k++) d50[k] = (uint8_t)m[k];
    tape_age_map(100, m); for (k = 0; k < 5u; k++) d100[k] = (uint8_t)m[k];
    age_measure(d50, &sd50, &nz50, &hi50);
    age_measure(d100, &sd100, &nz100, &hi100);
    printf("tape: (0.9) host figures, sd cents / noise dB / above 3 kHz dB: 0.8 AGE 50 %.2f / %.1f / %.1f, 0.8 AGE 100 %.2f / "
           "%.1f / %.1f; 0.9 AGE 50 %.2f / %.1f / %.1f, AGE 100 %.2f / %.1f / %.1f\n",
           sd5, nz5, hi5, sdr, nzr, hir, sd50, nz50, hi50, sd100, nz100, hi100);
    snprintf(what, sizeof what, "(0.9) AGE 50: pitch sd %.2f cents (>= %.1f and >= 0.8's AGE 100 %.2f), noise %.1f dB (>= %.1f "
             "and >= %.1f + 3)", sd50, AGE50_SD_MIN, sdr, nz50, AGE50_NZ_MIN, nzr);
    ck(what, sd50 >= AGE50_SD_MIN && sd50 >= sdr && nz50 >= AGE50_NZ_MIN && nz50 >= nzr + 3.0);
    /* (ruling: AGE 100's TONE 90 darkens the noise it adds, so its non-harmonic level is not above AGE 50's: "mangled"
     * means more wobble and a darker tape, hence sd and TONE rise and the share above 3 kHz falls) */
    ck("(0.9) AGE 100 more worn than AGE 50 (sd up, TONE up, above 3 kHz down), AGE 50 darker than 0.8's (TONE >= 50)",
       sd100 >= sd50 && hi100 < hi50 && d100[4] >= d50[4] && d50[4] >= 50u);
    ck("(0.9) the map pinned: TAPE_AGE_PT is the retuned one, row 0 all 0, each detail rising",
       !memcmp(TAPE_AGE_PT, AGE_PT_09, sizeof AGE_PT_09));
    tape_reset();
    tape_age_apply(0);
    tape_tone(220, 8000, 400);
    ck("(0.9) AGE 0: every detail 0, TAPE out of the chain, the output the input bit for bit",
       !tape_set() && !tape.on && !memcmp(out_l, in_l, 400u * CTL * sizeof out_l[0]));
    tape_reset();
}

/* ---- (0.9 final review I1) a 0.8 project loads onto the new map: spec 5 "projects saved in 0.8 with AGE above 0 sound
 * more worn". The format is 0.8's ('T' 'P' unchanged, no version mark), so a 0.8 save is packed here from its six ---- */
static void age08_map(int32_t age, int16_t d[5])         /* 0.8's map (AGE_PT_08, linear as tape_age_map) */
{
    uint32_t k, s = age > 50 ? 1u : 0u;
    int32_t a = age - (int32_t)s * 50;
    for (k = 0; k < 5u; k++)
        d[k] = (int16_t)(AGE_PT_08[s][k] + (AGE_PT_08[s + 1u][k] - AGE_PT_08[s][k]) * a / 50);
}
static int age08_trip(int32_t age, const int16_t d[5], project_store_t *st)   /* AGE and five packed, loaded, restored */
{
    project_t a, c;
    uint32_t k;
    tape_reset();
    song.g[G_TAGE] = (int16_t)age;
    for (k = 0; k < 5u; k++)
        song.g[G_TWOW + k] = d[k];
    project_capture(&a);
    if (!proj_pack(st, &a)) return 0;
    for (k = G_TAGE; k <= G_TTONE; k++)
        song.g[k] = 0;
    return proj_import(&c, st, sizeof *st) && !project_restore_runtime(&c);
}
static void test_age_08_load(void)
{
    static project_store_t st, st2;
    project_t c;
    int16_t d[5], m[5];
    int32_t ages[] = {1, 25, 50, 77, 100};
    uint32_t i;
    int ok = 1;
    char what[200];
    ui_power_on();
    for (i = 0; i < sizeof ages / sizeof ages[0]; i++) {
        age08_map(ages[i], d);
        tape_age_map(ages[i], m);
        ok &= age08_trip(ages[i], d, &st) && song.g[G_TAGE] == ages[i] && !memcmp(&song.g[G_TWOW], m, sizeof m) &&
              !tape_age_custom();
    }
    snprintf(what, sizeof what, "(0.9) a 0.8 project at AGE 1 25 50 77 100 on 0.8's map: loads onto the new map (AGE 100: %d %d %d "
             "%d %d), none by hand", song.g[G_TWOW], song.g[G_TFLUT], song.g[G_TSAT], song.g[G_THISS], song.g[G_TTONE]);
    ck(what, ok);
    project_capture(&c);
    ok = proj_pack(&st2, &c) && proj_import(&c, &st2, sizeof st2) && !project_restore_runtime(&c);
    tape_age_map(100, m);
    ck("(0.9) the migrated project (AGE 100) saved again and loaded: the new map kept (the migration once)",
       ok && song.g[G_TAGE] == 100 && !memcmp(&song.g[G_TWOW], m, sizeof m));
    age08_map(50, d);
    d[4] = (int16_t)(d[4] + 1);                         /* (TONE turned by hand: 36) */
    ok = age08_trip(50, d, &st) && song.g[G_TAGE] == 50 && !memcmp(&song.g[G_TWOW], d, sizeof d) && tape_age_custom() != 0;
    ck("(0.9) a 0.8 project at AGE 50 with a detail by hand: its five kept as stored", ok);
    age08_map(50, d);
    ok = age08_trip(0, d, &st) && !song.g[G_TAGE] && !memcmp(&song.g[G_TWOW], d, sizeof d);
    ck("(0.9) AGE 0 with details (by hand): kept as stored", ok);
    for (ok = 1, i = 0; i < sizeof ages / sizeof ages[0]; i++) {
        tape_age_map(ages[i], m);
        ok &= age08_trip(ages[i], m, &st) && song.g[G_TAGE] == ages[i] && !memcmp(&song.g[G_TWOW], m, sizeof m) &&
              proj_import(&c, &st, sizeof st) && proj_pack(&st2, &c) && !memcmp(&st, &st2, sizeof st);
    }
    ck("(0.9) a 0.9 project on the new map (AGE 1 25 50 77 100): loads unchanged, byte for byte", ok);
    ui_power_on();
    tape_reset();
}

/* ---- (0.9) WOW from rest: switched on fresh, it moves within the first second (its first segment from the centre is
 * short; it was always 3.33 s of half a swing, -0.47 cents: a device run measured over its first seconds heard none).
 * The pitch: the 220 Hz bin's phase over 50 ms windows (2205 samples, exactly 11 cycles: the image at 440 Hz cancels),
 * its step from window to window the frequency, as a band-passed fundamental's instantaneous frequency; the offset in
 * cents averaged over 200 ms (4 windows), its largest from 0.1 s to 1.0 s ---- */
#define WS_W 2205u
static double wow_start_cents(void)
{
    uint32_t k, i, nw = 44100u / WS_W;                  /* (the first second: 20 windows) */
    double ph[44100u / 2205u], c[44100u / 2205u], best = 0;
    tape_reset();
    tape_g(100, 0, 0, 0, 0);
    tape_tone(220, 8000, 44100u / CTL + 2u);
    for (k = 0; k < nw; k++) {
        double re = 0, im = 0;
        for (i = 0; i < WS_W; i++) {
            double a = 2.0 * M_PI * 220.0 * (double)(k * WS_W + i) / FS;
            re += out_l[k * WS_W + i] * cos(a);
            im -= out_l[k * WS_W + i] * sin(a);
        }
        ph[k] = atan2(im, re);
    }
    for (k = 1; k < nw; k++) {
        double d = ph[k] - ph[k - 1u];
        while (d > M_PI) d -= 2.0 * M_PI;
        while (d < -M_PI) d += 2.0 * M_PI;
        c[k] = 1200.0 * log2(1.0 + d / (2.0 * M_PI * WS_W / FS) / 220.0);   /* (d: 2 pi df a window) */
    }
    for (k = 3; k + 3u < nw; k++) {                     /* (from 0.1 s: the line's delay and fade-in left out) */
        double m = (c[k] + c[k + 1u] + c[k + 2u] + c[k + 3u]) / 4.0;
        if (fabs(m) > fabs(best)) best = m;
    }
    tape_reset();
    return best;
}
static void test_wow_start(void)
{
    char what[160];
    double c = wow_start_cents();
    snprintf(what, sizeof what, "(0.9) WOW 100 from rest: the pitch %+.2f cents off within the first second (|.| >= 2)", c);
    ck(what, fabs(c) >= 2.0);
}

int main(void)
{
    test_off();
    test_wow_start();
    test_off_again();
    test_tone_through();
    test_in_out();
    test_rung_click();
    test_huge();
    test_rest();
    test_rest_off_play();
    test_store();
    test_age_auto();
    test_new_project_motion();
    test_age_09();
    test_age_08_load();
    printf(bad ? "tape: %d FAILED\n" : "tape: all checks ok\n", bad);
    return bad != 0;
}
