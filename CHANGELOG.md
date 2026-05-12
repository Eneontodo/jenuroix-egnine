# Changelog

All notable changes are documented here.  
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [Unreleased]

### Added
- Examples 21–23: Day/Night cycle, CPU particle system, grid inventory UI
- English comments and documentation throughout all 23 examples
- `docs/API.md` — complete API reference
- `docs/BUILDING.md` — platform build guide (Linux, macOS, Windows)
- `docs/EDITOR.md` — scene editor user guide
- MIT `LICENSE` with third-party attribution table

### Changed
- Example 17 (`editor`): removed build/compilation panel and 3D widget
  overlays; editor now focuses on hierarchy, inspector, toolbar, and stats
- Example 19 (`editor_app`): removed `CodeGenerator`; standalone editor
  now uses `SceneEditor` + `ProjectManager` only
- Example 09 (`fps_player`): removed `onRender` HUD block; crosshair and
  status text moved to in-world approach (keeps example self-contained)
- All example comments translated from Russian to English

---

## [0.2.0] — Phase 2

### Added
- `SceneEditor` — full in-game scene editor (hierarchy, inspector, gizmos, undo/redo)
- `ProjectManager` — save/load `.jenuroix` project files
- `PhysicsWorld` — Jolt Physics integration with stub fallback
- `Audio` — miniaudio integration with stub fallback
- Examples 15–19: physics demo, audio demo, embedded editor, physics platformer,
  standalone editor app
- `App::Config` struct for fine-grained window setup
- `RigidBodyType::Kinematic` for driven moving platforms

### Changed
- `App::spawn()` now accepts an optional name string
- `GameObject::sphere()` accepts segment count parameter

---

## [0.1.0] — Phase 1

### Added
- `App` — window, game loop, lifecycle hooks (`onStart`, `onUpdate`, `onRender`)
- `GameObject` — entity handle with shape builders, transform, color, collider
- Free-cam, top-down, and FPS camera modes
- `Events` — typed event bus with lambda subscribers
- `UI::` — Dear ImGui wrappers (menus, HUD, inspector, overlays)
- Examples 01–14: hello world through textured model viewer
- CMake build system with `bootstrap.sh` / `bootstrap.bat`
