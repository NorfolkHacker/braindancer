/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's SEQ pages on the screen (3c-1 spec 2.1, parent spec 4.4; ux_seq.h), drawn through ux_section.c's page (header, knob
 * row, panel, status line, footer) as the 3b sections are:
 *   STEPS    top: the roll of the 16 steps shown (ui.bank's), pitch up and time across, a column a step over the strip's cells:
 *            notes ACID, accents ACCENT, slides SLIDE joined to the next note, a TIE carrying its note on, the picked step
 *            outlined TEXT, the playing step a light column, a step lock a mark at its foot. A DRUM track: the eight lanes
 *            over the same columns, a hit in its lane's colour (the blend, BD .. CB), an accent brighter, the lane picked
 *            underlaid. Under it the strip (uxq_strip) and the picked step spelled out ("5 . G2 . ACC").
 *            Knobs STEP . NOTE . ACC/SLD . GATE (the inherited STEP page's STEP, NOTE, FLAG, TIME: ui_input.c step_edit by the
 *            PAGES row's ids); a DRUM track's STEP . LANE . HIT . ACC (ui_input.c grid_edit). A step held (a lock): the
 *            knobs show what they lock, as the inherited card did (task 4 restyles it).
 *            32 / 64 steps: SELECT on the strip's last cell the next 16, on its first the 16 before (uxq_select); the header
 *            says which 16 ("2/4"), before the transport.
 *   STEP EXTRAS (3c-1 spec 2.2; task 3) the step picked on STEPS (one state, ui.cursor): knobs RATCHET . CHANCE . CONDITION . MICRO
 *            (the inherited CHANCE page's RATCH x1..x4 and CHANCE, COMP's TRIG: the step's condition, NOTE's MICRO: ui_input.c
 *            edit_param's GR_CHANCE).
 *            Top: the 16 shown as columns, each step's chance its fill height, its ratchets the parts it is cut in, its micro
 *            timing a mark from the column's middle (late right, early left); under them the strip, then the picked step's
 *            condition spelled ("5 . 1ST OF 4").
 *   PATTERN  (spec 2.3) knobs LENGTH . DIVISION . SWING . GATE (P_SLEN P_SDIV P_SSWING P_SGATE: ux_section.c's knob row, the
 *            inherited page's ids): the pattern's steps as cells in rows of 16 (past LEN dark), the offbeats moved late by SWING,
 *            each cell as wide as the GATE; under them "32 STEPS . 1/16".
 * The roll is redrawn when what it shows changes (ux_section.c's panel signature: uxq_sig), the playhead as it moves at most
 * every UX_TRK_MS (UX_TRK_SLOW_MS under load): one panel a played step at most, none while nothing changes. Included after
 * ux_more.c (uxm_lock_card), before ux_section.c (uxs_label, uxs_vtext: ux_section.h). */

#define UXQ_RY0 6                /* the roll / grid: its top row and height in the panel (spec: the top ~96 px) */
#define UXQ_RH 92
#define UXQ_STRIP_Y 104          /* the strip's row (UX_STRIP_H tall) */
#define UXQ_SAY_Y 128            /* the picked step spelled out (UF_HDR) */
#define UXQ_PY0 6                /* PATTERN: its first row of cells, the rows' pitch, a cell's height and widest (a full GATE) */
#define UXQ_PROW 24u
#define UXQ_PH 18
#define UXQ_PW (UX_STRIP_W - 5)
#define UXQ_SWING 5              /* .. an offbeat's shift at SWING 100 (px) */
#define UXQ_X(i) (UX_STRIP_X + (int32_t)(i) * UX_STRIP_PITCH)   /* cell / column i's left, UX_STRIP_W wide */

static const char *const UXQ_FLAG[4] = {"OFF", "ACC", "SLD", "BOTH"};   /* ACC/SLD's values (the inherited FLAG's - ACC SLD A+S) */
static const char *const UXQ_FLAG_SAY[4] = {"ACCENT AND SLIDE OFF", "ACCENT ON", "SLIDE ON", "ACCENT AND SLIDE ON"};
static const char *const UXQ_TIME[3] = {"NOTE", "TIE", "REST"};          /* GATE's (a NOTE one step long; longer: its steps) */
static const char *const UXQ_HIT[LM_N + 1] = {"OFF", "ON", "X2", "X4", "X8", "SHORT", "GHOST", "FLAM"};   /* (ui.c GRID_HIT's) */
/* the words the knob row, the status line and the spelled step are made of (one table: tests/ux_msg_scan.py checks each has its
 * glyphs in UF_LBL and UF_HDR); the knob labels, STEPS' and the grid's */
static const char *const UXQ_WORD[] = {"STEP ", " \xB7 ", " STEPS", "ACCENT ", "ON", "OFF", "ACC", "SLD", "--", "+", "/", " ",
                                       "LOCK", "AUTO", "ST", "LOCK \xB7 "};
enum { UXQ_W_STEP, UXQ_W_DOT, UXQ_W_STEPS, UXQ_W_ACCENT, UXQ_W_ON, UXQ_W_OFF, UXQ_W_ACC, UXQ_W_SLD, UXQ_W_NONE, UXQ_W_PLUS,
       UXQ_W_SLASH, UXQ_W_SP, UXQ_W_LOCK, UXQ_W_AUTO, UXQ_W_ST, UXQ_W_LOCKDOT };
static const char *const UXQ_LABEL[11] = {"STEP", "NOTE", "ACC/SLD", "GATE", "LANE", "HIT", "ACC", "RATCHET", "CHANCE", "CONDITION",
                                          "MICRO"};
enum { UXQ_L_STEP, UXQ_L_NOTE, UXQ_L_FLAG, UXQ_L_GATE, UXQ_L_LANE, UXQ_L_HIT, UXQ_L_ACC, UXQ_L_RATCH };
/* (task 3; pre-review fix) STEP EXTRAS' CONDITION: the step's TRIG component (core.h CP_TRIGS: on which loops of the pattern the
 * note plays, seq.c cp_trig_ok), in words: ALWAYS (none), then CP_TRIGS' a of b loops, the first loop alone, all but it */
#define UXQ_FULL (CP_NTRIG + 1u)                         /* (no TRIG and both component slots another's: none can be put) */
static const char *const UXQ_COND[CP_NTRIG + 1u] = {"ALWAYS", "1ST OF 2", "EVERY 2ND", "1ST OF 3", "2ND OF 3", "EVERY 3RD", "1ST OF 4",
                                                    "2ND OF 4", "3RD OF 4", "EVERY 4TH", "1ST OF 8", "1ST ONLY", "NOT 1ST"};
static const char *const UXQ_XW[6] = {"X", "%", "GRID", "/64", "-", "FULL"};   /* (a ratchet's X2, a chance's 65%, MICRO on the
                                                         * grid, its 64ths, early; CONDITION with no slot free) */
static const char *const UXQ_VNAME[ID_SQ_END - ID_SQ_STRETCH] = {"STRETCH", "REPEAT", "NUDGE", "HEAD", "RATE", "DIRECTION",
                                                                 "INTERVAL", "TYPE", "", "", "TYPE 2", "", "", "EVERY", "FOCUS",
                                                                 "PITCH", "MOVE", "PLAY", "EVENTS", "LOCKS", "ROW", "STEP",
                                                                 "SETTING", "VALUE", "RANGE", "RANGE"};
static const char *const UXQ_TYPE[COMP_N] = {"OFF", "CHANCE", "RATCHET", "ARP", "SEQ", "WALK", "PULSE", "OCTAVE", "RANDOM", "BEND",
                                             "TONAL", "JUMP", "REVERSE", "CONDITION", "FILL", "KILL"};
static const char *const UXQ_VAL1[COMP_N] = {"", "CHANCE", "RATCHET", "ORDER", "ORDER", "MODE", "PULSE", "MODE", "RANGE", "BEND",
                                             "SHIFT", "TO STEP", "", "CONDITION", "", ""};   /* (a TYPE's first value; none: no row) */
static const char *const UXQ_EVERY[EV_N] = {"ALWAYS", "1ST OF 2", "EVERY 2ND", "NEVER", "1ST OF 3", "2ND OF 3", "EVERY 3RD",
                                            "1ST OF 4", "2ND OF 4", "3RD OF 4", "EVERY 4TH", "1ST OF 8", "1ST ONLY", "NOT 1ST"};
static const char *const UXQ_WAY[4] = {"UP", "DOWN", "UP-DOWN", "RANDOM"};   /* (an ARP's ORDER, WALK's MODE, a head's way: +PING) */
#define UXQ_PSTEP "PITCH STEP"                          /* (RATCH's pitch step, its own row: "PITCH" is NOTE's) */
static const char *const UXQ_HDIR[4] = {"FORWARD", "REVERSE", "PING-PONG", "RANDOM"};   /* (fugue.c N_FGDIR) */
static const char *const UXQ_VW[] = {" DEG", " OCT", " LOOPS", " LOOP", "FIT", " STEPS", "ALL", "CHORD", "NOTE ", "ADD", "ON",
                                     "OFF", "TO 64", "\x86", " FADE", " RISE"};
enum { UXQ_V_DEG, UXQ_V_OCT, UXQ_V_LOOPS, UXQ_V_LOOP, UXQ_V_FIT, UXQ_V_STEPS, UXQ_V_ALL, UXQ_V_CHORD, UXQ_V_NOTE, UXQ_V_ADD,
       UXQ_V_ON, UXQ_V_OFF, UXQ_V_TO64, UXQ_V_LR, UXQ_V_FADE, UXQ_V_RISE };
static int32_t uxq_ph_drawn = -1;                       /* the playing cell drawn (-1: none), when (ms): ux_section_draw's pace */
static uint32_t uxq_ph_ms;

enum { UXQ_NONE, UXQ_STEPS, UXQ_EXTRAS, UXQ_PATT };     /* the SEQ pages drawn here */
static uint32_t uxq_page(const page_t *pg)
{
    if (pg->fam != FAM_SEQ || pg->graph == GR_MORE)
        return UXQ_NONE;
    if (pg->scope == SC_STEP && pg->graph == GR_ROLL && pg->id[0] == 0u)
        return UXQ_STEPS;                               /* (STEP: ids 0 1 3 2; NOTE's 4 ..) */
    if (pg->graph == GR_CHANCE)
        return UXQ_EXTRAS;                              /* (task 3: the row CHANCE was) */
    return pg->graph == GR_STEPS && pg->id[0] == P_SLEN ? UXQ_PATT : UXQ_NONE;   /* (not TRACK's, GR_STEPS too) */
}
static int uxq_owns(void) { return ui.home || ui.page >= NPAGES ? UXQ_NONE : (int)uxq_page(&PAGES[ui.page]); }
static uint32_t uxq_len(void) { return (uint32_t)clamp(TSEL->p[P_SLEN], 1, NSTEP); }
static uint32_t uxq_step(void) { uint32_t len = uxq_len(); return ui.cursor < len ? ui.cursor : len - 1u; }
static uint32_t uxq_first(void) { return (uxq_step() / 16u) * 16u; }   /* the first step of the 16 shown (ui.bank's) */
static int32_t uxq_ph(void)                             /* the playing cell of the 16 shown (PATTERN: the step; -1: none) */
{
    const track_t *t = TSEL;
    uint32_t s = seq_src(t, t->seq_idx), f = uxq_first();
    if (!song.playing || s >= uxq_len())
        return -1;
    if (uxq_owns() == UXQ_PATT)
        return (int32_t)s;
    return s >= f && s < f + 16u ? (int32_t)(s - f) : -1;
}
static void uxq_cat(char *b, uint32_t n, const char *s) { str_cpy(b + str_len(b), s, n - str_len(b)); }
static void uxq_num(char *b, uint32_t n, int32_t v) { char d[8]; fmt_int(d, v); uxq_cat(b, n, d); }
static uint32_t uxq_flag(const step_t *st) { return (st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u); }
static void uxq_note(const step_t *st, char *b, uint32_t n)   /* the step's note ("G2"; a chord "G2+2"; none "--") */
{
    char nb[8];
    b[0] = 0;
    if (st->time != ST_NOTE || !st->n) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_NONE]);
        return;
    }
    note_name(nb, st->note[0]);
    uxq_cat(b, n, nb);
    if (st->n > 1u) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_PLUS]);
        uxq_num(b, n, (int32_t)st->n - 1);
    }
}
static uint32_t uxq_gate(const step_t *st, uint32_t s)   /* GATE: a NOTE's length in steps (1: one), 0 a TIE or a REST */
{
    return st->time == ST_NOTE && st->n ? note_len(TSEL, s, uxq_len()) : 0u;
}
static void uxq_gate_txt(const step_t *st, uint32_t s, char *b, uint32_t n, int full)   /* "NOTE" "TIE" "REST" "3" (full: "3 STEPS") */
{
    uint32_t g = uxq_gate(st, s);
    b[0] = 0;
    if (g > 1u) {
        uxq_num(b, n, (int32_t)g);
        if (full)
            uxq_cat(b, n, UXQ_WORD[UXQ_W_STEPS]);
    } else
        uxq_cat(b, n, UXQ_TIME[st->time == ST_NOTE ? 0u : st->time == ST_TIE ? 1u : 2u]);
}
static void uxq_stepn(char *b, uint32_t n, int len)     /* "5/16" (len: with LEN) */
{
    b[0] = 0;
    uxq_num(b, n, (int32_t)uxq_step() + 1);
    if (len) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SLASH]);
        uxq_num(b, n, (int32_t)uxq_len());
    }
}
/* the picked step spelled out: "5 . G2 . ACC" (a chord G2+2, a TIE / REST so, SLD, a long note's steps); a DRUM track's
 * "5 . SNARE ON . ACC" (the lane picked, its hit) */
