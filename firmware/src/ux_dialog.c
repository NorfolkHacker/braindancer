/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* (ACID FM 3c-2 task 1) one full-screen dialog for the OCT- / OCT+ questions (ui.confirm; spec 3c-2 section 4, layout A): the
 * header kept (track, transport: the section page's, uxs_head), the page area (UX_Y_KNOBS .. UX_Y_STATUS) given way to the action
 * word (UF_LBL), the question (UF_VAL, up to two lines), one line on what is lost and how to get it back (UF_HDR, up to two
 * lines) and the two keys, OCT- and its no-word on the left, OCT+ and the verb on the right; the status line empty; the footer
 * "<SECTION> . <WORD>" / "OCT+ YES . OCT- NO". The word and the verb red (UX_ACCENT) when something is lost, else UX_ACID.
 * What the keys do is ui_input.c's, as before. GEN (CF_DICE_ALL) is GEN's screen. (Task 1b) The dice (CF_DICE_*) too: their
 * BEFORE / NOW in the panel between the question and the keys (ui_input.c dice_panel_body), the lost line in the status line;
 * the header HOME's own when the dialog opens on HOME. Drawn on a change only (dc_frame / cv_blit_cells: the changed cells).
 * Included after ux_layer.c. */

typedef struct { const char *word, *verb, *no; char q[40], lost[80]; uint8_t loses; } uxd_text_t;
static uint32_t uxd_draws;                              /* frames ux_dialog_draw drew its page area (tests) */

#define UXD_W 224                                       /* a line's width: the page's margins, 8 .. 232 */
#define UXD_LN 64                                       /* a wrapped line's bytes */

/* by kind (ui.c CF_*, designated: the mapping checks itself): the action word, the verb too ("" not drawn here; CF_NONE and GEN,
 * CF_DICE_ALL, none) */
static const char *const UXD_WORD[] = {
    [CF_NONE] = "", [CF_CLEAR_SEQ] = "CLEAR", [CF_CLEAR_TRK] = "CLEAR", [CF_OVR_PROJ] = "SAVE OVER", [CF_OVR_USER] = "SAVE OVER",
    [CF_LOAD_PAT] = "LOAD", [CF_DEL_ROW] = "DELETE", [CF_CLEAR_SONG] = "CLEAR", [CF_INIT_SOUND] = "INIT",
    [CF_CLEAR_MOTION] = "CLEAR", [CF_ERASE_USER] = "ERASE", [CF_CLEAR_PFX] = "CLEAR", [CF_RECALL_SOUND] = "RECALL",
    [CF_CLEAR_ALL] = "CLEAR", [CF_DICE_ALL] = "", [CF_INIT_SYS] = "NEW", [CF_DICE_SOUND] = "DICE", [CF_DICE_SEQ] = "DICE",
    [CF_DICE_LFO] = "DICE", [CF_DICE_FX] = "DICE", [CF_CPU_TEST] = "RUN", [CF_MUT_BACK] = "BACK", [CF_LIVE_COMMIT] = "KEEP",
    [CF_LOAD_PROJ] = "LOAD"};
/* the question in UF_VAL ('#' the track's number, '%' a label) */
static const char *const UXD_Q_VAL[] = {
    [CF_NONE] = "", [CF_CLEAR_SEQ] = "CLEAR T#'S SEQUENCE?", [CF_CLEAR_TRK] = "CLEAR TRACK #?", [CF_OVR_PROJ] = "SAVE OVER SECTION %?",
    [CF_OVR_USER] = "SAVE OVER %?", [CF_LOAD_PAT] = "REPLACE T#'S SEQUENCE?", [CF_DEL_ROW] = "DELETE ROW %?",
    [CF_CLEAR_SONG] = "CLEAR THE SONG?", [CF_INIT_SOUND] = "INIT T#'S SOUND?", [CF_CLEAR_MOTION] = "CLEAR T#'S AUTOMATION?",
    [CF_ERASE_USER] = "ERASE %?", [CF_CLEAR_PFX] = "CLEAR THE PUNCH LANE?", [CF_RECALL_SOUND] = "RECALL T#'S STORED SOUND?",
    [CF_CLEAR_ALL] = "CLEAR EVERY SEQUENCE?", [CF_DICE_ALL] = "", [CF_INIT_SYS] = "START A NEW PROJECT?",
    [CF_DICE_SOUND] = "KEEP THE DICED SOUND?", [CF_DICE_SEQ] = "KEEP THE DICED SEQUENCE?", [CF_DICE_LFO] = "KEEP THE DICED LFO?",
    [CF_DICE_FX] = "KEEP THE DICED EFFECTS?", [CF_CPU_TEST] = "BUILD THE CPU TEST SONG?", [CF_MUT_BACK] = "T# BACK TO BEFORE MUTATE?",
    [CF_LIVE_COMMIT] = "KEEP THE LIVE TRICKS?", [CF_LOAD_PROJ] = "LOAD SECTION %?"};
static const char *const UXD_QKIT_VAL[] = {             /* (a DRUM track: its kit too) */
    [CF_INIT_SOUND] = "INIT T#'S SOUND AND KIT?", [CF_RECALL_SOUND] = "RECALL T#'S STORED SOUND AND KIT?",
    [CF_DICE_SOUND] = "KEEP THE DICED KIT?"};
/* what is lost and how to get it back, in UF_HDR ('%' the pieces, in order; the dice: the status line's, one line) */
static const char *const UXD_LOST_HDR[] = {
    [CF_NONE] = "", [CF_CLEAR_SEQ] = "Loses %. GLO + G4 brings it back.", [CF_CLEAR_TRK] = "Loses %. GLO + G4 brings it back.",
    [CF_OVR_PROJ] = "Replaces %. There is no undo for this.", [CF_OVR_USER] = "Replaces %. There is no undo for this.",
    [CF_LOAD_PAT] = "Loads %. Loses %. GLO + G4 brings them back.",
    [CF_DEL_ROW] = "Its scene goes with it. There is no undo for this.", [CF_CLEAR_SONG] = "Loses %. There is no undo for this.",
    [CF_INIT_SOUND] = "The steps stay. GLO + G4 brings the sound back.", [CF_CLEAR_MOTION] = "Loses %. GLO + G4 brings them back.",
    [CF_ERASE_USER] = "Loses %. There is no undo for this.",
    [CF_CLEAR_PFX] = "Loses the lane's punch-ins. GLO + G4 brings it back.",
    [CF_RECALL_SOUND] = "Loses the changes since the section was stored. GLO + G4 brings them back.",
    [CF_CLEAR_ALL] = "Loses % and the punch lane. GLO + G4 brings it back.", [CF_DICE_ALL] = "",
    [CF_INIT_SYS] = "Clears the music in memory; stored sections stay. GLO + G4 brings it back.",
    [CF_DICE_SOUND] = "OCT- puts it back. SELECT dices again.", [CF_DICE_SEQ] = "OCT- puts it back. SELECT dices again.",
    [CF_DICE_LFO] = "OCT- puts it back. SELECT dices again.", [CF_DICE_FX] = "OCT- puts it back. SELECT dices again.",
    [CF_CPU_TEST] = "160 BPM on the internal clock; CLOCK comes back after. GLO + G4 undoes it.",
    [CF_MUT_BACK] = "Loses what MUTATE changed in its sound and pattern.",
    [CF_LIVE_COMMIT] = "Writes them into the steps. GLO + G4 takes them out.",
    [CF_LOAD_PROJ] = "Loses the changes not stored. GLO + G4 brings them back."};   /* (3c-2 task 4) */
/* the counted pieces of the lost line: a count's noun (one, then more), the joins; a count of 0 names the thing */
enum { UXD_P_NOTE, UXD_P_NOTES, UXD_P_LOCK, UXD_P_LOCKS, UXD_P_RECORD, UXD_P_RECORDS, UXD_P_ROW, UXD_P_ROWS, UXD_P_BAR,
       UXD_P_BARS, UXD_P_TRACK, UXD_P_TRACKS, UXD_P_AND, UXD_P_ON, UXD_P_COMMA, UXD_P_ITS_NOTES, UXD_P_THE_NOTES,
       UXD_P_ITS_RECORDS, UXD_P_ITS_ROWS, UXD_P_STORED };
static const char *const UXD_PIECE_HDR[] = {" note", " notes", " lock", " locks", " record", " records", " row", " rows", " bar",
                                            " bars", " track", " tracks", " and ", " on ", ", ", "its notes", "the notes",
                                            "its records", "its rows", "what is stored there"};
static const char *const UXD_NO[2] = {"NO", "KEEP"};   /* OCT-'s word (MUT BACK: KEEP) */
static const char *const UXD_VERB[1] = {"KEEP"};        /* OCT+'s word where it is not the action word (the dice) */
static const char *const UXD_FOOT[1] = {"OCT+ YES \xB7 OCT- NO"};
typedef char uxd_kinds_ok[sizeof UXD_WORD / sizeof UXD_WORD[0] == CF_LOAD_PROJ + 1u &&
                          sizeof UXD_Q_VAL / sizeof UXD_Q_VAL[0] == CF_LOAD_PROJ + 1u &&
                          sizeof UXD_LOST_HDR / sizeof UXD_LOST_HDR[0] == CF_LOAD_PROJ + 1u &&
                          sizeof UXD_QKIT_VAL / sizeof UXD_QKIT_VAL[0] == CF_DICE_SOUND + 1u ? 1 : -1];

/* every CF_* but CF_NONE and GEN (CF_DICE_ALL) */
static int ux_dialog_owns(uint32_t kind)
{
    return kind && kind <= CF_LOAD_PROJ && kind != CF_DICE_ALL;
}
static int uxd_dice(uint32_t kind) { return kind >= CF_DICE_SOUND && kind <= CF_DICE_FX; }

/* notes + hits on the track's P_SLEN steps */
static uint32_t uxd_notes(const track_t *t)
{
    uint32_t i, n = 0, len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), h;
    for (i = 0; i < len; i++)
        if (t->step[i].time == ST_NOTE) {
            n += t->step[i].n;
            for (h = t->step[i].hit; h; h &= h - 1u)
                n++;
        }
    return n;
}

