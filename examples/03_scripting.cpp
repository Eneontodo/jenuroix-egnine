// ============================================================
//  Example 03 — Per-Object Scripts
//  Each object knows how to behave via onUpdate lambdas
// ============================================================
#include "engine.h"
#include <cmath>
using namespace eng;

int main() {
    App app("Scripting");

    // Ground
    app.spawn("Ground")
       .grid(20, 20)
       .color(0.2f, 0.35f, 0.2f)
       .pos(0, -0.5f, 0);

    // Spinning cube — rotates 90 degrees per second around Y
    app.spawn("RotatingCube")
       .cube()
       .color(1.f, 0.3f, 0.3f)
       .onUpdate([](GameObject& self, float dt) {
           self.rotY(self.rot().y + 90.f * dt);
       });

    // Bouncing sphere — oscillates vertically with a sine wave
    float t = 0.f;
    app.spawn("BounceSphere")
       .sphere()
       .color(0.3f, 0.6f, 1.f)
       .pos(3.f, 0.f, 0.f)
       .onUpdate([&t](GameObject& self, float dt) {
           t += dt;
           self.y(std::abs(std::sin(t * 2.f)) * 2.f);
       });

    // Orbiting cube — circles the origin at radius 4
    float angle = 0.f;
    app.spawn("Orbiter")
       .cube(0.4f)
       .color(1.f, 1.f, 0.2f)
       .onUpdate([&angle](GameObject& self, float dt) {
           angle += dt;
           self.pos(std::cos(angle) * 4.f, 1.f, std::sin(angle) * 4.f);
       });

    return app.run();
}
