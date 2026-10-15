// Layered background: gradient sky, stars, celestial body, far / mid
// silhouettes, fog and floating motes. Layers can parallax via move triggers
// locked to the player X with different X mods.

#include "Ctx.hpp"

namespace inspo {

namespace {
constexpr int Z_SKY = -100, Z_STAR = -97, Z_CEL = -95, Z_FAR = -90, Z_MID = -80, Z_FOG = -70, Z_NEAR = -60, Z_MOTE = 25;
constexpr double PLAYER_SPEED_1X = 311.58; // units per second at normal speed

double noise1(double x, double const* ph, int oct) {
    double v = 0, amp = 1, f = 1, norm = 0;
    for (int i = 0; i < oct; i++) {
        v += std::sin(x * 0.011 * f + ph[i]) * amp;
        norm += amp;
        amp *= 0.5;
        f *= 2.13;
    }
    return v / norm; // -1..1
}
} // namespace

void genBackground(Ctx& c, double x0, double x1, double baseY, double trigX) {
    auto& r = c.rng;
    auto& d = c.d;
    auto& a = c.a;
    auto const& pal = c.pal;

    if (c.s.setLevelColors) {
        a.colorCycle(gd::ch::BG, pal.bg, pal.bgAlt, r.f(5, 8));
        a.atStart(Anim::color(gd::ch::GROUND, pal.ground, 0));
        a.atStart(Anim::color(gd::ch::GROUND2, mix(pal.ground, pal.bgAlt, 0.4), 0));
        a.atStart(Anim::color(gd::ch::LINE, pal.line, 0, 1, true));
    }

    bool parallax = c.s.parallax && c.anim(1);
    double pTrig = x0 - 18 * U;
    double L = (parallax ? pTrig - 8 * U : x0 - 12 * U), R = x1 + 14 * U;
    double W = R - L, mx = (L + R) / 2;
    double gy = baseY;

    int gSky = c.ga.get(), gCel = c.ga.get(), gFar = c.ga.get(), gMid = c.ga.get(), gFog = c.ga.get();

    // ---- sky gradient with dithered transitions
    struct Band { Role role; double y0, y1; };
    Band bands[4] = {{SKY4, gy - 2 * U, gy + 3.5 * U}, {SKY3, gy + 3.5 * U, gy + 7.5 * U},
                     {SKY2, gy + 7.5 * U, gy + 12 * U}, {SKY1, gy + 12 * U, gy + 24 * U}};
    Groups sky{gSky};
    for (auto const& b : bands) d.rect(mx, (b.y0 + b.y1) / 2, W, b.y1 - b.y0 + 1, 0, c.S(b.role, Z_SKY, sky));
    for (int i = 0; i < 3; i++) {
        double yb = bands[i].y1;
        for (int k = 0; k < 3; k++) {
            double h = 0.22 * U * (1.0 - k * 0.3);
            d.rect(mx, yb + (k * 0.5 + 0.3) * U, W, h, 0, c.S(bands[i].role, Z_SKY + 1, sky));
            d.rect(mx, yb - (k * 0.5 + 0.3) * U, W, h, 0, c.S(bands[i + 1].role, Z_SKY + 1, sky));
        }
    }

    // ---- stars
    int tw[3] = {c.ga.get(), c.ga.get(), c.ga.get()};
    int stars = 25 * c.s.detail + (int)(W / U);
    for (int i = 0; i < stars; i++) {
        V p{r.f(L, R), gy + r.f(5, 22) * U};
        double s = r.chance(0.12) ? r.f(3, 5) : r.f(1.2, 2.6);
        Groups g{gCel};
        if (r.chance(0.4)) g.push_back(tw[r.i(0, 2)]);
        d.diamond(p, s, c.S(PARTICLE, Z_STAR, g), r.chance(0.5) ? 45 : 0);
        if (s > 3 && c.s.detail >= 2) {
            d.rect(p.x, p.y, s * 3.5, 0.8, 0, c.S(PARTICLE, Z_STAR, g));
            d.rect(p.x, p.y, 0.8, s * 3.5, 0, c.S(PARTICLE, Z_STAR, g));
        }
    }
    if (c.anim(2))
        for (int k = 0; k < 3; k++) a.flicker(tw[k], 0.15, 1, r.f(1.5, 3), k * 0.7);

    // ---- celestial body
    {
        double side = r.chance(0.5) ? r.f(0.05, 0.25) : r.f(0.75, 0.95);
        V p{x0 + (x1 - x0) * side, gy + r.f(11.5, 14) * U};
        double cr = r.f(1.1, 1.7) * U;
        Groups g{gCel};
        d.glow(p, cr * 3.2, c.S(GLOW_SOFT, Z_CEL - 1, g), 4);
        d.circle(p, cr, c.S(CELESTIAL, Z_CEL, g), c.strip());
        d.ring(p, cr * 1.35, 1.6, c.S(CELESTIAL, Z_CEL, g), 36);
        if (r.chance(0.45)) {
            d.circle(p + V{cr * 0.42, cr * 0.18}, cr * 0.92, c.S(SKY2, Z_CEL + 1, g), c.strip());
        } else {
            for (int k = 0; k < 4; k++) {
                double ang = r.f(0, 2 * PI), dist = r.f(0, 0.6) * cr;
                d.circle(p + V{std::cos(ang) * dist, std::sin(ang) * dist}, r.f(0.12, 0.25) * cr, c.S(SKY4, Z_CEL + 1, g), 3);
            }
        }
    }

    // ---- far silhouettes
    double ph[4] = {r.f(0, 9), r.f(0, 9), r.f(0, 9), r.f(0, 9)};
    if (r.chance(0.5)) {
        Pts ridge{{L, gy - U}};
        for (double x = L; x <= R; x += r.f(0.5, 1.0) * U) {
            double h = (3.6 + 2.8 * noise1(x, ph, 4)) * U + r.f(-0.15, 0.15) * U;
            ridge.push_back({x, gy + h});
        }
        ridge.push_back({R, gy + 3 * U});
        ridge.push_back({R, gy - U});
        d.fill(ridge, c.S(FAR_SIL, Z_FAR, {gFar}), 8);
        Pts top(ridge.begin() + 1, ridge.end() - 2);
        d.polyline(top, 1.6, c.S(SKY4, Z_FAR + 1, {gFar}));
    } else {
        double x = L;
        while (x < R) {
            double bw = r.f(1.0, 2.6) * U;
            double env = 0.6 + 0.4 * noise1(x, ph, 2);
            double bh = r.f(2, 7.5) * U * env + U;
            d.rect(x + bw / 2, gy - U + bh / 2, bw, bh + 2 * U, 0, c.S(FAR_SIL, Z_FAR, {gFar}));
            d.line({x, gy + bh - U}, {x + bw, gy + bh - U}, 1.3, c.S(SKY4, Z_FAR + 1, {gFar}));
            if (r.chance(0.3)) d.line({x + bw * 0.5, gy + bh - U}, {x + bw * 0.5, gy + bh}, 1.3, c.S(FAR_SIL, Z_FAR, {gFar}));
            if (c.s.detail >= 2) {
                int nw = r.i(2, 7);
                for (int k = 0; k < nw; k++)
                    d.rect(x + r.f(0.2, 0.8) * bw, gy + r.f(0, bh - 1.4 * U), 2.5, 3.5, 0, c.S(SKY4, Z_FAR + 1, {gFar}));
            }
            x += bw + r.f(0, 0.25) * U;
        }
    }

    // ---- mid silhouettes
    {
        double ph2[4] = {r.f(0, 9), r.f(0, 9), r.f(0, 9), r.f(0, 9)};
        Pts hill{{L, gy - U}};
        Pts top;
        for (double x = L; x <= R; x += 0.5 * U) {
            double h = (1.8 + 1.3 * noise1(x * 1.4, ph2, 3)) * U;
            hill.push_back({x, gy + h});
            top.push_back({x, gy + h});
        }
        hill.push_back({R, gy - U});
        d.fill(hill, c.S(MID_SIL, Z_MID, {gMid}), 7);
        d.polyline(top, 2.2, c.S(GLOW_SOFT, Z_MID + 1, {gMid}));
        if (r.chance(0.6)) {
            for (size_t i = 1; i < top.size(); i += r.i(1, 3)) {
                if (!r.chance(0.45)) continue;
                double th = r.f(0.7, 1.6) * U, tw2 = th * 0.42;
                V b = top[i];
                for (int k = 0; k < 3; k++) {
                    double yy = b.y - 3 + k * th * 0.28, ww = tw2 * (1 - k * 0.25);
                    d.fill({{b.x - ww, yy}, {b.x, yy + th * 0.5}, {b.x + ww, yy}}, c.S(MID_SIL, Z_MID, {gMid}), 6);
                }
            }
        } else {
            for (int k = 0; k < (int)(W / (6 * U)); k++) {
                double sx = r.f(L, R), sh = r.f(2, 4) * U, sw = r.f(0.35, 0.6) * U;
                d.fill({{sx - sw, gy}, {sx, gy + sh + 1.5 * U}, {sx + sw, gy}}, c.S(MID_SIL, Z_MID, {gMid}), 6);
                d.diamond({sx, gy + sh + 1.6 * U}, 0.14 * U, c.S(GLOW, Z_MID + 1, {gMid}));
            }
        }
    }

    // ---- near ground rubble (static)
    {
        Pts g{{L, gy - U}};
        for (double x = L; x <= R; x += r.f(0.3, 0.7) * U) g.push_back({x, gy + r.f(0.05, 0.7) * U});
        g.push_back({R, gy - U});
        d.fill(g, c.S(NEAR_SIL, Z_NEAR, {}), 6);
        if (c.s.detail >= 2)
            for (double x = L; x < R; x += r.f(0.4, 1.2) * U)
                d.spike({x, gy + 0.55 * U}, r.f(-15, 15), r.f(0.3, 0.6), c.S(NEAR_SIL, Z_NEAR, {}), true);
    }

    // ---- fog
    // fog banks: stacked bands of shrinking width so the ends fade out
    for (int k = 0; k < 3 + c.s.detail; k++) {
        double fw = r.f(0.2, 0.4) * W, fx = r.f(L + fw / 2, R - fw / 2), fy = gy + r.f(0.5, 3.5) * U;
        double fh = r.f(0.8, 1.6) * U;
        for (int j = 0; j < 4; j++)
            d.rect(fx + r.f(-0.05, 0.05) * fw, fy, fw * (1 - j * 0.22), fh * (1 - j * 0.2), 0, c.S(FOG, Z_FOG, {gFog}));
    }
    if (c.anim(1)) a.bob(gFog, 3 * U, 0, r.f(10, 14));

    // ---- motes
    if (c.anim(2)) {
        int mg[4];
        for (auto& g : mg) g = c.ga.get();
        int motes = 10 * c.s.detail + (int)(W / (2 * U));
        for (int i = 0; i < motes; i++) {
            V p{r.f(x0 - 6 * U, x1 + 6 * U), gy + r.f(0.3, 8) * U};
            d.diamond(p, r.f(2.2, 4.2), c.S(PARTICLE, Z_MOTE, {mg[i % 4]}), r.f(0, 90));
        }
        for (int k = 0; k < 4; k++) a.rise(mg[k], r.f(-1, 1) * U, r.f(2, 3.5) * U, r.f(4.5, 7), k * 1.4);
    }

    // ---- parallax
    if (parallax) {
        double dur = std::max(5.0, (x1 + 15 * U - pTrig) / PLAYER_SPEED_1X);
        double ty = baseY + 18 * U;
        a.atX(pTrig, ty, Anim::parallax(gSky, 1.0, dur));
        a.atX(pTrig, ty + U, Anim::parallax(gCel, 0.96, dur));
        a.atX(pTrig, ty + 2 * U, Anim::parallax(gFar, 0.82, dur));
        a.atX(pTrig, ty + 3 * U, Anim::parallax(gMid, 0.6, dur));
    }
    (void)trigX;
}

} // namespace inspo
