// ╔══════════════════════════════════════════════════════════════════════════╗
// ║  12_model_viewer.cpp  —  Full-featured 3D model viewer                ║
// ║                                                                          ║
// ║  Перетащи .obj/.gltf/.glb прямо на .exe чтобы открыть модель,           ║
// ║  или отредактируй DEFAULT_MODEL ниже.                                    ║
// ║                                                                          ║
// ║  Controls:                                                             ║
// ║    Tab / ЛКМ  — захват/освобождение курсора                             ║
// ║    WASD       — движение камеры                                         ║
// ║    Мышь       — вращение камеры                                         ║
// ║    Колесо     — зум (FOV)                                               ║
// ║    R          — сбросить камеру                                          ║
// ║    F          — wireframe on/off                                         ║
// ║    Space      — авто-вращение модели on/off                              ║
// ║    Esc        — выход                                                    ║
// ╚══════════════════════════════════════════════════════════════════════════╝

#include "engine.h"
#include "ui/UIRenderer.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
namespace fs = std::filesystem;
using namespace eng;

// ── Найти все модели в директории ──────────────────────────────────────────
static std::vector<std::string> findModels(const std::string& dir) {
    std::vector<std::string> out;
    try {
        for (auto& e : fs::directory_iterator(dir)) {
            auto ext = e.path().extension().string();
            if (ext == ".obj" || ext == ".gltf" || ext == ".glb")
                out.push_back(e.path().string());
        }
    } catch (...) {}
    return out;
}

