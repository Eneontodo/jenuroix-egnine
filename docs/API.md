# API Reference

Full reference for `engine.h` and the optional subsystem headers.  
All public symbols live in the `eng` namespace.

---

## App

```cpp
// Construction
App app("Title");                   // 800×600 window, defaults
App app("Title", width, height);    // custom size

App::Config cfg;
cfg.title     = "My Game";
cfg.width     = 1440;
cfg.height    = 900;
cfg.vsync     = false;
cfg.targetFps = 144;
App app(cfg);
```

### Lighting

```cpp
app.skyColor(r, g, b);         // sky / clear colour
app.lightDir(x, y, z);        // directional light direction (need not be normalised)
app.lightColor(r, g, b);      // directional light colour (1,1,1 = white)
app.ambient(r, g, b);         // constant ambient fill
```

### Camera shortcuts

```cpp
app.camPos(x, y, z);          // set camera world position
app.camRot(yaw, pitch);       // set camera orientation (degrees)
app.freeCam(bool);            // enable WASD + mouse free-cam
app.fov(degrees);             // set field of view
app.camera();                 // → Camera& for fine-grained control
```

### Lifecycle hooks

```cpp
app.onStart ([&]()         { /* called once before the first frame  */ });
app.onUpdate([&](float dt) { /* called every frame; dt = seconds    */ });
app.onRender([&]()         { /* called after 3D render, before swap  */ });
// onRender is the correct place for all UI / ImGui drawing.
```

### Input queries (call inside onUpdate)

```cpp
app.key     (int glfwKey)  → bool   // held
app.keyDown (int glfwKey)  → bool   // pressed this frame (rising edge)
app.keyUp   (int glfwKey)  → bool   // released this frame
app.mouseDown(int btn)     → bool   // 0=left 1=right 2=middle
app.mouseDelta()           → glm::vec2
```

### Utilities

```cpp
app.run()     → int        // start the game loop; returns 0 on clean exit
app.quit()                 // request shutdown at end of current frame
app.world()   → ECS&       // access the entity-component world directly
app.physics() → PhysicsWorld&
app.window()  → Window&
```

---

## GameObject

Returned by `app.spawn(name)` or `app.spawn()`.  
Internally a lightweight handle — cheap to copy and store.

### Shape builders (chain-able, return `GameObject&`)

```cpp
.cube()                    // 1×1×1 cube
.cube(size)                // uniform cube
.sphere()                  // unit sphere, 16 segments
.sphere(radius, segments)
.grid(cols, rows)          // wireframe grid
.grid(cols, rows, cellSize)
.model(path)               // load .obj / .gltf / .glb
```

### Transform (chain-able)

```cpp
.pos(x, y, z)
.pos(glm::vec3)
.x(v)  .y(v)  .z(v)       // set individual axes
.move(dx, dy, dz)          // translate by delta
.move(glm::vec3)
.scale(s)                  // uniform scale
.scale(sx, sy, sz)
.rotX(deg) .rotY(deg) .rotZ(deg)   // set rotation around axis
.addRot(rx, ry, rz)        // add to current rotation (degrees/frame)
.rot(rx, ry, rz)           // set all rotation axes at once
```

### Appearance

```cpp
.color(r, g, b)
.show()
.hide()
```

### Collision

```cpp
.addCollider()             // attach a simple bounding-box collider
.overlaps(other)  → bool  // broad-phase overlap test
```

### Per-object script

```cpp
.onUpdate([](GameObject& self, float dt) {
    // runs every frame; self is this object
});
```

### Getters

```cpp
.pos()    → glm::vec3
.rot()    → glm::vec3      // Euler angles in degrees
.scale()  → glm::vec3
.id()     → EntityID
.valid()  → bool           // false after destroy()
.destroy()
```

---

## Camera

Accessed via `app.camera()`.

```cpp
cam.setPosition(glm::vec3);
cam.setRotation(yaw, pitch);         // degrees
cam.setFov(degrees);
cam.setAspect(w / h);

cam.position()  → glm::vec3
cam.fov()       → float
cam.forward()   → glm::vec3         // normalised look direction
cam.right()     → glm::vec3
cam.up()        → glm::vec3

// High-level helpers
cam.processMouse(dx, dy, sensitivity = 0.1f);
cam.processKeyboard(fwd, right, up, speed, dt);
```

---

## Physics (`physics/Physics.h`)

Wraps Jolt Physics. Falls back to a no-op stub if Jolt is not available.

### Adding bodies

```cpp
PhysicsWorld& phys = app.physics();

app.addRigidBody(
    entityID,
    RigidBodyType::Dynamic,    // Static | Dynamic | Kinematic
    ColliderShape::Box,        // Box | Sphere | Capsule
    halfExtents,               // glm::vec3 — half-size of the shape
    mass                       // float kg (ignored for Static)
);
```

### Queries & forces

```cpp
phys.applyImpulse(entityID, glm::vec3 impulse);
phys.setLinearVelocity(entityID, glm::vec3 vel);
phys.getLinearVelocity(entityID) → glm::vec3;

RayHit hit = phys.raycast(origin, direction, maxDistance);
// hit.operator bool() — true if something was hit
// hit.entityID, hit.distance, hit.normal
```

### Callbacks

```cpp
phys.onCollisionEnter([](const HitEvent& e) {
    // e.entityA, e.entityB  — the two bodies
    // e.impulse             — contact impulse magnitude (N·s)
});
```

