/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* MENU's rows and their settings: what ux_menu.c draws and steps, and what the editor reads and sets (editor_menu.c
 * MENU_DESC / MENU_SET). One apply path for both: menu_put. A row is a setting (its MI_* is its place in the menu's
 * tables, not stored: the editor's ids are editor_menu.c ED_MENU's). (ACID FM 3c-2 task 5) On three pages of four knobs and
 * MORE (MN_ROW, UXM_MENU below): AUDIO (SPEAKER EQ TRACKS LOAD BPM LOCK), CLICK (CLICK CLICK LEVEL COUNT-IN), CONTROLS (HOLD
 * TIME KNOB SPEEDUP FX LATCH CALIBRATION), MORE (SCREEN: LARGE ANIMATION SCREEN SLEEP; LEDS: LED GLOW SCALE LEDS; USB: USB AUDIO
 * USB LEVEL; SYSTEM: CPU CORES CPU CLOCK RESTORE LAST, what a power-on does; MIDI: MIDI IS ON GLO, a pointer; ABOUT). 0.9.18
 * .. 0.5: eight pages in five tabs (SCREEN AUDIO MIDI USB SYSTEM); until 0.9.17 one list in the tabs DISPLAY CONTROL AUDIO
 * MIDI SYSTEM, SELECT a row, any knob its value; the stored values and their meanings are the same. (ACID FM 3c-1 task 8) LEARN
 * and LEARN CLEAR are GLO > MORE's rows (params.c ID_ML_LEARN, ux_more.c UXM_A_LCLEAR); the map travels with the settings */
#include "cpu_safe.c"        /* (0.9.12) MENU > SYSTEM > CPU CORES, CPU CLOCK; the last crash kept in the settings */
enum { MI_COLOR, MI_SKIN, MI_SCOPE, MI_STYLE, MI_LARGE, MI_ANIM, MI_LEDS, MI_SCROFF, MI_HOLD, MI_ACCEL, MI_LATCH, MI_BPMLOCK, MI_SCLLED,
       MI_LOWCUT, MI_USB,
       MI_CLICK, MI_CLKLVL, MI_COUNTIN, MI_TRACKS, MI_LOAD, MI_MIDIOUT, MI_SERIAL, MI_RESTORE, MI_CPU,
       MI_CLOCK, MI_PANEL, MI_ABOUT, MI_GLOMIDI, MI_COUNT };   /* (1.1: the metronome's rows; 1.2: RESTORE LAST, SCALE LEDS; 0.9.12:
                      * MIDI OUT, LEARN, LEARN CLEAR; CPU, CPU CLOCK, cpu_safe.c; 0.9.18: INIT SYSTEM gone, SAVE > TOOLS > NEW
                      * PROJECT is the same; ACID FM 0.5, 3c-1 task 8: LEARN and LEARN CLEAR GLO > MORE's rows, MIDI OUT on no page
                      * (the editor's id 20: GLO > MORE's NOTES OUT and CLOCK OUT in its four words), MI_GLOMIDI MENU > MIDI's
                      * pointer, an action: OCT+ opens GLO > MIDI) */
#define MI_VALUES MI_PANEL
#define MI_NONE 0xFFu
static int menu_valued(uint32_t row) { return row < MI_VALUES; }   /* (a row with a value) */
/* (ACID FM 0.3) COLOR, SKIN, STYLE: one palette, one skin, one style. Their rows are on no page of the MENU; the editor's
 * ids (editor_menu.c ED_MENU 0, 1, 21) answer: one value, "ACID", 0; a set is taken and changes nothing. (ACID FM 3b) SCOPE
 * the same (id 22: one value, OFF): it chose between the synths' creatures and the scope on EDIT, both gone (the synths'
 * EDIT draws its wave, ux_section.c); its settings byte stays where it was (ui.c ui_scope), unread */