static __attribute__((noinline)) void uxq_spell(char *b, uint32_t n)
{
    uint32_t s = uxq_step(), f;
    const step_t *st = &TSEL->step[s];
    char v[16];
    uxq_stepn(b, n, 0);
    uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
    if (drum_track(TSEL)) {
        uint32_t l = ui.lane % NLANE;
        uxq_cat(b, n, drum_lane_name(TSEL, l));
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SP]);
        uxq_cat(b, n, UXQ_HIT[grid_hit_of(st, l)]);
        if ((step_accents(st) >> l) & 1u) {
            uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
            uxq_cat(b, n, UXQ_WORD[UXQ_W_ACC]);
        }
        return;
    }
    if (st->time != ST_NOTE || !st->n) {
        uxq_cat(b, n, UXQ_TIME[st->time == ST_TIE ? 1u : 2u]);
        return;
    }
    uxq_note(st, v, sizeof v);
    uxq_cat(b, n, v);
    f = uxq_flag(st);
    if (f & 1u) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_ACC]);
    }
    if (f & 2u) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SLD]);
    }
    if (uxq_gate(st, s) > 1u) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
        uxq_gate_txt(st, s, v, sizeof v, 1);
        uxq_cat(b, n, v);
    }
}
/* the header's right: which 16 of the pattern ("2/4 . "), then the transport (16 steps or fewer: the transport alone) */
static void uxq_head(char *b, uint32_t n)
{
    uint32_t pages = (uxq_len() + 15u) / 16u;
    char tr[16];
    b[0] = 0;
    if (pages > 1u) {
        uxq_num(b, n, (int32_t)(uxq_step() / 16u) + 1);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SLASH]);
        uxq_num(b, n, (int32_t)pages);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
    }
    ux_transport(tr, sizeof tr);
    uxq_cat(b, n, tr);
}
static int uxq_select(int32_t s)
{
    uint32_t c = uxq_step(), len = uxq_len();
    if (uxq_owns() != UXQ_STEPS || !s)
        return 0;
    if (s > 0 && c % 16u == 15u && c + 1u < len) {     /* (the last cell: the next 16, from its first) */
        cursor_set((int32_t)c + 1);
        return 1;
    }
    if (s < 0 && c % 16u == 0u && c >= 16u) {          /* (the first: the 16 before, on its last) */
        cursor_set((int32_t)c - 1);
        return 1;
    }
    return 0;
}

