/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker */
/* (JIANT 0.9.4) The engines' own creatures, drawn where JIANT drew its beings (ui_being.c). (ACID FM 3b task 4: the user's
 * "get rid of any creatures": the synths' (VOICE's tract, ANALOG's cell, NOISE's swarm, PHASE's snail, LOFI's caterpillar)
 * gone with their EDIT pages, which draw their wave now (ux_section.c); the two left go with FM6's and DRUM-X's own EDIT
 * pages in 3b task 5.) Line art only, antialiased (ui_organic.c og_line), in the beings' thermal colours (bg_col: heat_col),
 * from small point tables (int8) and the synthesis state the UI already reads; nothing in the audio path. Each moves only
 * with its sound: in silence it rests (its time runs with the level, as the beings').
 *   DRUM-X  the kit as a colony of small beings with eyes and a mouth, standing as a drum kit does: the kick large in
 *           the middle (its core hot), the snare flat on its stand, the toms on little legs, the hi-hat two shells (CH
 *           together, OH the top one lifted, its mouth open), the crash on a long stem and the cowbell a small head (BELL),
 *           the clap two little ones clapping together, the rim (RS) a spark on the snare's edge. A hit squashes its
 *           being (springing back), shuts its eyes and opens its mouth in a shout, as hard as the hit (the lane's own
 *           envelope: its velocity, its accent; it recovers with the sound's DECAY): a GHOST a small squash, a FLAM two
 *           (its hit to come a shadow), a ratchet repeated squashes. On EDIT > KIT (EDIT 1) the sound last struck named
 *           over it; on SOUND 1..4 the sound edited lit and named. KIT 2, KIT 3 and X-MOD keep the colony of 0.5
 *           (ui_being.c graph_colony: a being per sound, in two rows).
 *   FM6     (0.9.6, the mock-up's COLONIA) the six operators as bichos wired as the algorithm (FM6_CELL, fm6_routes:
 *           carriers on the output's floor, modulators above), each as large and hot as its LEVEL times its envelope now
 *           (the sounding notes'), its spikes its RATIO, pulses down the wires, FB a curled tail; on EDIT, on OPERATOR /
 *           OPERATOR 2 (the operator picked in the accent, DETUNE a shimmer). OP ENV: the envelope, its bicho riding it.
 * Each takes a rectangle (vx_draw: a whole panel) and scales to it.
 * State: vxs (in the pool: no global of the RAM region moves). MENU > ANIM OFF: they rest. */
#define VX_POOL __attribute__((section(".pool")))
#define VX_CODE __attribute__((noinline))                 /* (each creature its own function: its size on its own) */
#define VX_NE 9                                          /* the creatures: VOICE - DRUM-X ANALOG FM6 - NOISE PHASE LOFI (1 and 5
                                                          * were ANIMAL's and WHEEL's, retired in ACID FM 0.2) */
typedef struct { int32_t cx, cy, s; } vxf_t;            /* a unit (Q4: 16) -> Q4 px: c + u * s / 256 */
static struct {
    uint32_t tm[VX_NE], fr[VX_NE];                       /* each one's time (it runs with its sound), the frame it ran */
    int32_t en[VX_NE];                                   /* its level now (0..256) */
    uint32_t mt[VX_NE], ms[VX_NE];                       /* (0.9.6) the mock-up's time (Q8 frames, 30 a second: with the sound), ms */
    uint32_t nage[VX_NE], nms[VX_NE];                    /* .. the newest note seen (voice_t.age), when it came (fm1_ms) */
    uint32_t fhold;                                      /* OP ENV's hold */
    int16_t dal, frel;                                   /* lines drawn this much fainter (256 - alpha); OP ENV: the release from */
    uint8_t age[8], last;                                /* DRUM-X: each lane's age seen; the lane struck last */
    uint32_t hitm[8], tl[8], kms;                        /* .. its last hit (fm1_ms), its being's time; the ms drawn */
    uint8_t kc;                                          /* .. drawing it (its colours: vx_col) */
} vxs VX_POOL;

/* DRUM-X's beings as the beings (bg_col; until ACID FM 0.3 on JIANT's palette the thermal ramp of the mock-up the kit was
 * chosen from) */
static uint16_t vx_col(int32_t h)
{
    if (h < -1)                                          /* (0.9.6: -2 the theme's: the operator picked, the mock-up's) */
        return T_THEME;
    if (h < 0)                                           /* (a muted sound: dim, as the colony's) */
        return T_DIM;
    return bg_col(clamp(h, 0, 255));
}
static void vx_line(const vxf_t *f, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t h)
{
    int32_t X0 = f->cx + ((x0 * f->s) >> 8), Y0 = f->cy + ((y0 * f->s) >> 8);
    int32_t X1 = f->cx + ((x1 * f->s) >> 8), Y1 = f->cy + ((y1 * f->s) >> 8);
    og_line_a(X0, Y0, X1, Y1, vx_col(h), 256 - vxs.dal);
}
/* a smooth curve through n points (Q4 units, x y pairs): quadratic arcs from midpoint to midpoint, each point their
 * control; closed or open (from the first point to the last). vx_bez: its 1 + 3 e points into o (e: its arcs, returned) */
