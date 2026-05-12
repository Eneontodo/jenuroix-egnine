// ============================================================
//  Example 10 — Top-Down Shooter
//  Health, score, enemies, field bounds
//
//  Controls:
//    WASD / Arrows — move
//    Space         — shoot (in movement direction)
//    (menus shown on screen at startup and game-over)
// ============================================================
#include "engine.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>
using namespace eng;

struct Enemy { GameObject obj; float speed; bool alive = true; };

int main() {
    App app("Top-Down", 1280, 720);
    app.freeCam(false)
       .skyColor(0.06f, 0.06f, 0.1f)
       .lightDir(0, 1, 0.2f)
       .ambient(0.2f, 0.2f, 0.25f);

    app.camPos(0, 20, 0);
    app.camera().setRotation(-90.f, -89.9f);
    app.camera().setFov(60.f);

    enum class GState { MENU, PLAYING, GAMEOVER };
    GState gstate  = GState::MENU;
    int    score   = 0;
    int    hp      = 3;
    float  spawnT  = 0.f;
    float  spawnRate = 1.5f;
    float  elapsed = 0.f;

    glm::vec3 playerPos = {0, 0, 0};
    const float FIELD  = 9.f;
    const float PSPEED = 5.f;

    std::vector<Enemy>      enemies;
    std::vector<GameObject> bullets;
    GameObject              playerObj;

    // Reset / initialize scene state
    auto initScene = [&]() {
        for (auto& e : enemies) if (e.obj.valid()) e.obj.destroy();
        for (auto& b : bullets) if (b.valid()) b.destroy();
        enemies.clear(); bullets.clear();
        if (playerObj.valid()) playerObj.destroy();

        playerPos = {0, 0, 0};
        score = 0; hp = 3;
        spawnT = 0.f; spawnRate = 1.5f; elapsed = 0.f;

        // Arena
        app.spawn("Floor").grid(20, 20, 1.f).color(0.12f, 0.12f, 0.18f).pos(0, -0.4f, 0);

        // Boundary walls
        float bh = 0.4f;
        app.spawn("WN").cube().scale(FIELD*2+1, bh, 0.5f).color(0.25f,0.25f,0.3f).pos(0, 0,  FIELD+0.25f);
        app.spawn("WS").cube().scale(FIELD*2+1, bh, 0.5f).color(0.25f,0.25f,0.3f).pos(0, 0, -FIELD-0.25f);
        app.spawn("WE").cube().scale(0.5f, bh, FIELD*2).color(0.25f,0.25f,0.3f).pos( FIELD+0.25f, 0, 0);
        app.spawn("WW").cube().scale(0.5f, bh, FIELD*2).color(0.25f,0.25f,0.3f).pos(-FIELD-0.25f, 0, 0);

        playerObj = app.spawn("Player").cube(0.8f).color(0.2f, 0.7f, 1.f).pos(0, 0, 0);
    };

    app.onUpdate([&](float dt) {
        if (gstate != GState::PLAYING) return;

        elapsed   += dt;
        spawnT    += dt;
        spawnRate  = std::max(0.3f, 1.5f - elapsed * 0.05f);

        // Player movement
        glm::vec3 dir = {0, 0, 0};
        if (app.key(KEY_D) || app.key(KEY_RIGHT)) dir.x += 1;
        if (app.key(KEY_A) || app.key(KEY_LEFT))  dir.x -= 1;
        if (app.key(KEY_S) || app.key(KEY_DOWN))  dir.z += 1;
        if (app.key(KEY_W) || app.key(KEY_UP))    dir.z -= 1;
        if (glm::length(dir) > 0.01f) dir = glm::normalize(dir);

        playerPos += dir * PSPEED * dt;
        playerPos.x = std::clamp(playerPos.x, -FIELD, FIELD);
        playerPos.z = std::clamp(playerPos.z, -FIELD, FIELD);
        playerObj.pos(playerPos);
        if (glm::length(dir) > 0.01f)
            playerObj.rotY(glm::degrees(std::atan2(-dir.x, -dir.z)));

        // Shoot in movement direction
        if ((app.keyDown(KEY_SPACE) || app.keyDown(GLFW_KEY_ENTER)) && glm::length(dir) > 0.01f) {
            auto b = app.spawn("Bullet")
                .sphere(0.2f).color(1, 0.9f, 0.1f)
                .pos(playerPos.x, 0, playerPos.z);
            b.scale(dir.x, dir.z, 0); // store direction in scale (data hack)
            bullets.push_back(b);
        }

        // Move bullets
        const float BSPEED = 12.f;
        for (int i = (int)bullets.size() - 1; i >= 0; --i) {
            auto& b = bullets[i];
            if (!b.valid()) { bullets.erase(bullets.begin() + i); continue; }
            auto sc = b.scale();
            glm::vec3 bd = glm::normalize(glm::vec3(sc.x, 0, sc.y));
            b.move(bd * BSPEED * dt);
            auto bp = b.pos();
            if (std::abs(bp.x) > FIELD + 1 || std::abs(bp.z) > FIELD + 1) {
                b.destroy(); bullets.erase(bullets.begin() + i);
            }
        }

        // Spawn enemies from the arena edge
        if (spawnT >= spawnRate) {
            spawnT = 0.f;
            float a = ((float)rand() / RAND_MAX) * 6.2832f;
            float spd = std::min(1.5f + elapsed * 0.03f, 4.f);
            Enemy e;
            e.speed = spd;
            e.obj   = app.spawn("Enemy").cube(0.7f)
                         .color(1.f, 0.2f + (float)rand()/RAND_MAX * 0.3f, 0.1f)
                         .pos(std::cos(a) * (FIELD + 0.5f), 0, std::sin(a) * (FIELD + 0.5f));
            e.alive = true;
            enemies.push_back(e);
        }

        // Update enemies
        for (int i = (int)enemies.size() - 1; i >= 0; --i) {
            auto& e = enemies[i];
            if (!e.obj.valid()) { enemies.erase(enemies.begin() + i); continue; }
            glm::vec3 ep  = e.obj.pos();
            glm::vec3 ed  = glm::normalize(playerPos - ep);
            e.obj.move(ed * e.speed * dt);
            e.obj.rotY(glm::degrees(std::atan2(-ed.x, -ed.z)));

            // Hit player
            if (glm::length(playerPos - ep) < 0.8f) {
                if (--hp <= 0) gstate = GState::GAMEOVER;
                e.obj.destroy(); enemies.erase(enemies.begin() + i); continue;
            }

            // Hit by bullet
            bool killed = false;
            for (int j = (int)bullets.size() - 1; j >= 0; --j) {
                if (!bullets[j].valid()) continue;
                if (glm::length(ep - bullets[j].pos()) < 0.6f) {
                    e.obj.destroy(); bullets[j].destroy();
                    enemies.erase(enemies.begin() + i);
                    bullets.erase(bullets.begin() + j);
                    ++score;
                    killed = true; break;
                }
            }
            if (killed) continue;
        }

        // Camera tracks player
        app.camPos(playerPos.x, 20, playerPos.z);
        app.camera().setRotation(-90.f + playerPos.x * 0.5f, -89.9f);
    });

    // UI: menus, HUD, and danger flash — these all belong in onRender
    app.onRender([&]() {
        float SW = ImGui::GetIO().DisplaySize.x;
        float SH = ImGui::GetIO().DisplaySize.y;

        if (gstate == GState::MENU) {
            UI::DarkOverlay(0.7f);
            if (UI::BeginMainMenu("##menu")) {
                UI::Title("TOP DOWN");
                UI::LabelColored(UI::GRAY, "WASD / Arrows — move");
                UI::LabelColored(UI::GRAY, "Space         — shoot");
                UI::LabelColored(UI::GRAY, "Survive as long as possible!");
                UI::Spacing();
                if (UI::Button("Start Game")) { initScene(); gstate = GState::PLAYING; }
                UI::Separator();
                if (UI::Button("Quit")) app.quit();
                UI::EndMainMenu();
            }
            return;
        }

        if (gstate == GState::GAMEOVER) {
            UI::DarkOverlay(0.65f);
            if (UI::BeginMainMenu("##over")) {
                UI::Title("GAME OVER");
                UI::LabelColored(UI::YELLOW, "Score: %d", score);
                UI::LabelColored(UI::GRAY,   "Survived: %.0fs", elapsed);
                UI::Spacing();
                if (UI::Button("Play Again")) { initScene(); gstate = GState::PLAYING; }
                if (UI::Button("Main Menu"))   gstate = GState::MENU;
                UI::EndMainMenu();
            }
            return;
        }

        // In-game HUD
        if (UI::BeginFixed("##hud", 8, 8, 0, 0,
            ImGuiWindowFlags_NoTitleBar    |
            ImGuiWindowFlags_NoBackground  |
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::LabelColored(UI::YELLOW, "Score: %d",   score);
            UI::LabelColored(UI::WHITE,  "Time:  %.0fs", elapsed);
            UI::LabelColored(UI::GRAY,   "FPS:   %.0f",  Time::fps());
            std::string hearts;
            for (int i = 0; i < 3; ++i) hearts += (i < hp) ? "[HP]" : "[  ]";
            UI::LabelColored(hp > 1 ? UI::GREEN : UI::RED, "%s", hearts.c_str());
        }
        UI::End();

        // Red flash when enemies are close
        int close = 0;
        for (auto& e : enemies)
            if (e.obj.valid() && glm::length(playerPos - e.obj.pos()) < 3.f) ++close;
        if (close > 0) {
            auto* dl = ImGui::GetForegroundDrawList();
            float t  = (float)glfwGetTime();
            uint8_t a = (uint8_t)(std::abs(std::sin(t * 6.f)) * 180.f);
            dl->AddRect({0,0},{SW,SH}, IM_COL32(255,50,50,a), 0, 0, 4);
        }
    });

    return app.run();
}
