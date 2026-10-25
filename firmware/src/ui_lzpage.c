/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* (0.8.5) the lens panels of GLO (ui_lens.c), as the approved mock-up's; included by ui_graph.c after
 * ui_lzedit.c (ed_cap). The canvas is the lens's panel (240 x 140, x 4..236). (ACID FM 3b task 6: ENV's TRACKS and STAGE,
 * SCL's KEY and CHORD, ARP's panels gone: those sections are ux_section.c's pages, their pictures there) */
/* (ACID FM 3b task 7) LFO . MOD's panels (graph_mod_net, graph_lfodest, lfo_cap, mseq_cap, graph_mod_lz) gone: LFO is drawn by
 * ux_section.c (its wave, DEST, MOD MATRIX, MORE) */

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
