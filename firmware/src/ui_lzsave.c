/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* (0.8.5) SAVE's lens (ui.c save_*): the panels of its three levels, drawn as the approved HTML mock-up (SAVE, the
 * proposal "LENTE"), in the lens's panel (240 x 140 at y 84; the mock-up's coordinates less 84). Included by
 * ui_graph.c.
 *   LIBRARY  PROJECT: the four projects as tiles (4 x 1, 56 x 66): the number, the name (its section A's), its four
 *            sections as chips A..D (stored: the section's colour; empty: dark), the project shown TINT with a THEME
 *            frame, its section picked framed TEXT. TOOLS: the six tools as rows (the one picked TINT).
 *   PROJECT  (ACID FM 3c-2 task 3) gone: SAVE > SONG is ux_save.c's (the inherited SECTIONS and ARRANGE pages).
 *   SOUND    (ACID FM 3c-2 task 2) gone: SAVE > SOUND is ux_save.c's (the inherited USER and PRESETS pages).
 * The other projects' sections and names come from their flash headers (project.c lib_scan), one project a frame,
 * cached here until a project is written, loaded or renamed (lib_gen). */
/* a section's chip colour: stored, its colour (half: mixed with BG); empty: BG */
static uint16_t lz_sec(uint32_t s, int used, int half)
{
    uint16_t c = used ? sec_col(s) : half < 0 ? ux_mix(T_BG, T_RAISE, 50) : T_BG;
    return used && half ? ux_mix(T_BG, c, 50) : c;
}
#define LZ_TX(c) (4 + (int32_t)(c) * 117 / 2)          /* four columns of 56 px, 58 or 59 apart (the mock-up's 58.5) */
static void graph_lib(void)
{
    uint32_t i, j;
    for (i = 0; i < NSONG; i++) {
        int32_t x = LZ_TX(i % 4u), y = (int32_t)(i / 4u) * 70;
        int cur = i == song_cur;
        uint16_t bg = cur ? T_TINT : T_RAISE;
        char b[4], nm[13];
        cv_rect(x, y, 56, 66, bg);
        if (cur)
            cv_frame(x, y, 56, 66, T_THEME);
        fmt_int(b, (int32_t)i + 1);
        GFX_HOOK_ALIGN(0, y + 3, 0, y + 12, AL_V | AL_PASS, "library tile number");
        cv_text_on(x + 4, y + 3, &AF_X, b, cur ? T_THEME : T_DIM, bg);
        if (cur)
            str_cpy(nm, graph_project_used(0) ? graph_project_name(0) : "", sizeof nm);
        else
            str_cpy(nm, lib.name[i], sizeof nm);
        if (!nm[0] && (cur ? graph_project_used(0) : (lib.mask[i] & 1u)))
            str_cpy(nm, "USED", sizeof nm);
        cv_clip_text(x + 4, y + 14, &AF_X, nm[0] ? nm : "--", nm[0] ? T_TEXT : T_DIM, bg, 50);
        for (j = 0; j < 4u; j++) {
            int32_t sx = x + 4 + (int32_t)(j % 2u) * 25, sy = y + 28 + (int32_t)(j / 2u) * 18;
            int used = cur ? graph_project_used(j) : (int)((lib.mask[i] >> j) & 1u);
            uint16_t c = lz_sec(j, used, 0);
            char l[2] = {(char)('A' + j), 0};
            cv_rect(sx, sy, 23, 15, c);
            GFX_HOOK_ALIGN(sx, sy, sx + 23, sy + 15, AL_HV, "library section chip letter centred");
            cv_text_in(sx, sy + CAP_IN(X, 15), 23, &AF_X, l, used ? T_INK : T_DIM, c);
            if (cur && (int32_t)j + 1 == song.g[G_SLOT])
                cv_frame(sx - 1, sy - 1, 25, 17, T_TEXT);
        }
    }
}
static void graph_lz_tools(void)
{
    static const char *const WHAT[6] = {"CLEAR SEQ", "INIT SOUND", "DELETE ROW", "CLEAR SONG", "NEW PROJECT", "CPU TEST"};
    static const char *const ON[6] = {"STEPS + AUTO", "ENGINE DEFAULT", "", "ALL ROWS", "EVERYTHING", "HEAVY SONG"};
    uint32_t c;
    char b[12];
    for (c = 0; c < 6u; c++) {
        int32_t y = 1 + (int32_t)c * 23;
        int sel = ui.act == c + 1u;
        uint16_t bg = sel ? T_TINT : T_RAISE;
        cv_rect(4, y, 232, 21, bg);
        GFX_HOOK_ALIGN(0, y, 0, y + 21, AL_V, "tools row text centred up/down");
        cv_text_on(12, y + CAP_IN(M, 21), &AF_M, WHAT[c], sel ? T_TEXT : T_MID, bg);
        if (c == 2u) {                                   /* (the row ARRANGE has picked) */
            str_cpy(b, "ROW ", sizeof b);
            fmt_int(b + 4, (int32_t)ui.song_row + 1);
        }
        GFX_HOOK_ALIGN(0, y, 0, y + 21, AL_V, "tools row text centred up/down");
        cv_text_r(230, y + CAP_IN(X, 21), &AF_X, c == 2u ? b : ON[c], sel ? T_MID : T_DIM, bg);
    }
}
/* (ACID FM 3c-2 task 3) PROJECT's panel (graph_proj: SECTIONS' sections, ARRANGE's chips and strip) gone: SAVE > SONG is
 * ux_save.c's */
/* (ACID FM 3c-2 task 2) SOUND's panels (lz_track_card, graph_sound: USER's 32 slots; graph_lz_presets: PRESETS' list) gone: SAVE >
 * SOUND is ux_save.c's */
/* the lens's panel for the SAVE page shown */
static void graph_save_lz(const page_t *pg)
{
    if (pg->graph == GR_TOOLS) graph_lz_tools();
    else if (pg->graph == GR_SLOTS) graph_lib();
}
