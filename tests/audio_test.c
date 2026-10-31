/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* The real audio ISR with a simulated DMA and clock: overload must free work
 * within one block without cutting another part's lead or bass. */
#define main hostsim_main
#include "hostsim.c"
#undef main

#define FM1_AUDIO_HALF 0x80u
#define FM1_TICKS_PER_US 1u
static uint32_t host_clock, host_us, host_half_reads, host_dma_advanced, host_acks, host_nest;
static uint8_t host_pending;
static volatile uint32_t t5_nested_ticks;              /* audio.c's: TIMER5 nested in the render (main.c) */
static uint32_t fm1_ticks(void) { t5_nested_ticks = host_nest; host_clock += host_us; return host_clock; }
static uint8_t fm1_audio_pending(void) { return host_pending; }
static void fm1_audio_ack_aux(uint8_t p) { (void)p; }
static uint32_t fm1_audio_free_half(void) { return host_dma_advanced && host_half_reads++ > 0u; }
static void fm1_audio_ack_half(void) { host_acks++; }
static void fm1_audio_init(int32_t *b, uint32_t n, void (*isr)(void), uint32_t p)
{ (void)b; (void)n; (void)isr; (void)p; }
void isr_alnk0(void) {}
#define FELUCCA_UAC 1                                  /* audio.c taps USB audio: here into host_tap */
static int32_t host_tap[2 * 64];
static uint32_t host_tap_n;
static void uac_tap(const int32_t *out, uint32_t n) { memcpy(host_tap, out, 8u * n); host_tap_n = n; }
static uint32_t host_acks_at_start;
static void uac_render_start(void) { host_acks_at_start = host_acks; }   /* (just before the blocks) */
static void uac_render_end(void) {}
#include "../firmware/src/audio.c"
enum { FM1_ADC_BATT = 3, FM1_ADC_MASTER = 4 };
static int32_t host_pot = 1023;                        /* the MASTER pot as the ADC reads it (-1: timeout) */
static int32_t fm1_adc_read(uint32_t ch) { return ch == FM1_ADC_MASTER ? host_pot : 620; }
#include "../firmware/src/master.c"

static int bad;
static void check(const char *what, int ok)
{
    printf("audio: %-74s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
}
static void fresh(void)
{
    uint32_t p;
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    memset(&chain, 0, sizeof chain);
    host_tracks_init();
    for (p = 0; p < NPART; p++) host_preset(&trk[p], 0, 0);
    memset(&felucca_dbg, 0, sizeof felucca_dbg);
    audio_cpu_rem = audio_halves = audio_max_us = 0;
    shed_req = 0; shed_count = 0; shed_over = 0;
    audio_last_half = 2u; audio_lost = 0;
    load_tight = 0; tight_calm = 0; memset((void *)&load_diag, 0, sizeof load_diag);
    host_clock = host_us = host_half_reads = host_dma_advanced = host_acks = host_nest = 0;
    host_pending = FM1_AUDIO_HALF;
    transport_req = panic_req = 0;
}
static voice_t *held(track_t *t, uint32_t note)
{
    uint32_t i;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].note == note) return &t->v[i];
    return 0;
}
static void overload(void)
{
    voice_t *lead, *bass, *extra;
    uint32_t mode, i;
    int32_t out[2 * CTL];
    fresh();
    trk[0].p[P_VOICE] = V_MONO;
    trk_note_on(&trk[0], 40, 100);
    trk[1].p[P_VOICE] = V_POLY;
    trk_note_on(&trk[1], 48, 100);
    trk_note_on(&trk[1], 60, 100);
    lead = held(&trk[0], 40); bass = held(&trk[1], 48); extra = held(&trk[1], 60);
    shed_voice();
    check("overload keeps each part's MONO lead and lowest POLY note",
          lead && bass && extra && lead->gate && bass->gate && extra->stage == 4u && shed_count == 1u);
    check("overload: its victim leaves the budget at once (it fades, 0.6.74)", voices_busy() == 2u);
    for (i = 0; i < 6u; i++) mix_block(out, CTL);
    check("overload frees its victim within 6 control blocks (~4 ms, faded: 0.6.74, was one)", extra && !extra->active && voices_busy() == 2u);
    shed_voice();
    check("a lead alone on each part stays protected", lead->gate && bass->gate && shed_count == 1u);

    fresh();
    host_preset(&trk[0], ENGI_DRUM, 0);
    trk_note_on(&trk[0], 36, 100); trk_note_on(&trk[0], 38, 100);
    lead = held(&trk[0], 36); extra = held(&trk[0], 38);
    shed_voice();
    for (i = 0; i < 6u; i++) mix_block(out, CTL);
    check("overload frees a held one-shot drum without waiting for its decay (faded over ~4 ms)",
          lead && extra && lead->active && !extra->active && voices_busy() == 1u);

    fresh();
    trk[0].p[P_VOICE] = V_UNISON;
    trk_note_on(&trk[0], 55, 100);
    shed_voice(); mix_block(out, CTL);
    check("UNISON loses an extra voice and retains its lead", trk[0].v[0].active && trk[0].v[0].gate && voices_busy() == NVOICE - 1u);

    for (mode = V_MONO; mode <= V_LEGATO; mode++) {
        fresh(); trk[0].p[P_VOICE] = V_POLY;
        for (i = 0; i < NVOICE; i++) trk_note_on(&trk[0], 48u + 2u * i, 100);
        trk[0].p[P_VOICE] = (int16_t)mode;
        shed_voice(); mix_block(out, CTL);
        check("overload can reclaim stale POLY voices after a MONO / LEGATO change",
              shed_count == 1u && trk[0].v[0].gate && voices_busy() == NVOICE - 1u);
    }

    fresh();
    trk[0].p[P_VOICE] = V_POLY;
    trk_note_on(&trk[0], 60, 100); trk_note_on(&trk[0], 64, 100);
    lead = held(&trk[0], 60); extra = held(&trk[0], 64);
    trk_note_off(&trk[0], 60); trk_note_off(&trk[0], 64);
    lead->env = 10000; extra->env = 10;
    shed_voice();
    check("released voices shed the quieter tail first", extra->stage == 4u && lead->stage != 4u);
}
static void dma(void)
{
    uint32_t i, expected;
    fresh();
    host_pending = 0;
    fm1_alnk0_irq();
    check("an auxiliary interrupt does not render a DMA half", !host_acks && !audio_halves && !felucca_dbg.in_audio);
    host_pending = FM1_AUDIO_HALF;
    host_us = 5000;
    fm1_alnk0_irq();
    check("one slow half records elapsed load, tightens the budget, does not shed yet (0.6.19: two in a row again)",
          host_acks == 1u && !shed_req && felucca_dbg.last_us == 5000u && !felucca_dbg.late && load_tight &&
          load_diag.peak >= 80u);
    fm1_alnk0_irq();
    check("a second slow half in a row requests shedding", host_acks == 2u && shed_req);
    shed_req = 0;
    host_half_reads = 0; host_dma_advanced = 1; host_us = 100;
    fm1_alnk0_irq();
    check("DMA advancing during a render is counted once", host_acks == 3u && felucca_dbg.late == 1u && !felucca_dbg.in_audio && !shed_req);
    check("(0.8.9) the half is acknowledged before it renders: a flip during a late render stays pending",
          host_acks_at_start == host_acks && host_acks_at_start == 3u);
    {   /* the free half alternates: none lost; the same half twice in a row: the one between was never rendered */
        uint32_t lost0 = audio_lost;
        host_dma_advanced = 0;                         /* (free half 0 each time) */
        fm1_alnk0_irq();
        check("(0.8.9) the same free half twice in a row counts a lost half (audio_lost)", audio_lost == lost0 + 1u);
    }
    fresh();
    expected = HALF_FRAMES * 1000000u / FS;            /* the half's deadline in us */
    host_us = expected * 90u / 100u; host_nest = expected * 30u / 100u;   /* the render 60 %, TIMER5 nested 30 % */
    fm1_alnk0_irq(); fm1_alnk0_irq();
    check("TIMER5 nested in the render counts toward the deadline", shed_req && felucca_dbg.last_us == host_us - host_nest);
    fresh();
    host_us = 5000;
    fm1_alnk0_irq(); host_us = 100; fm1_alnk0_irq(); host_us = 5000; fm1_alnk0_irq();
    check("a lone slow half between fast ones does not shed (0.6.19: a spike is not an overload)", !shed_req);
    fresh();
    host_us = 580;
    for (i = 0; i < 512u; i++) fm1_alnk0_irq();
    expected = host_us * 256u / (HALF_FRAMES * 1000000u / FS);
    check("steady CPU load converges without integer smoothing bias", song.cpu_q8 >= expected - 1u && song.cpu_q8 <= expected);
}
/* MENU > USB LEVEL (fx.c fx_usb_fixed, audio.c audio_block): MASTER (default) as before, USB = the DAC's signal and
 * follows the knob; FIXED: USB at the full level whatever MASTER is (MASTER 0: USB still plays, the DAC is silent),
 * the DAC exactly USB scaled by MASTER */
