/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's HOME on the screen (spec 4.3), every piece one of ux_widgets.c's: the header ACID FM, the knob row, the panel
 * over the glow, the status line (spec 0: the screen says what the controls do; a knob turn shows its value at once; a
 * refusal says why), the footer HOME . page and n / 3.
 * TRACKS (3a task 6): six ridgelines, T1 at the top .. T6 at the bottom, each its track's level over the last ~2 s
 * scrolling right to left, the nearer (lower) lines hiding the ones behind, pulsar-plot style; the four tracks the knobs
 * control in their knobs' colours, the other two dimmed; the name at the left, the level 0..99 at the right; under them the
 * selected track's 16 steps, the playing one outlined and spelled out. KNOB 1..4: the levels of the four.
 * VISUAL (task 7): the rings of the blend turning with the bar, the song's name over them; KNOB 1..4 MACRO 1..4.
 * Included after ux_widgets.c; ux_home_draw is called by ui_draw.c ui_draw_body,
 * ux_levels_tick by ui_draw every frame. */

/* ---- the level history: the one reader of trk[c].peak (fx.c holds each track's peak until it is read; read here once a
 * frame on every page, smoothed with a fall-off, a point every UXR_MS into a ring of UXR_N per track: 672 B of .bss) */
#define UXR_HOLD 5u
static struct {
    uint8_t h[NTRK][UXR_N];      /* the points, 0..255; slot at is the oldest */
    uint8_t acc[NTRK];           /* the peak since the last point (0..255) */
    uint8_t lv[NTRK];            /* the level now, smoothed */
    int32_t pk[NTRK];            /* the peak as fx.c measured it since a reader last took it (ux_trk_peak: MIXER's meters,
                                  * EDIT's creatures), held at most UXR_HOLD frames unread (a page paced under load) */
    uint8_t pk_n, pk_taken;
    uint8_t at;
    uint16_t nz;                 /* points not 0: the lines move while there are any */
    uint32_t ms;                 /* the last point's time */
} uxr;

static void ux_levels_reset(void)
{
    memset(&uxr, 0, sizeof uxr);
    uxr.ms = fm1_ms;
}
static void ux_levels_tick(void)
{
    uint32_t c, n = (fm1_ms - uxr.ms) / UXR_MS;
    if (uxr.pk_taken || ++uxr.pk_n > UXR_HOLD) {
        memset(uxr.pk, 0, sizeof uxr.pk);
        uxr.pk_taken = uxr.pk_n = 0;
    }
    for (c = 0; c < NTRK; c++) {
        int32_t v = trk[c].peak;
        trk[c].peak = 0;
        if (v > uxr.pk[c])
            uxr.pk[c] = v;
        v = v <= 0 ? 0 : v >= 7000 ? 255 : v * 255 / 7000;  /* (ui_lzhome.c's scale: a track's peak near 7000 is loud) */
        if (v > uxr.acc[c])
            uxr.acc[c] = (uint8_t)v;
    }
    if (!n)
        return;
    if (n > UXR_N) {                                    /* (a long gap: the whole ring at once) */
        n = UXR_N;
        uxr.ms = fm1_ms;
    } else
        uxr.ms += n * UXR_MS;
    while (n--) {
        for (c = 0; c < NTRK; c++) {
            uint8_t *d = &uxr.h[c][uxr.at];
            int32_t l = uxr.lv[c], a = uxr.acc[c];
            l = a >= l ? a : l - (l - a + 3) / 4;       /* a hit at once, a quick fall */
            uxr.lv[c] = (uint8_t)l;
            uxr.acc[c] = 0;
            uxr.nz = (uint16_t)(uxr.nz - (*d != 0) + (l != 0));
            *d = (uint8_t)l;
        }
        uxr.at = (uint8_t)(uxr.at + 1u == UXR_N ? 0u : uxr.at + 1u);
    }
}
static int32_t ux_trk_peak(uint32_t c)
{
    uxr.pk_taken = 1;                                   /* (every reader this frame sees the same; the next frame anew) */
    return c < NTRK ? uxr.pk[c] : 0;
}
static uint32_t ux_hist(uint32_t c, uint32_t age)       /* the point age points old (0: the newest) */
{
    return uxr.h[c % NTRK][(uxr.at + UXR_N - 1u - age % UXR_N) % UXR_N];
}

/* a knob's label in UF_LBL's letters (capitals, digits, a few signs): others become a space */
static void ux_lbl_cpy(char *d, const char *s, uint32_t n)
{
    uint32_t i;
    for (i = 0; s && s[i] && i + 1u < n; i++) {
        char c = s[i];
        if (c >= 'a' && c <= 'z')
            c = (char)(c - 'a' + 'A');
        d[i] = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '%' || c == '+' || c == '-' || c == '.' ||
               c == '/' || c == '#' || c == ':' ? c : ' ';
    }
    d[i] = 0;
}

