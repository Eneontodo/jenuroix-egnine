// ============================================================
//  Example 22 — CPU Particle System
//
//  A lightweight, fully self-contained particle emitter:
//    - Fire / explosion / sparkle effects
//    - Particles spawned each frame, destroyed on expiry
//    - Color, size, and velocity controlled via config structs
//
//  Controls:
//    1  — Fire emitter (continuous)
//    2  — Explosion burst (one-shot)
//    3  — Sparkle fountain
//    4  — Snow shower
//    Space — pause / resume all emitters
//    Tab   — free-cam
// ============================================================
#include "engine.h"
#include <vector>
#include <cmath>
#include <cstdlib>
using namespace eng;

// ── Single particle ───────────────────────────────────────────────────────
struct Particle {
    GameObject  obj;
    glm::vec3   vel;
    float       life;     // seconds remaining
    float       maxLife;
    glm::vec3   colorStart;
    glm::vec3   colorEnd;
};

// ── Emitter configuration ─────────────────────────────────────────────────
struct EmitterCfg {
    glm::vec3 origin     = {0, 0, 0};
    glm::vec3 velMin     = {-1, 2, -1};
    glm::vec3 velMax     = { 1, 5,  1};
    glm::vec3 colorStart = {1, 0.5f, 0};
    glm::vec3 colorEnd   = {0.2f, 0.05f, 0};
    float     lifeMin    = 0.5f;
    float     lifeMax    = 1.5f;
    float     spawnRate  = 30.f;   // particles per second (0 = burst only)
    int       burstCount = 0;      // spawn N at once then stop
    float     gravity    = -4.f;
    float     sizeStart  = 0.12f;
    float     sizeEnd    = 0.02f;
    bool      active     = true;
};

// ── Particle manager ──────────────────────────────────────────────────────
class ParticleSystem {
public:
    std::vector<Particle> particles;
    float spawnAcc = 0.f;

    void burst(App& app, const EmitterCfg& cfg) {
        int n = (cfg.burstCount > 0) ? cfg.burstCount : 1;
        for (int i = 0; i < n; ++i) spawn(app, cfg);
    }

    void update(App& app, const EmitterCfg& cfg, float dt) {
        // Continuous spawning
        if (cfg.active && cfg.spawnRate > 0.f) {
            spawnAcc += cfg.spawnRate * dt;
            while (spawnAcc >= 1.f) {
                spawn(app, cfg);
                spawnAcc -= 1.f;
            }
        }

        // Update existing particles
        for (int i = (int)particles.size() - 1; i >= 0; --i) {
            auto& p = particles[i];
            if (!p.obj.valid()) { particles.erase(particles.begin() + i); continue; }

            p.life -= dt;
            if (p.life <= 0.f) {
                p.obj.destroy();
                particles.erase(particles.begin() + i);
                continue;
            }

            // Integrate velocity with gravity
            p.vel.y += cfg.gravity * dt;
            p.obj.move(p.vel * dt);

            // Interpolate color and size by remaining life fraction
            float t   = 1.f - p.life / p.maxLife;
            glm::vec3 col = p.colorStart + (p.colorEnd - p.colorStart) * t;
            float sz  = p.obj.scale().x; // current size (single uniform scale)
            float targetSz = cfg.sizeStart + (cfg.sizeEnd - cfg.sizeStart) * t;
            p.obj.scale(targetSz, targetSz, targetSz);
            p.obj.color(col.r, col.g, col.b);
        }
    }

private:
    float randf(float lo, float hi) {
        return lo + ((float)rand() / RAND_MAX) * (hi - lo);
    }

    void spawn(App& app, const EmitterCfg& cfg) {
        Particle p;
        float life  = randf(cfg.lifeMin, cfg.lifeMax);
        p.life      = life;
        p.maxLife   = life;
        p.vel       = {randf(cfg.velMin.x, cfg.velMax.x),
                       randf(cfg.velMin.y, cfg.velMax.y),
                       randf(cfg.velMin.z, cfg.velMax.z)};
        p.colorStart = cfg.colorStart;
        p.colorEnd   = cfg.colorEnd;
        p.obj = app.spawn()
            .sphere(cfg.sizeStart, 4) // low-poly sphere for performance
            .color(cfg.colorStart.r, cfg.colorStart.g, cfg.colorStart.b)
            .pos(cfg.origin.x + randf(-0.2f, 0.2f),
                 cfg.origin.y,
                 cfg.origin.z + randf(-0.2f, 0.2f));
        particles.push_back(p);
    }
};