static int menu_hidden(uint32_t row) { return row == MI_COLOR || row == MI_SKIN || row == MI_STYLE || row == MI_SCOPE; }
/* (0.9.18) the full names (the panel's rows, the editor's MENU_DESC; plain words, at most 12 characters) */
static const char *const MI_NAME[MI_COUNT] = {"COLOR", "SKIN", "SCOPE", "STYLE", "LARGE", "ANIMATION", "LED GLOW", "SCREEN SLEEP",
                                              "HOLD TIME", "KNOB SPEEDUP", "FX LATCH", "BPM LOCK", "SCALE LEDS",
                                              "SPEAKER EQ", "USB LEVEL", "CLICK", "CLICK LEVEL", "COUNT-IN", "TRACKS", "LOAD",
                                              "MIDI OUT", "USB AUDIO",
                                              "RESTORE LAST", "CPU CORES", "CPU CLOCK", "CALIBRATION", "ABOUT",
                                              "MIDI IS ON GLO"};   /* (the pointer's: its one row, a sentence) */
/* (ACID FM 3c-2 task 5, spec 3) MENU as a section: AUDIO, CLICK, CONTROLS (KNOB n a page's setting n), then MORE (a list, ux_more.c:
 * KNOB 1 picks, KNOB 2 sets); SELECT walks them and scrolls MORE (ux_menu.c). Each setting in one place (tests/ui_test.c
 * test_u5_cover); COLOR SKIN STYLE SCOPE (menu_hidden) and MIDI OUT (GLO > MORE's NOTES OUT, CLOCK OUT) on none. An action
 * (CALIBRATION; MORE's MIDI IS ON GLO, ABOUT): armed by its knob, OCT+ does it. The editor gets a row's page as its MENU_DESC
 * tab (mi_tab; 0.9.18 .. 0.5: SCREEN AUDIO MIDI USB SYSTEM, eight pages; page indexes are not ids). The inherited tabs, their
 * pages and ALGORITHM between them gone */
enum { MN_AUDIO, MN_CLICK, MN_CONTROLS, MN_MORE, MN_N };
static const char *const MN_NAME[MN_N] = {"AUDIO", "CLICK", "CONTROLS", "MORE"};
static const uint8_t MN_ROW[MN_MORE][4] = {{MI_LOWCUT, MI_TRACKS, MI_LOAD, MI_BPMLOCK},   /* (a page opens on its first: a value) */
                                           {MI_CLICK, MI_CLKLVL, MI_COUNTIN, MI_NONE},
                                           {MI_HOLD, MI_ACCEL, MI_LATCH, MI_PANEL}};