/* the knob row: STEP NOTE ACC/SLD GATE (a DRUM track STEP LANE HIT ACC); a step held: the lock card (spec 2.5), what KNOB 1..4 lock
 * (ui_input.c lock_turn: the sound page's four, a MORE list's pick and row), a value not locked dim; (task 4) a chord being entered
 * on the roll: KNOB 4 its LENGTH, KNOB 1..3 the locks'; arpeggiated: KNOB 1..3 its RATE ORDER OCTAVES, KNOB 4 the lock's (a MORE
 * list's: its pick) */
static const char *const UXQ_ENT[4] = {"RATE", "ORDER", "OCTAVES", "LENGTH"};
static void uxq_lock_knobs(ux_knob_t k[4])
{
    uint64_t held = lock_held();
    const page_t *lp = lock_src();
    uint32_t st = 0, c, ent = ui.entry_open && !grid_on(), more = lp && lp->graph == GR_MORE;
    while (st < NSTEP && !((held >> st) & 1u))
        st++;                                           /* (the first step held: its values) */
    for (c = 0; c < 4u; c++) {
        int on = 0;
        if (ent && (ui.ent_arp ? c < 3u : c == 3u)) {   /* (the chord's own) */
            k[c].label = UXQ_ENT[ui.ent_arp ? c : 3u];
            if (!ui.ent_arp || c == 2u)
                uxq_num(k[c].value, sizeof k[c].value, ui.ent_arp ? ui.ent_oct : ui.ent_len);
            else
                uxq_cat(k[c].value, sizeof k[c].value, c ? UXQ_WAY[ui.ent_ord & 3u] : ENT_RES[(ui.ent_res - 1u) & 3u]);
            on = 1;
        } else if (more) {
            int32_t r;
            char v[24];
            k[c].label = uxm_lock_card(lp, ent && ui.ent_arp && c == 3u ? 0u : c, st, v, sizeof v, &on, &r);
            str_cpy(k[c].value, v, sizeof k[c].value);
            k[c].bar = (uint8_t)(r >= 0 ? r * 255 / 1000 : 0);
        } else {
            uint32_t id = lock_id(c);
            const param_desc_t *d;
            int16_t v;
            if (id == 0xFFu)
                continue;
            d = track_desc(TSEL, id);
            on = motion_lock_get(TSEL, st, id, &v);
            if (!on)
                v = motion_base_value(TSEL, id);        /* (not locked: the sound's own, dim) */
            k[c].label = uxs_label(sec_of_fam(lp ? lp->fam : FAM_SEQ), 0, id, d->label);
            uxs_vtext(d, v, 0, id, k[c].value, sizeof k[c].value);
            if (d->max > d->min)
                k[c].bar = (uint8_t)clamp(((int32_t)v - d->min) * 255 / (d->max - d->min), 0, 255);
        }
        k[c].off = (uint8_t)!on;
    }
}
/* the step's ratchet as STEP EXTRAS shows it (the effective one: an ECHO component's x2..x8 and its shape, else the flags'
 * x1..x4): its hits, *shape RS_FLAT / RS_FADE / RS_RISE, *k the ECHO's slot (-1: the flags') */
static uint32_t uxq_ratch(const step_t *st, uint32_t *shape, int32_t *k)
{
    int32_t e = st->comp == CP_ECHO ? 0 : st->comp2 == CP_ECHO ? 1 : -1;
    uint32_t r = e < 0 ? 0u : rat_ui(st, (uint32_t)e);
    *k = e;
    *shape = r ? (r - 1u) / 7u % 3u : RS_FLAT;          /* (rat_ui: 1..7 FLAT x2..x8, 8..14 FADE, 15..21 RISE) */
    return r ? 2u + (r - 1u) % 7u : step_ratchet(st);
}
static void uxq_ratch_txt(const step_t *st, char *b, uint32_t n)   /* "X3", "X6 FADE" */
{
    uint32_t sh;
    int32_t k;
    uxq_cat(b, n, UXQ_XW[0]);
    uxq_num(b, n, (int32_t)uxq_ratch(st, &sh, &k));
    if (sh != RS_FLAT)
        uxq_cat(b, n, UXQ_VW[sh == RS_FADE ? UXQ_V_FADE : UXQ_V_RISE]);
}
static int uxq_on(const step_t *st) { return st->time == ST_NOTE && (st->n || st->hit); }   /* (the step plays something) */
/* (pre-review fix) CONDITION: the step's TRIG's slot (0 comp, 1 comp2; -1 none); its place in UXQ_COND (0 ALWAYS), UXQ_FULL */
static int32_t uxq_trig(const step_t *st) { return st->comp == CP_TRIG ? 0 : st->comp2 == CP_TRIG ? 1 : -1; }
static uint32_t uxq_cond(const step_t *st)
{
    int32_t k = uxq_trig(st);
    if (k >= 0)
        return 1u + (k ? st->carg2 : st->carg) % CP_NTRIG;
    return st->comp != CP_NONE && st->comp2 != CP_NONE ? UXQ_FULL : 0u;
}
/* .. a detent on (e +-1): the TRIG there edited in place (task 4: a TRIG in both slots, the note on the loops both allow: the first
 * shown and edited), else one put in the first free slot (comp, then comp2); ALWAYS takes it away (its slot emptied; both TRIGs
 * when both are, so ALWAYS plays always); no slot free: nothing written, FULL shown (never another component overwritten) */
static void uxq_cond_turn(step_t *st, int32_t e)
{
    uint32_t c = uxq_cond(st);
    int32_t k = uxq_trig(st);
    uint8_t *cp, *ap;
    if (c == UXQ_FULL)
        return;
    c = (uint32_t)clamp((int32_t)c + e, 0, CP_NTRIG);
    if (k < 0) {
        if (!c)
            return;
        k = st->comp == CP_NONE ? 0 : 1;
    }
    cp = k ? &st->comp2 : &st->comp;
    ap = k ? &st->carg2 : &st->carg;
    *cp = (uint8_t)(c ? CP_TRIG : CP_NONE);
    *ap = (uint8_t)(c ? c - 1u : 0u);
    if (!c && st->comp2 == CP_TRIG)                     /* (ALWAYS: the second TRIG too) */
        st->comp2 = st->carg2 = 0;
}
/* (task 3) STEP EXTRAS': RATCHET X1..X4, CHANCE 0..100%, CONDITION in words, MICRO +-n (64ths; GRID on it); a step with nothing to
 * play: its values dim, MICRO -- (micro_edit refuses it) */
