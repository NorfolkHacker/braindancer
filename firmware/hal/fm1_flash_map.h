/* SPDX-License-Identifier: GPL-3.0-only
 * Portions Copyright (C) 2026 NorfolkHacker */
/* Flash addresses with no hardware in them (so the host tests include them: fm1_flash.h is the SPI driver). */
#pragma once
/* (ACID FM 0.2) the sample area: songs 5..8's sectors (JIANT kept 8 songs, ACID FM 4), contiguous, for breakbeats
 * (piece 4 writes it; until then nothing does). Outside the app slot, so a firmware update never erases it. */
#define FL_SMP_LO       0x000B8000u
#define FL_SMP_HI       0x000D8000u
_Static_assert(FL_SMP_HI <= 0xD8000u, "the sample area ends below the song index");
