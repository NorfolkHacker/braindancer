/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's section pages on the screen (ux_section.h; 3b spec 2, parent spec 3.3): any PAGES row drawn with the 3a widgets.
 *   header   T3 . the sound, the transport at the right (ux_header)
 *   knobs    the PAGES row's four: the label in plain words (UXS_NAME, UXS_LABEL), the value as params.c param_format shows it, the
 *            bar its place in the range, KNOB 1..4 ACID SLIDE VIOLET ACCENT; MORE: KNOB 1 the setting picked (n/N), KNOB 2
 *            its value
 *   panel    the page's picture over the glow (Tasks 4..8 draw each page's; until then its title and the four values as
 *            bars), MORE its list (ux_more.c)
 *   status   a message (STOP TO EDIT in ACCENT), else the knob just turned: its full name and value ("RELEASE . 420 ms"),
 *            else the section's first-time hint ("ARP AGAIN: NEXT PAGE", once a power-on, spec 0 rule 7), else on MORE
 *            what its controls do
 *   footer   SECTION . PAGE at the left; n / N of the section's chain (N counts MORE) and the knobs' names (the knob row's,
 *            when they fit) at the right
 * Strips as HOME's (dc_frame / cv_blit_cells): only the changed cells go out. Included after ux_more.c (and ux_home.c:
 * ux_under, ux_trk_name). */

/* a setting's plain name (spec 0 rule 4: one name everywhere it shows, the knob row, the status line, MORE's rows, the footer),
 * by the section showing it, the track's engine, global or the track's, and its id: where JIANT's label is cryptic or names
 * more than one thing (MODE: the arp's, the glide's, the slicer's, the LFO's). UXS_ALL: every section / engine; the most
 * specific row wins (its section over every section's, then its engine over every engine's). JIANT's labels themselves
 * (TP / GP: the web editor reads them, editor.c) stay as they are. Each fits a knob's column in UF_LBL (tests/ui_test.c) */
#define UXS_ALL 0xFFu
typedef struct { uint8_t sec, eng, glo, id; const char *p; } uxs_name_t;   /* id: a P_* (glo 0) or G_* (glo 1), or a view */
static const uxs_name_t UXS_NAME[] = {
    {UXS_ALL, UXS_ALL, 0, P_GLMODE, "GLIDE MODE"},      /* (JIANT's MODE: RATE / TIME, the glide's) */
};
static const char *uxs_name_in(const uxs_name_t *t, uint32_t n, uint32_t sec, uint32_t eng, uint32_t glo, uint32_t id)
{
    const char *p = 0;
    uint32_t i, best = 0;
    for (i = 0; i < n; i++) {
        uint32_t sc;
        if (t[i].id != id || (t[i].glo != glo && id < ID_LPHS) ||          /* (a view: either) */
            (t[i].sec != UXS_ALL && t[i].sec != sec) || (t[i].eng != UXS_ALL && t[i].eng != eng))
            continue;
        sc = 1u + (t[i].sec != UXS_ALL ? 2u : 0u) + (t[i].eng != UXS_ALL ? 1u : 0u);
        if (sc > best)
            best = sc, p = t[i].p;
    }
    return p;
}
/* .. else plain words for JIANT's short labels, by their text: the same everywhere */
typedef struct { const char *j, *p; } uxs_lbl_t;
static const uxs_lbl_t UXS_LABEL[] = {
    {"ATK", "ATTACK"}, {"DEC", "DECAY"}, {"SUS", "SUSTAIN"}, {"REL", "RELEASE"}, {"RES", "RESO"}, {"CUT", "CUTOFF"},
    {"DLY", "DELAY"}, {"REV", "REVERB"}, {"FEEDBK", "FEEDBACK"}, {"PRIO", "PRIORITY"}, {"DIST", "DRIVE"},
};
static const char *uxs_label(uint32_t sec, uint32_t glo, uint32_t id, const char *l)
{
    uint32_t i;
    const char *p = id < 256u ? uxs_name_in(UXS_NAME, NELEM(UXS_NAME), sec, trk_eng(TSEL), glo ? 1u : 0u, id) : 0;
    if (p)
        return p;
    if (!l)
        return "";
    for (i = 0; i < NELEM(UXS_LABEL); i++)
        if (str_eq(l, UXS_LABEL[i].j))
            return UXS_LABEL[i].p;
    return l;
}

