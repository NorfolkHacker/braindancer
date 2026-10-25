/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* (0.8.5) the lens panels of LFO . MOD and GLO (ui_lens.c), as the approved mock-up's; included by ui_graph.c after
 * ui_lzedit.c (ed_cap). The canvas is the lens's panel (240 x 140, x 4..236). (ACID FM 3b task 6: ENV's TRACKS and STAGE,
 * SCL's KEY and CHORD, ARP's panels gone: those sections are ux_section.c's pages, their pictures there) */
/* LFO . MOD  MOD: the matrix as a net, the sources down the left (those a slot reads lit), a curve from each slot's
 *            source to its box D1..D4 on the right (DEST, AMT as a bar), the slot KNOB 1 picked framed, a pulse
 *            running along its curve; LFO: graph_lfo under "RATE . WAVE"; LFO DEST: the LFO small on the left, the four
 *            amounts as rows, each lit by what it sends now; MSEQ, MSEQ 2: graph_mseq under its caption */
static void graph_mod_net(const track_t *t)
{
    uint32_t k, s, used = 0;
    for (k = 0; k < NMSLOT; k++)
        used |= 1u << clamp(t->p[P_M1SRC + 3u * k], 0, MS_N - 1);
    for (s = 1; s < (uint32_t)MS_N; s++)
        cv_text(10, 6 + (int32_t)(s - 1u) * 9 - AF_X_CAP_Y, &AF_X, N_MSRC[s], used >> s & 1u ? T_TEXT : T_DIM);
    for (k = 0; k < NMSLOT; k++) {
        const int16_t *p = &t->p[P_M1SRC + 3u * k];
        int32_t y = 4 + (int32_t)k * 33, on = p[0] && p[1], sel = k == mod_ui_slot, sy = 9 + (p[0] - 1) * 9, w;
        uint16_t bg = sel ? T_TINT : T_RAISE, lc = sel ? T_THEME : on ? T_MID : T_DIM;
        char b[16];
        const char *u = "";
        if (on) {
            og_quad_a(48 * 16, sy * 16, 110 * 16, sy * 16, 158 * 16, (y + 11) * 16, lc, 16u, 256);
            if (sel && !(ui_prefs & PREF_ANIM_OFF)) {   /* (the pulse: a quadratic's point at u = frame) */
                int32_t q = (int32_t)(ui.frame * 6u % 256u), r = 256 - q;
                og_node((48 * r * r + 2 * 110 * r * q + 158 * q * q) >> 12, (sy * r * r + 2 * sy * r * q + (y + 11) * q * q) >> 12,
                        2, T_TEXT);
            }
        } else
            cv_text(124, y + 9 - AF_X_CAP_Y, &AF_X, "----", T_DIM);
        cv_rect(160, y, 72, 26, bg);
        if (sel) {
            cv_rect(160, y, 72, 1, T_THEME); cv_rect(160, y + 25, 72, 1, T_THEME);
            cv_rect(160, y, 1, 26, T_THEME); cv_rect(231, y, 1, 26, T_THEME);
        }
        b[0] = 'D'; b[1] = (char)('1' + k); b[2] = ' '; b[3] = 0;
        str_cpy(b + 3, on ? mod_dst_name(t, p[1]) : "OFF", 13);
        cv_clip_text(165, y + 5 - AF_X_CAP_Y, &AF_X, b, on ? T_TEXT : T_DIM, bg, 64);
        param_format(&TP[P_M1AMT], p[2], b, &u);
        w = p[2] * 30 / 128;
        cv_rect(166, y + 17, 60, 2, T_LINE);
        if (on && w)
            cv_rect(w < 0 ? 196 + w : 196, y + 16, w < 0 ? -w : w, 4, p[2] < 0 ? T_ACCENT : T_THEME);
    }
}
static void lfo_cap(const track_t *t, int32_t x, int32_t y, int mode)   /* "1.18Hz . SIN" (mode: ". FREE" / ". TRIG") */
{
    const char *u = "";
    char b[32], w[12];
    param_format(&TP[P_LRATE], t->p[P_LRATE], b, &u);
    str_cpy(b + str_len(b), u, 32 - str_len(b));
    str_cpy(b + str_len(b), " \xB7 ", 32 - str_len(b));
    param_format(&TP[P_LWAVE], t->p[P_LWAVE], w, &u);
    str_cpy(b + str_len(b), w, 32 - str_len(b));
    if (mode)                                           /* (0.9.18) LFO, LFO 2: MODE too (LFO DEST: no room) */
        str_cpy(b + str_len(b), t->p[P_LPHASE] < 0 ? " \xB7 FREE" : " \xB7 TRIG", 32 - str_len(b));
    ed_cap(x, y, b, T_TEXT);
}
static void graph_lfodest(const track_t *t, const page_t *pg)
{
    int32_t x, py = 0, src = mulq15(t->lfo_val, t->lfo_fade), k;
    uint32_t span = 0xFFFFFFFFu / 40u, ph = t->lfo_ph - 20u * span;
    lfo_cap(t, 10, 6, 0);
    cv_rect(10, 70, 80, 1, T_RAISE);
    for (x = 0; x < 80; x++) {                          /* (two cycles, the phase now a third in) */
        int32_t y = 70 - lfo_wave((track_t *)t, ph + (uint32_t)x * span) * 44 / 32768;
        if (x)
            cv_line_t(10 + x - 1, py, 10 + x, y, T_THEME, 2);
        py = y;
    }
    cv_rect(30, 22, 1, 96, T_MID);
    og_node(30 * 16, (70 - src * 44 / 32768) * 16, 3, T_TEXT);
    for (k = 0; k < 4; k++) {                           /* the amounts, lit by the LFO now */
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, (uint32_t)k, &vp);
        int32_t y = 8 + k * 32, w = *vp * 46 / 100, live = w * src / 32767;
        const char *u = "";
        char b[16];
        if (!d)
            continue;
        cv_rect(100, y, 132, 26, T_RAISE);
        ed_cap(105, y + 4, d->label, T_MID);
        param_format(d, *vp, b, &u);
        str_cpy(b + str_len(b), u, 16 - str_len(b));
        cv_text_r(228, y + 4 - AF_X_CAP_Y, &AF_X, b, T_TEXT, T_RAISE);
        cv_rect(106, y + 17, 120, 2, T_LINE);
        if (w)
            cv_rect(w < 0 ? 166 + w : 166, y + 16, w < 0 ? -w : w, 4, ux_mix(T_RAISE, T_THEME, 45));
        if (live)
            cv_rect(live < 0 ? 166 + live : 166, y + 16, live < 0 ? -live : live, 4, T_THEME);
    }
}
static void mseq_cap(const track_t *t, const page_t *pg)
{
    const char *u = "";
    char b[24];
    if (pg->id[0] == P_MSDIV) {                         /* MSEQ 2: "DIV 1/16" */
        str_cpy(b, "DIV ", 24);
        param_format(&TP[P_MSDIV], t->p[P_MSDIV], b + 4, &u);
        ed_cap(10, 6, b, T_ACCENT);
    } else
        ed_cap(10, 6, "MOD SRC: STEP", T_TEXT);
    str_cpy(b, "LEN ", 24);
    fmt_int(b + 4, t->p[P_MSLEN]);
    cv_text_r(228, 6 - AF_X_CAP_Y, &AF_X, b, T_DIM, T_SURF);
}
/* the lens's panel for LFO . MOD's page pg (1), else 0 (its own) */
static int graph_mod_lz(const track_t *t, const page_t *pg)
{
    if (pg->graph == GR_MOD)
        graph_mod_net(t);
    else if (pg->graph == GR_LFO) {
        lfo_cap(t, 10, 6, 1);
        cv_oy = GOY + 12;
        graph_lfo(t, T_THEME);
    } else if (pg->id[0] == P_LD_PIT)
        graph_lfodest(t, pg);
    else if (pg->graph == GR_MSEQ || pg->id[0] == P_MSDIV) {
        mseq_cap(t, pg);
        cv_oy = GOY + 12;
        graph_mseq(t, T_THEME);
    } else
        return 0;
    return 1;
}