/* ---- TRACKS */
static uint32_t ux_trk_first(void) { return trk_base4(); }                     /* the knobs' four: from this track */
/* track c's LEVEL as the mixer shows it (params.c param_format, F_DB: "-2.1" and "dB", "OFF"): one scale everywhere */
static const char *ux_lvl_fmt(uint32_t c, char val[16])
{
    const char *unit = "";
    param_format(&TP[P_LEVEL], trk[c].p[P_LEVEL], val, &unit);
    return unit ? unit : "";
}
/* a name fitted to w px (UF_LBL): first the vowels after each word's first letter go ("T4 BYTEBEAT" -> "T4 BYTBT", still
 * readable), then letters off its end */
static void ux_fit_name(char *b, int32_t w)
{
    uint32_t i, o;
    if (text_w(&UF_LBL, b) <= w)
        return;
    for (i = o = 0; b[i]; i++) {
        char ch = b[i];
        if (!(i > 0u && b[i - 1u] != ' ' && (ch == 'A' || ch == 'E' || ch == 'I' || ch == 'O' || ch == 'U')))
            b[o++] = ch;
    }
    b[o] = 0;
    while (text_w(&UF_LBL, b) > w && str_len(b) > 3u)
        b[str_len(b) - 1u] = 0;
}
static void ux_trk_name(uint32_t c, char *b, uint32_t n, int full)   /* "T3 ACID": the sound's first word (full: all of it) */
{
    char s[20];
    uint32_t i;
    sound_name(&trk[c], s);
    if (!s[0])
        str_cpy(s, trk_engp(&trk[c])->name, sizeof s);
    b[0] = 'T'; b[1] = (char)('1' + c); b[2] = ' ';
    ux_lbl_cpy(b + 3, s, n - 3u);
    for (i = 3; !full && b[i]; i++)
        if (b[i] == ' ') {
            b[i] = 0;
            break;
        }
}
static int32_t ux_trk_hot(void)                         /* the track whose LEVEL was just turned, or -1 */
{
    int32_t k = ux_knob_hot();
    return k < 0 || ux_home_page() != UXH_TRACKS ? -1 : (int32_t)ux_trk_first() + k;
}
/* the strip's page: the playing step's while playing, else the one SEQ shows; its playing cell (-1: none) */
static uint32_t ux_trk_page(int32_t *ph)
{
    const track_t *t = TSEL;
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), s = seq_src(t, t->seq_idx), pg = ui.bank;
    *ph = -1;
    if (song.playing && s < len) {
        pg = s / 16u;
        *ph = (int32_t)(s % 16u);
    }
    return pg * 16u < len ? pg : 0u;
}
/* where the music is: "A · 3/8" (long: "SECTION A · BAR 3/8") while a song chain runs, else "BAR 1/2" (the pattern's) */
static void ux_where_bar(char *b, uint32_t n, int lng)
{
    char d[8];
    b[0] = 0;
    if (chain.running) {
        const chain_row_t *r = &chain.config.row[chain.row % CHAIN_ROWS];
        uint32_t k = r->bars > chain.remaining ? r->bars - chain.remaining + 1u : 1u;
        d[0] = (char)('A' + (r->slot & 3u)); d[1] = 0;
        str_cpy(b, lng ? "SECTION " : "", n);
        str_cpy(b + str_len(b), d, n - str_len(b));
        str_cpy(b + str_len(b), lng ? " \xB7 BAR " : " \xB7 ", n - str_len(b));
        fmt_int(d, (int32_t)k); str_cpy(b + str_len(b), d, n - str_len(b));
        str_cpy(b + str_len(b), "/", n - str_len(b));
        fmt_int(d, r->bars); str_cpy(b + str_len(b), d, n - str_len(b));
    } else {
        int32_t ph;
        uint32_t len = (uint32_t)clamp(TSEL->p[P_SLEN], 1, NSTEP);
        str_cpy(b, "BAR ", n);
        fmt_int(d, (int32_t)ux_trk_page(&ph) + 1); str_cpy(b + str_len(b), d, n - str_len(b));
        str_cpy(b + str_len(b), "/", n - str_len(b));
        fmt_int(d, (int32_t)((len + 15u) / 16u)); str_cpy(b + str_len(b), d, n - str_len(b));
    }
}
/* the header's right, HOME's own (fix wave I3; spec 0 rules 5, 6: what is armed and set is in sight), right to left: the
 * battery (JIANT's level and thresholds, ui_draw.c batt_shown: a gauge of four segments, no glyph; low UX_ACCENT, USB power
 * all four in UX_ACID, as JIANT's full THEME), the BPM LOCK mark, "> 124 BPM", the REC dot (UX_ACCENT; filled: the
 * selected track armed, a ring: another), then "OCT+1 . " while the octave is shifted and, on TRACKS, where the music is
 * ("A . 3/8 . ", "BAR 1/2 . "): "BAR" goes first when it does not fit beside ACID FM, then where altogether */
