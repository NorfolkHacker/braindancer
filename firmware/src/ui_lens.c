/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* (ACID FM 3a) HOME is three pages, TRACKS -> VISUAL -> GEN (ux_nav.c; GEN JIANT's GEN 1..3 until piece 6): a HOME tap
 * from elsewhere the one last shown (GEN too), on HOME the next one, round; SELECT walks TRACKS VISUAL GEN 1..3 as one chain;
 * HOME + SELECT straight to GEN 1; OCT- in GEN: TRACKS. Every other section as below. */
/* (0.9.2) The navigation, as the approved mock-up (navegacion.html): six rules, the same in every section.
 *   1 SELECT walks the section's pages as one chain (no levels, no zoom), stopping at its first and last page.
 *   2 The section's button: from another section, the page last shown there; already inside, the first page of the
 *     next group (round).
 *   3 PRESETS loads presets: the sounds everywhere, the patterns in SEQ (the PHRASES loader), the genre in GEN; never a
 *     page, a track or a module (ui_input.c presets_turn).
 *   4 A page that picks something picks it with KNOB 1 (the step, the bar, the slot, the kit's sound ..); the track is
 *     ALGORITHM's.
 *   5 A button held is its layer, as before (ui_layer.c); held + SELECT: HOME GEN, EDIT SEQ LFO FX their dice, GLO the
 *     tempo, ENV SCL ARP SAVE their chain (nav_jump).
 *   6 The screen says where: on top the section's groups as tabs (the one shown lit, a dot a page), at the foot
 *     SECTION . GROUP . page and n/N of the chain (ui_draw.c draw_tabs, draw_crumbs).
 * Each section's chain is a table (CH_*): the page (its PAGES title; HOME's are its own), the name shown, the group
 * and the level its pictures were drawn for (0.8.5's lenses: the panels, the cards and the knobs as they were, read
 * through lens_level / pd_level / home_level). Each section remembers the page last shown (ui.nmem, EDIT
 * one per kind of track); a page put by code brings its entry (nav_cur). Included by ui.c. */
enum { LZ_SEQ, LZ_SAVE, LZ_FX, LZ_HOME, LZ_EDIT, LZ_ENV, LZ_MOD, LZ_KEY, LZ_GLO, LZ_LAYER, LZ_DLG, LZ_NONE };
enum { PD_EDIT, PD_ENV, PD_MOD, PD_KEY, PD_GLO, PD_N };
static uint32_t pd_of_fam(uint32_t f)
{
    return f == FAM_EDIT ? PD_EDIT : f == FAM_ENV ? PD_ENV : f == FAM_LFO ? PD_MOD : f == FAM_SCL || f == FAM_ARP ? PD_KEY :
           f == FAM_GLO ? PD_GLO : PD_N;
}
static uint32_t edit_kind(void)                          /* 0 a synth, 1 FM6, 2 DRUM, 3 DIGITAL (FELUCCA_FM4) */
{
    uint32_t e = trk_eng(TSEL);
    return e == ENGI_FM6 ? 1u : e == ENGI_DRUM ? 2u : FELUCCA_FM4 && e == ENGI_DIGITAL ? 3u : 0u;
}
static int pd_on(void) { return !ui.home && !page_over && pd_of_fam(PAGES[ui.page].fam) < PD_N; }
static uint32_t pd_cur(void) { return pd_of_fam(PAGES[ui.page].fam); }

/* ------------------------------------------------------------------------------------------- the chains --- */
enum { SEC_HOME, SEC_EDIT, SEC_SEQ, SEC_FX, SEC_GLO, SEC_ENV, SEC_LFO, SEC_SCL, SEC_ARP, SEC_SAVE, SEC_N };
static const char *const SEC_NAME[SEC_N] = {"HOME", "EDIT", "SEQ", "FX", "GLO", "ENV", "LFO", "SCL", "ARP", "SAVE"};
static const uint8_t SEC_FAM[SEC_N] = {FAM_HOME, FAM_EDIT, FAM_SEQ, FAM_FX, FAM_GLO, FAM_ENV, FAM_LFO, FAM_SCL, FAM_ARP,
                                       FAM_SAVE};
typedef struct { const char *t, *nm; uint8_t g, lv; } nv_t;   /* PAGES title (HOME: 0), the name shown (0: t), group, level */
typedef struct { const char *nm, *tab; } ng_t;                /* a group: its name, its tab's when the name is too wide */
typedef struct { const nv_t *e; const ng_t *g; uint8_t n, ng; } ch_t;
#define NV(t, nm, g, lv) {t, nm, g, lv}
static const nv_t CE_HOME[] = {NV(0, "TRACKS", 0, 0), NV(0, "VISUAL", 1, 0),   /* (ACID FM 3a: ux_nav.h UXH_*; was 0.9.2's */
                               NV(0, "GEN 1", 2, 2), NV(0, "GEN 2", 2, 2), NV(0, "GEN 3", 2, 2)};   /* ECO MACROS BEING ROUTES
                                                         * then GEN) */
static const ng_t CG_HOME[] = {{"TRACKS", 0}, {"VISUAL", 0}, {"GEN", 0}};
/* (ACID FM 3b) a synth's EDIT (ANALOG PHASE LOFI NOISE VOICE), on the ux pages (ux_section.c): SOUND (E1..E4), TONE (E5..E8),
 * ANALOG's VOICE or the others' FILTER (one group: a track shows one of them), MORE; a page a group, so EDIT steps them
 * round (spec 2.1). JIANT's MORPH, RETRIG,
 * ROTOR, VOICE, VOICE 2, WORDS pages are MORE's rows now, FOCUS gone */
static const nv_t CE_SYN[] = {NV("EDIT 1", "SOUND", 0, 0), NV("EDIT 2", "TONE", 1, 0), NV("SYN VOICE", "VOICE", 2, 1),
                              NV("SYN FILTER", "FILTER", 2, 1), NV("SYN MORE", "MORE", 3, 1)};   /* (page 3: one of the two) */
static const ng_t CG_SYN[] = {{"SOUND", 0}, {"TONE", 0}, {"VOICE/FILTER", "PAGE 3"}, {"MORE", 0}};
/* (3b task 5) FM6's EDIT: SOUND (EDIT 1: ALGORITHM FEEDBACK MOD LEVEL MOD RATIO), OPERATOR (OP RATIO FINE LEVEL), OP ENV (OP STAGE
 * RATE LEVEL), MORE; DRUM-X's: KIT (EDIT 1: MORPH TUNE TONE DECAY), SOUND (DRUM LEVEL PITCH DECAY), SOUND 2 (DRUM NOISE COLOR
 * WAVE), MORE. JIANT's EDIT 2, FILTER, VOICE, VOICE 2, OPERATOR 2, KIT 2, KIT 3, SOUND 3 / 4, LEVELS, X-MOD, STEREO pages are
 * MORE's rows (or on these pages: LEVELS on SOUND, the drum picked) */
static const nv_t CE_FM6[] = {NV("EDIT 1", "SOUND", 0, 0), NV("FM6 OP", "OPERATOR", 1, 0), NV("FM6 OP ENV", "OP ENV", 2, 0),
                              NV("FM6 MORE", "MORE", 3, 1)};
static const ng_t CG_FM6[] = {{"SOUND", 0}, {"OPERATOR", "OP"}, {"OP ENV", 0}, {"MORE", 0}};
static const nv_t CE_DRM[] = {NV("EDIT 1", "KIT", 0, 0), NV("DRUM SOUND", "SOUND", 1, 0), NV("DRUM SOUND 2", "SOUND 2", 2, 0),
                              NV("DRUM MORE", "MORE", 3, 1)};
static const ng_t CG_DRM[] = {{"KIT", 0}, {"SOUND", 0}, {"SOUND 2", 0}, {"MORE", 0}};
static const nv_t CE_DIG[] = {NV("EDIT 1", 0, 0, 0), NV("EDIT 2", 0, 0, 0), NV("FILTER", 0, 1, 1), NV("VOICE", 0, 1, 1),
                              NV("VOICE 2", 0, 1, 1), NV("OP1 ENV", 0, 2, 2), NV("OP2 ENV", 0, 2, 2), NV("OP3 ENV", 0, 2, 2),
                              NV("OP4 ENV", 0, 2, 2), NV("OP LEVEL", 0, 2, 2)};
static const ng_t CG_DIG[] = {{"SOUND", 0}, {"TONE", 0}, {"OPERATOR", "OPS"}};
enum { LV_PAT, LV_BAR, LV_STEP, LV_N };                 /* SEQ's pictures: the bars, the roll, the step large */
/* (ACID FM 3c-1 task 2) SEQ = STEPS -> STEP EXTRAS -> PATTERN -> MORE (spec 2), opening on STEPS, a page a group (its button steps
 * them round), all on the ux pages (ux_seq.c, ux_section.c; task 3: STEP EXTRAS, the row JIANT's hidden CHANCE was; task 4: MORE,
 * the inherited SEQ pages' settings: params.c UXM_SEQ) */
static const nv_t CE_SEQ[] = {NV("STEP", "STEPS", 0, LV_BAR), NV("STEP EXTRAS", 0, 1, LV_STEP), NV("PATTERN", 0, 2, LV_PAT),
                              NV("SEQ MORE", "MORE", 3, LV_PAT)};
static const ng_t CG_SEQ[] = {{"STEPS", 0}, {"STEP EXTRAS", "EXTRAS"}, {"PATTERN", 0}, {"MORE", 0}};
/* (ACID FM 3b task 8) FX on the ux pages (ux_section.c), a page a group (its button steps them round, spec 2.4): SENDS (JIANT's
 * FX: the track's DISTORT, CHORUS, DELAY, REVERB), SPACE (DLY TIME, DLY FDBK, REV TYPE, REV SIZE), MASTER (CLIP, PUNCH, DUCK, DUCK
 * TIME), MORE (the rest of JIANT's sixteen pages: params.c UXM_FX). The DJ FILTER stays GLO's */
static const nv_t CE_FX[] = {NV("FX", "SENDS", 0, 0), NV("FX SPACE", "SPACE", 1, 0), NV("MASTER", 0, 2, 0),
                             NV("FX MORE", "MORE", 3, 0)};
static const ng_t CG_FX[] = {{"SENDS", 0}, {"SPACE", 0}, {"MASTER", 0}, {"MORE", 0}};
/* (ACID FM 3c-1 task 6) GLO = LIVE -> MIX -> MIDI -> MORE (spec 3), a page a group (its button steps them round), all on the ux
 * pages (ux_glo.c): LIVE (0.9.11's first page), (task 7) MIX (LEVEL PAN SWING TUNE), (task 8) MIDI (IN OUT CHANNEL of the selected
 * track, CLOCK), MORE (the RAILS; SYSTEM, TEMPO, MIDI, UNDO). The inherited MIXER, SENDS, GLOBAL and SYSTEM gone (task 7: MIX's
 * knobs, MORE's rows) */
static const nv_t CE_GLO[] = {NV("LIVE", 0, 0, 0), NV("MIX", 0, 1, 0), NV("MIDI", 0, 2, 0), NV("GLO MORE", "MORE", 3, 0)};
static const ng_t CG_GLO[] = {{"LIVE", 0}, {"MIX", 0}, {"MIDI", 0}, {"MORE", 0}};
/* (ACID FM 3b task 6) ENV, SCL, ARP on the ux pages (ux_section.c), a page a group (the button steps them round, spec 2.2,
 * 2.5): ENV's AMP (ATTACK DECAY SUSTAIN RELEASE) and ENV TO (the same envelope's FILTER PITCH SHAPE amounts, LOOP; the
 * user's name, task 6 fix: the spec's MOD ENV read as a second envelope), JIANT's
 * TRACKS views (the six tracks' envelopes) and STAGE (the same four values a stage at a time) gone; SCL's KEY (ROOT SCALE SNAP
 * ALL TRACKS), CHORD (CHORD VOICING STRUM ROTATE), MORE (SEQ PITCH, TRANSPOSE); ARP's ARP, ARP 2 */
static const nv_t CE_ENV[] = {NV("ENV", "AMP", 0, 0), NV("ENV DEST", "ENV TO", 1, 0)};
static const ng_t CG_ENV[] = {{"AMP", 0}, {"ENV TO", 0}};
/* (ACID FM 3b task 7) LFO on the ux pages, a page a group (spec 2.3): LFO (RATE WAVE MODE FADE: its wave scrolling), DEST (the
 * LFO's PITCH FILTER SHAPE AMP), MOD MATRIX (JIANT's MOD: SLOT SOURCE DEST AMOUNT, the slot picked a track each), MORE (PHASE;
 * the STEPS, JIANT's MSEQ and MSEQ 2). JIANT's LFO 2 (PHASE alone) and MSEQ pages are MORE's rows */
static const nv_t CE_LFO[] = {NV("LFO", 0, 0, 0), NV("LFO DEST", "DEST", 1, 0), NV("MOD", "MOD MATRIX", 2, 0),
                              NV("LFO MORE", "MORE", 3, 0)};
static const ng_t CG_LFO[] = {{"LFO", 0}, {"DEST", 0}, {"MOD MATRIX", "MATRIX"}, {"MORE", 0}};
static const nv_t CE_SCL[] = {NV("SCL", "KEY", 0, 0), NV("CHORD", 0, 1, 0), NV("SCL MORE", "MORE", 2, 0)};
static const ng_t CG_SCL[] = {{"KEY", 0}, {"CHORD", 0}, {"MORE", 0}};
static const nv_t CE_ARP[] = {NV("ARP", 0, 0, 0), NV("ARP 2", 0, 1, 0)};
static const ng_t CG_ARP[] = {{"ARP", 0}, {"ARP 2", 0}};
/* (ACID FM 3c-2 task 2) SAVE = SOUND -> SONG -> PROJECT -> MORE (spec 2), a page a group, all on the ux pages (ux_save.c): SOUND
 * (the inherited USER and PRESETS pages), (task 3) SONG (SECTIONS and ARRANGE), (task 4) PROJECT (PROJECTS), MORE (params.c UXM_SAVE:
 * TOOLS' rows too). The levels (SV_*, save_level) gone with the inherited LIBRARY */
static const nv_t CE_SAVE[] = {NV("SAVE SOUND", "SOUND", 0, 0), NV("SAVE SONG", "SONG", 1, 0), NV("SAVE PROJECT", "PROJECT", 2, 0),
                               NV("SAVE MORE", "MORE", 3, 0)};
static const ng_t CG_SAVE[] = {{"SOUND", 0}, {"SONG", 0}, {"PROJECT", 0}, {"MORE", 0}};
#define CH(e, g) {e, g, (uint8_t)NELEM(e), (uint8_t)NELEM(g)}
#define NAV_KEYS (SEC_N + 3u)                           /* the sections' memories: EDIT's synth one, then FM6 DRUM DIGITAL */
static const ch_t CHAINS[NAV_KEYS] = {CH(CE_HOME, CG_HOME), CH(CE_SYN, CG_SYN), CH(CE_SEQ, CG_SEQ), CH(CE_FX, CG_FX),
                                      CH(CE_GLO, CG_GLO), CH(CE_ENV, CG_ENV), CH(CE_LFO, CG_LFO), CH(CE_SCL, CG_SCL),
                                      CH(CE_ARP, CG_ARP), CH(CE_SAVE, CG_SAVE), CH(CE_FM6, CG_FM6), CH(CE_DRM, CG_DRM),
                                      CH(CE_DIG, CG_DIG)};
/* the page each opens on at power-on (the mock-up's): SEQ its STEP (STEPS), ENV its ENV, LFO its LFO, (3c-1 task 7) GLO its LIVE
 * (the user: LIVE first in GLO); ui.nmem holds it ^ this */
static const uint8_t NAV_DEF[NAV_KEYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};   /* (3b task 6: ENV its AMP, 0; task 7: LFO
                                                         * its LFO, now the first) */   /* (0.9.11: GLO its MIXER; 0.9.18: SEQ its
                                                         * STEP, 5 with BAR gone; ACID FM 3c-1 task 2: SEQ its STEPS, the first;
                                                         * task 6: GLO its MIXER, 3, until task 7: LIVE, 0) */
static uint32_t nav_key(uint32_t sec) { uint32_t k = sec == SEC_EDIT ? edit_kind() : 0u; return k ? SEC_N - 1u + k : sec; }
static const ch_t *nav_ch(uint32_t sec) { return &CHAINS[nav_key(sec)]; }
static uint32_t nav_mem(uint32_t key) { return (uint32_t)(ui.nmem[key] ^ NAV_DEF[key]); }
static void nav_mem_put(uint32_t key, uint32_t e) { ui.nmem[key] = (uint8_t)(e ^ NAV_DEF[key]); }
static uint32_t sec_of_fam(uint32_t f)
{
    uint32_t s;
    for (s = SEC_EDIT; s < SEC_N; s++)
        if (SEC_FAM[s] == f)
            return s;
    return SEC_HOME;
}
static int gen_on(void) { return ui.confirm == CF_DICE_ALL; }   /* GEN: HOME's last group (the generator's screen) */
static int dlg_on(void) { return ui.confirm && ui.confirm != CF_DICE_ALL; }   /* a dialog over the page */
static uint32_t gen_page(void);                         /* (gen.c) */
static uint32_t nav_sec(void) { return ui.home || gen_on() ? SEC_HOME : sec_of_fam(PAGES[ui.page].fam); }
/* entry e of section sec: its PAGES index shown now (NPAGES: not on this track, LOAD ECO's, HOME's own) */
static uint32_t nv_page(uint32_t sec, uint32_t e)
{
    const ch_t *c = nav_ch(sec);
    uint32_t i, f = SEC_FAM[sec];
    if (e >= c->n || !c->e[e].t)
        return NPAGES;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == f && str_eq(PAGES[i].title, c->e[e].t) &&
            page_visible(i))
            return i;
    return NPAGES;
}
static int nv_ok(uint32_t sec, uint32_t e) { return sec == SEC_HOME ? e < NELEM(CE_HOME) : nv_page(sec, e) < NPAGES; }
/* the entry shown: HOME's its page (ux_nav.c) or GEN's; the others the one remembered when it is the page
 * shown, else the page's own (put by code: in the group left, the default page's, then the chain's first), else
 * (PHRASES, CHANCE: out of the chain) the one left */