static void uxd_cat(char *b, uint32_t n, const char *s)
{
    uint32_t l = str_len(b);
    if (l + 1u < n)
        str_cpy(b + l, s, n - l);
}
static void uxd_count(char *b, uint32_t n, uint32_t v, uint32_t one)   /* "14 notes", "1 note" */
{
    char d[12];
    fmt_int(d, (int32_t)v);
    uxd_cat(b, n, d);
    uxd_cat(b, n, UXD_PIECE_HDR[one + (v != 1u)]);
}
/* tpl into d (n bytes): '#' track k's number, each '%' the next of a, b */
static void uxd_fill(char *d, uint32_t n, const char *tpl, uint32_t k, const char *a, const char *b)
{
    uint32_t l = 0;
    char c[2] = {0, 0};
    d[0] = 0;
    for (; *tpl && l + 1u < n; tpl++) {
        if (*tpl == '%') {
            uxd_cat(d, n, a ? a : "");
            a = b;
            b = 0;
        } else {
            c[0] = *tpl == '#' ? (char)('1' + k % NTRK) : *tpl;
            uxd_cat(d, n, c);
        }
        l = str_len(d);
    }
}
/* a stored name (nm, its trailing spaces cut) fitted to a part of the line into a (n bytes), else "what is stored there" */
static void uxd_name(char *a, uint32_t n, char *nm)
{
    uint32_t l = str_len(nm);
    while (l && nm[l - 1u] == ' ')
        nm[--l] = 0;
    if (l)
        text_fit(a, n, nm, &UF_HDR, 120);
    else
        str_cpy(a, UXD_PIECE_HDR[UXD_P_STORED], n);
}
/* track t's notes and locks: "14 notes and 3 locks", "its notes" */
static void uxd_notes_locks(char *b, uint32_t n, const track_t *t)
{
    uint32_t nn = uxd_notes(t), nl = motion_lock_count(t);
    if (nn)
        uxd_count(b, n, nn, UXD_P_NOTE);
    else
        uxd_cat(b, n, UXD_PIECE_HDR[UXD_P_ITS_NOTES]);
    if (nl) {
        uxd_cat(b, n, UXD_PIECE_HDR[UXD_P_AND]);
        uxd_count(b, n, nl, UXD_P_LOCK);
    }
}