#define UXH_BAT_W 15                                    /* the gauge: 4 segments 3 px wide, 1 px apart */
#define UXH_LOCK_W 7
#define UXH_DOT 7
static uint32_t ux_bat_segs(uint16_t *col)              /* the gauge's lit segments (0..4) and their colour */
{
    int32_t l = batt_shown();                           /* (0 empty, 1 a third: low, 2, 3 full; 4 USB power) */
    *col = l == 4 ? UX_ACID : l == 1 ? UX_ACCENT : UX_MID;
    return l == 4 ? 4u : l ? (uint32_t)l + 1u : 0u;     /* (0: none lit, as JIANT's empty outline) */
}
static void ux_home_left(char *b, uint32_t n, uint32_t p, uint32_t drop)   /* "OCT+1 . BAR 1/2 . " (drop 1: no BAR, 2: no where) */
{
    char d[32];
    b[0] = 0;
    if (song.octave) {
        str_cpy(b, song.octave > 0 ? "OCT+" : "OCT-", n);
        fmt_int(b + str_len(b), song.octave > 0 ? song.octave : -song.octave);
        str_cpy(b + str_len(b), " \xB7 ", n - str_len(b));
    }
    if (p == UXH_TRACKS && drop < 2u) {
        ux_where_bar(d, sizeof d, 0);
        str_cpy(b + str_len(b), drop && d[0] == 'B' ? d + 4 : d, n - str_len(b));   /* ("BAR 1/2" -> "1/2") */
        str_cpy(b + str_len(b), " \xB7 ", n - str_len(b));
    }
}
static uint8_t ux_head_drop;                            /* what the header last left out to fit (ux_home_left's drop) */
/* the header (drawn when draw, else measured): its sig; *left the right part's left end */
static uint32_t ux_home_head(uint32_t p, int draw, int32_t *left)
{
    char tr[16], lf[40];
    uint16_t bc;
    uint32_t k, segs = ux_bat_segs(&bc), drop, rec = (song.rec >> song.sel) & 1u ? 2u : song.rec != 0u;
    int32_t ty = 3, y0 = ty + UF_HDR_CAP_Y, x = 232 - UXH_BAT_W - 4, lim = 8 + text_w(&UF_HDR, "ACID FM") + 6, xt, xl;
    int lock = (ui_prefs & PREF_BPM_LOCK) != 0;
    ux_transport(tr, sizeof tr);
    if (lock) x -= UXH_LOCK_W + 2;
    xt = x - text_w(&UF_HDR, tr);
    xl = rec ? xt - 2 - UXH_DOT : xt;
    for (drop = 0; drop < 3u; drop++) {
        ux_home_left(lf, sizeof lf, p, drop);
        if (xl - text_w(&UF_HDR, lf) >= lim) break;
    }
    *left = xl - text_w(&UF_HDR, lf);
    ux_head_drop = (uint8_t)(drop < 2u ? drop : 2u);
    if (draw) {
        int32_t sx = 232 - UXH_BAT_W;
        cv_begin(240, UX_H_LINE + UX_H_HEAD, UX_BG);
        ux_topline();
        GFX_HOOK_ALIGN(0, UX_H_LINE, 0, UX_H_LINE + UX_H_HEAD, AL_V, "header left");
        ux_text_grad(8, ty, &UF_HDR, "ACID FM", UX_BG);
        for (k = 0; k < 4u; k++)                        /* the battery */
            cv_rect(sx + (int32_t)k * 4, y0, 3, UF_HDR_CAP_H, k < segs ? bc : UX_LINE);
        if (lock) {                                     /* BPM LOCK: a shackle over a body, 7 x 8 */
            int32_t lx = 232 - UXH_BAT_W - 4 - UXH_LOCK_W;
            cv_rect(lx + 1, y0, 5, 1, UX_MID);
            cv_rect(lx + 1, y0, 1, 3, UX_MID);
            cv_rect(lx + 5, y0, 1, 3, UX_MID);
            cv_rect(lx, y0 + 3, UXH_LOCK_W, UF_HDR_CAP_H - 3, UX_MID);
        }
        GFX_HOOK_ALIGN(0, UX_H_LINE, 0, UX_H_LINE + UX_H_HEAD, AL_V, "header right");
        cv_text_r(x, ty, &UF_HDR, tr, UX_MID, UX_BG);
        if (rec) {                                      /* REC armed: the dot, a ring when another track is */
            int32_t dy = y0 + (UF_HDR_CAP_H - UXH_DOT) / 2;
            cv_rrect(xt - 2 - UXH_DOT, dy, UXH_DOT, UXH_DOT, 3, UX_ACCENT, UX_BG);
            if (rec == 1u)
                cv_rrect(xt - 2 - UXH_DOT + 2, dy + 2, UXH_DOT - 4, UXH_DOT - 4, 1, UX_BG, UX_ACCENT);
        }
        if (lf[0]) {
            GFX_HOOK_ALIGN(0, UX_H_LINE, 0, UX_H_LINE + UX_H_HEAD, AL_V, "header right");
            cv_text_r(xl, ty, &UF_HDR, lf, UX_MID, UX_BG);
        }
        cv_blit_cells(0, UX_Y_LINE);
    }
    return str_hash(str_hash(p + 1u + rec * 8u + (uint32_t)lock * 32u + segs * 64u + bc * 512u, tr), lf);
}
static const char *ux_vis_legend(void);                 /* (VISUAL's) */
/* the status line: a message (a refusal in UX_ACCENT), a turned knob's track and level, else what HOME does next */
#define UX_STOP "STOP TO EDIT"
static const char *ux_home_status(uint16_t *col)
{
    static char b[40];
    int32_t c = ux_trk_hot();
    if (ui.msg_t && ui.msg[0]) {
        *col = str_eq(ui.msg, UX_STOP) ? UX_ACCENT : UX_TEXT;
        return ui.msg;
    }
    if (ux_home_page() == UXH_VISUAL && (c = ux_knob_hot()) >= 0) {   /* "MACRO 2 · 71%": as the knob row names it */
        str_cpy(b, "MACRO 1 \xB7 ", sizeof b);
        b[6] = (char)('1' + c);
        fmt_int(b + str_len(b), macro_v[c] * 100 / 127);
        str_cpy(b + str_len(b), "%", sizeof b - str_len(b));
        *col = UX_TEXT;
        return b;
    }
    if (ux_home_page() == UXH_VISUAL && ux_vis_legend()) {   /* the first time on VISUAL: which ring is which */
        *col = UX_MID;
        return ux_vis_legend();
    }
    if (c >= 0) {
        char d[16];
        const char *u = ux_lvl_fmt((uint32_t)c, d);
        ux_trk_name((uint32_t)c, b, 22, 1);
        str_cpy(b + str_len(b), " \xB7 LEVEL ", sizeof b - str_len(b));
        str_cpy(b + str_len(b), d, sizeof b - str_len(b));
        if (u[0]) {                                     /* "T5 BRASS · LEVEL -2.1 dB" */
            str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
            str_cpy(b + str_len(b), u, sizeof b - str_len(b));
        }
        *col = UX_TEXT;
        return b;
    }
    *col = UX_MID;
    return ux_home_hint();
}

