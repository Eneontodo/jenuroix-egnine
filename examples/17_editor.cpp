// ============================================================
//  Example 17 — In-Game Scene Editor
//
//  Embeds a full scene editor inside any game app:
//    Hierarchy  — object list, selection, rename
//    Inspector  — Transform, Material, RigidBody editing
//    Toolbar    — create objects, Undo/Redo
//    Stats      — FPS, object count
//
//  Hotkeys:
//    F1       — show / hide editor panels
//    Delete   — delete selected object
//    Ctrl+D   — duplicate selected
//    Ctrl+Z   — undo
//    F        — focus camera on selection
//    W/E/R    — gizmo mode (Translate / Rotate / Scale)
// ============================================================
#include "engine.h"
#include "editor/SceneEditor.h"
using namespace eng;

int main() {
    App app("Scene Editor Demo", 1280, 720);
    app.skyColor(0.18f, 0.2f, 0.28f)
       .lightDir(0.5f, 1.f, 0.4f)
       .lightColor(1.f, 0.95f, 0.88f)
       .ambient(0.2f, 0.2f, 0.25f);
    app.camPos(0, 4, 10).camRot(-90.f, -20.f).freeCam(true);

    // ── Initial scene objects ─────────────────────────────────────────────
    app.spawn("Floor")
       .cube().scale(20, 0.2f, 20)
       .color(0.22f, 0.22f, 0.26f)
       .pos(0, -0.1f, 0);

    app.spawn("RedCube")
       .cube().scale(1.f)
       .color(0.85f, 0.25f, 0.2f)
       .pos(-3, 0.5f, 0);

    app.spawn("GreenSphere")
       .sphere().scale(0.8f)
       .color(0.2f, 0.75f, 0.3f)
       .pos(0, 0.5f, 0);

    app.spawn("BlueCube")
       .cube().scale(1.2f)
       .color(0.2f, 0.4f, 0.9f)
       .pos(3, 0.6f, 0);

    app.spawn("YellowBox")
       .cube().scale(0.6f, 1.4f, 0.6f)
       .color(0.9f, 0.75f, 0.1f)
       .pos(0, 0.7f, -3);

    // ── Editor ───────────────────────────────────────────────────────────
    SceneEditor editor(app);

    // Optional callbacks — log selection/creation/deletion
    editor.onSelect([](EntityID id) {
        LOG_INFO("Selected entity: " << id);
    });
    editor.onCreate([](EntityID id) {
        LOG_INFO("Created entity: " << id);
    });
    editor.onDelete([](EntityID id) {
        LOG_INFO("Deleted entity: " << id);
    });

    // Draw editor panels every frame
    app.onRender([&]() {
        editor.draw();
    });

    return app.run();
}
