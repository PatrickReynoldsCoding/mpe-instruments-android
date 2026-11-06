/*
 * The Grid as a MESH controller: square keys, held upright, the notes climbing row by row from
 * the bottom right (1.1, 1.2, 1.3, then 2.1...; from the left, flipped across), so three fingers
 * sit on three neighbouring notes. Each square a note of a scale, or every semitone with the
 * notes outside the scale darkened. Hold a square and move it: across bends toward the note you
 * drag to, up and down changes the tone (each can be set to loudness, tone, bend or volume); a
 * quick wiggle is vibrato. A square is lit while its note sounds, and fades after.
 *
 * Square k counts along the rows from the bottom, endlessly: the rows run on past the top and
 * bottom of the screen, and the scroll bar (a thin strip the height of the screen, with each
 * root's octave marked) slides them up and down under the finger, as the touch pad's note strip
 * does. At full size the squares fill the width, edge to edge. Either axis can be flipped.
 * Controls can sit in strips either side: Latch, Hold, Bend, Tone. The colours can mark notes
 * by their place in the key (the 4th always blue, say).
 *
 * A port of odo's built-in Grid (GridUi.kt, KeyGrid.kt), one section (no split).
 */
#include "mesh.h"

#define MAX_FINGERS 10
#define MAX_VOICES 15          /* the MIDI channels MPE has for notes */
#define MAX_RING 32
#define MAX_SIDE 10

/* ---- settings (the manifest's, in order) ---------------------------------------------------- */
enum {
    S_COLS, S_SIZE, S_SPACING, S_FLIP_X, S_FLIP_Y, S_CHROMATIC, S_SCALE, S_KEY, S_OCTAVE,
    S_ACROSS, S_UPDOWN, S_BEND_RANGE, S_BEND_SNAP, S_VIBRATO, S_LABELS, S_THEME, S_COLOURS, S_LATCH, S_LATCHMODE, S_BAR,
    S_SIDE0, S_TURN = S_SIDE0 + 4, S_COUNT
};
static double set_[S_COUNT] = {3, 100, 9, 0, 0, 0, 0, 0, 0, 3, 2, 0, 0, 2, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0};
/* Bend range: 0 toward the next note; else semitones at a whole square's slide (as the gyro's). */
static const float BEND_RANGES[7] = {0, 1, 2, 3, 5, 7, 12};
enum { TURN_AUTO, TURN_UPRIGHT, TURN_SIDEWAYS };

enum { T_NONE, T_LOUDNESS, T_TONE, T_BEND, T_VOLUME };
enum { LATCH_CHORD, LATCH_TAP };
enum { C_LATCH, C_HOLD, C_BEND, C_TONE, C_COUNT };   /* the side controls */
enum { COL_PLAIN, COL_DEGREES, COL_CHORD, COL_PILLARS, COL_NAMES };
enum { SIDE_OFF, SIDE_LEFT, SIDE_RIGHT };

/* SCALES[1..] of Perf.kt: bit d set when the degree d semitones above the key is in the scale. */
static const int MASKS[9] = {0xAB5, 0x5AD, 0x6AD, 0x6B5, 0x9AD, 0x295, 0x4A9, 0x4E9, 0xFFF};
static const float VIBRATO_DEPTH[4] = {0, 0.25f, 0.5f, 1};

/* ---- small maths ---------------------------------------------------------------------------- */
static float floorf_(float x) { return __builtin_floorf(x); }
static int roundi(float x) { return (int)floorf_(x + 0.5f); }
static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static float absf(float v) { return v < 0 ? -v : v; }
static int floordiv(int a, int b) { int q = a / b; return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q; }
static int floormod(int a, int b) { return a - b * floordiv(a, b); }
static float expf_(float x) {
    if (x < -80) return 0;
    if (x > 80) x = 80;
    float n = floorf_(x * 1.44269504f + 0.5f);
    float r = x - n * 0.69314718f;
    float p = 1 + r * (1 + r * (0.5f + r * (1.0f / 6 + r * (1.0f / 24 + r * (1.0f / 120)))));
    union { float f; unsigned u; } s;
    s.u = (unsigned)((int)n + 127) << 23;
    return p * s.f;
}

/* ---- the looks: GRID_THEMES ------------------------------------------------------------------- */
typedef struct { unsigned background, square, light, hot, on_light; int bright; } Theme;
static const Theme THEMES[6] = {
    {0x000000FFu, 0x0B1C18FFu, 0x23FFD0FFu, 0xDCFFF8FFu, 0x002A22FFu, 0},   /* Neon */
    {0x0D0705FFu, 0x27170FFFu, 0xFF8A4CFFu, 0xFFE2CCFFu, 0x3A1200FFu, 0},   /* Ember */
    {0x04070DFFu, 0x111A2AFFu, 0x7FC4FFFFu, 0xE6F3FFFFu, 0x02203AFFu, 0},   /* Ice */
    {0x08040FFFu, 0x1C1030FFu, 0xC58BFFFFu, 0xF3E6FFFFu, 0x240A40FFu, 0},   /* Violet */
    {0x0E0610FFu, 0x2A1226FFu, 0xFF5FA2FFu, 0xFFE0EEFFu, 0x3A0020FFu, 0},   /* Sunset */
    {0xEDE9E1FFu, 0xFFFFFFFFu, 0x0E7C66FFu, 0x6FE3C6FFu, 0xFFFFFFFFu, 1},   /* Paper */
};
static const unsigned WHITE = 0xFFFFFFFFu, BLACK = 0x000000FFu;