#define UXS_HINT " AGAIN: NEXT PAGE"                    /* (after the section's name: what its button does next) */
#define UXM_HINT "SELECT SCROLLS \xB7 KNOB 2 SETS"      /* MORE: what its controls do (KNOB 1 picks: its column says so) */
#define UXS_PICK "SETTING"                              /* MORE's KNOB 1 */
static uint16_t uxs_seen;        /* the sections opened since power-on (a bit per SEC_*): their hint said */
static uint8_t uxs_hint_sec = 0xFF, uxs_hint_page;      /* the page saying its section's hint (0xFF none): the first one
                                                         * opened there, until it is left or a knob turns */
static uint32_t uxs_frame;                              /* the UI frame last drawn here (another in between: left) */

/* the footer's left, "ARP . MORE"; its right's count, "3 / 3" (the chain's pages shown here, MORE too) */
static const char *uxs_where(void)
{
    static char b[32];
    str_cpy(b, SEC_NAME[nav_sec()], sizeof b);
    str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), nav_page_name(), sizeof b - str_len(b));
    return b;
}
static const char *uxs_count(void)
{
    static char b[12];
    uint8_t cnt[16], gi[16];
    uint32_t g, p, i, m;
    (void)nav_groups(cnt, gi, &g, &p, &i, &m);
    fmt_int(b, (int32_t)i + 1);
    str_cpy(b + str_len(b), " / ", sizeof b - str_len(b));
    fmt_int(b + str_len(b), (int32_t)m);
    return b;
}

/* the knob row of page pg (MORE: its pick and value), each label its plain name (uxs_label) */
static void uxs_knobs(const page_t *pg, ux_knob_t k[4])
{
    uint32_t c;
    for (c = 0; c < 4u; c++) {
        k[c].label = "";
        k[c].value[0] = 0;
        k[c].bar = 0;
    }
    if (pg->graph == GR_MORE) {
        uint32_t sec = nav_sec(), s = uxm_at[sec % SEC_N].sel, t = uxm_at[sec % SEC_N].top, n;
        uint64_t m;
        char v[24];
        if (!pg->more)
            return;
        m = uxm_mask(pg->more);
        n = uxm_n(pg->more);
        uxm_fix(pg->more, m, &s, &t);
        k[0].label = UXS_PICK;
        if (s < n) {
            uint32_t a = 0, b = 0, i;
            for (i = 0; i < n; i++)
                if (uxm_pick_ok(pg->more, m, i)) {
                    b++;
                    if (i <= s)
                        a = b;
                }
            fmt_int(k[0].value, (int32_t)a);
            str_cpy(k[0].value + str_len(k[0].value), "/", sizeof k[0].value - str_len(k[0].value));
            fmt_int(k[0].value + str_len(k[0].value), (int32_t)b);
            k[0].bar = (uint8_t)(b > 1u ? (a - 1u) * 255u / (b - 1u) : 255u);
            k[1].label = uxm_row_text(&pg->more->rows[s], v, sizeof v, 0, &k[1].bar);
            if (pg->more->rows[s].kind == UXR_ACTION)
                str_cpy(v, "GO", sizeof v);
            str_cpy(k[1].value, v, sizeof k[1].value);
        }
        return;
    }
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, c, &vp);
        const char *u;
        char v[16];
        if (!d || !vp || !d->label || d->label[0] == '-')
            continue;
        param_format(d, *vp, v, &u);
        str_cpy(k[c].value, v, sizeof k[c].value);
        k[c].label = uxs_label(nav_sec(), pg->scope == SC_GLOBAL, page_id(pg, c), d->label);
        if (d->max > d->min) {
            int32_t b = clamp(((int32_t)*vp - d->min) * 255 / (d->max - d->min), 0, 255);
            k[c].bar = (uint8_t)(d->fmt == F_PCTINV ? 255 - b : b);
        }
    }
}

