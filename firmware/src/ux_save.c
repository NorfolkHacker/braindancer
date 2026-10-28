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
 *   MORE     (spec 2.4; params.c UXM_SAVE) TOOLS: ERASE USER SLOT; SOUND: LOAD SLOT, ENGINE, FAVORITE, LIST (the inherited USER's
 *            LOAD, PRESETS' KNOB 2 .. 4: ui_input.c sv_view_turn).
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

static int uxv_knobs(const page_t *pg, ux_knob_t k[4])
{
    uint32_t total, cur, c;
    int used = up_used(ui.uslot);
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
static void uxv_picture(const page_t *pg)
{
    if (pg->graph == GR_SVSND)
        uxv_sound_picture();
}
/* what SOUND's picture is drawn from: the slot picked, the user bank (up_gen: a slot stored, erased, renamed), the track's sound */
static uint32_t uxv_sig(const page_t *pg)
{
    if (pg->graph != GR_SVSND)
        return 0;
    return (((ui.uslot * 131u + up_gen) * 8191u + TSEL->preset) * 31u + TSEL->eng_req * 7u + TSEL->user) * 131u + song.sel;
}