/* Colours for the notes of the key, by their interval above it: 1 red, 2 orange, 3 yellow,
   4 blue, 5 green, 6 indigo, 7 pink; the flattened 5th a slate between 4 and 5. */
static const unsigned DEGREE[12] = {
    0xFF5C5CFFu, 0xFF9F43FFu, 0xFF9F43FFu, 0xFFD43BFFu, 0xFFD43BFFu, 0x4DABF7FFu,
    0x8CA0B8FFu, 0x51CF66FFu, 0x9775FAFFu, 0x9775FAFFu, 0xF06595FFu, 0xF06595FFu};
/* By note name, whatever the key: round the circle of fifths, C red. */
static const unsigned NAMED[12] = {
    0xFF5C5CFFu, 0x9775FAFFu, 0xFFA94DFFu, 0xF06595FFu, 0xFFE066FFu, 0xE599F7FFu,
    0x69DB7CFFu, 0xFF8787FFu, 0x38D9A9FFu, 0xFFC078FFu, 0x4DABF7FFu, 0xC0EB75FFu};
static const Theme* theme(void) { int k = (int)set_[S_THEME]; return &THEMES[k < 0 || k > 5 ? 0 : k]; }

/* ---- the surface ---------------------------------------------------------------------------- */
static void turn_to(float sw, float sh);
/* The Grid is laid out upright, W x H; turned sideways (for a tablet held wide) everything it
   draws and every touch is turned a quarter: the bottom of the upright grid (its low notes) on
   the left, its right edge at the bottom. SW x SH is the surface itself. */
static float W = 400, H = 700, SW = 400, SH = 700;
static int turned;
static int eink;
static float center = 60;          /* the middle of the instrument, as a MIDI note */
static double now, last_tick = -1;
static int dirty = 1;

static const float STRIP = 64, BAR = 30, SIDE_TOP = 150;

/* Played state the side controls change (the rail's settings set them too). */
static int latch_on;
static float pos;                  /* the scroll: rows slid by (fractions too), up shows higher notes */
static int holding;
static float side_bend, side_tone;

/* Where things are: worked out again whenever something changes. */
static int cols = 3;
static float cell = 50, gap = 4, left = 0, shown = 7;    /* cell: a row's pitch; shown: rows on screen */
static int mask = 0xAB5, ndeg = 7, degs[12], kshift = 0;
static float bar_x = -1;                                   /* the scroll bar's left edge, or -1 */
typedef struct { int control; float x, y, w, h; } SideRect;
static SideRect sides[MAX_SIDE];
static int nsides;

static int chromatic(void) { return set_[S_CHROMATIC] != 0; }
static int key(void) { return (int)set_[S_KEY]; }
/* Unflipped, the notes climb right to left along each row (for the right hand) and rows climb up. */
static int mirrored(void) { return set_[S_FLIP_X] == 0; }
static int flipped_y(void) { return set_[S_FLIP_Y] != 0; }
static int side_of(int c) { return (int)set_[S_SIDE0 + c]; }

/* The strip of one side, from the bottom up as the thumb reaches: buttons their own height,
   sliders sharing the rest, none taller than 300. */
static void strip(int where, float x) {
    int list[C_COUNT], n = 0, nb = 0, ns = 0;
    for (int c = 0; c < C_COUNT; c++) if (side_of(c) == where) { list[n++] = c; if (c <= C_HOLD) nb++; else ns++; }
    if (!n) return;
    float top = SIDE_TOP, bot = H - 10;
    float spare = (bot - top) - 52.0f * (float)nb - 10.0f * (float)(n - 1);
    float sh = ns ? clampf(spare / (float)ns, 40, 300) : 0;
    float total = 52.0f * (float)nb + sh * (float)ns + 10.0f * (float)(n - 1);
    float y = bot - total;
    for (int k = 0; k < n && nsides < MAX_SIDE; k++) {
        float h = list[k] <= C_HOLD ? 52 : sh;
        SideRect r = {list[k], x + 6, y, STRIP - 12, h};
        sides[nsides++] = r;
        y += h + 10;
    }
}

static int index_of(int note);