/* one confirmation's words: kind a CF_*, k ui.confirm_trk */
static void uxd_text(uint32_t kind, uint32_t k, uxd_text_t *t)
{
    char a[40], b[40], nm[24];
    const track_t *tk = &trk[k % NTRK];
    const char *q;
    uint32_t i;
    if (kind > CF_LOAD_PROJ)
        kind = CF_NONE;
    a[0] = b[0] = nm[0] = 0;
    t->word = t->verb = UXD_WORD[kind];
    if (uxd_dice(kind))
        t->verb = UXD_VERB[0];
    t->no = UXD_NO[kind == CF_MUT_BACK];
    t->loses = (uint8_t)(kind != CF_LIVE_COMMIT && !uxd_dice(kind));
    q = UXD_Q_VAL[kind];
    switch (kind) {
    case CF_CLEAR_SEQ:
    case CF_CLEAR_TRK:
        uxd_notes_locks(a, sizeof a, tk);
        break;
    case CF_LOAD_PAT: {
        char tag[4];
        pat_label(pat_pick(), tag, b);
        str_cpy(nm, tag, sizeof nm);
        uxd_cat(nm, sizeof nm, " ");
        uxd_cat(nm, sizeof nm, b);
        uxd_name(a, sizeof a, nm);
        b[0] = 0;
        i = uxd_notes(tk);
        if (i)
            uxd_count(b, sizeof b, i, UXD_P_NOTE);
        else
            uxd_cat(b, sizeof b, UXD_PIECE_HDR[UXD_P_ITS_NOTES]);
        break;
    }
    case CF_LOAD_PROJ:                                  /* (3c-2 task 4) "LOAD SECTION B?" */
        b[0] = (char)('A' + (k & 3u));
        b[1] = 0;
        break;
    case CF_OVR_PROJ:
        b[0] = (char)('A' + (k & 3u));
        b[1] = 0;
        if (!project_name(k & 3u, nm))
            nm[0] = 0;
        uxd_name(a, sizeof a, nm);
        break;
    case CF_OVR_USER:
    case CF_ERASE_USER:
        up_slot_label(b, k);
        up_name(k, nm);
        uxd_name(a, sizeof a, nm);
        break;
    case CF_DEL_ROW:                                    /* "3 . B16": the row, its section and bars */
        fmt_int(b, (int32_t)k + 1);
        if (k < chain_config.count) {
            uint32_t l;
            uxd_cat(b, sizeof b, " \xB7 ");
            l = str_len(b);
            b[l] = (char)('A' + (chain_config.row[k].slot & 3u));
            b[l + 1u] = 0;
            fmt_int(nm, chain_config.row[k].bars);
            uxd_cat(b, sizeof b, nm);
        }
        break;
    case CF_CLEAR_SONG: {
        uint32_t bars = 0;
        for (i = 0; i < chain_config.count; i++)
            bars += chain_config.row[i].bars;
        if (chain_config.count) {
            uxd_count(a, sizeof a, chain_config.count, UXD_P_ROW);
            uxd_cat(a, sizeof a, UXD_PIECE_HDR[UXD_P_COMMA]);
            uxd_count(a, sizeof a, bars, UXD_P_BAR);
        } else
            str_cpy(a, UXD_PIECE_HDR[UXD_P_ITS_ROWS], sizeof a);
        break;
    }
    case CF_CLEAR_MOTION: {
        uint32_t nl = motion_lock_count(tk), nr = motion_count(tk) - nl;
        if (nr)
            uxd_count(a, sizeof a, nr, UXD_P_RECORD);
        else
            uxd_cat(a, sizeof a, UXD_PIECE_HDR[UXD_P_ITS_RECORDS]);
        if (nl) {
            uxd_cat(a, sizeof a, UXD_PIECE_HDR[UXD_P_AND]);
            uxd_count(a, sizeof a, nl, UXD_P_LOCK);
        }
        break;
    }
    case CF_CLEAR_ALL: {
        uint32_t nn = 0, nt = 0, c;
        for (i = 0; i < NTRK; i++) {
            c = uxd_notes(&trk[i]);
            nn += c;
            nt += c != 0u;
        }
        if (nn) {
            uxd_count(a, sizeof a, nn, UXD_P_NOTE);
            uxd_cat(a, sizeof a, UXD_PIECE_HDR[UXD_P_ON]);
            uxd_count(a, sizeof a, nt, UXD_P_TRACK);
        } else
            str_cpy(a, UXD_PIECE_HDR[UXD_P_THE_NOTES], sizeof a);
        break;
    }
    case CF_INIT_SOUND:
    case CF_RECALL_SOUND:
    case CF_DICE_SOUND:
        if (drum_track(tk))
            q = UXD_QKIT_VAL[kind];
        break;
    default:
        break;
    }
    if (kind == CF_LOAD_PAT)                            /* (the question names the track; the lost line the pattern, the notes) */
        uxd_fill(t->q, sizeof t->q, q, k, 0, 0);
    else
        uxd_fill(t->q, sizeof t->q, q, k, b, 0);
    uxd_fill(t->lost, sizeof t->lost, UXD_LOST_HDR[kind], k, a, b);
}

