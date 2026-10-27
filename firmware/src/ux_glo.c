/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's GLO pages on the screen (3c-1 spec 3; ux_glo.h), drawn through ux_section.c's page (header, knob row, panel, status
 * line, footer) as the SEQ pages are:
 *   LIVE     (spec 3.1; the inherited first LIVE page, MANO) what it does is ui_live.c's, as before: the white keys the tricks
 *            (LOOP 8 / 4 / 2 / 1, /2, x2, 3/2, 2/3, REVERSE, PING-PONG, RATCHET, THIN, OCT+-, RETURN), the black keys the tracks
 *            they act on, G#4 LATCH, C#5 / D#5 TURN, F#5 RANDOM; OCT- tapped RETURN, held BYPASS, OCT+ KEEP.
 *            Knobs RATE . LOOP . THIN . DJ FILTER (the first track picked's values; the DJ FILTER the song's).
 *            The picture: one bar as a timeline (its four beats), a row a track: the track's number on the left, lit when
 *            picked (the black keys held; none: all), its window as an ACID block over the bar (a LOOP where it was taken,
 *            a CUT from step 1), a track with any other trick a VIOLET band, and under it its tricks in words (the speed,
 *            REVERSE / PING-PONG arrows, RANDOM, THIN, RATCHET, OCT, TURN); while a trick runs and the song plays, a line
 *            where the bar is (its 1/16). LATCH on: said at the top right.
 *   MORE     (spec 3.3) its LIVE rows: the RAILS (the inherited second LIVE page's knobs, ui_live.c live_rail): TRACKS, RATE,
 *            CUT, DENSITY (uxg_vtext); OCT- / OCT+ there as on LIVE (ui_live.c live_page, uxg_more_live).
 * The picture is redrawn when what it shows changes (uxg_sig), the bar's line as it moves at most every UX_TRK_MS (UX_TRK_SLOW_MS
 * under load): a panel a 1/16 at most while tricks run, none while nothing changes. Included after ux_seq.c, before
 * ux_section.c. */

#define UXG_X0 24                /* the timeline: the bar's first 1/16 (x), a 1/16's width (16 x 13 = 208 px) */
#define UXG_CW 13
#define UXG_W (16 * UXG_CW)
#define UXG_RY 18                /* the first track's row (y), the rows' pitch, a row's band (its words under it) */
#define UXG_RP 21
#define UXG_BH 8
#define UXG_BX 3                 /* a track's number: its box */
#define UXG_BW 15
#define UXG_BHT 17

static const char *const UXG_RATE[LV_NRATE] = {"/4", "/3", "/2", "2/3", "X1", "3/2", "X2", "X3", "X4"};   /* (seq_live.c LV_RN /
                                                         * LV_RD: rate + LV_X1) */
static const char *const UXG_TGT[9] = {"ALL", "T1", "T2", "T3", "T4", "T5", "T6", "SYNTHS", "DRUMS"};   /* (RAILS' TRACKS: lv.tgt) */
static const char *const UXG_LABEL[7] = {"RATE", "LOOP", "THIN", "DJ FILTER", "TRACKS", "CUT", "DENSITY"};
enum { UXG_L_RATE, UXG_L_LOOP, UXG_L_THIN, UXG_L_DJF, UXG_L_TGT, UXG_L_CUT, UXG_L_DENS };
/* the words the knob row, the status line, the rows and the picture are made of (tests/ux_msg_scan.py: every one has its glyphs
 * in UF_LBL and UF_HDR) */
static const char *const UXG_WORD[] = {"OFF", " STEPS", "%", " PLAY", "ALL", "RATCHET X", "THIN ", " \xB7 ", "LATCH", "RANDOM",
                                       "OCT+", "OCT-", "TURN ", "+", "K", " LP", " HP", "LOW-PASS ", "HIGH-PASS ", "HZ", " ", " STEP"};
enum { UXG_W_OFF, UXG_W_STEPS, UXG_W_PCT, UXG_W_PLAY, UXG_W_ALL, UXG_W_RATCH, UXG_W_THIN, UXG_W_DOT, UXG_W_LATCH, UXG_W_RND,
       UXG_W_UP, UXG_W_DN, UXG_W_TURN, UXG_W_PLUS, UXG_W_K, UXG_W_LP, UXG_W_HP, UXG_W_LOW, UXG_W_HIGH, UXG_W_HZ, UXG_W_SP,
       UXG_W_STEP };
static const char *const UXG_ARROW_LBL[2] = {"<<", "<>"};   /* (the picture's REVERSE, PING-PONG: UF_LBL alone has < and >) */
/* the status line on LIVE with no knob turned (what the keys do; LATCH on), on a RAILS row (GLO > MORE: OCT- and OCT+ there) */
static const char *const UXG_SAY[3] = {"WHITE: TRICKS \xB7 BLACK: TRACKS", "LATCH ON: A KEY TURNS IT ON / OFF",
                                       "RAILS: OCT- RETURN \xB7 OCT+ KEEP"};
