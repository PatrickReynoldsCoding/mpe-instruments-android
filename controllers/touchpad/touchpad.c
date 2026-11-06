/*
 * The touch pad as a MESH controller: the screen as an instrument. Columns of notes left to
 * right in a key and scale, each finger its own voice. Slide sideways to glide, up for louder
 * and brighter (the bottom fades to silence), rock a finger for vibrato. The note strip along
 * the bottom shows the notes; drag it sideways to move them (locked, it plays instead).
 *
 * A port of odo's built-in touch pad (PadScreen.kt, TouchPad.kt, PadSettings in Perf.kt), one
 * pad (no split) in scale mode (no chord wheel or grid). The strip's shift lives in memory only.
 *
 * Android draws the pad in pixels; its pixel sizes (line widths, orbs, ripples) are given here
 * in dp at a typical phone density, PX dp per pixel.
 */
#include "mesh.h"

#define MAX_FINGERS 15
#define MAX_COLS 24
#define TRAIL 28
#define MAX_RIPPLES 16
#define MAX_GRABS 4

static const float PX = 0.38f;          /* dp per Android pixel, at ~2.6 px per dp */
static const float STRIP = 64;          /* the note strip's height, dp */

/* ---- settings (the manifest's, in order) ---------------------------------------------------- */
enum { S_SIZE, S_OCTAVE, S_KEY, S_SCALE, S_VERTICAL, S_FADE, S_GLIDE, S_VIBRATO, S_VIBSENSE, S_LOCKED, S_THEME, S_LINES,
       S_DRUMS, S_COUNT };
static double set_[S_COUNT] = {8, 0, 0, 1, 0, 2, 0, 2, 1, 0, 0, 1, 0};

static const int SCALES[] = {0, 0xAB5, 0x5AD, 0x6AD, 0x6B5, 0x9AD, 0x295, 0x4A9, 0x4E9, 0xFFF};
static const float VIBRATO_DEPTH[] = {0, 0.25f, 0.5f, 1};
static const float VIB_SENSE_GAIN[] = {0.5f, 1, 2, 4};
static const float FADE_HEIGHT[] = {0, 0.1f, 0.25f, 0.5f, 1};

/* Vibrato: semitones per dp of wobble at the Normal depth and Medium sensitivity, before it rounds off. */
static const float VIB_SEMIS_PER_DP = 0.08f, VIBRATO_NORMAL = 0.5f;
static const float VIB_STILL_DP = 0.4f;     /* wobble smaller than this is a still finger's jitter */
static const float VIB_SLIDE_DPS = 40;      /* a finger moving one way this fast is half sliding */
static const float SNAP_HOLD = 0.08f;       /* Snap: columns past the edge before the note changes */

/* The drum kit's notes (drums/kit.dsp, General MIDI order from C), most used first. */
static const int DRUM_ORDER[12] = {36, 38, 42, 46, 39, 37, 44, 40, 41, 43, 45, 47};
static const char* DRUM_NAMES[12] = {"Kick", "Rim", "Snare", "Clap", "Crash", "Tom 1", "Hat", "Tom 2", "Pedal", "Tom 3",
                                     "Open", "Bell"};

static int iset(int k) { return (int)set_[k]; }

/* ---- small maths ------------------------------------------------------------------------------ */
static float floorf_(float x) { return __builtin_floorf(x); }
static int roundi(float x) { return (int)floorf_(x + 0.5f); }
static float absf(float x) { return x < 0 ? -x : x; }
static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int mod12(int v) { return ((v % 12) + 12) % 12; }
static float expf_(float x) {           /* good to ~1e-5 for the small arguments used here */
    if (x < -20) return 0;
    if (x > 20) x = 20;
    float y = x / 32, r = 1 + y * (1 + y * (0.5f + y * (1.0f / 6 + y * (1.0f / 24 + y / 120))));
    for (int k = 0; k < 5; k++) r *= r;
    return r;
}
static float tanhf_(float x) {
    if (x > 9) return 1;
    if (x < -9) return -1;
    float e = expf_(2 * x);
    return (e - 1) / (e + 1);
}