/* the footer's right: cnt ("3 / 3"), then the knob row's names as it shows them (spec 0 rule 4: the same names; JIANT's short
 * labels were not: DIST under DRIVE) when they fit beside w, the footer's left */
static void uxs_foot(const char *w, const ux_knob_t k[4], const char *cnt, char *b, uint32_t nb)
{
    char t[48];
    uint32_t c, n = 0;
    str_cpy(b, cnt, nb);
    str_cpy(t, cnt, sizeof t);
    for (c = 0; c < 4u; c++)
        if (k[c].label[0]) {
            str_cpy(t + str_len(t), " \xB7 ", sizeof t - str_len(t));
            str_cpy(t + str_len(t), k[c].label, sizeof t - str_len(t));
            n++;
        }
    if (n && 8 + text_w(&UF_LBL, w) + 16 + text_w(&UF_LBL, t) <= 232)
        str_cpy(b, t, nb);
}

/* the status line (above); a knob's readout: "NAME . VALUE unit" */
static const char *uxs_status(const page_t *pg, uint16_t *col)
{
    static char b[48];
    int32_t h = ux_knob_hot();
    uint32_t sec = nav_sec();
    if (ui.msg_t && ui.msg[0]) {
        *col = str_eq(ui.msg, UX_STOP) ? UX_ACCENT : UX_TEXT;
        return ui.msg;
    }
    if (h >= 0) {
        const char *nm = 0;
        char v[24];
        uint8_t bar;
        if (pg->graph == GR_MORE && pg->more && h < 2) {   /* (KNOB 1 or 2: the setting picked) */
            uint32_t s = uxm_at[sec % SEC_N].sel, t = uxm_at[sec % SEC_N].top;
            uxm_fix(pg->more, uxm_mask(pg->more), &s, &t);
            if (s < uxm_n(pg->more))
                nm = uxm_row_text(&pg->more->rows[s], v, sizeof v, 1, &bar);
        } else if (pg->graph != GR_MORE) {
            int16_t *vp;
            const param_desc_t *d = page_desc(pg, (uint32_t)h, &vp);
            if (d && vp && d->label && d->label[0] != '-') {
                const char *u = "";
                param_format(d, *vp, v, &u);
                if (u && u[0]) {
                    str_cpy(v + str_len(v), " ", sizeof v - str_len(v));
                    str_cpy(v + str_len(v), u, sizeof v - str_len(v));
                }
                nm = uxs_label(sec, pg->scope == SC_GLOBAL, page_id(pg, (uint32_t)h), d->label);
            }
        }
        if (nm && nm[0]) {
            str_cpy(b, nm, sizeof b);
            str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
            str_cpy(b + str_len(b), v, sizeof b - str_len(b));
            *col = UX_TEXT;
            return b;
        }
    }
    *col = UX_MID;
    if (sec == uxs_hint_sec) {
        str_cpy(b, SEC_NAME[sec], sizeof b);
        str_cpy(b + str_len(b), UXS_HINT, sizeof b - str_len(b));
        return b;
    }
    return pg->graph == GR_MORE ? UXM_HINT : 0;
}

/* the page's picture: MORE its list; the others (until Tasks 4..8 give each its own) the page's title over its four
 * values as bars in the knobs' colours */