static uint32_t nav_cur(void)
{
    uint32_t sec = nav_sec(), key = nav_key(sec), e = nav_mem(key), k, pass, s;
    const ch_t *c = &CHAINS[key];
    if (sec == SEC_HOME)
        return gen_on() ? UXH_GEN0 + gen_page() : ux_home_page();
    s = ((((ui.page * 31u + key) * 131u + e) * 257u + TSEL->eng_req) * 8191u + (uint32_t)(uint16_t)TSEL->p[P_E1]) * 7u +
        sys_load + 1u;
    if (s == ui.nav_sig)
        return ui.nav_at;
    if (e >= c->n || nv_page(sec, e) != ui.page) {     /* (the group left, the default's, then any: ENV's ENV page in */
        uint32_t g0 = c->e[e < c->n ? e : 0].g, g1 = c->e[NAV_DEF[key]].g;   /* ENV, not TRACKS) */
        for (pass = 0; pass < 3u; pass++)
            for (k = 0; k < c->n; k++)
                if ((pass == 2u || c->e[k].g == (pass ? g1 : g0)) && nv_page(sec, k) == ui.page) {
                    nav_mem_put(key, k);
                    e = k;
                    pass = 3u;
                    break;
                }
        if (e >= c->n)
            e = 0;
    }
    ui.nav_sig = ((((ui.page * 31u + key) * 131u + e) * 257u + TSEL->eng_req) * 8191u + (uint32_t)(uint16_t)TSEL->p[P_E1]) * 7u +
                 sys_load + 1u;
    ui.nav_at = e;
    return e;
}
/* (3c-1 task 10) EDIT's page pg left behind by an engine change (not on this track now): this track's page in the same group
 * (page 3 stays page 3: ANALOG's VOICE to PHASE's FILTER, FM6's OP ENV, DRUM-X's SOUND 2; MORE stays MORE), remembered as
 * EDIT's; NPAGES none */