static double block_rms(const int32_t *b, uint32_t n)
{
    double s = 0;
    uint32_t i;
    for (i = 0; i < 2u * n; i++) s += (double)b[i] * b[i];
    return sqrt(s / (2.0 * n));
}
static double usb_run(uint32_t fixed, uint32_t master, int dac_is_tap_scaled_ok[1], double *dac_rms)
{
    int32_t out[2 * 32];
    uint32_t b, i;
    double usb = 0, dac = 0;
    fresh();
    lim_env = LIM_T; lev_env = 0; lev_g = lev_cur = 32768; lev_dg = 0; dc_l = dc_r = dce_l = dce_r = 0;    /* (the master's state as at power-on: runs compare) */
    trk[0].p[P_VOICE] = V_POLY;
    trk[0].p[P_DIST] = trk[0].p[P_CHOR] = trk[0].p[P_DLY] = trk[0].p[P_REV] = 0;   /* (no tails between runs) */
    trk_note_on(&trk[0], 48, 110); trk_note_on(&trk[0], 55, 110); trk_note_on(&trk[0], 64, 110);
    fx_usb_fixed = (uint8_t)fixed;
    song.master_q12 = master;
    dac_is_tap_scaled_ok[0] = 1;
    for (b = 0; b < 400u; b++) {                       /* ~0.3 s */
        audio_block(out, 32);
        for (i = 0; i < 64u; i++) {
            int32_t want = fixed ? ((host_tap[i] * (int32_t)master) >> 12) << OUT_SHIFT : host_tap[i] << OUT_SHIFT;
            if (out[i] != want) dac_is_tap_scaled_ok[0] = 0;
        }
        if (b >= 100u) { usb += block_rms(host_tap, 32); dac += block_rms(out, 32) / (1 << OUT_SHIFT); }
    }
    fx_usb_fixed = 0;
    *dac_rms = dac / 300.0;
    return usb / 300.0;
}

/* (0.7.5) MENU > USB IN+OUT (fx.c uin_mix): the host's audio (uin_l / uin_r, as usb.c uac_in_fill leaves them) comes in
 * on the last live track: into the dry mix with its LEVEL and PAN, its REVERB send from the sum, silenced by its MUTE
 * or LEVEL 0, nothing when uin_on is clear */