static uint32_t vx_bez(const int32_t *p, uint32_t n, int closed, int32_t *o)
{
    uint32_t i, k, e = closed ? n : n - 1u;
    for (i = 0; i < e; i++, o += 6) {
        const int32_t *a = &p[2u * (i ? i - 1u : (closed ? n - 1u : 0u))], *b = &p[2u * i], *c = &p[2u * ((i + 1u) % n)];
        int32_t x0 = (a[0] + b[0]) / 2, y0 = (a[1] + b[1]) / 2, x1 = (b[0] + c[0]) / 2, y1 = (b[1] + c[1]) / 2;
        if (!closed && i == 0u) { x0 = b[0]; y0 = b[1]; }
        if (!closed && i + 1u == e) { x1 = c[0]; y1 = c[1]; }
        if (i == 0u) { o[0] = x0; o[1] = y0; }
        for (k = 1; k <= 3u; k++) {
            int32_t t = (int32_t)k * 85, u = 256 - t;   /* (t / 256: thirds) */
            o[2u * k] = k == 3u ? x1 : (u * u * x0 + 2 * u * t * b[0] + t * t * x1) >> 16;
            o[2u * k + 1u] = k == 3u ? y1 : (u * u * y0 + 2 * u * t * b[1] + t * t * y1) >> 16;
        }
    }
    return e;
}
static void vx_curve(const vxf_t *f, const int32_t *p, uint32_t n, int closed, int32_t h)
{
    int32_t o[2 * 49];                                   /* (16 points closed at most) */
    uint32_t i, e = 3u * vx_bez(p, n, closed, o);
    for (i = 0; i < e; i++)
        vx_line(f, o[2u * i], o[2u * i + 1u], o[2u * i + 2u], o[2u * i + 3u], h);
}
/* an ellipse (Q4 units), n sides, its top at angle a0 .. a1 (16-bit turns) */
static void vx_arc(const vxf_t *f, int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint32_t a0, uint32_t a1, uint32_t n,
                   int32_t h)
{
    uint32_t i;
    int32_t px = 0, py = 0;
    for (i = 0; i <= n; i++) {
        uint32_t a = a0 + (uint32_t)(((uint64_t)(a1 - a0) * i) / n);
        int32_t x = cx + ((rx * og_cos(a)) >> 15), y = cy + ((ry * og_sin(a)) >> 15);
        if (i) vx_line(f, px, py, x, y, h);
        px = x;
        py = y;
    }
}
/* the level of track t now (0..256: its part's peak, slewed: a hit at once, a quick fall), k: which drawing; its time
 * runs with it (once a frame) */
static int32_t vx_level(const track_t *t, uint32_t k)
{
    uint32_t dt = ui.frame - vxs.fr[k], dm = fm1_ms - vxs.ms[k];
    int32_t v;
    if (!dt)
        return vxs.en[k];
    if (dt > 8u) dt = 8u;
    vxs.fr[k] = ui.frame;
    vxs.ms[k] = fm1_ms;
    if (dm > 100u) dm = 100u;                            /* (a pause: no jump) */
    if (vxs.en[k])                                       /* (0.9.6) the mock-up's time: 30 frames a second while it sounds */
        vxs.mt[k] += dm * 7680u / 1000u;
    if (ui_prefs & PREF_ANIM_OFF)
        return vxs.en[k] = 0;                            /* (ANIM OFF: at rest) */
    v = ux_trk_peak((uint32_t)(t - trk)) * 256 / 7000;   /* (ACID FM 3a: ux_home.c reads the peaks, once a frame) */
    v = v > 256 ? 256 : v;
    v = v > vxs.en[k] ? v : vxs.en[k] + (v - vxs.en[k] - 2) / 3;
    v = v > 4 ? v : 0;                                   /* (a level that rests a little above 0: silence) */
    vxs.en[k] = v;
    if (v)
        vxs.tm[k] += dt * (uint32_t)(120 + v * 4);
    return v;
}

/* ------------------------------------------------------------------------------------------------ DRUM-X --- */
/* the kit as a colony of small beings (units ~124 x 90: x right, y down, the floor at 40), each drum one with eyes and
 * a mouth: the kick large in the middle, the snare flat on its stand, the toms on little legs, the hi-hat two shells,
 * the crash on a long stem, the cowbell a small head over the kick, the clap two little ones clapping */
enum { VK_KICK, VK_SNARE, VK_CLAP, VK_HATC, VK_HATO, VK_TOM, VK_RIM, VK_BELL };
/* 256 * 2^(-ms / half), Q8 (the squash's fall) */
static int32_t vx_fall(uint32_t ms, uint32_t half)
{
    uint32_t n = ms / half;
    int32_t v;
    if (n > 8u) return 0;
    v = 256 >> n;
    return v - (int32_t)((uint32_t)v * (ms % half) / (2u * half));
}
/* a being's outline (ui_being.c bg_outline's, its lobes 3, the spikes' sharpness 4), squashed: at (x, y), radius r
 * (Q4 units), x stretched by ax / 256, then scaled sx sy / 256 about x and its foot (by: y's place after the squash) */
typedef struct { int32_t x, y, by, ax, sx, sy, sp, len, wob, rip, lob, shp; } vxb_t;   /* (0.9.6: lob lobes, shp 4 or 1) */
static void vx_blob(const vxf_t *f, const vxb_t *b, int32_t r, int outer, uint32_t t, uint32_t nv, int32_t h)
{
    uint32_t i, lb = (uint32_t)b->lob;
    int32_t px = 0, py = 0;
    for (i = 0; i <= nv; i++) {
        uint32_t th = (i % nv) * 65536u / nv;
        int32_t rr = r + ((((r * b->wob) >> 8) * ((9175 * og_sin(lb * th + t) + 3932 * og_sin((lb + 2u) * th - 2u * t)) >> 15)) >> 15), x, y;
        if (outer) {
            int32_t c2 = og_sin(((uint32_t)b->sp * th >> 1) + 0x4000u), p = c2 > 0 ? c2 : 0;
            if (b->shp > 1) {
                p = (p * p) >> 15;
                p = (p * p) >> 15;                       /* (c2^4) */
            }
            rr += (((((r * b->len) >> 8) * p) >> 15) * 26) >> 4;
            rr += ((((r * b->rip) >> 8) * og_sin(th * 18u + 3u * t)) >> 15) / 14;
        }
        x = b->x + (((((rr * og_cos(th)) >> 15) * b->ax) >> 8) * b->sx >> 8);
        y = b->by + ((((rr * og_sin(th)) >> 15) * b->sy) >> 8);
        if (i) vx_line(f, px, py, x, y, h);
        px = x;
        py = y;
    }
}
/* a small dot at (x, y) (Q4 units), radius r (Q4 px: under 1.5 px a 2 x 2 dot) */
static void vx_dot(const vxf_t *f, int32_t x, int32_t y, int32_t r, int32_t h)
{
    uint16_t c = vx_col(h);
    int32_t X = f->cx + ((x * f->s) >> 8), Y = f->cy + ((y * f->s) >> 8);
    if (r >= 32) {
        og_node(X, Y, r >> 4, c);
        return;
    }
    X = (X - 8) >> 4;
    Y = (Y - 8) >> 4;
    for (r = 0; r < 4; r++)
        og_plot(X + (r & 1), Y + (r >> 1), c, 256);
}
/* the bichos' face (DRUM-X's beings, the 0.9.6 creatures): two eyes ex either side of x at ey (shut: short lines,
 * else dots of er Q4 px), the mouth at my: a line mw either side, or open (mo) an ellipse mx wide; their heats */