static int32_t uxg_ph_drawn = -1;                       /* the bar's 1/16 drawn (-1: none), when (ms): ux_section_draw's pace */
static uint32_t uxg_ph_ms;

static int uxg_owns(void) { return !ui.home && ui.page < NPAGES && PAGES[ui.page].graph == GR_LIVE; }
static int uxg_more_live(void)
{
    uint32_t s, t;
    if (ui.home || ui.menu || ui.page >= NPAGES || PAGES[ui.page].more != &UXM_GLO)
        return 0;
    s = uxm_at[uxm_key()].sel;
    t = uxm_at[uxm_key()].top;
    uxm_fix(&UXM_GLO, uxm_mask(&UXM_GLO), &s, &t);
    return uxm_head_of(&UXM_GLO, s) == UXM_H_LIVE;
}
static void uxg_cat(char *b, uint32_t n, const char *s) { str_cpy(b + str_len(b), s, n - str_len(b)); }
static void uxg_num(char *b, uint32_t n, int32_t v) { char d[8]; fmt_int(d, v); uxg_cat(b, n, d); }
static uint32_t uxg_first(uint32_t tg) { return tg ? (uint32_t)__builtin_ctz(tg) : 0u; }   /* (the values shown: ui_live.c lv_set's) */
/* setting w (UXG_L_*) of track f in words into b (full: the status line's, "8 STEPS", "70% PLAY"); *bar its place, 1 in use */
static __attribute__((noinline)) int uxg_val(uint32_t w, uint32_t f, char *b, uint32_t n, int full, uint8_t *bar)
{
    int32_t r = lv.rate[f % NTRK], dn = lv.dn[f % NTRK];
    uint32_t ci = lv.cut[f % NTRK] % LV_NCUT;
    b[0] = 0;
    if (w == UXG_L_RATE) {
        uxg_cat(b, n, UXG_RATE[r + LV_X1]);
        *bar = (uint8_t)((r + LV_X1) * 255 / (LV_NRATE - 1));
        return r != 0;
    }
    if (w == UXG_L_LOOP || w == UXG_L_CUT) {
        if (ci)
            uxg_num(b, n, LV_CUT[ci]);
        if (!ci)
            uxg_cat(b, n, UXG_WORD[UXG_W_OFF]);
        else if (full || w == UXG_L_CUT)                /* ("1 STEP", "8 STEPS") */
            uxg_cat(b, n, UXG_WORD[LV_CUT[ci] == 1u ? UXG_W_STEP : UXG_W_STEPS]);
        *bar = (uint8_t)(ci * 255u / (LV_NCUT - 1u));
        return ci != 0u;
    }
    if (w == UXG_L_TGT) {
        uxg_cat(b, n, UXG_TGT[lv.tgt % 9u]);
        *bar = (uint8_t)(lv.tgt % 9u * 255u / 8u);
        return 1;
    }
    *bar = (uint8_t)(w == UXG_L_THIN ? (dn < 0 ? -dn * 255 / 9 : 0) : (dn + 9) * 255 / 12);
    if (w == UXG_L_DENS && dn > 0) {                    /* (DENSITY right: RATCH x2 .. x4) */
        uxg_cat(b, n, UXG_WORD[UXG_W_RATCH]);
        uxg_num(b, n, 1 + dn);
        return 1;
    }
    if (w == UXG_L_DENS && !dn) {
        uxg_cat(b, n, UXG_WORD[UXG_W_ALL]);
        return 0;
    }
    if (w == UXG_L_DENS)
        uxg_cat(b, n, UXG_WORD[UXG_W_THIN]);
    uxg_num(b, n, dn < 0 ? 100 + 10 * dn : 100);        /* (THIN: the share that plays, 100% none taken) */
    uxg_cat(b, n, UXG_WORD[UXG_W_PCT]);
    if (full && w == UXG_L_THIN)
        uxg_cat(b, n, UXG_WORD[UXG_W_PLAY]);
    return dn < 0;
}
/* the DJ FILTER (song.g[G_DJF]) in words: "1.2K LP", "800 HP", "OFF"; full: "LOW-PASS 1.2KHZ" (ui_draw.c djf_format's cutoff) */
static void uxg_djf(char *b, uint32_t n, int full)
{
    int32_t v = song.g[G_DJF];
    const char *u;
    char d[12];
    uint32_t i;
    b[0] = 0;
    djf_format(v, d, &u);
    for (i = 0; d[i]; i++)
        if (d[i] == 'k')
            d[i] = UXG_WORD[UXG_W_K][0];
    if (!v) {
        uxg_cat(b, n, UXG_WORD[UXG_W_OFF]);
        return;
    }
    if (full)
        uxg_cat(b, n, UXG_WORD[v < 0 ? UXG_W_LOW : UXG_W_HIGH]);
    uxg_cat(b, n, d);
    uxg_cat(b, n, full ? UXG_WORD[UXG_W_HZ] : UXG_WORD[v < 0 ? UXG_W_LP : UXG_W_HP]);
}
/* LIVE's knob row: RATE LOOP THIN of the first track picked (dim: not in use), the DJ FILTER */
static void uxg_knobs(ux_knob_t k[4])
{
    uint32_t f = uxg_first(lv_key_tracks()), c;
    char b[24];
    for (c = 0; c < 3u; c++) {
        k[c].label = UXG_LABEL[c];
        k[c].off = (uint8_t)!uxg_val(c, f, b, sizeof b, 0, &k[c].bar);
        str_cpy(k[c].value, b, sizeof k[c].value);
    }
    k[3].label = UXG_LABEL[UXG_L_DJF];
    uxg_djf(k[3].value, sizeof k[3].value, 0);
    k[3].bar = (uint8_t)((song.g[G_DJF] + 64) * 255 / 127);
}
/* the status line for LIVE's knob c just turned ("RATE . X2", "LOOP . 4 STEPS", "THIN . 70% PLAY", "DJ FILTER . LOW-PASS 800HZ") */
static const char *uxg_say(uint32_t c, char *b, uint32_t n)
{
    uint8_t bar;
    str_cpy(b, UXG_LABEL[c & 3u], n);
    uxg_cat(b, n, UXG_WORD[UXG_W_DOT]);
    if ((c & 3u) == 3u)
        uxg_djf(b + str_len(b), n - str_len(b), 1);
    else
        (void)uxg_val(c & 3u, uxg_first(lv_key_tracks()), b + str_len(b), n - str_len(b), 1, &bar);
    return b;
}
static const char *uxg_idle(void) { return UXG_SAY[lv.latch ? 1 : 0]; }   /* (LIVE, no knob turned) */
static const char *uxg_vtext(uint32_t id, char *val, uint32_t nv, uint8_t *bar)
{
    uint32_t w = id == ID_LV_TGT ? UXG_L_TGT : id == ID_LV_RATE ? UXG_L_RATE : id == ID_LV_CUT ? UXG_L_CUT : UXG_L_DENS;
    (void)uxg_val(w, uxg_first(lv_rail_tracks()), val, nv, 1, bar);
    return UXG_LABEL[w];
}