static void uin_run(int32_t level, int32_t pan, int mute, int32_t rev, int on, double *lr, double *rr, double *wr)
{
    int32_t out[2 * 32];
    uint32_t b, i, t, ph = 0;
    double l = 0, r = 0, w = 0;
    fresh();
    lim_env = LIM_T; lev_env = 0; lev_g = lev_cur = 32768; lev_dg = 0; dc_l = dc_r = dce_l = dce_r = 0;
    song.master_q12 = 4096;
    for (t = 0; t < NTRK; t++) trk[t].p[P_DIST] = trk[t].p[P_CHOR] = trk[t].p[P_DLY] = trk[t].p[P_REV] = 0;
    t = trk_live - 1u;
    trk[t].p[P_LEVEL] = (int16_t)level; trk[t].p[P_PAN] = (int16_t)pan; trk[t].p[P_MUTE] = (int16_t)mute; trk[t].p[P_REV] = (int16_t)rev;
    for (b = 0; b < 400u; b++) {
        for (i = 0; i < 32u; i++, ph++) {
            uin_l[i] = (int32_t)(8000.0 * sin(2.0 * M_PI * 440.0 * ph / 44100.0));
            uin_r[i] = uin_l[i];
        }
        uin_on = (uint8_t)on;
        audio_block(out, 32);
        if (b >= 100u) {
            for (i = 0; i < 32u; i++) {
                l += (double)(out[2u * i] >> OUT_SHIFT) * (out[2u * i] >> OUT_SHIFT);
                r += (double)(out[2u * i + 1u] >> OUT_SHIFT) * (out[2u * i + 1u] >> OUT_SHIFT);
                w += (double)wet[i] * wet[i];
            }
        }
    }
    uin_on = 0;
    *lr = sqrt(l / 9600.0); *rr = sqrt(r / 9600.0); *wr = sqrt(w / 9600.0);
}
static void usb_in(void)
{
    double l, r, w, l0, r0, w0;
    uin_run(100, 0, 0, 0, 1, &l, &r, &w);
    printf("audio: USB IN on the last track: L %.0f R %.0f wet %.0f\n", l, r, w);
    check("USB IN+OUT: the host's audio sounds through the last live track (both sides, centred), no send: no wet",
          l > 500.0 && fabs(20.0 * log10(l / r)) < 0.1 && w == 0.0);
    uin_run(100, 0, 0, 0, 0, &l0, &r0, &w0);
    check("USB IN+OUT: nothing when no audio comes in (uin_on clear)", l0 < 1.0 && r0 < 1.0);
    uin_run(100, 0, 1, 0, 1, &l0, &r0, &w0);
    check("USB IN+OUT: the track's MUTE silences it", l0 < 1.0 && r0 < 1.0);
    uin_run(0, 0, 0, 0, 1, &l0, &r0, &w0);             /* (twice: the master's DC block settles after the run before) */
    uin_run(0, 0, 0, 0, 1, &l0, &r0, &w0);
    check("USB IN+OUT: the track's LEVEL 0 silences it", l0 < 1.0 && r0 < 1.0);
    uin_run(100, -64, 0, 0, 1, &l0, &r0, &w0);
    check("USB IN+OUT: PAN full left: the right side out", l0 > 500.0 && r0 < l0 * 0.02);
    uin_run(100, 0, 0, 127, 1, &l0, &r0, &w0);
    printf("audio: USB IN with REVERB 127: wet %.0f\n", w0);
    check("USB IN+OUT: the track's REVERB send takes it into the reverb", w0 > 100.0);
    {   /* (0.8.0) STEMS+IN (fx_stems 2): the host's audio in a stem of its own, not in the last track's; IN+OUT with
         * the 8 stems (fx_stems 1): in the track's, as before */
        double ms = 0, ts = 0;
        uint32_t i, t = trk_live - 1u;
        fx_stems = 2;
        uin_run(100, 0, 0, 0, 1, &l0, &r0, &w0);
        for (i = 0; i < 32u; i++) { ms += fabs((double)stem_buf[NPART][i]); ts += fabs((double)stem_buf[t][i]); }
        check("USB STEMS+IN: the host's audio in its own stem, the last track's stem without it", ms > 32.0 * 1000.0 && ts < 32.0);
        fx_stems = 1;
        uin_run(100, 0, 0, 0, 1, &l0, &r0, &w0);
        ms = ts = 0;
        for (i = 0; i < 32u; i++) { ms += fabs((double)stem_buf[NPART][i]); ts += fabs((double)stem_buf[t][i]); }
        check("USB IN+OUT with the 8 stems: the host's audio in the last track's stem (as 0.7.5)", ts > 32.0 * 1000.0 && ms == 0.0);
        fx_stems = 0;
    }
}
static void usb_level(void)
{
    int ok_full, ok_quiet, ok_off, ok_fixed, ok_fixed0;
    double d_full, d_quiet, d_off, d_fixed, d_fixed0;
    double u_full = usb_run(0, 4096, &ok_full, &d_full), u_quiet = usb_run(0, 1024, &ok_quiet, &d_quiet);
    double u_off = usb_run(0, 0, &ok_off, &d_off), u_fixed = usb_run(1, 1024, &ok_fixed, &d_fixed);
    double u_fixed0 = usb_run(1, 0, &ok_fixed0, &d_fixed0);
    printf("audio: USB / DAC rms: MASTER full %.0f / %.0f, 1/4 %.0f / %.0f, 0 %.0f / %.0f; FIXED 1/4 %.0f / %.0f, 0 %.0f / %.0f\n",
           u_full, d_full, u_quiet, d_quiet, u_off, d_off, u_fixed, d_fixed, u_fixed0, d_fixed0);
    check("USB LEVEL MASTER (default): USB is the DAC's signal and follows the knob (0: silent)",
          ok_full && ok_quiet && ok_off && u_full > 1000.0 && u_quiet < u_full * 0.5 && u_off == 0.0);
    check("USB LEVEL FIXED: USB at the full level whatever MASTER is (within 0.1 dB), MASTER 0 too",
          fabs(20.0 * log10(u_fixed / u_full)) < 0.1 && fabs(20.0 * log10(u_fixed0 / u_full)) < 0.1);
    check("USB LEVEL FIXED: the DAC gets exactly USB scaled by MASTER (MASTER 0: silent, 1/4: -12 dB)",
          ok_fixed && ok_fixed0 && d_fixed0 == 0.0 && fabs(20.0 * log10(d_fixed / u_fixed) + 12.04) < 0.2);
}