static uint32_t nav_edit_same(uint32_t pg)
{
    static const uint8_t KEYS[] = {SEC_EDIT, SEC_N, SEC_N + 1u, SEC_N + 2u};   /* (nav_key: synth, FM6, DRUM, DIGITAL) */
    const ch_t *c;
    uint32_t i, k, g = 0xFFu, p;
    for (i = 0; i < NELEM(KEYS) && g == 0xFFu; i++)
        for (k = 0; k < CHAINS[KEYS[i]].n; k++)
            if (CHAINS[KEYS[i]].e[k].t && str_eq(CHAINS[KEYS[i]].e[k].t, PAGES[pg].title)) {
                g = CHAINS[KEYS[i]].e[k].g;
                break;
            }
    c = nav_ch(SEC_EDIT);
    for (k = 0; g != 0xFFu && k < c->n; k++)
        if (c->e[k].g == g && (p = nv_page(SEC_EDIT, k)) < NPAGES) {
            nav_mem_put(nav_key(SEC_EDIT), k);
            return p;
        }
    return NPAGES;
}
static uint32_t nav_lv(void) { return nav_ch(nav_sec())->e[nav_cur()].lv; }   /* the 0.8.5 lens level it is drawn as */
static uint32_t pd_level(void) { return nav_lv(); }
static uint32_t home_level(void) { return gen_on() ? 2u : 0u; }   /* (the 0.8.5 lens level: GEN 2, HOME's pages 0) */
static int home_macros(void) { return ui.home && !ui.menu && ux_home_page() == UXH_VISUAL; }   /* VISUAL: M1..M4 (spec 4.3;
                                                         * ux_home_page: GEN while it is open) */
