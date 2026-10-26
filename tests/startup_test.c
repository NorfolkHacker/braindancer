/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* Native emulator boot: includes main.c's real felucca_init, not just the storage loader. */
/* (JIANT 0.6.84) by @workeffortwaste (pull request #134, the ARRANGE power-off bug): it fails without main.c's
 * 0.6.81 fix (felucca_init clearing the arrangement persist_boot had read) and passes with it. */
#include "../web/emu/felucca_web.c"

static int startup_case(int saved, int restore, int noprefs, int picked4)
{
    song_index_t want, got;
    persist_t prefs = {0};
    uint32_t i;
    int ok;
    web_nor_erase();
    memset(&want, 0, sizeof want);
    want.magic = SONG_MAGIC;
    want.version = 2;
    want.cur = saved ? 2u : 0u;
    for (i = 0; i < NSONG; i++) chain_defaults(&want.rows[i]);
    if (saved) {
        want.rows[2].count = 2;
        want.rows[2].row[0] = (chain_row_t){0, 3};
        want.rows[2].row[1] = (chain_row_t){1, 7};
        SONG_END(&want.rows[2]) = 3;
        want.scene[2][0] = (scene_t){1, 2, 4, {11, 22, 33, 44}};
        want.rows[0].count = 1;
        want.rows[0].row[0] = (chain_row_t){3, 9};
        if (st_save(OBJ_SONGIDX, &want, sizeof want)) return 1;
    }
    prefs.magic = PERSIST_MAGIC;
    prefs.panel = PANEL_DEFAULT;
    prefs.favorites.factory[15][30] = restore ? PREF_RESTORE_ON : 0;
    prefs.favorites.factory[15][25] = picked4 ? 3u : 2u;   /* prefs_rev: 2 as JIANT and ACID FM 0.3 saved it (TRACKS bit clear:
                                                        * JIANT's default 4, not a choice); 3 = saved by 0.4 after its one-time
                                                        * TRACKS 6, here with the player's own pick, 4 */
    if (!noprefs && st_save(OBJ_SETTINGS, &prefs, sizeof prefs)) return 1;

    web_boot();
    if (noprefs) {                                     /* no settings saved: new settings, ACID FM 0.3's one palette (ACID) and
                                                        * one look (AIR's: no boxes, SURF is BG), SCOPE OFF; the stored SKIN
                                                        * byte AIR (3), as JIANT's new settings (an older firmware reads it) */
        int okf = ui_skin == SKIN_AIR, i2;
        for (i2 = 0; i2 < 2000; i2++) web_render(128);   /* (past the splash) */
        okf &= settings.palette == UI_ACID_INDEX && T_BG == UI_PALETTES[UI_ACID_INDEX].bg && T_SURF == T_BG &&
               ui_skin == SKIN_AIR && ui_scope == 0u;
        {   /* (ACID FM 3b) the demo's T5 / T6 play: new settings are TRACKS 6 */
            int okd = !seq_is_empty(&trk[0]) && !seq_is_empty(&trk[4]) && !seq_is_empty(&trk[5]) && song.g[G_BPM] == 124 &&
                      trk_live == NTRK && (ui_rec_prefs & (PREF_SIX >> 8));
            printf("startup: no settings, no autosave: the demo, TRACKS 6 %s\n", okd ? "ok" : "FAIL");
            okf &= okd;
        }
        printf("startup: empty flash, no settings: ACID, the one skin, SCOPE OFF (new settings) %s\n", okf ? "ok" : "FAIL");
        return !okf;
    }
    if (ui_skin || ui_scope || settings.palette != UI_ACID_INDEX || T_SURF != T_BG) {   /* settings saved by JIANT (their SKIN
                                                        * byte 0, COLORS, kept as saved, inert): ACID, the one skin, SCOPE OFF */
        printf("startup: saved settings: ACID, the one skin (their SKIN byte kept), SCOPE OFF: FAIL\n");
        return 1;
    }
    ok = song_cur == want.cur && !memcmp(&chain_config, &want.rows[want.cur], sizeof chain_config);
    ok &= !!(ui_prefs & PREF_RESTORE_ON) == restore;
    ok &= !memcmp(song_idx.scene, want.scene, sizeof want.scene);
    /* Advance beyond the splash and the arrangement's two-second automatic save. */
    for (i = 0; i < 2000u; i++) web_render(128);
    ok &= !memcmp(&chain_config, &want.rows[want.cur], sizeof chain_config);
    if (saved)
        ok &= st_load(OBJ_SONGIDX, &got, sizeof got) == (int)sizeof got && !memcmp(&got, &want, sizeof got);
    else
        ok &= st_load(OBJ_SONGIDX, &got, sizeof got) < 0;
    {   /* (ACID FM 3b) no autosave in any case: the power-on demo (T1 BEAT, T5 ACID line, T6 pad, 124 BPM, six tracks) */
        int okd = !seq_is_empty(&trk[0]) && !seq_is_empty(&trk[4]) && !seq_is_empty(&trk[5]) && seq_is_empty(&trk[1]) &&
                  song.g[G_BPM] == 124 && demo_on && (picked4 ? trk_live == 4u : trk_live == NTRK);   /* (ACID FM 0.4: older
                                                        * settings TRACKS 6 once; a 4 picked after that is kept) */
        printf("startup: no autosave: the demo is loaded (%s) %s\n", picked4 ? "TRACKS 4 picked on 0.4: kept" :
               "older settings: TRACKS 6 once", okd ? "ok" : "FAIL");
        ok &= okd;
    }
    {   /* the one-time switch is saved (prefs_rev 3), so it never runs again; a 4 picked after it is saved as 4 */
        persist_t s;
        int okm = st_load(OBJ_SETTINGS, &s, sizeof s) == (int)sizeof s && s.favorites.factory[15][25] >= 3u &&
                  !!(s.favorites.factory[15][28] & (PREF_SIX >> 8)) == !picked4;
        if (!picked4) {
            trks_put(0);                               /* MENU > TRACKS 4 */
            settings_save();
            for (i = 0; i < 400u; i++) web_render(128);
            okm &= trk_live == 4u && st_load(OBJ_SETTINGS, &s, sizeof s) == (int)sizeof s &&
                   s.favorites.factory[15][25] >= 3u && !(s.favorites.factory[15][28] & (PREF_SIX >> 8));
        }
        printf("startup: TRACKS migration saved once, the player's 4 saved as 4 %s\n", okm ? "ok" : "FAIL");
        ok &= okm;
    }
    printf("startup: %s, RESTORE LAST %s: rows, bars, END, scenes and flash %s\n",
           saved ? "saved project 3" : "empty flash", restore ? "ON" : "OFF", ok ? "ok" : "FAIL");
    return !ok;
}

int main(void)
{
    int mode, status, bad = 0;
    /* Each child starts with fresh static RAM, as a new device instance does. */
    for (mode = 0; mode < 5; mode++) {                 /* (0.9.7: 3, no settings saved at all; ACID FM 0.4: 4, settings
                                                        * saved by 0.4 with TRACKS 4 picked: a reboot keeps 4) */
        pid_t child = fork();
        if (child < 0) return 2;
        if (!child) exit(startup_case(mode == 1 || mode == 2, mode == 2, mode == 3, mode == 4));
        if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status)) bad++;
    }
    puts(bad ? "startup test FAILED" : "startup test passed");
    return !!bad;
}