/* ---- themes (PAD_THEMES) ------------------------------------------------------------------------ */
typedef struct { unsigned top, mid, bottom, line, label, strip, part; int flat; } Theme;
static const Theme THEMES[6] = {
    {0x1A1626FFu, 0x0E1014FFu, 0x0B1418FFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x07080BFFu, 0xFF8A65FFu, 0},   /* Night */
    {0x3A1430FFu, 0x2A1022FFu, 0x1A0C18FFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x12070FFFu, 0xFFB74DFFu, 0},   /* Sunset */
    {0x0A2E3AFFu, 0x07202BFFu, 0x041318FFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x020B0EFFu, 0x80DEEAFFu, 0},   /* Ocean */
    {0x1B2A17FFu, 0x131E11FFu, 0x0C140BFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x060B05FFu, 0xD4E157FFu, 0},   /* Forest */
    {0x000000FFu, 0x000000FFu, 0x000000FFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x1C1C1CFFu, 0xFFEB3BFFu, 0},   /* Contrast */
    {0xF7F3EAFFu, 0xF1ECE1FFu, 0xE9E3D6FFu, 0x1B1B1BFFu, 0x1B1B1BFFu, 0xDCD4C3FFu, 0xD84315FFu, 0},   /* Daylight */
};
static const Theme EINK_THEME = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x000000FFu, 0x000000FFu, 0xFFFFFFFFu, 0x000000FFu, 1};
static const unsigned WHITE = 0xFFFFFFFFu, BLACK = 0x000000FFu;

/* ---- the surface and its columns ---------------------------------------------------------------- */
static float W = 600, H = 300;
static int eink;
static float center = 60;
static double now;
static int dirty = 1;

static int columns[MAX_COLS], ncols = 8;
static int shift_, shift_lo, shift_hi;  /* the strip's shift (scale steps) and how far it can go */

static const Theme* theme(void) { return eink ? &EINK_THEME : &THEMES[clampi(iset(S_THEME), 0, 5)]; }
static float playH(void) { return H - STRIP > 1 ? H - STRIP : 1; }
static int drums(void) { return set_[S_DRUMS] != 0; }

static void layout(void) {
    if (drums()) {
        ncols = clampi(iset(S_SIZE), 4, 12);
        for (int i = 0; i < ncols; i++) columns[i] = DRUM_ORDER[i];
        shift_lo = shift_hi = 0;
        dirty = 1;
        return;
    }
    int size = clampi(iset(S_SIZE), 3, MAX_COLS);
    int mask = SCALES[clampi(iset(S_SCALE), 0, 9)];
    if (mask == 0) mask = 0xFFF;
    int in[128], n = 0;
    for (int p = 0; p < 128; p++) if ((mask >> mod12(p - iset(S_KEY))) & 1) in[n++] = p;
    float want = center + 12.0f * (float)iset(S_OCTAVE);
    int k = 0;
    for (int q = 1; q < n; q++) if (absf((float)in[q] - want) < absf((float)in[k] - want)) k = q;
    int base = k - size / 2;
    int last = n - size > 0 ? n - size : 0;
    shift_lo = -base; shift_hi = last - base;
    int start = clampi(base + shift_, 0, last);
    ncols = size;
    for (int i = 0; i < size; i++) columns[i] = start + i < n ? in[start + i] : 127;
    dirty = 1;
}

/* x (0..1 across the pad) to sounding pitch, for a glide amount g (0 snap .. 1 free). */
static float pitch_at(const int* cols, int n, float x, float g) {
    float u = x * (float)n - 0.5f;
    int i = clampi((int)floorf_(u), 0, n - 2);
    float t = u - (float)i;
    float r = g >= 0.995f ? t : clampf((t - (1 - g) / 2) / g, 0, 1);
    return (float)cols[i] + r * (float)(cols[i + 1] - cols[i]);
}
static int column_at(int n, float x) { return clampi(roundi(x * (float)n - 0.5f), 0, n - 1); }
/* Sounding pitch back to x across the pad, for drawing. */
static float x_of(float pitch) {
    if (ncols < 2) return 0.5f;
    int i = 0;
    while (i < ncols - 2 && pitch > (float)columns[i + 1]) i++;
    int span = columns[i + 1] - columns[i];
    if (span < 1) span = 1;
    return ((float)i + (pitch - (float)columns[i]) / (float)span + 0.5f) / (float)ncols;
}

/* ---- fingers ---------------------------------------------------------------------------------- */
typedef struct {
    int used, id, voice, drum;
    int cols[MAX_COLS], n;     /* the columns it went down on: dragging the strip does not bend it */
    float x, y;                /* across the pad 0..1 (may go past its edges); height 0 bottom .. 1 top */
    float base;                /* pitch from where it is */
    int col;                   /* Snap: the column it plays */
    float centre, drift, lastX, vib;
    int q_pitch, q_pr, q_sl, q_vol;   /* what was last sent, quantised as the host does */
    float trail[TRAIL * 2];
    int tn;
} Finger;
static Finger fingers[MAX_FINGERS];
static int next_voice = 1;