#define MR(r) {(r), UXR_MENU, 0}
#define MH(h) {(h), UXR_HEAD, 0}
static const ux_row_t UXM_MENU_ROWS[] = {
    MH(UXM_H_SCREEN), MR(MI_LARGE), MR(MI_ANIM), MR(MI_SCROFF),
    MH(UXM_H_LEDS), MR(MI_LEDS), MR(MI_SCLLED),
    MH(UXM_H_USB), MR(MI_SERIAL), MR(MI_USB),
    MH(UXM_H_SYSTEM), MR(MI_CPU), MR(MI_CLOCK), MR(MI_RESTORE),   /* (what a power-on does: their hints say so) */
    MH(UXM_H_MIDI), MR(MI_GLOMIDI),                    /* (a pointer: OCT+ opens GLO > MIDI) */
    MH(UXM_H_ABOUT), MR(MI_ABOUT),                     /* (an action: OCT+ opens ABOUT) */
};
#undef MR
#undef MH
static const ux_more_t UXM_MENU = {UXM_MENU_ROWS, (uint8_t)NELEM(UXM_MENU_ROWS)};
static uint32_t mi_tab(uint32_t row)                   /* the page a row is on (MN_*): MORE's rows, and those on none, MN_MORE */
{
    uint32_t p, c;
    for (p = 0; p < MN_MORE; p++)
        for (c = 0; c < 4u; c++)
            if (MN_ROW[p][c] == row)
                return p;
    return MN_MORE;
}
/* a row's knob label (its column's 48 px in UF_LBL; the list and the panel name it in full, MI_NAME): a whole word of its name */
static const char *mi_short(uint32_t row)
{
    static const struct { uint8_t row; char s[10]; } SH[] = {
        {MI_LOWCUT, "SPEAKER"}, {MI_CPU, "CORES"}, {MI_CLKLVL, "LEVEL"}, {MI_USB, "LEVEL"}, {MI_SERIAL, "USB"},
        {MI_ANIM, "ANIMATE"}, {MI_SCROFF, "SLEEP"}, {MI_HOLD, "HOLD"}, {MI_ACCEL, "SPEEDUP"}, {MI_SCLLED, "SCALE"},
        {MI_RESTORE, "RESTORE"}, {MI_CLOCK, "CLOCK"}, {MI_PANEL, "CALIBRATE"}, {MI_GLOMIDI, "GLO"}, {MI_LEDS, "GLOW"}};
    uint32_t k;
    for (k = 0; k < NELEM(SH); k++)
        if (SH[k].row == row)
            return SH[k].s;
    return row < MI_COUNT ? MI_NAME[row] : "";
}
static const char *const SCOPEOPT_N[2] = {"OFF", "ON"};   /* (SCOPE's one value, OFF) */
/* the two-valued rows: a bit of ui_prefs (PREF_REC: of ui_rec_prefs, ui.c PREF_SCALE_LEDS) and its names (bit clear =
 * the default, bit set). A step up (a knob right,
 * OCT+) = ON on the ON / OFF rows (the switch's knob to the right; ANIM, USB SERIAL: their bit clears), else the
 * second name (USB LEVEL FIXED); a step down the other (menu_step) */
typedef struct { uint8_t row; uint16_t bit; const char *name[2]; } menu_flag_t;
static const menu_flag_t MENU_FLAGS[] = {
    {MI_LARGE, PREF_LARGE, {"OFF", "ON"}},            /* #15 / Discussion #80: ON, big knob labels and values (ui.c large_kind) */
    {MI_ANIM, PREF_ANIM_OFF, {"ON", "OFF"}},          /* #46: OFF, values snap (ui_draw.c roll_note) */
    {MI_ACCEL, PREF_ACCEL_OFF, {"ON", "OFF"}},        /* #52: ui_input.c accel (JIANT: ON by default) */
    {MI_LATCH, PREF_LATCH, {"OFF", "ON"}},
    {MI_BPMLOCK, PREF_BPM_LOCK, {"OFF", "ON"}},        /* #58: ON, DICE ALL keeps the tempo (0.6.67: SELECT alone never sets it) */
    {MI_RESTORE, PREF_RESTORE_ON, {"OFF", "ON"}},      /* 1.2, #130; JIANT 0.6.40: OFF (the default), no autosave, power-on as
                                                        * INIT SYSTEM; ON, the last session back (project.c) */
    {MI_SCLLED, PREF_SCALE_LEDS, {"OFF", "ON"}},      /* 1.2, Discussion #127: ON, the keys show the scale (ui_input.c) */
    {MI_TRACKS, PREF_SIX, {"4", "6"}},                /* (JIANT 0.6.18) 4 tracks or all 6, the default since ACID FM 3b (voice.c trk_live) */
};
/* SPEAKER EQ (settings.lowcut, fx.c fx_lowcut): an EQ on the master for the small speaker, not a speaker switch
 * (#42: "OFF" read as the speaker off). FLAT is the old OFF (0, stored as before). The built-in speaker cannot be
 * turned off from the firmware (no amp enable or mute line is known); the EQ reaches
 * the headphone / line out and USB audio too (one DAC, uac_tap reads the master) */
static const char *const SPK_EQ[3] = {"FLAT", "LOWCUT", "BASS+"};
/* (0.7.0) USB LEVEL: MASTER (USB follows the knob, the default), FIXED (full level, the knob after: #42), then fixed
 * and attenuated (-6 .. -24 dB: a phone recording over USB clipped at the full level). ui.c usb_lvl, fx.c fx_usb_gain */