int main(int argc, char** argv) {
    App app("Model Viewer", 1280, 720);
    g_app = &app;
    app.freeCam(true);

    UIRenderer ui;
    ui.init(1280, 720);
    app.window().setResizeCallback([&](int w, int h) {
        ui.resize(w, h);
        app.camera().setAspect((float)w / h);
    });

    // ── Сцена ──────────────────────────────────────────────────────────────
    app.skyColor(0.1f, 0.12f, 0.18f);
    app.lightDir(0.5f, 1.f, 0.4f);
    app.lightColor(1.f, 0.95f, 0.88f);
    app.ambient(0.15f, 0.15f, 0.2f);
    app.camPos(0, 1.5f, 4.f).camRot(-90.f, -12.f);

    // Пол
    auto floor = app.spawn("Floor");
    floor.grid(20, 20, 1.f).color(0.12f, 0.12f, 0.15f).pos(0, 0, 0).scale(8, 1, 8);

    // Сетка осей (X=красный, Y=зелёный, Z=синий)
    auto axisX = app.spawn("AxisX"); axisX.cube(1.f).color(0.9f, 0.2f, 0.2f).pos(0.5f, 0.01f, 0.f).scale(1.f, 0.01f, 0.01f);
    auto axisY = app.spawn("AxisY"); axisY.cube(1.f).color(0.2f, 0.9f, 0.2f).pos(0.f, 0.5f, 0.f).scale(0.01f, 1.f, 0.01f);
    auto axisZ = app.spawn("AxisZ"); axisZ.cube(1.f).color(0.2f, 0.2f, 0.9f).pos(0.f, 0.01f, 0.5f).scale(0.01f, 0.01f, 1.f);

    // ── Определить что загружать ───────────────────────────────────────────
    // 1. Аргумент командной строки (перетаскивание на .exe)
    // 2. Первая модель в assets/models/
    // 3. Заглушка
    std::string modelPath;

    if (argc >= 2) {
        modelPath = argv[1];
        std::cout << "[ARG] Loading from argument: " << modelPath << "\n";
    } else {
        auto models = findModels("assets/models");
        if (!models.empty()) {
            modelPath = models[0];
            std::cout << "[AUTO] Found model: " << modelPath << "\n";
        }
    }

    // ── Загрузка модели ────────────────────────────────────────────────────
    GameObject modelObj;
    std::string statusMsg;
    bool modelLoaded = false;
    bool autoRotate  = true;
    bool wireframe   = false;

    auto loadModel = [&](const std::string& path) {
        if (modelObj.valid()) modelObj.destroy();

        if (path.empty() || !fs::exists(path)) {
            statusMsg = "File not found: " + (path.empty() ? "(none)" : path);
            std::cout << "[ERR] " << statusMsg << "\n";
            // Показать куб-заглушку
            modelObj = app.spawn("Placeholder");
            modelObj.cube(1.f).color(0.45f, 0.45f, 0.55f).pos(0, 0.5f, 0);
            modelLoaded = false;
            return;
        }

        modelObj = app.spawn("Model");
        modelObj.model(path);

        // Автоподбор масштаба: пытаемся вписать модель в куб ~2 единицы
        // (ResourceManager кэширует — повторный вызов мгновенный)
        modelObj.pos(0, 0, 0).scale(0.3f, 0.3f, 0.3f);

        modelLoaded = true;
        statusMsg = "Loaded: " + fs::path(path).filename().string();
        std::cout << "[OK] " << statusMsg << "\n";
        app.window().setTitle("Model Viewer  —  " + fs::path(path).filename().string());
    };

    loadModel(modelPath);

    // ── Обновление каждый кадр ─────────────────────────────────────────────
    app.onUpdate([&](float dt) {
        // Esc — выход
        if (Input::isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(app.window().handle(), 1);

        // R — сброс камеры
        if (Input::isKeyPressed(GLFW_KEY_R)) {
            app.camPos(0, 1.5f, 4.f).camRot(-90.f, -12.f);
        }

        // F — wireframe
        if (Input::isKeyPressed(GLFW_KEY_F)) {
            wireframe = !wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }

        // Space — авто-вращение
        if (Input::isKeyPressed(GLFW_KEY_SPACE))
            autoRotate = !autoRotate;

        // Вращение модели
        if (modelObj.valid() && autoRotate)
            modelObj.rotY(dt * 25.f);
    });

    // ── HUD ────────────────────────────────────────────────────────────────
    app.onRender([&]() {
        int W = app.window().width(), H = app.window().height();
        ui.begin();

        // Top bar
        ui.rect(0, 0, (float)W, 32, {0, 0, 0, 0.65f});
        ui.text("MODEL VIEWER", 10, 8, {0.5f, 0.8f, 1.f, 1.f}, 2.f);

        // FPS
        char fb[32]; snprintf(fb, sizeof(fb), "FPS: %.0f", Time::fps());
        ui.text(fb, W - (int)ui.textWidth(fb, 1.5f) - 10, 9, {0.4f, 0.9f, 0.4f, 1.f}, 1.5f);

        // Status (имя файла)
        ui.rect(0, (float)H - 28, (float)W, 28, {0, 0, 0, 0.6f});
        ui.text(statusMsg, 10, (float)H - 20,
            modelLoaded ? glm::vec4{0.6f, 0.9f, 0.5f, 1.f}
                        : glm::vec4{0.9f, 0.5f, 0.3f, 1.f}, 1.5f);

        // Badges
        if (wireframe) {
            ui.rect((float)W / 2 - 60, 36, 120, 18, {0.8f, 0.5f, 0.1f, 0.9f});
            ui.textCentered("[WIREFRAME]", (float)W / 2, 40, {1, 1, 1, 1}, 1.5f);
        }
        if (!autoRotate) {
            ui.rect(10, 36, 100, 18, {0.3f, 0.3f, 0.4f, 0.8f});
            ui.text("[PAUSED]", 14, 40, {0.8f, 0.8f, 0.5f, 1.f}, 1.5f);
        }

        // Controls hint (правый нижний угол)
        const char* hint = "Tab=Cam  R=Reset  F=Wire  Space=Spin  Esc=Quit";
        ui.text(hint, W - (int)ui.textWidth(hint, 1.f) - 10,
                (float)H - 20, {0.4f, 0.4f, 0.5f, 0.7f}, 1.f);

        ui.end();
    });

    return app.run();
}