static void layout(void) {
    cols = (int)set_[S_COLS];
    if (cols < 2) cols = 2;
    if (cols > 8) cols = 8;
    /* From the outside in, on each side: the scroll bar, then the strip of controls. */
    int nl = 0, nr = 0, bar = (int)set_[S_BAR];
    for (int c = 0; c < C_COUNT; c++) { if (side_of(c) == SIDE_LEFT) nl = 1; if (side_of(c) == SIDE_RIGHT) nr = 1; }
    float lw = (bar == SIDE_LEFT ? BAR : 0) + (nl ? STRIP : 0), rw = (bar == SIDE_RIGHT ? BAR : 0) + (nr ? STRIP : 0);
    bar_x = bar == SIDE_LEFT ? 0 : bar == SIDE_RIGHT ? W - BAR : -1;
    nsides = 0;
    strip(SIDE_LEFT, bar == SIDE_LEFT ? BAR : 0);
    strip(SIDE_RIGHT, W - rw);
    float gx = lw, gw = W - lw - rw;
    if (gw < 40) gw = 40;
    /* Full size: the squares reach both edges, with the spacing for a margin: a row's pitch p,
       its gap g*p, cols*p + g*p across. */
    float g = clampf((float)set_[S_SPACING] / 100.0f, 0.02f, 0.3f);
    float size = clampf((float)set_[S_SIZE] / 100.0f, 0.5f, 1);
    cell = size * gw / ((float)cols + g);
    gap = cell * g;
    left = gx + (gw - (cell * (float)cols + gap)) / 2 + gap;
    shown = H / cell;
    int s = (int)set_[S_SCALE];
    mask = MASKS[s < 0 || s > 8 ? 0 : s];
    ndeg = 0;
    for (int d = 0; d < 12; d++) if ((mask >> d) & 1) degs[ndeg++] = d;
    /* At no scroll, the middle of the screen holds the note nearest the instrument's middle. */
    int target = roundi(center + 12.0f * (float)set_[S_OCTAVE]);
    int mid = (int)floorf_(shown / 2) * cols + cols / 2;
    kshift = index_of(target) - mid;
    dirty = 1;
}

static int in_scale(int note) { return (mask >> floormod(note - key(), 12)) & 1; }

/* The notes in order: index k is the k-th note of the scale (or semitone) above the key's C-1. */
static int note_at(int k) {
    return chromatic() ? key() + k : key() + 12 * floordiv(k, ndeg) + degs[floormod(k, ndeg)];
}
/* The index of a note, or of the nearest note of the scale at or below it. */
static int index_of(int note) {
    int rel = note - key();
    if (chromatic()) return rel;
    int oct = floordiv(rel, 12), d = floormod(rel, 12), i = 0;
    for (int j = 0; j < ndeg; j++) if (degs[j] <= d) i = j;
    return oct * ndeg + i;
}
/* The note of square k (k counts along the rows from row 0, which is at the bottom unscrolled). */
static int note_of(int k) {
    int note = note_at(k + kshift);
    return note < 0 ? 0 : note > 127 ? 127 : note;
}

/* As far as the scroll goes: the lowest and highest notes just on screen. */
static void clamp_pos(void) {
    float lo = (float)floordiv(index_of(0) - kshift, cols) + 1;
    float hi = (float)floordiv(index_of(127) - kshift, cols) - shown + 1;
    if (hi < lo) hi = lo;
    pos = clampf(pos, lo, hi);
}

static int column(int c) { return mirrored() ? cols - 1 - c : c; }

/* Row r's top edge on screen (it may be off it). */
static float row_top(int r) {
    return flipped_y() ? ((float)r - pos) * cell : H - ((float)r - pos + 1) * cell;
}

static int square_at(float x, float y) {
    int c = (int)floorf_((x - left + gap / 2) / cell);
    if (c < 0 || c >= cols) return -100000;
    c = column(c);
    float f = flipped_y() ? y / cell + pos : (H - y) / cell + pos;
    int r = (int)floorf_(f);
    return r * cols + c;
}
static float square_x(int k) { return left + (float)column(floormod(k, cols)) * cell; }
static float square_y(int k) { return row_top(floordiv(k, cols)) + gap / 2; }

/* ---- fingers and their notes ------------------------------------------------------------------ */
typedef struct {
    int used, id, voice, n, note;
    float x0, y0, x, y, sx, sy;   /* where it came down, where it is, and its slow position */
} Finger;
static Finger fingers[MAX_FINGERS];
/* Notes left sounding after their fingers lifted (latch, or hold), oldest first. */
static Finger latched[MAX_VOICES];
static int nlatched;
/* Fingers that came down to let a latched square go: they play nothing until they lift. */
static int spent[MAX_FINGERS], nspent;
/* A square's light after its note was let go, fading. */
typedef struct { int n; float glow; } Ring;
static Ring ring[MAX_RING];
static int next_voice = 1;

typedef struct { int id, control; float from, top, h; float start, y; } SideTouch;
enum { C_BAR = 99 };     /* a finger on the scroll bar */
static SideTouch side_touches[MAX_FINGERS];
static int touched[C_COUNT];

static Finger* find(int id) {
    for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used && fingers[k].id == id) return &fingers[k];
    return 0;
}
static int nfingers(void) { int c = 0; for (int k = 0; k < MAX_FINGERS; k++) c += fingers[k].used; return c; }

typedef struct { float pitch, pressure, slide, volume; } Expr;

/* The axes, from how far the finger has moved (slowly) from where it came down; vibrato from the
   rest. Loudness, tone and volume reach their full change half a square away (the squares are big:
   a whole one was too far to hear much); bend aims at the neighbouring note, a whole square away. */
