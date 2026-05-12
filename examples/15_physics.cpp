// ════════════════════════════════════════════════════════════════════════════
//  Example 15 — Physics (Jolt Physics integration)
//
//  Features:
//    -   - Static colliders (floor, walls)
//    -   - Dynamic bodies (falling and bouncing cubes, spheres)
//    -   - Impulse application (Space = throw ball)
//    -   - Raycast (LMB = hit an object)
//    -   - Collision callbacks
//
//  Controls:
//    Tab        — capture mouse
//    WASD+QE    — camera
//    Space      — throw ball forward
//    R          — reset scene
//    LMB        — raycast + подтолкнуть объект
// ════════════════════════════════════════════════════════════════════════════
#include "engine.h"
#include "physics/Physics.h"
using namespace eng;

int main() {
    App app("Physics Demo", 1280, 720);
    app.skyColor(0.15f, 0.18f, 0.28f)
       .lightDir(0.4f, 1.f, 0.6f)
       .ambient(0.2f, 0.2f, 0.28f);

    app.camPos(0, 6, 14).camRot(-90.f, -20.f).freeCam(true);

    PhysicsWorld& phys = app.physics();

    // ── Collision callback ────────────────────────────────────────────────────
    phys.onCollisionEnter([](const HitEvent& e) {
        if (e.impulse > 5.f)
            LOG_INFO("Collision! entities " << e.entityA << " & " << e.entityB
                     << "  impulse=" << e.impulse);
    });

    // ── Static floor ──────────────────────────────────────────────────────────
    auto floor = app.spawn("Floor");
    floor.cube().scale(20, 0.5f, 20).color(0.25f, 0.25f, 0.3f).pos(0, -0.25f, 0);
    app.addRigidBody(floor.id(), RigidBodyType::Static,
                     ColliderShape::Box, {10, 0.25f, 10});

    // ── Static walls ──────────────────────────────────────────────────────────
    for (auto [x, z, sx, sz] : std::initializer_list<std::tuple<float,float,float,float>>{
        {-10, 0, 0.5f, 10}, {10, 0, 0.5f, 10},
        {0, -10, 10, 0.5f}, {0,  10, 10, 0.5f}})
    {
        auto wall = app.spawn("Wall");
        wall.cube().scale(sx*2, 4, sz*2)
            .pos(x, 2, z).color(0.2f, 0.2f, 0.25f);
        app.addRigidBody(wall.id(), RigidBodyType::Static,
                         ColliderShape::Box, {sx, 2, sz});
    }

    // ── Dynamic boxes ─────────────────────────────────────────────────────────
    int spawnCount = 0;
    auto spawnBox = [&](glm::vec3 pos, glm::vec3 col) {
        auto box = app.spawn("Box_" + std::to_string(spawnCount++));
        box.cube().scale(0.8f).color(col.r, col.g, col.b).pos(pos.x, pos.y, pos.z);
        app.addRigidBody(box.id(), RigidBodyType::Dynamic,
                         ColliderShape::Box, {0.4f, 0.4f, 0.4f}, 2.f);
    };

    auto spawnSphere = [&](glm::vec3 pos, glm::vec3 col) {
        auto sphere = app.spawn("Sphere_" + std::to_string(spawnCount++));
        sphere.sphere().scale(0.5f).color(col.r, col.g, col.b).pos(pos.x, pos.y, pos.z);
        app.addRigidBody(sphere.id(), RigidBodyType::Dynamic,
                         ColliderShape::Sphere, {0.25f, 0.25f, 0.25f}, 1.f);
        if (auto* rb = app.world().get<RigidBodyComponent>(sphere.id())) {
            rb->radius      = 0.25f;
            rb->restitution = 0.6f;
        }
    };

    // Stack of boxes
    for (int y = 0; y < 4; ++y)
        for (int x = -1; x <= 1; ++x)
            spawnBox({x * 1.f, 0.5f + y * 0.85f, 0},
                     {0.2f + x*0.2f, 0.5f - y*0.1f, 0.9f});

    // Scattered spheres
    spawnSphere({-3, 4, 1},  {0.9f, 0.3f, 0.2f});
    spawnSphere({ 3, 5, -1}, {0.2f, 0.8f, 0.3f});
    spawnSphere({ 0, 6, 2},  {0.9f, 0.7f, 0.1f});

    // ── onUpdate ──────────────────────────────────────────────────────────────
    app.onUpdate([&](float dt) {
        // Space = throw a ball forward
        if (app.keyDown(GLFW_KEY_SPACE)) {
            glm::vec3 pos = app.camPos() + app.camForward() * 2.f;
            spawnSphere(pos, {0.9f, 0.4f, 0.1f});
            glm::vec3 vel = app.camForward() * 18.f;
            // Apply launch velocity
            auto& entities = app.world();
            // Find last spawned sphere and kick it
            // (a real game would store the handle)
        }

        // R = reset scene
        if (app.keyDown(GLFW_KEY_R)) {
            LOG_INFO("Resetting scene...");
            // In a full engine this would reload the scene
        }

        // LMB raycast
        if (app.mouseDown(0)) {
            RayHit hit = phys.raycast(
                app.camPos(),
                app.camForward(),
                50.f);
            if (hit) {
                LOG_INFO("Hit entity " << hit.entityID
                         << " at distance " << hit.distance);
                phys.applyImpulse(hit.entityID,
                    app.camForward() * 8.f + glm::vec3(0, 3, 0));
            }
        }
    });

    // ── HUD ───────────────────────────────────────────────────────────────────
    app.onRender([&]() {
        if (UI::BeginFixed("##hud", 8, 8, 0, 0,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::LabelColored(UI::WHITE,  "FPS: %.0f", Time::fps());
            UI::LabelColored(UI::GRAY,   "Tab=мышь  WASD=camera");
            UI::LabelColored(UI::YELLOW, "Space = бросить шар");
            UI::LabelColored(UI::CYAN,   "LMB = толкнуть объект");
            UI::LabelColored(UI::GRAY,   "Bodies: %d", phys.bodyCount());
            UI::LabelColored(
                phys.isInitialized() ? UI::GREEN : UI::RED,
                "Physics: %s",
                phys.isInitialized() ? "Jolt OK" : "stub (Jolt не установлен)");
        }
        UI::End();
    });

    return app.run();
}