/* #137: keys pressed under the splash. main.c: master_boot before audio_init (the pot's level from the first block),
 * kb_boot_hold until the splash ends (seq.c keyboard_block: the keys sound nothing; one held across stays silent until
 * pressed again); the splash polls the pot (master_poll). boot 0: as before the fix */
static void boot_fresh(void)
{
    fresh();
    lim_env = LIM_T; lev_env = 0; lev_g = lev_cur = 32768; lev_dg = 0; dc_l = dc_r = dce_l = dce_r = 0;
    memset(kb_note, 0, sizeof kb_note); kb_prev = 0; kb_layer = 0;
    trk[0].p[P_VOICE] = V_POLY;
    trk[0].p[P_DIST] = trk[0].p[P_CHOR] = trk[0].p[P_DLY] = trk[0].p[P_REV] = 0;
}
static double boot_run(int32_t pot, int boot, uint32_t *splash_peak, uint32_t *first_master, uint32_t *busy)
{
    int32_t out[2 * 32];
    uint32_t b, i, peak = 0;
    double r = 0;
    boot_fresh();
    song.master_q12 = 2048;                            /* felucca_init */
    master_knob = 512 * 16;
    host_pot = pot;
    if (boot) {
        master_boot();
        kb_boot_hold = 1;
    }
    *first_master = song.master_q12;
    for (b = 0; b < 600u; b++) {                       /* the splash, ~0.43 s: keys hammered, one held at its end */
        fm1_in.notes = b >= 580u ? 1u << 12 : (b / 40u) & 1u ? (1u << 7) | (1u << 11) : 1u << 4;
        if (b % 14u == 0u && boot)
            master_poll();
        audio_block(out, 32);
        for (i = 0; i < 64u; i++)
            if ((uint32_t)abs(out[i]) > peak) peak = (uint32_t)abs(out[i]);
    }
    *splash_peak = peak;
    kb_boot_hold = 0;                                  /* the UI */
    *busy = 0;
    for (b = 0; b < 400u; b++) {                       /* the held key (silent), let go; then a chord pressed */
        fm1_in.notes = b < 40u ? 1u << 12 : b < 60u ? 0u : (1u << 0) | (1u << 4) | (1u << 7);
        audio_block(out, 32);
        if (b >= 60u) r += block_rms(out, 32);
        if (b == 39u) *busy = voices_busy();
    }
    fm1_in.notes = 0;
    return r / 340.0;
}
static void boot_keys(void)
{
    uint32_t peak, peak0, peak_old, m, m0, m_old, busy, busy0, busy_old, ref_m, b;
    double quiet, silent, old, ref = 0;
    int32_t out[2 * 32];
    quiet = boot_run(300, 1, &peak, &m, &busy);
    silent = boot_run(0, 1, &peak0, &m0, &busy0);
    old = boot_run(300, 0, &peak_old, &m_old, &busy_old);
    boot_fresh();                                      /* the reference: the chord, MASTER at the pot's level by hand */
    song.master_q12 = ref_m = master_of_pot(300);
    for (b = 0; b < 340u; b++) {
        fm1_in.notes = (1u << 0) | (1u << 4) | (1u << 7);
        audio_block(out, 32);
        ref += block_rms(out, 32);
    }
    fm1_in.notes = 0;
    ref /= 340.0;
    printf("audio: boot, pot 300 (MASTER %u): splash peak %u, then rms %.0f (by hand %.0f); before the fix: "
           "MASTER %u, splash peak %u, then rms %.0f\n", m, peak, quiet, ref, m_old, peak_old, old);
    check("#137 boot: MASTER is the pot's level before the first block (pot 300, pot 0)",
          m == ref_m && m0 == 0u && m_old == 2048u);
    check("#137 boot: keys hammered under the splash sound nothing (before the fix: they did)",
          peak == 0u && peak0 == 0u && peak_old > 0u);
    check("#137 boot: a key held across the splash's end stays silent until pressed again",
          busy == 0u && busy0 == 0u && busy_old > 0u);
    check("#137 boot: after the splash the keys play at the pot's level, as set by hand (pot 0: silent)",
          ref > 100.0 && fabs(quiet - ref) < 1e-9 && silent == 0.0);
}

/* (0.9.13) the load governor: over the budget, a voice less every 3 halves (0.9.14: was 6) down to 4; (0.9.14) past 75 %
 * a voice less every half, at 4 and still past it 8 halves LOAD LITE; a shed two less; two sheds in a second, LOAD LITE
 * of its own (CLOUD as ROOM); 0.5 s calm a voice back, 5 s calm LITE off; the MENU's LOAD first */