/* s in face f as up to two lines of UXD_W px, at the spaces: the lines used (3: it does not fit) */
static uint32_t uxd_wrap(const char *s, const aafont_t *f, char ln[2][UXD_LN])
{
    uint32_t n = 0, i = 0, j;
    ln[0][0] = ln[1][0] = 0;
    while (s[i] == ' ')
        i++;
    while (s[i]) {
        char w[UXD_LN];
        uint32_t l = str_len(ln[n]);
        for (j = i; s[j] && s[j] != ' '; j++) {}
        str_cpy(w, ln[n], sizeof w);
        if (l)
            uxd_cat(w, sizeof w, " ");
        l = str_len(w);
        if (l + (j - i) + 1u > sizeof w)
            return 3u;
        memcpy(w + l, s + i, j - i);
        w[l + (j - i)] = 0;
        if (text_w(f, w) <= UXD_W) {
            str_cpy(ln[n], w, UXD_LN);
            for (i = j; s[i] == ' '; i++) {}
        } else if (!ln[n][0] || ++n >= 2u) {
            return 3u;
        }
    }
    return ln[1][0] ? 2u : 1u;
}

/* the footer: "SEQ . CLEAR", "OCT+ YES . OCT- NO" */
static void uxd_foot(char *left, uint32_t nl, char *right, uint32_t nr)
{
    uint32_t kind = ui.confirm <= CF_LOAD_PROJ ? ui.confirm : CF_NONE;
    str_cpy(left, SEC_NAME[nav_sec()], nl);
    uxd_cat(left, nl, " \xB7 ");
    uxd_cat(left, nl, UXD_WORD[kind]);
    str_cpy(right, UXD_FOOT[0], nr);
}

