// ════════════════════════════════════════════════════════════════════════════
//  Example 16 — Audio (miniaudio)
//
//  Features:
//    -   - Background music (looping)
//    -   - Sound effects (play once)
//    -   - 3D positional audio (громкость зависит от расстояния до камеры)
//    -   - Volume and pitch control
//    -   - Pause / Resume
//
//  Controls:
//    Tab     — capture mouse
//    WASD    — camera (3D-звук меняется при движении)
//    1       — воспроизвести эффект
//    2       — пауза / resume музыки
//    +/-     — громкость музыки
//    P       — изменить pitch
//
//  Необходимые файлы (положи в assets/sounds/):
//    music.ogg   — фоновая музыка
//    hit.wav     — короткий эффект
//    ambient.ogg — 3D-источник звука
// ════════════════════════════════════════════════════════════════════════════
#include "engine.h"
#include "audio/Audio.h"
using namespace eng;

int main() {
    App app("Audio Demo", 1280, 720);
    app.skyColor(0.08f, 0.08f, 0.12f)
       .lightDir(0.3f, 1.f, 0.5f)
       .ambient(0.15f, 0.15f, 0.2f);
    app.camPos(0, 2, 8).camRot(-90.f, -10.f).freeCam(true);

    // ── 3D sound sources (visual markers) ────────────────────────────────────
    struct SoundSource {
        glm::vec3    pos;
        SoundHandle  handle = INVALID_SOUND;
        std::string  label;
        glm::vec3    color;
    };

    std::vector<SoundSource> sources = {
        {{ -4, 1,  0}, INVALID_SOUND, "Ambient A",  {0.2f,0.6f,1.f}},
        {{  4, 1,  0}, INVALID_SOUND, "Ambient B",  {1.f, 0.5f,0.1f}},
        {{  0, 1, -4}, INVALID_SOUND, "Ambient C",  {0.2f,0.9f,0.3f}},
    };

    // Spawn visual spheres for sources
    for (auto& src : sources) {
        app.spawn(src.label)
           .sphere().scale(0.4f)
           .color(src.color.r, src.color.g, src.color.b)
           .pos(src.pos.x, src.pos.y, src.pos.z);
    }

    // ── Audio ─────────────────────────────────────────────────────────────────
    SoundHandle music    = INVALID_SOUND;
    float       musicVol = 0.7f;
    bool        musicPaused = false;

    app.onStart([&]() {
        // Background music
        music = Audio::loop("assets/sounds/music.ogg", musicVol);

        // 3D ambient sources
        for (auto& src : sources) {
            src.handle = Audio::loop3D(
                "assets/sounds/ambient.ogg",
                src.pos, 0.8f, 15.f);
        }
    });

    app.onUpdate([&](float dt) {
        // 1 = play one-shot effect
        if (app.keyDown(GLFW_KEY_1))
            Audio::play("assets/sounds/hit.wav");

        // 2 = pause/resume music
        if (app.keyDown(GLFW_KEY_2)) {
            musicPaused = !musicPaused;
            if (musicPaused) Audio::pause(music);
            else             Audio::resume(music);
        }

        // +/- = volume
        if (app.keyHeld(GLFW_KEY_EQUAL)) {
            musicVol = std::min(1.f, musicVol + dt * 0.5f);
            Audio::setVolume(music, musicVol);
        }
        if (app.keyHeld(GLFW_KEY_MINUS)) {
            musicVol = std::max(0.f, musicVol - dt * 0.5f);
            Audio::setVolume(music, musicVol);
        }

        // P = pitch shift
        static float pitch = 1.f;
        if (app.keyDown(GLFW_KEY_P)) {
            pitch = (pitch >= 2.f) ? 0.5f : pitch + 0.25f;
            Audio::setPitch(music, pitch);
        }

        // Update 3D source positions (they're static here, but could move)
        for (auto& src : sources)
            Audio::setPosition(src.handle, src.pos);
    });

    // ── HUD ───────────────────────────────────────────────────────────────────
    app.onRender([&]() {
        if (UI::BeginFixed("##hud", 8, 8, 0, 0,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::LabelColored(UI::WHITE,  "FPS: %.0f", Time::fps());
            UI::LabelColored(UI::GRAY,   "Tab=мышь  WASD=двигай камеру");
            UI::Separator();
            UI::LabelColored(UI::YELLOW, "1 = звуковой эффект");
            UI::LabelColored(UI::YELLOW, "2 = пауза/resume музыки");
            UI::LabelColored(UI::YELLOW, "+/- = громкость  P = pitch");
            UI::Separator();
            UI::LabelColored(
                Audio::isInitialized() ? UI::GREEN : UI::RED,
                "Audio: %s", Audio::isInitialized()
                    ? "miniaudio OK" : "stub (нет miniaudio.h)");
            UI::LabelColored(UI::CYAN, "Музыка: %.0f%%  %s",
                musicVol * 100.f, musicPaused ? "[ПАУЗА]" : "[ИГРАЕТ]");
            UI::LabelColored(UI::GRAY, "Активных звуков: %d",
                Audio::activeSoundCount());
            UI::Separator();
            UI::LabelColored(UI::GRAY,
                "Шары = 3D источники. Подойди ближе.");
        }
        UI::End();
    });

    return app.run();
}