static const char *const USBLVL_N[6] = {"MASTER", "FIXED", "-6 dB", "-12 dB", "-18 dB", "-24 dB"};
/* (0.7.1) MENU > SYSTEM > USB (was USB SERIAL ON / OFF, #67): CONSOLE, the serial console with the stereo audio (the
 * default, PREF_SERIAL_OFF clear: the descriptors as before); STEREO, audio + MIDI only (class 0: the old OFF); STEMS,
 * audio as 8 channels (the six parts mono, the FX return L R; fx.c, usb.c), no console; (0.7.5) IN+OUT, the stereo
 * audio and the host's audio output too (usb.c usb_duplex: the computer or the phone plays into the last track), no
 * console; (0.8.0) STEMS+IN, the stems with the host's audio output too (10 channels in: the stems, then the host's
 * audio as the last track mixed it and as it came; the phone app records a voice through the FM-1 and moves it by
 * the latency). ui.c usb_out */
static const char *const USBOUT_N[5] = {"CONSOLE", "STEREO", "STEMS", "IN+OUT", "STEMS+IN"};
static const char *const HOLD_N[4] = {"0.3 s", "0.4 s", "0.5 s", "0.6 s"};   /* (HOLD TIME: the menu's and the editor's names) */
/* 1.1 (Discussion #131): the metronome (click.c) and the count-in (seq.c); ui.c ui_rec_prefs holds them */
static const char *const CLICK_N[3] = {"OFF", "REC", "ON"};          /* REC: while a track is armed and playing */
static const char *const CLKLVL_N[3] = {"LOW", "MID", "HIGH"};
static const char *const COUNTIN_N[3] = {"OFF", "1 BAR", "2 BARS"};
/* 1.1.5: SCREEN OFF (ui.c scr_*: the screen dark after this long without panel input; NEVER the default since 1.1.5.1) */
static const char *const SCROFF_N[5] = {"NEVER", "5 MIN", "15 MIN", "30 MIN", "60 MIN"};
/* (JIANT 0.6.72) MIDI OUT over USB (ui.c midi_out_mode, seq.c MO_*): the keys; and the clock + START / STOP; and every
 * note the FM-1 plays; nothing. (ACID FM 0.5) GLO > MORE's NOTES OUT and CLOCK OUT (ui.c midi_outs) read and set in these four
 * words (the editor's id 20; mout_mode its nearest of the six pairs) */
static const char *const MIDIOUT_N[4] = {"KEYS", "CLOCK", "ALL", "OFF"};

/* MENU > USB SERIAL (#67). The serial console is a developer tool (README: FELUCCA_CDC). ON (the default) presents it,
 * the descriptors byte for byte as before; OFF re-enumerates as audio + MIDI only, device class 0 (the bytes of a
 * FELUCCA_CDC=0 build): macOS 13-15 then attach their USB audio driver (with the console Apple's CDC composite driver
 * takes the device and the audio input never appears). USB-MIDI stays, and with it the editor, the update installer
 * and the soft key (SysEx on EP1). Applied at boot before USB starts (main.c: enumerated with it from the start, never
 * on then off) and while the menu is closed (ui_input: one re-enumeration when it closes, not one per KNOB 1 step).
 * A change from the editor (MENU_SET) waits 200 ms (menu_serial_at) so its reply leaves before the device drops off
 * the bus: the whole device re-enumerates, USB-MIDI and audio too */