typedef struct { int used, id, shift; float x; } Grab;
static Grab grabs[MAX_GRABS];

typedef struct { float x, y, age; } Ripple;
static Ripple ripples[MAX_RIPPLES];

static float pitch_of(const Finger* f) { return f->base + f->vib; }

/* Where the finger is to its pitch, by the glide. Snap holds a note until the finger is clearly in the next column. */
static void place(Finger* f) {
    float g = (float)set_[S_GLIDE];
    int n = f->n;
    if (f->drum) { f->base = (float)f->cols[clampi(f->col, 0, n - 1)]; return; }
    if (g <= 0.005f || n < 2) {
        float u = f->x * (float)n - 0.5f;
        if (f->col < 0 || absf(u - (float)f->col) > 0.5f + SNAP_HOLD) f->col = clampi(roundi(u), 0, n - 1);
        f->base = (float)f->cols[f->col];
    } else {
        f->col = -1;
        f->base = pitch_at(f->cols, n, f->x, g);
    }
}

/* Pressure, slide and volume for a finger's height, by the Up/down setting. */
static void expression(const Finger* f, float* p, float* sl, float* vol) {
    float y = f->y, h = FADE_HEIGHT[clampi(iset(S_FADE), 0, 4)];
    float fade = 1;
    if (h > 0) { fade = clampf(y / h, 0, 1); fade *= fade; }
    switch (iset(S_VERTICAL)) {
    case 1: *p = 0.7f; *sl = y; *vol = 1; break;
    case 2: *p = y; *sl = 0.5f; *vol = fade; break;
    default: *p = y; *sl = y; *vol = fade; break;
    }
}

static void send_expression(Finger* f) {
    float p, sl, vol, pitch = pitch_of(f);
    expression(f, &p, &sl, &vol);
    int qp = roundi(pitch * 8192.0f / 48), qr = roundi(p * 127), qs = roundi(sl * 127), qv = roundi(vol * 127);
    if (qv != f->q_vol) { f->q_vol = qv; mesh_note_volume(f->voice, vol, now); }
    if (qp != f->q_pitch || qr != f->q_pr || qs != f->q_sl) {
        f->q_pitch = qp; f->q_pr = qr; f->q_sl = qs;
        mesh_note_change(f->voice, pitch, p, sl, now);
    }
}

static Finger* find(int id) {
    for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used && fingers[k].id == id) return &fingers[k];
    return 0;
}
static Grab* find_grab(int id) {
    for (int k = 0; k < MAX_GRABS; k++) if (grabs[k].used && grabs[k].id == id) return &grabs[k];
    return 0;
}

static void down(int id, float x, float y) {
    Finger* f = 0;
    for (int k = 0; k < MAX_FINGERS && !f; k++) if (!fingers[k].used) f = &fingers[k];
    if (!f) return;
    f->used = 1; f->id = id; f->voice = next_voice++; f->drum = drums();
    f->n = ncols;
    for (int i = 0; i < ncols; i++) f->cols[i] = columns[i];
    f->x = x; f->y = clampf(y, 0, 1);
    f->col = column_at(ncols, x);
    f->centre = f->lastX = x; f->drift = 0; f->vib = 0; f->tn = 0;
    place(f);
    float p, sl, vol, pitch = pitch_of(f);
    expression(f, &p, &sl, &vol);
    /* The strike follows the height too. */
    mesh_note_on(f->voice, 0, pitch, p, sl, p, now);
    /* The engine clears bend and pressure at the note on, which the host sends just before it:
       send them again, first a hair off so the host does not drop them as unchanged. */
    mesh_note_change(f->voice, pitch + 0.02f, p < 0.5f ? p + 0.01f : p - 0.01f, sl, now);
    mesh_note_change(f->voice, pitch, p, sl, now);
    mesh_note_volume(f->voice, vol, now);
    f->q_pitch = roundi(pitch * 8192.0f / 48); f->q_pr = roundi(p * 127); f->q_sl = roundi(sl * 127); f->q_vol = roundi(vol * 127);
    if (!eink) for (int r = 0; r < MAX_RIPPLES; r++) if (ripples[r].age <= 0 || ripples[r].age > 0.7f) {
        ripples[r].x = x; ripples[r].y = f->y; ripples[r].age = 1e-4f;
        break;
    }
    dirty = 1;
}

