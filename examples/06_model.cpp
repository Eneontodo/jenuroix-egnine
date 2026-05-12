// ============================================================
//  Example 06 — 3D Model Loading
//
//  Supported formats:  .obj   .gltf   .glb
//
//  How to use:
//    1. Place your model file in assets/models/
//    2. Set MODEL_PATH below
//    3. Run — the model appears in the scene
//
//  Controls:
//    Tab   — capture / release cursor (free-cam)
//    WASD  — move camera
//    Mouse — rotate camera
//    Wheel — FOV zoom
//    Esc   — quit
// ============================================================
#include "engine.h"
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;
using namespace eng;

int main() {
    App app("06 Model Loader", 1280, 720);
    g_app = &app;

    // Scene lighting
    app.skyColor(0.08f, 0.1f, 0.15f)
       .lightDir(0.5f, 1.f, 0.4f)
       .lightColor(1.f, 0.95f, 0.88f)
       .ambient(0.12f, 0.12f, 0.18f);
    app.camPos(0, 1.5f, 5.f).camRot(-90.f, -10.f).freeCam(true);

    // Ground plane
    app.spawn("Ground")
       .grid(20, 20, 1.f)
       .color(0.15f, 0.15f, 0.18f)
       .pos(0, -0.01f, 0)
       .scale(10, 1, 10);

    // ── Option A: Load a specific file ────────────────────────────────────
    const std::string MODEL_PATH = "assets/models/phone.obj";
    // Other path examples:
    //   "assets/models/sword.obj"
    //   "assets/models/character.gltf"
    //   "assets/models/car.glb"

    if (fs::exists(MODEL_PATH)) {
        auto modelObj = app.spawn("MyModel");
        modelObj.model(MODEL_PATH)       // key call: load mesh from disk
                .pos(0, 0, 0)
                .scale(0.3f, 0.3f, 0.3f); // adjust scale to fit your model

        // Slow spin so you can inspect from all angles
        modelObj.onUpdate([](GameObject self, float dt) {
            self.rotY(dt * 20.f);
        });

        std::cout << "[OK] Loaded: " << MODEL_PATH << "\n";
    } else {
        // Fallback: colored cube placeholder + console instructions
        std::cout << "[!] Model not found: " << MODEL_PATH << "\n";
        std::cout << "    Put your .obj/.gltf/.glb in assets/models/\n";
        std::cout << "    Current dir: " << fs::current_path() << "\n";

        for (int i = 0; i < 3; ++i) {
            float cols[][3] = {{0.9f,0.2f,0.2f},{0.2f,0.8f,0.3f},{0.2f,0.4f,0.9f}};
            auto c = app.spawn("Placeholder_" + std::to_string(i));
            c.cube(0.5f).color(cols[i][0], cols[i][1], cols[i][2]).pos((i-1)*1.2f, 0, 0);
            c.onUpdate([](GameObject self, float dt) {
                self.rotY(dt * 30.f).rotX(dt * 20.f);
            });
        }
        app.window().setTitle("06 Model — NO MODEL FOUND, see console");
    }

    // ── Option B: Auto-scan assets/models/ and load first match ──────────
    // Uncomment to enable:
    /*
    for (auto& entry : fs::directory_iterator("assets/models")) {
        auto ext = entry.path().extension().string();
        if (ext == ".obj" || ext == ".gltf" || ext == ".glb") {
            app.spawn("AutoModel")
               .model(entry.path().string())
               .pos(0, 0, 0).scale(0.3f, 0.3f, 0.3f)
               .onUpdate([](GameObject self, float dt){ self.rotY(dt * 20.f); });
            break;
        }
    }
    */

    // Decorative orbiting spheres around the model
    for (int i = 0; i < 8; ++i) {
        float phase = (float)i / 8.f * 6.2832f;
        float r = 0.3f + i * 0.05f, b = 0.8f - i * 0.05f;
        auto orb = app.spawn("Orb_" + std::to_string(i));
        orb.sphere(0.08f, 8).color(r, 0.5f, b);
        orb.onUpdate([phase](GameObject self, float) {
            float t = (float)glfwGetTime();
            float a = t * 0.8f + phase;
            self.pos(cosf(a) * 1.8f, sinf(t * 0.5f + phase) * 0.3f, sinf(a) * 1.8f);
        });
    }

    // Update window title with FPS
    app.onUpdate([&](float dt) {
        static float acc = 0.f;
        if ((acc += dt) > 0.3f) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                "06 Model Loader  |  FPS: %.0f  |  Tab=Camera  Esc=Quit",
                Time::fps());
            app.window().setTitle(buf);
            acc = 0.f;
        }
    });

    return app.run();
}
