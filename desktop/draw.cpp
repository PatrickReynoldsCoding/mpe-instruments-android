#include "draw.h"
#include <cmath>
#include <cstring>

namespace draw {

Color rgba(uint32_t c) {
    return {float((c >> 24) & 255) / 255.f, float((c >> 16) & 255) / 255.f, float((c >> 8) & 255) / 255.f, float(c & 255) / 255.f};
}

static SDL_FColor fc(Color c) { return {c.r, c.g, c.b, c.a}; }

static void tris(SDL_Renderer* r, const std::vector<SDL_Vertex>& v) {
    if (!v.empty()) SDL_RenderGeometry(r, nullptr, v.data(), int(v.size()), nullptr, 0);
}

static int segments(float rad) { return std::max(8, std::min(64, int(rad * 0.8f))); }

// A convex outline as a fan of triangles around its first point.
static void fan(SDL_Renderer* r, const std::vector<SDL_FPoint>& pts, Color c) {
    if (pts.size() < 3 || c.a <= 0) return;
    std::vector<SDL_Vertex> v;
    for (size_t i = 1; i + 1 < pts.size(); i++)
        for (SDL_FPoint p : {pts[0], pts[i], pts[i + 1]}) v.push_back({p, fc(c), {0, 0}});
    tris(r, v);
}

static void polyline(SDL_Renderer* r, const std::vector<SDL_FPoint>& pts, bool closed, Color c, float width) {
    if (c.a <= 0 || width <= 0) return;
    size_t n = pts.size();
    for (size_t i = 0; i + (closed ? 0 : 1) < n; i++) {
        SDL_FPoint a = pts[i], b = pts[(i + 1) % n];
        line(r, a.x, a.y, b.x, b.y, c, width);
    }
}

static std::vector<SDL_FPoint> roundedRect(float x, float y, float w, float h, float rad) {
    std::vector<SDL_FPoint> p;
    rad = std::min(rad, std::min(w, h) / 2);
    if (rad <= 0.5f) return {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
    const float cx[4] = {x + w - rad, x + w - rad, x + rad, x + rad}, cy[4] = {y + rad, y + h - rad, y + h - rad, y + rad};
    const int seg = 6;
    for (int k = 0; k < 4; k++)
        for (int s = 0; s <= seg; s++) {
            float a = float(M_PI) * (-0.5f + 0.5f * float(k) + 0.5f * float(s) / seg);
            p.push_back({cx[k] + rad * std::cos(a), cy[k] + rad * std::sin(a)});
        }
    return p;
}

void fillRect(SDL_Renderer* r, float x, float y, float w, float h, Color c, float radius) { fan(r, roundedRect(x, y, w, h, radius), c); }
void strokeRect(SDL_Renderer* r, float x, float y, float w, float h, Color c, float width, float radius) {
    polyline(r, roundedRect(x, y, w, h, radius), true, c, width);
}

void line(SDL_Renderer* r, float x1, float y1, float x2, float y2, Color c, float width) {
    float dx = x2 - x1, dy = y2 - y1, len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-4f || c.a <= 0) return;
    float nx = -dy / len * width / 2, ny = dx / len * width / 2;
    SDL_FPoint a{x1 + nx, y1 + ny}, b{x2 + nx, y2 + ny}, cc{x2 - nx, y2 - ny}, d{x1 - nx, y1 - ny};
    std::vector<SDL_Vertex> v;
    for (SDL_FPoint p : {a, b, cc, a, cc, d}) v.push_back({p, fc(c), {0, 0}});
    tris(r, v);
}

static std::vector<SDL_FPoint> ring(float cx, float cy, float rad) {
    std::vector<SDL_FPoint> p;
    int n = segments(rad);
    for (int k = 0; k < n; k++) {
        float a = 2 * float(M_PI) * float(k) / float(n);
        p.push_back({cx + rad * std::cos(a), cy + rad * std::sin(a)});
    }
    return p;
}

void circle(SDL_Renderer* r, float cx, float cy, float rad, Color fill, Color stroke, float width) {
    auto p = ring(cx, cy, rad);
    fan(r, p, fill);
    polyline(r, p, true, stroke, width);
}

static void glow(SDL_Renderer* r, float cx, float cy, float rad, Color inner, Color outer) {
    auto p = ring(cx, cy, rad);
    std::vector<SDL_Vertex> v;
    for (size_t i = 0; i < p.size(); i++) {
        v.push_back({{cx, cy}, fc(inner), {0, 0}});
        v.push_back({p[i], fc(outer), {0, 0}});
        v.push_back({p[(i + 1) % p.size()], fc(outer), {0, 0}});
    }
    tris(r, v);
}

static void vgradient(SDL_Renderer* r, float x, float y, float w, float h, Color top, Color bottom) {
    SDL_Vertex a{{x, y}, fc(top), {0, 0}}, b{{x + w, y}, fc(top), {0, 0}}, c{{x + w, y + h}, fc(bottom), {0, 0}},
        d{{x, y + h}, fc(bottom), {0, 0}};
    tris(r, {a, b, c, a, c, d});
}

// SDL's debug font is ASCII: sharps and flats become # and b.
static std::string ascii(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { out += char(c); i++; continue; }
        if (!s.compare(i, 3, "\xE2\x99\xAF")) { out += '#'; i += 3; continue; }
        if (!s.compare(i, 3, "\xE2\x99\xAD")) { out += 'b'; i += 3; continue; }
        if (!s.compare(i, 3, "\xE2\x88\x92")) { out += '-'; i += 3; continue; }
        out += '?';
        i++;
        while (i < s.size() && ((unsigned char)s[i] & 0xC0) == 0x80) i++;
    }
    return out;
}

