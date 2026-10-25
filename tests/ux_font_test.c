/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 NorfolkHacker */
/* ACID FM's Inter Tight faces (build/gen/ux_fonts.h, tools/gen_aa_font.py --preset acid): each UF_* exists, has the
 * glyphs of its character set, and their capital heights grow LBL < HDR < VAL < TTL < MARK. */
#define KC_REF 1
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define __attribute__(x)
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#include "../firmware/src/gfx.c"

static int bad;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); bad++; } } while (0)

#ifdef UX_MSG_LITS
#include "ux_msg_lits.h"                       /* (tests/ux_msg_scan.py, at test-build time: the message literals of firmware/src) */
/* every byte of every text in list has a glyph in f (fix wave, I1: a byte without one is drawn as a space) */
static void has_all(const aafont_t *f, const char *name, const char *const *list)
{
    uint32_t n = 0;
    for (; *list; list++, n++) {
        const char *s;
        for (s = *list; *s; s++)
            if ((uint8_t)*s >= 0x20u && glyph_at(f, (uint8_t)*s) < 0) {   /* (below 0x20: an icon code, MSG_NOFILE) */
                printf("FAIL %s lacks 0x%02X of \"%s\"\n", name, (uint8_t)*s, *list); bad++;
            }
    }
    printf("%s: %u texts checked\n", name, (unsigned)n);
}
#endif

static void has(const aafont_t *f, const char *name, const char *chars)
{
    for (; *chars; chars++)
        if (glyph_at(f, (uint8_t)*chars) < 0) { printf("FAIL %s lacks 0x%02X\n", name, (uint8_t)*chars); bad++; }
}

int main(void)
{
    /* printable ASCII less ! " $ & ' ( ) * , ; < = > ? @ [ \ ] ^ _ ` { | } ~ (left out to keep the faces under 12 KiB), plus the middle dot */
    const char *all = " #%+-./0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\xB7";
    CHECK(text_w(&UF_VAL, "128") > 0);
    CHECK(text_w(&UF_TTL, "A1") > 0);
    has(&UF_MARK, "MARK", "ACID FM");
    has(&UF_LBL, "LBL", " #%+-./0123456789:<>ABCDEFGHIJKLMNOPQRSTUVWXYZ\xB7\x86");
    has(&UF_HDR, "HDR", all);
    has(&UF_HDR, "HDR", "'(),=>[]\x85\x86\x87\x88");   /* (the status line's messages, the transport) */
#ifdef UX_MSG_LITS
    has_all(&UF_HDR, "HDR (status-line texts)", UX_MSG_HDR);
    has_all(&UF_LBL, "LBL (footer texts)", UX_MSG_LBL);
#endif
    has(&UF_VAL, "VAL", all);
    has(&UF_TTL, "TTL", " -./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    CHECK(UF_LBL_CAP_H < UF_HDR_CAP_H);
    CHECK(UF_HDR_CAP_H < UF_VAL_CAP_H);
    CHECK(UF_VAL_CAP_H < UF_TTL_CAP_H);
    CHECK(UF_TTL_CAP_H < UF_MARK_CAP_H);
    printf("cap heights %d %d %d %d %d\n", UF_LBL_CAP_H, UF_HDR_CAP_H, UF_VAL_CAP_H, UF_TTL_CAP_H, UF_MARK_CAP_H);
    puts(bad ? "ux font: FAILED" : "ux font: ok");
    return bad != 0;
}