static void ux_home_knobs(ux_knob_t k[4], char lbl[4][10])
{
    uint32_t c, f = ux_trk_first();
    for (c = 0; c < 4u; c++) {
        if (ux_home_page() == UXH_VISUAL) {             /* MACRO 1..4, 0..100 % (the footer: M1..M4) */
            str_cpy(lbl[c], "MACRO 1", 10);
            lbl[c][6] = (char)('1' + c);
            fmt_int(k[c].value, macro_v[c] * 100 / 127);
            str_cpy(k[c].value + str_len(k[c].value), "%", sizeof k[c].value - str_len(k[c].value));
            k[c].bar = (uint8_t)(macro_v[c] * 255u / 127u);
        } else {                                        /* TRACKS: the four tracks' LEVELs */
            lbl[c][0] = 'T'; lbl[c][1] = (char)('1' + f + c);
            str_cpy(lbl[c] + 2, " LEVEL", 8);
            char d[16];
            (void)ux_lvl_fmt(f + c, d);                 /* the number ("-10.0dB" does not fit a column: "dB" in the status
                                                         * line on a turn, so every column reads the same way) */
            str_cpy(k[c].value, d, sizeof k[c].value);
            k[c].bar = (uint8_t)((uint32_t)clamp(trk[f + c].p[P_LEVEL], 0, 127) * 255u / 127u);
        }
        k[c].label = lbl[c];
    }
}

/* one ridgeline: the history over x UXR_X0 .. UXR_X0 + UXR_W - 1 (each column the ring read at its place, two points
 * mixed), a 1 px line (thick: 2) whose pixels at or below hz[x] are hidden (a nearer line's area), then hz[] raised to it.
 * The lines are drawn nearest first (T6 .. T1), so the glow stays under every line, no fill to paint */