#define REACH 0.5f
static void apply(Expr* e, int target, float d, int across, const Finger* f, float* semis) {
    float v = clampf(d, -1, 1), w = clampf(d / REACH, -1, 1);
    switch (target) {
    case T_LOUDNESS: {
        /* From 0.6: up to full, down to nearly nothing. The volume follows it too, so it works on
           struck and plucked instruments, which hardly listen to pressure once the note has started. */
        float level = clampf(0.6f + (w >= 0 ? 0.4f : 0.58f) * w, 0.02f, 1);
        e->pressure = level;
        e->volume = clampf(0.4f + level, 0, 1);
        break;
    }
    case T_TONE: e->slide = clampf(0.5f + 0.5f * w, 0, 1); break;
    case T_BEND: {
        float amount = clampf((absf(v) - 0.08f) / 0.92f, 0, 1);
        int r = (int)set_[S_BEND_RANGE];
        float range = BEND_RANGES[r < 0 || r > 6 ? 0 : r];
        if (range > 0) { *semis += (v >= 0 ? range : -range) * amount; break; }
        /* Toward the note you drag to: across, the next square or the one before (unflipped the
           next is to the left); up and down, the row above or below. */
        int step = !across ? (flipped_y() ? -cols : cols) : mirrored() ? -1 : 1;
        int to = note_of(f->n + (v >= 0 ? step : -step));
        *semis += (float)(to - f->note) * amount;
        break;
    }
    case T_VOLUME: e->volume = clampf(0.7f + (w >= 0 ? 0.3f : 0.7f) * w, 0, 1); break;   /* both ways, from 0.7 */
    default: break;
    }
}

static Expr expression(const Finger* f) {
    /* Where nothing moves them: a firm press, the tone in the middle, full volume (or Volume's 0.7). */
    Expr e = {0, 0.75f, 0.5f, 1};
    if ((int)set_[S_ACROSS] == T_VOLUME || (int)set_[S_UPDOWN] == T_VOLUME) e.volume = 0.7f;
    float semis = 0;
    apply(&e, (int)set_[S_ACROSS], (f->sx - f->x0) / cell, 1, f, &semis);
    apply(&e, (int)set_[S_UPDOWN], (f->y0 - f->sy) / cell, 0, f, &semis);
    semis += side_bend;
    /* Snap the bend onto semitones, as the gyro's does: the middle of each semitone holds the note
       and the move to the next is squeezed toward the edges (all of it, at full snap). Not the
       vibrato, added after. */
    float g = 1 - clampf((float)set_[S_BEND_SNAP] / 100.0f, 0, 1);
    if (g < 0.995f && semis != 0) {
        float base = floorf_(semis), t = semis - base;
        semis = base + (g < 0.005f ? (t < 0.5f ? 0 : 1) : clampf((t - (1 - g) / 2) / g, 0, 1));
    }
    e.slide = clampf(e.slide + side_tone, 0, 1);
    int vi = (int)set_[S_VIBRATO];
    float depth = VIBRATO_DEPTH[vi < 0 || vi > 3 ? 0 : vi];
    if (depth > 0) semis += clampf((f->x - f->sx) * 0.08f * depth / 0.5f, -depth, depth);
    e.pitch = (float)f->note + semis;
    return e;
}

/* The MESH sends only what changed, so every finger can be sent every frame. */
static void send(const Finger* f) {
    Expr e = expression(f);
    mesh_note_volume(f->voice, e.volume, now);
    mesh_note_change(f->voice, e.pitch, e.pressure, e.slide, now);
}

static void glow_after(int n) {
    for (int k = 0; k < MAX_RING; k++) if (ring[k].glow <= 0 || ring[k].n == n) { ring[k].n = n; ring[k].glow = eink ? 0 : 1; return; }
}

/* Note-off, and the square's light fades after it. */
static void let_go(const Finger* f) {
    mesh_note_off(f->voice, now);
    glow_after(f->n);
    dirty = 1;
}

static void unlatch(int k) {
    let_go(&latched[k]);
    for (int j = k; j < nlatched - 1; j++) latched[j] = latched[j + 1];
    nlatched--;
}

static void release_latched(void) {
    while (nlatched > 0) unlatch(0);
}

static int latching(void) { return latch_on || holding; }

static int find_latched(int n) {
    for (int k = 0; k < nlatched; k++) if (latched[k].n == n) return k;
    return -1;
}

static void down(int id, float x, float y) {
    int n = square_at(x, y);
    if (n == -100000) return;
    if (latch_on && set_[S_LATCHMODE] == LATCH_TAP) {
        /* Tap a held square to let it go. */
        int k = find_latched(n);
        if (k >= 0) { unlatch(k); if (nspent < MAX_FINGERS) spent[nspent++] = id; return; }
    } else if (latch_on && nfingers() == 0) release_latched();    /* a new hand: the last chord goes */
    /* The same note held over (hold, or a chord being added to) is played again, not doubled. */
    int k = find_latched(n);
    if (k >= 0) unlatch(k);
    /* Every voice busy: the oldest latched note makes way. */
    if (nfingers() + nlatched >= MAX_VOICES) { if (nlatched) unlatch(0); else return; }
    Finger* f = 0;
    for (int j = 0; j < MAX_FINGERS; j++) if (!fingers[j].used) { f = &fingers[j]; break; }
    if (!f) return;
    f->used = 1; f->id = id; f->voice = next_voice++; f->n = n; f->note = note_of(n);
    f->x0 = f->x = f->sx = x; f->y0 = f->y = f->sy = y;
    for (int r = 0; r < MAX_RING; r++) if (ring[r].n == n) ring[r].glow = 0;
    Expr e = expression(f);
    mesh_note_on(f->voice, 0, e.pitch, e.pressure, e.slide, e.pressure, now);
    if (e.volume < 1) mesh_note_volume(f->voice, e.volume, now);
    dirty = 1;
}

