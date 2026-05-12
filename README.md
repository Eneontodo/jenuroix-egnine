# Jenuroix Egnine

> A lightweight, batteries-included 3D game engine written in C++17.  
> Built on OpenGL · GLFW · Dear ImGui · Jolt Physics · miniaudio.

```cpp
#include "engine.h"
using namespace eng;

int main() {
    App app("Hello Engine");
    app.spawn("Box").cube().color(0.3f, 0.6f, 1.0f);
    return app.run();
}
```

---

## Features

| Category | What you get |
|---|---|
| **Rendering** | OpenGL 3.3 core, directional light, ambient, per-object color/material |
| **Scene** | Entity-Component system, hierarchy, spawn/destroy, show/hide |
| **Camera** | Free-cam, top-down, FPS controller, FOV, aspect ratio |
| **Physics** | Jolt Physics integration — static, dynamic, kinematic bodies, raycasting, collision callbacks |
| **UI** | Dear ImGui wrappers — menus, HUD, inspector, overlays |
| **Editor** | In-game scene editor — hierarchy, inspector, gizmos, undo/redo, save/load |
| **Scripting** | Per-object `onUpdate` lambdas, `EventBus` for typed events |
| **Models** | `.obj` / `.gltf` / `.glb` loading via tinyobjloader & tinygltf |

---

## Quick Start

### Prerequisites

| Tool | Minimum version |
|---|---|
| C++ compiler | GCC 11 / Clang 14 / MSVC 2022 |
| CMake | 3.21 |
| OpenGL driver | 3.3 core |

