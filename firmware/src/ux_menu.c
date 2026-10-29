/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM 3c-2 task 5 (spec 3): MENU (HOME held) as a section, AUDIO -> CLICK -> CONTROLS -> MORE (menu_items.c MN_ROW, UXM_MENU),
 * on the ux pieces: the header, the knob row, the panel (a page: its four settings, name, value and what each does; MORE: the
 * list, ux_more.c), the status line (the hint of the setting turned or picked last), the footer "MENU . AUDIO   1 / 4 . NEXT
 * CLICK". KNOB n sets the page's setting n (menu_put(menu_step), as the inherited menu did), an action (CALIBRATION) armed by its
 * knob and done by OCT+; OCT+ / OCT- step the setting turned last. SELECT walks the pages (no wrap) and scrolls MORE (before its
 * first row: CONTROLS). MORE: KNOB 1 picks, KNOB 2 sets, an action row (MIDI IS ON GLO, ABOUT) runs on KNOB 2 right or OCT+.
 * ALGORITHM and PRESETS do nothing; HOME closes (ui_input.c, from ABOUT too). ABOUT and PROFILE (ui.menu 2, 3): their documents
 * (ui_menu.c menu_document, prof_document) scrolled by SELECT, OCT+ PROFILE, OCT- back. Its own state: ui.menu, ui.mn_page,
 * ui.mn_sel / mn_top (MORE's pick and scroll), ui.menu_sel (the setting turned or picked last), ui.menu_scroll. Included after
 * ux_dialog.c (ux_section.c's uxs_foot). */

static const char *const UXN_WORD[] = {"MENU \xB7 ", "OCT+", " / ", "ABOUT", "PROFILE", "SELECT SCROLLS \xB7 OCT+ PROFILE",
                                       "SELECT SCROLLS \xB7 OCT- ABOUT"};
#define UXN_ROW_P 37             /* a page's rows on the panel: pitch, height, the name's and the hint's lines */
#define UXN_ROW_H 34
#define UXN_BAND (MENU_DOC_H / 2)   /* ABOUT / PROFILE drawn in two bands of the document's rows */
#define UXN_LIVE_MS 1000u        /* .. their live figures (the load, PROFILE's second) redrawn at most this often */
static uint32_t uxn_panels;      /* the panels drawn (tests/ui_test.c: one a change, none idle) */

static uint32_t uxn_page(void) { return ui.mn_page < MN_N ? ui.mn_page : MN_AUDIO; }
static uint32_t uxn_row(uint32_t c) { return uxn_page() < MN_MORE && c < 4u ? MN_ROW[uxn_page()][c] : MI_NONE; }
static uint32_t uxn_pick(void)                          /* MORE's row picked (MI_*; none: MI_NONE) */
{
    uint32_t s = ui.mn_sel, t = ui.mn_top;
    uxm_fix(&UXM_MENU, uxm_mask(&UXM_MENU), &s, &t);
    ui.mn_sel = (uint8_t)s;
    ui.mn_top = (uint8_t)t;
    return s < UXM_MENU.n && UXM_MENU.rows[s].kind == UXR_MENU ? UXM_MENU.rows[s].id : MI_NONE;
}
/* page p shown: a page's first setting the one OCT steps (a value, never an action), MORE's the row picked */
static void uxn_go(uint32_t p)
{
    ui.mn_page = (uint8_t)(p < MN_N ? p : MN_MORE);
    ui.menu_sel = (uint8_t)(p < MN_MORE ? MN_ROW[p][0] : uxn_pick());
}
/* an action: CALIBRATION the panel's setup, MIDI IS ON GLO there (GLO > MIDI; LEARN, NOTES OUT, CLOCK OUT GLO > MORE's), ABOUT */
static void uxn_do(uint32_t row)
{
    if (row == MI_GLOMIDI) {
        menu_close();
        nav_put(SEC_GLO, "MIDI");
    } else if (row == MI_PANEL) {
        panel_setup();
        scrn.idle = fm1_ms;                             /* (SCREEN OFF: its own loop had the input) */
        ui.force = 1;
    } else if (row == MI_ABOUT) {
        ui.menu = 2;
        ui.menu_scroll = 0;
        ui.force = 1;
    }
}
/* a MENU row turned s (a page's knob, MORE's KNOB 2: ux_more.c): a value stepped (stopping at its ends), an action done on a
 * turn up */
static void uxn_set(uint32_t row, int32_t s)
{
    if (row >= MI_COUNT || menu_hidden(row) || !s)
        return;
    ui.menu_sel = (uint8_t)row;
    if (menu_valued(row))
        menu_put(row, menu_step(row, s));
    else if (s > 0)
        uxn_do(row);
}

static void ux_menu_input(uint32_t oct)                /* oct: ui_input.c oct_taps */
{
    int32_t s;
    uint32_t k, p = uxn_page(), up = (oct >> 1) & 1u, dn = oct & 1u;
    if (ui.menu >= 2) {                                /* ABOUT: SELECT scrolls (bounded), OCT- back to MORE; (0.9.14) OCT+
                                                         * PROFILE (its OCT- back to ABOUT) */
        if (dn)
            ui.menu = (uint8_t)(ui.menu - 1), ui.menu_scroll = 0, ui.force = 1;
        else if (up && ui.menu == 2) {
            if (!prf_on)
                memset(&prf_cur, 0, sizeof prf_cur);
            ui.menu = 3, ui.menu_scroll = 0, prf_on = 1, ui.force = 1;
        }
        else if ((s = panel_enc(EN_SELECT)) != 0)
            ui.menu_scroll = (uint16_t)clamp((int32_t)ui.menu_scroll + clamp(s, -128, 128) * 18, 0, menu_scroll_max());
        enc_drop();
        return;
    }
    if ((s = panel_enc(EN_SELECT)) != 0) {             /* SELECT: the pages (no wrap), then MORE's rows */
        if (p == MN_MORE) {
            uint32_t sl = ui.mn_sel, t = ui.mn_top;
            if (ux_more_turn(&UXM_MENU, &sl, &t, UXM_SELECT, s)) {
                ui.mn_sel = (uint8_t)sl;
                ui.mn_top = (uint8_t)t;
                ui.menu_sel = (uint8_t)uxn_pick();
            } else
                uxn_go(MN_CONTROLS);                    /* (before its first row: the page before) */
        } else if (clamp((int32_t)p + s, 0, MN_MORE) != (int32_t)p)
            uxn_go((uint32_t)clamp((int32_t)p + s, 0, MN_MORE));
    }
    for (k = 0; k < 4u; k++) {                         /* KNOB n: the page's setting n; MORE: KNOB 1 picks, KNOB 2 sets */
        uint32_t r = uxn_row(k), sl = ui.mn_sel, t = ui.mn_top;
        if ((s = panel_enc(EN_K1 + k)) == 0 || (p != MN_MORE && r == MI_NONE))
            continue;
        ux_knob_touch(k);
        if (p == MN_MORE) {
            if (k < 2u) {
                (void)uxn_pick();                       /* (the row KNOB 2 sets: the one shown picked) */
                sl = ui.mn_sel;
                t = ui.mn_top;
                (void)ux_more_turn(&UXM_MENU, &sl, &t, k, s);
                ui.mn_sel = (uint8_t)sl;
                ui.mn_top = (uint8_t)t;
                if (ui.menu == 1)
                    ui.menu_sel = (uint8_t)uxn_pick();
            } else
                ui_message(UXM_K34);
        } else if (menu_valued(r))
            uxn_set(r, s);
        else
            ui.menu_sel = (uint8_t)(s > 0 ? r : MN_ROW[p][0]);   /* (an action: armed, or let go) */
        if (ui.menu != 1)                               /* (an action left the pages: ABOUT, GLO > MIDI) */
            break;
    }
    if (ui.menu == 1 && menu_valued(ui.menu_sel)) {     /* OCT+ the next value, OCT- the previous, of the setting turned last */
        s = (int32_t)up - (int32_t)dn;
        if (s)
            menu_put(ui.menu_sel, menu_step(ui.menu_sel, s));
    } else if (ui.menu == 1 && up)
        uxn_do(ui.menu_sel);                            /* (an action armed or picked: done) */
    enc_drop();                                        /* (ALGORITHM, PRESETS and the rest: nothing while the menu is up) */
}

/* the footer's left, "MENU . AUDIO" ("MENU . ABOUT", "MENU . PROFILE") */
static const char *uxn_where(void)
{
    static char b[24];
    str_cpy(b, UXN_WORD[0], sizeof b);
    str_cpy(b + str_len(b), ui.menu == 2 ? UXN_WORD[3] : ui.menu == 3 ? UXN_WORD[4] : MN_NAME[uxn_page()], sizeof b - str_len(b));
    return b;
}
/* the knob row: a page's four settings (its knob label, its value, its place), an action's "GO" / "OCT+" armed; MORE's as a
 * section's (ux_more.c) */
static void uxn_knobs(ux_knob_t k[4])
{
    uint32_t c, r, n;
    for (c = 0; c < 4u; c++) {
        k[c].label = "";
        k[c].value[0] = 0;
        k[c].bar = 0;
        k[c].off = 0;
        k[c].mark = 0;
    }
    if (uxn_page() == MN_MORE) {
        uxm_knobs(&UXM_MENU, ui.mn_sel, ui.mn_top, k);
        return;
    }
    for (c = 0; c < 4u; c++) {
        if ((r = uxn_row(c)) == MI_NONE)
            continue;
        k[c].label = mi_short(r);
        if (!menu_valued(r)) {
            str_cpy(k[c].value, r == ui.menu_sel ? UXN_WORD[1] : UXM_GO_K, sizeof k[c].value);
            continue;
        }
        str_cpy(k[c].value, menu_vname(r, menu_get(r)), sizeof k[c].value);
        n = menu_n(r);
        k[c].bar = (uint8_t)(n > 1u ? menu_pos(r) * 255u / (n - 1u) : 0u);
    }
}
/* the footer's right: "1 / 4 . NEXT CLICK" (MORE: "4 / 4" and its knobs, as a section's: ux_section.c uxs_foot) */
static void uxn_foot(const ux_knob_t k[4], char *b, uint32_t nb)
{
    char cnt[12];
    uint32_t p = uxn_page();
    if (ui.menu >= 2) {
        str_cpy(b, UXN_WORD[ui.menu == 2 ? 5 : 6], nb);
        return;
    }
    fmt_int(cnt, (int32_t)p + 1);
    str_cpy(cnt + str_len(cnt), UXN_WORD[2], sizeof cnt - str_len(cnt));
    fmt_int(cnt + str_len(cnt), MN_N);
    uxs_foot(uxn_where(), k, cnt, p + 1u < MN_N ? MN_NAME[p + 1u] : 0, b, nb);
}
/* the status line: a message; else what the setting turned or picked last does (CPU CORES, CPU CLOCK, RESTORE LAST: at
 * power-on, their hints say); MORE with none: what its controls do */
static const char *uxn_status(uint16_t *col)
{
    const char *h = menu_help(ui.menu_sel);
    if (ui.msg_t && ui.msg[0]) {
        *col = str_eq(ui.msg, UX_STOP) ? UX_ACCENT : UX_TEXT;
        return ui.msg;
    }
    *col = ux_knob_hot() >= 0 ? UX_TEXT : UX_MID;
    return h[0] ? h : uxn_page() == MN_MORE ? UXM_HINT : 0;
}
/* the header: the track and its sound, the transport (as a section's) */
static uint32_t uxn_head(int draw)
{
    char nm[24], tr[24];
    ux_trk_name(song.sel % NTRK, nm, sizeof nm, 1);
    ux_transport(tr, sizeof tr);
    if (draw)
        ux_header(song.sel % NTRK + 1u, nm + 3, tr);   /* (ux_trk_name's "T3 " left out: the header adds it) */
    return str_hash(str_hash(song.sel + 1u, nm), tr);
}
/* a page's panel: its four settings, a row each (the knob's number in its colour, the full name, the value, what it does), the
 * one turned last lit (BG2, its knob's colour at its edge) */
static void uxn_rows(void)
{
    uint32_t c, r;
    for (c = 0; c < 4u; c++) {
        int32_t y = 3 + (int32_t)c * UXN_ROW_P, ty = y + 5, vx;
        uint16_t kc = ux_knob_col(c), bg;
        int on;
        char kn[2] = {0, 0}, v[16];
        if ((r = uxn_row(c)) == MI_NONE)
            continue;
        on = r == ui.menu_sel;
        if (on) {
            cv_rect(8, y, 224, UXN_ROW_H, UX_BG2);
            cv_rect(8, y, 2, UXN_ROW_H, kc);
            bg = UX_BG2;
        } else
            bg = ux_under(16, ty + 4);
        GFX_HOOK_CELL(8, y, 232, y + UXN_ROW_H);
        kn[0] = (char)('1' + c);
        cv_text_on(16, ty, &UF_HDR, kn, kc, bg);
        cv_text_on(30, ty, &UF_HDR, MI_NAME[r], on ? UX_TEXT : UX_MID, bg);
        str_cpy(v, !menu_valued(r) ? (on ? UXN_WORD[1] : UXM_GO) : menu_vname(r, menu_get(r)), sizeof v);
        vx = 226 - text_w(&UF_HDR, v);
        cv_text_on(vx, ty, &UF_HDR, v, on ? kc : UX_TEXT, on ? bg : ux_under(vx, ty + 4));
        cv_text_on(30, ty + 15, &UF_HDR, menu_help(r), ux_mix(UX_BG, UX_MID, 70), on ? bg : ux_under(30, ty + 19));
    }
}
/* what the panel shows (a change: drawn again) */
static uint32_t uxn_sig(void)
{
    uint32_t i, sig = uxn_page() * 2654435761u ^ ui.menu_sel * 977u ^ ux_glow_eff(1) * 131u;
    if (uxn_page() == MN_MORE) {
        uint64_t m = uxm_mask(&UXM_MENU);
        sig = sig * 131u + ui.mn_sel * 7u + ui.mn_top + (uint32_t)(m ^ (m >> 32));
    }
    for (i = 0; i < MI_VALUES; i++)
        sig = sig * 33u + menu_get(i);
    return sig;
}
/* ABOUT, PROFILE: the document from ui.menu_scroll in two bands under the header, the scroll bar at its right */
static void uxn_doc(void)
{
    uint32_t pass;
    int32_t scroll = ui.menu_scroll, max = menu_scroll_max(), thumb = MENU_DOC_H * MENU_DOC_H / (max + MENU_DOC_H);
    if (thumb < 12)
        thumb = 12;
    for (pass = 0; pass < 2u; pass++) {
        cv_begin(240, UXN_BAND, UX_BG);
        cv_oy = -(int32_t)(pass * UXN_BAND);
        cv_scroll = 1;                                  /* (its text runs past a band's edge: the lint knows) */
        menu_document(-scroll, 1);
        cv_scroll = 0;
        cv_rect(235, 2, 3, MENU_DOC_H - 4, UX_LINE);   /* the scroll bar */
        cv_rect(235, 2 + (max ? scroll * (MENU_DOC_H - 4 - thumb) / max : 0), 3, thumb, UX_MID);
        cv_oy = 0;
        cv_blit_cells(0, UX_Y_KNOBS + pass * UXN_BAND);
    }
}
static void ux_menu_draw(void)
{
    static uint32_t head_sig, panel_sig, live_sig, live_ms, foot_sig;
    uint32_t all, sig, live = 0;
    ux_knob_t k[4];
    const char *st;
    uint16_t sc;
    char b[48];
    dc_frame(ui.force);
    all = dc_all;
    sig = uxn_head(0);
    if (all || sig != head_sig) {
        head_sig = sig;
        (void)uxn_head(1);
    }
    uxn_knobs(k);
    if (ui.menu >= 2) {                                 /* ABOUT / PROFILE: drawn on a change; the live figures paced */
        sig = ui.menu * 7u + ui.menu_scroll * 48611u + 1u;
        live = ui.menu == 3 ? prf_seq * 2971u + prf_have : (song.cpu_q8 >> 3) * 7741u + load_diag.peak * 3329u +
               load_diag.late * 1931u + load_diag.shed * 2671u + cpt_res * 13u;
        if (all || sig != panel_sig || (live != live_sig && fm1_ms - live_ms >= UXN_LIVE_MS)) {   /* (3c-2 task 9: by time
                                                         * alone, never frozen by LOAD LITE or a shed: the overload they show) */
            panel_sig = sig;
            live_sig = live;
            live_ms = fm1_ms;
            uxn_panels++;
            uxn_doc();
        }
    } else {
        ux_knobs(k, -1);
        sig = uxn_sig();
        if (all || sig != panel_sig) {
            panel_sig = sig;
            uxn_panels++;
            ux_panel_begin(1);
            if (uxn_page() == MN_MORE)
                ux_more_draw(&UXM_MENU, ui.mn_sel, ui.mn_top);
            else
                uxn_rows();
            ux_panel_end();
        }
        st = uxn_status(&sc);
        ux_status(st, sc);
    }
    uxn_foot(k, b, sizeof b);
    sig = str_hash(str_hash(3u, uxn_where()), b);
    if (all || sig != foot_sig) {
        foot_sig = sig;
        ux_footer(uxn_where(), b);
    }
}