/* the entries shown now (on this track, at this LOAD), in order: their indices in v; how many */
static uint32_t nav_list(uint32_t sec, uint8_t *v)
{
    const ch_t *c = nav_ch(sec);
    uint32_t k, n = 0;
    for (k = 0; k < c->n; k++)
        if (nv_ok(sec, k))
            v[n++] = (uint8_t)k;
    return n;
}
static void go_home(void);
static void page_entered(void);
static void page_flash(void);
static void confirm_open(uint32_t kind, uint32_t trk);
static void gen_open(void);
static void gen_page_put(uint32_t p);
static int chain_busy(void);
static void ui_message(const char *s);
/* entry e of section sec shown (one that is not here: the nearest after it, else before); outside: from another
 * section (SAVE: SOUND (3c-2 task 2: was USER) comes with its SAVE armed, SAVE . OCT+ . OCT+ as before; SECTIONS did, gone in
 * task 3) */
static void nav_go(uint32_t sec, uint32_t e, int outside)
{
    const ch_t *c = nav_ch(sec);
    uint32_t key = nav_key(sec), k, i;
    if (e >= c->n)
        e = 0;
    for (k = e; k < c->n && !nv_ok(sec, k); k++) {}
    if (k >= c->n)
        for (k = e; k-- > 0u && !nv_ok(sec, k);) {}
    if (k >= c->n)
        return;
    e = k;
    if (sec == SEC_HOME && e >= UXH_GEN0 && !gen_on() && chain_busy()) {
        uint32_t p = ui.home && !ui.menu ? ux_home_page() : UXH_TRACKS;
        ui_message("STOP TO EDIT");                     /* (GEN refused while the song plays, said why (spec 0 rule 6): the */
        e = p < UXH_GEN0 ? p : UXH_TRACKS;              /* page stays (VISUAL), from elsewhere TRACKS) */
    }
    nav_mem_put(key, e);
    if (sec == SEC_HOME) {
        if (e >= UXH_GEN0) {                            /* GEN: the generator's screen, HOME's third page */
            if (!gen_on()) {
                confirm_open(CF_DICE_ALL, song.sel);
                gen_open();
            }
            gen_page_put(e - UXH_GEN0);
        } else if (gen_on()) {
            ui.confirm = 0;
        }
        if (!ui.home)
            go_home();
        ui.force = 1;
        page_flash();
        return;
    }
    if (gen_on())
        ui.confirm = 0;
    i = nv_page(sec, e);
    ui.page = (uint8_t)i;
    ui.home = 0;
    page_entered();
    if (PAGES[i].graph == GR_SVSND)
        ui.act = outside ? 4u : 0u;
    page_flash();
}
/* the entry section sec's button goes to from the one shown: the next group's first with a page here (ARP's one: its first);
 * 0xFF none. (3b task 5 fix: the footer names it, ux_section.c uxs_next) */
