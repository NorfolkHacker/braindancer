/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* Parameter descriptors, formatting and the page table. */
static const char *const N_LWAVE[] = {"SIN", "TRI", "SAW", "SQR", "S&H"};
/* (JIANT 0.6.51) the LFO's RATE below 0: in time with the tempo, -1 the slowest (next to the slowest free rate) */
static const char *const N_LSYNC[12] = {"4BAR", "2BAR", "1BAR", "1/2", "1/4", "1/8D", "1/4T", "1/8", "1/8T", "1/16",
                                        "1/16T", "1/32"};
static const char *const N_AMODE[] = {"OFF", "UP", "DN", "UPDN", "RND", "ORD", "REPEAT",   /* (append-only: seq.c AM_*) */
                                       "DNUP", "UP+8", "CONV", "DIVG", "PINKY", "THUMB", "WALK", "CHORD",
                                       "TRNS"};                         /* (JIANT: seq.c AM_TRNS) */
static const char *const N_DIV[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR"};
/* (JIANT 0.4) the delay's TIME: N_DIV's ten, then free times past the last division (5 ms .. 1.48 s, exponential;
 * fx.c delay_samples, DTIME_MS); (0.8.4) appended up to the longest the line holds at a quarter rate (the values
 * before them mean what they did) */
#define DT_FREE 10
/* (0.8.4) the delay's line (fx.c dly_buf): DLY_LEN samples of int16, the one constant of its size. 32768 (64 KB): the full
 * rate to 0.74 s, half to 1.49 s, a quarter to 2.97 s; 65536 (128 KB): 1.49 / 2.97 / 5.94 s */
#ifndef DLY_LEN
#define DLY_LEN 32768u
#endif
static const char *const N_DTIME[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR", "5ms", "6ms", "7ms", "8ms", "9ms", "10ms", "11ms", "12ms", "13ms", "15ms", "17ms", "19ms", "21ms", "24ms", "27ms", "31ms", "35ms", "39ms", "44ms", "50ms", "56ms", "64ms", "72ms", "81ms", "91ms", "103ms", "116ms", "131ms", "148ms", "167ms", "189ms", "213ms", "241ms", "272ms", "307ms", "346ms", "391ms", "441ms", "498ms", "562ms", "634ms", "716ms", "808ms", "912ms", "1.03s", "1.16s", "1.31s", "1.48s",
                                      /* (0.8.4) past the line at the full rate (fx.c dly_lo): half rate to 2.97 s, then a quarter */
                                      "1.67s", "1.89s", "2.13s", "2.40s", "2.71s"
#if DLY_LEN >= 65536u
                                      , "3.06s", "3.45s", "3.90s", "4.40s", "4.97s", "5.61s", "5.90s"
#else
                                      , "2.95s"
#endif
                                      };
static const uint16_t DTIME_MS[NELEM(N_DTIME) - DT_FREE] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 17, 19, 21, 24, 27, 31, 35, 39, 44, 50, 56, 64, 72, 81, 91, 103, 116, 131, 148, 167, 189, 213, 241, 272, 307, 346, 391, 441, 498, 562, 634, 716, 808, 912, 1029, 1162, 1311, 1480,
                                      1670, 1885, 2128, 2402, 2711
#if DLY_LEN >= 65536u
                                      , 3060, 3454, 3899, 4400, 4967, 5606, 5900
#else
                                      , 2950
#endif
                                      };
static const char *const N_SCALE[] = {"CHR", "MAJ", "MIN", "DOR", "MIX", "PEN", "MPEN", "HARM",
                                    "PHRY", "LYD", "LOC", "MEL", "BLUES", "WHOLE", "DIMHW", "DIMWH"};
static const char *const N_ONOFF[] = {"OFF", "ON"};
/* (ACID FM 0.6) PLAYS (G_PLAYS1 ..) and STEP EXTRAS' CONDITION (ux_seq.c): ALWAYS (none), then core.h CP_TRIGS' a of b loops,
 * the first loop alone, all but it */
static const char *const N_PLAYS[CP_NTRIG + 1u] = {"ALWAYS", "1ST OF 2", "EVERY 2ND", "1ST OF 3", "2ND OF 3", "EVERY 3RD",
                                                   "1ST OF 4", "2ND OF 4", "3RD OF 4", "EVERY 4TH", "1ST OF 8", "1ST ONLY",
                                                   "NOT 1ST"};
static const char *const N_DLYTYPE[] = {"ANALOG", "DIGITAL", 0};   /* (0.6.90) DLY 3 > TYPE */
static const char *const N_PFXRES[] = {"1/8", "1/16", "1/32"};     /* (0.6.99) FX > PUNCH: the lane's step */
/* seq.c kb_map; 1 = SNAP (stored projects: the former ON); 3 = SEQ: SNAP, and the sequencer's notes snap too as
 * they play (seq.c seq_step; the steps keep what was written). Append-only: older projects hold 0..2 */