static void lift(Finger* f) {
    mesh_note_off(f->voice, now);
    f->used = 0;
    dirty = 1;
}

/* A touch on the note strip drags the notes sideways; locked (or a drum kit), it plays. */
static void strip_down(int id, float x, float y) {
    if (set_[S_LOCKED] != 0 || drums()) { down(id, x, y); return; }
    for (int k = 0; k < MAX_GRABS; k++) if (!grabs[k].used) {
        grabs[k].used = 1; grabs[k].id = id; grabs[k].x = x; grabs[k].shift = shift_;
        dirty = 1;
        return;
    }
}

static void grab_move(Grab* g, float x) {
    int lo = shift_lo < g->shift ? shift_lo : g->shift, hi = shift_hi > g->shift ? shift_hi : g->shift;
    int s = clampi(g->shift - roundi((x - g->x) * (float)ncols), lo, hi);
    if (s != shift_) { shift_ = s; layout(); }
}

/*
 * A small side-to-side wobble of a resting finger, made bigger. The wobble is the finger's
 * distance from where it has been over the last ~0.12 s; while the finger slides that distance
 * is a lag, not a wobble, so it is let through less the faster the slide. A tanh keeps a big
 * wobble from going past the setting's depth.
 */
static void vibrato(Finger* f, float dt) {
    float dp_per_x = W;
    float depth = f->drum ? 0 : VIBRATO_DEPTH[clampi(iset(S_VIBRATO), 0, 3)];
    float v = (f->x - f->lastX) * dp_per_x / dt;
    f->lastX = f->x;
    f->drift += (v - f->drift) * (1 - expf_(-dt / 0.15f));
    float d = f->drift / VIB_SLIDE_DPS;
    float not_sliding = 1 / (1 + d * d * d * d);
    float a = 1 - expf_(-dt / 0.12f), b = 1 - not_sliding;
    f->centre += (f->x - f->centre) * (a > b ? a : b);
    if (depth == 0) { f->vib = 0; return; }
    float dev = (f->x - f->centre) * dp_per_x;
    float mag = absf(dev) - VIB_STILL_DP;
    float wobble = mag > 0 ? (dev < 0 ? -mag : mag) : 0;
    float gain = VIB_SEMIS_PER_DP * VIB_SENSE_GAIN[clampi(iset(S_VIBSENSE), 0, 3)];
    f->vib = depth * tanhf_(wobble * not_sliding * gain / VIBRATO_NORMAL);
}

/* Where to draw a finger across the pad: a drum on its column, otherwise where its pitch is. */
static float finger_x(const Finger* f) {
    if (f->drum) return ((float)f->col + 0.5f) / (float)ncols;
    return x_of(pitch_of(f));
}

/* ---- drawing ---------------------------------------------------------------------------------- */
static const char* NAMES[12] = {"C", "C\xE2\x99\xAF", "D", "E\xE2\x99\xAD", "E", "F", "F\xE2\x99\xAF", "G", "A\xE2\x99\xAD", "A",
                                "B\xE2\x99\xAD", "B"};

static void note_name(int n, char* out) {
    const char* s = drums() ? DRUM_NAMES[mod12(n)] : NAMES[mod12(n)];
    int k = 0;
    while (s[k]) { out[k] = s[k]; k++; }
    if (!drums()) {
        int o = n / 12 - 1;
        if (o < 0) { out[k++] = '-'; o = -o; }
        if (o >= 10) out[k++] = (char)('0' + o / 10);
        out[k++] = (char)('0' + o % 10);
    }
    out[k] = 0;
}

static int is_root(int n) { return drums() ? n % 12 == 0 : mod12(n - iset(S_KEY)) == 0; }

static void orb(float x, float y, float pressure, unsigned c) {
    const Theme* t = theme();
    float r = (60 + 70 * pressure) * PX;
    if (t->flat) {
        mesh_circle(x, y, r, WHITE, c, 6 * PX);
        mesh_circle(x, y, r * 0.3f, c, 0, 0);
        return;
    }
    mesh_glow(x, y, r * 1.8f, mesh_alpha(c, 0.55f), mesh_alpha(c, 0));
    mesh_circle(x, y, r, 0, mesh_alpha(c, 0.8f), 3 * PX);
    mesh_circle(x, y, r * 0.62f, mesh_alpha(t->bottom, 0.55f), 0, 0);
    mesh_circle(x, y, r * 0.24f, mesh_alpha(WHITE, 0.9f), 0, 0);
    mesh_circle(x, y, r * 0.17f, c, 0, 0);
}

