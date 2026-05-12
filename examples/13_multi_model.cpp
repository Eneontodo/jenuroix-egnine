// ╔══════════════════════════════════════════════════════════════════════════╗
// ║  13_multi_model.cpp  —  Multiple models in one scene                 ║
// ║  Features: загрузка нескольких моделей, позиция, масштаб, поворот     ║
// ╚══════════════════════════════════════════════════════════════════════════╝
#include "engine.h"
#include <filesystem>
#include <vector>
#include <string>
namespace fs = std::filesystem;
using namespace eng;

int main() {
    App app("13 Multi Model", 1280, 720);
    g_app = &app;

    app.skyColor(0.08f, 0.09f, 0.14f);
    app.lightDir(0.4f, 1.f, 0.5f).lightColor(1.f, 0.95f, 0.88f).ambient(0.12f, 0.12f, 0.18f);
    app.camPos(0, 3.f, 10.f).camRot(-90.f, -17.f);
    app.freeCam(true);

    // Пол
    app.spawn("Floor").grid(24, 24, 1.f).color(0.15f, 0.15f, 0.18f).pos(0, 0, 0).scale(12, 1, 12);

    // ────────────────────────────────────────────────────────────────────────
    //  Вариант A: явно указать список моделей
    // ────────────────────────────────────────────────────────────────────────
    struct ModelPlacement {
        std::string path;
        glm::vec3   position;
        float       scale;
        glm::vec3   color;       // используется если модель белая (нет .mtl)
    };

    std::vector<ModelPlacement> placements = {
        // путь к файлу,         позиция,          масштаб,  запасной цвет
        { "assets/models/phone.obj",  {-3.f, 0.f, 0.f},  0.3f,  {0.7f,0.5f,0.3f} },
        { "assets/models/phone.obj",  { 0.f, 0.f, 0.f},  0.4f,  {0.3f,0.6f,0.9f} },
        { "assets/models/phone.obj",  { 3.f, 0.f, 0.f},  0.25f, {0.9f,0.4f,0.3f} },
        //
        // Добавляй свои:
        // { "assets/models/sword.obj",  { 0.f, 1.f, -4.f}, 1.f,   {0.8f,0.8f,0.8f} },
        // { "assets/models/chest.gltf", { 3.f, 0.f, -4.f}, 0.5f,  {1.f, 1.f, 1.f}  },
    };

    std::vector<GameObject> models;
    int loaded = 0, failed = 0;

    for (auto& p : placements) {
        auto obj = app.spawn("Model_" + std::to_string(models.size()));

        if (fs::exists(p.path)) {
            obj.model(p.path);   // ← загрузка модели
            loaded++;
        } else {
            // Файла нет — куб с цветом как подсказка
            obj.cube(0.8f);
            failed++;
        }

        obj.color(p.color.r, p.color.g, p.color.b)
           .pos(p.position.x, p.position.y, p.position.z)
           .scale(p.scale, p.scale, p.scale);

        // Разная скорость вращения у каждого
        float speed = 15.f + models.size() * 7.f;
        obj.onUpdate([speed](GameObject self, float dt) {
            self.rotY(dt * speed);
        });

        models.push_back(obj);
    }

    // ────────────────────────────────────────────────────────────────────────
    //  Вариант B: загрузить ВСЁ что есть в assets/models/
    // ────────────────────────────────────────────────────────────────────────
    //  Раскомментируй если хочешь автоматически подгрузить все файлы:
    /*
    std::vector<std::string> allModels;
    try {
        for (auto& e : fs::directory_iterator("assets/models")) {
            auto ext = e.path().extension().string();
            if (ext == ".obj" || ext == ".gltf" || ext == ".glb")
                allModels.push_back(e.path().string());
        }
    } catch (...) {}

    for (int i = 0; i < (int)allModels.size(); i++) {
        float x = (i - (int)allModels.size() / 2) * 3.f;
        auto obj = app.spawn("Auto_" + std::to_string(i));
        obj.model(allModels[i]).pos(x, 0, -5.f).scale(0.3f, 0.3f, 0.3f);
        obj.onUpdate([](GameObject self, float dt){ self.rotY(dt * 20.f); });
    }
    */

    // Вывод статистики
    char title[128];
    snprintf(title, sizeof(title),
        "13 Multi Model  |  Loaded: %d  Failed: %d  |  Tab=Camera", loaded, failed);
    app.window().setTitle(title);

    // Декоративный свет (вращающаяся точечная лампа)
    auto lamp = app.spawn("Lamp");
    lamp.sphere(0.1f, 8).color(1.f, 0.95f, 0.5f);
    lamp.onUpdate([](GameObject self, float) {
        float t = (float)glfwGetTime();
        self.pos(cosf(t * 0.7f) * 4.f, 3.f, sinf(t * 0.7f) * 4.f);
    });

    app.onUpdate([&](float) {
        if (Input::isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(app.window().handle(), 1);
    });

    return app.run();
}