/* what the page area is drawn from: the kind, the track, the counts it says (names: on a change of kind or slot); the dice,
 * what BEFORE / NOW show (ui_input.c dice_sig) */
static uint32_t uxd_sig(void)
{
    uint32_t kind = ui.confirm, k = ui.confirm_trk, sig = kind * 2654435761u + k * 977u + ux_glow_eff(1) * 31u, i;
    const track_t *t = &trk[k % NTRK];
    sig = sig * 31u + (uint32_t)drum_track(t);
    if (uxd_dice(kind))
        sig = sig * 31u + dice_sig();
    if (kind == CF_CLEAR_SEQ || kind == CF_CLEAR_TRK || kind == CF_LOAD_PAT || kind == CF_CLEAR_MOTION)
        sig = sig * 31u + uxd_notes(t) * 7u + motion_count(t) * 131u + motion_lock_count(t);
    if (kind == CF_LOAD_PAT)
        sig = sig * 31u + pat_pick();
    if (kind == CF_CLEAR_ALL)
        for (i = 0; i < NTRK; i++)
            sig = sig * 31u + uxd_notes(&trk[i]);
    if (kind == CF_CLEAR_SONG || kind == CF_DEL_ROW)
        for (sig = sig * 31u + chain_config.count, i = 0; i < chain_config.count && i < CHAIN_ROWS; i++)
            sig = sig * 31u + chain_config.row[i].bars * 7u + chain_config.row[i].slot;
    return sig;
}

