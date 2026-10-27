/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* (ACID FM 0.5, 3c-1 task 8) GLO > MORE's NOTES OUT and CLOCK OUT as stored in the device's settings (ui.c midi_outs: engine 15's
 * byte 14): bits 0-1 NOTES OUT (MOUT_OFF, MOUT_KEYS, MOUT_SEQ: the keys' notes and the sequence's; 3 unknown = KEYS), bit 2 CLOCK
 * OUT (MOUT_CLK), bit 7 (MOUT_MARK) set once it holds them. Included by ui.c and settings_persist.c (which migrates MIDI OUT, ui.c
 * midi_out_mode, into it once: mout_of_mode) */
#ifndef MIDI_OUTS_H
#define MIDI_OUTS_H
enum { MOUT_OFF, MOUT_KEYS, MOUT_SEQ };
#define MOUT_CLK 4u
#define MOUT_MARK 0x80u
/* MIDI OUT's KEYS CLOCK ALL OFF (0..3) as the two, marked: KEYS = KEYS; CLOCK = KEYS + CLOCK OUT (it sent the keys' notes too); ALL
 * = KEYS+SEQUENCE + CLOCK OUT; OFF = OFF. What each sent, it sends still */
static uint32_t mout_of_mode(uint32_t m)
{
    static const uint8_t M[4] = {MOUT_KEYS, MOUT_KEYS | MOUT_CLK, MOUT_SEQ | MOUT_CLK, MOUT_OFF};
    return MOUT_MARK | M[m & 3u];
}
#endif
