// editor_main.cpp — Jenuroix Editor (standalone application)
// Runs as a standalone exe, like the Unity Editor.
// Empty scene on start. You can create objects, edit,
// generate main.cpp, compile and run.

#include "engine.h"
#include "editor/configure.h"
#include "editor/SceneEditor.h"
#include "editor/CodeGenerator.h"
#include "editor/ProjectManager.h"
#include "ui/UI.h"
using namespace eng;

int main() {
    App::Config cfg;
    cfg.title     = EngineConfig::editorTitleFull();
    cfg.width     = 1440;
    cfg.height    = 900;
    cfg.vsync     = false;
    cfg.targetFps = 144;
    App app(cfg);
    app.skyColor(0.13f, 0.14f, 0.18f)
       .lightDir(0.4f, 1.0f, 0.5f)
       .lightColor(1.0f, 0.97f, 0.9f)
       .ambient(0.18f, 0.18f, 0.22f);

    app.camPos(0, 4, 10).camRot(-90.0f, -20.0f).freeCam(true);

    // Apply default editor theme (Cyberpunk — yellow/cyan)
    // Change in View → Theme at runtime
    UI::ApplyTheme(UI::Themes::Dark());

    // Empty scene — just the base floor
    app.spawn("DirectionalLight").color(1.f, 0.97f, 0.9f);
    app.spawn("Floor")
       .cube().scale(20, 0.1f, 20)
       .color(0.18f, 0.18f, 0.22f)
       .pos(0, -0.05f, 0);

    // Editor systems
    SceneEditor   editor (app);
    CodeGenerator codegen(app);
    ProjectManager proj  (app);

    proj.load("project.jenuroix"); // load last project if it exists

    // Wire File > New Scene → creates a new scene tab instead of wiping the world
    proj.onNewScene([&]() { editor.sceneTabs_newTab(); });

    // Wire UI widgets so codegen includes them in generated code
    codegen.setUIWidgets(&editor.uiWidgets());

    app.onRender([&]() {
        // Keep codegen in sync with all scene tabs every frame
        static SceneSnapshot s_activeSnap;
        s_activeSnap = editor.activeSnapshot();
        codegen.setSceneTabs(&editor.tabs(), &s_activeSnap, editor.activeTabIdx());

        proj   .drawMenuBar();   // must be first — reserves top menu bar space
        editor .draw();          // hierarchy + toolbar + tabs + inspector + gizmo
        codegen.drawPanel();     // bottom code panel
    });

    return app.run();
}