static void uxs_picture(const page_t *pg, const ux_knob_t k[4])
{
    const aafont_t *f = &UF_TTL;
    int32_t w, x, i;
    if (pg->graph == GR_MORE) {
        uint32_t sec = nav_sec() % SEC_N;
        ux_more_draw(pg->more, uxm_at[sec].sel, uxm_at[sec].top);
        return;
    }
    w = text_w(f, pg->title);
    if (w > 224) {
        f = &UF_VAL;
        w = text_w(f, pg->title);
    }
    x = (240 - w) / 2;
    ux_text_grad(x, 30, f, pg->title, ux_under(x + w / 2, 40));
    for (i = 0; i < 4; i++) {
        int32_t y = 86 + i * 14, fw = (int32_t)k[i].bar * 200 / 255;
        cv_rect(20, y, 200, 6, UX_EMPTY);
        if (fw > 0 && k[i].label[0])
            cv_rect(20, y, fw, 6, ux_knob_col((uint32_t)i));
    }
}

static void ux_section_draw(void)
{
    static uint32_t head_sig, panel_sig, foot_sig;
    const page_t *pg = cur_page();
    uint32_t sec = nav_sec(), sig, all, c;
    ux_knob_t k[4];
    const char *st;
    uint16_t sc;
    char b[48];
    if (ui.frame != uxs_frame + 1u || sec != uxs_hint_sec || ui.page != uxs_hint_page)
        uxs_hint_sec = 0xFF;                            /* (the first page left: its hint gone) */
    uxs_frame = ui.frame;
    if (sec < 16u && !((uxs_seen >> sec) & 1u)) {       /* the section's first opening since power-on: its hint */
        uxs_seen |= (uint16_t)(1u << sec);
        uxs_hint_sec = (uint8_t)sec;
        uxs_hint_page = ui.page;
    }
    if (ux_knob_hot() >= 0)                             /* (a knob turned: the page found out) */
        uxs_hint_sec = 0xFF;
    dc_frame(ui.force);
    all = dc_all;
    {   /* the header: T3 . the sound, the transport */
        char nm[24], tr[16];
        ux_trk_name(song.sel % NTRK, nm, sizeof nm, 1);
        ux_transport(tr, sizeof tr);
        sig = str_hash(str_hash(song.sel + 1u, nm), tr);
        if (all || sig != head_sig) {
            head_sig = sig;
            ux_header(song.sel % NTRK + 1u, nm + 3, tr);   /* (ux_trk_name's "T3 " left out: the header adds it) */
        }
    }
    uxs_knobs(pg, k);
    ux_knobs(k, -1);
    sig = (uint32_t)(pg - PAGES) * 2654435761u ^ ux_glow_eff(1) * 977u;   /* (the panel: drawn when what it shows changed) */
    for (c = 0; c < 4u; c++)
        sig = str_hash(sig * 31u + k[c].bar, k[c].value);
    if (pg->graph == GR_MORE && pg->more) {             /* (MORE: the pick, the scroll, the rows shown and their values) */
        uint64_t m = uxm_mask(pg->more);
        uint32_t i;
        sig = sig * 131u + uxm_at[sec % SEC_N].sel * 7u + uxm_at[sec % SEC_N].top + (uint32_t)(m ^ (m >> 32));
        for (i = 0; i < uxm_n(pg->more); i++)
            if (uxm_on(m, i) && pg->more->rows[i].kind != UXR_HEAD) {
                int16_t *vp;
                if (param_ref(pg->more->rows[i].kind == UXR_GLOBAL ? 0 : TSEL, pg->more->rows[i].id, &vp) && vp)
                    sig = sig * 33u + (uint16_t)*vp;
            }
    }
    if (all || sig != panel_sig) {
        panel_sig = sig;
        ux_panel_begin(1);
        uxs_picture(pg, k);
        ux_panel_end();
    }
    {   /* the footer: where, n / N and the knobs' names (when they fit) */
        const char *w = uxs_where();
        uxs_foot(w, k, uxs_count(), b, sizeof b);
        sig = str_hash(str_hash(1u, w), b);
        if (all || sig != foot_sig) {
            foot_sig = sig;
            ux_footer(w, b);
        }
    }
    st = uxs_status(pg, &sc);
    ux_status(st, sc);
}
