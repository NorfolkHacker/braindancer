/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker
 * (0.9.1) song_chain.c and storage.c agree on NSONG / NSONG_INDEX: with song_chain.c's NSONG another value (TIE_NSONG,
 * default 5) or its NSONG_INDEX another (TIE_NSONG_INDEX, default 8) storage.c must not build. tests/nsong_tie_test.sh
 * builds it three ways (syntax only) */
#ifndef TIE_NSONG
#define TIE_NSONG 5u
#endif
#ifndef TIE_NSONG_INDEX
#define TIE_NSONG_INDEX 8u
#endif
#define NSONG TIE_NSONG
#define NSONG_INDEX TIE_NSONG_INDEX
#include "hostsim.c"
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
#include "../firmware/src/storage.c"