static void vx_face(const vxf_t *f, int32_t x, int32_t ey, int32_t ex, int32_t er, int shut, int32_t my, int32_t mw,
                    int32_t mo, int32_t mx, int32_t he, int32_t hm, int32_t hl)
{
    int32_t k;
    for (k = -1; k <= 1; k += 2) {
        if (shut)
            vx_line(f, x + k * ex - 22, ey, x + k * ex + 22, ey, he);
        else
            vx_dot(f, x + k * ex, ey, er, he);
    }
    if (mo)
        vx_arc(f, x, my + mo / 2, mx, mo, 0, 65536u, 12, hm);
    else
        vx_line(f, x - mw, my, x + mw, my, hl);
}
/* .. as the mock-up's bface: at (x, y) for a body of radius r (Q4 units), m its shout (0..256), shut its eyes; hot
 * warms the mouth (0..256) */
static void vx_bface(const vxf_t *f, int32_t x, int32_t y, int32_t r, int32_t m, int shut, int32_t hot)
{
    int32_t ey = y - r / 10, mo = clamp(m, 0, 256) * r * 42 / 25600, er = ((r * f->s) >> 8) / 10;
    vx_face(f, x, ey, r * 36 / 100, er, shut, mo > 8 ? ey + r * 36 / 100 : ey + r * 34 / 100, r * 14 / 100,
            mo > 8 ? mo : 0, r * 16 / 100 + mo * 22 / 100, 243, 205 + hot / 5, 154);
}
/* a drum's being at (x, y), rx x ry (Q4 units): e its hit now (0..256: the lane's envelope), age since it (ms), t its
 * time (it runs with the sound), sp its spikes, len their length, legs (Q4 units, 0: none); lit 1 the sound edited, -1
 * muted. Struck it squashes (springing back with the hit's age), shuts its eyes and shouts (the mouth open) */