static uint32_t nav_next(uint32_t sec)
{
    const ch_t *c = nav_ch(sec);
    uint32_t g = c->e[nav_cur()].g, j, k;
    for (j = 1; j <= c->ng; j++)
        for (k = 0; k < c->n; k++)
            if (c->e[k].g == (g + j) % c->ng && nv_ok(sec, k))
                return k;
    return 0xFFu;
}
/* rule 2: a section's button. HOME (ACID FM 3a): from elsewhere the page last shown on HOME, on it the next one, round */
static void nav_button(uint32_t sec)
{
    uint32_t e;
    if (sec == SEC_HOME) {
        if (ui.home || gen_on())
            ux_home_flip();
        else
            nav_go(SEC_HOME, ux_home_entry(), 1);
        return;
    }
    if (nav_sec() != sec || ui.home) {
        nav_go(sec, nav_mem(nav_key(sec)), 1);
        return;
    }
    if ((e = nav_next(sec)) != 0xFFu)                   /* (the next group with a page here; ARP's one: its first) */
        nav_go(sec, e, 0);
}
/* rule 1: SELECT, s detents along the chain (no wrap) */
static void nav_select(int32_t s)
{
    uint8_t v[16];
    uint32_t sec = nav_sec(), n = nav_list(sec, v), e = nav_cur(), i;
    int32_t at = -1, to;
    for (i = 0; i < n; i++)
        if (v[i] == e)
            at = (int32_t)i;
    if (at < 0) {                                       /* (the entry left is not here: the nearest that is) */
        for (i = 0; i < n && v[i] < e; i++) {}
        at = (int32_t)i - (s > 0 ? 1 : 0);
    }
    to = clamp(at + s, 0, (int32_t)n - 1);
    if (n && (to != at || v[to] != e))
        nav_go(sec, v[to], 0);
}
/* rule 5: ENV / SCL / ARP / SAVE held + SELECT: into the section (its page last shown), then along its chain */
static void nav_jump(uint32_t sec, int32_t s)
{
    if (nav_sec() != sec || ui.home)
        nav_go(sec, nav_mem(nav_key(sec)), 1);
    if (nav_sec() == sec)
        nav_select(s);
}
/* rule 6: the groups shown (with a page here): their number n, the one shown (*g), its page's place in it (*p), each
 * one's pages (cnt) and chain index of the first (first); the chain's place *i of *m */
