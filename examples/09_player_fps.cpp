// ============================================================
//  Example 09 — FPS Player Controller
//  Features: gravity, jump (double-jump), sprint, head-bob,
//             fly mode, FOV kick, crosshair overlay
//
//  Controls:
//    Tab    — capture / release mouse
//    WASD   — move
//    Ctrl   — sprint
//    Space  — jump (press twice for double-jump)
//    F      — toggle fly mode
// ============================================================
#include "engine.h"
#include <cmath>
#include <algorithm>
using namespace eng;

int main() {
    App app("FPS Player", 1280, 720);
    app.freeCam(false)
       .skyColor(0.45f, 0.65f, 0.9f)
       .lightDir(0.5f, 1.f, 0.3f)
       .ambient(0.15f, 0.15f, 0.2f);

    // Player physics state
    glm::vec3 playerPos = {0.f, 2.f, 0.f};
    glm::vec3 velocity  = {0.f, 0.f, 0.f};
    bool  onGround      = false;
    bool  sprinting     = false;
    bool  fly           = false;
    float yaw           = -90.f;
    float pitch         = 0.f;
    float bobTimer      = 0.f;
    int   jumpCount     = 0;

    const float GRAVITY    = -22.f;
    const float JUMP_FORCE =  7.5f;
    const float WALK_SPEED =  4.5f;
    const float SPRINT_MUL =  1.8f;
    const float FLY_SPEED  = 10.f;

    // Build scene
    app.onStart([&]() {
        app.spawn("Ground").grid(30, 30, 1.f)
           .color(0.22f, 0.38f, 0.22f).pos(0, -0.5f, 0);

        // Ascending platforms to jump between
        app.spawn("Platform1").cube().scale(6, 0.4f, 6).color(0.5f, 0.45f, 0.35f).pos(8,  2.f, 0);
        app.spawn("Platform2").cube().scale(4, 0.4f, 4).color(0.45f,0.4f, 0.55f).pos(14, 4.5f, 0);
        app.spawn("Platform3").cube().scale(3, 0.4f, 3).color(0.4f, 0.5f, 0.45f).pos(19, 7.f,  0);

        // Angled ramp
        app.spawn("Ramp").cube().scale(4, 0.3f, 8)
           .color(0.55f, 0.5f, 0.4f).pos(4, 0.4f, -6).rotZ(-20.f);

        // Obstacle row
        for (int i = 0; i < 6; ++i)
            app.spawn("Box_" + std::to_string(i)).cube(1.f)
               .color(0.7f, 0.35f, 0.2f).pos(-4.f + i * 1.5f, 0, 5);

        // Pillars
        for (int i = 0; i < 4; ++i) {
            float a = i * 1.5708f;
            app.spawn("Pillar_" + std::to_string(i)).cube().scale(0.5f, 4, 0.5f)
               .color(0.6f, 0.6f, 0.65f).pos(std::cos(a)*6.f, 2, std::sin(a)*6.f);
        }

        app.window().captureCursor(true);
        app.camera().setFov(75.f);
    });

    app.onUpdate([&](float dt) {
        dt = std::min(dt, 0.05f);

        // Mouse look
        if (app.window().isCursorCaptured()) {
            auto d = app.mouseDelta();
            yaw   += d.x * 0.12f;
            pitch  = std::clamp(pitch - d.y * 0.12f, -89.f, 89.f);
        }
        if (app.keyDown(GLFW_KEY_TAB))
            app.window().captureCursor(!app.window().isCursorCaptured());
        if (app.keyDown(KEY_F))
            fly = !fly;

        float yr = glm::radians(yaw);
        glm::vec3 fwd_h = {std::cos(yr), 0, std::sin(yr)};
        glm::vec3 rgt_h = {-std::sin(yr), 0, std::cos(yr)};

        float spd = app.key(KEY_CTRL) ? WALK_SPEED * SPRINT_MUL : WALK_SPEED;
        sprinting  = app.key(KEY_CTRL);

        if (fly) {
            // Fly mode: full 6-DOF movement
            glm::vec3 dir = {0, 0, 0};
            if (app.key(KEY_W)) dir += fwd_h;
            if (app.key(KEY_S)) dir -= fwd_h;
            if (app.key(KEY_D)) dir += rgt_h;
            if (app.key(KEY_A)) dir -= rgt_h;
            if (app.key(KEY_SPACE)) dir.y += 1;
            if (app.key(KEY_SHIFT)) dir.y -= 1;
            if (glm::length(dir) > 0.01f) dir = glm::normalize(dir);
            playerPos += dir * FLY_SPEED * dt;
            velocity   = {0, 0, 0};
            onGround   = false;
        } else {
            // Walk mode with gravity
            glm::vec3 wishDir = {0, 0, 0};
            if (app.key(KEY_W)) wishDir += fwd_h;
            if (app.key(KEY_S)) wishDir -= fwd_h;
            if (app.key(KEY_D)) wishDir += rgt_h;
            if (app.key(KEY_A)) wishDir -= rgt_h;
            if (glm::length(wishDir) > 0.01f) wishDir = glm::normalize(wishDir);

            velocity.x = wishDir.x * spd;
            velocity.z = wishDir.z * spd;

            if (!onGround) velocity.y += GRAVITY * dt;

            // Double-jump
            if (app.keyDown(KEY_SPACE) && jumpCount < 2) {
                velocity.y = JUMP_FORCE;
                ++jumpCount;
            }

            playerPos += velocity * dt;

            // Flat ground collision
            if (playerPos.y <= 0.f) {
                playerPos.y = 0.f;
                velocity.y  = 0.f;
                onGround    = true;
                jumpCount   = 0;
            } else {
                onGround = false;
            }

            // Head-bob
            if (onGround && glm::length(glm::vec2(wishDir.x, wishDir.z)) > 0.01f)
                bobTimer += dt * (sprinting ? 12.f : 8.f);
        }

        // Camera with eye offset and head-bob
        float bobY = onGround ? std::sin(bobTimer) * 0.05f : 0.f;
        float bobX = onGround ? std::sin(bobTimer * 0.5f) * 0.02f : 0.f;
        app.camera().setPosition(playerPos + glm::vec3(bobX, 1.75f + bobY, 0));
        app.camera().setRotation(yaw, pitch);

        // Sprint FOV kick
        float targetFov = sprinting ? 82.f : 75.f;
        app.camera().setFov(app.camera().fov() + (targetFov - app.camera().fov()) * 10.f * dt);
    });

    return app.run();
}
