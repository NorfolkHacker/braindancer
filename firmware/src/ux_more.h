/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's MORE list (3b spec 2, parent spec 4.1 rule 5): a section's last page, a list of everything else in it, one setting
 * a row. SELECT scrolls, KNOB 1 picks a row, KNOB 2 sets it (through ui_input.c param_knob, the write a page's knob does:
 * undo, locks, automation and MIDI LEARN as there). A MORE page is a PAGES row (params.c) whose graph is GR_MORE and whose
 * .more points at its list. Types only here (params.c includes it for page_t); the code is ux_more.c, included after
 * ux_home.c. */
#ifndef UX_MORE_H
#define UX_MORE_H

enum { UXR_TRACK, UXR_GLOBAL, UXR_ACTION, UXR_HEAD };   /* a row: a track setting (P_*), a global (G_*), an action (an
                                                         * index of ux_more.c UXM_ACT), a heading (an index of UXM_HEAD) */
/* eng: shown only on that engine's tracks (its ENGINES index + 1; 0: any), where one stored slot means two things (P_LRTRG is
 * LOFI's RETRIG and VOICE's WORDS): each row says only one of them */
typedef struct { uint16_t id; uint8_t kind, eng; } ux_row_t;
typedef struct ux_more_s { const ux_row_t *rows; uint8_t n; } ux_more_t;
#define UXR_ON(e) ((uint8_t)((e) + 1u))   /* (a row's eng: on engine e only) */

/* the headings (UXR_HEAD rows' ids; ux_more.c UXM_HEAD their words) and the actions (UXR_ACTION rows' ids; UXM_ACT) */
enum { UXM_H_ARP, UXM_H_VOICE, UXM_H_FILTER, UXM_H_TONE, UXM_H_MOD, UXM_H_MSEQ, UXM_H_DELAY, UXM_H_REVERB, UXM_H_CHORUS,
       UXM_H_SLICER, UXM_H_WORDS, UXM_H_KIT, UXM_H_OP, UXM_H_DRUM, UXM_H_MLFO, UXM_H_PATCH, UXM_H_LFO, UXM_H_DIST, UXM_H_APAN,
       UXM_H_SLPIT, UXM_H_LANE, UXM_H_TRACK, UXM_H_STEP, UXM_H_MUTATE, UXM_H_SHAPE, UXM_H_HEADS, UXM_H_AUTO, UXM_H_LIVE,
       UXM_H_SYSTEM, UXM_H_TEMPO, UXM_H_UNDO, UXM_H_MIDI, UXM_H_N };   /* (3c-1 task 8: GLO's MIDI) */   /* (3c-1 task 6: GLO's LIVE, the RAILS rows; task 7: its SYSTEM, TEMPO,
                                                         * UNDO) */   /* (3c-1
                                                         * task 4: SEQ's; STEP names the step picked, "STEP 5") */   /* (3b task 8: FX's DISTORT, AUTO PAN, SLICE PITCH, PUNCH LANE) */   /* (3b task 5: OP and DRUM name the
                                                         * operator and the drum picked, "OP 3", "DRUM \xB7 SD": ux_more.c;
                                                         * task 7: LFO's MORE, LFO and MSEQ: "STEPS") */
enum { UXM_A_LYRIC, UXM_A_LANE, UXM_A_CLRAUTO, UXM_A_UNDO, UXM_A_LCLEAR, UXM_A_N };   /* (3c-1 task 8: GLO's LEARN CLEAR,
                                                         * MENU's until 0.5) */   /* (3c-1 task 7: GLO's UNDO, the GLO layer's G4) */   /* (3b task 8: FX's CLEAR LANE, the punch-in lane emptied, asking
                                                         * first; 3c-1 task 4: SEQ's CLEAR, the track's automation, asking) */

#define UXM_ROWS 8u              /* rows on the panel at once */
#define UXM_PITCH 18             /* .. a row's height (8 x 18 = 144 of the panel's 150) */
#define UXM_SELECT 4u            /* ux_more_turn's knob for SELECT (0..3: KNOB 1..4) */
#define UXM_K34 "KNOB 1 PICKS \xB7 KNOB 2 SETS"   /* KNOB 3 or 4 turned on MORE: what the knobs do (ui_input.c) */
#define UXM_KBD_SEL "KEYBOARD PLAYS SELECTED"           /* (3c-1 task 8) GLO's G_MRSEL in the list (KEYBOARD on the knob) */
#define UXM_LCLEAR "LEARN CLEAR"                        /* (3c-1 task 8) .. its LEARN CLEAR (CLEAR on the knob) */
#define UXS_PICK "SETTING"                              /* MORE's KNOB 1 (the knob row, a step held's cards) */
/* (3b task 9) why a MORE row takes no step lock (ux_more.c uxm_lock_why, UXM_WHY its words): it does (UXM_LOCKS), a global, an FM6
 * patch byte, a DRUM-X kit byte, an action, a track setting the lock store does not hold (motion.c motion_param: "NAME" + its words) */
enum { UXM_LOCKS, UXM_NL_GLOBAL, UXM_NL_PATCH, UXM_NL_KIT, UXM_NL_ACTION, UXM_NL_SET, UXM_NL_N };

#endif
