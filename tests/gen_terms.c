/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker
 * (0.9.1) GEN's calibration (docs/braindancer/gen-calibration-0.9.1.md): each device row's piece rebuilt from the
 * identity the console printed (gen_id), in the session's order from a NEW PROJECT (as the device made them: a piece
 * reads which track is the DRUM track), and the CPU model's terms of its heaviest section (gen.c gc_terms), for
 * tools/gen_cal.py rows. MATCH 1: the rebuilt name is the device's and one section's cost is a cost the device printed.
 *   tools/host-cc -O1 -w -Ibuild/gen -Ifirmware/src -Itests -o build/host/gen_terms tests/gen_terms.c -lm
 *   python3 tools/gen_cal.py ids docs/braindancer/gen-calibration-0.9.1.csv | build/host/gen_terms > build/cal/terms-091.txt
 * in:  TAG|seed two dev form tape main part energy complex|cost_s|cost_e|NAME
 * out: TAG|MATCH|FIXED|TAPE|CCOMP|e:v;e:v..|COST|NAME */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

int main(void)
{
    char line[256], tag[64], nm[64], cs[16], ce[16];
    unsigned f[9];
    ui_power_on();
    ui_rec_prefs |= PREF_SIX >> 8; rp_apply();          /* TRACKS 6 (Task 8 Step 3) */
    settings.palette = UI_ACID_INDEX; palette_set(UI_ACID_INDEX);
    system_init();                                      /* (NEW PROJECT: the power-on parts, Task 8 Step 3) */
    while (fgets(line, sizeof line, stdin)) {
        genid_t id;
        gc_terms_t m, best;
        uint32_t s, cost, bc = 0, hit = 0, k;
        if (sscanf(line, "%63[^|]|%u %u %u %u %u %u %u %u %u|%15[^|]|%15[^|]|%63[^\n]", tag, &f[0], &f[1], &f[2], &f[3],
                   &f[4], &f[5], &f[6], &f[7], &f[8], cs, ce, nm) != 13)
            continue;
        memset(&id, 0, sizeof id);
        id.seed = f[0]; id.two = f[1]; id.dev = f[2]; id.form = f[3]; id.tape = f[4];
        id.main = (uint8_t)f[5]; id.part = (uint8_t)f[6]; id.energy = (uint8_t)f[7]; id.complex = (uint8_t)f[8];
        mc_host_v12 = id.two;                           /* (two cores: the host's 12 voices, as tests/gen_test.c) */
        gen_build(&id);
        memset(&best, 0, sizeof best);
        for (s = 0; s < 4u; s++) {
            if (s && (!((gs.wrote >> s) & 1u) || section_load(s))) continue;
            gs.in_sec = 1;                              /* (the device's figure: the music now, its muted tracks free) */
            gc_terms(1, &m);
            gs.in_sec = 0;
            cost = gc_sum(&m);
            hit |= cost == (uint32_t)atoi(cs) || cost == (uint32_t)atoi(ce);
            if (cost >= bc) { bc = cost; best = m; }
        }
        if (gs.wrote & 1u) section_load(0);             /* (A again: the music the next press starts from) */
        printf("%s|%u|%u|%u|%u|", tag, (unsigned)(hit && str_eq(gp.last, nm)), best.fixed, best.tape, best.ccomp);
        for (k = 0; k < best.n; k++) printf("%s%u:%u", k ? ";" : "", best.eng[k], best.v[k]);
        printf("|%u|%s\n", bc, gp.last);
    }
    return 0;
}