float textWidth(float size, const std::string& utf8) { return float(ascii(utf8).size()) * size; }

void text(SDL_Renderer* r, float x, float y, float size, Color c, const std::string& utf8, int align, bool bold) {
    std::string s = ascii(utf8);
    if (s.empty() || c.a <= 0) return;
    float scale = size / 8.f, w = float(s.size()) * size;
    if (align == 1) x -= w / 2;
    else if (align == 2) x -= w;
    y -= size;   // y is the baseline
    // The debug font is scaled with the render scale, which also scales the viewport's place:
    // so draw without the viewport, moved by its offset, and put both back after.
    const bool hasViewport = SDL_RenderViewportSet(r);
    SDL_Rect vp{0, 0, 0, 0};
    if (hasViewport) SDL_GetRenderViewport(r, &vp);
    x += float(vp.x);
    y += float(vp.y);
    float sx, sy;
    SDL_GetRenderScale(r, &sx, &sy);
    SDL_SetRenderViewport(r, nullptr);
    SDL_SetRenderScale(r, sx * scale, sy * scale);
    // Kept inside the viewport it was meant for (a side of a split), as the shapes are.
    if (hasViewport) {
        SDL_Rect clip{int(std::floor(float(vp.x) / scale)), int(std::floor(float(vp.y) / scale)),
                      int(std::ceil(float(vp.w) / scale)), int(std::ceil(float(vp.h) / scale))};
        SDL_SetRenderClipRect(r, &clip);
    }
    SDL_SetRenderDrawColorFloat(r, c.r, c.g, c.b, c.a);
    SDL_RenderDebugText(r, x / scale, y / scale, s.c_str());
    if (bold) SDL_RenderDebugText(r, x / scale + 0.6f, y / scale, s.c_str());
    if (hasViewport) SDL_SetRenderClipRect(r, nullptr);
    SDL_SetRenderScale(r, sx, sy);
    if (hasViewport) SDL_SetRenderViewport(r, &vp);
}

// ---- a controller's draw list ------------------------------------------------------------------

namespace {
struct Reader {
    const uint8_t* p;
    const uint8_t* end;
    bool ok() const { return p + 4 <= end; }
    uint32_t u() { uint32_t v = 0; if (p + 4 <= end) std::memcpy(&v, p, 4); p += 4; return v; }
    float f() { uint32_t v = u(); float x; std::memcpy(&x, &v, 4); return x; }
};
}  // namespace

void list(SDL_Renderer* r, const std::vector<uint8_t>& bytes, float w, float h) {
    Reader in{bytes.data(), bytes.data() + bytes.size()};
    while (in.ok()) {
        switch (in.u()) {
            case 1: fillRect(r, 0, 0, w, h, rgba(in.u())); break;
            case 2: {
                float x = in.f(), y = in.f(), ww = in.f(), hh = in.f(), rad = in.f();
                Color fill = rgba(in.u()), stroke = rgba(in.u());
                float sw = in.f();
                fillRect(r, x, y, ww, hh, fill, rad);
                if (sw > 0) strokeRect(r, x, y, ww, hh, stroke, sw, rad);
                break;
            }
            case 3: {
                float x = in.f(), y = in.f(), rad = in.f();
                Color fill = rgba(in.u()), stroke = rgba(in.u());
                circle(r, x, y, rad, fill, stroke, in.f());
                break;
            }
            case 4: {
                float x1 = in.f(), y1 = in.f(), x2 = in.f(), y2 = in.f();
                Color c = rgba(in.u());
                line(r, x1, y1, x2, y2, c, in.f());
                break;
            }
            case 5: {
                uint32_t n = in.u();
                Color fill = rgba(in.u()), stroke = rgba(in.u());
                float sw = in.f();
                if (n > 4096) return;
                std::vector<SDL_FPoint> pts(n);
                for (auto& p : pts) { p.x = in.f(); p.y = in.f(); }
                fan(r, pts, fill);
                if (sw > 0) polyline(r, pts, true, stroke, sw);
                break;
            }
            case 6: {
                float x = in.f(), y = in.f(), size = in.f();
                Color c = rgba(in.u());
                uint32_t flags = in.u(), n = in.u();
                if (n > 4096 || in.p + ((n + 3) & ~3u) > in.end) return;
                std::string s(reinterpret_cast<const char*>(in.p), n);
                in.p += (n + 3) & ~3u;
                text(r, x, y, size, c, s, int(flags & 3), (flags & 4) != 0);
                break;
            }
            case 7: {
                float x = in.f(), y = in.f(), ww = in.f(), hh = in.f();
                Color top = rgba(in.u());
                vgradient(r, x, y, ww, hh, top, rgba(in.u()));
                break;
            }
            case 8: {
                float x = in.f(), y = in.f(), rad = in.f();
                Color inner = rgba(in.u());
                glow(r, x, y, rad, inner, rgba(in.u()));
                break;
            }
            default: return;   // an operation this MESH doesn't know: stop rather than misread the rest
        }
    }
}

}  // namespace draw