static void uxq_extras_knobs(ux_knob_t k[4])
{
    const step_t *st = &TSEL->step[uxq_step()];
    uint32_t sh, r, ch = step_chance(st), cd = uxq_cond(st), c;
    int32_t m = ST_MICRO(st), ek;
    r = uxq_ratch(st, &sh, &ek);                        /* (task 4: the effective ratchet, an ECHO's x2..x8 and shape too) */
    int on = uxq_on(st);
    for (c = 0; c < 4u; c++) {
        k[c].label = UXQ_LABEL[UXQ_L_RATCH + c];
        k[c].off = (uint8_t)!on;
    }
    uxq_ratch_txt(st, k[0].value, sizeof k[0].value);
    k[0].bar = (uint8_t)((r - 1u) * 255u / 7u);
    uxq_num(k[1].value, sizeof k[1].value, (int32_t)ch);
    uxq_cat(k[1].value, sizeof k[1].value, UXQ_XW[1]);
    k[1].bar = (uint8_t)(ch * 255u / 100u);
    uxq_cat(k[2].value, sizeof k[2].value, cd == UXQ_FULL ? UXQ_XW[5] : UXQ_COND[cd]);
    k[2].bar = (uint8_t)(cd == UXQ_FULL ? 0u : cd * 255u / CP_NTRIG);
    k[2].off = (uint8_t)(!on || cd == UXQ_FULL);
    if (!on)
        uxq_cat(k[3].value, sizeof k[3].value, UXQ_WORD[UXQ_W_NONE]);
    else if (!m)
        uxq_cat(k[3].value, sizeof k[3].value, UXQ_XW[2]);
    else {
        uxq_cat(k[3].value, sizeof k[3].value, m > 0 ? UXQ_WORD[UXQ_W_PLUS] : UXQ_XW[4]);
        uxq_num(k[3].value, sizeof k[3].value, m > 0 ? m : -m);
    }
    k[3].bar = (uint8_t)((m - MICRO_MIN) * 255 / (MICRO_MAX - MICRO_MIN));
    k[3].off = (uint8_t)(!on || !m);
}
/* the knob row of SEQ page pg (0: the page shown); 0: not one drawn here with its own (PATTERN: ux_section.c's, its settings') */
static __attribute__((noinline)) int uxq_knobs(const page_t *pg, ux_knob_t k[4])
{
    uint32_t s = uxq_step(), len = uxq_len(), kind = pg ? uxq_page(pg) : (uint32_t)uxq_owns();
    const step_t *st = &TSEL->step[s];
    if (kind != UXQ_STEPS && kind != UXQ_EXTRAS)
        return 0;
    if (kind == UXQ_STEPS && lock_held()) {             /* (a step held: on STEPS only, lock_held) */
        uxq_lock_knobs(k);
        return 1;
    }
    if (kind == UXQ_EXTRAS) {
        uxq_extras_knobs(k);
        return 1;
    }
    k[0].label = UXQ_LABEL[UXQ_L_STEP];
    uxq_stepn(k[0].value, sizeof k[0].value, 1);
    k[0].bar = (uint8_t)(len > 1u ? s * 255u / (len - 1u) : 255u);
    if (drum_track(TSEL)) {
        uint32_t l = ui.lane % NLANE, h = grid_hit_of(st, l), a = (step_accents(st) >> l) & 1u;
        k[1].label = UXQ_LABEL[UXQ_L_LANE];
        str_cpy(k[1].value, drum_lane_name(TSEL, l), sizeof k[1].value);
        k[1].bar = (uint8_t)(l * 255u / (NLANE - 1u));
        k[2].label = UXQ_LABEL[UXQ_L_HIT];
        str_cpy(k[2].value, UXQ_HIT[h], sizeof k[2].value);
        k[2].bar = (uint8_t)(h * 255u / LM_N);
        k[2].off = (uint8_t)!h;
        k[3].label = UXQ_LABEL[UXQ_L_ACC];
        str_cpy(k[3].value, UXQ_WORD[a ? UXQ_W_ON : UXQ_W_OFF], sizeof k[3].value);
        k[3].bar = (uint8_t)(a ? 255u : 0u);
        k[3].off = (uint8_t)!a;
        return 1;
    }
    {
        uint32_t f = uxq_flag(st), g = uxq_gate(st, s);
        k[1].label = UXQ_LABEL[UXQ_L_NOTE];
        uxq_note(st, k[1].value, sizeof k[1].value);
        if (text_w(&UF_VAL, k[1].value) > UX_COL_W - 4) {   /* (a chord's "+2" past the column, "A#-1+2": the note; the status
                                                         * line says it whole) */
            char *p = k[1].value;
            while (*p && *p != '+')
                p++;
            *p = 0;
        }
        k[1].bar = (uint8_t)(st->time == ST_NOTE && st->n ? st->note[0] * 255u / 127u : 0u);
        k[1].off = (uint8_t)!(st->time == ST_NOTE && st->n);
        k[2].label = UXQ_LABEL[UXQ_L_FLAG];
        str_cpy(k[2].value, UXQ_FLAG[f], sizeof k[2].value);
        k[2].bar = (uint8_t)(f * 85u);
        k[2].off = (uint8_t)!f;
        k[3].label = UXQ_LABEL[UXQ_L_GATE];
        uxq_gate_txt(st, s, k[3].value, sizeof k[3].value, 0);
        k[3].bar = (uint8_t)(g ? (g > 16u ? 255u : g * 255u / 16u) : 0u);
    }
    return 1;
}
/* the status line for knob c just turned, in words: "STEP 5 . G2", "NOTE G#2", "SLIDE ON", "GATE TIE", "GATE 3 STEPS"; a DRUM
 * track's "STEP 5 . SNARE ON", "LANE SNARE", "HIT X2", "ACCENT ON"; a step held: the lock's "NAME . VALUE"; STEP EXTRAS'
 * "CHANCE 65%", "MICRO +4/64". 0: none (PATTERN: ux_section.c's) */
static __attribute__((noinline)) const char *uxq_say(uint32_t c, char *b, uint32_t n)
{
    uint32_t s = uxq_step(), i;
    const step_t *st = &TSEL->step[s];
    int drum = drum_track(TSEL), lk = lock_held() != 0;
    ux_knob_t k[4];
    for (i = 0; i < 4u; i++) { k[i].label = ""; k[i].value[0] = 0; k[i].bar = 0; k[i].off = 0; }
    c &= 3u;
    b[0] = 0;
    if (!uxq_knobs(0, k) || !k[c].label || !k[c].label[0])
        return 0;
    if (!lk && uxq_owns() == UXQ_EXTRAS) {              /* (task 3) "RATCHET X2", "CHANCE 65%", "CONDITION 1ST OF 4", "MICRO +4/64" */
        uxq_cat(b, n, k[c].label);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SP]);
        uxq_cat(b, n, k[c].value);
        if (c == 3u && ST_MICRO(st) && uxq_on(st))
            uxq_cat(b, n, UXQ_XW[3]);
        return b;
    }
    if (!lk && c == 0u) {                               /* STEP: "STEP " and the spelled step to its second part ("5 . G2", */
        char *p;                                        /* "5 . SNARE ON") */
        uxq_cat(b, n, UXQ_WORD[UXQ_W_STEP]);
        uxq_spell(b + str_len(b), n - str_len(b));
        for (p = b, i = 0; *p && i < 2u; p++)
            i += *p == UXQ_WORD[UXQ_W_DOT][1];
        if (i == 2u)
            p[-2] = 0;                                  /* (the " . " before the third) */
        return b;
    }
    if (!lk && c == 2u && !drum) {                      /* ACC/SLD in words */
        uxq_cat(b, n, UXQ_FLAG_SAY[uxq_flag(st)]);
        return b;
    }
    if (lk && ui.entry_open && !grid_on() && (ui.ent_arp ? c < 3u : c == 3u))
        lk = 0;                                         /* (task 4: a chord's own: "LENGTH 3", "ORDER UP") */
    if (lk) {                                           /* the lock card's (spec 2.5): "LOCK . NAME . VALUE" */
        uxq_cat(b, n, UXQ_WORD[UXQ_W_LOCKDOT]);
        uxq_cat(b, n, k[c].label);
        uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
    } else if (c == 3u && drum) {
        uxq_cat(b, n, UXQ_WORD[UXQ_W_ACCENT]);          /* (the grid's ACC: "ACCENT ON") */
    } else {
        uxq_cat(b, n, k[c].label);                      /* "NOTE G#2", "GATE TIE", "LANE SNARE", "HIT X2" */
        uxq_cat(b, n, UXQ_WORD[UXQ_W_SP]);
    }
    if (!lk && !drum && c == 1u)
        uxq_note(st, b + str_len(b), n - str_len(b));   /* (NOTE: a chord's "+2" whole) */
    else if (!lk && !drum && c == 3u)
        uxq_gate_txt(st, s, b + str_len(b), n - str_len(b), 1);   /* (GATE: "3 STEPS") */
    else
        uxq_cat(b, n, k[c].value);
    return b;
}

/* ------------------------------------------------------------------------------------- SEQ > MORE's views (task 4) --- */
/* (params.c ID_SQ_*; spec 2.4) the inherited SEQ pages' controls as MORE rows: their names, their values in plain words (COMP's
 * TYPEs: ui.c COMP_*, the knob's order; EVERY: core.h EV_*, the knob's EV_ORD; PLAYHEADS: fugue.c's) */