/* ------------------------------------------------------------------------------------------------------- the picture --- */
/* the 1/16 of the bar the song is in while a trick runs (-1: stopped, or nothing live) */
static int32_t uxg_ph(void)
{
    uint32_t bs = beat_samples();
    if (!song.playing || !lv.busy || !bs)
        return -1;
    return (int32_t)(((beat_n & 3u) * 4u + (beat_pos < bs ? beat_pos * 4u / bs : 3u)) & 15u);
}
static const lv_mod_t *uxg_mod(uint32_t i) { return song.playing && LV_ON(i) ? &lv.cur[i] : &lv.want[i]; }   /* (what sounds) */
/* track m's tricks but its window in words, as many as fit the bar ("X2 << THIN 70% RATCHET X3 OCT+ TURN +1") */
static void uxg_tags(const lv_mod_t *m, char *b, uint32_t n)
{
    const char *t[7];
    char tn[4][12];
    uint32_t k = 0, i, d = LF_DIR(m->fl);
    if (m->rate)
        t[k++] = UXG_RATE[m->rate + LV_X1];
    if (d)
        t[k++] = d == LD_RND ? UXG_WORD[UXG_W_RND] : UXG_ARROW_LBL[d == LD_PING];
    tn[0][0] = tn[1][0] = tn[2][0] = 0;
    if (m->drop) {
        uxg_cat(tn[0], 12, UXG_WORD[UXG_W_THIN]);
        uxg_num(tn[0], 12, 100 - m->drop);
        uxg_cat(tn[0], 12, UXG_WORD[UXG_W_PCT]);
        t[k++] = tn[0];
    }
    if (m->rat > 1u) {
        uxg_cat(tn[1], 12, UXG_WORD[UXG_W_RATCH]);
        uxg_num(tn[1], 12, m->rat);
        t[k++] = tn[1];
    }
    if (m->fl & (LF_UP | LF_DN))
        t[k++] = UXG_WORD[m->fl & LF_UP ? UXG_W_UP : UXG_W_DN];
    if (m->rot) {
        uxg_cat(tn[2], 12, UXG_WORD[UXG_W_TURN]);
        if (m->rot > 0)
            uxg_cat(tn[2], 12, UXG_WORD[UXG_W_PLUS]);
        uxg_num(tn[2], 12, m->rot);
        t[k++] = tn[2];
    }
    b[0] = 0;
    for (i = 0; i < k; i++) {
        uint32_t at = str_len(b);
        if (at)
            uxg_cat(b, n, UXG_WORD[UXG_W_SP]);
        uxg_cat(b, n, t[i]);
        if (text_w(&UF_LBL, b) > UXG_W) {               /* (what does not fit left out) */
            b[at] = 0;
            return;
        }
    }
}
/* track i's row at y: its number (lit picked), the bar's band, its window, its tricks in words under it */
static void uxg_row(uint32_t i, uint32_t tg, int32_t y)
{
    const lv_mod_t *m = uxg_mod(i);
    uint32_t len = (uint32_t)clamp(trk[i].p[P_SLEN], 1, NSTEP), cn = m->cut && m->cut < len ? m->cut : 0u;
    uint32_t ws = cn && (m->fl & LF_HERE) ? m->ws : 0u, a, w;
    int on = (int)((tg >> i) & 1u);
    uint16_t bx = on ? UX_ACID : UX_BG2;
    char d[2] = {(char)('1' + i), 0}, b[48];
    cv_rect(UXG_BX, y, UXG_BW, UXG_BHT, bx);
    cv_text_on(UXG_BX + (UXG_BW - text_w(&UF_LBL, d)) / 2, y + (UXG_BHT - UF_LBL_CAP_H) / 2 - UF_LBL_CAP_Y, &UF_LBL, d,
               on ? UX_BG : UX_MID, bx);
    cv_rect(UXG_X0, y, UXG_W, UXG_BH, UX_BG2);
    if (lv_neutral(m))
        return;
    if (cn) {                                           /* (the window: a LOOP where it was taken, a CUT from step 1) */
        if (ws + cn > len)
            ws = len - cn;
        a = ws % 16u;
        w = cn < 16u - a ? cn : 16u - a;
        cv_rect(UXG_X0 + (int32_t)a * UXG_CW, y, (int32_t)w * UXG_CW - 1, UXG_BH, UX_ACID);
    } else {
        cv_rect(UXG_X0, y, UXG_W, UXG_BH, ux_mix(UX_BG2, UX_VIOLET, 45));
    }
    uxg_tags(m, b, sizeof b);
    if (b[0])
        cv_text_on(UXG_X0, y + UXG_BH + 1, &UF_LBL, b, UX_TEXT, ux_under(UXG_X0, y + UXG_BH + 5));
}
static __attribute__((noinline)) void uxg_picture(void)
{
    uint32_t tg = lv_key_tracks(), i;
    int32_t ph = uxg_ph(), x;
    for (i = 0; i < 4u; i++) {                          /* the bar's beats */
        char d[2] = {(char)('1' + i), 0};
        x = UXG_X0 + (int32_t)i * 4 * UXG_CW + 2;
        cv_text_on(x, 4, &UF_LBL, d, UX_MID, ux_under(x, 8));
        if (i)
            cv_rect(x - 3, UXG_RY - 3, 1, (int32_t)NTRK * UXG_RP, UX_LINE);
    }
    if (lv.latch) {
        x = UXG_X0 + UXG_W - text_w(&UF_LBL, UXG_WORD[UXG_W_LATCH]);
        cv_text_on(x, 4, &UF_LBL, UXG_WORD[UXG_W_LATCH], UX_ACCENT, ux_under(x, 8));
    }
    for (i = 0; i < NTRK; i++)
        uxg_row(i, tg, UXG_RY + (int32_t)i * UXG_RP);
    if (ph >= 0)                                        /* (where the bar is) */
        cv_rect(UXG_X0 + ph * UXG_CW + UXG_CW / 2, UXG_RY - 3, 1, (int32_t)NTRK * UXG_RP, UX_TEXT);
}
/* what the picture is drawn from but the bar's line (that one paced: ux_section_draw): each track's tricks as they sound, its
 * LEN, the tracks picked, LATCH */
static __attribute__((noinline)) uint32_t uxg_sig(void)
{
    uint32_t h = lv_key_tracks() * 131u + lv.latch * 7u + 1u, i, k;
    for (i = 0; i < NTRK; i++) {
        const uint8_t *p = (const uint8_t *)uxg_mod(i);
        for (k = 0; k < sizeof(lv_mod_t); k++)
            h = (h ^ p[k]) * 16777619u;
        h = h * 31u + (uint32_t)trk[i].p[P_SLEN];
    }
    return h;
}