static uint32_t nav_groups(uint8_t *cnt, uint8_t *gi, uint32_t *g, uint32_t *p, uint32_t *i, uint32_t *m)
{
    uint8_t v[16];
    uint32_t sec = nav_sec(), n = nav_list(sec, v), e = nav_cur(), k, ng = 0, last = 0xFFu;
    const ch_t *c = nav_ch(sec);
    *g = *p = *i = 0;
    *m = n;
    for (k = 0; k < n; k++) {
        uint32_t gg = c->e[v[k]].g;
        if (gg != last) {
            cnt[ng] = 0;
            gi[ng++] = (uint8_t)gg;
            last = gg;
        }
        if (v[k] == e) {
            *g = ng - 1u;
            *p = cnt[ng - 1u];
            *i = k;
        }
        cnt[ng - 1u]++;
    }
    return ng;
}
static const char *nav_group_name(uint32_t g) { const ch_t *c = nav_ch(nav_sec()); return c->g[g % c->ng].nm; }
static const char *nav_group_tab(uint32_t g)
{
    const ch_t *c = nav_ch(nav_sec());
    return c->g[g % c->ng].tab ? c->g[g % c->ng].tab : c->g[g % c->ng].nm;
}
/* the name in the chain of entry e of section sec, page pg: EDIT 1 . OSC (the engine's page name), KIT, TRACKS DEST,
 * SENDS .. (nav_page_name: the page shown; 0.9.12 the locks' page, ui_draw.c foot_crumbs) */