static void up(int id) {
    for (int k = 0; k < nspent; k++) if (spent[k] == id) { spent[k] = spent[--nspent]; return; }
    Finger* f = find(id);
    if (!f) return;
    f->used = 0;
    if (latching() && nlatched < MAX_VOICES) {
        /* It stays as it is now (no vibrato left in it), until the latch or hold lets it go. */
        f->sx = f->x; f->sy = f->y;
        send(f);
        latched[nlatched++] = *f;
    } else let_go(f);
    dirty = 1;
}

static void set_latch(int on) {
    latch_on = on;
    if (!on && !holding) release_latched();
    dirty = 1;
}

/* The Hold button: while pressed, lifted fingers leave their notes; let go and they go (unless latched). */
static void hold(int on) {
    holding = on;
    if (!on && !latch_on) release_latched();
    dirty = 1;
}

static void resend_all(void) {
    for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used) send(&fingers[k]);
    for (int k = 0; k < nlatched; k++) send(&latched[k]);
}


/* ---- drawing, turned or not ----------------------------------------------------------------------- */
/* Upright (x, y) on the surface: turned, x along the upright grid goes down the surface and y,
   from the bottom, goes left to right. */
static float tx(float x, float y) { return turned ? H - y : x; }
static float ty(float x, float y) { return turned ? x : y; }
static void g_clear(unsigned c) { mesh_clear(c); }
static void g_rect(float x, float y, float w, float h, float r, unsigned fill, unsigned stroke, float sw) {
    if (turned) mesh_rect(H - (y + h), x, h, w, r, fill, stroke, sw);
    else mesh_rect(x, y, w, h, r, fill, stroke, sw);
}
static void g_circle(float x, float y, float r, unsigned fill, unsigned stroke, float sw) { mesh_circle(tx(x, y), ty(x, y), r, fill, stroke, sw); }
static void g_line(float x1, float y1, float x2, float y2, unsigned c, float w) { mesh_line(tx(x1, y1), ty(x1, y1), tx(x2, y2), ty(x2, y2), c, w); }
static void g_glow(float x, float y, float r, unsigned a, unsigned b) { mesh_glow(tx(x, y), ty(x, y), r, a, b); }
/* Text centred on an upright point, then moved by (dx, dy) on the surface itself: lines of a
   button stay one above the other, turned or not. */
static void g_text_at(float x, float y, float dx, float dy, float size, unsigned c, int flags, const char* str) {
    mesh_text(tx(x, y) + dx, ty(x, y) + dy + size * 0.35f, size, c, flags, str);
}
/* Text stays upright: its middle is moved, and it is drawn level there. */
static void g_text(float x, float y, float size, unsigned c, int flags, const char* str) {
    float my = y - size * 0.35f;
    mesh_text(tx(x, my), ty(x, my) + size * 0.35f, size, c, flags, str);
}

/* ---- the side controls ---------------------------------------------------------------------- */
static float frac(SideTouch* s, float y) { return clampf(1 - (y - s->top) / s->h, 0, 1); }

static void side_drag(SideTouch* s, float at) {
    if (s->control == C_BEND) {
        side_bend = clampf((at - s->from) * 4, -2, 2);
        resend_all();
    } else if (s->control == C_TONE) {
        side_tone = clampf(at - 0.5f, -0.5f, 0.5f);
        resend_all();
    }
    dirty = 1;
}

/* The scroll bar: the rows follow the finger, as a list does. */
static void bar_drag(SideTouch* s, float y) {
    float rows = (y - s->y) / cell;
    pos = s->start + (flipped_y() ? -rows : rows);
    clamp_pos();
    dirty = 1;
}

static int side_down(int id, float x, float y) {
    if (bar_x >= 0 && x >= bar_x - 4 && x <= bar_x + BAR + 4) {
        for (int j = 0; j < MAX_FINGERS; j++) if (side_touches[j].id == 0) {
            SideTouch* s = &side_touches[j];
            s->id = id; s->control = C_BAR; s->start = pos; s->y = y;
            break;
        }
        return 1;
    }
    for (int k = 0; k < nsides; k++) {
        SideRect* r = &sides[k];
        if (x < r->x - 5 || x > r->x + r->w + 5 || y < r->y || y > r->y + r->h) continue;
        for (int j = 0; j < MAX_FINGERS; j++) if (side_touches[j].id == 0) {
            SideTouch* s = &side_touches[j];
            s->id = id; s->control = r->control; s->top = r->y; s->h = r->h;
            s->from = frac(s, y);
            touched[r->control]++;
            if (r->control == C_HOLD) hold(1);
            side_drag(s, s->from);
            break;
        }
        return 1;
    }
    return 0;
}