static void uxq_signed(char *b, uint32_t n, int32_t v)  /* "+3", "-2", "0" */
{
    if (v > 0)
        uxq_cat(b, n, UXQ_WORD[UXQ_W_PLUS]);
    uxq_num(b, n, v);
}
/* component slot k's TYPE as the list shows it (COMP's: its CHANCE and RATCH remembered a step, ui.c comp_view1) */
static uint32_t uxq_cview(uint32_t k) { return k ? comp_ui(TSEL->step[ui.cursor % NSTEP].comp2) : comp_view1(); }
static const char *uxq_cname(uint32_t k, uint32_t which)   /* .. its first / second value's name ("": none, no row) */
{
    uint32_t v = uxq_cview(k);
    if (which == 1u)
        return UXQ_VAL1[v];
    if (which == 3u)                                    /* (WALK's, OCT's, an ARP's RANGE: MODE / ORDER's own row; RATCH's */
        return v == COMP_WALK || v == COMP_OCT || v == COMP_ARPN || v == COMP_ARPQ ? UXQ_VNAME[ID_SQ_RNG - ID_SQ_STRETCH] :
               v == COMP_RATCH ? UXQ_PSTEP : "";        /* PITCH STEP, its RATE's own) */
    return v == COMP_RATCH || v == COMP_ARPN || v == COMP_WALK || v == COMP_OCT ? UXQ_VNAME[ID_SQ_HRATE - ID_SQ_STRETCH] :
           v == COMP_ARPQ && !k ? "LAP" : "";
}
static __attribute__((noinline)) void uxq_cval(uint32_t k, uint32_t which, char *b, uint32_t n)   /* .. its value in words */
{
    const step_t *st = &TSEL->step[ui.cursor % NSTEP];
    uint32_t v = uxq_cview(k), a = k ? st->carg2 : st->carg;
    int32_t i = (int32_t)a;
    if (which == 2u) {
        if (v == COMP_RATCH) {                          /* the hits' spacing (FIT: the step's) */
            char r[8];
            if (CT_DIV(st))
                rat_rate_name(TSEL, CT_DIVS[CT_DIV(st) % CT_NDIV], r);
            else
                str_cpy(r, UXQ_VW[UXQ_V_FIT], sizeof r);
            uxq_cat(b, n, r);
        } else if (v == COMP_ARPN) {
            uxq_cat(b, n, N_DIV[ARP_DIVI[CP_RATE(a)] % 4u]);
        } else if (v == COMP_ARPQ) {                    /* LAP: the note it plays this pass of the pattern */
            uint32_t lap = song.playing ? TSEL->loops : 0u, c = COMP_CP[v];
            uint8_t kn[4 + NLANE];
            if (CP_ORD(a) == 3u)
                uxq_cat(b, n, UXQ_WAY[3]);
            else if (drum_track(TSEL))
                uxq_cat(b, n, cp_kit_arp(TSEL, st, c, a, lap, kn) ? drum_lane_abbr(TSEL, drum_lane(kn[0])) : UXQ_WORD[UXQ_W_NONE]);
            else
                note_name(b + str_len(b), cp_arp_note(TSEL, st, c, a, lap));
        } else {                                        /* WALK, OCT: a move every so many loops */
            uint32_t r = CP_WRATES[CP_WRATE(a) % CP_NWRATE];
            uxq_num(b, n, (int32_t)r);
            uxq_cat(b, n, UXQ_VW[r == 1u ? UXQ_V_LOOP : UXQ_V_LOOPS]);
        }
        return;
    }
    if (which == 3u && v == COMP_RATCH) {               /* PITCH STEP: each hit a degree up, down, an octave up every other */
        static const char *const PN[4] = {"OFF", "+1 DEG", "-1 DEG", "+OCT"};
        uxq_cat(b, n, PN[rat_pit_of(st, k) & 3u]);
        return;
    }
    switch (v) {
    case COMP_CHANCE: uxq_num(b, n, (int32_t)step_chance(st)); uxq_cat(b, n, UXQ_XW[1]); break;
    case COMP_RATCH: {                                  /* x1, x2..x8 and the shape (COMP's list, rat_ui) */
        uint32_t r = rat_ui(st, k);
        uxq_cat(b, n, UXQ_XW[0]);
        uxq_num(b, n, r ? 2 + (int32_t)((r - 1u) % 7u) : 1);
        if (r > 7u)
            uxq_cat(b, n, UXQ_VW[r > 14u ? UXQ_V_RISE : UXQ_V_FADE]);
        break;
    }
    case COMP_TRIG: uxq_cat(b, n, UXQ_COND[1u + a % CP_NTRIG]); break;
    case COMP_WALK: case COMP_OCT:                      /* MODE ("UP"); its RANGE (which 3) "5 DEG", "2 OCT" */
        if (which == 3u) {
            uxq_num(b, n, v == COMP_OCT ? (int32_t)CP_WRNG(a) + 1 : (int32_t)CP_WDEGS[CP_WRNG(a) & 3u]);
            uxq_cat(b, n, UXQ_VW[v == COMP_OCT ? UXQ_V_OCT : UXQ_V_DEG]);
        } else
            uxq_cat(b, n, UXQ_WAY[CP_WMODE(a) & 3u]);
        break;
    case COMP_ARPN: case COMP_ARPQ:                     /* ORDER ("UP"); its RANGE "2 OCT" */
        if (which == 3u) {
            uxq_num(b, n, (int32_t)CP_RNG(a) + 1);
            uxq_cat(b, n, UXQ_VW[UXQ_V_OCT]);
        } else
            uxq_cat(b, n, UXQ_WAY[CP_ORD(a) & 3u]);
        break;
    case COMP_RAND: uxq_num(b, n, i); uxq_cat(b, n, UXQ_VW[UXQ_V_DEG]); break;
    case COMP_TONAL: uxq_signed(b, n, i - 7); uxq_cat(b, n, UXQ_VW[UXQ_V_DEG]); break;
    case COMP_BEND: uxq_signed(b, n, i - 12); uxq_cat(b, n, UXQ_WORD[UXQ_W_SP]); uxq_cat(b, n, UXQ_WORD[UXQ_W_ST]); break;
    case COMP_PULSE: uxq_num(b, n, 1 + i % 4); uxq_cat(b, n, UXQ_VW[UXQ_V_STEPS]); break;
    case COMP_JUMP: uxq_num(b, n, (int32_t)CP_JTO(a) + 1); break;
    default: break;
    }
}
static uint32_t uxq_vk(uint32_t id) { return id == ID_SQ_VALB || id == ID_SQ_VAL2B || id == ID_SQ_RNGB; }   /* (a value view's slot, */
static uint32_t uxq_vw(uint32_t id) { return id == ID_SQ_VAL || id == ID_SQ_VALB ? 1u : id == ID_SQ_RNG || id == ID_SQ_RNGB ? 3u : 2u; }   /* which) */
static int uxq_vshown(uint32_t id)
{
    const step_t *st = &TSEL->step[ui.cursor % NSTEP];
    switch (id) {
    case ID_SQ_VAL: case ID_SQ_VAL2: case ID_SQ_VALB: case ID_SQ_VAL2B: case ID_SQ_RNG: case ID_SQ_RNGB:
        return uxq_cname(uxq_vk(id), uxq_vw(id))[0] != 0;
    case ID_SQ_EVERY:                                   /* (the step's components' loops: shown with one) */
        return st->comp != CP_NONE || st->comp2 != CP_NONE;
    case ID_SQ_FOCUS: case ID_SQ_PITCH: case ID_SQ_MOVE:   /* (NOTE's: a kit's steps are hits, STEPS' and the grid's) */
        return !drum_track(TSEL);
    default:
        return sqv_is(id);
    }
}
static __attribute__((noinline)) const char *uxq_vtext(uint32_t id, char *val, uint32_t nv, uint8_t *bar)
{
    const track_t *t = TSEL;
    const step_t *st = &t->step[ui.cursor % NSTEP];
    const fg_set_t *f = &fg_set[song.sel % NTRK][fg_ui % FG_HEADS];
    const char *nm = sqv_is(id) ? UXQ_VNAME[id - ID_SQ_STRETCH] : "";
    val[0] = 0;
    *bar = 0;
    switch (id) {
    case ID_SQ_STRETCH: {                               /* x1 as it was, /8 .. /2, x2 .. x8 (ui.c shp_k) */
        int32_t k = shp_k();
        uxq_cat(val, nv, k > 0 ? UXQ_XW[0] : UXQ_WORD[UXQ_W_SLASH]);
        uxq_num(val, nv, k > 0 ? k : -k);
        *bar = (uint8_t)(shp_i * 255u / 14u);
        break;
    }
    case ID_SQ_REPEAT: {
        uint32_t r = shp_rep();
        if (r == 1u)
            uxq_cat(val, nv, UXQ_VW[UXQ_V_OFF]);
        else if (r) {
            uxq_cat(val, nv, UXQ_XW[0]);
            uxq_num(val, nv, (int32_t)r);
        } else
            uxq_cat(val, nv, UXQ_VW[UXQ_V_TO64]);
        *bar = (uint8_t)(shp_r * 255u / 8u);
        break;
    }
    case ID_SQ_NUDGE: uxq_cat(val, nv, UXQ_VW[UXQ_V_LR]); break;
    case ID_SQ_HEAD: uxq_num(val, nv, (int32_t)(fg_ui % FG_HEADS) + 2); *bar = (uint8_t)(fg_ui % FG_HEADS * 127u); break;
    case ID_SQ_HRATE: {                                 /* (fugue.c N_FGRATE, its x in capitals) */
        char *p;
        uxq_cat(val, nv, N_FGRATE[f->rate % FG_NRATE]);
        for (p = val; *p; p++)
            if (*p == 'x')
                *p = 'X';
        *bar = (uint8_t)(f->rate % FG_NRATE * 255u / (FG_NRATE - 1u));
        break;
    }
    case ID_SQ_HDIR: uxq_cat(val, nv, UXQ_HDIR[f->dir & 3u]); break;
    case ID_SQ_HIVL: uxq_signed(val, nv, f->ivl); uxq_cat(val, nv, UXQ_VW[UXQ_V_DEG]); *bar = (uint8_t)((f->ivl + 14) * 255 / 28); break;
    case ID_SQ_TYPE: case ID_SQ_TYPE2: uxq_cat(val, nv, UXQ_TYPE[uxq_cview(id == ID_SQ_TYPE2)]); break;
    case ID_SQ_VAL: case ID_SQ_VAL2: case ID_SQ_VALB: case ID_SQ_VAL2B: case ID_SQ_RNG: case ID_SQ_RNGB:
        nm = uxq_cname(uxq_vk(id), uxq_vw(id));
        uxq_cval(uxq_vk(id), uxq_vw(id), val, nv);
        break;
    case ID_SQ_EVERY: uxq_cat(val, nv, UXQ_EVERY[ST_EVERY(st) % EV_N]); break;
    case ID_SQ_FOCUS: case ID_SQ_PITCH: {               /* (NOTE's: the chord or one of its notes, its pitch) */
        uint32_t on = st->time == ST_NOTE && st->n, fc = on && ui.pr_focus <= st->n ? ui.pr_focus : 0u;
        if (id == ID_SQ_PITCH) {
            if (on)
                note_name(val, st->note[fc ? fc - 1u : 0u]);
            else
                uxq_cat(val, nv, UXQ_WORD[UXQ_W_NONE]);
        } else if (fc) {
            uxq_cat(val, nv, UXQ_VW[UXQ_V_NOTE]);
            uxq_num(val, nv, (int32_t)fc);
        } else
            uxq_cat(val, nv, UXQ_VW[on && st->n > 1u ? UXQ_V_CHORD : UXQ_V_ALL]);
        break;
    }
    case ID_SQ_MOVE: uxq_stepn(val, nv, 0); break;
    case ID_SQ_PLAY: uxq_cat(val, nv, UXQ_VW[motion_enabled(t) ? UXQ_V_ON : UXQ_V_OFF]); break;
    case ID_SQ_EVENTS: uxq_num(val, nv, (int32_t)(motion_count(t) - motion_lock_count(t))); break;
    case ID_SQ_LOCKS: uxq_num(val, nv, (int32_t)motion_lock_count(t)); break;
    default: {                                          /* AUTO LIST's (ui_events.c): ROW STEP SETTING VALUE; the last row ADD */
        uint8_t idx[MOTION_MAX];
        uint32_t n = ev_rows(idx), r = ev_row(n), step, pid;
        int16_t v;
        if (r < n) {
            const motion_event_t *e = &motion.event[idx[r]];
            step = MOTION_STEP(e);
            pid = MOTION_ID(e);
            v = MOTION_VAL(e);
            if (id == ID_SQ_EVAL)
                nm = UXQ_WORD[(motion.event[idx[r]].param & MOTION_LOCK) ? UXQ_W_LOCK : UXQ_W_AUTO];
        } else {
            ev_fix();
            step = ui.ev_step;
            pid = ui.ev_id;
            v = pid < P_COUNT ? motion_base_value(t, pid) : 0;
        }
        if (id == ID_SQ_ROW) {
            if (r < n) {
                uxq_num(val, nv, (int32_t)r + 1);
                uxq_cat(val, nv, UXQ_WORD[UXQ_W_SLASH]);
                uxq_num(val, nv, (int32_t)n);
            } else
                uxq_cat(val, nv, UXQ_VW[UXQ_V_ADD]);
        } else if (id == ID_SQ_ESTEP)
            uxq_num(val, nv, (int32_t)step + 1);
        else if ((pid >= P_COUNT && pid != MOT_DJF) || !ev_id_ok(t, pid))
            uxq_cat(val, nv, UXQ_WORD[UXQ_W_NONE]);
        else if (id == ID_SQ_EPARAM) {
            char b[12];
            ev_name(t, pid, b);
            uxq_cat(val, nv, b);
        } else
            uxs_vtext(track_desc(t, pid), v, 0, pid, val, nv);
        break;
    }
    }
    return nm;
}

