// ============================================================
//  Example 19 — Standalone Scene Editor Application
//
//  A complete editor-mode app with:
//    - Scene hierarchy and inspector
//    - Multi-tab scene management
//    - Save / load scene (*.jenuroix project files)
//    - Menu bar (File / Edit / View / Help)
//
//  Press F1 to toggle editor panels.
//  All editing happens through the SceneEditor UI.
// ============================================================
#include "engine.h"
#include "editor/SceneEditor.h"
#include "editor/ProjectManager.h"
using namespace eng;

int main() {
    App::Config cfg;
    cfg.title     = "Jenuroix Editor";
    cfg.width     = 1440;
    cfg.height    = 900;
    cfg.vsync     = false;
    cfg.targetFps = 144;
    App app(cfg);

    app.skyColor(0.13f, 0.14f, 0.18f)
       .lightDir(0.4f, 1.f, 0.5f)
       .lightColor(1.f, 0.97f, 0.9f)
       .ambient(0.18f, 0.18f, 0.22f);
    app.camPos(0, 4, 10).camRot(-90.f, -20.f).freeCam(true);

    // Default starter floor so the viewport is not empty
    app.spawn("Floor")
       .cube().scale(20, 0.1f, 20)
       .color(0.18f, 0.18f, 0.22f)
       .pos(0, -0.05f, 0);

    // ── Editor + project manager ──────────────────────────────────────────
    SceneEditor    editor(app);
    ProjectManager proj(app);

    // Restore last session if a project file exists
    proj.load("project.jenuroix");

    // Wire project manager's "New Scene" action to editor's tab system
    proj.onNewScene([&]() { editor.sceneTabs_newTab(); });

    app.onRender([&]() {
        proj.drawMenuBar(); // must draw first (owns the main menu bar)
        editor.draw();
    });

    return app.run();
}