static SideTouch* side_find(int id) {
    for (int j = 0; j < MAX_FINGERS; j++) if (side_touches[j].id == id) return &side_touches[j];
    return 0;
}

static void side_up(SideTouch* s) {
    if (s->control == C_BAR) { s->id = 0; dirty = 1; return; }
    touched[s->control]--;
    if (s->control == C_HOLD) hold(0);
    else if (s->control == C_LATCH) set_latch(!latch_on);
    else if (s->control == C_BEND) { side_bend = 0; resend_all(); }
    s->id = 0;
    dirty = 1;
}

/* ---- drawing ---------------------------------------------------------------------------------- */
static const char* NAMES[12] = {"C", "C\xE2\x99\xAF", "D", "E\xE2\x99\xAD", "E", "F", "F\xE2\x99\xAF", "G", "A\xE2\x99\xAD", "A",
                                "B\xE2\x99\xAD", "B"};

static char* put(char* p, const char* s) { while (*s) *p++ = *s++; *p = 0; return p; }
static char* put_int(char* p, int v, int plus) {
    if (v < 0) { *p++ = '-'; v = -v; } else if (plus) *p++ = '+';
    char d[12]; int n = 0;
    do { d[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *p++ = d[--n];
    *p = 0;
    return p;
}

static void draw_button(SideRect* r, const char* label, const char* value, int on, const Theme* th, unsigned ink) {
    unsigned fill = on ? (eink ? BLACK : mesh_alpha(ink, 0.85f)) : eink ? WHITE : th->square;
    unsigned edge = on ? (eink ? BLACK : th->hot) : mesh_alpha(ink, 0.5f);
    g_rect(r->x, r->y, r->w, r->h, 10, fill, edge, 1.5f);
    unsigned text = on ? (eink ? WHITE : th->on_light) : ink;
    float cx = r->x + r->w / 2, cy = r->y + r->h / 2;
    g_text_at(cx, cy, 0, -7, 11, text, MESH_TEXT_CENTER | MESH_TEXT_BOLD, label);
    g_text_at(cx, cy, 0, 8, 9, mesh_alpha(text, 0.75f), MESH_TEXT_CENTER, value);
}

static void draw_slider(SideRect* r, const char* label, const char* value, float at, int ticks, const Theme* th, unsigned ink) {
    g_rect(r->x, r->y, r->w, r->h, 10, eink ? WHITE : th->square, mesh_alpha(ink, touched[r->control] ? 0.9f : 0.5f), 1.5f);
    float top = r->y + 46, bot = r->y + r->h - 12;
    for (int i = 0; i < ticks; i++) {
        float y = bot - (bot - top) * (float)i / (float)(ticks - 1);
        g_line(r->x + r->w * 0.3f, y, r->x + r->w * 0.7f, y, mesh_alpha(ink, 0.3f), 1);
    }
    float y = bot - (bot - top) * clampf(at, 0, 1), mid = bot - (bot - top) * 0.5f, x = r->x + r->w / 2;
    g_line(x, mid, x, y, mesh_alpha(ink, 0.6f), 4);
    g_rect(r->x + r->w * 0.18f, y - 5, r->w * 0.64f, 10, 5, eink ? BLACK : th->hot, 0, 0);
    /* The name and value at the slider's top end (upright), its right end (turned). */
    if (turned) {
        g_text_at(x, r->y + 22, 0, -7, 11, ink, MESH_TEXT_CENTER | MESH_TEXT_BOLD, label);
        g_text_at(x, r->y + 22, 0, 7, 9, mesh_alpha(ink, 0.75f), MESH_TEXT_CENTER, value);
    } else {
        g_text(x, r->y + 15, 11, ink, MESH_TEXT_CENTER | MESH_TEXT_BOLD, label);
        g_text(x, r->y + 28, 10, mesh_alpha(ink, 0.75f), MESH_TEXT_CENTER, value);
    }
}

static void draw_sides(const Theme* th) {
    unsigned ink = eink ? BLACK : th->light;
    char v[24];
    for (int k = 0; k < nsides; k++) {
        SideRect* r = &sides[k];
        switch (r->control) {
        case C_LATCH:
            if (!latch_on) put(v, "off");
            else if (nlatched > 0) put(put_int(v, nlatched, 0), " held");
            else put(v, "on");
            draw_button(r, "Latch", v, latch_on, th, ink);
            break;
        case C_HOLD: draw_button(r, "Hold", holding ? "holding" : "press", holding, th, ink); break;
        case C_BEND: {
            int tenths = roundi(side_bend * 10);
            if (tenths == 0) put(v, "0");
            else {
                char* p = v;
                if (tenths < 0) { *p++ = '-'; tenths = -tenths; } else *p++ = '+';
                p = put_int(p, tenths / 10, 0);
                *p++ = '.';
                put_int(p, tenths % 10, 0);
            }
            draw_slider(r, "Bend", v, 0.5f + side_bend / 4, 5, th, ink);
            break;
        }
        case C_TONE:
            put_int(v, roundi(side_tone * 200), 1);
            draw_slider(r, "Tone", v, 0.5f + side_tone, 3, th, ink);
            break;
        }
    }
}

/* The colour a note is marked with, or 0 to use the theme's. */
static unsigned note_colour(int note) {
    int scheme = (int)set_[S_COLOURS], iv = floormod(note - key(), 12);
    if (chromatic() && !in_scale(note)) return 0;
    switch (scheme) {
    case COL_DEGREES: return DEGREE[iv];
    case COL_CHORD: return iv == 0 || iv == 3 || iv == 4 || iv == 7 ? DEGREE[iv] : 0;
    case COL_PILLARS: return iv == 0 || iv == 5 || iv == 7 ? DEGREE[iv] : 0;
    case COL_NAMES: return NAMED[floormod(note, 12)];
    default: return 0;
    }
}

static int is_held(int n) { for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used && fingers[k].n == n) return 1; return 0; }
static int is_kept(int n) { for (int k = 0; k < nlatched; k++) if (latched[k].n == n) return 1; return 0; }
static float glow_of(int n) {
    float g = 0;
    for (int k = 0; k < MAX_RING; k++) if (ring[k].glow > g && ring[k].n == n) g = ring[k].glow;
    return g;
}

static void note_name(char* out, int note, int octave) {
    char* p = put(out, NAMES[note % 12]);
    if (octave) put_int(p, note / 12 - 1, 0);
}

/* The scroll bar: the height of the screen, a marker and its octave at each row that holds the key's root. */
static void draw_bar(const Theme* th) {
    if (bar_x < 0) return;
    int flat = eink;
    unsigned ink = flat ? BLACK : th->light;
    g_rect(bar_x + 3, 4, BAR - 6, H - 8, 8, flat ? WHITE : mesh_alpha(th->square, 0.9f), mesh_alpha(ink, 0.35f), 1);
    int r0 = (int)floorf_(pos) - 1, r1 = (int)floorf_(pos + shown) + 1;
    for (int r = r0; r <= r1; r++) {
        int root = -1;
        for (int c = 0; c < cols; c++) { int note = note_of(r * cols + c); if (floormod(note - key(), 12) == 0) { root = note; break; } }
        if (root < 0) continue;
        float y = row_top(r) + cell / 2;
        if (y < 10 || y > H - 10) continue;
        unsigned c = note_colour(root);
        unsigned mark = c && !flat ? c : ink;
        g_line(bar_x + 5, y, bar_x + BAR - 5, y, mesh_alpha(mark, 0.8f), 2);
        char name[8];
        note_name(name, root, 1);
        /* The octave beside its mark: above it upright, to its right turned. */
        if (turned) g_text_at(bar_x + BAR / 2, y, 6, 0, 10, mark, MESH_TEXT_LEFT | MESH_TEXT_BOLD, name);
        else g_text_at(bar_x + BAR / 2, y, 0, -9, 10, mark, MESH_TEXT_CENTER | MESH_TEXT_BOLD, name);
    }
}

/* Lit outlines on squares a shade apart from the background; a sounding square full of light. */
static void draw(void) {
    layout();
    clamp_pos();
    const Theme* th = theme();
    int flat = eink;
    mesh_begin();
    g_clear(flat ? WHITE : th->background);
    float radius = cell * 0.08f, side = cell - gap;
    unsigned light = flat ? BLACK : th->light;
    int r0 = (int)floorf_(pos) - 1, r1 = (int)floorf_(pos + shown) + 1;
    for (int r = r0; r <= r1; r++) for (int c = 0; c < cols; c++) {
        int n = r * cols + c;
        float ox = square_x(n), oy = square_y(n);
        if (oy > H || oy + side < 0) continue;
        int note = note_of(n);
        int root = floormod(note - key(), 12) == 0;
        int off = chromatic() && !in_scale(note);
        int on = is_held(n), kept = is_kept(n);
        float glow = on || kept ? 1 : glow_of(n);
        unsigned colour = flat ? 0 : note_colour(note);
        unsigned lit = colour ? colour : light;
        /* The square: a shade apart from the background, tinted with its note's colour; a note
           outside the scale sinks nearly into the background. */
        if (!flat) {
            g_rect(ox, oy, side, side, radius, off ? mesh_alpha(th->square, 0.3f) : th->square, 0, 0);
            if (colour && glow < 1) g_rect(ox, oy, side, side, radius, mesh_alpha(colour, th->bright ? 0.35f : 0.22f), 0, 0);
        }
        /* The light: an outer bloom, then the fill. */
        if (glow > 0) {
            if (!flat && !th->bright) g_rect(ox - 6, oy - 6, side + 12, side + 12, radius + 6, mesh_alpha(lit, 0.28f * glow), 0, 0);
            g_rect(ox, oy, side, side, radius, mesh_alpha(lit, (flat ? 1 : 0.92f) * glow), 0, 0);
        }
        /* The outline: the root's fully lit, the scale's soft, a note outside the scale faint. */
        unsigned edge;
        if (on || glow > 0.6f) edge = flat ? BLACK : colour ? WHITE : th->hot;
        else if (off) edge = mesh_alpha(light, flat ? 0.25f : 0.10f);
        else if (colour) edge = mesh_alpha(colour, root ? 1 : 0.8f);
        else edge = root ? light : mesh_alpha(light, flat ? 0.8f : 0.38f);
        if (root && !flat && !on && !th->bright)
            g_rect(ox - 3, oy - 3, side + 6, side + 6, radius + 3, 0, mesh_alpha(colour ? colour : light, 0.18f), 6);
        g_rect(ox, oy, side, side, radius, 0, edge, off ? 1.0f : root ? 2.5f : 1.5f);
        /* A latched square: a dot in its corner, as long as it is held. */
        if (kept && !on)
            g_circle(ox + side * 0.84f, oy + side * 0.16f, side * 0.06f,
                        glow > 0.5f ? (flat ? WHITE : th->on_light) : (flat ? BLACK : th->hot), 0, 0);
        if (set_[S_LABELS] != 0) {
            float size = side * 0.16f;
            unsigned tc = glow > 0.5f ? (flat ? WHITE : colour ? 0x101010FFu : th->on_light)
                        : mesh_alpha(colour ? colour : light, off ? 0.3f : 0.85f);
            char name[16];
            note_name(name, note, root);
            g_text(ox + side / 2, oy + side / 2 + size * 0.35f, size, tc, MESH_TEXT_CENTER | (root ? MESH_TEXT_BOLD : 0), name);
        }
    }
    /* A halo that follows each finger. */
    if (!flat && !th->bright)
        for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used)
            g_glow(fingers[k].x, fingers[k].y, cell * 0.9f, mesh_alpha(th->hot, 0.5f), mesh_alpha(th->light, 0));
    draw_bar(th);
    draw_sides(th);
    mesh_end();
    dirty = 0;
}