static const ux_row_t *uxq_more_row(void)
{
    const page_t *pg = cur_page();
    return !ui.home && !ui.menu && pg->more == &UXM_SEQ ? uxm_lock_row(pg) : 0;
}
static uint32_t uxq_more_kind(void)
{
    const ux_row_t *r = uxq_more_row();
    uint32_t h = r ? uxm_head_of(&UXM_SEQ, (uint32_t)(r - UXM_SEQ.rows)) : UXM_H_N;
    if (r && r->kind == UXR_TRACK && r->id >= ID_SQ_ROW && r->id <= ID_SQ_EVAL)
        return UXQ_MK_LIST;
    return h == UXM_H_MUTATE ? UXQ_MK_MUTATE : h == UXM_H_AUTO ? UXQ_MK_AUTO : h == UXM_H_STEP ? UXQ_MK_STEP : UXQ_MK_NONE;
}
static int uxq_more_busy(void)
{
    const ux_row_t *r = uxq_more_row();
    return r && uxm_head_of(&UXM_SEQ, (uint32_t)(r - UXM_SEQ.rows)) == UXM_H_TRACK;
}
static const char *const UXQ_MSAY[] = {"OCT+ ADD A LOCK", "OCT+ LOCK OR AUTO", "OCT+ KEEP  GLO+A4 FREEZE", "REC + PLAY THEN TURN A KNOB"};
static const char *uxq_more_say(void)                   /* the status line on SEQ > MORE: what OCT+ does there (the pages' hints) */
{
    switch (uxq_more_kind()) {
    case UXQ_MK_LIST: {
        uint8_t idx[MOTION_MAX];
        uint32_t n = ev_rows(idx);
        return UXQ_MSAY[ev_row(n) < n];
    }
    case UXQ_MK_MUTATE: return UXQ_MSAY[2];
    case UXQ_MK_AUTO: return UXQ_MSAY[3];
    default: return 0;
    }
}

/* (task 4) the status line while a step is held and no knob turned: "LOCK . STEP 5" (the card's: spec 2.5); 0 none held */
static const char *uxq_lock_line(char *b, uint32_t n)
{
    uint64_t held = uxq_owns() == UXQ_STEPS ? lock_held() : 0u;
    uint32_t st = 0;
    if (!held)
        return 0;
    while (st < NSTEP && !((held >> st) & 1u))
        st++;
    b[0] = 0;
    uxq_cat(b, n, UXQ_WORD[UXQ_W_LOCKDOT]);
    uxq_cat(b, n, UXQ_WORD[UXQ_W_STEP]);
    uxq_num(b, n, (int32_t)st + 1);
    return b;
}