### State

```cpp
phys.isInitialized() → bool    // false when running without Jolt
phys.bodyCount()     → int
phys.step(dt)                  // called automatically by App::run()
```

---

## Audio (`audio/Audio.h`)

Wraps miniaudio. Falls back to stubs if `miniaudio.h` is not present.

### Playback

```cpp
SoundHandle Audio::play  (path, volume = 1.f)                      → SoundHandle
SoundHandle Audio::loop  (path, volume = 1.f)                      → SoundHandle
SoundHandle Audio::loop3D(path, position, volume = 1.f, maxDist = 20.f) → SoundHandle
// INVALID_SOUND is returned when miniaudio is unavailable
```

### Control

```cpp
Audio::pause     (SoundHandle);
Audio::resume    (SoundHandle);
Audio::stop      (SoundHandle);
Audio::setVolume (SoundHandle, float 0–1);
Audio::setPitch  (SoundHandle, float);    // 1.0 = normal
Audio::setPosition(SoundHandle, glm::vec3); // update 3D source position
```

### State

```cpp
Audio::isInitialized()  → bool
Audio::activeSoundCount() → int
```

---

## UI Wrappers (`UI::`)

Thin helpers over Dear ImGui. Call only inside `app.onRender()`.

### Overlay window

```cpp
bool UI::BeginFixed(label, x, y, w, h, flags) → bool
// flags: combine ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | etc.
void UI::End()
```

### Centred menu window

```cpp
bool UI::BeginMainMenu(label)   → bool
void UI::EndMainMenu()
void UI::DarkOverlay(alpha)     // full-screen tinted background; call before BeginMainMenu
```

### Widgets (inside any Begin/End)

```cpp
void UI::Title(text)
void UI::Header(text)
void UI::Separator()
void UI::Spacing()
void UI::SameLine()

bool UI::Button     (label)                           → bool  // clicked
bool UI::SmallButton(label)                           → bool
bool UI::Section    (label)                           → bool  // collapsible header

void UI::LabelColored(ImVec4 color, fmt, ...)         // printf-style
void UI::SliderFloat (label, float& val, min, max)
void UI::SliderInt   (label, int& val, min, max)
void UI::Bool        (label, bool& val)               // checkbox
void UI::Dropdown    (label, int& idx, const char** items, count)
```

### Color constants

```cpp
UI::WHITE  = {1,1,1,1}
UI::GRAY   = {0.6f,0.6f,0.6f,1}
UI::GREEN  = {0.3f,0.9f,0.3f,1}
UI::RED    = {0.9f,0.3f,0.3f,1}
UI::YELLOW = {0.95f,0.85f,0.2f,1}
UI::CYAN   = {0.3f,0.9f,0.9f,1}
UI::BLUE   = {0.3f,0.5f,0.95f,1}
```

---

## EventBus (`Events()`)

Global singleton. Any number of listeners can subscribe to any event type.

```cpp
// Define a custom event
struct PlayerDiedEvent { int lives; };

// Subscribe (anywhere; captures by ref are safe for objects with longer lifetime)
Events().on<PlayerDiedEvent>([](const PlayerDiedEvent& e) {
    LOG_INFO("Lives remaining: " << e.lives);
});

// Emit
Events().emit(PlayerDiedEvent{2});

// Built-in event types
struct KeyPressedEvent  { int key; bool repeat; };
struct KeyReleasedEvent { int key; };
struct MouseButtonEvent { int button; bool pressed; };
```

---

## Scene Editor (`editor/SceneEditor.h`)

```cpp
SceneEditor editor(app);

// Callbacks (all optional)
editor.onSelect([](EntityID id) { ... });
editor.onCreate([](EntityID id) { ... });
editor.onDelete([](EntityID id) { ... });

// Draw all editor panels (call inside onRender)
editor.draw();

// Multi-tab scene management
editor.sceneTabs_newTab();

// Programmatic selection
editor.select(entityID);
EntityID editor.selected() → EntityID;

// Runtime toggle
// F1 shows/hides all panels
```

### Editor hotkeys

| Key | Action |
|---|---|
| F1 | Show / hide all panels |
| Delete | Delete selected object |
| Ctrl+D | Duplicate selected |
| Ctrl+Z | Undo |
| F | Focus camera on selected |
| W | Translate gizmo |
| E | Rotate gizmo |
| R | Scale gizmo |

---

## Logging

```cpp
LOG_INFO ("message " << value);    // stdout with [INFO] prefix
LOG_WARN ("message");              // stdout with [WARN] prefix
LOG_ERROR("message");              // stderr with [ERROR] prefix
```

---

## Time

```cpp
float Time::fps()           // current frames per second (smoothed)
float Time::delta()         // last frame duration in seconds
double Time::elapsed()      // total seconds since app.run()
```

---

## Constants (key codes)

The engine re-exports GLFW key codes.  
Common aliases defined in `engine.h`:

```cpp
KEY_W   KEY_A   KEY_S   KEY_D
KEY_UP  KEY_DOWN  KEY_LEFT  KEY_RIGHT
KEY_SPACE  KEY_ESC  KEY_SHIFT  KEY_CTRL
KEY_F   KEY_R   KEY_E
```

Use any `GLFW_KEY_*` constant directly for keys without an alias.