static void vx_bug(const vxf_t *f, int32_t x, int32_t y, int32_t rx, int32_t ry, int32_t e, uint32_t age, uint32_t t,
                   int32_t sp, int32_t len, int32_t legs, int32_t lit)
{
    vxb_t b;
    int32_t sq = e ? (((e * og_cos(age * 229u)) >> 15) * vx_fall(age, 115u)) >> 8 : 0;   /* (22 rad/s, e^-6t) */
    int32_t k, ey, ex, mo, hb = lit > 0 ? 50 : 0, ho = lit < 0 ? -1 : 41 + e * 77 / 256 + hb;
    int32_t hi = lit < 0 ? -1 : 92 + e * 90 / 256 + hb, he = lit < 0 ? -1 : 243;
    uint32_t nv = (uint32_t)clamp(((rx + ry) * f->s >> 12) * 2, 20, 64);
    if (nv < (uint32_t)sp * 3u) nv = (uint32_t)sp * 3u;
    if (sys_load) nv = nv * (sys_load == LOAD_ECO ? 1u : 2u) / (sys_load == LOAD_ECO ? 2u : 3u);   /* (LITE, ECO) */
    b.x = x;
    b.y = y;
    b.sx = 256 + sq * 90 / 256;
    b.sy = 256 - sq * 102 / 256;
    b.by = y + ry * (256 - b.sy) / 256;
    b.ax = rx * 256 / ry;
    b.sp = sp;
    b.len = len + e * 60 / 256;
    b.wob = 40 + e * 120 / 256;
    b.rip = e * 100 / 256;
    b.lob = 3;
    b.shp = 4;
    if (legs)
        for (k = -1; k <= 1; k += 2) {
            int32_t x1 = x + k * rx * 7 / 10, y1 = y + ry + legs * 6 / 10;
            vx_line(f, x + k * rx / 2, y + ry * 7 / 10, x1, y1, lit < 0 ? -1 : 56 + hb);
            vx_line(f, x1, y1, x + k * rx * 6 / 10, y + ry + legs, lit < 0 ? -1 : 56 + hb);
        }
    vx_blob(f, &b, ry, 1, t, nv, ho);                    /* the membrane (cold), the core */
    vx_blob(f, &b, ry * 66 / 100, 0, t, nv * 2u / 3u, hi);
    ey = b.by - ry * 15 * b.sy / 25600;
    ex = rx * 32 * b.sx / 25600;
    mo = e * ry * 4 / 2560;                              /* the eyes shut on a hard hit; the mouth a shout as loud as it */
    vx_face(f, x, ey, ex, ((ry * 12 / 100 * f->s) >> 12) << 4, e > 90, mo > 6 ? ey + ry * 32 * b.sy / 25600 : ey + ry * 3 / 10,
            ry * 15 / 100, mo > 6 ? mo : 0, ry * 18 / 100 + mo / 5, he, 205, lit < 0 ? -1 : 154);
}
/* the kit at (cx, cy) Q4 px, s: a unit's px (Q8); sel: the sound edited (lit), -1 none */
static void vx_kit(const track_t *t, int32_t cx, int32_t cy, int32_t s, int32_t sel)
{
    vxf_t f = {cx, cy, s};
    drum_lane_t *K = drum_kit_of(t);
    int32_t e[8], lt[8], en = vx_level(t, 2), i, k;
    uint32_t ag[8], now = fm1_ms, dt = now - vxs.kms;
    if (dt > 100u) dt = 100u;                            /* (a pause: no jump) */
    vxs.kms = now;
    vxs.kc = 1;
    for (i = 0; i < 8; i++) {                            /* each lane's hit now: its envelope (velocity, accent, decay) */
        int32_t a = K ? K[i].age : 255;
        lt[i] = dx_lane_muted((uint32_t)i) ? -1 : i == sel;
        e[i] = K && K[i].x.live && lt[i] >= 0 && !(ui_prefs & PREF_ANIM_OFF) ? clamp(K[i].x.ea >> 22, 0, 256) : 0;
        if (K && K[i].dm == 2u) e[i] = e[i] * 2 / 3;   /* (GHOST: a small squash) */
        if (K && a < vxs.age[i] && e[i]) {               /* a new hit (a FLAM's two, a ratchet's each) */
            vxs.hitm[i] = now;
            vxs.last = (uint8_t)i;
        }
        vxs.age[i] = (uint8_t)a;
        ag[i] = now - vxs.hitm[i] < 4000u ? now - vxs.hitm[i] : 4000u;
        if (en || e[i])                                  /* (its time runs with the sound: in silence it rests) */
            vxs.tl[i] += dt * (uint32_t)(60 * 256 + e[i] * 300) * 30u / 256000u;
    }
    vx_line(&f, -60 * 16, 40 * 16, 60 * 16, 40 * 16, 26);   /* the floor */
    /* the crash (BELL) on its stem */
    vx_line(&f, 46 * 16, -24 * 16, 48 * 16, 40 * 16, 41);
    vx_bug(&f, 44 * 16, -30 * 16, 16 * 16, 4 * 16, e[VK_BELL], ag[VK_BELL], vxs.tl[VK_BELL] + 3000u, 9, 40, 0, lt[VK_BELL]);
    /* the hi-hat: two shells on their stand, the top one lifting (OH: its mouth open), together on CH */
    {
        int32_t op = e[VK_HATO] > 5 ? 48 + e[VK_HATO] * 80 / 256 : e[VK_HATC] > 5 ? 0 : 24;
        vx_line(&f, -52 * 16, -2 * 16, -52 * 16, 40 * 16, 41);
        vx_bug(&f, -52 * 16, 0, 12 * 16, 54, e[VK_HATC] * 3 / 5, ag[VK_HATC], vxs.tl[VK_HATC], 12, 10, 0, lt[VK_HATC]);
        vx_bug(&f, -52 * 16, -5 * 16 - op, 12 * 16, 54, e[VK_HATC] > e[VK_HATO] ? e[VK_HATC] : e[VK_HATO],
               ag[VK_HATC] < ag[VK_HATO] ? ag[VK_HATC] : ag[VK_HATO], vxs.tl[VK_HATO] + 900u, 12, 10, 0,
               lt[VK_HATO] < 0 && lt[VK_HATC] < 0 ? -1 : lt[VK_HATO] > 0 || lt[VK_HATC] > 0);
    }
    /* the toms (TOM): two on little legs over the kick, the floor tom on longer ones */
    vx_bug(&f, -15 * 16, -14 * 16, 9 * 16, 7 * 16, e[VK_TOM], ag[VK_TOM], vxs.tl[VK_TOM] + 200u, 5, 20, 6 * 16, lt[VK_TOM]);
    vx_bug(&f, 15 * 16, -14 * 16, 9 * 16, 7 * 16, e[VK_TOM] * 3 / 4, ag[VK_TOM] + 30u, vxs.tl[VK_TOM] + 400u, 5, 20, 6 * 16, lt[VK_TOM]);
    vx_bug(&f, 40 * 16, 18 * 16, 11 * 16, 10 * 16, e[VK_TOM] / 2, ag[VK_TOM] + 60u, vxs.tl[VK_TOM] + 600u, 5, 20, 12 * 16, lt[VK_TOM]);
    /* the kick, large in the middle; the snare flat on its stand */
    vx_bug(&f, 0, 16 * 16, 22 * 16, 20 * 16, e[VK_KICK], ag[VK_KICK], vxs.tl[VK_KICK] + 1000u, 4, 14, 4 * 16, lt[VK_KICK]);
    vx_bug(&f, -34 * 16, 10 * 16, 13 * 16, 6 * 16, e[VK_SNARE], ag[VK_SNARE], vxs.tl[VK_SNARE] + 1400u, 14, 12, 22 * 16, lt[VK_SNARE]);
    /* the rim (RS): a spark on the snare's edge, its rays turning as it rings */
    if (e[VK_RIM] > 5 || lt[VK_RIM] > 0)
        for (k = 0; k < 6; k++) {
            uint32_t a = (uint32_t)k * 10923u + ag[VK_RIM] * 63u;
            int32_t r1 = 3 * 16 + e[VK_RIM] * 4 * 16 / 256;
            vx_line(&f, -47 * 16 + ((32 * og_cos(a)) >> 15), 8 * 16 + ((32 * og_sin(a)) >> 15), -47 * 16 + ((r1 * og_cos(a)) >> 15),
                    8 * 16 + ((r1 * og_sin(a)) >> 15), 154 + e[VK_RIM] * 102 / 256);
        }
    og_node(cx + ((-47 * 16 * s) >> 8), cy + ((8 * 16 * s) >> 8), 1, vx_col(lt[VK_RIM] < 0 ? -1 : 102 + e[VK_RIM] * 154 / 256));
    /* the cowbell (BELL): a small head over the kick */
    vx_bug(&f, 0, -30 * 16, 6 * 16, 5 * 16, e[VK_BELL], ag[VK_BELL], vxs.tl[VK_BELL] + 1800u, 3, 30, 0, lt[VK_BELL]);
    /* the clap: two little ones, apart, clapping together */
    {
        int32_t g = 8 * 16 - e[VK_CLAP] * 6 * 16 / 256;
        vx_bug(&f, -27 * 16 - g, -33 * 16, 4 * 16, 4 * 16, e[VK_CLAP], ag[VK_CLAP], vxs.tl[VK_CLAP] + 2200u, 5, 20, 0, lt[VK_CLAP]);
        vx_bug(&f, -27 * 16 + g, -33 * 16, 4 * 16, 4 * 16, e[VK_CLAP], ag[VK_CLAP], vxs.tl[VK_CLAP] + 2600u, 5, 20, 0, lt[VK_CLAP]);
    }
    /* a FLAM's hit to come: the being's shadow, doubled */
    for (i = 1; i < 8; i++)
        if (K && K[i].fl) {
            static const int8_t PX[8] = {0, -34, -27, -52, -52, -15, -47, 44}, PY[8] = {16, 10, -33, 0, -8, -14, 8, -30};
            vx_arc(&f, PX[i] * 16 + 2 * 16, PY[i] * 16 + 16, 6 * 16, 3 * 16, 0, 65536u, 12, 64);
        }
    vxs.kc = 0;
}

/* ============================================================================ (0.9.6) the mock-up's family === */
/* the mock-up's frame (uFrame): its unit space 124 x 94 fitted in the rectangle A (x y w h px), k / 100 larger, lifted
 * dy (Q4 units) */
