// ╔══════════════════════════════════════════════════════════════════════════╗
// ║  14_model_texture.cpp  —  Модель + текстура + материал                  ║
// ║  Показывает как применить текстуру к загруженной модели                 ║
// ╚══════════════════════════════════════════════════════════════════════════╝
#include "engine.h"
#include <filesystem>
namespace fs = std::filesystem;
using namespace eng;

int main() {
    App app("14 Model + Texture", 1280, 720);
    g_app = &app;

    app.skyColor(0.1f, 0.11f, 0.16f);
    app.lightDir(0.5f, 1.f, 0.3f).lightColor(1.f, 0.95f, 0.9f).ambient(0.15f, 0.15f, 0.2f);
    app.camPos(0, 2.f, 5.f).camRot(-90.f, -15.f);
    app.freeCam(true);

    app.spawn("Floor").grid(20, 20, 1.f).color(0.13f, 0.13f, 0.16f).pos(0, 0, 0).scale(8, 1, 8);

    // ── Объект 1: Модель с автоматическим материалом из .mtl ───────────────
    // Если .obj имеет рядом .mtl — цвета берутся оттуда автоматически
    {
        auto obj = app.spawn("ModelWithMTL");
        if (fs::exists("assets/models/phone.obj"))
            obj.model("assets/models/phone.obj");
        else
            obj.cube(0.8f);

        obj.pos(-2.f, 0.f, 0.f).scale(0.3f, 0.3f, 0.3f);
        obj.onUpdate([](GameObject self, float dt){ self.rotY(dt * 20.f); });
    }

    // ── Объект 2: Модель + принудительная текстура ─────────────────────────
    // texture() заменяет albedoMap для всех субмешей
    {
        auto obj = app.spawn("ModelWithTexture");
        if (fs::exists("assets/models/phone.obj"))
            obj.model("assets/models/phone.obj");
        else
            obj.cube(0.8f);

        // Применить текстуру поверх модели:
        if (fs::exists("assets/textures/box_texture.jpg"))
            obj.texture("assets/textures/box_texture.jpg");
        else
            obj.color(0.3f, 0.6f, 0.9f);   // если нет текстуры — цвет

        obj.pos(0.f, 0.f, 0.f).scale(0.3f, 0.3f, 0.3f);
        obj.onUpdate([](GameObject self, float dt){ self.rotY(dt * 20.f); });
    }

    // ── Объект 3: Модель + color() tint ────────────────────────────────────
    // color() задаёт цвет-множитель (tint), работает поверх текстуры
    {
        auto obj = app.spawn("ModelTinted");
        if (fs::exists("assets/models/phone.obj"))
            obj.model("assets/models/phone.obj");
        else
            obj.cube(0.8f);

        obj.color(0.9f, 0.4f, 0.2f)          // оранжевый tint
           .pos(2.f, 0.f, 0.f)
           .scale(0.3f, 0.3f, 0.3f);
        obj.onUpdate([](GameObject self, float dt){ self.rotY(dt * 20.f); });
    }

    // Подписи
    auto label = [&](const char* txt, glm::vec3 pos) {
        auto c = app.spawn(txt);
        c.cube(0.05f).color(1.f, 1.f, 0.3f).pos(pos.x, pos.y, pos.z).scale(0.8f, 0.05f, 0.05f);
    };

    app.onUpdate([&](float) {
        if (Input::isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(app.window().handle(), 1);
    });

    app.window().setTitle("14 Model+Texture  |  Left=MTL  Center=Texture  Right=Tint  |  Tab=Cam");
    return app.run();
}