static void ux_ridge(uint32_t c, int32_t base, uint16_t col, int thick, uint8_t *hz)
{
    int32_t i, yp = base;
    uint32_t pos = 0, step = ((UXR_N - 1u) << 16) / (UXR_W - 1u) + 1u;    /* (16.16: the last column the newest point) */
    for (i = 0; i < (int32_t)UXR_W; i++, pos += step) {
        uint32_t j = pos >> 16, f = (pos >> 8) & 255u;
        int32_t a = uxr.h[c][(uxr.at + j) % UXR_N], b = j + 1u < UXR_N ? uxr.h[c][(uxr.at + j + 1u) % UXR_N] : a;
        int32_t y = base - (a * 256 + (b - a) * (int32_t)f) * UXR_PEAK / (255 * 256), t = y < yp ? y : yp, bot = y < yp ? yp : y;
        if (i == 0) t = bot = y;
        if (thick) t--;
        if (bot >= hz[i]) bot = hz[i] - 1;
        if (bot >= t)
            cv_rect(UXR_X0 + i, t, 1, bot - t + 1, col);
        if (t < hz[i]) hz[i] = (uint8_t)(t < 0 ? 0 : t);
        yp = y;
    }
}
static uint16_t ux_under(int32_t x, int32_t y) { return swap16(cv_px[(uint32_t)(y + cv_oy) * cv_w + (uint32_t)x]); }
static void ux_tracks_panel(void)
{
    uint8_t hz[UXR_W];
    uint32_t f = ux_trk_first(), r, i, n16, pg, len;
    int32_t hot = ux_trk_hot(), ph;
    step_t st[16];
    char b[40];                                         /* ("STEP 64 \xB7 C#3 \xB7 ACCENT \xB7 SLIDE": 31 bytes) */
    const track_t *t = TSEL;
    memset(hz, UX_H_PANEL, sizeof hz);
    for (r = NTRK; r-- > 0u;) {                         /* the lines, the nearest (T6) first */
        int kn = r >= f && r < f + 4u, sel = r == song.sel;
        uint16_t col = kn ? ux_knob_col(r - f) : UX_DIM(UX_MID, 45);
        if (trk[r].p[P_MUTE]) col = UX_DIM(col, 55);
        ux_ridge(r, UXR_BASE(r), col, sel || (int32_t)r == hot, hz);
    }
    for (r = 0; r < NTRK; r++) {                        /* the names, the levels (in their columns: nothing over them) */
        int kn = r >= f && r < f + 4u;
        uint16_t col = kn ? ux_knob_col(r - f) : UX_DIM(UX_MID, 45);
        int32_t y = UXR_BASE(r) - UF_LBL_CAP_Y - UF_LBL_CAP_H, w;
        char d[16];
        if (trk[r].p[P_MUTE]) col = UX_DIM(col, 55);
        ux_trk_name(r, b, sizeof b, 0);
        ux_fit_name(b, UXR_X0 - 10);                    /* (fitted to the column) */
        w = text_w(&UF_LBL, b);
        GFX_HOOK_CELL(8, y, UXR_X0 - 2, y + UF_LBL.h);
        cv_text_on(8, y, &UF_LBL, b, r == song.sel ? UX_TEXT : col, ux_under(8 + w / 2, y + 5));
        (void)ux_lvl_fmt(r, d);                         /* (the number only: "dB" in the knob row and the status line) */
        w = text_w(&UF_LBL, d);
        GFX_HOOK_CELL(UXR_X0 + (int32_t)UXR_W + 2, y, 232, y + UF_LBL.h);
        cv_text_on(232 - w, y, &UF_LBL, d, (int32_t)r == hot ? UX_TEXT : col, ux_under(232 - w / 2, y + 5));
    }
    len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP);
    pg = ux_trk_page(&ph);
    n16 = len - pg * 16u < 16u ? len - pg * 16u : 16u;
    for (i = 0; i < 16u; i++) {                         /* the selected track's steps (a drum hit as a note) */
        st[i] = t->step[(pg * 16u + i) % NSTEP];
        if (st[i].hit && !st[i].n && st[i].time == ST_NOTE) st[i].n = 1;
        if (st[i].acc) st[i].flags |= SF_ACCENT;
    }
    {   /* the caption: whose steps, and the playing one spelled out */
        char d[8];
        b[0] = 'T'; b[1] = (char)('1' + song.sel); b[2] = 0;
        str_cpy(b + 2, " STEPS ", sizeof b - 2u);
        fmt_int(d, (int32_t)(pg * 16u + 1u)); str_cpy(b + str_len(b), d, sizeof b - str_len(b));
        str_cpy(b + str_len(b), "-", sizeof b - str_len(b));
        fmt_int(d, (int32_t)(pg * 16u + n16)); str_cpy(b + str_len(b), d, sizeof b - str_len(b));
        GFX_HOOK_CELL(8, UXR_CAP_Y, 232, UXR_CAP_Y + UF_LBL.h);
        cv_text_on(8, UXR_CAP_Y, &UF_LBL, b, UX_MID, ux_under(8, UXR_CAP_Y + 5));
        if (ph >= 0) {
            const step_t *s = &st[ph];
            str_cpy(b, "STEP ", sizeof b);
            fmt_int(d, (int32_t)(pg * 16u + (uint32_t)ph + 1u)); str_cpy(b + str_len(b), d, sizeof b - str_len(b));
            if (s->time == ST_TIE)
                str_cpy(b + str_len(b), " \xB7 TIE", sizeof b - str_len(b));
            else if (s->time != ST_NOTE || !s->n)
                str_cpy(b + str_len(b), " \xB7 REST", sizeof b - str_len(b));
            else {
                str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
                if (s->hit && drum_track(t))
                    str_cpy(b + str_len(b), "HIT", sizeof b - str_len(b));
                else {
                    note_name(d, s->note[0]);
                    str_cpy(b + str_len(b), d, sizeof b - str_len(b));
                }
                if (s->flags & SF_ACCENT) str_cpy(b + str_len(b), " \xB7 ACCENT", sizeof b - str_len(b));
                if (s->flags & SF_SLIDE) str_cpy(b + str_len(b), " \xB7 SLIDE", sizeof b - str_len(b));
            }
            cv_text_on(232 - text_w(&UF_LBL, b), UXR_CAP_Y, &UF_LBL, b, UX_TEXT, ux_under(200, UXR_CAP_Y + 5));
        }
    }
    ux_strip(UXR_STRIP_Y, st, n16, ph, ph);
}
static uint32_t ux_trk_draws, ux_trk_ms;              /* TRACKS panels drawn (the host tests count them), the last one's time */
static uint8_t ux_trk_glow;                             /* .. its glow level */
static uint32_t ux_tracks_sig(void)
{
    int32_t ph;
    uint32_t c, h = 2166136261u;
    h = (h ^ (uxr.nz ? uxr.at + 1u : 0u) ^ ux_glow_eff(2) << 24) * 16777619u;   /* (the glow off at once when load goes tight) */
    h = (h ^ (song.sel + 1u) ^ ((uint32_t)(ux_trk_hot() + 1) << 4) ^ (ux_trk_page(&ph) << 8)) * 16777619u;
    h = (h ^ (uint32_t)(ph + 1) ^ steps_hash(TSEL) ^ ((uint32_t)TSEL->p[P_SLEN] << 9)) * 16777619u;
    for (c = 0; c < NTRK; c++)
        h = (h ^ (uint32_t)trk[c].p[P_LEVEL] ^ ((uint32_t)trk[c].p[P_MUTE] << 7) ^ (uint32_t)trk[c].preset << 8 ^
             (uint32_t)trk[c].eng_req << 16) * 16777619u;
    return h;
}