static vxf_t vx_uf(const int32_t *A, int32_t k, int32_t dy)
{
    vxf_t f;
    int32_t a = A[2] * 256 / 124, b = A[3] * 256 / 94;
    f.s = (a < b ? a : b) * k / 100;
    f.cx = (A[0] * 2 + A[2]) * 8;
    f.cy = (A[1] * 2 + A[3]) * 8 - ((dy * f.s) >> 8);
    return f;
}
/* the angle (16-bit turns) of t * r radians, t the mock-up's frames (mt, Q8), r = r1000 / 1000 */
static uint32_t vx_ang(uint32_t mt, int32_t r1000)
{
    return mt * (uint32_t)r1000 * 2670u >> 16;          /* (mod 2^32: an angle's 16 bits exact) */
}
/* the squash of a hit (Q8): e (0..256) at age ms (22 rad/s, e^-6t) */
static int32_t vx_sq(int32_t e, uint32_t age)
{
    return (((e * og_cos(age * 229u)) >> 15) * vx_fall(age, 115u)) >> 8;
}
/* ms since track t's newest note (k: which creature saw it; 4000 at most); *note: that note */
static uint32_t vx_age(const track_t *t, uint32_t k, int32_t *note)
{
    uint32_t i, a = 0, n = 60;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].age > a) { a = t->v[i].age; n = t->v[i].note; }
    if (a > vxs.nage[k])
        vxs.nms[k] = fm1_ms;
    vxs.nage[k] = a;
    if (note) *note = (int32_t)n;
    return fm1_ms - vxs.nms[k] < 4000u ? fm1_ms - vxs.nms[k] : 4000u;
}
/* a polyline of n points (Q4 units: x y pairs), closed or not */
/* a line between two points in polar form about c (cx cy, x y scales / 256): angles (16-bit turns), radii (Q4 units) */
/* a ring (Q4 units), its sides as many as its size asks */
static void vx_ring(const vxf_t *f, int32_t x, int32_t y, int32_t rx, int32_t ry, int32_t h)
{
    uint32_t n = (uint32_t)clamp(((rx + ry) * f->s) >> 12, 8, 28);
    vx_arc(f, x, y, rx, ry, 0, 65536u, n, h);
}
/* a bicho's body (isoU): its outer outline (the spikes) of radius r, its inner one ri (0: none); g its features (spikes,
 * their length, wobble, ripple, lobes, sharpness, x stretch) and its place (x, by, squashed sx sy about it) */
static void vx_body(const vxf_t *f, const vxb_t *g, int32_t r, int32_t ri, uint32_t t, int32_t ho, int32_t hi, uint32_t no, uint32_t ni)
{
    if (sys_load) {                                      /* (LITE, ECO) */
        no = no * (sys_load == LOAD_ECO ? 1u : 2u) / (sys_load == LOAD_ECO ? 2u : 3u);
        ni = ni * (sys_load == LOAD_ECO ? 1u : 2u) / (sys_load == LOAD_ECO ? 2u : 3u);
    }
    vx_blob(f, g, r, 1, t, no, ho);
    if (ri) vx_blob(f, g, ri, 0, t, ni, hi);
}
/* text s centred on (x, its capitals' top y) Q4 px */
static void vx_text(int32_t x, int32_t y, const char *s, uint16_t c)
{
    cv_text((x >> 4) - text_w(&AF_X, s) / 2, (y >> 4) - AF_X_CAP_Y, &AF_X, s, c);
}

/* ---------------------------------------------------------------------------------------------- FM6 --- */
#ifndef VX_FM6_HOOK
#define VX_FM6_HOOK(k, x, y, r) ((void)0)               /* ui_render.c (host tests): each bicho as drawn (its lint) */
#endif
static const uint8_t FM6_CELL[32][6];                    /* (ui_graph.c) */
static void fm6_routes(uint32_t a, uint8_t *m);
static uint32_t fm6_fb_op(uint32_t a);
static uint32_t fm6_car_ops(uint32_t a);
static uint32_t fm6_alg_of(const track_t *t);
static void fop_freq(const uint8_t *op, char *s);
typedef struct {
    const uint8_t *p;                                    /* the patch as it plays (the macros on it) */
    int32_t env[6], amp[6], rat[6], rad[6], x[6], y[6];  /* per operator 1..6: its envelope now, level x envelope (0..256),
                                                          * ratio (Q8), radius, place (Q4 units) */
    uint32_t alg, car, fbo, st[6];                       /* the algorithm, its carriers, the feedback operator; each stage */
    uint8_t m[6];
} vxfm_t;
/* the operators of track t now: the sounding notes' envelopes (the loudest), each one's level, ratio, size, place */
static void vx_fm_ops(const track_t *t, vxfm_t *o)
{
    uint32_t tr = (uint32_t)(t - trk) % NTRK, k, v, hi = 0, top = 0;
    o->p = fm6_eff[tr].ok ? fm6_eff[tr].p : fm6_patch[tr];
    o->alg = fm6_alg_of(t);
    o->car = fm6_car_ops(o->alg);
    o->fbo = fm6_fb_op(o->alg);
    fm6_routes(o->alg, o->m);
    for (k = 0; k < 6u; k++) {
        const uint8_t *op = o->p + (5u - k) * FP_OP;
        int32_t e = 0, ol = op[FP_OL] % 100u, a = ol * 256 / 99, c = op[FP_FC] & 31;
        o->st[k] = 4;
        for (v = 0; v < FM6_POLY; v++) {                 /* (msfa's level: scaleout units 0..127, the stage's L 0..99) */
            const fm6_env_t *n = &fm6_note[tr][v].env[5u - k];
            int32_t s = (((n->level >> 16) - n->outlevel + 4256) / 32 - 28) * 256 / 99;
            if (!fm6_note[tr][v].live || !t->v[v].active) continue;
            if (s > e || o->st[k] == 4u) { o->st[k] = n->ix | (n->down ? 0u : 8u); }
            e = s > e ? s : e;
        }
        o->env[k] = clamp(e, 0, 256);
        o->amp[k] = ol ? (a * ((a * 154 >> 8) + 102) / 256) * o->env[k] / 256 : 0;   /* ((ol / 99) ^ 1.6) x env */
        o->rat[k] = op[FP_MODE] ? 256 : (c ? c * 256 : 128) * (100 + op[FP_FF] % 100u) / 100;
        o->rad[k] = 120 * (141 + 141 * ol / 99) / 256 * (200 + (o->env[k] * 82 >> 8)) / 256;
        hi = (FM6_CELL[o->alg][k] & 7u) > hi ? FM6_CELL[o->alg][k] & 7u : hi;
        top = (uint32_t)(FM6_CELL[o->alg][k] >> 4) > top ? (uint32_t)FM6_CELL[o->alg][k] >> 4 : top;
    }
    for (k = 0; k < 6u; k++) {                           /* (fmPos: columns 26 apart, the carriers on the floor 38) */
        o->x[k] = ((int32_t)(FM6_CELL[o->alg][k] & 7u) * 2 - (int32_t)hi) * 13 * 16;
        o->y[k] = 28 * 16 - (int32_t)(FM6_CELL[o->alg][k] >> 4) * (top >= 3u ? 324 : 352);
    }
}
/* operator k's bicho at (x, y): spikes its ratio, size and heat its level now, breathing with its envelope; sel: the
 * operator picked (the accent); sh: DETUNE's shimmer (Q4 units) */