Optional: [Jolt Physics](https://github.com/jrouwe/JoltPhysics) for the physics examples.  

### Build

**Linux / macOS**
```bash
git clone https://github.com/your-org/jenuroix-engine.git
cd jenuroix-engine
chmod +x bootstrap.sh && ./bootstrap.sh
```

**Windows**
```bat
git clone https://github.com/your-org/jenuroix-engine.git
cd jenuroix-engine
bootstrap.bat
```

The scripts run `cmake -B build -DCMAKE_BUILD_TYPE=Release` and then build.  
Binaries land in `build/bin/`.

### Run an example

```bash
./build/bin/01_hello          # Hello World
./build/bin/07_full_game      # DODGE! mini-game
./build/bin/17_editor         # In-game scene editor
```

---

## Examples

| # | File | What it shows |
|---|---|---|
| 01 | `01_hello.cpp` | Minimal app — window + cube |
| 02 | `02_movement.cpp` | WASD movement, top-down camera |
| 03 | `03_scripting.cpp` | Per-object `onUpdate` lambdas |
| 04 | `04_collision.cpp` | Simple overlap-based coin pickup |
| 05 | `05_events.cpp` | `EventBus` key-press events |
| 06 | `06_model.cpp` | Load `.obj` / `.gltf` / `.glb` models |
| 07 | `07_full_game.cpp` | Complete mini-game: **DODGE!** |
| 08 | `08_ui_menu.cpp` | Main menu, settings, pause screen |
| 09 | `09_player_fps.cpp` | FPS controller — gravity, jump, sprint, head-bob |
| 10 | `10_player_topdown.cpp` | Top-down shooter with enemies and HP |
| 11 | `11_ui_inspector.cpp` | Live object inspector panel |
| 12 | `12_model_viewer.cpp` | Drag-and-drop model viewer |
| 13 | `13_multi_model.cpp` | Multiple models in one scene |
| 14 | `14_model_texture.cpp` | Textured models |
| 15 | `15_physics.cpp` | Jolt Physics — stacking, raycasting, impulses |
| 16 | `16_audio.cpp` | miniaudio — music, SFX, 3D positional sound |
| 17 | `17_editor.cpp` | In-game scene editor (F1 to toggle) |
| 18 | `18_platformer.cpp` | Physics platformer with moving platforms |
| 19 | `19_editor_app.cpp` | Standalone editor application |
| 20 | `20_scripts.cpp` | Library of 30 ready-to-use behaviour scripts |
| 21 | `21_day_night.cpp` | Day/night cycle — dynamic sky, sun/moon orbit |
| 22 | `22_particles.cpp` | CPU particle system — fire, snow, explosions |
| 23 | `23_inventory.cpp` | Grid inventory UI with hotbar and tooltips |

---

## API Overview

### App

```cpp
App app("My Game", 1280, 720);      // create window
app.skyColor(r, g, b);              // background / sky colour
app.lightDir(x, y, z);             // directional light direction
app.lightColor(r, g, b);           // sun colour
app.ambient(r, g, b);              // ambient fill light
app.camPos(x, y, z);               // initial camera position
app.camRot(yaw, pitch);            // initial camera angles
app.freeCam(true);                  // enable free-cam (Tab + WASD)
app.fov(75.f);                      // field of view in degrees

app.onStart([&]()      { /* runs once before first frame  */ });
app.onUpdate([&](float dt) { /* runs every frame         */ });
app.onRender([&]()     { /* runs after 3D, before swap   */ });

return app.run();                   // enter the game loop
```

### GameObject

```cpp
auto obj = app.spawn("Name");       // create an entity

// Shapes (chain-able)
obj.cube();                         // unit cube
obj.cube(0.5f);                     // cube with half-size
obj.sphere();                       // unit sphere
obj.sphere(0.4f, 16);              // sphere with radius + segment count
obj.grid(cols, rows, cellSize);    // wireframe grid
obj.model("assets/models/x.obj"); // load from file

// Transform
obj.pos(x, y, z);
obj.pos(glm::vec3{...});
obj.move(dx, dy, dz);
obj.scale(sx, sy, sz);
obj.rotX(deg); obj.rotY(deg); obj.rotZ(deg);
obj.rot(rx, ry, rz);

// Appearance
obj.color(r, g, b);
obj.show(); obj.hide();

// Collision
obj.addCollider();
bool hit = obj.overlaps(other);

// Scripting
obj.onUpdate([](GameObject& self, float dt) {
    self.rotY(self.rot().y + 90.f * dt);
});

// Queries
glm::vec3 p   = obj.pos();
glm::vec3 r   = obj.rot();
glm::vec3 s   = obj.scale();
EntityID  id  = obj.id();
bool      ok  = obj.valid();

obj.destroy();
```

### Input

```cpp
app.key(KEY_W)          // held this frame
app.keyDown(KEY_SPACE)  // pressed this frame (edge)
app.keyUp(KEY_SPACE)    // released this frame
app.mouseDown(0)        // left mouse button down

app.mouseDelta()        // glm::vec2 mouse delta since last frame
```

### Camera

```cpp
auto& cam = app.camera();
cam.setPosition({x, y, z});
cam.setRotation(yaw, pitch);
cam.setFov(75.f);
cam.processMouse(dx, dy);
cam.processKeyboard(fwd, right, up, speed, dt);
glm::vec3 pos = cam.position();
float     fov = cam.fov();
```

### Physics (Jolt)

```cpp
#include "physics/Physics.h"

PhysicsWorld& phys = app.physics();

app.addRigidBody(obj.id(),
    RigidBodyType::Dynamic,     // Static | Dynamic | Kinematic
    ColliderShape::Box,         // Box | Sphere | Capsule
    {halfX, halfY, halfZ},      // half-extents
    mass);                      // kg (ignored for Static)

phys.applyImpulse(id, glm::vec3{0, 10, 0});

RayHit hit = phys.raycast(origin, direction, maxDist);
if (hit) { hit.entityID; hit.distance; }

phys.onCollisionEnter([](const HitEvent& e) {
    // e.entityA, e.entityB, e.impulse
});
```

### Audio (miniaudio)

```cpp
#include "audio/Audio.h"

SoundHandle music = Audio::loop("assets/sounds/music.ogg", volume);
SoundHandle sfx   = Audio::play("assets/sounds/hit.wav");
SoundHandle amb   = Audio::loop3D("assets/sounds/ambient.ogg",
                                   position, volume, maxDist);

Audio::pause(music);
Audio::resume(music);
Audio::setVolume(music, 0.5f);
Audio::setPitch(music,  1.25f);
Audio::setPosition(amb, newPos);
```

### UI

```cpp
// Fixed overlay window (HUD)
if (UI::BeginFixed("##hud", x, y, w, h, flags)) {
    UI::LabelColored(UI::GREEN, "FPS: %.0f", Time::fps());
    UI::LabelColored(UI::GRAY,  "some text");
}
UI::End();

// Centred menu
if (UI::BeginMainMenu("##menu")) {
    UI::Title("MY GAME");
    if (UI::Button("Play"))    { ... }
    if (UI::Button("Quit"))    app.quit();
    UI::Separator();
    UI::SliderFloat("FOV", fov, 40, 120);
    UI::Bool("VSync", vsync);
    UI::EndMainMenu();
}

// Dark translucent background overlay
UI::DarkOverlay(0.6f);  // call before BeginMainMenu

// Colour constants
UI::WHITE  UI::GRAY  UI::GREEN  UI::RED  UI::YELLOW  UI::CYAN  UI::BLUE
```

### Events

```cpp
// Subscribe
Events().on<KeyPressedEvent>([](const KeyPressedEvent& e) {
    if (e.key == KEY_SPACE) { ... }
});

// Emit
Events().emit(KeyPressedEvent{KEY_SPACE, false});
```

### Scene Editor

```cpp
#include "editor/SceneEditor.h"

SceneEditor editor(app);

editor.onSelect([](EntityID id) { ... });
editor.onCreate([](EntityID id) { ... });
editor.onDelete([](EntityID id) { ... });

app.onRender([&]() {
    editor.draw();          // draws all editor panels
});
// F1 toggles panel visibility at runtime
```

---

## Project Layout

```
jenuroix-engine/
├── src/
│   ├── engine.h            ← single include for user code
│   ├── main.cpp            ← engine bootstrap / internal
│   ├── editor/
│   │   ├── SceneEditor.h/.cpp
│   │   ├── ProjectManager.h
│   │   └── configure.h
│   ├── physics/
│   │   └── Physics.h
│   ├── audio/
│   │   └── Audio.h
│   └── ui/
│       ├── UI.h/.cpp
│       └── ImGuiLayer.h
├── examples/
│   ├── 01_hello.cpp
│   ├── 02_movement.cpp
│   └── ... (23 examples total)
├── docs/
│   ├── API.md
│   ├── BUILDING.md
│   └── EDITOR.md
├── assets/                 ← sample assets (CC0)
│   ├── models/
│   └── sounds/
├── CMakeLists.txt
├── bootstrap.sh
├── bootstrap.bat
└── LICENSE
```

---

## Contributing

1. Fork the repository and create a feature branch.
2. Follow the existing code style (C++17, 4-space indent, `snake_case` for variables).
3. Add or update the relevant example if your change affects the public API.
4. Open a pull request with a clear description of what changed and why.

Please keep third-party dependencies minimal — prefer single-header libraries.

---

## License

Released under the **MIT License** — see [LICENSE](LICENSE) for the full text.

Third-party libraries bundled or optionally linked retain their own licenses  
(GLFW, glad, glm, Dear ImGui, stb, tinyobjloader, tinygltf, miniaudio, Jolt, nlohmann/json).  
See [LICENSE](LICENSE) for the complete attribution table.
