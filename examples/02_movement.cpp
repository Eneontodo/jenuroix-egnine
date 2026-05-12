// ============================================================
//  Example 02 — Movement
//  Control a cube with arrow keys / WASD
// ============================================================
#include "engine.h"
using namespace eng;

int main() {
    App app("Movement");
    app.freeCam(false);  // disable free-cam so we drive the object instead

    // Ground plane
    app.spawn("Ground")
       .grid(20, 20, 1.f)
       .color(0.3f, 0.5f, 0.3f)
       .pos(0, -0.5f, 0);

    // Player cube
    auto player = app.spawn("Player")
        .cube()
        .color(1.f, 0.4f, 0.1f)
        .pos(0, 0, 0);

    const float speed = 4.f;

    app.onUpdate([&](float dt) {
        // Read directional input
        float dx = 0.f, dz = 0.f;
        if (app.key(KEY_D) || app.key(KEY_RIGHT)) dx += speed * dt;
        if (app.key(KEY_A) || app.key(KEY_LEFT))  dx -= speed * dt;
        if (app.key(KEY_W) || app.key(KEY_UP))    dz -= speed * dt;
        if (app.key(KEY_S) || app.key(KEY_DOWN))  dz += speed * dt;

        player.move(dx, 0, dz);

        // Top-down camera follows the player
        auto p = player.pos();
        app.camera().setPosition({p.x, 8.f, p.z + 5.f});
        app.camera().setRotation(-90.f, -55.f);
    });

    return app.run();
}