/* ---- the interface ------------------------------------------------------------------------------- */
/* Upright or sideways: Auto turns it sideways when the surface is wider than it is tall. */
static void turn_to(float sw, float sh) {
    SW = sw; SH = sh;
    int t = (int)set_[S_TURN];
    turned = t == TURN_SIDEWAYS || (t == TURN_AUTO && sw > sh * 1.05f);
    W = turned ? sh : sw;
    H = turned ? sw : sh;
}

MESH_EXPORT(init) void init(float width, float height, int flags, double t) {
    eink = flags & MESH_EINK; now = t;
    turn_to(width, height);
    layout();
    draw();
}

MESH_EXPORT(instrument) void instrument(int part, float center_note) {
    if (part == 0) { center = center_note; dirty = 1; }
}

MESH_EXPORT(setting) void setting(int index, double value) {
    if (index < 0 || index >= S_COUNT) return;
    set_[index] = value;
    if (index == S_LATCH) set_latch(value != 0);
    if (index == S_TURN) turn_to(SW, SH);
    layout();
    clamp_pos();
}

MESH_EXPORT(touch) void touch(int id, int phase, float x, float y, float size, float force, double t) {
    (void)size; (void)force;
    now = t;
    if (turned) { float ux = y, uy = H - x; x = ux; y = uy; }   /* the touch, as on the upright grid */
    id += 1;    /* 0 marks a free side touch */
    if (phase == MESH_DOWN) {
        if (find(id)) up(id);
        layout();
        if (!side_down(id, x, y)) down(id, x, y);
    } else if (phase == MESH_MOVE) {
        SideTouch* s = side_find(id);
        if (s) { if (s->control == C_BAR) bar_drag(s, y); else side_drag(s, frac(s, y)); return; }
        Finger* f = find(id);
        if (!f) return;
        f->x = x; f->y = y;
        send(f);
    } else {
        SideTouch* s = side_find(id);
        if (s) side_up(s);
        else up(id);
    }
    dirty = 1;
}