/* a key's cell: its name (OCT- / OCT+) over its word, a rule under (OCT+: 2 px in the colour) */
static void uxd_key(int32_t x, const char *key, const char *word, uint16_t col, int yes)
{
    GFX_HOOK_CELL(x, 100, x + 108, 142);
    cv_text_on(x, 104, &UF_LBL, key, UX_MID, ux_under(x, 108));
    cv_text_on(x, 116, &UF_VAL, word, yes ? col : UX_TEXT, ux_under(x, 124));
    cv_rect(x, 138, 104, yes ? 2 : 1, yes ? col : UX_LINE);
}

/* the header: the page's under the dialog (HOME's own, else the section pages'; drawn when draw): its sig */
static uint32_t uxd_head(int draw)
{
    int32_t hx;
    return ui.home ? ux_home_head(ux_home_page(), draw, &hx) : uxs_head(draw);
}

/* ui.confirm full screen: header, page area, status, footer. The dice (task 1b): the question in one line over their BEFORE /
 * NOW (ui_input.c dice_panel_body), the keys under; the lost line the status line's */
static void ux_dialog_draw(void)
{
    static uint32_t head_sig, panel_sig, foot_sig;
    uint32_t sig, all, i, n, dice = uxd_dice(ui.confirm);
    char l[40], r[40];
    dc_frame(ui.force);
    all = dc_all;
    sig = uxd_head(0);
    if (all || sig != head_sig) {
        head_sig = sig;
        (void)uxd_head(1);
    }
    sig = uxd_sig();
    if (all || sig != panel_sig) {
        uxd_text_t t;
        char ln[2][UXD_LN];
        uint16_t col;
        panel_sig = sig;
        uxd_draws++;
        uxd_text(ui.confirm, ui.confirm_trk, &t);
        col = t.loses ? UX_ACCENT : UX_ACID;
        cv_begin(240, UX_H_KNOBS, UX_BG);               /* the knob row's rows: the action word */
        GFX_HOOK_CELL(8, 8, 232, 24);
        cv_text_on(8, 12, &UF_LBL, t.word, col, UX_BG);
        cv_blit_cells(0, UX_Y_KNOBS);
        ux_panel_begin(1);                              /* the panel's: the question, the lost line (the dice: BEFORE / NOW), the keys */
        n = uxd_wrap(t.q, &UF_VAL, ln);
        GFX_HOOK_CELL(8, 2, 232, dice ? 24 : 46);
        for (i = 0; i < n && i < (dice ? 1u : 2u); i++)
            cv_text_on(8, 4 + (int32_t)i * 22, &UF_VAL, ln[i], UX_TEXT, ux_under(8, 12 + (int32_t)i * 22));
        if (dice) {
            dice_panel_body();
        } else {
            n = uxd_wrap(t.lost, &UF_HDR, ln);
            GFX_HOOK_CELL(8, 56, 232, 90);
            for (i = 0; i < n && i < 2u; i++)
                cv_text_on(8, 58 + (int32_t)i * 16, &UF_HDR, ln[i], UX_MID, ux_under(8, 64 + (int32_t)i * 16));
        }
        uxd_key(8, "OCT-", t.no, col, 0);
        uxd_key(124, "OCT+", t.verb, col, 1);
        ux_panel_end();
    }
    uxd_foot(l, sizeof l, r, sizeof r);
    sig = str_hash(str_hash(5u, l), r);
    if (all || sig != foot_sig) {
        foot_sig = sig;
        ux_footer(l, r);
    }
    ux_status(dice ? UXD_LOST_HDR[ui.confirm] : 0, UX_MID);   /* (the dice's: no count in it) */
}