/* GLO  MIX: the six strips (draw_tracks, track_col6; SENDS: the four sends as bars); GLOBAL keeps its bar of sixteenths;
 *      SYSTEM: the tracks that play as chips, LOAD's three settings (the one set lit), MIDI's route, the CPU load over
 *      the last seconds as a line */
static void graph_system(void)
{
    static const char *const LD[3] = {"ECO", "LITE", "FULL"};
    static uint8_t cpu[56], cpu_at;
    static uint32_t cpu_ms;
    uint32_t c, k, ld = 3;
    int32_t py = 0;
    const char *u = "";
    char b[24];
    int16_t *vp;
    const param_desc_t *dl = page_desc(cur_page(), 1, &vp);   /* (LOAD, as its card names it) */
    if (dl) {
        param_format(dl, *vp, b, &u);
        for (k = 0; k < 3u; k++)
            if (str_eq(b, LD[k]))
                ld = k;
    }
    for (c = 0; c < NTRK; c++) {
        char n[3] = {'T', (char)('1' + c), 0};
        uint16_t bg = c < trk_live ? T_THEME : T_RAISE;
        cv_rect(8 + (int32_t)c * 22, 8, 20, 16, bg);
        cv_text_in(8 + (int32_t)c * 22, 12 - AF_X_CAP_Y, 20, &AF_X, n, c < trk_live ? T_INK : T_DIM, bg);
    }
    fmt_int(b, (int32_t)trk_live);
    str_cpy(b + str_len(b), " TRACKS PLAY", 24 - str_len(b));
    ed_cap(144, 13, b, T_TEXT);
    for (k = 0; k < 3u; k++) {
        uint16_t bg = k == ld ? T_THEME : T_RAISE;
        cv_rect(8 + (int32_t)k * 40, 34, 38, 14, bg);
        cv_text_in(8 + (int32_t)k * 40, 38 - AF_X_CAP_Y, 38, &AF_X, LD[k], k == ld ? T_INK : T_MID, bg);
    }
    param_format(&GP[G_ROUTE], song.g[G_ROUTE], b + 5, &u);
    b[0] = 'M'; b[1] = 'I'; b[2] = 'D'; b[3] = 'I'; b[4] = ' ';
    ed_cap(134, 38, b, T_DIM);
    if (fm1_ms - cpu_ms >= 250u) {                      /* (a sample each quarter second) */
        cpu_ms = fm1_ms;
        cpu[cpu_at++ % 56u] = (uint8_t)clamp((int32_t)(song.cpu_q8 * 100u / 256u), 0, 100);
    }
    for (k = 0; k < 56u; k++) {
        int32_t y = 126 - cpu[(cpu_at + k) % 56u] * 56 / 100;
        if (k)
            cv_line_t(8 + (int32_t)(k - 1u) * 4, py, 8 + (int32_t)k * 4, y, T_THEME, 2);
        py = y;
    }
    cv_rect(8, 127, 224, 1, T_RAISE);
    str_cpy(b, "CPU ", 24);
    fmt_int(b + 4, (int32_t)(song.cpu_q8 * 100u / 256u));
    str_cpy(b + str_len(b), "%", 2);
    cv_text_r(230, 62 - AF_X_CAP_Y, &AF_X, b, T_TEXT, T_SURF);
}