static uint32_t menu_serial_at;                        /* fm1_ms | 1 of the editor's change; 0 none */
static void usb_serial_apply(void)
{
    if (menu_serial_at && fm1_ms - menu_serial_at < 200u)
        return;
    menu_serial_at = 0;
#if FELUCCA_CDC
    usb_cdc_switch(FELUCCA_CDC_DEFAULT && !(ui_prefs & PREF_SERIAL_OFF) && !usb_out);   /* (a FELUCCA_CDC_DEFAULT=0 build: off) */
#endif
#if FELUCCA_UAC
    usb_stems_switch(usb_out == 1u || usb_out == 3u);  /* (0.7.1) MENU > USB STEMS: the 8-channel device */
    usb_duplex_switch(usb_out == 2u || usb_out == 3u); /* (0.7.5) MENU > USB IN+OUT: the audio output too */
#endif                                                 /* ((0.8.0) STEMS+IN: both, the 10-channel device) */
    fx_stems = (uint8_t)(usb_out == 1u ? 1u : usb_out == 3u ? 2u : 0u);   /* (the parts write their stems from the next block) */
}
static const menu_flag_t *menu_flag(uint32_t row)
{
    uint32_t i;
    for (i = 0; i < NELEM(MENU_FLAGS); i++)
        if (MENU_FLAGS[i].row == row)
            return &MENU_FLAGS[i];
    return 0;
}

/* a row's value as 0..menu_n(row) - 1 in the order the menu steps it (LEDS: OFF DIM LO DIM HI INV, LEDS_MENU) */
static uint32_t menu_n(uint32_t row)
{
    return menu_hidden(row) ? 1u : row == MI_LOWCUT || (row >= MI_CLICK && row <= MI_COUNTIN) || row == MI_LOAD ? 3u :
           row == MI_HOLD || row == MI_MIDIOUT ? 4u : row == MI_USB ? 6u : row == MI_SERIAL ? 5u : row == MI_LEDS ? LEDS_COUNT : row == MI_SCROFF ? NELEM(SCROFF_N) : 2u;
}
static uint32_t menu_get(uint32_t row)
{
    const menu_flag_t *f = menu_flag(row);
    uint32_t i = 0;
    if (f)
        return ((ui_prefs | (uint32_t)ui_rec_prefs << 8) & f->bit) != 0;
    switch (row) {
    case MI_LOWCUT: return settings.lowcut % 3u;
    case MI_USB: return (ui_prefs & PREF_USB_FIXED) ? 1u + usb_lvl % 5u : 0u;
    case MI_SERIAL: return usb_out == 3u ? 4u : usb_out == 2u ? 3u : usb_out ? 2u : (ui_prefs & PREF_SERIAL_OFF) ? 1u : 0u;
    case MI_HOLD: return settings_hold % 4u;
    case MI_SCROFF: return scr_get();
    case MI_CLICK: case MI_CLKLVL: case MI_COUNTIN: return rp_get(row - MI_CLICK);
    case MI_LOAD: return load_get();
    case MI_MIDIOUT: return mout_mode();
    case MI_CPU: case MI_CLOCK: return cpu_get(row == MI_CLOCK);
    case MI_LEDS:
        while (i + 1u < LEDS_COUNT && LEDS_MENU[i] != settings_leds)
            i++;
        return i;
    }
    return 0;
}
static const char *menu_vname(uint32_t row, uint32_t v)
{
    const menu_flag_t *f = menu_flag(row);
    if (f)
        return f->name[v & 1u];
    switch (row) {
    case MI_COLOR: case MI_SKIN: case MI_STYLE: return UI_PALETTES[UI_ACID_INDEX].name;   /* (ACID FM 0.3: one value) */
    case MI_SCOPE: return SCOPEOPT_N[v & 1u];
    case MI_LOWCUT: return SPK_EQ[v % 3u];
    case MI_USB: return USBLVL_N[v % 6u];
    case MI_SERIAL: return USBOUT_N[v % 5u];
    case MI_HOLD: return HOLD_N[v & 3u];
    case MI_CLICK: return CLICK_N[v % 3u];
    case MI_CLKLVL: return CLKLVL_N[v % 3u];
    case MI_COUNTIN: return COUNTIN_N[v % 3u];
    case MI_LOAD: return N_LOAD[v % 3u];
    case MI_MIDIOUT: return MIDIOUT_N[v & 3u];
    case MI_CPU: case MI_CLOCK: return CPU_N[row == MI_CLOCK][v & 1u];
    case MI_SCROFF: return SCROFF_N[v % NELEM(SCROFF_N)];
    case MI_LEDS: return LEDS_NAME[LEDS_MENU[v % LEDS_COUNT]];
    }
    return "";
}
/* set a row (v below menu_n(row)) and do what the menu does on a change: the EQ and USB LEVEL at once;
 * LARGE, ANIM, LEDS, HOLD, the flags are read where they are used, every frame (COLOR, SKIN, STYLE: nothing);
 * USB SERIAL when the menu has closed (usb_serial_apply). Saved by the caller (menu_close, the editor) */