/* ---- VISUAL (3a task 7; polish, tweaks): six thin rings round the centre, one a track's - T1 outside .. T6 at the centre -
 * all inside the panel, a faint glow behind them at the centre, the corners plain dark. Each track a colour of its own, spread
 * along the blend (ux_vis_hue), the six turned together a little with the bar (the phase: a turn every four bars from
 * chain.bar_pos; stopped, a slow drift); a ring's brightness its track's level (the loudest point of ux_levels_tick's
 * history in the last step: no work of its own on the audio), eased half the way each step, a silent track's ring a dim
 * LINE grey; a ring on a hit swells a px each side. Everything moves in steps (fix 1): taken at most every UXV_STEP_MS, so
 * a redraw between two steps paints the same picture and cv_blit_cells sends nothing of it (the LCD's SPI is the cost: a
 * whole panel is ~72 KB). Over them a darker band, the song's name and where it is: readability first (spec 0). The first
 * time on VISUAL the status line says which ring is which (spec 0.7) */
static uint32_t ux_vis_draws;                           /* VISUAL panels drawn (the host tests count them) */
static struct {
    uint32_t bars, ms, q_ms, shed, shed_ms, intro_ms;   /* bars passed; the last panel's time; the last step's; load_diag.shed
                                                         * seen, the time it last rose; the ring legend's start */
    uint16_t qph;                                       /* the rings' phase (0..511) at the last step */
    uint8_t b[NTRK];                                    /* each track's ring brightness (0..255), eased */
    uint8_t frac, q_ok, shed_any, intro;                /* the bar position last seen (0..255); a step taken; a shed seen; the
                                                         * legend shown (once a power-on) */
} uxv;
#define UXV_STEP_MS 200u                                /* the rings change at most every 200 ms (5 steps a second) */
#define UXV_PH_Q 4u                                     /* .. their phase in steps of 4 / 512 (a few a step: smooth) */
#define UXV_IDLE_MS 48u                                 /* stopped: a phase step every 48 ms (a turn in ~25 s) */
#define UXV_SWELL 160u                                  /* a ring this bright (a hit) is a px wider each side */
#define UXV_INTRO "RINGS: T1 OUTSIDE \x86 T6 CENTRE"     /* (UF_HDR's arrow, 0x86) */
#define UXV_INTRO_MS 4000u
#define UXV_TTL_W 200                                   /* the name in UF_TTL up to 200 px (20 px each side), else UF_VAL */
static const char *ux_vis_legend(void)                  /* the ring legend while it shows (UXV_INTRO_MS, once), else NULL */
{
    if (!uxv.intro) {
        uxv.intro = 1;
        uxv.intro_ms = fm1_ms;
    }
    return fm1_ms - uxv.intro_ms < UXV_INTRO_MS ? UXV_INTRO : 0;
}
static uint32_t ux_vis_phase(void)
{
    uint32_t bar = chain_bar(), f;
    if (!song.playing)
        return (fm1_ms / UXV_IDLE_MS) & 511u;
    f = bar ? (uint32_t)(chain.bar_pos * 256u) / bar : 0u;   /* (bar_pos < bar <= ~600000 samples: fits 32 bits x 256) */
    if (f > 255u) f = 255u;
    if (f < uxv.frac)                                   /* the bar line passed */
        uxv.bars++;
    uxv.frac = (uint8_t)f;
    return ((uxv.bars * 256u + f) / 2u) & 511u;
}
static uint32_t ux_vis_level(uint32_t c)                /* track c's loudest point of the last step (~200 ms, 0..255) */
{
    uint32_t a, m = 0;
    for (a = 0; a <= UXV_STEP_MS / UXR_MS; a++)
        if (ux_hist(c, a) > m) m = ux_hist(c, a);
    return m;
}
/* track c's colour: spread along the blend (T1 ACID .. T6 QUEUED, 46 steps apart), all six turned together by the phase
 * (0 .. UXV_TURN steps there and back): each track keeps a colour of its own, neighbours clearly apart */
