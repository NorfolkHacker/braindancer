/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's SAVE pages on the screen (3c-2 spec 2; ux_save.h), drawn through ux_section.c's page (header, knob row, panel, status
 * line, footer) as GLO's are (ux_glo.c):
 *   SOUND    (spec 2.1; task 2: the inherited USER and PRESETS pages) KNOB 1 SLOT (ui.uslot, the 32 user slots), 2 PRESET (the track's
 *            engine's presets, ui.c preset_step: the inherited PRESETS KNOB 1), 3 NAME (a turn right: the slot's rename, ui_name.c
 *            name_rename; an empty slot: EMPTY SLOT), 4 SAVE (an action knob: right arms it, left drops it; it comes armed from
 *            another section, SAVE . OCT+ . OCT+ as before). OCT+ with SAVE armed: STOP TO SAVE while playing, the overwrite
 *            dialog over a used slot (CF_OVR_USER), else NAME (ui_input.c save_oct: the inherited USER's SAVE). The picture: the
 *            track's sound named on top, its engine's colour beside it; under it the 32 slots as a list scrolling with the pick
 *            (8 rows), each its number, its engine's colour and name (an empty slot its number alone, dim), the picked one lit.
 *            The status line: the slot, its sound and what OCT+ does ("U07 . DEEP SUB . OCT+ SAVES OVER IT"); PRESET turned, the
 *            sound loaded ("PRESET . DEEP SUB").
 *   SONG     (spec 2.2, layout A; task 3: the inherited SECTIONS and ARRANGE pages) KNOB 1 SECTION (A..D: G_SLOT; a row picked:
 *            its section too), 2 ROW (the song's rows, "3/6"), 3 BARS (the row's, 1..CHAIN_BARS), 4 SCENE (the inherited ARRANGE
 *            KNOB 4: right stores the state now, left clears it): ui_input.c save_knob. OCT+ a row after the one picked
 *            (song_row_insert). The picture: the four sections as blocks (the letter, the name, EMPTY dim, NOW on the one playing,
 *            the picked one lit), under them ARRANGE and the song's rows as chips ("A16", the picked one lit, the one playing
 *            underlined, a dot for a scene), the song's length and end ("72 BARS . LOOP") under them. The status line: the row
 *            and what OCT+ does ("ROW 3 . B . 16 BARS . OCT+ ADDS A ROW").
 *   MORE     (spec 2.4; params.c UXM_SAVE) SONG (task 3): END, DELETE ROW, CLEAR SONG; TOOLS: ERASE USER SLOT; SOUND: LOAD SLOT,
 *            ENGINE, FAVORITE, LIST (the inherited USER's LOAD, PRESETS' KNOB 2 .. 4, ARRANGE's END: ui_input.c sv_view_turn).
 * The picture is redrawn when what it shows changes (uxv_sig): none while nothing changes. Included after ux_glo.c, before
 * ux_section.c. */

#define UXV_TOP 4                /* the sound's line (y) */
#define UXV_RULE 21              /* the rule under it */
#define UXV_RY 25                /* the slots' list: the first row (y), the rows' pitch, a row's height, how many shown */
#define UXV_RP 15
#define UXV_RH 13
#define UXV_ROWS 8u
#define UXV_NX 14                /* a row's number, its engine's colour, its name (x) */
#define UXV_CX 42
#define UXV_TX 52
static const char *const UXV_LABEL[4] = {"SLOT", "PRESET", "NAME", "SAVE"};
/* the words the knob row, the status line and the picture are made of (tests/ux_msg_scan.py: every one has its glyphs in UF_LBL
 * and UF_HDR) */
static const char *const UXV_WORD[] = {"EDIT", "--", "OCT+", "EMPTY", " \xB7 ", "OCT+ SAVES OVER IT", "OCT+ SAVES HERE",
                                       "STOP TO SAVE", "NONE", "/"};
enum { UXV_W_EDIT, UXV_W_NONE, UXV_W_OCT, UXV_W_EMPTY, UXV_W_DOT, UXV_W_OVER, UXV_W_HERE, UXV_W_STOP, UXV_W_NOSND, UXV_W_OF };
/* (task 3) SONG's knob names and words (tests/ux_msg_scan.py, as above) */
static const char *const UXV_SLABEL[4] = {"SECTION", "ROW", "BARS", "SCENE"};
static const char *const UXV_SWORD[] = {"ROW ", " BAR", "S", "NO ROWS", "OCT+ ADDS A ROW", "16 ROWS", "STOP TO EDIT", "ARRANGE",
                                        "ON", "NOW", "USED"};
enum { UXV_S_ROW, UXV_S_BAR, UXV_S_S, UXV_S_NOROWS, UXV_S_ADD, UXV_S_FULL, UXV_S_STOP, UXV_S_ARR, UXV_S_ON, UXV_S_NOW, UXV_S_USED };

static int uxv_owns(void)
{
    uint32_t g;
    if (ui.home || ui.page >= NPAGES || PAGES[ui.page].fam != FAM_SAVE)
        return 0;
    g = PAGES[ui.page].graph;
    return g == GR_SVSND || g == GR_SVSONG || g == GR_SVPROJ || g == GR_MORE;
}
static uint16_t uxv_eng_col(uint32_t e) { return ux_knob_col(eng_rank(e) % 4u); }
static void uxv_cat(char *b, uint32_t n, const char *s) { str_cpy(b + str_len(b), s, n - str_len(b)); }
static int uxv_armed(void) { return ui.act == 4u; }     /* (SOUND's SAVE picked: OCT+ saves) */

static int uxv_song_knobs(ux_knob_t k[4]);
static int uxv_song_say(char *b, uint32_t nb);
static int uxv_knobs(const page_t *pg, ux_knob_t k[4])
{
    uint32_t total, cur, c;
    int used = up_used(ui.uslot);
    if (pg->graph == GR_SVSONG)
        return uxv_song_knobs(k);
    if (pg->graph != GR_SVSND)
        return 0;
    for (c = 0; c < 4u; c++)
        k[c].label = UXV_LABEL[c];
    up_slot_label(k[0].value, ui.uslot);                /* SLOT: "U07" */
    k[0].bar = (uint8_t)(ui.uslot * 255u / (UP_SLOTS - 1u));
    cur = preset_pos(&total);                           /* PRESET: "5/32" in the list (the inherited PRESETS' No.) */
    if (cur < total) {
        fmt_int(k[1].value, (int32_t)cur + 1);
        uxv_cat(k[1].value, sizeof k[1].value, UXV_WORD[UXV_W_OF]);
        fmt_int(k[1].value + str_len(k[1].value), (int32_t)total);
        if (text_w(&UF_VAL, k[1].value) > UX_COL_W - 4)   /* ("58/106" too wide for the column: the number alone) */
            fmt_int(k[1].value, (int32_t)cur + 1);
        k[1].bar = (uint8_t)(total > 1u ? cur * 255u / (total - 1u) : 255u);
    } else {
        str_cpy(k[1].value, UXV_WORD[total ? UXV_W_NONE : UXV_W_NOSND], sizeof k[1].value);
        k[1].off = 1;
    }
    str_cpy(k[2].value, UXV_WORD[used ? UXV_W_EDIT : UXV_W_NONE], sizeof k[2].value);   /* NAME: the slot's rename */
    k[2].off = (uint8_t)!used;
    str_cpy(k[3].value, UXV_WORD[uxv_armed() ? UXV_W_OCT : UXV_W_NONE], sizeof k[3].value);   /* SAVE: armed, OCT+ saves */
    k[3].bar = (uint8_t)(uxv_armed() ? 255u : 0u);
    k[3].off = (uint8_t)(!uxv_armed() || transport_busy());
    return 1;
}

/* the status line on SOUND: PRESET just turned, the sound ("PRESET . DEEP SUB"); else the slot, its sound and what OCT+ does with
 * SAVE armed ("U07 . DEEP SUB . OCT+ SAVES OVER IT", "U08 . EMPTY . OCT+ SAVES HERE", playing: STOP TO SAVE); too wide, its sound
 * left out */
static int uxv_say(const page_t *pg, char *b, uint32_t nb)
{
    char nm[16];
    uint32_t at;
    if (pg->graph == GR_SVSONG)
        return uxv_song_say(b, nb);
    if (pg->graph != GR_SVSND)
        return 0;
    if (ux_knob_hot() == 1) {
        sound_name(TSEL, nm);
        str_cpy(b, UXV_LABEL[1], nb);
        uxv_cat(b, nb, UXV_WORD[UXV_W_DOT]);
        uxv_cat(b, nb, nm[0] ? nm : UXV_WORD[UXV_W_NOSND]);
        return 1;
    }
    up_slot_label(b, ui.uslot);
    at = str_len(b);
    uxv_cat(b, nb, UXV_WORD[UXV_W_DOT]);
    if (up_used(ui.uslot)) {
        up_name(ui.uslot, nm);
        uxv_cat(b, nb, nm);
    } else
        uxv_cat(b, nb, UXV_WORD[UXV_W_EMPTY]);
    if (!uxv_armed())
        return 1;
    {
        const char *w = UXV_WORD[transport_busy() ? UXV_W_STOP : up_used(ui.uslot) ? UXV_W_OVER : UXV_W_HERE];
        uxv_cat(b, nb, UXV_WORD[UXV_W_DOT]);
        uxv_cat(b, nb, w);
        if (text_w(&UF_HDR, b) > 224) {                 /* (a long name: the slot and what OCT+ does) */
            b[at] = 0;
            uxv_cat(b, nb, UXV_WORD[UXV_W_DOT]);
            uxv_cat(b, nb, w);
        }
    }
    return 1;
}

/* ------------------------------------------------------------------------------------------------------- the picture --- */
static uint32_t uxv_top(void)                           /* the list's first row: the pick the fourth, the ends kept */
{
    uint32_t t = ui.uslot > 3u ? ui.uslot - 3u : 0u;
    return t + UXV_ROWS > UP_SLOTS ? UP_SLOTS - UXV_ROWS : t;
}
/* slot k's row at y: its number (UF_LBL), its engine's colour and its name (UF_HDR); empty, the number dim; the pick lit (BG2,
 * KNOB 1's colour at its edge) */
static void uxv_row(uint32_t k, int32_t y)
{
    int on = k == ui.uslot, used = up_used(k);
    uint16_t bg;
    int32_t ly = y + (UXV_RH - UF_LBL_CAP_H) / 2 - UF_LBL_CAP_Y, ty = y + (UXV_RH - UF_HDR_CAP_H) / 2 - UF_HDR_CAP_Y;
    char b[16];
    if (on) {
        cv_rect(8, y, 220, UXV_RH, UX_BG2);
        cv_rect(8, y, 2, UXV_RH, UX_ACID);
        bg = UX_BG2;
    } else
        bg = ux_under(UXV_NX, y + UXV_RH / 2);
    GFX_HOOK_CELL(8, y, 228, y + UXV_RH);
    up_slot_label(b, k);
    cv_text_on(UXV_NX, ly, &UF_LBL, b, on ? (used ? UX_TEXT : UX_MID) : used ? UX_MID : UX_DIM(UX_MID, 55), bg);
    if (!used)
        return;
    cv_rect(UXV_CX, y + 2, 4, UXV_RH - 4, uxv_eng_col(up_engine(k)));
    up_name(k, b);
    cv_text_on(UXV_TX, ty, &UF_HDR, b, on ? UX_TEXT : UX_MID, on ? bg : ux_under(UXV_TX, y + UXV_RH / 2));
}
static __attribute__((noinline)) void uxv_sound_picture(void)
{
    uint32_t i, top = uxv_top(), e = trk_eng(TSEL);
    int32_t x, h, th, ty;
    char nm[16];
    cv_rect(8, UXV_TOP + 1, 4, UF_HDR_CAP_H + 2, uxv_eng_col(e));   /* the track's sound: its engine's colour, its name, the engine */
    sound_name(TSEL, nm);
    GFX_HOOK_CELL(8, UXV_TOP - 2, 232, UXV_TOP + 14);
    x = 228 - text_w(&UF_LBL, ENGINES[e]->name);
    cv_text_on(x, UXV_TOP + 2, &UF_LBL, ENGINES[e]->name, UX_MID, ux_under(x, UXV_TOP + 6));
    cv_text_on(18, UXV_TOP - 1, &UF_HDR, nm[0] ? nm : UXV_WORD[UXV_W_NOSND], UX_TEXT, ux_under(18, UXV_TOP + 4));
    cv_rect(8, UXV_RULE, 224, 1, UX_LINE);
    for (i = 0; i < UXV_ROWS; i++)
        uxv_row(top + i, UXV_RY + (int32_t)i * UXV_RP);
    h = (int32_t)UXV_ROWS * UXV_RP - 2;                 /* the scroll: where the rows shown are in the 32 */
    th = h * (int32_t)UXV_ROWS / (int32_t)UP_SLOTS;
    ty = UXV_RY + (h - th) * (int32_t)top / (int32_t)(UP_SLOTS - UXV_ROWS);
    cv_rect(233, UXV_RY, 2, h, UX_LINE);
    cv_rect(233, ty, 2, th, UX_MID);
}
/* ------------------------------------------------------------------------------------------------------------ SONG --- */
#define UXV_BY 4                 /* the sections: the blocks' top, height, pitch, width */
#define UXV_BH 44
#define UXV_BP 57
#define UXV_BW 53
#define UXV_AY 56                /* ARRANGE's word (its caps' top) */
#define UXV_CY 66                /* the chips: the first row's top, the rows' pitch, a chip's height, pitch, width; 8 a row */
#define UXV_CP 24
#define UXV_CH 20
#define UXV_CXP 28
#define UXV_CW 26
#define UXV_LY 8                 /* the song's length and end: its caps' top this far under the last row of chips */
static uint32_t uxv_srow(void)                          /* the row picked (past the last, a row deleted: the last; none: 0) */
{
    return ui.song_row < chain_config.count ? ui.song_row : chain_config.count ? chain_config.count - 1u : 0u;
}
static uint32_t uxv_bars(void)                          /* the song's bars */
{
    uint32_t i, n = 0;
    for (i = 0; i < chain_config.count && i < CHAIN_ROWS; i++)
        n += chain_config.row[i].bars;
    return n;
}
static int uxv_song_knobs(ux_knob_t k[4])
{
    uint32_t c, n = chain_config.count, r = uxv_srow(), s = (uint32_t)(song.g[G_SLOT] - 1) & 3u;
    const chain_row_t *w = &chain_config.row[r];
    for (c = 0; c < 4u; c++)
        k[c].label = UXV_SLABEL[c];
    k[0].value[0] = (char)('A' + s);                    /* SECTION: "B" */
    k[0].value[1] = 0;
    k[0].bar = (uint8_t)(s * 255u / 3u);
    if (!n) {                                           /* (no row: ROW, BARS, SCENE none) */
        for (c = 1; c < 4u; c++) {
            str_cpy(k[c].value, UXV_WORD[UXV_W_NONE], sizeof k[c].value);
            k[c].off = 1;
        }
        return 1;
    }
    fmt_int(k[1].value, (int32_t)r + 1);                /* ROW: "3/6" */
    uxv_cat(k[1].value, sizeof k[1].value, UXV_WORD[UXV_W_OF]);
    fmt_int(k[1].value + str_len(k[1].value), (int32_t)n);
    k[1].bar = (uint8_t)(n > 1u ? r * 255u / (n - 1u) : 255u);
    fmt_int(k[2].value, w->bars);                       /* BARS: "16" */
    k[2].bar = (uint8_t)(w->bars * 255u / CHAIN_BARS);
    c = song_idx.scene[song_cur][r].on != 0;            /* SCENE: ON, or none */
    str_cpy(k[3].value, c ? UXV_SWORD[UXV_S_ON] : UXV_WORD[UXV_W_NONE], sizeof k[3].value);
    k[3].bar = (uint8_t)(c ? 255u : 0u);
    k[3].off = (uint8_t)!c;
    return 1;
}
/* SONG's status line: the row picked, its section and bars, what OCT+ does ("ROW 3 . B . 16 BARS . OCT+ ADDS A ROW"; no rows: "NO
 * ROWS . OCT+ ADDS A ROW"; full: 16 ROWS; the song playing: STOP TO EDIT) */
static int uxv_song_say(char *b, uint32_t nb)
{
    uint32_t r = uxv_srow();
    const chain_row_t *w = &chain_config.row[r];
    char t[4];
    if (!chain_config.count)
        str_cpy(b, UXV_SWORD[UXV_S_NOROWS], nb);
    else {
        str_cpy(b, UXV_SWORD[UXV_S_ROW], nb);
        fmt_int(b + str_len(b), (int32_t)r + 1);
        uxv_cat(b, nb, UXV_WORD[UXV_W_DOT]);
        t[0] = (char)('A' + (w->slot & 3u));
        t[1] = 0;
        uxv_cat(b, nb, t);
        uxv_cat(b, nb, UXV_WORD[UXV_W_DOT]);
        fmt_int(b + str_len(b), w->bars);
        uxv_cat(b, nb, UXV_SWORD[UXV_S_BAR]);
        if (w->bars != 1u)
            uxv_cat(b, nb, UXV_SWORD[UXV_S_S]);
    }
    uxv_cat(b, nb, UXV_WORD[UXV_W_DOT]);
    uxv_cat(b, nb, UXV_SWORD[chain_busy() ? UXV_S_STOP : chain_config.count >= CHAIN_ROWS ? UXV_S_FULL : UXV_S_ADD]);
    return 1;
}
/* section i's block: its colour at the edge, its letter, NOW playing, its name (EMPTY dim); the one SECTION picks lit (BG2, KNOB 1's
 * colour on top) */
static void uxv_section(uint32_t i)
{
    int32_t x = 8 + (int32_t)i * UXV_BP, y = UXV_BY;
    int on = i == (((uint32_t)song.g[G_SLOT] - 1u) & 3u), used = graph_project_used(i);
    const char *nm = used ? (graph_project_name(i)[0] ? graph_project_name(i) : UXV_SWORD[UXV_S_USED]) : UXV_WORD[UXV_W_EMPTY];
    uint16_t bg;
    char l[2] = {(char)('A' + i), 0};
    if (on) {
        cv_rect(x, y, UXV_BW, UXV_BH, UX_BG2);
        cv_rect(x, y, UXV_BW, 2, UX_ACID);
        bg = UX_BG2;
    } else {
        cv_frame(x, y, UXV_BW, UXV_BH, UX_LINE);
        bg = ux_under(x + 8, y + UXV_BH / 2);
    }
    GFX_HOOK_CELL(x, y, x + UXV_BW, y + UXV_BH);
    cv_rect(x + 3, y + 5, 3, UXV_BH - 10, used ? sec_col(i) : UX_DIM(sec_col(i), 70));
    cv_text_on(x + 10, y + 7 - UF_VAL_CAP_Y, &UF_VAL, l, used ? UX_TEXT : UX_MID, bg);
    if (used && live.cur == (int8_t)i)                  /* (the section playing) */
        cv_text_on(x + UXV_BW - 4 - text_w(&UF_LBL, UXV_SWORD[UXV_S_NOW]), y + 9 - UF_LBL_CAP_Y, &UF_LBL, UXV_SWORD[UXV_S_NOW],
                   on ? UX_ACID : UX_MID, bg);
    cv_clip_text(x + 10, y + 29 - UF_LBL_CAP_Y, &UF_LBL, nm, used ? (on ? UX_TEXT : UX_MID) : UX_DIM(UX_MID, 55), bg, UXV_BW - 13);
}
/* row i's chip: its section's colour (picked: full, framed; else dimmed), "A16", a dot for its scene, a line under the one playing */
static void uxv_chip(uint32_t i)
{
    const chain_row_t *w = &chain_config.row[i];
    int32_t x = 8 + (int32_t)(i % 8u) * UXV_CXP, y = UXV_CY + (int32_t)(i / 8u) * UXV_CP;
    int on = i == uxv_srow();
    uint16_t c = on ? sec_col(w->slot) : UX_DIM(sec_col(w->slot), 62), ink = on ? UX_BG : UX_TEXT;
    char b[6];
    b[0] = (char)('A' + (w->slot & 3u));
    fmt_int(b + 1, w->bars);
    cv_rect(x, y, UXV_CW, UXV_CH, c);
    if (on)
        cv_frame(x - 1, y - 1, UXV_CW + 2, UXV_CH + 2, UX_TEXT);
    GFX_HOOK_CELL(x, y, x + UXV_CW, y + UXV_CH);
    cv_text_in(x, y + (UXV_CH - UF_LBL_CAP_H) / 2 - UF_LBL_CAP_Y, UXV_CW, &UF_LBL, b, ink, c);
    if (song_idx.scene[song_cur][i].on)                 /* a scene: a dot */
        cv_rect(x + UXV_CW - 5, y + 2, 3, 3, ink);
    if (chain.running && chain.row == i)                /* the row playing */
        cv_rect(x, y + UXV_CH + 1, UXV_CW, 2, UX_TEXT);
}
static __attribute__((noinline)) void uxv_song_picture(void)
{
    uint32_t i, e = sv_get(ID_SV_END);
    int32_t ly = UXV_CY + (int32_t)((chain_config.count + 7u) / 8u) * UXV_CP + UXV_LY - (chain_config.count ? 4 : 0);
    char b[24];
    for (i = 0; i < 4u; i++)
        uxv_section(i);
    GFX_HOOK_CELL(8, UXV_AY - 2, 232, UXV_AY + 8);
    cv_text_on(8, UXV_AY - UF_LBL_CAP_Y, &UF_LBL, UXV_SWORD[UXV_S_ARR], UX_MID, ux_under(8, UXV_AY + 3));
    for (i = 0; i < chain_config.count && i < CHAIN_ROWS; i++)
        uxv_chip(i);
    if (chain_config.count) {                           /* "72 BARS . LOOP" */
        fmt_int(b, (int32_t)uxv_bars());
        uxv_cat(b, sizeof b, UXV_SWORD[UXV_S_BAR]);
        uxv_cat(b, sizeof b, UXV_SWORD[UXV_S_S]);
        uxv_cat(b, sizeof b, UXV_WORD[UXV_W_DOT]);
        uxv_cat(b, sizeof b, N_SV_END[e < 9u ? e : 0u]);
    } else
        str_cpy(b, UXV_SWORD[UXV_S_NOROWS], sizeof b);
    GFX_HOOK_CELL(8, ly - 2, 232, ly + 10);
    cv_text_on(8, ly - UF_HDR_CAP_Y, &UF_HDR, b, chain_config.count ? UX_MID : UX_DIM(UX_MID, 55), ux_under(8, ly + 4));
}
static void uxv_picture(const page_t *pg)
{
    if (pg->graph == GR_SVSND)
        uxv_sound_picture();
    else if (pg->graph == GR_SVSONG)
        uxv_song_picture();
}
/* what SOUND's picture is drawn from: the slot picked, the user bank (up_gen: a slot stored, erased, renamed), the track's sound;
 * (task 3) SONG's: the section picked and playing, the sections' names, the rows, their scenes, the row picked and playing, END */
static uint32_t uxv_sig(const page_t *pg)
{
    uint32_t h, i;
    if (pg->graph == GR_SVSONG) {
        h = (((uint32_t)song.g[G_SLOT] * 131u + ui.song_row) * 8191u + chain_config.count) * 31u + SONG_END(&chain_config);
        h = h * 131u + (uint32_t)(live.cur + 1) + song_cur * 7u + graph_pname_sig;
        for (i = 0; i < 4u; i++)
            h = h * 3u + (uint32_t)graph_project_used(i);
        for (i = 0; i < chain_config.count && i < CHAIN_ROWS; i++)
            h = (h ^ (chain_config.row[i].slot + 4u * chain_config.row[i].bars + 1024u * song_idx.scene[song_cur][i].on)) * 16777619u;
        return h ^ (chain.running ? (chain.row + 1u) * 104729u : 0u);
    }
    if (pg->graph != GR_SVSND)
        return 0;
    return (((ui.uslot * 131u + up_gen) * 8191u + TSEL->preset) * 31u + TSEL->eng_req * 7u + TSEL->user) * 131u + song.sel;
}
