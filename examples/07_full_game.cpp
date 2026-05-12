// ============================================================
//  Example 07 — Mini-Game: DODGE!
//  Survive as long as possible — dodge flying cubes!
//  Controls: WASD / Arrow keys — move
//            R               — restart after death
//  Score increases over time; projectile speed ramps up.
// ============================================================
#include "engine.h"
#include <vector>
#include <cstdlib>
#include <cmath>
using namespace eng;

int main() {
    App app("DODGE!", 1280, 720);
    app.freeCam(false)
       .skyColor(0.05f, 0.05f, 0.1f)
       .ambient(0.1f, 0.1f, 0.15f)
       .lightDir(0.3f, 1.f, 0.5f);
    app.camPos(0, 18, 0);
    app.camera().setRotation(-90.f, -89.f);

    // Arena floor
    app.spawn("Floor").grid(20, 20).color(0.15f, 0.15f, 0.2f).pos(0, -0.5f, 0);

    // Visual boundary walls
    const float wc[3] = {0.3f, 0.3f, 0.4f};
    for (int i = 0; i < 4; ++i) {
        float a = i * 3.14159f / 2.f;
        app.spawn("Wall_" + std::to_string(i))
           .cube(0.5f).scale(0.5f, 2, 20)
           .color(wc[0], wc[1], wc[2])
           .pos(std::cos(a) * 10.f, 0, std::sin(a) * 10.f);
    }

    // Player
    auto player = app.spawn("Player")
        .cube(0.8f).color(0.2f, 0.8f, 0.3f).addCollider();

    bool  dead          = false;
    float score         = 0.f;
    float playerSpeed   = 3.5f;
    float spawnTimer    = 0.f;
    float spawnInterval = 1.2f;

    // Projectile pool
    struct Bullet { GameObject obj; glm::vec3 dir; float spd; };
    std::vector<Bullet> bullets;

    // Spawn one projectile aimed at the player from the arena edge
    auto spawnBullet = [&]() {
        float angle = ((float)rand() / RAND_MAX) * 6.2831f;
        glm::vec3 start = {std::cos(angle) * 8.f, 0, std::sin(angle) * 8.f};
        glm::vec3 dir   = glm::normalize(player.pos() - start);
        float     spd   = 4.f + score * 0.02f; // speed increases with score

        auto obj = app.spawn()
            .cube(0.5f)
            .color(1.f, 0.2f + (float)rand() / RAND_MAX * 0.3f, 0.1f)
            .pos(start.x, 0, start.z)
            .addCollider();

        bullets.push_back({obj, dir, spd});
    };

    app.onUpdate([&](float dt) {
        // ── Restart state ─────────────────────────────────────────────────
        if (dead) {
            if (app.keyDown(GLFW_KEY_R)) {
                for (auto& b : bullets) b.obj.destroy();
                bullets.clear();
                player.pos(0, 0, 0).show();
                dead          = false;
                score         = 0.f;
                playerSpeed   = 3.5f;
                spawnInterval = 1.2f;
            }
            return;
        }

        // ── Player movement ───────────────────────────────────────────────
        float dx = (float)((app.key(KEY_D) || app.key(KEY_RIGHT)) -
                           (app.key(KEY_A) || app.key(KEY_LEFT)));
        float dz = (float)((app.key(KEY_S) || app.key(KEY_DOWN)) -
                           (app.key(KEY_W) || app.key(KEY_UP)));
        glm::vec3 mv = {dx, 0, dz};
        if (glm::length(mv) > 0.01f)
            mv = glm::normalize(mv) * playerSpeed * dt;
        player.move(mv);

        // Clamp inside arena
        auto p = player.pos();
        p.x = std::clamp(p.x, -9.f, 9.f);
        p.z = std::clamp(p.z, -9.f, 9.f);
        player.pos(p);
        player.rotY(player.rot().y + 180.f * dt);

        // ── Projectile spawning ───────────────────────────────────────────
        if ((spawnTimer += dt) >= spawnInterval) {
            spawnTimer    = 0.f;
            spawnBullet();
            spawnInterval = std::max(0.35f, spawnInterval - 0.02f);
        }

        // ── Move & check projectiles ──────────────────────────────────────
        for (auto it = bullets.begin(); it != bullets.end();) {
            it->obj.move(it->dir * it->spd * dt);
            it->obj.rotY(it->obj.rot().y + 200.f * dt);

            auto bp = it->obj.pos();
            if (std::abs(bp.x) > 12.f || std::abs(bp.z) > 12.f) {
                it->obj.destroy();
                it = bullets.erase(it);
                continue;
            }
            if (player.overlaps(it->obj)) {
                dead = true;
                player.hide();
                LOG_INFO("GAME OVER! Score: " << (int)score);
                LOG_INFO("Press R to restart");
                break;
            }
            ++it;
        }

        // Score grows with time survived
        score += dt * 10.f;
    });

    LOG_INFO("=== DODGE! ===  WASD — move.  R — restart after death.");

    return app.run();
}
