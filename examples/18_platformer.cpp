// ════════════════════════════════════════════════════════════════════════════
//  Example 18 — Physics Platformer (Physics + Audio + Editor combined)
//
//  Показывает как Phase 1 и Phase 2 работают вместе:
//    - Персонаж управляется через физику (CharacterComponent)
//    - Платформы двигаются (кинематические тела)
//    - Звук при прыжке и приземлении
//    - F1 = открыть редактор прямо во время игры
//
//  Controls:
//    WASD       — движение
//    Space      — прыжок
//    Tab        — мышь
//    F1         — редактор сцены
// ════════════════════════════════════════════════════════════════════════════
#include "engine.h"
#include "physics/Physics.h"
#include "audio/Audio.h"
#include "editor/SceneEditor.h"
using namespace eng;

int main() {
    App app("Physics Platformer", 1280, 720);
    app.skyColor(0.4f, 0.65f, 1.f)
       .lightDir(0.5f, 1.f, 0.3f)
       .lightColor(1.f, 0.97f, 0.9f)
       .ambient(0.3f, 0.35f, 0.45f);

    app.camPos(0, 3, 8).camRot(-90.f, -15.f).freeCam(false);

    PhysicsWorld& phys = app.physics();

    // ── Floor ─────────────────────────────────────────────────────────────────
    auto floor = app.spawn("Floor");
    floor.cube().scale(30, 0.5f, 10).color(0.3f, 0.35f, 0.3f).pos(0, -0.25f, 0);
    app.addRigidBody(floor.id(), RigidBodyType::Static,
                     ColliderShape::Box, {15, 0.25f, 5});

    // ── Platforms ─────────────────────────────────────────────────────────────
    struct Platform {
        GameObject  obj;
        EntityID    id;
        float       phase = 0;
        glm::vec3   basePos;
        glm::vec3   moveAxis;
        float       moveAmp = 2.f;
        float       moveSpeed = 1.2f;
    };

    std::vector<Platform> platforms;
    auto makePlatform = [&](glm::vec3 pos, glm::vec3 axis, float amp, float speed) {
        Platform p;
        p.obj = app.spawn("Platform");
        p.obj.cube().scale(3, 0.3f, 2.5f).color(0.4f, 0.5f, 0.7f).pos(pos.x, pos.y, pos.z);
        app.addRigidBody(p.obj.id(), RigidBodyType::Kinematic,
                         ColliderShape::Box, {1.5f, 0.15f, 1.25f});
        p.id        = p.obj.id();
        p.basePos   = pos;
        p.moveAxis  = axis;
        p.moveAmp   = amp;
        p.moveSpeed = speed;
        p.phase     = (float)(platforms.size()) * 1.2f;
        platforms.push_back(p);
    };

    makePlatform({ 5, 1.5f, 0}, {1,0,0}, 2.f, 1.0f);
    makePlatform({10, 3.0f, 0}, {0,1,0}, 1.5f, 0.8f);
    makePlatform({15, 2.0f, 0}, {1,0,0}, 3.f, 1.3f);

    // ── Collectibles ──────────────────────────────────────────────────────────
    struct Coin {
        GameObject obj;
        bool       collected = false;
    };
    std::vector<Coin> coins;
    for (float x : {2.f, 5.f, 10.f, 12.f, 15.f}) {
        Coin c;
        c.obj = app.spawn("Coin");
        c.obj.sphere().scale(0.3f).color(1.f, 0.8f, 0.f).pos(x, 2.5f, 0);
        coins.push_back(c);
    }
    int score = 0;

    // ── Player ────────────────────────────────────────────────────────────────
    auto player = app.spawn("Player");
    player.cube().scale(0.6f, 0.9f, 0.6f).color(0.9f, 0.3f, 0.25f).pos(0, 1.f, 0);

    auto& cc = app.world().add<CharacterComponent>(player.id());
    cc.height   = 0.9f;
    cc.radius   = 0.3f;
    cc.maxSpeed = 6.f;
    phys.addCharacter(player.id(), cc, {0, 1.f, 0});

    bool wasGrounded = false;

    // ── Audio ─────────────────────────────────────────────────────────────────
    app.onStart([&]() {
        Audio::loop("assets/sounds/music.ogg", 0.5f);
    });

    // ── Editor ────────────────────────────────────────────────────────────────
    SceneEditor editor(app);
    editor.hide(); // hidden by default, F1 to open

    // ── Update ────────────────────────────────────────────────────────────────
    app.onUpdate([&](float dt) {
        float t = (float)glfwGetTime();

        // Move platforms
        for (auto& p : platforms) {
            float f = std::sin(t * p.moveSpeed + p.phase);
            glm::vec3 newPos = p.basePos + p.moveAxis * (f * p.moveAmp);
            phys.setPosition(p.id, newPos);
        }

        // Player movement
        auto* ch = app.world().get<CharacterComponent>(player.id());
        if (ch) {
            glm::vec3 vel = {};
            if (app.keyHeld(GLFW_KEY_A)) vel.x -= 1;
            if (app.keyHeld(GLFW_KEY_D)) vel.x += 1;
            if (app.keyHeld(GLFW_KEY_W)) vel.z -= 1;
            if (app.keyHeld(GLFW_KEY_S)) vel.z += 1;
            if (glm::length(vel) > 0) vel = glm::normalize(vel) * ch->maxSpeed;

            // Jump
            if (app.keyDown(GLFW_KEY_SPACE) && ch->isGrounded) {
                vel.y = 8.f;
                Audio::play("assets/sounds/jump.wav");
            }

            ch->desiredVel = vel;
            phys.updateCharacter(player.id(), *ch, dt);

            // Landing sound
            if (ch->isGrounded && !wasGrounded)
                Audio::play("assets/sounds/land.wav", 0.6f);
            wasGrounded = ch->isGrounded;

            // Camera follows player
            if (auto* t = app.world().get<TransformComponent>(player.id())) {
                glm::vec3 p = t->position;
                app.camPos(p.x, p.y + 3.f, p.z + 8.f);
            }
        }

        // Collect coins
        if (auto* pt = app.world().get<TransformComponent>(player.id())) {
            for (auto& c : coins) {
                if (c.collected) continue;
                if (auto* ct = app.world().get<TransformComponent>(c.obj.id())) {
                    float dist = glm::length(pt->position - ct->position);
                    if (dist < 0.8f) {
                        c.collected = true;
                        app.world().get<MeshRendererComponent>(c.obj.id())->visible = false;
                        Audio::play("assets/sounds/coin.wav");
                        score++;
                    }
                }
                // Spin coin
                if (auto* ct = app.world().get<TransformComponent>(c.obj.id()))
                    ct->rotation = glm::vec3(0, t * 2.f, 0);
            }
        }
    });

    // ── Render ────────────────────────────────────────────────────────────────
    app.onRender([&]() {
        editor.draw();

        if (UI::BeginFixed("##hud", 8, 8, 0, 0,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::LabelColored(UI::WHITE,  "FPS: %.0f", Time::fps());
            UI::LabelColored(UI::YELLOW, "Score: %d / %d", score, (int)coins.size());
            UI::LabelColored(UI::GRAY,   "WASD = движение  Space = прыжок");
            UI::LabelColored(UI::CYAN,   "F1 = редактор сцены");
        }
        UI::End();
    });

    return app.run();
}
