/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's HOME on the screen (spec 4.3), every piece one of ux_widgets.c's: the header ACID FM, the knob row, the panel
 * over the glow, the status line (spec 0: the screen says what the controls do; a knob turn shows its value at once; a
 * refusal says why), the footer HOME . page and n / 3.
 * TRACKS (3a task 6): six ridgelines, T1 at the top .. T6 at the bottom, each its track's level over the last ~2 s
 * scrolling right to left, the nearer (lower) lines hiding the ones behind, pulsar-plot style; the four tracks the knobs
 * control in their knobs' colours, the other two dimmed; the name at the left, the level 0..99 at the right; under them the
 * selected track's 16 steps, the playing one outlined and spelled out. KNOB 1..4: the levels of the four.
 * VISUAL: a placeholder until task 7. Included after ux_widgets.c; ux_home_draw is called by ui_draw.c ui_draw_body,
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
static void ux_trk_right(char *b)                       /* the header's right: "A · 3/8 · ▶ 124 BPM", "BAR 1/2 · ■ 124 BPM" */
{
    uint32_t n;
    char d[8];
    b[0] = 0;
    if (chain.running) {
        const chain_row_t *r = &chain.config.row[chain.row % CHAIN_ROWS];
        b[0] = (char)('A' + (r->slot & 3u)); b[1] = ' '; b[2] = (char)0xB7; b[3] = ' '; b[4] = 0;
        n = r->bars > chain.remaining ? r->bars - chain.remaining + 1u : 1u;
        fmt_int(d, (int32_t)n); str_cpy(b + str_len(b), d, 32 - str_len(b));
        str_cpy(b + str_len(b), "/", 32 - str_len(b));
        fmt_int(d, r->bars); str_cpy(b + str_len(b), d, 32 - str_len(b));
    } else {
        int32_t ph;
        uint32_t len = (uint32_t)clamp(TSEL->p[P_SLEN], 1, NSTEP);
        str_cpy(b, "BAR ", 32);
        fmt_int(d, (int32_t)ux_trk_page(&ph) + 1); str_cpy(b + str_len(b), d, 32 - str_len(b));
        str_cpy(b + str_len(b), "/", 32 - str_len(b));
        fmt_int(d, (int32_t)((len + 15u) / 16u)); str_cpy(b + str_len(b), d, 32 - str_len(b));
    }
    str_cpy(b + str_len(b), " \xB7 ", 32 - str_len(b));
    ux_transport(b + str_len(b), 32 - str_len(b));
}
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
        if (ux_home_page() == UXH_VISUAL) {             /* M1..M4, 0..100 % */
            lbl[c][0] = 'M'; lbl[c][1] = (char)('1' + c); lbl[c][2] = 0;
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
    char b[28];
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
static uint32_t ux_tracks_sig(void)
{
    int32_t ph;
    uint32_t c, h = 2166136261u;
    h = (h ^ (uxr.nz ? uxr.at + 1u : 0u)) * 16777619u;
    h = (h ^ (song.sel + 1u) ^ ((uint32_t)(ux_trk_hot() + 1) << 4) ^ (ux_trk_page(&ph) << 8)) * 16777619u;
    h = (h ^ (uint32_t)(ph + 1) ^ steps_hash(TSEL) ^ ((uint32_t)TSEL->p[P_SLEN] << 9)) * 16777619u;
    for (c = 0; c < NTRK; c++)
        h = (h ^ (uint32_t)trk[c].p[P_LEVEL] ^ ((uint32_t)trk[c].p[P_MUTE] << 7) ^ (uint32_t)trk[c].preset << 8 ^
             (uint32_t)trk[c].eng_req << 16) * 16777619u;
    return h;
}

static void ux_home_draw(void)
{
    static uint32_t head_sig, panel_sig, foot_sig, panel_ms;
    ux_knob_t k[4];
    char lbl[4][10], right[32];
    uint32_t p = ux_home_page(), all, sig;
    uint16_t sc;
    const char *st = ux_home_status(&sc);
    dc_frame(ui.force);
    all = dc_all;
    right[0] = 0;
    if (p == UXH_TRACKS)
        ux_trk_right(right);
    sig = str_hash(p + 1u, right);
    if (all || sig != head_sig) {
        head_sig = sig;
        ux_header(0, "ACID FM", right);
    }
    ux_home_knobs(k, lbl);
    ux_knobs(k, -1);
    if (p == UXH_TRACKS) {
        sig = ux_tracks_sig();
        if (all || (sig != panel_sig && !(sys_load && fm1_ms - panel_ms < (sys_load == LOAD_ECO ? 70u : 50u)))) {
            panel_sig = sig;                            /* (LOAD LITE / ECO: the lines at most every 50 / 70 ms) */
            panel_ms = fm1_ms;
            ux_panel_begin(2);
            ux_tracks_panel();
            ux_panel_end();
        }
    } else if (all || panel_sig != p + 1u) {            /* VISUAL: the placeholder (task 7) */
        const char *nm = CE_HOME[p].nm;
        int32_t w = text_w(&UF_TTL, nm), x = (240 - w) / 2, y = (int32_t)(UX_H_PANEL - UF_TTL.h) / 2;
        panel_sig = p + 1u;
        ux_panel_begin(3);
        ux_text_grad(x, y, &UF_TTL, nm, swap16(cv_px[(uint32_t)(y + UF_TTL.h / 2) * cv_w + (uint32_t)(x + w / 2)]));
        ux_panel_end();
    }
    if (all || foot_sig != p + 1u) {
        char fr[40];
        foot_sig = p + 1u;
        str_cpy(fr, ux_home_count(), sizeof fr);
        if (p == UXH_TRACKS)
            str_cpy(fr + str_len(fr), " \xB7 LVL \xB7 LVL \xB7 LVL \xB7 LVL", sizeof fr - str_len(fr));
        ux_footer(ux_home_where(), fr);
    }
    ux_status(st, sc);
}