static VX_CODE void vx_fm_bicho(const vxf_t *f, const vxfm_t *o, uint32_t k, int32_t x, int32_t y, int32_t r, uint32_t age, uint32_t mt,
                        int sel, int32_t sh)
{
    int32_t a = o->amp[k], car = (int32_t)(o->car >> k & 1u), sp = clamp((o->rat[k] + 128) >> 8, 1, 9), d;
    int32_t sq = vx_sq(car ? a : a * 3 / 5, age), sx = 256 + (sq * 77 >> 8), sy = 256 - (sq * 82 >> 8), by = y + r * (256 - sy) / 256;
    uint32_t tt = (mt * (uint32_t)(60 + (a * 260 >> 8)) >> 8) + k * 3000u;
    int32_t oc = sel ? -2 : 41 + (a * 102 >> 8);
    vxb_t g = {x, y, by, 256, sx, sy, sp, 12 + (a * 50 >> 8), 40 + (a * 110 >> 8), (a * 80 >> 8), 3, 4};
    if (!(o->p[(5u - k) * FP_OP + FP_OL] % 100u)) {      /* (silent: a cold outline) */
        g.by = y; g.sx = g.sy = 256;
        vx_body(f, &g, r, 0, tt, 20, 0, 40, 0);
        return;
    }
    r = r * (256 + ((og_sin(vx_ang(mt, 220 * (o->rat[k] < 12 * 256 ? o->rat[k] : 12 * 256) / 256)) * 15 >> 15) * a >> 8)) / 256;
    if (sh) {                                            /* (DETUNE: a shimmer, two faint copies) */
        vxs.dal = 102;
        for (d = -1; d <= 1; d += 2) {
            g.x = x + d * sh;
            vx_body(f, &g, r, 0, tt, 51 + (a * 77 >> 8), 0, 48, 0);
        }
        vxs.dal = 0;
        g.x = x;
    }
    vx_body(f, &g, r, (r * 159 >> 8), tt, oc, sel ? -2 : 87 + (a * 128 >> 8), 48, 20);
    vx_bface(f, x, by, r, car ? a * 8 / 5 : a * 3 / 5, age < 90u, 0);
}
/* the operator's number by its bicho */
static void vx_fm_num(const vxf_t *f, const vxfm_t *o, uint32_t k, int32_t x, int32_t y, int sel)
{
    char n[2] = {(char)('1' + k), 0};
    vx_text(f->cx + ((x * f->s) >> 8), f->cy + ((y * f->s) >> 8) - 64, n,
            sel ? T_THEME : o->p[(5u - k) * FP_OP + FP_OL] % 100u ? T_MID : T_DIM);
}
/* COLONIA: the operators wired as the algorithm, the carriers standing on the output; sel the operator picked (or 6),
 * op2: OPERATOR 2 (its DETUNE a shimmer) */