#define UXV_TURN 24u
static uint16_t ux_vis_hue(uint32_t c)
{
    uint32_t p = uxv.qph < 256u ? uxv.qph : 511u - uxv.qph;
    return UX_BLEND[(c % NTRK) * 46u + p * UXV_TURN / 255u];
}
static void ux_vis_step(void)                           /* phase and brightness, at most every UXV_STEP_MS (a page entered: now) */
{
    uint32_t c;
    if (uxv.q_ok && fm1_ms - uxv.q_ms < UXV_STEP_MS && !ui.force)
        return;
    uxv.q_ms = fm1_ms;
    uxv.qph = (uint16_t)(ux_vis_phase() & ~(UXV_PH_Q - 1u));
    for (c = 0; c < NTRK; c++) {
        uint32_t t = ux_vis_level(c);
        uxv.b[c] = (uint8_t)(!uxv.q_ok || ui.force ? t : ((uint32_t)uxv.b[c] + t + 1u) / 2u);   /* (eased: half the way) */
    }
    uxv.q_ok = 1;
}
/* the song's name (the project's, as loaded or saved; none: ACID FM) into b (16 bytes) and its face: the letters UF_TTL has
 * (capitals, figures, space - . /; another sign a space, in every face), in UF_TTL while it fits UXV_TTL_W, else UF_VAL, then
 * UF_HDR */
static const aafont_t *ux_vis_name(char *b)
{
    char s[16];
    uint32_t i;
    project_cur_name(s);
    if (!s[0])
        str_cpy(s, "ACID FM", sizeof s);
    for (i = 0; s[i]; i++) {
        char c = s[i];
        b[i] = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '.' || c == '/' ? c : ' ';
    }
    while (i > 1u && b[i - 1u] == ' ')
        i--;
    b[i] = 0;
    if (text_w(&UF_TTL, b) <= UXV_TTL_W)
        return &UF_TTL;
    return text_w(&UF_VAL, b) <= 224 ? &UF_VAL : &UF_HDR;
}
static void ux_vis_sub(char *b) { ux_where_bar(b, 32, 1); }   /* "SECTION B · BAR 3/8", "BAR 1/2" */
static void ux_visual_panel(void)
{
    uint8_t e[2u * NTRK];
    uint16_t col[1u + 2u * NTRK], scr[1u + 2u * NTRK];
    uint32_t k, n = 0, gl = ux_glow_eff(2);
    char nm[16], sub[32];
    const aafont_t *f = ux_vis_name(nm);
    int32_t cy = f == &UF_TTL ? UF_TTL_CAP_Y : f == &UF_VAL ? UF_VAL_CAP_Y : UF_HDR_CAP_Y;
    int32_t ch = f == &UF_TTL ? UF_TTL_CAP_H : f == &UF_VAL ? UF_VAL_CAP_H : UF_HDR_CAP_H;
    int32_t top = ((int32_t)UX_H_PANEL - (ch + 9 + UF_LBL_CAP_H)) / 2, ty = top - cy, sy = top + ch + 9 - UF_LBL_CAP_Y, w;
    ux_vis_step();
    /* the segments from the centre out: the glow inside the innermost ring, each ring and the gap inside the next (the glow),
     * past the last ring plain BG (no more rings: the corners dark). A segment's glow is the panel glow's colour at its
     * middle radius (ux_glow_col, ring 0 outermost at ~128 px); ring k is track NTRK - 1 - k (T6 at the centre) */
#define UXV_GLOW(r) ux_glow_col(gl, UX_GLOW_N - 1u - ((r) * UX_GLOW_N / 128u < UX_GLOW_N ? (r) * UX_GLOW_N / 128u : UX_GLOW_N - 1u))
    for (k = 0; k < NTRK; k++) {
        uint32_t c = NTRK - 1u - k, br = uxv.b[c], sw = br >= UXV_SWELL;
        e[n] = (uint8_t)(4u * k + (sw ? 0u : 1u));                    /* the ring's inner edge (swollen: a px in) */
        col[n++] = UXV_GLOW(k ? UX_RING_R[4u * k] - 2u : 3u);       /* (inside it: the gap or the centre's glow) */
        e[n] = (uint8_t)(4u * k + (sw ? 3u : 2u));                    /* .. its outer edge */
        col[n++] = ux_mix(UX_LINE, ux_vis_hue(c), (int32_t)(12u + br * 88u / 255u));   /* silent: LINE, a hint of its colour */
    }
    col[n] = UX_BG;                                     /* past the last ring */
    for (k = 0; k <= n; k++)
        scr[k] = ux_mix(UX_BG, col[k], 40);             /* (under the text: TEXT on it stays near its contrast on BG) */
    cv_begin_raw(240, UX_H_PANEL, UX_BG);               /* (the rings write every pixel: no fill under them) */
    ux_rings(e, n, col, scr, top - 8, top + ch + 9 + UF_LBL_CAP_H + 8);
    w = text_w(f, nm);
    GFX_HOOK_CELL(8, ty, 232, ty + f->h);
    cv_text_on((240 - w) / 2, ty, f, nm, UX_TEXT, ux_under(120, top + ch / 2));
    ux_vis_sub(sub);
    w = text_w(&UF_LBL, sub);
    GFX_HOOK_CELL(8, sy, 232, sy + UF_LBL.h);
    cv_text_on((240 - w) / 2, sy, &UF_LBL, sub, UX_TEXT, ux_under(120 + w / 2 + 2, sy + UF_LBL_CAP_Y + 3));
    ux_vis_draws++;
}

