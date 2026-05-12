# Scene Editor Guide

Jenuroix ships with a built-in scene editor that can be embedded in any
app or run as a standalone application.

---

## Two Modes

| Mode | Example | Description |
|---|---|---|
| **Embedded** | `17_editor.cpp` | Editor panels appear inside your game; press F1 to toggle |
| **Standalone** | `19_editor_app.cpp` | Editor launches as the primary application with a menu bar |

---

## Panels

### Hierarchy

Lists every entity in the scene.

- Click to select an entity.
- Double-click to rename.
- **+ Cube / + Sphere / + Empty** buttons create new entities.
- **Delete** key or the bin button removes the selected entity.
- Ctrl+D duplicates the selection.

### Inspector

Shows and edits components of the selected entity.

**Transform**

| Field | Description |
|---|---|
| Position | World-space XYZ |
| Rotation | Euler angles (degrees) |
| Scale | Per-axis scale factors |

**Material**

| Field | Description |
|---|---|
| Color | RGB color picker |
| Visible | Show / hide toggle |

**RigidBody** (only when a physics body exists)

| Field | Description |
|---|---|
| Type | Static / Dynamic / Kinematic |
| Mass | Kilograms |
| Restitution | Bounciness 0–1 |

### Toolbar

- **Play / Pause / Stop** — run the scene in-place (simulation mode).
- **Undo / Redo** — Ctrl+Z / Ctrl+Y.
- **Gizmo buttons** — W (translate), E (rotate), R (scale).

### Stats

Compact overlay in the corner:

```
FPS: 144        Objects: 12
Physics bodies: 8         Audio sources: 3
```

---

## Gizmos

With the editor open and an entity selected, use the gizmo to transform
it directly in the viewport.

| Key | Gizmo |
|---|---|
| W | Move (arrows along each axis) |
| E | Rotate (rings around each axis) |
| R | Scale (handles on each axis) |

Press F to fly the camera to the selected object.

Hold **Ctrl** while dragging to snap to a grid (default 0.25 units for
translate, 5° for rotate, 0.1 for scale). Configure snap values via the
View menu.

---

## Save & Load

The standalone editor (`19_editor_app.cpp`) uses `ProjectManager` to
persist the scene.

```
File → Save            Ctrl+S   Save to project.jenuroix
File → Save As…                 Choose a path
File → Open…           Ctrl+O   Load a .jenuroix file
File → New Scene                Clear and start fresh
```

The `.jenuroix` format is JSON — you can inspect and edit it manually.

---

## Embedding in Your Game

```cpp
#include "engine.h"
#include "editor/SceneEditor.h"
using namespace eng;

int main() {
    App app("My Game", 1280, 720);

    // ... spawn your objects ...

    SceneEditor editor(app);

    app.onRender([&]() {
        editor.draw();   // draws all panels; F1 hides/shows them
    });

    return app.run();
}
```

The editor is zero-cost when hidden — `draw()` returns immediately if the
panels are toggled off.

---

## Callbacks

```cpp
editor.onSelect([](EntityID id) {
    // fired when user clicks an entity in the Hierarchy
});
editor.onCreate([](EntityID id) {
    // fired when the editor spawns a new entity
});
editor.onDelete([](EntityID id) {
    // fired just before an entity is destroyed
});
```

---

## Hotkey Reference

| Key | Action |
|---|---|
| F1 | Show / hide all editor panels |
| Delete | Delete selected entity |
| Ctrl+D | Duplicate selected entity |
| Ctrl+Z | Undo last action |
| Ctrl+Y | Redo |
| F | Focus camera on selection |
| W | Switch to Translate gizmo |
| E | Switch to Rotate gizmo |
| R | Switch to Scale gizmo |
| Ctrl+S | Save project (standalone mode) |
| Ctrl+O | Open project (standalone mode) |
