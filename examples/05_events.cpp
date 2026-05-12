// ============================================================
//  Example 05 — Events
//  React to key presses through the EventBus
//  Space  → cycle box color
//  F      → toggle wireframe
// ============================================================
#include "engine.h"
using namespace eng;

int main() {
    App app("Events");

    auto box = app.spawn("Box").cube().color(0.4f, 0.8f, 0.4f);
    bool wireframe = false;

    // Subscribe to key-press events
    Events().on<KeyPressedEvent>([&](const KeyPressedEvent& e) {
        if (e.key == KEY_SPACE) {
            // Cycle through a palette of colors
            static int colorIdx = 0;
            static float colors[][3] = {
                {1.f,0.3f,0.3f}, {0.3f,1.f,0.3f}, {0.3f,0.3f,1.f},
                {1.f,1.f,0.2f},  {1.f,0.5f,0.1f}, {0.8f,0.2f,0.8f}
            };
            int c = colorIdx++ % 6;
            box.color(colors[c][0], colors[c][1], colors[c][2]);
            LOG_INFO("Color changed! Press Space again.");
        }
        if (e.key == GLFW_KEY_F) {
            wireframe = !wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
            LOG_INFO("Wireframe: " << (wireframe ? "ON" : "OFF"));
        }
    });

    // Emit events from app input so EventBus subscribers receive them
    app.onUpdate([&](float dt) {
        if (app.keyDown(KEY_SPACE))
            Events().emit(KeyPressedEvent{KEY_SPACE, false});
        if (app.keyDown(GLFW_KEY_F))
            Events().emit(KeyPressedEvent{GLFW_KEY_F, false});

        box.rotY(box.rot().y + 45.f * dt);
    });

    LOG_INFO("Space = change color   F = wireframe   Tab = camera");

    return app.run();
}