static void draw(void) {
    const Theme* t = theme();
    float ph = playH(), cw = W / (float)ncols;
    unsigned c = t->part;
    mesh_begin();
    if (t->flat) mesh_clear(t->top);
    else {
        mesh_vgradient(0, 0, W, H / 2, t->top, t->mid);
        mesh_vgradient(0, H / 2, W, H - H / 2, t->mid, t->bottom);
    }
    /* Soft, Clear, Bold: line alpha, root line alpha, line width, root width, band alpha */
    float la = 0.45f, ra = 0.85f, lw = 3, rw = 5, ba = 0.05f;
    if (iset(S_LINES) == 0) { la = 0.22f; ra = 0.5f; lw = 2; rw = 3; ba = 0; }
    else if (iset(S_LINES) == 2) { la = 0.75f; ra = 1; lw = 4; rw = 7; ba = 0.10f; }
    int held[MAX_COLS] = {0};
    for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used) {
        Finger* f = &fingers[k];
        int i = f->drum ? f->col : column_at(ncols, x_of(pitch_of(f)));
        if (i >= 0 && i < ncols) held[i] = 1;
    }
    for (int i = 0; i < ncols; i++) {
        float x = ((float)i + 0.5f) * cw, x0 = (float)i * cw;
        /* Every other column shaded, so it is easy to see which one a finger is in. */
        if (i % 2 == 1 && ba > 0) mesh_rect(x0, 0, cw, ph, 0, mesh_alpha(t->line, ba), 0, 0);
        if (held[i]) {
            if (t->flat) mesh_rect(x0, 0, cw, ph, 0, 0xAAAAAAFFu, 0, 0);
            else {   /* a horizontal gradient, clear at the edges and 0.2 in the middle: nested bands */
                enum { BANDS = 8 };
                for (int s = 1; s <= BANDS; s++) {
                    float bw = cw * (float)s / BANDS;
                    mesh_rect(x - bw / 2, 0, bw, ph, 0, mesh_alpha(c, 0.0275f), 0, 0);   /* 1 - 0.8^(1/8) */
                }
            }
        }
        int root = is_root(columns[i]);
        unsigned lc = t->flat ? BLACK : mesh_alpha(root ? c : t->line, root ? ra : la);
        float width = t->flat ? (root ? rw + 2 : lw) : (root ? rw : lw);
        mesh_line(x, 0, x, ph, lc, width * PX);
    }
    /* The note strip: the notes' names; unlocked, a handle to drag them by. */
    float top = H - STRIP;
    int dragging = 0;
    for (int g = 0; g < MAX_GRABS; g++) if (grabs[g].used) dragging = 1;
    mesh_rect(0, top, W, STRIP, 0, dragging && !t->flat ? mesh_alpha(c, 0.25f) : t->strip, 0, 0);
    mesh_line(0, top, W, top, t->flat ? t->line : mesh_alpha(t->line, 0.5f), (t->flat ? 3 : 2) * PX);
    float ts = STRIP * 0.36f < cw * 0.42f ? STRIP * 0.36f : cw * 0.42f;
    for (int i = 0; i < ncols; i++) {
        float x = ((float)i + 0.5f) * cw;
        int root = is_root(columns[i]);
        if (held[i] && t->flat) mesh_rect((float)i * cw, top, cw, STRIP, 0, BLACK, 0, 0);
        if (i > 0) mesh_line((float)i * cw, top + STRIP * 0.2f, (float)i * cw, H - STRIP * 0.2f,
                             mesh_alpha(t->label, t->flat ? 0.6f : 0.18f), 2 * PX);
        unsigned tc = held[i] && t->flat ? WHITE : t->flat ? BLACK : root ? c : mesh_alpha(t->label, 0.85f);
        char name[16];
        note_name(columns[i], name);
        mesh_text(x, top + STRIP * 0.62f, ts, tc, MESH_TEXT_CENTER | (root ? MESH_TEXT_BOLD : 0), name);
    }
    if (set_[S_LOCKED] == 0 && !drums()) {   /* grip marks at the ends while the strip can be dragged */
        unsigned gc = mesh_alpha(t->label, t->flat ? 1 : 0.45f);
        for (int k = 0; k < 3; k++) {
            float y = top + STRIP * (0.38f + 0.12f * (float)k);
            mesh_line(8 * PX, y, 26 * PX, y, gc, 2 * PX);
            mesh_line(W - 26 * PX, y, W - 8 * PX, y, gc, 2 * PX);
        }
    }
    /* Ripples where fingers went down. */
    if (!t->flat) for (int r = 0; r < MAX_RIPPLES; r++) if (ripples[r].age > 0 && ripples[r].age <= 0.7f) {
        float k = ripples[r].age / 0.7f;
        mesh_circle(ripples[r].x * W, (1 - ripples[r].y) * ph, (40 + 220 * k) * PX, 0, mesh_alpha(c, 0.5f * (1 - k)),
                    (4 * (1 - k) + 1) * PX);
    }
    /* Each finger: its trail, then its orb, as big as it is loud. */
    for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used) {
        Finger* f = &fingers[k];
        for (int q = 0; q < f->tn; q++) {
            float a = (float)(q + 1) / (float)f->tn;
            mesh_circle(f->trail[2 * q] * W, (1 - f->trail[2 * q + 1]) * ph, (8 + 26 * a) * PX, mesh_alpha(c, 0.22f * a), 0, 0);
        }
        float p, sl, vol;
        expression(f, &p, &sl, &vol);
        orb(finger_x(f) * W, (1 - f->y) * ph, p, c);
    }
    mesh_end();
    dirty = 0;
}

