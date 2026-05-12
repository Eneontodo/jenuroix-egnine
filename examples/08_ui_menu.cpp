// ============================================================
//  Example 08 — UI: Main Menu, Settings, Pause Screen
//  Demonstrates the UI:: namespace for game menus
//
//  States: MENU → PLAYING → PAUSED → SETTINGS
//  Tab   — capture / release cursor
//  Esc   — pause while playing
// ============================================================
#include "engine.h"
#include <cmath>
using namespace eng;

enum class State { MENU, SETTINGS, PLAYING, PAUSED };

int main() {
    App app("UI Example", 1280, 720);
    app.freeCam(false)
       .skyColor(0.1f, 0.1f, 0.15f)
       .lightDir(0.6f, 1.f, 0.4f);

    State state      = State::MENU;
    float camSpeed   = 5.f;
    float fovVal     = 70.f;
    bool  vsync      = true;
    bool  showFPS    = true;
    int   qualityIdx = 1;
    const char* qualities[] = {"Low", "Medium", "High", "Ultra"};

    // Build scene
    app.onStart([&]() {
        app.spawn("Ground").grid(20, 20).color(0.2f, 0.35f, 0.2f).pos(0, -0.5f, 0);

        // Five spinning, bouncing cubes arranged in a circle
        for (int i = 0; i < 5; ++i) {
            float a = i * 1.2566f;
            app.spawn("Cube_" + std::to_string(i))
               .cube()
               .color(0.2f + i * 0.15f, 0.4f, 0.8f - i * 0.1f)
               .pos(std::cos(a) * 4.f, 0, std::sin(a) * 4.f)
               .onUpdate([a](GameObject& self, float dt) {
                   static float t = 0; t += dt;
                   self.rotY(self.rot().y + 60.f * dt);
                   self.y(std::abs(std::sin(t + a)) * 1.5f);
               });
        }
        app.camPos(0, 4, 10).camRot(-90, -20).fov(fovVal);
    });

    // Input — only active while playing
    app.onUpdate([&](float dt) {
        if (state != State::PLAYING) return;

        if (app.keyDown(KEY_ESC)) {
            state = State::PAUSED;
            app.window().captureCursor(false);
        }
        if (app.keyDown(GLFW_KEY_TAB))
            app.window().captureCursor(!app.window().isCursorCaptured());

        if (app.window().isCursorCaptured())
            app.camera().processMouse(app.mouseDelta().x, app.mouseDelta().y);

        float fwd = (float)(app.key(KEY_W) - app.key(KEY_S));
        float rgt = (float)(app.key(KEY_D) - app.key(KEY_A));
        float up  = (float)(app.key(KEY_SPACE) - app.key(KEY_SHIFT));
        app.camera().processKeyboard(fwd, rgt, up, camSpeed, dt);
    });

    // All menus and HUD live in onRender so they draw on top of the 3D scene
    app.onRender([&]() {
        // ── Main Menu ─────────────────────────────────────────────────────
        if (state == State::MENU) {
            UI::DarkOverlay(0.6f);
            if (UI::BeginMainMenu("##menu")) {
                UI::Title("MY GAME");
                if (UI::Button("Play")) {
                    app.window().captureCursor(true);
                    state = State::PLAYING;
                }
                if (UI::Button("Settings")) state = State::SETTINGS;
                UI::Separator();
                if (UI::Button("Quit")) app.quit();
                UI::EndMainMenu();
            }
        }

        // ── Settings ──────────────────────────────────────────────────────
        if (state == State::SETTINGS) {
            UI::DarkOverlay(0.6f);
            if (UI::BeginMainMenu("##settings")) {
                UI::Title("SETTINGS");
                if (UI::Section("Graphics")) {
                    UI::Dropdown("Quality", qualityIdx, qualities, 4);
                    UI::SliderFloat("FOV",  fovVal, 40.f, 120.f);
                    UI::Bool("VSync",       vsync);
                    app.fov(fovVal);
                }
                if (UI::Section("Gameplay")) {
                    UI::SliderFloat("Camera Speed", camSpeed, 1.f, 20.f);
                    UI::Bool("Show FPS", showFPS);
                }
                UI::Separator();
                if (UI::Button("Back")) state = State::MENU;
                UI::EndMainMenu();
            }
        }

        // ── Pause Screen ──────────────────────────────────────────────────
        if (state == State::PAUSED) {
            UI::DarkOverlay(0.4f);
            if (UI::BeginMainMenu("##pause")) {
                UI::Title("PAUSED");
                if (UI::Button("Resume")) {
                    app.window().captureCursor(true);
                    state = State::PLAYING;
                }
                if (UI::Button("Settings")) state = State::SETTINGS;
                UI::Separator();
                if (UI::Button("Main Menu")) {
                    app.window().captureCursor(false);
                    state = State::MENU;
                }
                UI::EndMainMenu();
            }
        }

        // ── HUD (in-game overlay) ─────────────────────────────────────────
        if (state == State::PLAYING && showFPS) {
            if (UI::BeginFixed("##hud", 8, 8, 0, 0,
                ImGuiWindowFlags_NoTitleBar    |
                ImGuiWindowFlags_NoBackground  |
                ImGuiWindowFlags_AlwaysAutoResize))
            {
                UI::LabelColored(UI::GREEN, "FPS: %.0f", Time::fps());
                auto p = app.camera().position();
                UI::LabelColored(UI::GRAY,  "%.1f / %.1f / %.1f", p.x, p.y, p.z);
                UI::LabelColored(UI::GRAY,  "Tab = mouse  ESC = pause");
            }
            UI::End();
        }
    });

    return app.run();
}
