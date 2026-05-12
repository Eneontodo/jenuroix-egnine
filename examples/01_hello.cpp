// ============================================================
//  Example 01 — Hello World
//  Minimal program: window + a colored cube
// ============================================================
#include "engine.h"
using namespace eng;

int main() {
    App app("Hello Engine");

    // Spawn a blue cube in the center of the scene
    app.spawn("Box")
       .cube()
       .color(0.3f, 0.6f, 1.0f);

    return app.run();
}