int main() {
    App app("Particle System", 1280, 720);
    app.skyColor(0.03f, 0.03f, 0.07f)
       .ambient(0.08f, 0.08f, 0.12f)
       .lightDir(0.3f, 1.f, 0.5f);
    app.camPos(0, 4, 12).camRot(-90.f, -15.f).freeCam(true);

    app.spawn("Ground").grid(20, 20, 1.f).color(0.1f, 0.1f, 0.14f).pos(0, -0.5f, 0);

    // ── Emitter presets ───────────────────────────────────────────────────
    EmitterCfg fireCfg;
    fireCfg.origin     = {-4, 0, 0};
    fireCfg.velMin     = {-0.5f, 2.f, -0.5f};
    fireCfg.velMax     = { 0.5f, 5.f,  0.5f};
    fireCfg.colorStart = {1.0f, 0.55f, 0.05f};
    fireCfg.colorEnd   = {0.6f, 0.05f, 0.02f};
    fireCfg.lifeMin    = 0.4f;  fireCfg.lifeMax  = 1.0f;
    fireCfg.spawnRate  = 40.f;  fireCfg.gravity  = -1.f;
    fireCfg.sizeStart  = 0.15f; fireCfg.sizeEnd  = 0.01f;

    EmitterCfg snowCfg;
    snowCfg.origin     = {4, 8, 0};
    snowCfg.velMin     = {-1.5f, -2.f, -1.5f};
    snowCfg.velMax     = { 1.5f, -0.5f, 1.5f};
    snowCfg.colorStart = {0.9f, 0.95f, 1.f};
    snowCfg.colorEnd   = {0.5f, 0.6f, 0.8f};
    snowCfg.lifeMin    = 2.f;   snowCfg.lifeMax  = 4.f;
    snowCfg.spawnRate  = 20.f;  snowCfg.gravity  = -0.5f;
    snowCfg.sizeStart  = 0.07f; snowCfg.sizeEnd  = 0.05f;

    EmitterCfg sparkleCfg;
    sparkleCfg.origin     = {0, 0, 0};
    sparkleCfg.velMin     = {-3, 3, -3};
    sparkleCfg.velMax     = { 3, 8,  3};
    sparkleCfg.colorStart = {1.f, 0.9f, 0.2f};
    sparkleCfg.colorEnd   = {0.2f, 0.1f, 0.8f};
    sparkleCfg.lifeMin    = 0.8f;  sparkleCfg.lifeMax  = 2.0f;
    sparkleCfg.spawnRate  = 0.f;   sparkleCfg.gravity  = -8.f;
    sparkleCfg.burstCount = 80;
    sparkleCfg.sizeStart  = 0.12f; sparkleCfg.sizeEnd  = 0.02f;

    ParticleSystem fire, snow, sparkle, explosion;
    bool paused = false;

    // Place visual source markers
    app.spawn("FireSrc").sphere(0.18f).color(1,0.4f,0).pos(fireCfg.origin);
    app.spawn("SnowSrc").sphere(0.12f).color(0.8f,0.9f,1).pos(snowCfg.origin);
    app.spawn("SpkSrc") .sphere(0.15f).color(1,0.9f,0.1f).pos(sparkleCfg.origin);

    app.onUpdate([&](float dt) {
        if (app.keyDown(KEY_SPACE)) paused = !paused;
        if (paused) return;

        // Toggle emitters
        if (app.keyDown(GLFW_KEY_1)) fireCfg.active    = !fireCfg.active;
        if (app.keyDown(GLFW_KEY_3)) snowCfg.active    = !snowCfg.active;
        if (app.keyDown(GLFW_KEY_2)) sparkle.burst(app, sparkleCfg);
        if (app.keyDown(GLFW_KEY_4)) {
            EmitterCfg ex = sparkleCfg;
            ex.colorStart = {1.f,0.6f,0.1f}; ex.colorEnd = {0.4f,0.05f,0.0f};
            ex.burstCount = 150; ex.gravity = -14.f;
            explosion.burst(app, ex);
        }

        fire.update(app, fireCfg, dt);
        snow.update(app, snowCfg, dt);
        sparkle.update(app, sparkleCfg, dt);
        explosion.update(app, sparkleCfg, dt);
    });

    LOG_INFO("Particle System — 1=fire  2=sparkle burst  3=snow  4=explosion  Space=pause  Tab=cam");

    return app.run();
}