static VX_CODE void vx_fm6(const track_t *t, const int32_t *A, uint32_t sel, int op2)
{
    vxfm_t o;
    vxf_t f = vx_uf(A, 100, 0);
    int32_t en = vx_level(t, 4), x0 = 9999, x1 = -9999, y0 = 9999, s, l = 9999, r = -9999, k, j, fb;
    uint32_t mt = vxs.mt[4], age = vx_age(t, 4, 0);
    vxs.kc = 1;
    vx_fm_ops(t, &o);
    for (k = 0; k < 6; k++) {
        x0 = o.x[k] < x0 ? o.x[k] : x0; x1 = o.x[k] > x1 ? o.x[k] : x1; y0 = o.y[k] < y0 ? o.y[k] : y0;
        if (o.car >> k & 1u) { l = o.x[k] < l ? o.x[k] : l; r = o.x[k] > r ? o.x[k] : r; }
    }
    x0 -= 16 * 16; x1 += 18 * 16; y0 -= 13 * 16;         /* (fitTo: the colony fills the frame, at most 1.75 x) */
    s = f.s * 124 * 16 / (x1 - x0);
    j = f.s * 94 * 16 / (47 * 16 - y0);
    s = s < j ? s : j;
    s = s < f.s * 7 / 4 ? s : f.s * 7 / 4;
    f.cx -= ((x0 + x1) / 2 * s) >> 8;
    f.cy -= ((y0 + 47 * 16) / 2 * s) >> 8;
    f.s = s;
    l = (l < -160 ? l : -160) - 128;                    /* the output: the floor, an arrow out */
    r = (r > 160 ? r : 160) + 192;
    vx_line(&f, l, 38 * 16, r, 38 * 16, 36 + en / 2);
    vx_line(&f, r - 48, 36 * 16, r, 38 * 16, 77 + en / 2);
    vx_line(&f, r - 48, 40 * 16, r, 38 * 16, 77 + en / 2);
    for (k = 0; k < 6; k++)                              /* the wires: down, across, down; pulses as fast as the ratio */
        for (j = 0; j < 6; j++)
            if (o.m[k] >> j & 1u) {
                int32_t a = o.amp[j], ya = o.y[j] + o.rad[j], yb = o.y[k] - o.rad[k], my = (ya + yb) / 2, q[8];
                q[0] = o.x[j]; q[1] = ya; q[2] = o.x[j]; q[3] = my; q[4] = o.x[k]; q[5] = my; q[6] = o.x[k]; q[7] = yb;
                vx_curve(&f, q, 4, 0, 31 + (a * 154 >> 8));
                if (a > 8) {                             /* (two pulses along it, as fast as the modulator's ratio) */
                    int32_t n, dx = o.x[k] > o.x[j] ? o.x[k] - o.x[j] : o.x[j] - o.x[k], len = (my - ya) + dx + (yb - my);
                    int32_t rr = f.s * 7 >> 8, mr = o.rat[j] < 14 * 256 ? o.rat[j] : 14 * 256;
                    for (n = 0; n < 2; n++) {
                        int32_t d = (int32_t)(((mt * (uint32_t)(256 + mr / 4) * 786u >> 16) + (uint32_t)n * 128u) & 255u) * len / 256;
                        if (d < my - ya) vx_dot(&f, o.x[j], ya + d, rr, 154 + (a * 102 >> 8));
                        else if ((d -= my - ya) < dx) vx_dot(&f, o.x[j] + (o.x[k] > o.x[j] ? d : -d), my, rr, 154 + (a * 102 >> 8));
                        else vx_dot(&f, o.x[k], my + d - dx, rr, 154 + (a * 102 >> 8));
                    }
                }
            }
    for (k = 0; k < 6; k++)                              /* the carriers' legs, on the floor */
        if (o.car >> k & 1u)
            for (j = -1; j <= 1; j += 2)
                vx_line(&f, o.x[k] + (j * o.rad[k] * 115 >> 8), o.y[k] + (o.rad[k] * 179 >> 8), o.x[k] + (j * o.rad[k] * 154 >> 8), 38 * 16,
                        51 + (o.amp[k] * 77 >> 8));
    fb = clamp(o.p[FP_FB] + t->p[P_E1], 0, 7);           /* FB: a curled tail */
    {
        int32_t X = o.x[o.fbo], Y = o.y[o.fbo], rd = o.rad[o.fbo], q[10];
        q[0] = X + (rd * 205 >> 8); q[1] = Y - (rd * 102 >> 8);
        q[2] = X + rd + 48 + fb * 48 / 7; q[3] = Y - rd - 32 - fb * 32 / 7;
        q[4] = X + rd + 96 + fb * 64 / 7; q[5] = Y + ((og_sin(vx_ang(mt, 200)) * fb * 32 / 7) >> 15);
        q[6] = X + rd + 32; q[7] = Y + (rd * 179 >> 8);
        q[8] = X + (rd * 179 >> 8); q[9] = Y + rd / 4;
        vx_curve(&f, q, 5, 0, fb ? 64 + fb * 154 / 7 : 20);
        if (fb) vx_dot(&f, q[4], q[5], f.s >> 5, 154 + fb * 102 / 7);
    }
    for (k = 0; k < 6; k++) {
        const uint8_t *op = o.p + (5u - (uint32_t)k) * FP_OP;
        int32_t sh = op2 && (uint32_t)k == sel ? (op[FP_DET] > 7u ? op[FP_DET] - 7 : 7 - op[FP_DET]) * 8 : 0;
        vx_fm_bicho(&f, &o, (uint32_t)k, o.x[k], o.y[k], o.rad[k], age, mt, (uint32_t)k == sel, sh);
        vx_fm_num(&f, &o, (uint32_t)k, o.x[k] - o.rad[k] - 64, o.y[k] - o.rad[k] + 16, (uint32_t)k == sel);
        VX_FM6_HOOK(k, f.cx + ((o.x[k] * f.s) >> 8), f.cy + ((o.y[k] * f.s) >> 8), (145 * f.s) >> 8);   /* (the largest it gets) */
    }
    vxs.kc = 0;
}
/* OPERATOR / OPERATOR 2: the colony on the whole panel, the operator picked lit; "ALG 05" over it on the left, its
 * frequency on the right ("OP3 x1.50") */
static VX_CODE void vx_fm6_page(const track_t *t, uint32_t sel, int op2)
{
    static const int32_t A[4] = {4, 14, 232, 124};
    const uint8_t *pt = fm6_patch[(t - trk) % NTRK];
    uint32_t alg = fm6_alg_of(t);
    uint32_t hot = ui.hot_t ? (uint32_t)cur_page()->id[ui.hot_col & 3u] : 0u;
    char b[16] = {'A', 'L', 'G', ' ', (char)('0' + (alg + 1u) / 10u), (char)('0' + (alg + 1u) % 10u), 0};
    char fq[16] = {'O', 'P', (char)('1' + sel), ' ', 0};
    cv_oy = 0;
    vx_fm6(t, A, sel, op2);
    if (!t->p[P_E0])
        str_cpy(b + 6, " PAT", 5);
    cv_text(10, 4, &AF_S, b, hot == P_E0 || hot == P_E7 ? T_ACCENT : T_MID);
    fop_freq(pt + (5u - sel) * FP_OP, fq + 4);
    cv_text_r(228, 4, &AF_S, fq, T_TEXT, T_SURF);
}
/* OP ENV: the picked operator's envelope as the patch has it (from L4 to L1 at R1, L2 at R2, L3 at R3, held while the key
 * is down: a dashed line where it is let go, back to L4 at R4; a stage as wide as its rate is slow), the stage KNOB 3 / 4
 * edit in the accent, its bicho riding the curve where the operator is now; "OP3" top left, the stage's R and L right */
