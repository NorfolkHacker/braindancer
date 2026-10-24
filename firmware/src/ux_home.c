/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's HOME on the screen (spec 4.3). For now (piece 3a task 5) a placeholder for both pages, every piece one of
 * ux_widgets.c's: the header ACID FM, the knob row (TRACKS: the selected sound's four, as JIANT's HOME; VISUAL: M1..M4),
 * the page's name over the glow, the status line saying what HOME does next (spec 0: the screen says what the controls
 * do), the footer HOME . page and n / 2. Tasks 6 and 7 draw TRACKS and VISUAL here. Included after ux_widgets.c; called
 * by ui_draw.c ui_draw_body. */

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
static void ux_home_knobs(ux_knob_t k[4], char lbl[4][8])
{
    uint32_t c;
    for (c = 0; c < 4u; c++) {
        if (ux_home_page() == UXH_VISUAL) {             /* M1..M4, 0..100 % */
            lbl[c][0] = 'M'; lbl[c][1] = (char)('1' + c); lbl[c][2] = 0;
            fmt_int(k[c].value, macro_v[c] * 100 / 127);
            str_cpy(k[c].value + str_len(k[c].value), "%", sizeof k[c].value - str_len(k[c].value));
            k[c].bar = (uint8_t)(macro_v[c] * 255u / 127u);
        } else {                                        /* the sound's four (JIANT's HOME knobs) */
            int16_t *vp;
            const param_desc_t *d = home_param(c, &vp);
            const char *unit;
            char val[16];
            param_format(d, *vp, val, &unit);
            ux_lbl_cpy(lbl[c], d->label, 8);
            str_cpy(k[c].value, val, sizeof k[c].value);
            if (unit && unit[0] && str_len(k[c].value) + str_len(unit) < sizeof k[c].value)
                str_cpy(k[c].value + str_len(k[c].value), unit, sizeof k[c].value - str_len(k[c].value));
            k[c].bar = (uint8_t)(d->max > d->min ? (uint32_t)(clamp(*vp, d->min, d->max) - d->min) * 255u /
                                                   (uint32_t)(d->max - d->min) : 0u);
        }
        k[c].label = lbl[c];
    }
}
static void ux_home_draw(void)
{
    static uint32_t panel_sig;
    ux_knob_t k[4];
    char lbl[4][8];
    uint32_t p = ux_home_page(), all;
    const char *nm = CE_HOME[p].nm, *st = ui.msg_t && ui.msg[0] ? ui.msg : ux_home_hint();
    dc_frame(ui.force);
    all = dc_all;
    ux_home_knobs(k, lbl);
    if (all || panel_sig != p + 1u) {                   /* (the header, the panel, the footer: drawn when they change) */
        int32_t w = text_w(&UF_TTL, nm), x = (240 - w) / 2, y = (int32_t)(UX_H_PANEL - UF_TTL.h) / 2;
        panel_sig = p + 1u;
        ux_header(0, "ACID FM", 0);
        ux_panel_begin(3);
        ux_text_grad(x, y, &UF_TTL, nm, swap16(cv_px[(uint32_t)(y + UF_TTL.h / 2) * cv_w + (uint32_t)(x + w / 2)]));
        ux_panel_end();
        ux_footer(ux_home_where(), ux_home_count());
    }
    ux_knobs(k, ui.hot_t ? (int32_t)(ui.hot_col & 3u) : -1);
    ux_status(st, st == ui.msg ? UX_TEXT : UX_MID);
}