MESH_EXPORT(tick) void tick(double t) {
    float dt = last_tick < 0 ? 0 : (float)((t - last_tick) / 1000.0);
    if (dt < 0) dt = 0;
    if (dt > 0.25f) dt = 0.25f;
    last_tick = t;
    now = t;
    /* The slow position trails the finger by about 0.12 s: a wiggle is faster than that. */
    float k = 1 - expf_(-dt / 0.12f);
    int any = 0;
    for (int j = 0; j < MAX_FINGERS; j++) if (fingers[j].used) {
        Finger* f = &fingers[j];
        f->sx += (f->x - f->sx) * k;
        f->sy += (f->y - f->sy) * k;
        send(f);
        any = 1;
    }
    /* A let-go square's light fades, as the sound does (about half a second; the MESH does not say
       how loud a voice still is). */
    for (int r = 0; r < MAX_RING; r++) if (ring[r].glow > 0) {
        ring[r].glow = eink ? 0 : ring[r].glow * expf_(-dt / 0.25f) - dt * 0.2f;
        if (ring[r].glow < 0.004f) ring[r].glow = 0;
        any = 1;
    }
    if (any && !eink) dirty = 1;
    if (dirty) draw();
}

MESH_EXPORT(stop) void stop(void) {
    for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used) { let_go(&fingers[k]); fingers[k].used = 0; }
    for (int k = 0; k < MAX_FINGERS; k++) side_touches[k].id = 0;
    for (int c = 0; c < C_COUNT; c++) touched[c] = 0;
    nspent = 0;
    holding = 0;
    side_bend = 0;
    release_latched();
}