static void menu_put(uint32_t row, uint32_t v)
{
    const menu_flag_t *f = menu_flag(row);
    if (f) {
        if (f->bit & PREF_REC)
            ui_rec_prefs = (uint8_t)(v ? ui_rec_prefs | f->bit >> 8 : ui_rec_prefs & ~(f->bit >> 8));
        else
            ui_prefs = (uint8_t)(v ? ui_prefs | f->bit : ui_prefs & ~f->bit);
        fx_usb_fixed = (ui_prefs & PREF_USB_FIXED) != 0u;   /* (at once; every frame too: ui_input) */
        return;
    }
    switch (row) {
    case MI_LOWCUT: settings.lowcut = v; fx_lowcut = (uint8_t)v; break;
    case MI_USB:                                   /* (at once; every frame too: ui_input) */
        ui_prefs = (uint8_t)(v ? ui_prefs | PREF_USB_FIXED : ui_prefs & ~PREF_USB_FIXED);
        usb_lvl = (uint8_t)(v ? (v - 1u) % 5u : 0u);
        fx_usb_fixed = v != 0u;
        fx_usb_gain = USB_GAIN[usb_lvl];
        break;
    case MI_SERIAL:                                /* (0.7.1) applied when the menu closes (usb_serial_apply) */
        ui_prefs = (uint8_t)(v ? ui_prefs | PREF_SERIAL_OFF : ui_prefs & ~PREF_SERIAL_OFF);
        usb_out = (uint8_t)(v == 2u ? 1u : v == 3u ? 2u : v == 4u ? 3u : 0u);   /* (0.7.5: 2 IN+OUT; 0.8.0: 3 STEMS+IN) */
        break;
    case MI_HOLD: settings_hold = (uint8_t)v; break;
    case MI_SCROFF: scr_put(v); break;                 /* (read every frame: ui.c scr_frame) */
    case MI_LEDS: settings_leds = LEDS_MENU[v]; break;
    case MI_CLICK: case MI_CLKLVL: case MI_COUNTIN: rp_put(row - MI_CLICK, v); break;   /* (at once: click.c, seq.c) */
    case MI_LOAD: load_put(v % 3u); break;
    case MI_CPU: case MI_CLOCK: cpu_put(row == MI_CLOCK, v & 1u); break;   /* (0.9.12, cpu_safe.c) */
    case MI_MIDIOUT: midi_outs = (uint8_t)mout_of_mode(v); break;   /* (seq.c reads it every frame: ui_input) */
    }
}
/* the menu's step, any of KNOB 1..4 or OCT+ (s > 0) / OCT- (s < 0): the next / previous value, stopping at the ends
 * (until ACID FM 0.3 COLOR wrapped round its palettes). On an ON / OFF row up is ON (MENU_FLAGS) */
static uint32_t menu_step(uint32_t row, int32_t s)
{
    const menu_flag_t *f = menu_flag(row);
    uint32_t v = menu_get(row), n = menu_n(row);
    if (f && str_eq(f->name[0], "ON"))                 /* (ANIM, USB SERIAL, RESTORE LAST: ON is their value 0) */
        s = -s;
    if (s > 0)
        return v + 1u < n ? v + 1u : v;
    if (s < 0)
        return v ? v - 1u : 0u;
    return v;
}