/* .. the footer while a step is held (0.9.12's, ui_input.c lock_select: SELECT walks the pages the knobs lock): "LOCK . SOUND",
 * its place "2 / 5" (out of the walk: ENV LFO FX's page, HOME's four; none). 0: none held */
static int uxq_lock_foot(char *w, uint32_t nw, char *c, uint32_t nc)
{
    uint8_t v[16];
    uint32_t at, m;
    if (uxq_owns() != UXQ_STEPS || !lock_held())
        return 0;
    m = lock_chain(v, &at);
    w[0] = c[0] = 0;
    uxq_cat(w, nw, UXQ_WORD[UXQ_W_LOCKDOT]);
    uxq_cat(w, nw, at < m ? nv_name(SEC_EDIT, v[at], ui.plk_src - 1u) : ui.plk_src ? PAGES[ui.plk_src - 1u].title : SEC_NAME[SEC_HOME]);
    if (at < m) {
        uxq_num(c, nc, (int32_t)at + 1);
        uxq_cat(c, nc, " / ");
        uxq_num(c, nc, (int32_t)m);
    }
    return 1;
}

/* ------------------------------------------------------------------------------------------------------- the pictures --- */
static uint16_t uxq_lane_col(uint32_t l) { return UX_BLEND[(l % NLANE) * 255u / (NLANE - 1u)]; }
/* the strip (ux_widgets.c ux_strip) for steps first..first+15: a TIE first carries its note's colour from before; the step playing
 * lit and the picked one outlined where they are in these 16; a chord being entered in step recording, KNOB 1 its length: the
 * steps it will hold outlined in ACCENT (the inherited roll's) */
static void uxq_strip(int y, uint32_t first)
{
    const track_t *t = TSEL;
    uint32_t len = uxq_len(), i, pick = uxq_step(), s = seq_src(t, t->seq_idx), n16 = first < len ? len - first : 0u;
    int drum = drum_track(t);
    uint16_t prev = UX_EMPTY;
    if (first && first < len && t->step[first].time == ST_TIE) {
        uint32_t src = pr_src(t, first, len);
        prev = src < NSTEP ? ux_step_col(&t->step[src], UX_EMPTY, drum) : UX_EMPTY;
    }
    ux_strip(y, &t->step[first < NSTEP ? first : 0u], n16 < 16u ? n16 : 16u, pick >= first && pick < first + 16u ? (int32_t)(pick - first) : -1,
             song.playing && s < len && s >= first && s < first + 16u ? (int32_t)(s - first) : -1, prev, drum);
    for (i = 0; ui.entry_open && !ui.ent_arp && ui.ent_nn && ui.ent_len > 1u && i < 16u; i++) {
        uint32_t si = first + i;
        if (si != pick && si < len && (si + len - pick) % len < ui.ent_len)
            cv_frame(UXQ_X(i), y, UX_STRIP_W, UX_STRIP_H, UX_ACCENT);
    }
}
/* the columns' common parts: past LEN dark, the playing step a light column, the beats a line before them */
static void uxq_cols(int32_t y0, int32_t h, uint32_t ncol, int32_t ph)
{
    uint32_t i;
    for (i = 4u; i < 16u; i += 4u)
        cv_rect(UXQ_X(i) - 1, y0, 1, h, UX_LINE);
    for (i = ncol; i < 16u; i++)
        cv_rect(UXQ_X(i), y0, UX_STRIP_W, h, UX_BG2);
    if (ph >= 0)
        cv_rect(UXQ_X(ph), y0, UX_STRIP_W, h, ux_mix(UX_BG, UX_TEXT, 14));
}
static void uxq_marks(int32_t y0, int32_t h, uint32_t ncol)   /* the step locks (a mark at the foot), the picked step outlined */
{
    uint64_t locks = motion_lock_steps(trk_index(TSEL));
    uint32_t i, f = uxq_first(), pick = uxq_step();
    for (i = 0; i < ncol; i++)
        if ((locks >> (f + i)) & 1u)
            cv_rect(UXQ_X(i) + 3, y0 + h - 3, UX_STRIP_W - 6, 2, UX_VIOLET_LT);
    if (pick >= f && pick < f + 16u)
        cv_frame(UXQ_X(pick - f) - 1, y0, UX_STRIP_W + 2, h, UX_TEXT);
}
static __attribute__((noinline)) void uxq_roll(int y0, int h)
{
    const track_t *t = TSEL;
    uint32_t len = uxq_len(), f = uxq_first(), ncol = f < len ? (len - f < 16u ? len - f : 16u) : 0u, i, j;
    int32_t lo = 127, hi = -1, span, rh, top, ph = uxq_ph();
    for (i = 0; i < ncol; i++) {                        /* (the notes of the 16 shown, a TIE's its note's) */
        uint32_t s = pr_src(t, f + i, len);
        for (j = 0; s < NSTEP && j < 4u + NLANE; j++) {
            int32_t n = pr_note(&t->step[s], j);
            if (n < 0)
                continue;
            lo = n < lo ? n : lo;
            hi = n > hi ? n : hi;
        }
    }
    if (hi < 0)
        lo = hi = 60;                                   /* (none shown: no row is drawn, the view is moot; fix 1: not last_note,
                                                         * which every key played moves) */
    span = hi - lo + 1;
    if (span < 12) {                                    /* (an octave at least, the notes centred) */
        lo -= (12 - span) / 2;
        span = 12;
    }
    rh = h / span;
    if (rh < 2) {                                       /* (too wide: two rows a px, the picked step's note in the middle) */
        const step_t *ps = &t->step[uxq_step()];
        rh = 2;
        span = h / 2;
        lo = (ps->time == ST_NOTE && ps->n ? ps->note[0] : (lo + hi) / 2) - span / 2;
    }
    if (rh > 6)
        rh = 6;
    lo = clamp(lo, 0, 128 - span);
    top = y0 + (h - span * rh) / 2;
    uxq_cols(y0, h, ncol, ph);
    for (i = 0; i < ncol; i++) {
        uint32_t si = f + i, s = pr_src(t, si, len), nx = (si + 1u) % len;
        const step_t *st, *ns = &t->step[nx];
        int32_t x = UXQ_X(i), w = UX_STRIP_W;
        int slide;
        uint16_t c;
        if (s == NSTEP)
            continue;                                   /* (a REST, empty, a TIE after one) */
        st = &t->step[s];
        slide = (st->flags & SF_SLIDE) && step_ratchet(st) == 1u && ns->time == ST_NOTE && step_on(ns) && i + 1u < ncol;   /* (as
                                                         * the sequencer: no slide out of a ratchet) */
        c = st->flags & SF_ACCENT ? UX_ACCENT : st->flags & SF_SLIDE ? UX_SLIDE : UX_ACID;
        if (s != si) {                                  /* (a TIE: its note carried on through the gap before) */
            x -= 1;
            w += 1;
        }
        if (ns->time == ST_TIE && i + 1u < ncol)        /* (held on: into the gap after) */
            w += 1;
        for (j = 0; j < 4u + NLANE; j++) {
            int32_t n = pr_note(st, j), y;
            if (n < 0)
                continue;
            y = top + (lo + span - 1 - clamp(n, lo, lo + span - 1)) * rh;
            pr_split(x, y, w, rh > 2 ? rh - 1 : rh, (n < lo || n >= lo + span) ? UX_DIM(c, 50) : c, s == si ? step_ratchet(st) : 1u);
            if (slide && j == 0u) {                     /* (a slide: joined to the next note, in SLIDE) */
                int32_t yn = top + (lo + span - 1 - clamp((int32_t)ns->note[0], lo, lo + span - 1)) * rh, a = y < yn ? y : yn,
                        b = y < yn ? yn : y;
                cv_rect(UXQ_X(i) + UX_STRIP_W - 2, a, 3, b - a + (rh > 2 ? rh - 1 : rh), UX_SLIDE);
            }
        }
    }
    uxq_marks(y0, h, ncol);
}
static __attribute__((noinline)) void uxq_grid(int y0, int h)
{
    const track_t *t = TSEL;
    uint32_t len = uxq_len(), f = uxq_first(), ncol = f < len ? (len - f < 16u ? len - f : 16u) : 0u, i, l, sel = ui.lane % NLANE;
    int32_t lp = h / (int32_t)NLANE, ph = uxq_ph();
    cv_rect(UXQ_X(0) - 2, y0 + (int32_t)sel * lp, UXQ_X(15) + UX_STRIP_W + 2 - (UXQ_X(0) - 2), lp, UX_BG2);   /* (the lane picked) */
    uxq_cols(y0, h, ncol, ph);
    for (l = 0; l < NLANE; l++) {
        int32_t y = y0 + (int32_t)l * lp + 1;
        uint16_t c = uxq_lane_col(l);
        for (i = 0; i < ncol; i++) {
            const step_t *st = &t->step[f + i];
            int32_t x = UXQ_X(i);
            if ((step_lanes(st) >> l) & 1u)
                pr_split(x + 1, y, UX_STRIP_W - 2, lp - 2, (step_accents(st) >> l) & 1u ? ux_mix(c, UX_TEXT, 55) : c, step_ratchet(st));
            else
                cv_rect(x + UX_STRIP_W / 2 - 1, y + lp / 2 - 2, 2, 2, l == sel || i % 4u == 0u ? UX_MID : UX_LINE);
        }
    }
    uxq_marks(y0, h, ncol);
}
/* (task 3) STEP EXTRAS' top: the 16 shown as columns, each step that plays filled from the foot as high as its chance, cut in its
 * ratchets' parts (pr_split, as the roll), its micro timing a line from the column's middle to where it plays (late: right) */