static __attribute__((noinline)) const char *nv_name(uint32_t sec, uint32_t e, uint32_t pg)
{
    static char b[24];
    const nv_t *v = &nav_ch(sec)->e[e];
    if (sec != SEC_HOME && v->t && !str_eq(PAGES[pg].title, v->t)) {   /* (PHRASES, CHANCE: their own) */
        str_cpy(b, PAGES[pg].title, 24);
        return b;
    }
    str_cpy(b, v->nm ? v->nm : v->t, 24);
    if (sec == SEC_EDIT && v->g == 0u && !v->nm && PAGES[pg].scope == SC_ENGINE) {   /* (EDIT 1 . OSC) */
        const char *pt = trk_engp(TSEL)->page_title[PAGES[pg].id[0] != P_E0];
        if (pt && pt[0] && pt[0] != '-') {
            str_cpy(b + str_len(b), " \xB7 ", 4);
            str_cpy(b + str_len(b), pt, 24 - str_len(b));
        }
    }
    return b;
}
static const char *nav_page_name(void) { return nv_name(nav_sec(), nav_cur(), ui.page); }
/* (code, the tests, the renders) the page of section sec named nm in its chain */
static int nav_put(uint32_t sec, const char *nm)
{
    const ch_t *c = nav_ch(sec);
    uint32_t k;
    for (k = 0; k < c->n; k++)
        if (str_eq(c->e[k].nm ? c->e[k].nm : c->e[k].t, nm) && nv_ok(sec, k)) {
            nav_go(sec, k, 0);
            return 1;
        }
    return 0;
}