static void governor(void)
{
    uint32_t i, c0, c1, c2, lite, cap_lo, h1, h2;
    gov_cap = 16u; gov_lite = 0; gov_hot = gov_calm = gov_win = gov_lcalm = 0; gov_sheds = gov_floor = 0;
    c0 = voice_cap();
    for (i = 0; i < 12u; i++) gov_tick(1, 0, 0, 0);
    c1 = gov_cap;
    for (i = 0; i < 200u; i++) gov_tick(1, 0, 0, 0);
    cap_lo = gov_cap;
    gov_tick(1, 0, 0, 1); gov_tick(1, 0, 0, 0); gov_tick(1, 0, 0, 1);
    lite = gov_lite;
    check("governor: over the budget, a voice less every 3 halves (12 halves: 4), down to 4", c1 == 8u && cap_lo == 4u && !gov_floor);
    check("  two sheds in a second: LOAD LITE of its own (the sound's LOAD; the MENU's stays FULL)",
          lite == LOAD_LITE && LOAD_EFF() == LOAD_LITE && sys_load == LOAD_FULL);
    for (i = 0; i < 172u * 3u; i++) gov_tick(0, 0, 1, 0);
    c2 = gov_cap;
    for (i = 0; i < 1720u; i++) gov_tick(0, 0, 1, 0);
    check("  calm: a voice back every 0.5 s, LITE off after 5 s, the cap back to the full",
          c2 == 7u && !gov_lite && voice_cap() == c0);
    gov_tick(0, 0, 0, 1);
    check("  one shed alone: two voices less (12 -> 10), no LITE", gov_cap == 10u && !gov_lite);
    gov_cap = 16u; gov_lite = 0; gov_hot = gov_calm = gov_win = gov_lcalm = 0; gov_sheds = gov_floor = 0;
    for (i = 0; i < 4u; i++) gov_tick(1, 1, 0, 0);
    h1 = gov_cap;
    for (i = 0; i < 4u + 7u; i++) gov_tick(1, 1, 0, 0);
    h2 = gov_lite;
    gov_tick(1, 1, 0, 0);
    check("  (0.9.14) past 75 %: a voice less every half (4 halves: 12 -> 8); at 4 and still past it 8 halves, LITE",
          h1 == 8u && gov_cap == 4u && !h2 && gov_lite == LOAD_LITE);
    gov_cap = 16u; gov_lite = 0; gov_hot = gov_calm = gov_win = gov_lcalm = 0; gov_sheds = gov_floor = 0;
}

/* (ACID FM 0.5.2) the safety net past the governor (audio.c fx_safety_tick): at its floor (4 voices, LITE) and still
 * behind (a half past its own time, or late) two halves in a row: the long delay to its full-rate line (fx.c dly_rate),
 * then, 0.5 s on and still behind, a GEN piece's texture part hushed, then its arp; each counted (the console's
 * status); back in reverse order after 1 s of halves under 80 % each, longer when it comes back within 5 s (no flapping) */
