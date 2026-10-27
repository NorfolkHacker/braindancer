/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's held layers (ux_layer.h; 3b spec 2.6; 3c-1 tasks 5, 9 spec 2.6, 3.4): EDIT, LFO's MACRO, FX's performance layer, SCL,
 * SEQ TOOLS, REC and GLO (the mutes, the solos, KNOB 1..4 the levels) in the 3a look. The keys,
 * the knobs, the LEDs, the gesture and the latches are ui_layer.c's (ly_keyname names each key and its state, layer_knob turns
 * the knobs, layer_state lists what is on): only the drawing is here. The screen as a section page's (ux_section.c): the header
 * (T3 . the sound, the transport), the knob row (the layer's four in KNOB 1..4's colours, plain names: FX's macros or the
 * glitcher's knobs, MACRO 1..4, EDIT's the engine's first page as EDIT shows it, SCL's ROOT SCALE ROTATE SEQ PITCH), the panel
 * (MAP: the keyboard F3 .. G5 in small, each key in its state's colour, then the keys that do something listed by name in
 * keyboard order; SCL: the keyboard large, the root ACID, the scale's notes lit; STATE: what is on, as chips; ALGORITHM between
 * them), the status line (a message; a knob's readout; else the layer and what its keys and OCT do), the footer (the layer and
 * how it is held; ALGORITHM's next view). Included after ux_section.c (uxs_knobs, uxs_val) and ui_layer.c (its tables). */

static int ux_layer_owns(uint32_t l)
{
    return l == LAYER_EDIT || l == LAYER_MACRO || l == LAYER_FX || l == LAYER_SCL || l == LAYER_SEQ || l == LAYER_REC ||
           l == LAYER_GLO;
}

/* FX's KNOB 1..4, the performance macros (perform.c perf_m), by what they do (spec 0 rule 4; one name a meaning): STEP LOOP
 * (JIANT's STEPS: ui_live.c loops the steps the playhead is in, 1/2 1/4 1/8 of the pattern or one step; STEPS is LFO's
 * modulation sequence), INTERVAL (JIANT's TRANSP: every sequence an interval up or down, seq.c pm_trn; TRANSPOSE is SCL's,
 * one track's semitones), MUTATE, DJ FILTER (JIANT's FILTER: the DJ FILTER's low-pass while it is up, fx.c djf_v) */
static const char *const UXL_PM[4] = {"STEP LOOP", "INTERVAL", "MUTATE", "DJ FILTER"};
static const char *const UXL_LOOP[5] = {"OFF", "1/2", "1/4", "1/8", "1 STEP"};
static const char *const UXL_IVL[8] = {"OFF", "+2ND", "+3RD", "+4TH", "+5TH", "+6TH", "+OCT", "-OCT"};
/* .. while a glitcher's key is down, its four (ui_layer.c GX_K: GLITCH, TAPE STOP, SHUFFLE, FREEZE; JIANT's DENS VARI) */
static const char *const UXL_GX[16] = {"DENSITY", "PITCH", "SIZE", "VARIANT", "SCRATCH", "TIME", "CURVE", "",
                                       "AMOUNT", "SIZE", "SPEED", "VARIANT", "DENSITY", "PITCH", "SPRAY", "SIZE"};
static const char *const UXL_LV[NTRK] = {"T1 LEVEL", "T2 LEVEL", "T3 LEVEL", "T4 LEVEL", "T5 LEVEL", "T6 LEVEL"};   /* GLO's */
static const char *const UXL_MAC[4] = {"MACRO 1", "MACRO 2", "MACRO 3", "MACRO 4"};   /* (the matrix's SOURCE names) */
/* the keys' words (ly_keyname's, JIANT's) in plain ones on the MAP; the others as they are */
static const char *const UXL_KEYJ[17] = {"RPT 1/8", "RPT 1/16", "RPT 1/32", "OCT-", "OCT+", "1/2 TEMPO", "DEC-", "DEC+",
                                         "STUT 1/8", "STUT 1/16", "ATK+", "TRK ALL", "TRK SYN", "TRK DRM", "INIT+KIT", "RCL+KIT",
                                         "UNMUTE"};
static const char *const UXL_KEYP[17] = {"REPEAT 1/8", "REPEAT 1/16", "REPEAT 1/32", "OCT DOWN", "OCT UP", "HALF TEMPO",
                                         "DECAY -", "DECAY +", "STUTTER 1/8", "STUTTER 1/16", "ATTACK +", "ON ALL", "ON SYNTHS",
                                         "ON DRUMS", "INIT + KIT", "RECALL + KIT", "UNMUTE ALL"};
/* the status line's words when nothing else is said: the layer, what its keys and OCT do */
enum { UXL_S_FX, UXL_S_LATCH, UXL_S_LOCK, UXL_S_EDIT, UXL_S_EDRM, UXL_S_SCL, UXL_S_MAC, UXL_S_MLAT, UXL_S_TOOLS, UXL_S_TDRM,
       UXL_S_TBUSY, UXL_S_REC, UXL_S_GLO, UXL_S_GUSB, UXL_S_GTRS, UXL_S_N };
static const char *const UXL_SAY[UXL_S_N] = {"FX: KEYS PLAY EFFECTS \xB7 BLACK KEYS MUTE", "FX LATCH: KEYS ON / OFF \xB7 OCT- ALL OFF",
                                             "FX LOCKED: PRESS FX TO UNLOCK", "EDIT: F3 INIT \xB7 G3 RECALL \xB7 SELECT DICE",
                                             "EDIT: SELECT DICE \xB7 BLACK KEYS MUTE",
                                             "SCL: A KEY SETS THE ROOT \xB7 NO OCTAVE", "LFO: F3 DICE \xB7 G3 CLEAR \xB7 SELECT DICE",
                                             "MACROS LATCHED: PRESS LFO TO CLOSE",
                                             "SEQ TOOLS: WHITE KEYS \xB7 REC CLEARS ALL",
                                             "SEQ TOOLS: LANES ON BLACK \xB7 REC CLEARS ALL",
                                             "STOP TO EDIT \xB7 REC CLEARS ALL",
                                             "REC: F3 CLEAR \xB7 G3 CLICK \xB7 KNOB 1 CLICK",
                                             "BLACK MUTE \xB7 WHITE SOLO \xB7 PLAY RESTART",   /* (3c-1 task 9) */
                                             "CLOCK: USB \xB7 NO TAP \xB7 NO RESTART",
                                             "CLOCK: TRS \xB7 NO TAP \xB7 NO RESTART"};
static const char *const UXL_GXSAY[4] = {"GLITCH HELD: ITS FOUR KNOBS", "TAPE STOP HELD: KNOB 1 SCRATCHES",
                                         "SHUFFLE HELD: ITS FOUR KNOBS", "FREEZE HELD: ITS FOUR KNOBS"};
/* the footer: the layer and how it is open; ALGORITHM's next view (MAP, STATE) */
enum { UXL_W_FX, UXL_W_LATCH, UXL_W_LOCK, UXL_W_EDIT, UXL_W_SCL, UXL_W_MAC, UXL_W_MLAT, UXL_W_TOOLS, UXL_W_REC, UXL_W_GLO, UXL_W_N };
static const char *const UXL_WHERE[UXL_W_N] = {"FX \xB7 HOLD", "FX \xB7 LATCH", "FX \xB7 LOCKED", "EDIT \xB7 HELD", "SCL \xB7 HELD",
                                               "LFO \xB7 MACROS", "LFO \xB7 LATCHED", "SEQ \xB7 TOOLS", "REC \xB7 HELD",
                                               "GLO \xB7 HELD"};
static const char *const UXL_ALGO[2] = {"1 / 2 \xB7 ALGORITHM: STATE", "2 / 2 \xB7 ALGORITHM: MAP"};
/* (3b task 9 fix) layer l's view: 1 STATE, 0 MAP (its own: ui.uxlv; SAVE keeps ui.lylv) */
static uint32_t uxl_state(uint32_t l) { return (ui.uxlv >> l) & 1u; }
static uint32_t uxl_panels;                             /* the panels drawn (tests/ui_test.c: one a change, none idle) */

/* KNOB 1..4 of layer l as the knob row shows them */
static void uxl_knobs(uint32_t l, ux_knob_t k[4])
{
    uint32_t c;
    for (c = 0; c < 4u; c++) {
        k[c].label = "";
        k[c].value[0] = 0;
        k[c].bar = 0;
        k[c].off = 0;
    }
    if (l == LAYER_FX && perf_gfocus() != PF_N) {       /* a glitcher's key down: its four */
        uint32_t x = perf_gx(perf_gfocus());
        for (c = 0; c < 4u; c++) {
            param_desc_t d = gx_desc(x, c);
            int32_t v = perf_gp[x][c];
            k[c].label = UXL_GX[x * 4u + c];
            if (!d.label[0])
                continue;
            if (!d.max) {                               /* (TAPE's SCRATCH: the hand on the record, once stopped) */
                str_cpy(k[c].value, pf.mode == BM_TAPE && pf.state ? "TURN" : "-", sizeof k[c].value);
                continue;
            }
            uxs_val(&d, v, k[c].value, sizeof k[c].value);
            k[c].bar = (uint8_t)clamp((v - d.min) * 255 / (d.max - d.min), 0, 255);
        }
    } else if (l == LAYER_FX) {                         /* the macros, OFF at the left */
        for (c = 0; c < 4u; c++) {
            uint32_t m = perf_m[c];
            k[c].label = UXL_PM[c];
            k[c].bar = (uint8_t)(m * 255u / PM_MAX[c]);
            k[c].off = (uint8_t)(c == 1u ? !(pm_trn() % 8u) : !m);   /* (3b task 9 fix: OFF dim, as JIANT's cards) */
            if (!c)
                str_cpy(k[c].value, UXL_LOOP[(m + 2u) / 3u % 5u], sizeof k[c].value);
            else if (c == 1u)
                str_cpy(k[c].value, UXL_IVL[pm_trn() % 8u], sizeof k[c].value);
            else if (!m)
                str_cpy(k[c].value, UXL_LOOP[0], sizeof k[c].value);
            else {
                fmt_int(k[c].value, (int32_t)m);
                str_cpy(k[c].value + str_len(k[c].value), "%", sizeof k[c].value - str_len(k[c].value));
            }
        }
    } else if (l == LAYER_MACRO) {                      /* M1..M4, 0 .. 100 % */
        for (c = 0; c < 4u; c++) {
            k[c].label = UXL_MAC[c];
            fmt_int(k[c].value, macro_v[c] * 100 / 127);
            str_cpy(k[c].value + str_len(k[c].value), "%", sizeof k[c].value - str_len(k[c].value));
            k[c].bar = (uint8_t)(macro_v[c] * 255u / 127u);
        }
    } else if (l == LAYER_GLO) {                        /* T1..T4 LEVEL (T5, T6 selected: T3..T6), as HOME's window; a muted or
                                                         * unsoloed track's dim */
        for (c = 0; c < 4u; c++) {
            uint32_t t = c + trk_base4();
            int16_t v = trk[t].p[P_LEVEL];
            k[c].label = UXL_LV[t % NTRK];
            uxs_val(&TP[P_LEVEL], v, k[c].value, sizeof k[c].value);
            k[c].bar = (uint8_t)clamp((v - TP[P_LEVEL].min) * 255 / (TP[P_LEVEL].max - TP[P_LEVEL].min), 0, 255);
            k[c].off = (uint8_t)!glo_sounding(t);
        }
    } else if (l == LAYER_REC) {                        /* K1 CLICK (OFF REC ON); K2..K4 are nobody's (ui_layer.c RL_N) */
        uint32_t v = rp_get(RL_F[0]);
        k[0].label = RL_LABEL[0];
        str_cpy(k[0].value, rl_name(0, v), sizeof k[0].value);
        k[0].bar = (uint8_t)(v * 127u);
        k[0].off = (uint8_t)!v;
    } else if (l == LAYER_EDIT || l == LAYER_SCL || l == LAYER_SEQ) {   /* the page's four as its section names them (EDIT:
                                                         * EDIT 1, the engine's first; SCL: LY_SCL; SEQ TOOLS: LY_SEQ, the
                                                         * PATTERN page's), wherever the page is */
        uint8_t h = ui.home, pg = ui.page, nm[sizeof ui.nmem];
        memcpy(nm, ui.nmem, sizeof nm);
        ui.home = 0;
        ui.page = (uint8_t)page_first(LAYERS[l].fam);
        page_over = ly_page(l);
        uxs_knobs(cur_page(), k);
        page_over = 0;
        ui.home = h;
        ui.page = pg;
        memcpy(ui.nmem, nm, sizeof nm);
    }
}

/* the status line: a message; the knob just turned, its name and value; else what the layer's keys and OCT do */
static const char *uxl_status(uint32_t l, const ux_knob_t k[4], uint16_t *col)
{
    static char b[48];
    int32_t h = ux_knob_hot();
    if (ui.msg_t && ui.msg[0]) {
        *col = str_eq(ui.msg, UX_STOP) ? UX_ACCENT : UX_TEXT;
        return ui.msg;
    }
    if (h >= 0 && k[h].label[0]) {
        str_cpy(b, k[h].label, sizeof b);
        str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
        str_cpy(b + str_len(b), k[h].value, sizeof b - str_len(b));
        *col = UX_TEXT;
        return b;
    }
    *col = UX_MID;
    if (l == LAYER_FX)
        return perf_gfocus() != PF_N ? UXL_GXSAY[perf_gx(perf_gfocus()) % 4u] : fx_lock && !ui.ly ? UXL_SAY[UXL_S_LOCK] :
               perf_latch_on ? UXL_SAY[UXL_S_LATCH] : UXL_SAY[UXL_S_FX];
    if (l == LAYER_EDIT)
        return UXL_SAY[drum_track(TSEL) ? UXL_S_EDRM : UXL_S_EDIT];
    if (l == LAYER_SCL)
        return UXL_SAY[UXL_S_SCL];
    if (l == LAYER_REC)
        return UXL_SAY[UXL_S_REC];
    if (l == LAYER_GLO)                                 /* (an outside clock: TAP and RESTART refuse, tempo_ext / layer_play) */
        return UXL_SAY[song.g[G_CLOCK] ? song.g[G_CLOCK] == 1 ? UXL_S_GUSB : UXL_S_GTRS : UXL_S_GLO];
    if (l == LAYER_SEQ)
        return UXL_SAY[chain_busy() ? UXL_S_TBUSY : drum_track(TSEL) ? UXL_S_TDRM : UXL_S_TOOLS];
    return UXL_SAY[layer_open() ? UXL_S_MAC : UXL_S_MLAT];
}
static const char *uxl_where(uint32_t l)
{
    if (l == LAYER_FX)
        return UXL_WHERE[fx_lock && !ui.ly ? UXL_W_LOCK : perf_latch_on ? UXL_W_LATCH : UXL_W_FX];
    if (l == LAYER_SEQ || l == LAYER_REC || l == LAYER_GLO)
        return UXL_WHERE[l == LAYER_SEQ ? UXL_W_TOOLS : l == LAYER_REC ? UXL_W_REC : UXL_W_GLO];
    return UXL_WHERE[l == LAYER_EDIT ? UXL_W_EDIT : l == LAYER_SCL ? UXL_W_SCL : layer_open() ? UXL_W_MAC : UXL_W_MLAT];
}

/* a key's colour by its state (ly_keyname: ACID the value now, ACCENT held, QUEUED waiting for its beat, VIOLET muted, dim
 * unavailable, a grey one that does something; none: dark) */
static uint16_t uxl_fill(uint32_t st, int named, int blk)
{
    return st == LS_SEL ? UX_ACID : st == LS_HELD ? UX_ACCENT : st == LS_WAIT ? UX_QUEUED : st == LS_MUTE ? UX_VIOLET :
           !named ? (blk ? UX_EMPTY : UX_BG2) : st == LS_DIM ? UX_LINE : ux_mix(UX_BG, UX_MID, blk ? 40 : 55);
}
static uint16_t uxl_ink(uint32_t st)                    /* a key's name in the list */
{
    return st == LS_SEL ? UX_ACID : st == LS_HELD ? UX_ACCENT : st == LS_WAIT ? UX_QUEUED : st == LS_MUTE ? UX_VIOLET_LT :
           st == LS_DIM ? ux_mix(UX_BG, UX_MID, 55) : UX_TEXT;
}
static const char *uxl_word(const char *nm)             /* a key's name in plain words */
{
    static char b[16];
    uint32_t i;
    if (nm[0] == 'C' && nm[1] == 'L' && nm[2] == 'R' && nm[3] == ' ') {   /* SEQ TOOLS: CLR AUTO, CLR STEP, CLR BD: CLEAR .. */
        str_cpy(b, "CLEAR ", sizeof b);
        str_cpy(b + 6, nm + 4, sizeof b - 6);
        return b;
    }
    for (i = 0; i < NELEM(UXL_KEYJ); i++)
        if (str_eq(nm, UXL_KEYJ[i]))
            return UXL_KEYP[i];
    return nm;
}
/* key n (0..26 in keyboard order, F3 .. G5): black (*blk) or white, its note name ("F#3"); its place among those (returned) */
static uint32_t uxl_key(uint32_t n, int *blk, char *nt)
{
    uint32_t i = 0, j = 0, w, p = 0;
    *blk = 0;
    for (w = 0; w < 16u; w++) {
        if (j < 11u && PK_BLK[j] == w) {                /* (black key j sits just left of white key PK_BLK[j]) */
            if (i++ == n) {
                *blk = 1;
                p = j;
                break;
            }
            j++;
        }
        if (i++ == n) {
            p = w;
            break;
        }
    }
    if (*blk) {
        nt[0] = B_NOTE[p];
        nt[1] = '#';
        nt[2] = (char)(PK_BLK[p] - 1u < 4u ? '3' : PK_BLK[p] - 1u < 11u ? '4' : '5');
        nt[3] = 0;
    } else {
        nt[0] = W_NOTE[p];
        nt[1] = (char)(p < 4u ? '3' : p < 11u ? '4' : '5');
        nt[2] = 0;
    }
    return p;
}
#define UXL_KB_Y 3               /* MAP's small keyboard: its white keys' top, height, pitch from x 8 */
#define UXL_KB_H 30
#define UXL_KB_W 14
#define UXL_LS_Y 40              /* .. the list under it: its first row, a row's pitch, rows a column, a column's width */
#define UXL_LS_P 12
#define UXL_LS_R 9u
#define UXL_LS_W 75
#define UXL_LS_NM 19             /* .. a name's place after its note (the note: "G#3", 16 px at most) */
static void uxl_map(uint32_t l)
{
    char nm[12], nt[4];
    uint32_t i, n = 0, st;
    int blk;
    if (l == LAYER_SCL) {                               /* every key its note: the keyboard large, as SCL > KEY's (ux_section.c
                                                         * uxs_scl_key: the root ACID, the scale's notes SLIDE), its letters */
        for (i = 0; i < 16u; i++) {
            int32_t x0 = 4 + (int32_t)i * 232 / 16, w = 4 + (int32_t)(i + 1u) * 232 / 16 - x0 - 1;
            uint16_t f;
            st = ly_keyname(l, 0, i, nm);
            f = st == LS_SEL ? UX_ACID : st == LS_DIM ? UX_LINE : UX_SLIDE;
            cv_rect(x0, 3, w, 144, f);
            nt[0] = W_NOTE[i];
            nt[1] = 0;
            GFX_HOOK_ALIGN(x0, 0, x0 + w, 0, AL_H, "SCL layer key's note centred");
            cv_text_in(x0, 132, w, &UF_LBL, nt, st == LS_DIM ? UX_MID : UX_BG, f);
        }
        for (i = 0; i < 11u; i++) {
            int32_t x0 = 4 + (int32_t)PK_BLK[i] * 232 / 16 - 5;
            st = ly_keyname(l, 1, i, nm);
            cv_rect(x0 - 1, 3, 12, 85, UX_BG);
            cv_rect(x0, 3, 10, 84, st == LS_SEL ? UX_ACID : st == LS_DIM ? UX_BG2 : ux_mix(UX_BG, UX_SLIDE, 75));
        }
        return;
    }
    for (i = 0; i < 16u; i++) {                         /* the small keyboard: white keys, then the black over them */
        st = ly_keyname(l, 0, i, nm);
        cv_rect(8 + (int32_t)i * UXL_KB_W, UXL_KB_Y, UXL_KB_W - 1, UXL_KB_H, uxl_fill(st, nm[0] != 0, 0));
    }
    for (i = 0; i < 11u; i++) {
        int32_t x0 = 8 + (int32_t)PK_BLK[i] * UXL_KB_W - 5;
        st = ly_keyname(l, 1, i, nm);
        cv_rect(x0 - 1, UXL_KB_Y, 12, 19, UX_BG);
        cv_rect(x0, UXL_KB_Y, 10, 18, uxl_fill(st, nm[0] != 0, 1));
    }
    for (i = 0; i < 27u; i++) {                         /* the keys that do something, in keyboard order, down the columns */
        uint32_t p = uxl_key(i, &blk, nt);
        int32_t x, y;
        st = ly_keyname(l, blk, p, nm);
        if (!nm[0])
            continue;
        if (n >= 3u * UXL_LS_R)
            break;
        x = 8 + (int32_t)(n / UXL_LS_R) * UXL_LS_W;
        y = UXL_LS_Y + (int32_t)(n % UXL_LS_R) * UXL_LS_P;
        n++;
        GFX_HOOK_CELL(x, y, x + UXL_LS_W - 1, y + UXL_LS_P);
        cv_text_on(x, y + 1, &UF_LBL, nt, UX_MID, ux_under(x, y + 5));
        cv_text_on(x + UXL_LS_NM, y + 1, &UF_LBL, uxl_word(nm), uxl_ink(st), ux_under(x + UXL_LS_NM, y + 5));
    }
}

/* STATE's chips (ui_layer.c layer_state, lst_chip): two a row, a mark in ACID before the ones on; an effect in plain words */
static void uxl_chip(uint32_t n, const char *s, int on)
{
    int32_t x = 8 + (int32_t)(n & 1u) * 116, y = 6 + (int32_t)(n >> 1) * 18;
    if (n >= 16u)
        return;
    GFX_HOOK_CELL(x, y, x + 112, y + 16);
    if (on)
        cv_rect(x, y + 4, 2, 9, UX_ACID);
    cv_clip_text(x + 6, y + 4, &UF_HDR, uxl_word(s), on ? UX_TEXT : UX_MID, ux_under(x + 6, y + 8), 104);   /* (an effect: plain) */
}

static void ux_layer_draw(void)
{
    static uint32_t head_sig, panel_sig, foot_sig;
    uint32_t l = ui.layer % LAYER_N, sig, c, all;
    ux_knob_t k[4];
    const char *st, *w;
    uint16_t sc;
    dc_frame(ui.force);
    all = dc_all;
    {   /* the header: T3 . the sound, the transport (as the section pages') */
        char nm[24], tr[16];
        ux_trk_name(song.sel % NTRK, nm, sizeof nm, 1);
        ux_transport(tr, sizeof tr);
        sig = str_hash(str_hash(song.sel + 1u, nm), tr);
        if (all || sig != head_sig) {
            head_sig = sig;
            ux_header(song.sel % NTRK + 1u, nm + 3, tr);
        }
    }
    uxl_knobs(l, k);
    ux_knobs(k, -1);
    for (c = 0; c < 4u; c++) {                          /* (STATE's chips for EDIT and SCL: the knobs' values, layer_state) */
        str_cpy(ly_card[c].l, k[c].label, sizeof ly_card[c].l);
        str_cpy(ly_card[c].v, k[c].value, sizeof ly_card[c].v);
    }
    sig = layer_sig(l) * 31u + ux_glow_eff(1) * 977u + (uxl_state(l) ? 7u : 0u);
    for (c = 0; c < 4u; c++)
        sig = str_hash(sig * 31u + k[c].bar, k[c].value);
    if (all || sig != panel_sig) {
        panel_sig = sig;
        uxl_panels++;
        ux_panel_begin(1);
        if (uxl_state(l)) {                                  /* STATE (ALGORITHM: MAP <-> STATE) */
            lst_ux = 1;
            layer_state(l);
            lst_ux = 0;
        } else
            uxl_map(l);
        ux_panel_end();
    }
    w = uxl_where(l);
    sig = str_hash(str_hash(3u, w), UXL_ALGO[uxl_state(l)]);
    if (all || sig != foot_sig) {
        foot_sig = sig;
        ux_footer(w, UXL_ALGO[uxl_state(l)]);
    }
    st = uxl_status(l, k, &sc);
    ux_status(st, sc);
}
