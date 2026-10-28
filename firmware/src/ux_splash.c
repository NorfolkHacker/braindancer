/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 NorfolkHacker */
/* ACID FM's power-on splash (spec 4.5, the wordmark; main.c fm1_main and web/emu felucca_web.c call it): the blend line,
 * 3 px at y 96, sweeps across left to right; ACID FM in UF_MARK gradient text types on under it a letter at a time; then
 * the version and "BY NORFOLKHACKER" in UF_LBL MID. SPL_WRITE ms of that (splash_write, main.c's loop), SPL_HOLD ms shown
 * whole. MENU > ANIMATION OFF draws the last frame at once. One 240 x 108 strip a step, in the constant ux_theme colours:
 * nothing but what settings_init (before the splash) gives. Included by ui_draw.c where JIANT's signature splash was
 * (after ux_theme.c, gfx.c): the names spl_anim, draw_splash, splash_write, SPL_WRITE, SPL_HOLD are the ones they use. */
#define SPL_WRITE 900u                              /* ms: the animation */
#define SPL_HOLD 600u                               /* .. then shown whole, before the UI */
#define SPL_Y 96                                    /* the line's top row on the screen; the strip starts here */
#define SPL_CH 108                                  /* the strip: line at 0 .. 2, wordmark 10 .. 61, labels 70 and 84 */
#define SPL_LINE_MS 600u                            /* the line across */
#define SPL_TYPE0 150u                              /* the first letter */
#define SPL_TYPE_MS 100u                            /* a letter every .. */
#define SPL_LBL_MS 850u                             /* the labels appear */
static const char SPL_MARK[] = "ACID FM";
static uint8_t spl_anim;                            /* main.c: 1 = the splash to be animated (splash_write) */
static uint32_t spl_last = ~0u;                     /* the last frame drawn (its state), so a held splash is not sent again */

/* the wordmark, ACID FM in UF_MARK gradient text centred across, its line's top at canvas y (on UX_BG); its left x. (ACID FM 3c-2
 * task 5) ABOUT's first screen draws it too (ui_menu.c about_top) */
static int32_t spl_mark(int32_t y)
{
    int32_t wx = ink_in(&UF_MARK, SPL_MARK, 240);
    ux_text_grad(wx, y, &UF_MARK, SPL_MARK, UX_BG);    /* (the gradient spans the whole word: the letters come out of it) */
    return wx;
}
static void splash_frame(uint32_t ms)
{
    char pre[sizeof SPL_MARK];
    int32_t sw, n, wx, x, k, len = (int32_t)sizeof SPL_MARK - 1;
    uint32_t key;
    if (ms > SPL_WRITE)
        ms = SPL_WRITE;
    sw = ms >= SPL_LINE_MS ? 240 : (int32_t)(ms * 240u / SPL_LINE_MS);
    n = ms < SPL_TYPE0 ? 0 : (int32_t)((ms - SPL_TYPE0) / SPL_TYPE_MS);
    if (n > len)
        n = len;
    key = (uint32_t)sw | (uint32_t)n << 8 | (ms >= SPL_LBL_MS ? 1u << 12 : 0u);
    if (key == spl_last)
        return;
    spl_last = key;
    cv_begin(240, SPL_CH, UX_BG);
    for (x = 0; x < sw; x++)
        cv_rect(x, 0, 1, 3, UX_BLEND[x * 255 / 239]);
    if (n) {
        wx = spl_mark(10);
        if (n < len) {
            for (k = 0; k < n; k++)
                pre[k] = SPL_MARK[k];
            pre[n] = 0;
            x = wx + text_w(&UF_MARK, pre);
            cv_rect(x, 10, 240 - x, UF_MARK.h, UX_BG);
        }
    }
    if (ms >= SPL_LBL_MS) {
        cv_text_in(0, 70, 240, &UF_LBL, FELUCCA_VERSION, UX_MID, UX_BG);
        cv_text_in(0, 84, 240, &UF_LBL, "BY NORFOLKHACKER", UX_MID, UX_BG);
    }
    cv_blit(0, SPL_Y);
}
static void draw_splash(void)
{
    lcd_fill(0, 0, 240, 240, UX_BG);
    spl_last = ~0u;
    splash_frame(spl_anim && !(ui_prefs & PREF_ANIM_OFF) ? 0u : SPL_WRITE);
    lcd_sync();
}
/* main.c's splash loop: the splash as it is ms after the start */
static void splash_write(uint32_t ms)
{
    if (!spl_anim || (ui_prefs & PREF_ANIM_OFF))
        return;
    splash_frame(ms);
    lcd_sync();
}
