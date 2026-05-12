// ============================================================
//  Example 04 — Collisions
//  Player collects coins; overlaps drive pickup logic
// ============================================================
#include "engine.h"
#include <vector>
#include <cmath>
using namespace eng;

int main() {
    App app("Collision");
    app.freeCam(false);
    app.camPos(0, 12, 0);
    app.camera().setRotation(-90.f, -89.f);

    // Ground
    app.spawn("Ground")
       .grid(16, 16)
       .color(0.2f, 0.4f, 0.2f)
       .pos(0, -0.5f, 0);

    // Player
    auto player = app.spawn("Player")
        .cube(0.8f)
        .color(0.2f, 0.5f, 1.f)
        .addCollider();

    // Coins arranged in a circle
    std::vector<GameObject> coins;
    int score = 0;

    for (int i = 0; i < 8; ++i) {
        float angle = i * 3.14159f * 2.f / 8.f;
        auto coin = app.spawn("Coin_" + std::to_string(i))
            .sphere(0.3f)
            .color(1.f, 0.85f, 0.f)
            .pos(std::cos(angle) * 4.f, 0.f, std::sin(angle) * 4.f)
            .addCollider();
        coins.push_back(coin);
    }

    app.onUpdate([&](float dt) {
        // Player movement
        const float spd = 4.f;
        float dx = (app.key(KEY_D) - app.key(KEY_A)) * spd * dt;
        float dz = (app.key(KEY_S) - app.key(KEY_W)) * spd * dt;
        player.move(dx, 0, dz);

        // Spin coins
        for (auto& c : coins)
            if (c.valid()) c.rotY(c.rot().y + 120.f * dt);

        // Pickup check
        for (auto& c : coins) {
            if (c.valid() && player.overlaps(c)) {
                c.destroy();
                ++score;
                LOG_INFO("Score: " << score);
            }
        }
    });

    return app.run();
}