static __attribute__((noinline)) void uxq_extras(int y0, int h)
{
    const track_t *t = TSEL;
    uint32_t len = uxq_len(), f = uxq_first(), ncol = f < len ? (len - f < 16u ? len - f : 16u) : 0u, i;
    int32_t ch = h - 6, drum = drum_track(t), ek;
    uint32_t sh;
    uxq_cols(y0, h, ncol, uxq_ph());
    for (i = 0; i < ncol; i++) {
        const step_t *st = &t->step[f + i];
        int32_t x = UXQ_X(i), fh, m = ST_MICRO(st), cx = x + UX_STRIP_W / 2, mx;
        if (!uxq_on(st))
            continue;
        fh = ch * (int32_t)step_chance(st) / 100;
        pr_split(x, y0 + h - (fh > 1 ? fh : 1), UX_STRIP_W, fh > 1 ? fh : 1, ux_step_col(st, UX_EMPTY, drum), uxq_ratch(st, &sh, &ek));
        mx = cx + m * UX_STRIP_PITCH / (MICRO_MAX - MICRO_MIN + 1);
        if (m)
            cv_rect(m > 0 ? cx : mx, y0 + 1, (m > 0 ? mx - cx : cx - mx) + 1, 2, UX_TEXT);
    }
    uxq_marks(y0, h, ncol);
}
/* (task 3) PATTERN: its steps as cells, rows of 16 (four: 64 at most; past LEN dark), each as wide as the GATE, an offbeat moved late
 * by SWING; the playing one lit */
static __attribute__((noinline)) void uxq_pattern(void)
{
    const track_t *t = TSEL;
    uint32_t len = uxq_len(), i;
    int32_t sw = clamp(t->p[P_SSWING], 0, 100) * UXQ_SWING / 100, w = 2 + (UXQ_PW - 2) * clamp(t->p[P_SGATE], 1, 127) / 127,
            ph = uxq_ph(), drum = drum_track(t);
    uint16_t prev = UX_EMPTY;
    for (i = 0; i < NSTEP; i++) {
        int32_t x = UXQ_X(i % 16u), y = UXQ_PY0 + (int32_t)(i / 16u * UXQ_PROW);
        uint16_t c;
        if (i >= len) {
            cv_rect(x, y, UX_STRIP_W, UXQ_PH, UX_BG2);
            continue;
        }
        c = ux_step_col(&t->step[i], prev, drum);
        if (t->step[i].time != ST_TIE)
            prev = c;
        if ((int32_t)i == ph)
            c = c == UX_EMPTY ? UX_LINE : ux_mix(c, UX_TEXT, 45);
        cv_rect(x + (i & 1u ? sw : 0), y, w, UXQ_PH, c);
    }
}
static void uxq_extras_line(char *b, uint32_t n)        /* STEP EXTRAS' line: the picked step's condition, "5 . 1ST OF 4" */
{
    uxq_stepn(b, n, 0);
    uxq_cat(b, n, UXQ_WORD[UXQ_W_DOT]);
    {
        uint32_t cd = uxq_cond(&TSEL->step[uxq_step()]);
        uxq_cat(b, n, cd == UXQ_FULL ? UXQ_XW[5] : UXQ_COND[cd]);
    }
}
static __attribute__((noinline)) void uxq_picture(const page_t *pg)
{
    char b[40];
    int32_t x = UX_STRIP_X;
    uint32_t kind = uxq_page(pg);
    if (kind == UXQ_PATT) {
        uxq_pattern();
        b[0] = 0;
        uxq_num(b, sizeof b, (int32_t)uxq_len());
        uxq_cat(b, sizeof b, UXQ_WORD[UXQ_W_STEPS]);
        uxq_cat(b, sizeof b, UXQ_WORD[UXQ_W_DOT]);
        uxq_cat(b, sizeof b, N_DIV[clamp(TSEL->p[P_SDIV], 0, (int32_t)NELEM(N_DIV) - 1)]);
    } else {
        if (kind == UXQ_EXTRAS)
            uxq_extras(UXQ_RY0, UXQ_RH);
        else if (drum_track(TSEL))
            uxq_grid(UXQ_RY0, UXQ_RH);
        else
            uxq_roll(UXQ_RY0, UXQ_RH);
        uxq_strip(UXQ_STRIP_Y, uxq_first());
        if (kind == UXQ_EXTRAS)
            uxq_extras_line(b, sizeof b);
        else
            uxq_spell(b, sizeof b);
    }
    GFX_HOOK_ALIGN(0, UXQ_SAY_Y + UF_HDR_CAP_Y, 0, UXQ_SAY_Y + UF_HDR_CAP_Y + UF_HDR_CAP_H, AL_V | AL_PASS, "SEQ's line spelled");
    cv_text_on(x, UXQ_SAY_Y, &UF_HDR, b, UX_TEXT, ux_under(x, UXQ_SAY_Y + 4));
}
/* what the picture is drawn from but the playhead (that one paced: ux_section_draw): the steps, LEN, the step picked and its 16,
 * the lane, the locks, a DRUM track or not, a chord being entered (fix 1: not last_note, which a key played moves and an empty
 * 16 never shows) */
static __attribute__((noinline)) uint32_t uxq_sig(void)
{
    const track_t *t = TSEL;
    uint64_t locks = motion_lock_steps(trk_index(t));
    uint32_t h = ((steps_hash(t) * 31u + uxq_len()) * 131u + uxq_step()) * 257u + ui.lane * 7u + (uint32_t)drum_track(t) * 3u +
                 (uint32_t)(locks ^ (locks >> 32)) * 977u +
                 (ui.entry_open * 2u + ui.ent_arp + ui.ent_nn * 4u + ui.ent_len * 64u) * 40503u,   /* (the chord's length outlined) */
             i;
    if (uxq_owns() == UXQ_EXTRAS)                       /* (task 3) the chances, the conditions: the components (not in steps_hash) */
        for (i = 0; i < NSTEP; i++) {
            const step_t *st = &t->step[i];
            h = (h ^ (st->probability | (uint32_t)st->comp << 8 | (uint32_t)st->carg << 16 | (uint32_t)st->comp2 << 24)) * 16777619u;
            h = (h ^ st->carg2) * 16777619u;
        }
    return h * 31u + (uint32_t)t->p[P_SSWING] * 131u + (uint32_t)t->p[P_SGATE] * 7u + (uint32_t)t->p[P_SDIV];   /* (PATTERN's) */
}
