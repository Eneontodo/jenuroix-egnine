// ============================================================
//  Example 21 — Day / Night Cycle
//
//  Demonstrates driving scene lighting from a time-of-day value:
//    - Sky color lerps through dawn → day → dusk → night
//    - Sun and moon spheres orbit the scene
//    - Directional light direction and color follow the sun
//    - Ambient light darkens at night
//
//  Controls:
//    Left / Right  — rewind / fast-forward time
//    Space         — pause / resume
//    Tab           — free-cam
// ============================================================
#include "engine.h"
#include <cmath>
using namespace eng;

// Linearly interpolate between two colors given t in [0,1]
static glm::vec3 lerpCol(glm::vec3 a, glm::vec3 b, float t) {
    return a + (b - a) * t;
}

int main() {
    App app("Day / Night Cycle", 1280, 720);
    app.freeCam(true);
    app.camPos(0, 4, 12).camRot(-90.f, -15.f);

    // ── Scene ─────────────────────────────────────────────────────────────
    // Ground
    app.spawn("Ground").grid(30, 30, 1.f)
       .color(0.22f, 0.35f, 0.18f).pos(0, -0.5f, 0);

    // Some static objects to receive dynamic lighting
    for (int i = 0; i < 5; ++i) {
        float a = i * 1.2566f;
        app.spawn("Building_" + std::to_string(i))
           .cube().scale(1.2f, 2.f + i * 0.5f, 1.2f)
           .color(0.55f, 0.52f, 0.48f)
           .pos(std::cos(a) * 5.f, 1.f + i * 0.25f, std::sin(a) * 5.f);
    }

    // Sun visual marker (bright yellow sphere)
    auto sun  = app.spawn("Sun").sphere(0.6f).color(1.f, 0.95f, 0.5f);
    // Moon visual marker (cool white sphere, smaller)
    auto moon = app.spawn("Moon").sphere(0.35f).color(0.8f, 0.85f, 1.f);

    // ── Time state ────────────────────────────────────────────────────────
    float dayTime  = 0.25f;  // 0=midnight, 0.25=sunrise, 0.5=noon, 0.75=sunset
    float daySpeed = 0.03f;  // full cycle in ~33 seconds (adjust freely)
    bool  paused   = false;

    // Sky color key-frames: midnight, dawn, morning, noon, afternoon, dusk, night
    struct SkyKey { float t; glm::vec3 sky; glm::vec3 amb; glm::vec3 sun; };
    SkyKey keys[] = {
        { 0.00f, {0.02f,0.02f,0.06f}, {0.04f,0.04f,0.06f}, {0.1f,0.1f,0.25f} }, // midnight
        { 0.22f, {0.35f,0.22f,0.18f}, {0.10f,0.08f,0.08f}, {0.9f,0.6f,0.3f } }, // pre-dawn
        { 0.30f, {0.55f,0.72f,0.95f}, {0.25f,0.25f,0.30f}, {1.0f,0.95f,0.8f} }, // morning
        { 0.50f, {0.38f,0.60f,0.90f}, {0.35f,0.35f,0.40f}, {1.0f,0.98f,0.9f} }, // noon
        { 0.70f, {0.60f,0.42f,0.28f}, {0.22f,0.18f,0.14f}, {1.0f,0.75f,0.4f} }, // dusk
        { 0.80f, {0.05f,0.05f,0.12f}, {0.06f,0.06f,0.08f}, {0.2f,0.2f,0.5f } }, // night
        { 1.00f, {0.02f,0.02f,0.06f}, {0.04f,0.04f,0.06f}, {0.1f,0.1f,0.25f} }, // midnight (wrap)
    };
    const int nKeys = (int)(sizeof(keys)/sizeof(keys[0]));

    app.onUpdate([&](float dt) {
        // Time controls
        if (app.keyDown(KEY_SPACE)) paused = !paused;
        if (app.key(KEY_RIGHT))     dayTime += daySpeed * dt * 5.f;
        if (app.key(KEY_LEFT))      dayTime -= daySpeed * dt * 5.f;
        if (!paused)                dayTime += daySpeed * dt;
        dayTime = fmod(dayTime + 1.f, 1.f); // wrap 0..1

        // Find surrounding key-frames and interpolate
        int   k1 = 0;
        for (int i = 0; i < nKeys - 1; ++i)
            if (keys[i+1].t <= dayTime) k1 = i + 1;
        int   k2  = std::min(k1 + 1, nKeys - 1);
        float span = keys[k2].t - keys[k1].t;
        float f    = (span > 0.f) ? (dayTime - keys[k1].t) / span : 0.f;
        f = std::clamp(f, 0.f, 1.f);

        glm::vec3 skyCol = lerpCol(keys[k1].sky, keys[k2].sky, f);
        glm::vec3 ambCol = lerpCol(keys[k1].amb, keys[k2].amb, f);
        glm::vec3 sunCol = lerpCol(keys[k1].sun, keys[k2].sun, f);

        app.skyColor(skyCol.r, skyCol.g, skyCol.b);
        app.ambient(ambCol.r,  ambCol.g,  ambCol.b);
        app.lightColor(sunCol.r, sunCol.g, sunCol.b);

        // Sun/moon orbit: sun on one side, moon opposite
        float angle = dayTime * 6.2832f; // full revolution per day
        float orbitR = 12.f;
        float sx = std::cos(angle) * orbitR;
        float sy = std::sin(angle) * orbitR;

        sun.pos(sx, sy, 0.f);
        moon.pos(-sx, -sy, 0.f);

        // Light direction = toward the sun (normalized)
        glm::vec3 lightDir = glm::normalize(glm::vec3(sx, sy, 0.f));
        app.lightDir(lightDir.x, lightDir.y, lightDir.z);

        // Sun visible only above horizon; moon below
        float dayFactor = std::max(0.f, std::sin(angle));
        sun.color(sunCol.r, sunCol.g * dayFactor + 0.4f, sunCol.b * 0.6f);
        float moonBright = std::max(0.f, -std::sin(angle)) * 0.9f + 0.1f;
        moon.color(moonBright * 0.8f, moonBright * 0.85f, moonBright * 1.f);
    });

    LOG_INFO("Day/Night Cycle  — Space=pause  Left/Right=rewind/forward  Tab=camera");

    return app.run();
}