/* ---- the interface ------------------------------------------------------------------------------- */
MESH_EXPORT(init) void init(float width, float height, int flags, double t) {
    W = width > 1 ? width : 1; H = height > 1 ? height : 1; eink = flags & MESH_EINK; now = t;
    layout();
    draw();
}

MESH_EXPORT(instrument) void instrument(int part, float center_note) {
    if (part == 0 && center_note != center) { center = center_note; layout(); }
}

MESH_EXPORT(setting) void setting(int index, double value) {
    if (index < 0 || index >= S_COUNT || set_[index] == value) return;
    set_[index] = value;
    /* New notes: lay the columns out again (the strip keeps its shift, as on Android). */
    if (index == S_SIZE || index == S_OCTAVE || index == S_KEY || index == S_SCALE || index == S_DRUMS) layout();
    dirty = 1;
}

MESH_EXPORT(touch) void touch(int id, int phase, float x, float y, float size, float force, double t) {
    (void)size; (void)force;
    now = t;
    float fx = x / W, fy = 1 - y / playH();
    if (phase == MESH_DOWN) {
        Finger* old = find(id);
        if (old) lift(old);
        if (y > H - STRIP) strip_down(id, fx, fy);
        else down(id, fx, fy);
    } else if (phase == MESH_MOVE) {
        Grab* g = find_grab(id);
        if (g) { grab_move(g, fx); return; }
        Finger* f = find(id);
        if (!f) return;
        f->x = fx; f->y = clampf(fy, 0, 1);
        place(f);
        send_expression(f);
        dirty = 1;
    } else {
        Grab* g = find_grab(id);
        if (g) { g->used = 0; dirty = 1; return; }
        Finger* f = find(id);
        if (f) lift(f);
    }
}

MESH_EXPORT(tick) void tick(double t) {
    float dt = (float)((t - now) / 1000.0);
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    now = t;
    int moving = 0;
    for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used) {
        Finger* f = &fingers[k];
        if (dt > 0) vibrato(f, dt);
        place(f);
        send_expression(f);
        if (!eink) {
            if (f->tn == TRAIL) {
                for (int q = 0; q < (TRAIL - 1) * 2; q++) f->trail[q] = f->trail[q + 2];
                f->tn--;
            }
            f->trail[2 * f->tn] = finger_x(f); f->trail[2 * f->tn + 1] = f->y;
            f->tn++;
            moving = 1;
        }
    }
    for (int r = 0; r < MAX_RIPPLES; r++) if (ripples[r].age > 0) {
        ripples[r].age += dt;
        if (ripples[r].age > 0.7f) ripples[r].age = 0;
        moving = 1;
    }
    if (dirty || moving) draw();
}

MESH_EXPORT(stop) void stop(void) {
    for (int k = 0; k < MAX_FINGERS; k++) if (fingers[k].used) lift(&fingers[k]);
    for (int g = 0; g < MAX_GRABS; g++) grabs[g].used = 0;
    dirty = 1;
}