static void safe_reset(uint32_t cap, uint32_t lite)
{
    uint32_t i;
    fresh();
    gov_cap = (uint8_t)cap; gov_lite = (uint8_t)lite; gov_hot = gov_calm = gov_win = gov_lcalm = 0; gov_sheds = gov_floor = 0;
    fx_safe = 0; dly_safety = 0; safe_hush_n[0] = safe_hush_n[1] = 0; tape_safety = 0;
    dsf_behind = 0; dsf_calm = 0; dsf_wait = 0; safe_kills = 0;
    for (i = 0; i < 4u; i++) { dsf_since[i] = 0xFFFFu; dsf_rearm[i] = 0; dsf_seq[i] = 0; }
    dsf_tick = 0;
    memset(&tape, 0, sizeof tape);                      /* (Braindancer 0.8) TAPE off (fresh: its globals 0), out of the chain */
    prf.rest_ph = 0;
    safe_part[0] = safe_part[1] = NTRK;
    song.g[G_BPM] = 120; song.g[G_DTIME] = 8; song.g[G_DMIX] = 127; song.g[G_DFDBK] = 40;   /* 2BAR: 4 s, a quarter rate */
    trk[0].p[P_VOICE] = V_POLY; trk[0].p[P_DLY] = 127; trk_note_on(&trk[0], 40, 100);   /* (a bass held into it) */
    fx.primed = 0;
    for (i = 0; i < 4u; i++) { host_us = (HALF_FRAMES * 1000000u / FS) / 2u; fm1_alnk0_irq(); }   /* (the real delay, rendered) */
    dsf_behind = 0;
}
static uint32_t safe_run(uint32_t n, uint32_t pct, uint32_t *flips)
{
    uint32_t i, was = fx_safe, f = 0, first = 0;
    for (i = 0; i < n; i++) {
        host_us = (HALF_FRAMES * 1000000u / FS) * pct / 100u;
        fm1_alnk0_irq();
        if (fx_safe != was) { f++; was = fx_safe; if (!first) first = i + 1u; }
    }
    if (flips) *flips += f;
    return first;
}
static void safety(void)
{
    uint32_t at, at2, fl, i;
    {   /* behind from the full cap: the governor sheds first, the net only once it is at its floor */
        uint32_t early = 0, n = 0;
        safe_reset(16u, 0u);
        for (i = 0; i < 200u && !fx_safe; i++) {
            safe_run(1u, 130u, 0);
            n++;
            early |= fx_safe && (gov_cap > 4u || !gov_lite);
        }
        printf("audio: safety net from a full cap at 130 %%: on after %u halves (cap %u, LITE %u)\n", n, gov_cap, gov_lite);
        check("safety net: behind with voices left to shed: the governor first (4 voices, LITE), only then the delay",
              !early && fx_safe == SAFE_DLY && dly_safety == 1u && n > 2u);
    }
    safe_reset(4u, LOAD_LITE);
    safe_run(400u, 70u, 0);
    check("  at the floor and keeping up (70 %): untouched", !fx_safe && !dly_safety);
    safe_reset(4u, LOAD_LITE);
    for (i = 0; i < 40u; i++) { safe_run(1u, 120u, 0); safe_run(3u, 70u, 0); }
    check("  at the floor, a lone half behind now and then (a spike): untouched", !fx_safe && !dly_safety);
    safe_reset(4u, LOAD_LITE);
    song.g[G_DTIME] = 1;                                /* (1/8: the full rate) */
    for (i = 0; i < 400u; i++) { host_us = (HALF_FRAMES * 1000000u / FS) * 125u / 100u; fm1_alnk0_irq(); }
    check("  at the floor and behind, the delay already at the full rate, not a GEN piece: nothing to do, not counted",
          !fx_safe && !dly_safety && !safe_hush_n[0] && !safe_hush_n[1]);
    safe_reset(4u, LOAD_LITE);
    at = safe_run(20u, 120u, 0);
    check("  at the floor and behind (120 %): the long delay to its full rate within 2 halves, counted once",
          fx_safe == SAFE_DLY && at >= 1u && at <= 2u && dly_safety == 1u && gov_cap == 4u && gov_lite);
    at = safe_run(343u, 60u, 0);
    at2 = safe_run(2u, 60u, 0);
    check("  calm (60 %): kept for 1 s (344 halves), then back", !at && at2 >= 1u && !fx_safe && dly_safety == 1u);
    safe_reset(4u, LOAD_LITE);
    safe_run(4u, 120u, 0);
    safe_run(300u, 70u, 0); safe_run(1u, 85u, 0); at = safe_run(343u, 70u, 0);
    at2 = safe_run(2u, 70u, 0);
    check("  a half over 80 % in the calm starts its second again", !at && at2 && !fx_safe);
    safe_reset(4u, LOAD_LITE);
    fl = 0;
    for (i = 0; i < 17u; i++) { safe_run(2u, 115u, &fl); safe_run(398u, 75u, &fl); }   /* (20 s) */
    check("  a borderline load (75 %, two halves behind every 1.16 s for 20 s): no flapping (two switches back at most)",
          dly_safety <= 2u && fl <= 3u && fx_safe == SAFE_DLY);
    printf("audio: safety net under the borderline load: %u engaged, %u switches in 20 s\n", dly_safety, fl);
    {   /* the ladder: a GEN piece (texture T6, arp T3), still behind with the delay short: T6 hushed, then T3; back
         * in reverse order */
        uint32_t v, d0, st0;
        safe_reset(4u, LOAD_LITE);
        safe_part[0] = 5u; safe_part[1] = 2u;
        trk[5].p[P_VOICE] = V_POLY; trk_note_on(&trk[5], 60, 100);
        trk[2].p[P_VOICE] = V_POLY; trk_note_on(&trk[2], 64, 100);
        d0 = fx.dr_sh;
        st0 = voices_stolen();
        {   /* the half each lever comes on / goes off */
            uint32_t on[3] = {0, 0, 0}, off[3] = {0, 0, 0}, was = 0, h, full = 0, back = 0;
            for (h = 1; h <= 600u; h++) {
                safe_run(1u, 125u, 0);
                if (!full && !fx.dr_sh) full = h;
                for (i = 0; i < 3u; i++) if ((fx_safe >> i & 1u) && !(was >> i & 1u)) on[i] = h;
                was = fx_safe;
            }
            v = 0;
            for (i = 0; i < NVOICE; i++) v += trk[5].v[i].active + trk[2].v[i].active;
            trk_note_on(&trk[5], 62, 100); trk_note_on(&trk[2], 65, 100); trk_note_on(&trk[0], 43, 100);
            for (i = 0; i < NVOICE; i++) v += (trk[5].v[i].active && trk[5].v[i].note == 62) + (trk[2].v[i].active && trk[2].v[i].note == 65);
            printf("audio: the ladder at 125 %%: the delay at half %u (its line at the full rate at %u), the texture at %u, "
                   "the arp at %u\n", on[0], full, on[1], on[2]);
            check("  a 2BAR delay rendered, at the floor and behind: its line at the full rate within 4 halves of the overload",
                  d0 == 2u && on[0] <= 2u && full && full <= 4u);
            check("  still behind with the delay short: the texture hushed 0.5 s on, then the arp; voices faded, no new notes, "
                  "the bass plays", on[0] <= 2u && on[1] == on[0] + DSF_STEP && on[2] == on[1] + DSF_STEP &&
                  fx_safe == (SAFE_DLY | SAFE_TEX | SAFE_ARP) && !v && held(&trk[0], 43) &&
                  dly_safety == 1u && safe_hush_n[0] == 1u && safe_hush_n[1] == 1u);
            check("  the hush's faded voices are not counted as stolen (the console's voices_stolen)",
                  safe_kills == 2u && voices_stolen() == st0);
            for (h = 1; h <= 1200u; h++) {
                safe_run(1u, 60u, 0);
                if (!back && fx.dr_sh == 2u) back = h;
                for (i = 0; i < 3u; i++) if (!(fx_safe >> i & 1u) && (was >> i & 1u)) off[i] = h;
                was = fx_safe;
            }
            printf("audio: the ladder back at 60 %%: the arp at half %u, the texture at %u, the delay at %u\n", off[2], off[1], off[0]);
            check("  calm: back a lever a second in reverse order (the arp, the texture, the delay)",
                  off[2] == 344u && off[1] == 688u && off[0] == 1032u && !fx_safe);
            check("  .. and the delay's line at a quarter rate again within 3 halves of its lever off", back >= 1032u && back <= 1035u);
        }
        trk_note_on(&trk[5], 67, 100);
        check("  the texture plays again", held(&trk[5], 67) != 0);
    }
    {   /* (Braindancer 0.8) TAPE the first rung: AGE 50, the same GEN piece's ladder at 125 %: TAPE faded out first (its
         * settings kept), then the delay, the texture, the arp a step apart each; back in reverse order, TAPE last */
        uint32_t on[4] = {0, 0, 0, 0}, off[4] = {0, 0, 0, 0}, was = 0, h, zero = 0, out = 0, full = 0, rung, bph = HALF_FRAMES / CTL;
        int16_t g6[6];
        safe_reset(4u, LOAD_LITE);
        tape_age_apply(50);
        memcpy(g6, &song.g[G_TAGE], sizeof g6);
        safe_part[0] = 5u; safe_part[1] = 2u;
        trk[5].p[P_VOICE] = V_POLY; trk_note_on(&trk[5], 60, 100);
        trk[2].p[P_VOICE] = V_POLY; trk_note_on(&trk[2], 64, 100);
        for (h = 1; h <= 4u; h++) safe_run(1u, 60u, 0);   /* (TAPE in the chain, whole) */
        rung = tape.on && tape.g == TAPE_FULL;
        for (h = 1; h <= 700u; h++) {
            safe_run(1u, 125u, 0);
            for (i = 0; i < 4u; i++) if ((fx_safe >> i & 1u) && !(was >> i & 1u)) on[i] = h;
            if (on[3] && !zero && !tape.g) zero = h;
            if (on[3] && !out && !tape.on) out = h;
            was = fx_safe;
        }
        printf("audio: TAPE the first rung at 125 %%: TAPE at half %u (its gain 0 %u halves on, out of the chain at %u), the "
               "delay at %u, the texture at %u, the arp at %u\n", on[3], zero - on[3], out - on[3], on[0], on[1], on[2]);
        check("  (0.8) TAPE in the chain (AGE 50), at the floor and behind: TAPE's rung first, within 2 halves, counted once",
              rung && on[3] >= 1u && on[3] <= 2u && tape_safety == 1u && dly_safety == 1u);
        check("  .. faded out (its gain 0 within TAPE_FADE / CTL + 1 blocks of the rung), out of the chain, AGE's six kept",
              zero && (zero - on[3]) * bph <= TAPE_FADE / CTL + 1u && out && !tape.on && !memcmp(g6, &song.g[G_TAGE], sizeof g6));
        check("  .. then the delay, the texture, the arp a step apart each (as before, a step later)",
              on[0] == on[3] + DSF_STEP && on[1] == on[0] + DSF_STEP && on[2] == on[1] + DSF_STEP &&
              fx_safe == (SAFE_TAPE | SAFE_DLY | SAFE_TEX | SAFE_ARP) && safe_hush_n[0] == 1u && safe_hush_n[1] == 1u);
        zero = 0;
        for (h = 1; h <= 1600u; h++) {
            safe_run(1u, 60u, 0);
            for (i = 0; i < 4u; i++) if (!(fx_safe >> i & 1u) && (was >> i & 1u)) off[i] = h;
            if (off[3] && !zero && tape.on) zero = h;
            if (off[3] && !full && tape.g == TAPE_FULL) full = h;
            was = fx_safe;
        }
        printf("audio: TAPE's rung back at 60 %%: the arp at half %u, the texture at %u, the delay at %u, TAPE at %u (in the "
               "chain %u halves on, whole at %u)\n", off[2], off[1], off[0], off[3], zero - off[3], full - off[3]);
        check("  calm: back in reverse order a second apart (the arp, the texture, the delay, TAPE at 344, 688, 1032, 1376)",
              off[2] == 344u && off[1] == 688u && off[0] == 1032u && off[3] == 1376u && !fx_safe);
        check("  .. TAPE in the chain again, whole within its hold and fade, AGE's six as they were",
              zero == off[3] + 1u && full && (full - off[3] - 1u) * HALF_FRAMES <= (TAPE_D0 >> 8) + 8u + TAPE_FADE + HALF_FRAMES &&
              !memcmp(g6, &song.g[G_TAGE], sizeof g6));
        safe_reset(4u, LOAD_LITE);
        tape_age_apply(50);
        fl = 0;
        {
            uint32_t tw = 0, tf = 0;
            for (i = 0; i < 17u; i++) {
                for (h = 0; h < 400u; h++) {
                    safe_run(1u, h < 2u ? 115u : 75u, &fl);
                    tf += (fx_safe & SAFE_TAPE) != tw;
                    tw = fx_safe & SAFE_TAPE;
                }
            }
            printf("audio: TAPE's rung under the borderline load: %u engaged, %u switches of it in 20 s (its calm x%u)\n",
                   tape_safety, tf, 1u << dsf_rearm[3]);
            check("  (0.8) a borderline load (two halves behind every 1.16 s, 20 s): TAPE's rung switched 3 times at most",
                  tf <= 3u && tape_safety <= 2u);
        }
        {   /* (review 4) the rungs back in the reverse of the order they came on: the delay's rung on (TAPE off), then
             * AGE turned up while still behind: TAPE's rung on after it, so it goes first in the calm */
            uint32_t ot = 0, od = 0, ft = 0, fd = 0, w2 = 0;
            safe_reset(4u, LOAD_LITE);
            for (h = 1; h <= 100u; h++) safe_run(1u, 125u, 0);
            tape_age_apply(50);
            for (h = 1; h <= 300u; h++) {
                safe_run(1u, 125u, 0);
                if ((fx_safe & SAFE_TAPE) && !ot) ot = h;
            }
            od = fx_safe & SAFE_DLY;
            w2 = fx_safe;
            for (h = 1; h <= 800u; h++) {
                safe_run(1u, 60u, 0);
                if (!(fx_safe & SAFE_TAPE) && (w2 & SAFE_TAPE)) ft = h;
                if (!(fx_safe & SAFE_DLY) && (w2 & SAFE_DLY)) fd = h;
                w2 = fx_safe;
            }
            printf("audio: the delay's rung, then AGE up: TAPE's rung %u halves on; back at 60 %%: TAPE at %u, the delay at %u\n",
                   ot, ft, fd);
            check("  (0.8) TAPE's rung on after the delay's (AGE turned up meanwhile): back first (TAPE 344, the delay 688)",
                  od && ot && ft == 344u && fd == 688u && !fx_safe && tape_safety == 1u && dly_safety == 1u);
        }
        safe_reset(4u, LOAD_LITE);
        tape_age_apply(50);
        fx_safe = SAFE_TAPE | SAFE_DLY;
        safe_clear();
        check("  (0.8) new music (safe_clear): TAPE's rung off", !fx_safe);
        {   /* (0.9.1 spec 4) new music (safe_clear) drops the levers on as a calm release does: each stamped released, so
             * one on again within 5 s takes the longer calm (4 s) and the net cannot flap once on the next overload */
            uint32_t h;
            int ok;
            safe_reset(4u, LOAD_LITE);
            safe_part[0] = 5u;
            for (h = 0; h < 400u; h++) safe_run(1u, 125u, 0);
            ok = (fx_safe & (SAFE_DLY | SAFE_TEX)) == (SAFE_DLY | SAFE_TEX);
            safe_clear();
            ok &= !fx_safe && dsf_since[0] == 0u && dsf_since[1] == 0u && dsf_since[2] == 0xFFFFu && dsf_since[3] == 0xFFFFu;
            safe_part[0] = 5u;                          /* (re-armed: a section of the piece loaded) */
            for (h = 0; h < 400u; h++) safe_run(1u, 125u, 0);
            printf("audio: after safe_clear, the delay's and the texture's levers on again: calm x%u, x%u\n",
                   1u << dsf_rearm[0], 1u << dsf_rearm[1]);
            check("  (0.9.1) new music (safe_clear): the levers on stamped released; on again within 5 s: the 4 s calm",
                  ok && (fx_safe & (SAFE_DLY | SAFE_TEX)) == (SAFE_DLY | SAFE_TEX) && dsf_rearm[0] == 2u && dsf_rearm[1] == 2u);
        }
        safe_reset(4u, LOAD_LITE);
    }
    {   /* (review M2) the delay bus at rest (fx_rest: nothing sent, its rate a stale one): its lever not on */
        uint32_t sh;
        safe_reset(4u, LOAD_LITE);
        trk_note_off(&trk[0], 40); trk[0].p[P_DLY] = 0; song.g[G_DFDBK] = 0;
        safe_run(3440u, 50u, 0);                        /* (10 s: the echoes gone, the buses at rest) */
        sh = fx.dr_sh;
        safe_run(100u, 125u, 0);
        printf("audio: the buses at rest (fx_rest's phase %u), the line's rate a quarter's (%u)\n", prf.rest_ph, sh);
        check("  the delay's bus at rest, its line a quarter rate's: the delay lever not on (it would save nothing)",
              prf.rest_ph && sh == 2u && !fx_safe && !dly_safety);
    }
    {   /* (review I3) each lever its own back-off: a load the texture's lever takes from 105 % to 75 % (the delay lever
         * on and staying on), 20 s; then the delay alone, from 105 % to 75 % */
        uint32_t h, was = 0, tex_on = 0, dly_on = 0;
        safe_reset(4u, LOAD_LITE);
        safe_part[0] = 5u;
        for (h = 0; h < 6880u; h++) {
            safe_run(1u, (fx_safe & SAFE_TEX) ? 75u : (fx_safe & SAFE_DLY) ? 105u : 125u, 0);
            tex_on += (fx_safe & SAFE_TEX) && !(was & SAFE_TEX);
            dly_on += (fx_safe & SAFE_DLY) && !(was & SAFE_DLY);
            was = fx_safe;
        }
        printf("audio: 20 s, the texture's lever taking 105 %% to 75 %%: on %u times (its calm x%u), the delay's %u\n",
               tex_on, 1u << dsf_rearm[1], dly_on);
        check("  a lever under a load at its edge (the texture, 105 % -> 75 %, 20 s): on 4 times at most (1, 4, 8, 8 s of "
              "calm), its own calm 8 s; the delay's lever on once", tex_on <= 4u && dsf_rearm[1] == 3u && dly_on == 1u);
        safe_reset(4u, LOAD_LITE);
        was = 0; dly_on = 0;
        for (h = 0; h < 6880u; h++) {
            safe_run(1u, (fx_safe & SAFE_DLY) ? 75u : 105u, 0);
            dly_on += (fx_safe & SAFE_DLY) && !(was & SAFE_DLY);
            was = fx_safe;
        }
        printf("audio: 20 s, the delay's lever taking 105 %% to 75 %%: on %u times\n", dly_on);
        check("  .. the delay's the same (105 % -> 75 %, 20 s): on 4 times at most", dly_on <= 4u && dsf_rearm[0] == 3u);
    }
    safe_reset(16u, 0u);
    trk_note_off(&trk[0], 40);
    song.g[G_DTIME] = 1;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    usb_level();
    boot_keys();
    overload(); dma(); governor();
    usb_in();                                          /* (last: its reverb tail would reach the tests after it) */
    safety();                                          /* (ACID FM 0.5.2: after it, its notes' tails as well) */
    printf(bad ? "audio: %d FAILED\n" : "audio: all passed\n", bad);
    return bad != 0;
}