static const char *const N_QUANT[] = {"OFF", "SNAP", "WHITE", "SEQ"};
static const char *const N_DTYPE[] = {"SOFT", "HARD", "FOLD", "CRUSH", "RECT", "AMP"};   /* (JIANT) fx.c DT_*; 0.7.8 AMP */
static const char *const N_FTYPE[] = {"LP", "HP", "BP", "COMB"};   /* (JIANT 0.5) P_FTYPE: eng_analog.c FT_* */
static const char *const N_VOICE[] = {"POLY", "MONO", "LEG", "UNI"};   /* V_POLY .. V_UNISON */
static const char *const N_GLMODE[] = {"RATE", "TIME"};
static const char *const N_PRIO[] = {"LAST", "LOW", "HIGH"};
static const char *const N_ALLOC[] = {"ROT", "REUSE"};
static const char *const N_ORDER[] = {"NOTE", "PLAY"};
static const char *const N_CLOCK[] = {"INT", "USB", "TRS"};
static const char *const N_MIDI_INPUT[] = {"USB", "TRS"};
static const char *const N_ROUTE[] = {"CH1-6", "SEL"};   /* MIDI IN (seq.c): 1..6 -> parts, 7..16 ignored / all -> selected */
static const char *const N_NOTE[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const N_DASH[] = {"--"};
static const char *const N_RTYPE[] = {"ROOM", "SPRING", "SHIMR", "RESO", "CLOUD"};   /* (JIANT 0.5: + 3) */   /* G_RTYPE: the reverb bus's model (fx.c) */
static const char *const N_GO[] = {"--", "GO"};
static const char *const N_SLCR[] = {"OFF", "GATE", "STUT"};             /* SL_OFF .. SL_STUT (slicer.c) */
static const char *const N_SLDIV[] = {"1/8", "1/16", "1/32", "8T", "16T", "32T"};   /* SL_DEN */
static const char *const N_SLPQ[] = {"CHR", "SCL"};                      /* (JIANT 0.6.3) the SLICER's pitch steps */
static const char *const N_APSYNC[] = {"FREE", "BEAT"};                  /* (JIANT 0.6.9) AUTO PAN's RATE: Hz / the tempo */
static const char *const N_APDIV[] = {"4BAR", "2BAR", "1BAR", "1/2", "1/4", "1/8", "1/16", "1/32"};   /* .. BEAT (fx.c AP_SIXT) */
/* modulation matrix (mod.c): sources, destinations (E1..E8 = P_E0..P_E7: shown with the engine's labels) */
static const char *const N_MSRC[] = {"OFF", "LFO", "ENV", "VEL", "KEY", "RAND", "MODW", "AT", "EXPR",
                                     "M1", "M2", "M3", "M4", "STEP", "NOISE"};   /* (0.7.8) NOISE */   /* (JIANT: the macros, LFO held; the modulation sequence) */
static const char *const N_MDST[] = {"OFF", "PITCH", "CUT", "SHP", "AMP", "PAN", "DIST", "CHO", "DLY", "REV", "RATE",
                                     "VIB", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8",
                                     "CLIP", "PNCH",   /* (JIANT: the master, FX > MASTER) */
                                     "D.TIM", "D.FB", "D.TON", "D.MIX", "R.SIZ", "R.DMP", "C.RAT", "C.DEP"};   /* (0.7.0) the FX buses'
                                                         * globals (DELAY TIME FDBK TONE MIX, REVERB SIZE DAMP, CHORUS RATE
                                                         * DEPTH): mod.c macro_master, fx.c GFX() */
static const char *const N_ENGNAME[] = {"ANALOG", FELUCCA_FM4 ? "DIGITAL" : "-", "PHASE", "LOFI", "-", "VOICE", "-", "-", "-", "-",
                                             "DRUM", "NOISE", "FM6", "-", "-"};   /* (JIANT: SAMPLE 4, GRAIN 8, SLICE 13 retired;
                                                         * ACID FM 0.2: WHEEL 7, ANIMAL 14) */

#define PD(l, f, mn, mx, df) {l, f, mn, mx, df, 0, 0}
#define PE(l, n, df) {l, F_ENUM, 0, (int16_t)(sizeof(n) / sizeof(n[0]) - 1), df, n, 0}

static const param_desc_t TP[P_COUNT] = {
    [P_LEVEL] = PD("LEVEL", F_DB, 0, 127, 104),
    [P_ATK] = PD("ATK", F_TIME, 0, 127, 10),
    [P_DEC] = PD("DEC", F_TIME, 0, 127, 70),
    [P_SUS] = PD("SUS", F_PCT, 0, 127, 90),
    [P_REL] = PD("REL", F_TIME, 0, 127, 60),
    [P_ED_FLT] = PD("FILTER", F_BIPCT, -64, 63, 0),
    [P_ED_PIT] = PD("PITCH", F_BIPCT, -64, 63, 0),
    [P_ED_SHP] = PD("SHAPE", F_BIPCT, -64, 63, 0),
    [P_ED_FX] = PE("LOOP", N_ONOFF, 0),          /* (JIANT 0.4, P_ELOOP) ON: held, the ENV starts again at the sustain */
    [P_LRATE] = PD("RATE", F_LFOHZ, -12, 127, 60),  /* (JIANT 0.6.51) below 0 in time: 4BAR .. 1/32 (N_LSYNC) */
    [P_LWAVE] = PE("WAVE", N_LWAVE, 0),
    [P_LPHASE] = PD("PHASE", F_INT, -1, 127, 0),   /* (JIANT 0.6.51) -1: FREE, never restarted by a note (0.9.18: nor its
                                                     * FADE); the pages show it as MODE TRIG / FREE and PHASE 0..127 */
    [P_LFADE] = PD("FADE", F_TIME, 0, 127, 0),
    [P_LD_PIT] = PD("PITCH", F_BIPCT, -64, 63, 0),
    [P_LD_FLT] = PD("FILTER", F_BIPCT, -64, 63, 0),
    [P_LD_SHP] = PD("SHAPE", F_BIPCT, -64, 63, 0),
    [P_LD_AMP] = PD("AMP", F_PCT, 0, 127, 0),
    [P_AMODE] = PE("MODE", N_AMODE, 0),
    [P_ARATE] = PE("RATE", N_DIV, 2),
    [P_AOCT] = PD("OCT", F_INT, 1, 4, 1),
    [P_AGATE] = PD("GATE", F_PCT, 1, 127, 64),
    [P_ASWING] = PD("SWING", F_PCT, 0, 100, 0),
    [P_APROB] = PD("PROB", F_PCT, 0, 127, 127),
    [P_AHOLD] = PE("HOLD", N_ONOFF, 0),
    [P_AORDER] = PE("ORDER", N_ORDER, 0),
    [P_ROOT] = PD("ROOT", F_NOTE, 0, 11, 0),
    [P_SCALE] = PE("SCALE", N_SCALE, 0),
    [P_QUANT] = PE("QUANT", N_QUANT, 0),
    [P_TRANS] = PD("TRANS", F_SEMI, -24, 24, 0),
    [P_SLEN] = PD("LEN", F_STEPS, 1, NSTEP, 16),
    [P_SDIV] = PE("DIV", N_DIV, 2),
    [P_SSWING] = PD("SWING", F_PCT, 0, 100, 0),
    [P_SGATE] = PD("GATE", F_PCT, 1, 127, 64),
    [P_DIST] = PD("DIST", F_PCT, 0, 127, 0),
    [P_CHOR] = PD("CHORUS", F_PCT, 0, 127, 0),
    [P_DLY] = PD("DLY", F_PCT, 0, 127, 0),
    [P_REV] = PD("REV", F_PCT, 0, 127, 0),
    [P_VOICE] = PE("VOICE", N_VOICE, 0),
    [P_GLIDE] = PD("GLIDE", F_TIME, 0, 127, 0),
    [P_GLMODE] = PE("MODE", N_GLMODE, 0),
    [P_PRIO] = PE("PRIO", N_PRIO, 0),
    [P_ALLOC] = PE("ALLOC", N_ALLOC, 0),
    [P_DETUNE] = PD("DETUNE", F_INT, 0, 127, 40),
    [P_PAN] = PD("PAN", F_BIPCT, -64, 63, 0),
    [P_MUTE] = PE("MUTE", N_ONOFF, 0),
    [P_APDEP] = PD("DEPTH", F_PCT, 0, 127, 0),       /* (JIANT 0.6.9) AUTO PAN (core.h: the old SLICER's slots) */
    [P_APRATE] = PD("RATE", F_LFOHZ, 0, 127, 2),    /* (BEAT: 0..7, page_desc AP_BEAT) */
    [P_APWAVE] = PE("WAVE", N_LWAVE, 1),
    [P_APSYNC] = PE("SYNC", N_APSYNC, 1),
#define MSLOT(k) [P_M##k##SRC] = PE("SRC" #k, N_MSRC, 0), [P_M##k##DST] = PE("DEST" #k, N_MDST, 0), \
                 [P_M##k##AMT] = PD("AMT" #k, F_BIPCT, -64, 63, 0)
    MSLOT(1), MSLOT(2), MSLOT(3), MSLOT(4),
#undef MSLOT
/* the OP ENV / OP LEVEL values of DIGITAL (ids 61..80): with FELUCCA_FM4 its operator envelopes; without it
 * inert, on no page and in no editor layout, read only when a DIGITAL sound converts (fm4_convert.c). Their labels
 * stay: the editor's library files key parameters by label ("ATK#2" ..), so an old file's values still find them */
#define FMOP(k) [P_FM##k##_ATK] = PD("ATK", F_TIME, 0, 127, 0), \
                [P_FM##k##_DEC] = PD("DEC", F_TIME, 0, 127, 0), \
                [P_FM##k##_SUS] = PD("SUS", F_PCT, 0, 127, 127), \
                [P_FM##k##_REL] = PD("REL", F_TIME, 0, 127, 0), \
                [P_FM##k##_LEVEL] = PD("LEVEL", F_PCT, 0, 127, 127)
    FMOP(1), FMOP(2), FMOP(3), FMOP(4),
#undef FMOP
    [P_SOFS] = PD("ROTATE", F_INT, -32, 32, 0),    /* the sequence played OFS steps later (seq.c seq_src; 0.6.92: was OFFSET) */
    [P_POFS] = PD("PITCH", F_SEMI, -24, 24, 0),   /* its notes PIT semitones up (not on a kit) */
/* the DRUM engine's lane levels (eng_drum.c, #97): 100 % = the kit as designed (the sound before 1.1) */
#define LANE(k, l) [P_LN0 + (k)] = PD(l, F_PCT, 0, 127, 127)
    LANE(0, "KICK"), LANE(1, "SNARE"), LANE(2, "CLAP"), LANE(3, "HATCL"),
    LANE(4, "HATOP"), LANE(5, "TOM"), LANE(6, "RIM"), LANE(7, "BELL"),
#undef LANE
    [P_DTYPE] = PE("TYPE", N_DTYPE, 0),         /* (JIANT) DIST: SOFT (as before) HARD FOLD CRUSH RECT AMP */
    [P_DTONE] = PD("TONE", F_BIPCT, -64, 63, 0),   /* .. its tone: darker / brighter */
    [P_MSLEN] = PD("LEN", F_STEPS, 1, 16, 16),  /* (JIANT) the modulation sequence (mod.c MS_STEP) */
    [P_MSDIV] = PE("DIV", N_DIV, 2),
    [P_MSSLW] = PD("SLEW", F_PCT, 0, 127, 0),
#define MSTEP(k) [P_MS0 + (k)] = PD("LEVEL", F_PCT, 0, 127, 0)
    MSTEP(0), MSTEP(1), MSTEP(2), MSTEP(3), MSTEP(4), MSTEP(5), MSTEP(6), MSTEP(7),
    MSTEP(8), MSTEP(9), MSTEP(10), MSTEP(11), MSTEP(12), MSTEP(13), MSTEP(14), MSTEP(15),
#undef MSTEP
    [P_FTYPE] = PE("TYPE", N_FTYPE, 0),         /* (JIANT 0.5) the filter: LP (as before) HP BP COMB */
    [P_FCUT] = PD("CUT", F_CUTOFF, 0, 127, 127),  /* .. the other engines' (on the part): open, */
    [P_FRES] = PD("RES", F_PCT, 0, 127, 0),       /* no resonance: out of the chain (its drive: DIST, the part's) */
};

static const param_desc_t GP[G_COUNT] = {
    [G_BPM] = PD("BPM", F_BPM, 40, 240, 120),
    [G_SWING] = PD("SWING", F_PCT, 0, 100, 0),
    [G_CLOCK] = PE("CLOCK", N_CLOCK, 0),
    [G_TUNE] = PD("TUNE", F_INT, -50, 50, 0),
    [G_DTIME] = PE("TIME", N_DTIME, 1),                 /* (JIANT 0.4: past the divisions, free times) */
    [G_DFDBK] = PD("FEEDBK", F_PCT, 0, 120, 60),
    [G_DCOLOR] = PD("TONE", F_PCT, 0, 127, 64),        /* (JIANT 0.4, was COLR + HPF) the repeats: below 64 darker, above
                                                        * thinner (64 open) */
    [G_DMIX] = PD("MIX", F_PCT, 0, 127, 90),
    [G_RSIZE] = PD("SIZE", F_PCT, 0, 127, 90),
    [G_RDAMP] = PD("DAMP", F_PCT, 0, 127, 60),
    [G_CRATE] = PD("RATE", F_LFOHZ, 0, 127, 40),
    [G_CDEPTH] = PD("DEPTH", F_PCT, 0, 127, 60),
    [G_MIDI] = PE("MIDI", N_MIDI_INPUT, 0),
    [G_DREL] = PD("REL", F_PCT, 0, 100, 30),     /* DUCK's release: 40 .. 600 ms (fx.c duck_block) */
    [G_ROUTE] = PE("ROUTE", N_ROUTE, 0),          /* (was "--": stored 0 = CH1-4, as MIDI IN always was) */
    [G_INFO] = PD("CPU", F_INT, 0, 0, 0),
    [G_SLOT] = PD("SLOT", F_INT, 1, 4, 1),
    [G_DJF] = PD("DJ FILT", F_BIPCT, -64, 63, 0),   /* (0.9.1) the master DJ FILTER: 0 off, - low-pass, + high-pass (fx.c) */
    [G_LOAD] = PE("LOAD", N_GO, 0),
    [G_SAVE] = PE("SAVE", N_GO, 0),
    [G_ENGSEL] = PE("ENG", N_ENGNAME, 0),
    [G_CLIP] = PD("CLIP", F_PCT, 0, 100, 0),     /* the master clipper: drive x1 .. x4 into the soft clip */
    [G_CLRSEQ] = PE("CLRSQ", N_GO, 0),
    [G_INITSND] = PE("INIT", N_GO, 0),
    /* the reverb's model on the REVERB page: the id of the old GM drum channel (G_DRCH, inert since 1.0) */
    [G_RTYPE] = PE("TYPE", N_RTYPE, 0),
    /* (JIANT) the drum bus and the kick's ducking (fx.c, eng_drum.c); the ids of the GM drum part's level and
     * reverb send, inert since 1.0 */
    [G_PUNCH] = PD("PUNCH", F_PCT, 0, 100, 0),    /* DRUM fattened: up to +5 dB the first 4 ms, the body +3 dB (0.5) */
    [G_DUCK] = PD("DUCK", F_PCT, 0, 100, 0),     /* the kick ducks the other parts: up to -18 dB */
    /* (JIANT, FUNB) the buses: not in a project's header (project.c: after the punch-in lane) */
    [G_CSTER] = PD("STEREO", F_BIPCT, -64, 63, 0), /* (0.6.9) the CHORUS's stereo: 0 as before, -64 mono, 63 ~2x (was G_DHPF) */
    [G_WIDTH] = PD("WIDE", F_PCT, 0, 127, 0),    /* the delay's right echo up to 1/32 later, the chorus's taps apart */
    [G_DTYPE] = {"TYPE", F_ENUM, 0, 1, 0, N_DLYTYPE, 0},   /* (0.6.90) ANALOG (saturating, resonant, past 100 % grows) / DIGITAL */
    [G_DPING] = {"PING", F_ENUM, 0, 1, 0, N_ONOFF, 0},   /* (0.6.90) PING-PONG: the repeats left, right, left .. */
    [G_PFXLEN] = PD("LEN", F_INT, 1, 64, 64),    /* (0.6.99) the punch-in lane: its steps and their size */
    [G_PFXRES] = {"RES", F_ENUM, 0, 2, 1, N_PFXRES, 0},
    [G_DMOD] = PD("MOD", F_PCT, 0, 127, 0),      /* (0.6.89: 0, the repeats exact copies, by default) */     /* (0.6.86) the delay's drift: WOW + FLUTTER (fx.c dly_wow_adv) */
    [G_RMOD] = PD("MOD", F_PCT, 0, 127, 0),      /* the reverb's modulation depth (ROOM: the combs, SPRING: its wobble) */
    [G_RRATE] = PD("RATE", F_LFOHZ, 0, 127, 30), /* .. its rate */
    [G_RPRE] = PD("PRE", F_INT, -24, 100, 0),    /* the reverb's pre-delay, ms (0.6.12: SHIMMER / CLOUD's AIR, -24..24, below 0) */
    [G_RFILT] = PD("FILT", F_BIPCT, -64, 63, 0), /* (JIANT 0.3) the reverb's input: darker (low-pass) / brighter (low cut) */
    [G_RWIDE] = PD("WIDE", F_PCT, 0, 127, 0),    /* .. its width: its combs (ROOM) / taps (SPRING) apart, left and right */
    [G_DPIT] = PD("PITCH", F_SEMI, -12, 12, 0),  /* the GRAIN delay: each repeat through two grains this far up / down */
    [G_DSPRY] = PD("SPRAY", F_PCT, 0, 127, 0),    /* .. their spray: each grain from a random place (0: plain delay) */
    [G_STRN] = PD("ALL TR", F_INT, -14, 14, 0),  /* (JIANT 0.4) every sequence this many steps up / down its scale (0.6.45:
                                                  * named ALL TR, was STRN: nobody knew what moved their notes) */
    [G_SLMODE] = PE("MODE", N_SLCR, 0),         /* (JIANT 0.6.3) the SLICER bus (slicer.c) */
    [G_SLPAT] = PD("PAT", F_INT, 1, 16, 1),
    [G_SLRATE] = PE("RATE", N_SLDIV, 1),
    [G_SLDEP] = PD("DEPTH", F_PCT, 0, 127, 127),
    [G_SLT1] = PE("T1", N_ONOFF, 1), [G_SLT1 + 1] = PE("T2", N_ONOFF, 1), [G_SLT1 + 2] = PE("T3", N_ONOFF, 1),
    [G_SLT1 + 3] = PE("T4", N_ONOFF, 1), [G_SLT1 + 4] = PE("T5", N_ONOFF, 1), [G_SLT6] = PE("T6", N_ONOFF, 1),
    [G_SLFX] = PE("FX", N_ONOFF, 1),            /* the delay, reverb and chorus returns: on, the tails cut too */
    [G_SLPLEN] = PD("LEN", F_INT, 1, 16, 16),
    [G_SLPQ] = PE("QUANT", N_SLPQ, 1),            /* the pitch steps: every semitone / the scale's */
    [G_SLP0] = PD("PITCH", F_SEMI, -24, 24, 0), [G_SLP0 + 1] = PD("PITCH", F_SEMI, -24, 24, 0),
    [G_SLP0 + 2] = PD("PITCH", F_SEMI, -24, 24, 0), [G_SLP0 + 3] = PD("PITCH", F_SEMI, -24, 24, 0),
    [G_SLP0 + 4] = PD("PITCH", F_SEMI, -24, 24, 0), [G_SLP0 + 5] = PD("PITCH", F_SEMI, -24, 24, 0),
    [G_SLP0 + 6] = PD("PITCH", F_SEMI, -24, 24, 0), [G_SLP0 + 7] = PD("PITCH", F_SEMI, -24, 24, 0),
    [G_SLP0 + 8] = PD("PITCH", F_SEMI, -24, 24, 0), [G_SLP0 + 9] = PD("PITCH", F_SEMI, -24, 24, 0),
    [G_SLP0 + 10] = PD("PITCH", F_SEMI, -24, 24, 0), [G_SLP0 + 11] = PD("PITCH", F_SEMI, -24, 24, 0),
    [G_SLP0 + 12] = PD("PITCH", F_SEMI, -24, 24, 0), [G_SLP0 + 13] = PD("PITCH", F_SEMI, -24, 24, 0),
    [G_SLP0 + 14] = PD("PITCH", F_SEMI, -24, 24, 0), [G_SLP15] = PD("PITCH", F_SEMI, -24, 24, 0),
    /* (ACID FM 0.5) MIDI per track (seq.c): IN, OUT, CHANNEL; KEYBOARD PLAYS SELECTED (project.c: block 'MR') */
#define MRT(k) [G_MRIN1 + (k)] = PE("IN", N_ONOFF, 1), [G_MROUT1 + (k)] = PE("OUT", N_ONOFF, 1), \
               [G_MRCH1 + (k)] = PD("CHANNEL", F_INT, 1, 16, (k) + 1)
    MRT(0), MRT(1), MRT(2), MRT(3), MRT(4), MRT(5),
#undef MRT
    [G_MRSEL] = PE("SELECTED", N_ONOFF, 0),
    /* (ACID FM 0.6) PLAYS per track (seq.c trk_plays_on; project.c: block 'PL'; SEQ > MORE: ID_SQ_PLAYS, the selected one's) */
    [G_PLAYS1] = PE("PLAYS", N_PLAYS, 0), [G_PLAYS1 + 1] = PE("PLAYS", N_PLAYS, 0), [G_PLAYS1 + 2] = PE("PLAYS", N_PLAYS, 0),
    [G_PLAYS1 + 3] = PE("PLAYS", N_PLAYS, 0), [G_PLAYS1 + 4] = PE("PLAYS", N_PLAYS, 0), [G_PLAYS6] = PE("PLAYS", N_PLAYS, 0),
    /* (Braindancer 0.8) TAPE (fx.c tape_block; project.c: block 'TP'): AGE, the macro; its five details. All 0: out of
     * the chain */
    [G_TAGE] = PD("AGE", F_PCT, 0, 100, 0), [G_TWOW] = PD("WOW", F_PCT, 0, 100, 0),
    [G_TFLUT] = PD("FLUTTER", F_PCT, 0, 100, 0), [G_TSAT] = PD("SAT", F_PCT, 0, 100, 0),
    [G_THISS] = PD("HISS", F_PCT, 0, 100, 0), [G_TTONE] = PD("TONE", F_PCT, 0, 100, 0),
};

static const param_desc_t LOFI_RTRG = {"RTRG", F_ONOFF, 0, 1, 0, 0, 0};   /* (0.6.12) P_LRTRG on a LOFI track */
static const param_desc_t TRK_CHANCE = {"CHANCE", F_PCTINV, 0, 127, 0, 0, 0};   /* (0.6.37) P_TCHAN: 0 = 100 % */
static const param_desc_t MORPH_DESC = {"MORPH", F_PCT, 0, 127, 0, 0, 0};   /* (0.6.45) P_MORPH */
static const param_desc_t DXIMG_DESC = {"IMAGE", F_INT, 0, 127, 0, 0, 0};       /* (0.6.48) P_DXIMG */
static const char *const N_MUTBARS[] = {"OFF", "1", "2", "4", "8", "16", "32"};
static const char *const N_MUTMODE[] = {"ORBIT", "DRIFT"};
static const param_desc_t MUT_DESC[4] = {                /* (0.6.43) MUTATE: P_MSND P_MPAT P_MAMT P_MMODE */
    {"SOUND", F_ENUM, 0, 6, 0, N_MUTBARS, 0}, {"PATTERN", F_ENUM, 0, 6, 0, N_MUTBARS, 0},
    {"AMOUNT", F_PCT, 0, 127, 0, 0, 0}, {"MODE", F_ENUM, 0, 1, 0, N_MUTMODE, 0}};
static const char *const N_CHRD[] = {"OFF", "TRIAD", "7TH", "ADD9", "SUS2", "SUS4", "MAJ", "MIN", "7", "MAJ7", "MIN7",
                                     "POWER", 0};
static const char *const N_CVOI[] = {"CLOSE", "LEAD", "OPEN", "+BASS", 0};
static const param_desc_t CHORD_DESC[3] = {              /* (0.6.81) SCL > CHORD: P_CHRD P_CVOI P_CSTR */
    {"CHORD", F_ENUM, 0, 11, 0, N_CHRD, 0}, {"VOICING", F_ENUM, 0, 3, 0, N_CVOI, 0}, {"STRUM", F_PCT, 0, 127, 0, 0, 0}};
static const param_desc_t HUM_DESC[3] = {                /* (0.6.87) SEQ > HUMAN: P_HVEL P_HTIM P_HGAT (127 = 0 %) */
    {"HUMAN", F_PCTINV, 0, 127, 127, 0, 0}, {"TIME", F_PCTINV, 0, 127, 127, 0, 0}, {"GATE", F_PCTINV, 0, 127, 127, 0, 0}};   /* (0.6.92:
                                                         * one amount, P_HVEL; TIME and GATE stay stored, 127) */
static const param_desc_t RECQ_DESC = {"REC Q", F_PCT, 0, 127, 127, 0, 0};   /* (0.6.88) P_HRQ: 100 % on the grid */
static const param_desc_t DRIFT_DESC = {"DRIFT", F_PCTINV, 0, 127, 127, 0, 0};   /* (0.7.8) P_DRIFT: 127 = 0 % */
static const param_desc_t WROT_DESC[2] = {               /* (0.7.8) WHEEL's ROTOR: P_WRDEP (127 = 100 %, as before), P_WDUST */
    {"DEPTH", F_PCT, 0, 127, 127, 0, 0}, {"DUST", F_PCTINV, 0, 127, 127, 0, 0}};
static const param_desc_t VLANG_DESC = {"LANG", F_ENUM, 0, LG_N - 1, 0, N_LANG, 0};   /* (0.8.5) VOICE's words: */
static const param_desc_t VLYR_DESC = {"LYRIC", F_ENUM, 0, LYR_N - 1, 0, N_LYRIC, 0};  /* P_VLANG, P_VLYR (sing.c) */
static const param_desc_t *track_desc(const track_t *t, uint32_t id)
{
    if (id == MOT_DJF)                                /* (0.9.1) a DJ FILTER record (motion.c): the global's */
        return &GP[G_DJF];
    if ((id == P_VLANG || id == P_VLYR) && ENGINES[eng_idx(t->eng_req)] == &ENG_FORMANT)
        return id == P_VLANG ? &VLANG_DESC : &VLYR_DESC;
    if (id >= P_E0 && id <= P_E7) {                   /* the engine asked for (t->engine follows after a fade) */
        const engine_t *e = ENGINES[eng_idx(t->eng_req)];
        const param_desc_t *d = e->desc ? e->desc(t, id - P_E0) : 0;   /* a mode-dependent label / names */
        return d ? d : &e->edit[id - P_E0];
    }
    if (id == P_LRTRG && ENGINES[eng_idx(t->eng_req)] == &ENG_LOFI)   /* (0.6.12) LOFI's RTRG in DIGITAL's slot */
        return &LOFI_RTRG;
    if (id == P_TCHAN && !(FELUCCA_FM4 && t->eng_req % NENGINES == ENGI_DIGITAL))   /* (0.6.37) SHIFT's CHANCE */
        return &TRK_CHANCE;
    if (!(FELUCCA_FM4 && t->eng_req % NENGINES == ENGI_DIGITAL))   /* (0.6.43) MUTATE's four */
        switch (id) {
        case P_MSND: return &MUT_DESC[0];
        case P_MPAT: return &MUT_DESC[1];
        case P_MAMT: return &MUT_DESC[2];
        case P_MMODE: return &MUT_DESC[3];
        case P_MORPH: return &MORPH_DESC;
        case P_DXIMG: return &DXIMG_DESC;
        case P_CHRD: return &CHORD_DESC[0];
        case P_CVOI: return &CHORD_DESC[1];
        case P_CSTR: return &CHORD_DESC[2];
        case P_HVEL: return &HUM_DESC[0];
        case P_HTIM: return &HUM_DESC[1];
        case P_HGAT: return &HUM_DESC[2];
        case P_HRQ: return &RECQ_DESC;
        case P_DRIFT: return &DRIFT_DESC;
        case P_WRDEP: return &WROT_DESC[0];
        case P_WDUST: return &WROT_DESC[1];
        default: break;
        }
    return &TP[id];
}

/* the descriptor of parameter id for engine e without the engine's desc hook (the device display
 * only; range and default are the same): what the editor protocol, user presets and projects use */
static const param_desc_t *param_desc_of(uint32_t e, uint32_t id)
{
    return id >= P_E0 ? &ENGINES[e]->edit[id - P_E0] : &TP[id];
}

/* a retired F_ENUM value kept as an alias (the sample engines' SET / SRC aliases left with them in JIANT): none now.
 * -> the value v stands for */
static int32_t enum_orig(const param_desc_t *d, int32_t v)
{
    (void)d;
    return v;
}

/* a stored value as the parameter takes it (projects, user presets, motion): inside d's range. FM6's SLOT is never
 * stored as a bank slot (JIANT 0.6.11, from Felucca 1.4.1): 9..40 (B1..B32) is 8, OWN, and the patch comes with what is
 * loaded */
static int32_t param_fit(const param_desc_t *d, int32_t v)
{
    v = clamp(v, d->min, d->max);
    return d->names == N_FM6_PATCH && v > (int32_t)FM6_OWN ? (int32_t)FM6_OWN : v;
}

/* v is a value a knob passes over: an alias, or one of FM6's bank slots without a voice */
static int enum_skip(const param_desc_t *d, int32_t v)
{
    return enum_orig(d, v) != v || (d->names == N_FM6_PATCH && !fm6_slot_ok(v));
}
/* a knob moved an F_ENUM from `from` to v: past any value it skips in that direction; with none left to the end, the
 * last one it does not skip on the way from `from` (else `from`) */
static int32_t enum_step(const param_desc_t *d, int32_t from, int32_t v)
{
    int32_t dir = v > from ? 1 : -1, w = v;
    while (w != from && enum_skip(d, w))
        w = w + dir > d->max || w + dir < d->min ? from : w + dir;
    while (w == from && v != from && enum_skip(d, v))
        v -= dir;
    return w == from ? v : w;
}

/* #48: the note divisions in the order of their length, longest first (triplets between their neighbours), on the
 * knobs and the gauges; the stored values (N_DIV, N_SLDIV indices: projects, presets, the editor protocol) stay.
 * -> the shown order of d's values (index: position, entry: value), 0 = the values' own order */
static const uint8_t DIV_ORDER[10] = {9, 8, 7, 6, 0, 1, 4, 2, 5, 3};   /* 4BAR 2BAR 1/1 1/2 1/4 1/8 8T 1/16 16T 1/32 */
static const uint8_t SLDIV_ORDER[6] = {0, 3, 1, 4, 2, 5};              /* 1/8 8T 1/16 16T 1/32 32T */
static const uint8_t RWAVE_ORDER[7] = {5, 6, 0, 1, 2, 3, 4};   /* (JIANT 0.4) LOFI's WAVE: BYTE first (0.5.1: FLOAT
                                                         * next), then PLS .. WRAM */
static const uint8_t DTIME_ORDER[NELEM(N_DTIME)] = {9, 8, 7, 6, 0, 1, 4, 2, 5, 3, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                                                 58, 59, 60, 61, 62
#if DLY_LEN >= 65536u
                                                 , 63, 64, 65, 66, 67, 68, 69
#else
                                                 , 63
#endif
                                                 };   /* the divisions as DIV_ORDER,
                                                         * then the free times, short to long */
static const uint8_t *enum_order(const param_desc_t *d)
{
    return d->names == N_DIV ? DIV_ORDER : d->names == N_SLDIV ? SLDIV_ORDER : d->names == N_DTIME ? DTIME_ORDER :
           d->names == N_RWAVE ? RWAVE_ORDER : 0;
}
static int32_t enum_rank(const param_desc_t *d, int32_t v)   /* v's place in the shown order (+ min): the gauges */
{
    const uint8_t *o = enum_order(d);
    int32_t r;
    if (d->fmt == F_PCTINV) return d->max + d->min - v;   /* (the gauge full at 0) */
    for (r = 0; o && r < d->max - d->min; r++)
        if (o[r] == v - d->min)
            break;
    return o ? r + d->min : v;
}
/* a knob turned `steps` (signed) from v: clamped, over the shown order, past aliases (enum_step) */
/* (0.9.17) a value with names (F_INT, its range split evenly over them: engine desc hooks): its name's index, *k the
 * names */
static uint32_t name_of(const param_desc_t *d, int32_t v, uint32_t *k)
{
    uint32_t n = 0;
    while (d->names[n])
        n++;
    *k = n;
    return (uint32_t)(clamp(v, d->min, d->max) - d->min) * n / (uint32_t)(d->max - d->min + 1);
}
static int32_t param_turn(const param_desc_t *d, int32_t v, int32_t steps)
{
    const uint8_t *o = enum_order(d);
    uint32_t k, i, w = (uint32_t)(d->max - d->min + 1);
    if (d->fmt == F_PCTINV) steps = -steps;         /* (right: more chance, less skipped) */
    if (o)
        return o[clamp(enum_rank(d, v) - d->min + steps, 0, d->max - d->min)] + d->min;
    if (d->fmt == F_INT && d->names && (i = name_of(d, v, &k), k < w)) {   /* (0.9.17) a name a detent (VOICE's
                                                         * THEME was 16 detents a name), onto its middle */
        i = (uint32_t)clamp((int32_t)i + steps, 0, (int32_t)k - 1);
        return d->min + (int32_t)((2u * i + 1u) * w / (2u * k));
    }
    return enum_step(d, v, clamp(v + steps, d->min, d->max));
}

static uint8_t fmt_named;        /* the last param_format was a name (F_ENUM, even "4"): ui_draw.c's digits do not roll it */

/* value string (<= 5 chars) and unit for a parameter value */
static void param_format(const param_desc_t *d, int32_t v, char *val, const char **unit)
{
    *unit = "";
    fmt_named = d->fmt == F_ENUM;
    if (d == &FMT_WDESC[0] && v <= 0) {                 /* (0.8.5) VOICE's words: POS 0, each note the next */
        str_cpy(val, "AUTO", 6);
        fmt_named = 1;
        return;
    }
    if (d == &TP[P_LPHASE] && v < 0) {                  /* (0.6.51) the LFO's PHASE FREE */
        str_cpy(val, "FREE", 6);
        fmt_named = 1;
        return;
    }
    switch (d->fmt) {
    case F_PCT:                                     /* a 0..100 range (SWG) is its value, others a share of 127 */
        fmt_int(val, d->max == 100 || d->max == 120 ? v : (v * 100 + 63) / 127);   /* (0.6.86: DLY FEEDBK 0..120 %, 100 the
                                                                              * loop's gain 1) */
        *unit = "%";
        break;
    case F_PCTINV:
        fmt_int(val, ((d->max - v) * 100 + 63) / 127);
        *unit = "%";
        break;
    case F_BIPCT:
        fmt_int(val, v * 100 / 64);
        if (v > 0) {
            char t[8];
            fmt_int(t, v * 100 / 64);
            val[0] = '+';
            str_cpy(val + 1, t, 6);
        }
        *unit = "%";
        break;
    case F_TIME: {
        uint32_t ms10 = TIME_MS_X10[v & 127];
        if (ms10 < 100u) {
            fmt_fix(val, (int32_t)ms10, 1);
            *unit = "ms";
        } else if (ms10 < 10000u) {
            fmt_int(val, (int32_t)((ms10 + 5u) / 10u));
            *unit = "ms";
        } else {
            fmt_fix(val, (int32_t)(ms10 / 100u), 2);
            if (ms10 >= 100000u)
                fmt_fix(val, (int32_t)(ms10 / 1000u), 1);
            *unit = "s";
        }
        break;
    }
    case F_LFOHZ: {
        uint32_t h = LFO_HZ_X100[v & 127];
        if (v < 0) {                                    /* (0.6.51) the LFO in time */
            str_cpy(val, N_LSYNC[(uint32_t)(-v - 1) % 12u], 6);
            break;
        }
        if (h < 1000u)
            fmt_fix(val, (int32_t)h, 2);
        else
            fmt_fix(val, (int32_t)(h / 10u), 1);
        *unit = "Hz";
        break;
    }
    case F_CUTOFF: {
        uint32_t h = CUTOFF_HZ[v & 127];
        if (h < 1000u) {
            fmt_int(val, (int32_t)h);
            *unit = "Hz";
        } else {
            fmt_fix(val, (int32_t)(h / 100u), 1);
            *unit = "kHz";
        }
        break;
    }
    case F_DB:
        if (v <= 0) {
            str_cpy(val, "OFF", 6);
        } else {
            fmt_fix(val, LEVEL_DB_X10[v], 1);
            *unit = "dB";
        }
        break;
    case F_SEMI:
        fmt_int(val, v);
        if (v > 0) {
            char t[8];
            fmt_int(t, v);
            val[0] = '+';
            str_cpy(val + 1, t, 6);
        }
        *unit = "st";
        break;
    case F_ENUM:
        str_cpy(val, d->names[v < d->min ? d->min : v > d->max ? d->max : v], 6);
        if (d->unit)
            *unit = d->unit;
        break;
    case F_BPM:
        fmt_int(val, v);
        *unit = "BPM";
        break;
    case F_NOTE:
        str_cpy(val, N_NOTE[v % 12], 6);
        break;
    case F_ONOFF:
        str_cpy(val, N_ONOFF[v ? 1 : 0], 6);
        break;
    case F_STEPS:
        fmt_int(val, v);
        *unit = "STEP";
        break;
    default:
        if (d->names) {                               /* F_INT with a 0-terminated name list: the range */
            uint32_t k;                               /* split evenly over the names (engine desc hooks) */
            str_cpy(val, d->names[name_of(d, v, &k)], 6);
        } else {
            fmt_int(val, v);
        }
        if (d->unit)
            *unit = d->unit;
        break;
    }
}

/* ------------------------------------------------------------ pages --- */
enum { FAM_HOME, FAM_ENV, FAM_LFO, FAM_FX, FAM_SCL, FAM_EDIT, FAM_GLO, FAM_SAVE, FAM_ARP, FAM_SEQ, FAM_TRK,
       FAM_COUNT };
enum { SC_TRACK, SC_GLOBAL, SC_ENGINE, SC_STEP };   /* (ACID FM 3c-1 task 7: SC_TRK, JIANT's MIXER's, gone with it) */
enum { GR_NONE, GR_ADSR, GR_LFO, GR_STEPS, GR_ARP, GR_SCALE, GR_FX, GR_ROLL,
       GR_MOD, GR_PATS, GR_CHANCE, GR_MORPH, GR_DXIMG,
       GR_LYRIC, GR_LIVE, GR_MORE, GR_SYN, GR_FM6P, GR_DXP, GR_MIX, GR_MIDI, GR_SVSND, GR_SVSONG, GR_SVPROJ };   /* (ACID FM
                                                         * 3c-2 task 2) GR_SVSND: SAVE > SOUND (ux_save.c), the inherited USER
                                                         * (GR_USER) and PRESETS (GR_BROWSE) gone; GR_SVSONG, GR_SVPROJ: SAVE's SONG
                                                         * and PROJECT (tasks 3, 4); (task 3) GR_SONG, the inherited ARRANGE and
                                                         * SEQ > SONG, gone with them; (task 4) GR_SLOTS and
                                                         * GR_TOOLS, the inherited PROJECT and TOOLS, too */   /* (ACID FM 3c-1 task 8) GR_MIDI: GLO > MIDI (ux_glo.c);
                                                         * (task 7) GR_MIX: GLO > MIX (ux_glo.c); GR_TRK,
                                                         * JIANT's MIXER and SENDS, gone with them */   /* (ACID FM 3c-1 task 4 fix 1: GR_MOTION GR_EVENTS GR_COMP GR_MUTATE
                                                         * GR_FUGUE GR_SHAPE gone with SEQ's pages; no value is stored, PAGES' own) */   /* (ACID FM 3b task 4) GR_SYN: a synth's EDIT page 3 (VOICE, FILTER);
                                                         * (task 5) GR_FM6P FM6's OPERATOR, OP ENV; GR_DXP DRUM-X's SOUND, SOUND 2 */   /* (ACID FM 3b) MORE: a section's last page, its list (ux_more.h; .more) */   /* (0.9.11) GLO > LIVE: MANO, RIELES (ui_live.c) */   /* (0.8.5) EDIT > WORDS: VOICE's lyric (sing.c) */   /* (0.8.5:
                                                         * LENTE's EDIT > FOCUS, ENV > STAGE) */   /* (0.6.99) FX > PUNCH: the punch-in lane */   /* (JIANT 0.6.39) SEQ > COMP: the step components (ui_input.c comp_*, ui_graph.c graph_comp) */   /* (ACID FM 3b task 5: GR_FMOP GR_FMOP2 GR_FMENV GR_DXSND, JIANT's FM6 operator and DRUM-X sound pages, gone; task 7: GR_MSEQ, MSEQ's;
                                                         * task 8: GR_SLCR GR_SLPIT GR_PUNCH GR_BUSES, JIANT's FX pages': FX the ux
                                                         * pages', GR_FX its SENDS) */

#include "ux_more.h"
typedef struct {
    const char *title;
    uint8_t fam, scope, graph;
    uint8_t id[4];               /* param ids; 0xFF = empty slot */
    const ux_more_t *more;       /* (ACID FM 3b) GR_MORE: its list (ux_more.c); else 0 */
} page_t;

/* (0.6.18) GLO > SYSTEM's TRKS: how many tracks play, the setting of MENU > AUDIO > TRACKS (ui.c ui_rec_prefs bit 7,
 * kept with the settings, not in the song). An id past the globals (no song.g of its own): trks_ui mirrors the setting
 * (ui.c rp_apply), a turn writes it back (ui_input.c knob_turn) */
#define G_TRKS 0xFEu
static const char *const N_TRKS[] = {"4", "6"};
static const param_desc_t TRKS_DESC = PE("TRACKS", N_TRKS, 0);
static int16_t trks_ui;
/* (0.6.30) .. and LOAD, next to it (ui.c ui_load, voice.c sys_load): the heavy parts lightened or gone while they
 * sound, ECO the lightest. load_ui mirrors it as trks_ui does */
#define G_LOADP 0xFDu
/* (0.7.6) a track page's slot holding the global ALL TR (G_STRN): SCL's KNOB 4, where TRANS was (page_desc) */
#define ID_ALLTR 0xFCu
static const char *const N_LOAD[] = {"ECO", "LITE", "FULL"};
static const param_desc_t LOAD_DESC = PE("LOAD", N_LOAD, 2);
static int16_t load_ui;
/* (0.9.18) the LFO's MODE TRIG / FREE (LFO KNOB 3) and its PHASE (LFO 2 KNOB 1): views of P_LPHASE, whose -1 has been FREE
 * since 0.6.51 (stored as ever: every project and preset opens as it was). page_desc reads them into lmode_ui / lphase_ui,
 * a turn writes P_LPHASE back (ui_input.c lfo_view_put). FREE keeps no phase: the one TRIG had stays in lfo_ph_mem
 * (RAM, a track each) for TRIG again, and PHASE turned in FREE moves only that (shown dimmed) */
#define ID_LMODE 0xFBu
#define ID_LPHS 0xFAu
static const char *const N_LMODE[] = {"TRIG", "FREE"};
static const param_desc_t LFO_MODE_DESC = PE("MODE", N_LMODE, 0);
static const param_desc_t LFO_PHASE_DESC = PD("PHASE", F_INT, 0, 127, 0);
static int16_t lmode_ui, lphase_ui;
static uint8_t lfo_ph_mem[NTRK];
static int lfo_mode_of(const track_t *t) { return t->p[P_LPHASE] < 0; }   /* 1 FREE */
/* (ACID FM 3b task 7) LFO > MOD MATRIX's KNOB 1, the slot picked (a track each: mod_ui_slot) as the ux knob row names it: a view
 * as ID_LMODE; ui_input.c edit_param turns it as JIANT's MOD page did (a slot a detent) */
#define ID_MSLOT 0xF9u
static const char *const N_MSLOT[] = {"1", "2", "3", "4"};
static const param_desc_t MSLOT_DESC = PE("SLOT", N_MSLOT, 0);
static int16_t mslot_ui;
/* (ACID FM 3b) EDIT's MORE on a synth track (ANALOG PHASE LOFI NOISE VOICE): what its three pages leave, grouped. A row on a
 * page shown in the chain (ANALOG's VOICE page, the others' FILTER: its TYPE) is not listed again (ux_more.c); a row JIANT showed only
 * on some engines follows its rule (MORPH, RETRIG, WORDS). ALLOC is on no page or list (tests/ux_cover_test.c UXC_HIDDEN) */
static const ux_row_t UXM_SYN_ROWS[] = {
    {UXM_H_VOICE, UXR_HEAD, 0}, {P_VOICE, UXR_TRACK, 0}, {P_GLIDE, UXR_TRACK, 0}, {P_GLMODE, UXR_TRACK, 0},
    {P_PRIO, UXR_TRACK, 0}, {P_DETUNE, UXR_TRACK, 0}, {P_LRTRG, UXR_TRACK, UXR_ON(ENGI_LOFI)},   /* (LOFI BYTE / FLOAT: RETRIG) */
    {UXM_H_TONE, UXR_HEAD, 0}, {P_FTYPE, UXR_TRACK, 0}, {P_MORPH, UXR_TRACK, 0}, {P_DRIFT, UXR_TRACK, 0},
    {UXM_H_WORDS, UXR_HEAD, 0}, {P_VLANG, UXR_TRACK, UXR_ON(ENGI_VOICE)}, {P_VLYR, UXR_TRACK, UXR_ON(ENGI_VOICE)},
    {UXM_A_LYRIC, UXR_ACTION, 0},
};
static const ux_more_t UXM_SYN = {UXM_SYN_ROWS, (uint8_t)(sizeof UXM_SYN_ROWS / sizeof UXM_SYN_ROWS[0])};
/* (ACID FM 3b task 5) FM6's operators and DRUM-X's kit as settings the ux pages and MORE rows name: views (as ID_LMODE) of
 * bytes that are no track setting, FM6's patch (eng_fm6.c fm6_patch: the operator picked, fop_op, and OP ENV's stage,
 * fop_stg) and the section's kit (drumx_voice.c dx_kit, dx_lay, dx_tone, dx_hands; eng_drum.c dx_snd, dx_mot: the drum
 * picked, dx_ui_lane, and the side, dx_ui_side). Nothing is stored under these ids: a value is read from its byte
 * (fld_ref: its descriptor, the value in a scratch) and a turn written to it as JIANT's operator and kit pages wrote it
 * (fld_turn: an operator's byte makes the patch the track's own, SLOT OWN; no automation, no lock, as there). Ids past
 * every track setting and below the views of 0xFA.. (ui_fm6op.c, ui_dx.c) */
enum { ID_FO_OP = 0xC0, ID_FO_CRS, ID_FO_FINE, ID_FO_LVL, ID_FO_STG, ID_FO_RATE, ID_FO_ELVL, ID_FO_MODE, ID_FO_DTUN,
       ID_FO_VEL, ID_FO_END };
enum { ID_DX_LANE = 0xD0, ID_DX_PITCH, ID_DX_DECAY, ID_DX_NOISE, ID_DX_COLOR, ID_DX_WAVE, ID_DX_NTYPE, ID_DX_SNAP, ID_DX_SIDE,
       ID_DX_PMOD, ID_DX_PMT, ID_DX_DRV, ID_DX_CLANG, ID_DX_LFB, ID_DX_SENDTO, ID_DX_SEND, ID_DX_BP, ID_DX_KFB, ID_DX_HANDS,
       ID_DX_XRATE, ID_DX_XDEP, ID_DX_XSHP, ID_DX_XRND, ID_DX_END };
typedef char fld_ids_ok[(uint32_t)P_COUNT <= (uint32_t)ID_FO_OP && (uint32_t)ID_DX_END <= 0xFAu ? 1 : -1];
static int fld_is(uint32_t id) { return (id >= ID_FO_OP && id < ID_FO_END) || (id >= ID_DX_LANE && id < ID_DX_END); }
/* (ACID FM 3c-1 task 4) SEQ > MORE's views (spec 2.4): what the inherited SEQ pages turned that is no track setting, as MORE
 * rows: SHAPE's STRETCH REPEAT NUDGE (ui_input.c shape_knob, applied as you turn), PLAYHEADS' four (fugue.c fg_set), the picked
 * step's (its two components' TYPE and values, EVERY, NOTE's FOCUS PITCH MOVE: ui_input.c comp_knob, note_edit), AUTOMATION's
 * (PLAY, the counts, the list: ui_events.c). Nothing is stored under these ids: ux_seq.c uxq_vtext reads each, ui_input.c
 * seq_view_turn turns it with the inherited page's handler. Ids past every track setting, below the fields of 0xC0 */
enum { ID_SQ_STRETCH = 0xA0, ID_SQ_REPEAT, ID_SQ_NUDGE, ID_SQ_HEAD, ID_SQ_HRATE, ID_SQ_HDIR, ID_SQ_HIVL, ID_SQ_TYPE, ID_SQ_VAL,
       ID_SQ_VAL2, ID_SQ_TYPE2, ID_SQ_VALB, ID_SQ_VAL2B, ID_SQ_EVERY, ID_SQ_FOCUS, ID_SQ_PITCH, ID_SQ_MOVE, ID_SQ_PLAY,
       ID_SQ_EVENTS, ID_SQ_LOCKS, ID_SQ_ROW, ID_SQ_ESTEP, ID_SQ_EPARAM, ID_SQ_EVAL, ID_SQ_RNG, ID_SQ_RNGB, ID_SQ_PLAYS, ID_SQ_END };   /* (RNG:
                                                         * WALK's, OCT's and an ARP's RANGE, MODE / ORDER's own row) */
typedef char sqv_ids_ok[(uint32_t)P_COUNT <= (uint32_t)ID_SQ_STRETCH && (uint32_t)ID_SQ_END <= (uint32_t)ID_FO_OP ? 1 : -1];
static int sqv_is(uint32_t id) { return id >= ID_SQ_STRETCH && id < ID_SQ_END; }
/* (ACID FM 3c-1 task 6) GLO > MORE's RAILS (spec 3.1): LIVE's latched layer as the inherited second LIVE page's knobs set it
 * (ui_live.c live_rail): TRACKS (ALL, T1 .. T6, SYNTHS, DRUMS: the tracks the other three and OCT- / OCT+ act on there), RATE,
 * CUT (the first N steps), DENSITY (THIN 90 .. 10 %, the middle all, RATCH x2 .. x4). Nothing stored: views of seq_live.c's lv
 * (ux_glo.c uxg_vtext its words), global rows (no step lock). Ids past every global, below the views of 0xF9 */
enum { ID_LV_TGT = 0xF5, ID_LV_RATE, ID_LV_CUT, ID_LV_DENS, ID_LV_END };
typedef char lvv_ids_ok[(uint32_t)G_COUNT <= (uint32_t)ID_LV_TGT && (uint32_t)ID_LV_END <= 0xF9u ? 1 : -1];
static int lvv_is(uint32_t id) { return id >= ID_LV_TGT && id < ID_LV_END; }
/* (ACID FM 3c-1 task 8) GLO > MORE's MIDI rows that are no song global: NOTES OUT (OFF / KEYS / KEYS+SEQUENCE) and CLOCK OUT, the
 * device's (ui.c midi_outs: saved with the settings, MENU's MIDI OUT until 0.5), LEARN (MIDI LEARN armed: midi_learn.c ml.on,
 * never saved; MENU's until 0.5). Views as TRACKS and LOAD: param_ref reads one into mov_ui, a turn writes it back (ui_input.c
 * param_knob, mov_put). Ids past every global, below the RAILS' */
enum { ID_MO_NOTES = 0xF2, ID_MO_CLOCK, ID_ML_LEARN, ID_MO_END };
typedef char mov_ids_ok[(uint32_t)G_COUNT <= (uint32_t)ID_MO_NOTES && (uint32_t)ID_MO_END <= (uint32_t)ID_LV_TGT ? 1 : -1];
static int mov_is(uint32_t id) { return id >= ID_MO_NOTES && id < ID_MO_END; }
/* (ACID FM 3c-2 task 2) SAVE > MORE's SOUND rows that are no setting of their own: the inherited PRESETS page's KNOB 2 (ENGINE: the
 * track's, ui.c select_engine a detent a turn), KNOB 3 (FAVORITE: the sound's mark, preset_mark) and KNOB 4 (LIST: ALL, FAV or a sound
 * type, list_put); (task 3) its SONG's END (the inherited ARRANGE's END row: STOP, X2 .. X8, LOOP; song_chain.c song_end_step). Views
 * as GLO's NOTES OUT: param_ref reads one into sv_ui (sv_get), ui_input.c sv_view_turn writes it with the inherited knob's handler.
 * Ids past every global (ID_DX_END is 0xE7), below GLO's MIDI views */
enum { ID_SV_ENGINE = 0xE7, ID_SV_FAV, ID_SV_LIST, ID_SV_END, ID_SV_N };   /* (ID_DX_END is 0xE7) */
typedef char svv_ids_ok[(uint32_t)ID_SV_N <= (uint32_t)ID_MO_NOTES ? 1 : -1];
typedef char svv_past_ok[(uint32_t)G_COUNT <= (uint32_t)ID_SV_ENGINE && (uint32_t)ID_DX_END <= (uint32_t)ID_SV_ENGINE ? 1 : -1];
static int svv_is(uint32_t id) { return id >= ID_SV_ENGINE && id < ID_SV_N; }
/* ENGINE's words: the engines' names in the order they are shown (engines.c ENGINE_ORDER; tests/ui_test.c test_u2_more) */
static const char *const N_SV_ENG[] = {"DRUM", "VOICE", "PHASE", "LOFI", "ANALOG", "FM6", "NOISE",
#if FELUCCA_FM4
                                       "DIGITAL",
#endif
};
typedef char sv_eng_ok[sizeof N_SV_ENG / sizeof N_SV_ENG[0] == NENG_SHOWN ? 1 : -1];
/* (JIANT 0.9.12) PRESETS' LIST: ALL, FAV, then the sound types (ui.c list_get; here for SAVE > MORE's LIST, 3c-2 task 2) */
static const char *const N_LIST[PT_N + 1] = {"ALL", "FAV", "BASS", "LEAD", "PAD", "KEYS", "PLUCK", "BELL", "VOX", "DRUM",
                                              "FX"};
/* (3c-2 task 3) END's words: the song stops after once, plays 2 .. 8 times, round and round (sv_get: song_end_step's places) */
static const char *const N_SV_END[9] = {"STOP", "X2", "X3", "X4", "X5", "X6", "X7", "X8", "LOOP"};
static const param_desc_t SV_DESC[ID_SV_N - ID_SV_ENGINE] = {PE("ENGINE", N_SV_ENG, 0), PE("FAVORITE", N_ONOFF, 0),
                                                             PE("LIST", N_LIST, 0), PE("END", N_SV_END, 0)};
static int16_t sv_ui;
static uint32_t sv_get(uint32_t id);                    /* (ui.c: the value now) */
static const char *const N_MONOTES[] = {"OFF", "KEYS", "KEYS+SEQ"};   /* (the one short form; the status line KEYS + SEQUENCE:
                                                         * ux_section.c uxs_vtext, past param_format's five letters, uxs_vfull) */
static const param_desc_t MOV_DESC[3] = {PE("NOTES OUT", N_MONOTES, 1), PE("CLOCK OUT", N_ONOFF, 0), PE("LEARN", N_ONOFF, 0)};
static int16_t mov_ui;
static uint32_t mov_get(uint32_t id);                   /* (ui.c: the setting now) */
static const param_desc_t *fld_ref(uint32_t id, int16_t **valp);   /* (ui_dx.c) its descriptor, its value (a scratch) */
static void fld_turn(uint32_t id, int32_t steps, uint32_t knob);    /* (ui_dx.c) turned steps detents of KNOB knob (0..3) */
static uint32_t dx_lane_picked(void);                   /* (ui_dx.c) the drum picked, 0..7 (BD .. CB): DRUM-X's SOUND's LEVEL */
/* their words (the values as the knob row shows them; each fits 48 px, tests/ui_test.c) */
static const char *const N_FO_OP[] = {"1", "2", "3", "4", "5", "6"};
static const char *const N_FO_RAT[] = {"0.50", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15",
                                       "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
                                       "31"};
static const char *const N_FO_HZ[] = {"1HZ", "10HZ", "100HZ", "1KHZ"};
static const char *const N_FO_STG[] = {"ATK", "DEC", "SUS", "REL"};
static const char *const N_FO_MODE[] = {"RATIO", "FIXED"};
static const char *const N_FO_DT[] = {"-7", "-6", "-5", "-4", "-3", "-2", "-1", "0", "+1", "+2", "+3", "+4", "+5", "+6", "+7"};
static const char *const N_DX_LANE[] = {"BD", "SD", "CP", "CH", "OH", "TM", "RS", "CB"};
static const char *const N_DX_WAVE[] = {"SINE", "FM", "METAL", "BELL"};
static const char *const N_DX_NTYPE[] = {"LP", "BP", "HP"};
static const char *const N_DX_SIDE[] = {"A", "B"};
static const char *const N_DX_PMT[] = {"DECAY", "LONG", "NOISE", "SINE"};
static const char *const N_DX_BUS[] = {"OFF", "CHO", "DLY", "REV"};
static const char *const N_DX_XRATE[] = {"OFF", "4BAR", "2BAR", "1BAR", "1/2", "1/4", "1/8", "1/16", "1/32"};
static const char *const N_DX_XSHP[] = {"SINE", "TRI", "SAW", "RAMP", "S/H"};
static const char *const N_DX_OFF[] = {"OFF"};
static const param_desc_t FLD_DESC[] = {
    [ID_FO_OP - ID_FO_OP] = PE("OP", N_FO_OP, 0),
    [ID_FO_CRS - ID_FO_OP] = PE("RATIO", N_FO_RAT, 1),
    [ID_FO_FINE - ID_FO_OP] = PD("FINE", F_INT, 0, 99, 0),
    [ID_FO_LVL - ID_FO_OP] = PD("LEVEL", F_INT, 0, 99, 99),
    [ID_FO_STG - ID_FO_OP] = PE("STAGE", N_FO_STG, 0),
    [ID_FO_RATE - ID_FO_OP] = PD("RATE", F_INT, 0, 99, 99),
    [ID_FO_ELVL - ID_FO_OP] = PD("LEVEL", F_INT, 0, 99, 99),
    [ID_FO_MODE - ID_FO_OP] = PE("FREQ MODE", N_FO_MODE, 0),
    [ID_FO_DTUN - ID_FO_OP] = PE("OP DETUNE", N_FO_DT, 7),
    [ID_FO_VEL - ID_FO_OP] = PD("VELOCITY", F_INT, 0, 7, 0),
};
static const param_desc_t FLD_DX_DESC[] = {
    [ID_DX_LANE - ID_DX_LANE] = PE("DRUM", N_DX_LANE, 0),
    [ID_DX_PITCH - ID_DX_LANE] = PD("PITCH", F_INT, 0, 127, 64),
    [ID_DX_DECAY - ID_DX_LANE] = PD("DECAY", F_PCT, 0, 127, 64),
    [ID_DX_NOISE - ID_DX_LANE] = PD("NOISE", F_PCT, 0, 127, 0),
    [ID_DX_COLOR - ID_DX_LANE] = PD("COLOR", F_PCT, 0, 127, 64),
    [ID_DX_WAVE - ID_DX_LANE] = PE("WAVE", N_DX_WAVE, 0),
    [ID_DX_NTYPE - ID_DX_LANE] = PE("NOISE TYPE", N_DX_NTYPE, 0),
    [ID_DX_SNAP - ID_DX_LANE] = PE("SNAP", N_ONOFF, 0),
    [ID_DX_SIDE - ID_DX_LANE] = PE("SIDE", N_DX_SIDE, 0),
    [ID_DX_PMOD - ID_DX_LANE] = PD("PITCH MOD", F_PCT, 0, 127, 0),
    [ID_DX_PMT - ID_DX_LANE] = PE("MOD TYPE", N_DX_PMT, 0),
    [ID_DX_DRV - ID_DX_LANE] = PE("DRIVE", N_ONOFF, 0),
    [ID_DX_CLANG - ID_DX_LANE] = PD("CLANG", F_INT, 0, 15, 0),
    [ID_DX_LFB - ID_DX_LANE] = PD("FEEDBACK", F_INT, 0, 15, 0),
    [ID_DX_SENDTO - ID_DX_LANE] = PE("SEND TO", N_DX_BUS, 0),
    [ID_DX_SEND - ID_DX_LANE] = PD("SEND", F_PCT, 6, 100, 6),
    [ID_DX_BP - ID_DX_LANE] = PD("METAL", F_CUTOFF, 80, 108, 80),
    [ID_DX_KFB - ID_DX_LANE] = PD("FEEDBACK", F_INT, 0, 15, 0),
    [ID_DX_HANDS - ID_DX_LANE] = PD("HANDS", F_INT, 0, 15, 0),
    [ID_DX_XRATE - ID_DX_LANE] = PE("RATE", N_DX_XRATE, 0),
    [ID_DX_XDEP - ID_DX_LANE] = PD("DEPTH", F_PCT, 0, 127, 0),
    [ID_DX_XSHP - ID_DX_LANE] = PE("SHAPE", N_DX_XSHP, 0),
    [ID_DX_XRND - ID_DX_LANE] = PD("RANDOM", F_PCT, 0, 127, 0),
};
static const param_desc_t FLD_DX_BPOFF = PE("METAL", N_DX_OFF, 0);   /* (METAL's band-pass off) */
static const param_desc_t FLD_DX_SENDOFF = PE("SEND", N_DX_OFF, 0);   /* (3b task 5 fix: SEND TO OFF, no amount) */
static const param_desc_t FLD_FO_FIXED = PE("FREQ", N_FO_HZ, 0);   /* (an operator FIXED: its COARSE a decade) */
typedef char fld_desc_ok[NELEM(FLD_DESC) == ID_FO_END - ID_FO_OP && NELEM(FLD_DX_DESC) == ID_DX_END - ID_DX_LANE ? 1 : -1];
/* EDIT's MORE on an FM6 track (spec 2.1): what SOUND, OPERATOR and OP ENV leave, grouped: the filter first (the one most
 * turned: CUTOFF a detent away), the patch (which one plays, its carriers' DETUNE), the modulators' (MOD TIME, MOD VEL: SOUND's
 * MOD LEVEL and MOD RATIO's family), the picked operator's (its heading names it: OP 3); ALLOC hidden as on the synths
 * (tests/ux_cover_test.c UXC_HIDDEN). (3b task 5 fix: TONE heads filter and tone rows only) */
static const ux_row_t UXM_FM6_ROWS[] = {
    {UXM_H_FILTER, UXR_HEAD, 0}, {P_FTYPE, UXR_TRACK, 0}, {P_FCUT, UXR_TRACK, 0}, {P_FRES, UXR_TRACK, 0},
    {P_DIST, UXR_TRACK, 0},
    {UXM_H_PATCH, UXR_HEAD, 0}, {P_E7, UXR_TRACK, 0}, {P_E6, UXR_TRACK, 0},
    {UXM_H_MOD, UXR_HEAD, 0}, {P_E4, UXR_TRACK, 0}, {P_E5, UXR_TRACK, 0},
    {UXM_H_OP, UXR_HEAD, 0}, {ID_FO_MODE, UXR_TRACK, UXR_ON(ENGI_FM6)}, {ID_FO_DTUN, UXR_TRACK, UXR_ON(ENGI_FM6)},
    {ID_FO_VEL, UXR_TRACK, UXR_ON(ENGI_FM6)},
    {UXM_H_VOICE, UXR_HEAD, 0}, {P_VOICE, UXR_TRACK, 0}, {P_GLIDE, UXR_TRACK, 0}, {P_GLMODE, UXR_TRACK, 0},
    {P_PRIO, UXR_TRACK, 0}, {P_DETUNE, UXR_TRACK, 0},
};
static const ux_more_t UXM_FM6 = {UXM_FM6_ROWS, (uint8_t)(sizeof UXM_FM6_ROWS / sizeof UXM_FM6_ROWS[0])};
/* .. on a DRUM-X track: the kit's (KIT 2's E5..E8, KIT 3's, STEREO), the picked drum's (its heading names it: DRUM . SD),
 * MORPH's LFO (JIANT's X-MOD), the part's filter. Hidden on purpose (tests/ux_cover_test.c UXC_HIDDEN): VOICE's four (a kit
 * plays its drums as hits: voice.c trk_vmode), the filter's DRIVE (FX > SENDS DRIVE's own setting). (3b task 5 fix: a drum's
 * own FEEDBACK shown, after CLANG as on JIANT's SOUND 4: the generator sets the snare's, gen.c) */
static const ux_row_t UXM_DRM_ROWS[] = {
    {UXM_H_KIT, UXR_HEAD, 0}, {P_E4, UXR_TRACK, 0}, {P_E5, UXR_TRACK, 0}, {P_E6, UXR_TRACK, 0}, {P_E7, UXR_TRACK, 0},
    {ID_DX_KFB, UXR_TRACK, UXR_ON(ENGI_DRUM)}, {ID_DX_BP, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {ID_DX_HANDS, UXR_TRACK, UXR_ON(ENGI_DRUM)}, {P_DXIMG, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {UXM_H_DRUM, UXR_HEAD, 0}, {ID_DX_SIDE, UXR_TRACK, UXR_ON(ENGI_DRUM)}, {ID_DX_PMOD, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {ID_DX_PMT, UXR_TRACK, UXR_ON(ENGI_DRUM)}, {ID_DX_NTYPE, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {ID_DX_SNAP, UXR_TRACK, UXR_ON(ENGI_DRUM)}, {ID_DX_DRV, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {ID_DX_CLANG, UXR_TRACK, UXR_ON(ENGI_DRUM)}, {ID_DX_LFB, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {ID_DX_SENDTO, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {ID_DX_SEND, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {UXM_H_MLFO, UXR_HEAD, 0}, {ID_DX_XRATE, UXR_TRACK, UXR_ON(ENGI_DRUM)}, {ID_DX_XDEP, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {ID_DX_XSHP, UXR_TRACK, UXR_ON(ENGI_DRUM)}, {ID_DX_XRND, UXR_TRACK, UXR_ON(ENGI_DRUM)},
    {UXM_H_FILTER, UXR_HEAD, 0}, {P_FTYPE, UXR_TRACK, 0}, {P_FCUT, UXR_TRACK, 0}, {P_FRES, UXR_TRACK, 0},
};
static const ux_more_t UXM_DRM = {UXM_DRM_ROWS, (uint8_t)(sizeof UXM_DRM_ROWS / sizeof UXM_DRM_ROWS[0])};
/* (3b task 6) SCL's MORE: TRANSPOSE, every note the track plays in semitones (spec 2.5); ALL TRACKS (scale steps, every sequence)
 * is the KEY page's KNOB 4. (3c-1 task 4: SEQ PITCH SEQ > MORE's, 3c-1 spec 2.4: one place a setting.) ARP and ENV have none
 * (their eight settings on two pages each) */
static const ux_row_t UXM_SCL_ROWS[] = {
    {P_TRANS, UXR_TRACK, 0},
};
static const ux_more_t UXM_SCL = {UXM_SCL_ROWS, (uint8_t)(sizeof UXM_SCL_ROWS / sizeof UXM_SCL_ROWS[0])};
/* (3b task 7) LFO's MORE: the LFO's PHASE (where MODE TRIG starts it), then the STEPS (JIANT's MSEQ: sixteen levels the matrix's
 * source STEPS plays, LENGTH steps of RATE each, SMOOTH gliding between them, mod.c MS_STEP) */
static const ux_row_t UXM_LFO_ROWS[] = {
    {UXM_H_LFO, UXR_HEAD, 0}, {ID_LPHS, UXR_TRACK, 0},
    {UXM_H_MSEQ, UXR_HEAD, 0}, {P_MSLEN, UXR_TRACK, 0}, {P_MSDIV, UXR_TRACK, 0}, {P_MSSLW, UXR_TRACK, 0},
    {P_MS0, UXR_TRACK, 0}, {P_MS0 + 1, UXR_TRACK, 0}, {P_MS0 + 2, UXR_TRACK, 0}, {P_MS0 + 3, UXR_TRACK, 0},
    {P_MS0 + 4, UXR_TRACK, 0}, {P_MS0 + 5, UXR_TRACK, 0}, {P_MS0 + 6, UXR_TRACK, 0}, {P_MS0 + 7, UXR_TRACK, 0},
    {P_MS0 + 8, UXR_TRACK, 0}, {P_MS0 + 9, UXR_TRACK, 0}, {P_MS0 + 10, UXR_TRACK, 0}, {P_MS0 + 11, UXR_TRACK, 0},
    {P_MS0 + 12, UXR_TRACK, 0}, {P_MS0 + 13, UXR_TRACK, 0}, {P_MS0 + 14, UXR_TRACK, 0}, {P_MS0 + 15, UXR_TRACK, 0},
};
static const ux_more_t UXM_LFO = {UXM_LFO_ROWS, (uint8_t)(sizeof UXM_LFO_ROWS / sizeof UXM_LFO_ROWS[0])};
/* (3b task 8) FX's MORE (spec 2.4): what SENDS (the track's DISTORT and three sends), SPACE (DLY TIME, DLY FDBK, REV TYPE, REV
 * SIZE) and MASTER (CLIP PUNCH DUCK DUCK TIME) leave of JIANT's sixteen FX pages, in the spec's order: the track's DISTORT (TYPE,
 * TONE) and AUTO PAN, then the buses (globals): DELAY, REVERB (its fourth row PRE / AIR / PITCH by the TYPE, DAMP FOLLOW on RESO:
 * params.c param_ref, ux_section.c uxs_label), CHORUS, SLICER and its pitch steps (JIANT's SLICER, SL BUS, SL BUS 2, SL PITCH;
 * BUSES' four amounts each in its own group), the punch-in lane (JIANT's PUNCH: its length, step, CLEAR asking first). CHORUS and
 * the SLICER none under LOAD ECO (ux_more.c ux_row_shown: ECO takes them out of the sound, as JIANT hid their pages) */
static const ux_row_t UXM_FX_ROWS[] = {
    {UXM_H_DIST, UXR_HEAD, 0}, {P_DTYPE, UXR_TRACK, 0}, {P_DTONE, UXR_TRACK, 0},
    {UXM_H_APAN, UXR_HEAD, 0}, {P_APDEP, UXR_TRACK, 0}, {P_APRATE, UXR_TRACK, 0}, {P_APSYNC, UXR_TRACK, 0},
    {P_APWAVE, UXR_TRACK, 0},
    {UXM_H_DELAY, UXR_HEAD, 0}, {G_DCOLOR, UXR_GLOBAL, 0}, {G_DMIX, UXR_GLOBAL, 0}, {G_WIDTH, UXR_GLOBAL, 0},
    {G_DPIT, UXR_GLOBAL, 0}, {G_DSPRY, UXR_GLOBAL, 0}, {G_DMOD, UXR_GLOBAL, 0}, {G_DTYPE, UXR_GLOBAL, 0}, {G_DPING, UXR_GLOBAL, 0},
    {UXM_H_REVERB, UXR_HEAD, 0}, {G_RDAMP, UXR_GLOBAL, 0}, {G_RPRE, UXR_GLOBAL, 0}, {G_RMOD, UXR_GLOBAL, 0},
    {G_RRATE, UXR_GLOBAL, 0}, {G_RFILT, UXR_GLOBAL, 0}, {G_RWIDE, UXR_GLOBAL, 0},
    {UXM_H_CHORUS, UXR_HEAD, 0}, {G_CRATE, UXR_GLOBAL, 0}, {G_CDEPTH, UXR_GLOBAL, 0}, {G_CSTER, UXR_GLOBAL, 0},
    {UXM_H_SLICER, UXR_HEAD, 0}, {G_SLMODE, UXR_GLOBAL, 0}, {G_SLPAT, UXR_GLOBAL, 0}, {G_SLRATE, UXR_GLOBAL, 0},
    {G_SLDEP, UXR_GLOBAL, 0}, {G_SLT1, UXR_GLOBAL, 0}, {G_SLT1 + 1, UXR_GLOBAL, 0}, {G_SLT1 + 2, UXR_GLOBAL, 0},
    {G_SLT1 + 3, UXR_GLOBAL, 0}, {G_SLT1 + 4, UXR_GLOBAL, 0}, {G_SLT6, UXR_GLOBAL, 0}, {G_SLFX, UXR_GLOBAL, 0},
    {UXM_H_SLPIT, UXR_HEAD, 0}, {G_SLPLEN, UXR_GLOBAL, 0}, {G_SLPQ, UXR_GLOBAL, 0},
    {G_SLP0, UXR_GLOBAL, 0}, {G_SLP0 + 1, UXR_GLOBAL, 0}, {G_SLP0 + 2, UXR_GLOBAL, 0}, {G_SLP0 + 3, UXR_GLOBAL, 0},
    {G_SLP0 + 4, UXR_GLOBAL, 0}, {G_SLP0 + 5, UXR_GLOBAL, 0}, {G_SLP0 + 6, UXR_GLOBAL, 0}, {G_SLP0 + 7, UXR_GLOBAL, 0},
    {G_SLP0 + 8, UXR_GLOBAL, 0}, {G_SLP0 + 9, UXR_GLOBAL, 0}, {G_SLP0 + 10, UXR_GLOBAL, 0}, {G_SLP0 + 11, UXR_GLOBAL, 0},
    {G_SLP0 + 12, UXR_GLOBAL, 0}, {G_SLP0 + 13, UXR_GLOBAL, 0}, {G_SLP0 + 14, UXR_GLOBAL, 0}, {G_SLP15, UXR_GLOBAL, 0},
    {UXM_H_LANE, UXR_HEAD, 0}, {G_PFXLEN, UXR_GLOBAL, 0}, {G_PFXRES, UXR_GLOBAL, 0}, {UXM_A_LANE, UXR_ACTION, 0},
};
typedef char uxm_fx_fits[sizeof UXM_FX_ROWS / sizeof UXM_FX_ROWS[0] <= 64u ? 1 : -1];   /* (ux_more.c: a list holds 64 at most) */
static const ux_more_t UXM_FX = {UXM_FX_ROWS, (uint8_t)(sizeof UXM_FX_ROWS / sizeof UXM_FX_ROWS[0])};
/* (ACID FM 3c-1 task 4) SEQ's MORE (spec 2.4): everything the inherited SEQ pages reached that STEPS, STEP EXTRAS and PATTERN do
 * not: TRACK (JIANT's TRACK page), the picked step's (its heading names it, STEP 5: the two components of COMP and COMP 2, EVERY,
 * NOTE's FOCUS PITCH MOVE), MUTATE, SHAPE (STRETCH: applied as you turn; ROTATE, SCL's until now), PLAYHEADS, AUTOMATION (PLAY,
 * the counts, CLEAR asking first, then AUTO LIST's rows: OCT+ adds a lock or turns a record into the other kind). STEP right after
 * TRACK (not the spec's fifth): the step picked on STEPS a few detents away (tests/ui_test.c's task race) */
static const ux_row_t UXM_SEQ_ROWS[] = {
    {UXM_H_TRACK, UXR_HEAD, 0}, {P_POFS, UXR_TRACK, 0}, {P_TCHAN, UXR_TRACK, 0}, {P_HVEL, UXR_TRACK, 0}, {P_HRQ, UXR_TRACK, 0},
    {ID_SQ_PLAYS, UXR_TRACK, 0},                         /* (3c-2 task 6) PLAYS: the track's G_PLAYS1 + k (ux_seq.c) */
    {UXM_H_STEP, UXR_HEAD, 0}, {ID_SQ_TYPE, UXR_TRACK, 0}, {ID_SQ_VAL, UXR_TRACK, 0}, {ID_SQ_RNG, UXR_TRACK, 0},
    {ID_SQ_VAL2, UXR_TRACK, 0}, {ID_SQ_TYPE2, UXR_TRACK, 0}, {ID_SQ_VALB, UXR_TRACK, 0}, {ID_SQ_RNGB, UXR_TRACK, 0},
    {ID_SQ_VAL2B, UXR_TRACK, 0}, {ID_SQ_EVERY, UXR_TRACK, 0},
    {ID_SQ_FOCUS, UXR_TRACK, 0}, {ID_SQ_PITCH, UXR_TRACK, 0}, {ID_SQ_MOVE, UXR_TRACK, 0},
    {UXM_H_MUTATE, UXR_HEAD, 0}, {P_MSND, UXR_TRACK, 0}, {P_MPAT, UXR_TRACK, 0}, {P_MAMT, UXR_TRACK, 0}, {P_MMODE, UXR_TRACK, 0},
    {UXM_H_SHAPE, UXR_HEAD, 0}, {ID_SQ_STRETCH, UXR_TRACK, 0}, {ID_SQ_REPEAT, UXR_TRACK, 0}, {ID_SQ_NUDGE, UXR_TRACK, 0},
    {P_SOFS, UXR_TRACK, 0},
    {UXM_H_HEADS, UXR_HEAD, 0}, {ID_SQ_HEAD, UXR_TRACK, 0}, {ID_SQ_HRATE, UXR_TRACK, 0}, {ID_SQ_HDIR, UXR_TRACK, 0},
    {ID_SQ_HIVL, UXR_TRACK, 0},
    {UXM_H_AUTO, UXR_HEAD, 0}, {ID_SQ_PLAY, UXR_TRACK, 0}, {ID_SQ_EVENTS, UXR_TRACK, 0}, {ID_SQ_LOCKS, UXR_TRACK, 0},
    {UXM_A_CLRAUTO, UXR_ACTION, 0}, {ID_SQ_ROW, UXR_TRACK, 0}, {ID_SQ_ESTEP, UXR_TRACK, 0}, {ID_SQ_EPARAM, UXR_TRACK, 0},
    {ID_SQ_EVAL, UXR_TRACK, 0},
};
static const ux_more_t UXM_SEQ = {UXM_SEQ_ROWS, (uint8_t)(sizeof UXM_SEQ_ROWS / sizeof UXM_SEQ_ROWS[0])};
/* (ACID FM 3c-1 task 6) GLO's MORE (spec 3.3): LIVE's RAILS (the inherited second LIVE page's four knobs, spec 3.1); (task 7)
 * SYSTEM (JIANT's GLO > SYSTEM: TRACKS 4 / 6, LOAD, CPU a readout; ROUTE, inert since 0.5, gone), TEMPO (GLOBAL's BPM, the rules
 * HOME's KNOB 4 has: ui_input.c param_knob; task 8: its CLOCK the MIDI page's KNOB 4), (task 8, spec 4.1) MIDI (KEYBOARD PLAYS
 * SELECTED, CLOCK OUT, NOTES OUT, LEARN, LEARN CLEAR: the last four MENU > MIDI's until 0.5), UNDO (the GLO layer's G4: ui_layer.c
 * glo_undo) */
static const ux_row_t UXM_GLO_ROWS[] = {
    {UXM_H_LIVE, UXR_HEAD, 0}, {ID_LV_TGT, UXR_GLOBAL, 0}, {ID_LV_RATE, UXR_GLOBAL, 0}, {ID_LV_CUT, UXR_GLOBAL, 0},
    {ID_LV_DENS, UXR_GLOBAL, 0},
    {UXM_H_SYSTEM, UXR_HEAD, 0}, {G_TRKS, UXR_GLOBAL, 0}, {G_LOADP, UXR_GLOBAL, 0}, {G_INFO, UXR_GLOBAL, 0},
    {UXM_H_TEMPO, UXR_HEAD, 0}, {G_BPM, UXR_GLOBAL, 0},
    {UXM_H_MIDI, UXR_HEAD, 0}, {G_MRSEL, UXR_GLOBAL, 0}, {ID_MO_CLOCK, UXR_GLOBAL, 0}, {ID_MO_NOTES, UXR_GLOBAL, 0},
    {ID_ML_LEARN, UXR_GLOBAL, 0}, {UXM_A_LCLEAR, UXR_ACTION, 0},
    {UXM_H_UNDO, UXR_HEAD, 0}, {UXM_A_UNDO, UXR_ACTION, 0},
};
static const ux_more_t UXM_GLO = {UXM_GLO_ROWS, (uint8_t)(sizeof UXM_GLO_ROWS / sizeof UXM_GLO_ROWS[0])};
/* (ACID FM 3c-2 task 2) SAVE's MORE (spec 2.4): (task 3) SONG (END, the inherited ARRANGE's END row; DELETE ROW, the dialog on the
 * row SONG picks; CLEAR SONG, the inherited TOOLS'), TOOLS (task 4: the inherited TOOLS page's CLEAR SEQ, INIT SOUND, NEW PROJECT,
 * CPU TEST, each its dialog; ERASE USER SLOT, the inherited USER page's ERASE: the dialog on the slot SOUND picks), SOUND (what the
 * inherited USER and PRESETS pages reached that SOUND's knobs do not: USER's LOAD, PRESETS' ENGINE, FAVORITE, LIST) */
static const ux_row_t UXM_SAVE_ROWS[] = {
    {UXM_H_SONG, UXR_HEAD, 0}, {ID_SV_END, UXR_GLOBAL, 0}, {UXM_A_DELROW, UXR_ACTION, 0}, {UXM_A_CLRSONG, UXR_ACTION, 0},
    {UXM_H_TOOLS, UXR_HEAD, 0}, {UXM_A_CLRSEQ, UXR_ACTION, 0}, {UXM_A_INITSND, UXR_ACTION, 0}, {UXM_A_ERASE, UXR_ACTION, 0},
    {UXM_A_NEWPROJ, UXR_ACTION, 0}, {UXM_A_CPUTEST, UXR_ACTION, 0},
    {UXM_H_SOUND, UXR_HEAD, 0}, {UXM_A_ULOAD, UXR_ACTION, 0}, {ID_SV_ENGINE, UXR_GLOBAL, 0}, {ID_SV_FAV, UXR_GLOBAL, 0},
    {ID_SV_LIST, UXR_GLOBAL, 0},
};
static const ux_more_t UXM_SAVE = {UXM_SAVE_ROWS, (uint8_t)(sizeof UXM_SAVE_ROWS / sizeof UXM_SAVE_ROWS[0])};
static const page_t PAGES[] = {
    {"ENV", FAM_ENV, SC_TRACK, GR_ADSR, {P_ATK, P_DEC, P_SUS, P_REL}},
    {"ENV DEST", FAM_ENV, SC_TRACK, GR_NONE, {P_ED_FLT, P_ED_PIT, P_ED_SHP, P_ELOOP}},   /* JIANT 0.4: LOOP */
    {"LFO", FAM_LFO, SC_TRACK, GR_LFO, {P_LRATE, P_LWAVE, ID_LMODE, P_LFADE}},   /* (0.9.18: MODE where PHASE was) */
    {"LFO DEST", FAM_LFO, SC_TRACK, GR_NONE, {P_LD_PIT, P_LD_FLT, P_LD_SHP, P_LD_AMP}},
    {"MOD", FAM_LFO, SC_TRACK, GR_MOD, {ID_MSLOT, P_M1SRC, P_M1DST, P_M1AMT}},   /* KNOB 1: the slot (mod_ui_slot) */
    /* (ACID FM 3b task 7) JIANT's MSEQ (KNOB 1 the step, its LEVEL, LEN, SLEW) and MSEQ 2 (DIV) gone: LFO's MORE rows (STEPS) */
    /* (ACID FM 3b task 8) FX on the ux pages (ui_lens.c CE_FX): SENDS (JIANT's FX: the track's DISTORT and its three sends),
     * SPACE, MASTER, MORE. JIANT's DIST, AUTO PAN, BUSES, DLY, DLY 2, DLY 3, REVERB, REVERB 2, CHORUS, SLICER, SL BUS, SL BUS 2,
     * SL PITCH and PUNCH pages gone: MORE's rows (UXM_FX) */
    {"FX", FAM_FX, SC_TRACK, GR_FX, {P_DIST, P_CHOR, P_DLY, P_REV}},
    {"FX SPACE", FAM_FX, SC_GLOBAL, GR_NONE, {G_DTIME, G_DFDBK, G_RTYPE, G_RSIZE}},   /* the delay's time and repeats, the
                                                         * reverb's model and size */
    {"MASTER", FAM_FX, SC_GLOBAL, GR_NONE, {G_CLIP, G_PUNCH, G_DUCK, G_DREL}},   /* JIANT: clipper, drum bus, ducking */
    {"SCL", FAM_SCL, SC_TRACK, GR_SCALE, {P_ROOT, P_SCALE, P_QUANT, ID_ALLTR}},   /* (JIANT: ROOT, SCALE every melodic track's;
                                                         * 0.7.6: ALL TR, the global, where TRANS was: SCL 2 gone) */
    {"CHORD", FAM_SCL, SC_TRACK, GR_NONE, {P_CHRD, P_CVOI, P_CSTR, 0xFF}},   /* (0.6.81) a key plays a chord (seq.c); (ACID FM 3c-1
                                                         * task 4) KNOB 4 empty: ROTATE is SEQ > MORE's (3c-1 spec 2.4: one place) */
    {"EDIT 1", FAM_EDIT, SC_ENGINE, GR_NONE, {P_E0, P_E1, P_E2, P_E3}},
    {"EDIT 2", FAM_EDIT, SC_ENGINE, GR_NONE, {P_E4, P_E5, P_E6, P_E7}},
    {"WORDS", FAM_EDIT, SC_TRACK, GR_LYRIC, {P_VLANG, P_VLYR, P_E0, 0xFF}},   /* (0.8.5) VOICE: LANG LYRIC POS, OCT+ TYPE
                                                         * (the lyric on the NAME screen, ui_name.c) */
    {"RETRIG", FAM_EDIT, SC_TRACK, GR_NONE, {P_LRTRG, 0xFF, 0xFF, 0xFF}},
    {"MORPH", FAM_EDIT, SC_TRACK, GR_MORPH, {P_MORPH, P_DRIFT, 0xFF, 0xFF}},   /* (0.6.45) ANALOG PHASE LOFI: the wave morphs;
                                                         * (0.7.8) DRIFT: the voices wander */
    {"ROTOR", FAM_EDIT, SC_TRACK, GR_NONE, {P_WRDEP, P_WDUST, 0xFF, 0xFF}},   /* (0.7.8) WHEEL: the rotor's depth, DUST
                                                         * (ACID FM 0.2: WHEEL retired, never shown: ui.c page_visible) */
    /* (JIANT 0.6.54) DRUM's KIT 3, X-MOD and FM6's OPERATOR, OP ENV, OPERATOR 2 (ACID FM 3b task 5): gone, their settings on
     * the ux pages below and in MORE (fld_*). STEREO kept: IMAGE's rule (a kit's; P_DXIMG is VOICE's LYRIC slot) */
    {"STEREO", FAM_EDIT, SC_TRACK, GR_DXIMG, {P_DXIMG, 0xFF, 0xFF, 0xFF}}, /* (0.6.58) IMAGE: the kit's stereo placings; (0.7.8)
                                                         * with the kit's pages, before the sounds (the volumes last) */
    {"FILTER", FAM_EDIT, SC_TRACK, GR_NONE, {P_FTYPE, P_E4, P_E5, P_E6}},   /* (JIANT 0.5) ANALOG: TYPE CUT RES DRV */
    {"FILTER", FAM_EDIT, SC_TRACK, GR_NONE, {P_FTYPE, P_FCUT, P_FRES, P_DIST}},   /* .. every other engine's (on the part;
                                                         * DRV: the part's DIST, after it) */
    /* (ACID FM 3b task 5) DRUM's SOUND 1..4, LEVELS, LEVELS 2 gone: a drum's settings on the ux pages and in MORE */
    {"OP1 ENV", FAM_EDIT, SC_TRACK, GR_ADSR, {P_FM1_ATK, P_FM1_DEC, P_FM1_SUS, P_FM1_REL}},
    {"OP2 ENV", FAM_EDIT, SC_TRACK, GR_ADSR, {P_FM2_ATK, P_FM2_DEC, P_FM2_SUS, P_FM2_REL}},
    {"OP3 ENV", FAM_EDIT, SC_TRACK, GR_ADSR, {P_FM3_ATK, P_FM3_DEC, P_FM3_SUS, P_FM3_REL}},
    {"OP4 ENV", FAM_EDIT, SC_TRACK, GR_ADSR, {P_FM4_ATK, P_FM4_DEC, P_FM4_SUS, P_FM4_REL}},
    {"OP LEVEL", FAM_EDIT, SC_TRACK, GR_NONE, {P_FM1_LEVEL, P_FM2_LEVEL, P_FM3_LEVEL, P_FM4_LEVEL}},
    {"VOICE", FAM_EDIT, SC_TRACK, GR_NONE, {P_VOICE, P_GLIDE, P_GLMODE, P_PRIO}},
    /* (ACID FM 3c-1 task 7) JIANT's GLOBAL (BPM SWING CLOCK TUNE) and SYSTEM (TRACKS LOAD ROUTE CPU) gone: SWING and TUNE GLO >
     * MIX's knobs, the rest GLO > MORE's rows (UXM_GLO; ROUTE, inert since 0.5's per-track MIDI, on none) */
    /* (ACID FM 3c-2 task 2) SAVE > SOUND (spec 2.1, ux_save.c): SLOT PRESET NAME SAVE (no ids: ui_input.c save_knob); the inherited
     * PRESETS (the browser: No. ENG FAV LIST) and USER (SLOT LOAD ERASE SAVE) pages gone, the rest SAVE > MORE's rows (UXM_SAVE) */
    {"SAVE SOUND", FAM_SAVE, SC_GLOBAL, GR_SVSND, {0xFF, 0xFF, 0xFF, 0xFF}},
    /* (ACID FM 3c-2 task 3) SAVE > SONG (spec 2.2, ux_save.c): SECTION ROW BARS SCENE (no ids: ui_input.c save_knob); the inherited
     * SECTIONS (PROJECT's KNOB 1 the section) and ARRANGE (the song's rows) pages gone, END SAVE > MORE's row */
    {"SAVE SONG", FAM_SAVE, SC_GLOBAL, GR_SVSONG, {0xFF, 0xFF, 0xFF, 0xFF}},
    /* (ACID FM 3c-2 task 4) SAVE > PROJECT (spec 2.3, ux_save.c): PROJECT LOAD SAVE NAME (no ids: ui_input.c save_knob); the
     * inherited PROJECT (PROJECT, SECT, LOAD, SAVE: G_SLOT G_LOAD G_SAVE) and TOOLS (CLEAR SEQ INIT SOUND DELETE ROW CLEAR SONG NEW
     * PROJECT CPU TEST) pages gone: SECT SAVE > SONG's SECTION, the tools SAVE > MORE's rows */
    {"SAVE PROJECT", FAM_SAVE, SC_GLOBAL, GR_SVPROJ, {0xFF, 0xFF, 0xFF, 0xFF}},
    {"ARP", FAM_ARP, SC_TRACK, GR_ARP, {P_AMODE, P_ARATE, P_AOCT, P_AGATE}},
    {"ARP 2", FAM_ARP, SC_TRACK, GR_NONE, {P_ASWING, P_APROB, P_AHOLD, P_AORDER}},
    /* (0.6.90) SEQ: STEP (the piano roll) and the pages that edit it, one after another: NOTE, TIMING, STRETCH, COMP,
     * COMP 2, WHEN; then the track's: PATTERN, SHIFT, HUMAN, PLAYHEADS, AUTOMATION, AUTO LIST, MUTATE; PHRASES, SONG and
     * CHANCE out of the cycle (ui.c page_visible) */
    /* (0.6.92) eleven pages, was fifteen: the piano roll, the pattern, the roll's edits (NOTE: FOCUS PITCH MOVE MICRO,
     * TIMING folded in), STRETCH (on the turn), the step's two components (EVERY on them: WHEN and TRIG's loops in
     * one), TRACK (SHIFT's PITCH and CHANCE, HUMAN as one amount, REC Q; C.LEN retired from the pages), then the rest */
    {"STEP", FAM_SEQ, SC_STEP, GR_ROLL, {0, 1, 3, 2}},   /* (ACID FM 3c-1 task 2: SEQ > STEPS, ux_seq.c) KNOB 1..4 the fields
                                                         * STEP NOTE FLAG TIME (ui_input.c step_edit): STEP NOTE ACC/SLD GATE */
    {"PATTERN", FAM_SEQ, SC_TRACK, GR_STEPS, {P_SLEN, P_SDIV, P_SSWING, P_SGATE}},
    /* (ACID FM 3c-1 task 4) JIANT's NOTE, STRETCH, COMP, COMP 2, PLAYHEADS, TRACK, AUTOMATION, AUTO LIST and MUTATE pages gone:
     * SEQ > MORE's rows (UXM_SEQ; their settings and the inherited handlers they turned: ui_input.c seq_view_turn) */
    /* (0.8.2) the pages above edit the piano roll and keep it in the panel (ui_graph.c roll_view); TRACK and the ones
     * after are the track's own, with their own graphs. (0.8.3) LENTE (ui.c lens_*) groups them by level, the table
     * as it was: PATTERN (PATTERN STRETCH PLAYHEADS TRACK MUTATE), BAR (STEP NOTE AUTOMATION AUTO LIST), STEP (NOTE
     * COMP COMP 2); SELECT zooms, a SEQ tap the level's next set */
    {"PHRASES", FAM_SEQ, SC_GLOBAL, GR_PATS, {0xFF, 0xFF, 0xFF, 0xFF}},    /* pattern loader (ui.c pat_load); 0.6.37: hidden */
    /* (ACID FM 3c-1 task 7) JIANT's MIXER (LEVEL PAN REV, the DJ FILTER) gone: GLO > MIX (below), REV FX > SENDS', the DJ FILTER
     * GLO > LIVE's KNOB 4 */
    /* (ACID FM 3c-2 task 3) SEQ > SONG (never shown since 0.6.24: the song in one place) gone with ARRANGE: SAVE > SONG */
    {"STEP EXTRAS", FAM_SEQ, SC_STEP, GR_CHANCE, {0xFF, 0xFF, 0xFF, 0xFF}},   /* (ACID FM 3c-1 task 3: was CHANCE, hidden since
                                                         * 0.6.37) the picked step's RATCHET CHANCE CONDITION MICRO (ux_seq.c) */
    /* (0.8.5) LENTE everywhere (ui_lens.c): pages the mock-up added, appended (the table's order is no one's index) */
    {"VOICE 2", FAM_EDIT, SC_TRACK, GR_NONE, {P_ALLOC, P_DETUNE, 0xFF, 0xFF}},   /* EDIT > TONE: ALLOC, UNISON's DETUNE (in
                                                         * TP since Felucca, on no page until now) */
    /* (ACID FM 3b) EDIT > FOCUS gone (a view into the LFO's MOD matrix for one setting: the matrix keeps it all) */
    /* (ACID FM 3b task 6) ENV > STAGE gone: it turned the four values ENV's AMP page turns (spec 2.2) */
    /* (ACID FM 3c-1 task 7) GLO > MIX > SENDS (the track's sends again) gone: FX > SENDS has them (3c-1 spec 3.2) */
    /* (0.9.18) SEQ > BAR gone (0.9.2 .. 0.9.17: KNOB 1 the bar, KNOB 2 LEN): the bar is the roll's cursor / strip and
     * STEP's KNOB 1 walks all 64 steps, LEN is PATTERN's */
    {"LIVE", FAM_GLO, SC_GLOBAL, GR_LIVE, {0xFF, 0xFF, 0xFF, 0xFF}},   /* (0.9.11) GLO > LIVE: the keys a board of live
                                                         * tricks (ui_live.c, seq_live.c); (ACID FM 3c-1 task 6) on the ux pages
                                                         * (ux_glo.c), its title was MANO; the inherited second page's knobs
                                                         * (RIELES) GLO > MORE's RAILS rows (UXM_GLO) */
    /* (ACID FM 3b task 7) LFO 2 (0.9.18: PHASE, where TRIG starts) gone: LFO's MORE row */
    /* (ACID FM 3b) EDIT on a synth track (ui_lens.c CE_SYN): SOUND (EDIT 1), TONE (EDIT 2), then ANALOG's VOICE or the
     * others' FILTER, then MORE. Titles of their own: nav_page takes the first page shown with its chain entry's title */
    {"SYN VOICE", FAM_EDIT, SC_TRACK, GR_SYN, {P_VOICE, P_GLIDE, P_PRIO, P_DETUNE}},   /* ANALOG's (ui.c page_jiant) */
    {"SYN FILTER", FAM_EDIT, SC_TRACK, GR_SYN, {P_FTYPE, P_FCUT, P_FRES, P_DIST}},   /* the others' (FILTER's rule) */
    {"SYN MORE", FAM_EDIT, SC_TRACK, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_SYN},
    /* (ACID FM 3b task 5) EDIT on an FM6 track (ui_lens.c CE_FM6): SOUND (EDIT 1), OPERATOR, OP ENV, MORE; KNOB 1 the operator
     * on both (one pick, fop_op), its bytes after it (fld_*) */
    {"FM6 OP", FAM_EDIT, SC_TRACK, GR_FM6P, {ID_FO_OP, ID_FO_CRS, ID_FO_FINE, ID_FO_LVL}},
    {"FM6 OP ENV", FAM_EDIT, SC_TRACK, GR_FM6P, {ID_FO_OP, ID_FO_STG, ID_FO_RATE, ID_FO_ELVL}},
    {"FM6 MORE", FAM_EDIT, SC_TRACK, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_FM6},
    /* .. on a DRUM-X track (CE_DRM): KIT (EDIT 1), SOUND, SOUND 2, MORE; KNOB 1 the drum on both (one pick, dx_ui_lane: a key
     * struck picks it too), its LEVEL the track's P_LN0 + the drum (page_id) */
    {"DRUM SOUND", FAM_EDIT, SC_TRACK, GR_DXP, {ID_DX_LANE, P_LN0, ID_DX_PITCH, ID_DX_DECAY}},
    {"DRUM SOUND 2", FAM_EDIT, SC_TRACK, GR_DXP, {ID_DX_LANE, ID_DX_NOISE, ID_DX_COLOR, ID_DX_WAVE}},
    {"DRUM MORE", FAM_EDIT, SC_TRACK, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_DRM},
    /* (ACID FM 3b task 6) SCL's MORE (ui_lens.c CE_SCL: KEY, CHORD, MORE): TRANSPOSE (every note the track plays, on no page since
     * 0.7.6; 3c-1 task 4: SEQ PITCH SEQ > MORE's) */
    {"SCL MORE", FAM_SCL, SC_TRACK, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_SCL},
    /* (ACID FM 3b task 7) LFO's MORE (ui_lens.c CE_LFO: LFO, DEST, MOD MATRIX, MORE): PHASE (JIANT's LFO 2), the STEPS (JIANT's
     * MSEQ, MSEQ 2: the modulation sequence the matrix's source STEPS reads) */
    {"LFO MORE", FAM_LFO, SC_TRACK, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_LFO},
    /* (ACID FM 3b task 8) FX's MORE (ui_lens.c CE_FX: SENDS, SPACE, MASTER, MORE): a track page (its DISTORT and AUTO PAN rows the
     * track's: REC + FX clears their automation, ui_input.c rec_clear_auto), the buses' rows globals */
    {"FX MORE", FAM_FX, SC_TRACK, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_FX},
    /* (ACID FM 3c-1 task 4) SEQ's MORE (ui_lens.c CE_SEQ: STEPS, STEP EXTRAS, PATTERN, MORE): a track page */
    {"SEQ MORE", FAM_SEQ, SC_TRACK, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_SEQ},
    /* (ACID FM 3c-1 task 6) GLO's MORE (ui_lens.c CE_GLO: LIVE, MIX, MIDI, MORE): the song's (its rows globals and views) */
    {"GLO MORE", FAM_GLO, SC_GLOBAL, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_GLO},
    /* (ACID FM 3c-1 task 7) GLO > MIX (3c-1 spec 3.2, ux_glo.c): KNOB 1 LEVEL, 2 PAN of the selected track (no id: ui_input.c
     * tracks_edit, the inherited MIXER's write), 3 SWING, 4 TUNE (the song's, as JIANT's GLOBAL turned them) */
    {"MIX", FAM_GLO, SC_GLOBAL, GR_MIX, {0xFF, 0xFF, G_SWING, G_TUNE}},
    /* (ACID FM 3c-1 task 8) GLO > MIDI (spec 4.1, ux_glo.c): KNOB 1 IN, 2 OUT, 3 CHANNEL of the selected track (page_id: the
     * first track's id + the track), 4 CLOCK (INT / USB / TRS, the song's; JIANT's GLOBAL's) */
    {"MIDI", FAM_GLO, SC_GLOBAL, GR_MIDI, {G_MRIN1, G_MROUT1, G_MRCH1, G_CLOCK}},
    /* (ACID FM 3c-2 task 2) SAVE's MORE (ui_lens.c CE_SAVE: SOUND, .., MORE): the song's (its rows views and actions) */
    {"SAVE MORE", FAM_SAVE, SC_GLOBAL, GR_MORE, {0xFF, 0xFF, 0xFF, 0xFF}, &UXM_SAVE},
};
#define NPAGES (sizeof(PAGES) / sizeof(PAGES[0]))
static uint8_t mod_ui_slot_tr[NTRK];   /* the MOD page: the matrix slot (0..3) KNOB 2..4 edit, a track each (3b task 7: as FM6's
                                                         * operator and DRUM-X's drum, ui_fm6op.c fop_op; RAM, 0 at boot) */
#define mod_ui_slot (mod_ui_slot_tr[song.sel % NTRK])
static const char *const N_RFOLW[] = {"OFF", "T1", "T2", "T3", "T4", "T5", "T6"};
static const param_desc_t RESO_PITCH = PD("PITCH", F_INT, 0, 14, 0);   /* (0.6.5) RESO: the chord's steps up the scale (G_RPRE) */
static const param_desc_t RESO_FOLW = PE("FOLLOW", N_RFOLW, 0);          /* .. the track whose notes it follows (G_RDAMP) */
static const param_desc_t REV_PRE = PD("PRE", F_INT, 0, 100, 0);       /* ROOM / SPRING: the pre-delay, ms (G_RPRE) */
static const param_desc_t REV_AIR = PD("AIR", F_SEMI, -24, 24, 0);    /* (0.6.12) SHIMMER / CLOUD: pitch and tone, down dark,
                                                                       * up airy (G_RPRE; fx.c rev_air) */
static const param_desc_t AP_BEAT = PE("RATE", N_APDIV, 2);          /* (0.6.9) AUTO PAN's RATE on the tempo (SYNC BEAT) */
/* (ACID FM 3b task 8: sl_ui_step, SL PITCH's step picked, gone with its page: FX > MORE's PITCH 1 .. 16 rows) */

/* (ACID FM 3b) the setting id of track t (0: the globals, song.g) as a knob turns it: its descriptor and value (*valp; 0 and 0:
 * none). The ids past both tables are views (ID_ALLTR, G_TRKS, ID_LMODE / ID_LPHS, G_LOADP, ID_MSLOT), whatever t. page_desc (a page's
 * knob) and ux_more.c (a MORE row) read through it, ui_input.c param_knob writes through it */
#define ID_NONE 0xFFFFu
static const param_desc_t *param_ref(track_t *t, uint32_t id, int16_t **valp)
{
    if (id == ID_NONE || id == 0xFFu) {
        *valp = 0;
        return 0;
    }
    if (id == ID_ALLTR) {                             /* (0.7.6) SCL's KNOB 4: the global ALL TR on the track page */
        *valp = &song.g[G_STRN];
        return &GP[G_STRN];
    }
    if (id == G_TRKS) {                               /* (0.6.18) not a song global: the device's setting */
        *valp = &trks_ui;
        return &TRKS_DESC;
    }
    if (id == ID_LMODE || id == ID_LPHS) {           /* (0.9.18) the LFO's MODE and PHASE: views of P_LPHASE */
        track_t *lt = t ? t : TSEL;
        int32_t ph = lt->p[P_LPHASE];
        lmode_ui = (int16_t)(ph < 0);
        lphase_ui = (int16_t)(ph < 0 ? lfo_ph_mem[(uint32_t)(lt - trk) % NTRK] : ph);
        *valp = id == ID_LMODE ? &lmode_ui : &lphase_ui;
        return id == ID_LMODE ? &LFO_MODE_DESC : &LFO_PHASE_DESC;
    }
    if (id == ID_MSLOT) {                             /* (3b task 7) MOD MATRIX's SLOT: the slot picked */
        mslot_ui = (int16_t)(mod_ui_slot & 3u);
        *valp = &mslot_ui;
        return &MSLOT_DESC;
    }
    if (id == G_LOADP) {                              /* (0.6.30) the same */
        *valp = &load_ui;
        return &LOAD_DESC;
    }
    if (mov_is(id)) {                                 /* (3c-1 task 8) GLO > MORE's NOTES OUT, CLOCK OUT, LEARN: the same */
        mov_ui = (int16_t)mov_get(id);
        *valp = &mov_ui;
        return &MOV_DESC[id - ID_MO_NOTES];
    }
    if (svv_is(id)) {                                 /* (3c-2 task 2) SAVE > MORE's ENGINE, FAVORITE, LIST: the same */
        sv_ui = (int16_t)sv_get(id);
        *valp = &sv_ui;
        return &SV_DESC[id - ID_SV_ENGINE];
    }
    if (t && fld_is(id))                              /* (3b task 5) FM6's operator, DRUM-X's kit: their bytes */
        return fld_ref(id, valp);
    if (sqv_is(id) || lvv_is(id)) {                   /* (3c-1 task 4) SEQ > MORE's views, (task 6) GLO > MORE's RAILS: no value
                                                         * of their own (ux_seq.c, ux_glo.c) */
        *valp = 0;
        return 0;
    }
    if (!t) {
        *valp = &song.g[id];
        if (song.g[G_RTYPE] == 3 && (id == G_RPRE || id == G_RDAMP))   /* (0.6.5) RESO: PITCH, FOLW in their slots */
            return id == G_RPRE ? &RESO_PITCH : &RESO_FOLW;
        if (id == G_RPRE)                             /* (0.6.12) SHIMMER / CLOUD: AIR; ROOM / SPRING: PRE */
            return song.g[G_RTYPE] == 2 || song.g[G_RTYPE] == 4 ? &REV_AIR : &REV_PRE;
        return &GP[id];
    }
    *valp = &t->p[id];
    if (id == P_APRATE && t->p[P_APSYNC])            /* (0.6.9) AUTO PAN on the tempo: a division */
        return &AP_BEAT;
    return track_desc(t, id);
}
/* page pg's knob slot: the setting it turns (the slot shown on MOD), ID_NONE none; whose (0: global) */
static uint32_t page_id(const page_t *pg, uint32_t slot)
{
    uint32_t id = pg->id[slot];
    if (id == 0xFFu)
        return ID_NONE;
    if (id == ID_ALLTR || id == G_TRKS || id == ID_LMODE || id == ID_LPHS || id == G_LOADP || id == ID_MSLOT)
        return id;
    if (pg->scope == SC_STEP)
        return ID_NONE;
    if (pg->graph == GR_MIDI && slot < 3u)            /* (3c-1 task 8) GLO > MIDI's IN OUT CHANNEL: the selected track's */
        return id + song.sel % NTRK;
    if (pg->scope == SC_GLOBAL)
        return id;                                      /* (3b task 8: SL PITCH's step picked gone: MORE's rows) */
    if (pg->scope == SC_ENGINE && id == P_E4 && ENGINES[TSEL->eng_req % NENGINES] == &ENG_LOFI && TSEL->p[P_E1] < RW_BYTE)
        return TSEL->p[P_E1] == RW_WRAM ? P_DRIFT : P_MORPH;   /* (ACID FM 3b task 4 fix) LOFI's TONE KNOB 1, JIANT's empty
                                                         * slot outside BYTE / FLOAT (no knob dead): MORPH, the chip wave on
                                                         * towards the next; WRAM, the last, which MORPH cannot move: DRIFT */
    if (pg->graph == GR_DXP && id == P_LN0)          /* (3b task 5) DRUM-X's SOUND: the LEVEL of the drum picked */
        return P_LN0 + dx_lane_picked();
    if (pg->graph == GR_MOD)                          /* SRC DST AMT of the slot shown */
        id += 3u * mod_ui_slot;
    return id;
}
static track_t *page_trk(const page_t *pg) { return pg->scope == SC_GLOBAL ? 0 : TSEL; }
static const param_desc_t *page_desc(const page_t *pg, uint32_t slot, int16_t **valp)
{
    return param_ref(page_trk(pg), page_id(pg, slot), valp);
}