/* the footer's right: the page of HOME's and the knobs' short names */
static void ux_home_foot(char *b, uint32_t n)
{
    uint32_t p = ux_home_page();
    str_cpy(b, ux_home_count(), n);
    if (p == UXH_TRACKS)
        str_cpy(b + str_len(b), " \xB7 LVL \xB7 LVL \xB7 LVL \xB7 LVL", n - str_len(b));
    else if (p == UXH_VISUAL)
        str_cpy(b + str_len(b), " \xB7 M1 \xB7 M2 \xB7 M3 \xB7 M4", n - str_len(b));
}

static void ux_home_draw(void)
{
    static uint32_t head_sig, panel_sig, panel_p, foot_sig;   /* (panel_p: the page whose panel is shown, + 1) */
    ux_knob_t k[4];
    char lbl[4][10];
    int32_t hx;
    uint32_t p = ux_home_page(), all, sig, shed;
    uint16_t sc;
    const char *st = ux_home_status(&sc);
    dc_frame(ui.force);
    all = dc_all;
    sig = ux_home_head(p, 0, &hx);
    if (all || sig != head_sig) {
        head_sig = sig;
        (void)ux_home_head(p, 1, &hx);
    }
    ux_home_knobs(k, lbl);
    ux_knobs(k, -1);
    {   /* the overload guard's sheds: the time it last shed (VISUAL's and TRACKS' panels frozen UX_SHED_HOLD_MS after) */
        uint32_t sh = load_diag.shed;
        if (sh != uxv.shed) {
            uxv.shed_ms = fm1_ms;
            uxv.shed_any = 1;
        }
        uxv.shed = sh;
        shed = uxv.shed_any ? fm1_ms - uxv.shed_ms : UX_NO_SHED;
    }
    if (p == UXH_TRACKS) {                              /* TRACKS: entered at once; then on a change, as the load allows */
        sig = ux_tracks_sig();                          /* (ux_pace_at: every UX_TRK_MS at most, UX_TRK_SLOW_MS while tight or
                                                         * LOAD LITE / ECO, frozen while the guard's shed hold or gov_lite; the
                                                         * header, knob row and status line answer every frame all the same) */
        uint32_t gl = ux_glow_eff(2), gn = gl != ux_trk_glow;   /* (the glow changed - off when load goes tight: at once) */
        if (all || panel_p != p + 1u || (sig != panel_sig &&
            ux_pace_at(fm1_ms, ux_trk_ms, load_tight, shed, gov_lite, sys_load, 0, gn ? 0u : UX_TRK_MS,
                       gn ? 0u : UX_TRK_SLOW_MS).draw)) {
            panel_sig = sig;
            panel_p = p + 1u;
            ux_trk_glow = (uint8_t)gl;
            ux_trk_ms = fm1_ms;
            ux_panel_begin(2);
            ux_tracks_panel();
            ux_panel_end();
            ux_trk_draws++;
        }
    } else if (all || panel_p != p + 1u ||             /* VISUAL: entered at once, then as the load allows (ux_pace) */
               ux_pace(fm1_ms, uxv.ms, load_tight, shed, gov_lite, sys_load, 0).draw) {
        panel_p = p + 1u;
        uxv.ms = fm1_ms;
        ux_visual_panel();
        ux_panel_end();
    }
    if (all || foot_sig != p + 1u) {
        char fr[40];
        foot_sig = p + 1u;
        ux_home_foot(fr, sizeof fr);
        ux_footer(ux_home_where(), fr);
    }
    ux_status(st, sc);
}