static VX_CODE void vx_fm6env(const track_t *t)
{
    static const char *const STG[4] = {"ATK", "DEC", "SUS", "REL"};
    static const int32_t A[4] = {4, 12, 232, 126};
    const uint8_t *op = fop_bytes((uint32_t)(t - trk));
    vxfm_t o;
    vxf_t f = vx_uf(A, 100, 0);
    int32_t x[6], y[6], k, i, en = vx_level(t, 4), px, py, e, a, r, u, sg = fop_stg & 3u;
    uint32_t mt = vxs.mt[4], age = vx_age(t, 4, 0), ko = fop_op % 6u, st;
    char b[24] = {'O', 'P', (char)('1' + ko), 0};
    (void)en;
    cv_oy = 0;
    vxs.kc = 1;
    vx_fm_ops(t, &o);
#define VFY(l) (22 * 16 - (int32_t)(l) * 42 * 16 / 99)
#define VFW(r) ((4 + (99 - (int32_t)(r)) * (99 - (int32_t)(r)) * 44 / 9801) * 112 * 16 / 216)
    x[0] = -56 * 16;
    y[0] = VFY(op[FP_L1 + 3]);
    for (k = 0; k < 3; k++) {
        x[k + 1] = x[k] + VFW(op[FP_R1 + k]);
        y[k + 1] = VFY(op[FP_L1 + k]);
    }
    x[5] = 56 * 16;
    x[4] = x[5] - VFW(op[FP_R1 + 3]);
    y[4] = y[3];
    y[5] = y[0];
    vx_line(&f, -60 * 16, 26 * 16, 60 * 16, 26 * 16, 26);
    for (i = -20 * 16; i < 22 * 16; i += 48)             /* the key let go */
        vx_line(&f, x[4], i, x[4], i + 22, 31);
    for (k = 0; k < 5; k++)                              /* (3: the hold, no stage of its own) */
        for (i = 0; i < 10; i++) {
            int32_t on = k != 3 && (k < 3 ? k : 3) == sg, dx = x[k + 1] - x[k], dy = y[k + 1] - y[k];
            int32_t ya = y[k] + dy * i / 10, d;
            for (d = 0; d <= on * 12; d += 12)
                vx_line(&f, x[k] + dx * i / 10, ya + d, x[k] + dx * (i + 1) / 10, y[k] + dy * (i + 1) / 10 + d,
                        on ? -2 : 38 + ((22 * 16 - ya) * 154 / (42 * 16)));
        }
    k = sg < 3 ? sg + 1 : 5;
    vx_ring(&f, x[k], y[k], 32, 32, -2);
    vx_ring(&f, x[k], y[k], 40, 40, -2);
    e = o.env[ko];                                       /* where the operator is now: on its stage, as far as its level */
    st = o.st[ko];                                       /* came (held: along the hold, 1.5 s across) */
    py = VFY(e * 99 / 256);
    k = (st & 7u) >= 4u ? 5 : st == 3u ? 3 : st < 3u ? (int32_t)st : 4;
    if (k == 3) {
        if (!vxs.fhold) vxs.fhold = fm1_ms | 1u;
        u = clamp((int32_t)(fm1_ms - vxs.fhold) * 256 / 1500, 0, 256);
    } else {
        int32_t l0 = k == 4 ? vxs.frel * 99 / 256 : op[FP_L1 + ((k + 3) & 3)], l1 = op[FP_L1 + (k < 4 ? k : 3)];
        vxs.fhold = 0;
        u = l1 != l0 ? clamp((e * 99 / 256 - l0) * 256 / (l1 - l0), 0, 256) : 256;
    }
    if (k == 5) { k = 4; u = 256; }
    if (k != 4) vxs.frel = (int16_t)e;                   /* (the level its release sets out from) */
    px = x[k] + (x[k + 1] - x[k]) * u / 256;
    a = o.amp[ko];
    r = 64 + (96 * e >> 8);
    {
        int32_t sp = clamp((o.rat[ko] + 128) >> 8, 1, 9);
        uint32_t tt = mt * 90u >> 8;
        vxb_t g = {px, 0, py - r, 256, 256, 256, sp, 12 + (a * 50 >> 8), 40 + (a * 110 >> 8), (a * 80 >> 8), 3, 4};
        vx_body(&f, &g, r, (r * 159 >> 8), tt, -2, 90 + a / 2, 48, 28);
        vx_bface(&f, px, py - r, r, a * 7 / 5, age < 90u, 0);
    }
    k = sg < 3 ? (x[sg] + x[sg + 1]) / 2 : (x[4] + x[5]) / 2;
    k = clamp(k, -49 * 16, 49 * 16);                     /* (the stage's name under it, inside the curve's ends) */
    vx_text(f.cx + ((k * f.s) >> 8),
            f.cy + ((31 * 16 * f.s) >> 8) - 112, STG[sg], T_THEME);
#undef VFY
#undef VFW
    vxs.kc = 0;
    cv_text(10, 4, &AF_S, b, T_MID);
    str_cpy(b, STG[sg], sizeof b);
    str_cpy(b + str_len(b), "  R ", 5);
    fmt_int(b + str_len(b), op[FP_R1 + sg]);
    str_cpy(b + str_len(b), "  L ", 5);
    fmt_int(b + str_len(b), op[FP_L1 + sg]);
    cv_text_r(228, 4, &AF_S, b, T_TEXT, T_SURF);
}

/* ------------------------------------------------------------------------------------------------ the glue --- */
/* the engine's own creature in the rectangle x y w h (px: EDIT's left panel 8 14 108 112, HOME's middle 146 x 112, or a
 * whole panel: each fits itself to it); 0 when the engine has none (its being then). (ACID FM 3a: the level HOME's
 * BEING passed in, ui_lzhome.c's, gone with it) */
static int32_t vx_fit(int32_t w, int32_t h, int32_t uw, int32_t uh)   /* a unit's px (Q8): uw x uh units in w x h px */
{
    return w * 256 / uw < h * 256 / uh ? w * 256 / uw : h * 256 / uh;
}
static int vx_draw(const track_t *t, int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t A[4] = {x, y, w, h}, cx = x * 16 + w * 8, cy = y * 16 + h * 8, s;
    if (trk_eng(t) == ENGI_DRUM) {                       /* (the kit ~128 x 92) */
        s = vx_fit(w, h, 128, 92);
        vx_kit(t, cx - (s >> 4), cy + (48 * s >> 8), s, -1);
    } else if (trk_eng(t) == ENGI_FM6)
        vx_fm6(t, A, 6u, 0);
    else
        return 0;                                        /* (ACID FM 3b: the synths' creatures gone; their EDIT pages draw
                                                          * their wave, ux_section.c) */
    return 1;
}
/* EDIT > KIT (lane -1) and SOUND 1..4 (a DRUM track): the kit large on the panel, KIT over it on the left; on the right
 * the sound last struck (KIT) or the sound edited, lit (SOUND: lane) */
static void vx_kit_page(const track_t *t, int32_t lane)
{
    char b[16];
    uint32_t l;
    vx_kit(t, 120 * 16, 73 * 16, 368, lane);
    l = lane < 0 ? vxs.last & 7u : (uint32_t)lane & 7u;
    str_cpy(b, drum_lane_abbr(t, l), sizeof b);
    str_cpy(b + str_len(b), " \xB7 ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), drum_lane_name(t, l), sizeof b - str_len(b));
    cv_text(8, 6 - AF_X_CAP_Y, &AF_X, "KIT", T_THEME);
    cv_text_r(232, 6 - AF_X_CAP_Y, &AF_X, b, lane < 0 ? T_TEXT : T_THEME, T_SURF);
}
