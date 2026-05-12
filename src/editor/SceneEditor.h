#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  SceneEditor.h  —  Built-in scene editor (Phase 2)
//
//  editor in game window, with dockable panels:
//    - Scene Hierarchy  (List all objects, select, rename)
//    - Inspector        (editing Transform, Material, RigidBody)
//    - Toolbar          (Play/Pause/Stop, Create objects)
//    - Stats overlay    (FPS, draw calls, physics bodies, audio sounds)
//
//  connection:
//    #include "editor/SceneEditor.h"
//    SceneEditor editor(app);
//    app.onRender([&](){ editor.draw(); });
//
//  Hot keys:
//    F1          — Show/Hide editor
//    Delete      — Delete selected object
//    Ctrl+D      — Dublicate selected object
//    Ctrl+Z      — undo last action
//    F            — focus camera on selected object
// ─────────────────────────────────────────────────────────────────────────────
#include "engine.h"
#include "editor/Gizmo.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

namespace eng {

// ── Forward ───────────────────────────────────────────────────────────────────
class App;

// ── UndoAction — base for undo/redo stack ─────────────────────────────────────
struct UndoAction {
    std::string description;
    std::function<void()> undo;
    std::function<void()> redo;
};

// ── UIWidget — descriptor for UI Scene canvas widgets ────────────────────────
struct UIWidget {
    enum class Type {
        Button, Label, Slider, TextField, ColoredText,
        Panel, Image, Toggle, ProgressBar, Dropdown
    };

    Type        type      = Type::Label;
    char        text[128] = "Widget";
    char        name[64]  = "Widget";   // hierarchy display name
    ImVec2      pos       = {100, 100};
    ImVec2      size      = {160, 32};
    ImVec4      color     = {1.f, 0.85f, 0.f, 1.f};
    ImVec4      bgColor   = {0.10f, 0.10f, 0.14f, 1.f};
    float       sliderVal = 0.5f;
    float       progress  = 0.5f;
    bool        toggled   = false;
    char        inputBuf[128] = {};
    bool        selected  = false;
    int         zOrder    = 0;
    float       fontSize  = 1.0f;      // multiplier
    bool        visible   = true;
    int         dropdownIdx = 0;
    char        dropdownItems[256] = "Item 1,Item 2,Item 3";
    float       rounding  = 4.f;
    float       borderWidth = 1.f;
    ImVec4      borderColor = {1.f, 0.85f, 0.f, 0.5f};
};

// ── SceneSnapshot — serialised entity list for one scene tab ──────────────────
struct SceneSnapshot {
    struct EntitySnap {
        std::string name;
        glm::vec3   pos   = {0,0,0};
        glm::vec3   rot   = {0,0,0};
        glm::vec3   scale = {1,1,1};
        glm::vec3   color = {1,1,1};
        bool        hasMesh = false;
        bool        isSphere = false;
        std::string modelPath;
    };
    std::vector<EntitySnap> entities;
    std::vector<UIWidget>    uiWidgets;  // UI Scene canvas widgets
};

// ── Scene type ────────────────────────────────────────────────────────────────
enum class SceneType {
    Scene3D,   // normal 3D scene — all objects allowed, UI objects have transform
    SceneUI,   // flat 2D UI scene — only UI widgets, no 3D primitives or physics
};

// ── SceneTab — one open scene ─────────────────────────────────────────────────
struct SceneTab {
    std::string    name  = "Untitled";
    SceneType      type  = SceneType::Scene3D;
    SceneSnapshot  snap;          // saved snapshot (restored on tab switch)
    bool           dirty = false;
    std::string    filePath;
};

// ─────────────────────────────────────────────────────────────────────────────
class SceneEditor {
public:
    explicit SceneEditor(App& app);

    // Call inside app.onRender()
    void draw();

    // Toggle visibility
    void show()   { m_visible = true; }
    void hide()   { m_visible = false; }
    void toggle() { m_visible = !m_visible; }
    bool isVisible() const { return m_visible; }

    // Selected entity
    EntityID selected() const { return m_selected; }
    void     select(EntityID id) { m_selected = id; }

    // Callbacks
    void onSelect  (std::function<void(EntityID)> fn) { m_onSelect   = fn; }
    void onDelete  (std::function<void(EntityID)> fn) { m_onDelete   = fn; }
    void onCreate  (std::function<void(EntityID)> fn) { m_onCreate   = fn; }

    // Scene tab API (public so external code can trigger new tab)
    void sceneTabs_newTab();
    void sceneTabs_addTab(SceneType type); // create tab of specific type

    // Access current tab UI widgets (for codegen)
    const std::vector<UIWidget>& uiWidgets() const { return m_uiWidgets; }

    // Access all tabs (for multi-scene code generation)
    const std::vector<SceneTab>& tabs()        const { return m_tabs; }
    int                          activeTabIdx() const { return m_activeTab; }
    // Returns a snapshot of the current live world (active tab)
    SceneSnapshot                activeSnapshot() { sceneTabs_saveActive(); return m_tabs[m_activeTab].snap; }

private:
    App&      m_app;
    World&    m_world;
    bool      m_visible   = true;
    EntityID  m_selected  = NULL_ENTITY;

    // Undo stack
    std::vector<UndoAction> m_undoStack;
    int                     m_undoIdx = -1;

    // Play/Pause state
    enum class PlayState { Editing, Playing, Paused };
    PlayState m_playState = PlayState::Editing;

    // Rename state
    EntityID  m_renamingId  = NULL_ENTITY;
    char      m_renameBuffer[128] = {};

    // Create popup
    bool m_showCreateMenu = false;
    bool m_newTabPopup    = false;  // open "choose scene type" popup
    bool m_showUIScene    = false;

    // Gizmo state
    GizmoMode m_gizmoMode = GizmoMode::Translate;

    // Callbacks
    std::function<void(EntityID)> m_onSelect;
    std::function<void(EntityID)> m_onDelete;
    std::function<void(EntityID)> m_onCreate;

    // ── Scene Tabs ────────────────────────────────────────────────────────────
    std::vector<SceneTab> m_tabs;
    int                   m_activeTab  = 0;
    int                   m_pendingTab = -1;  // tab to force-select for one frame

    // ── UI Scene state (per-tab, saved into SceneTab::snap.uiWidgets) ─────────
    std::vector<UIWidget> m_uiWidgets;
    int                   m_uiSelIdx  = -1;
    int                   m_uiDragIdx = -1;
    ImVec2                m_uiDragOffset = {0,0};

    void sceneTabs_init();           // create default "Scene 1" tab
    void drawSceneTabs();            // ImGui::BeginTabBar rendering
    void sceneTabs_saveActive();     // snapshot active world → m_tabs[m_activeTab]
    void sceneTabs_loadActive();     // restore snapshot → world
    // (sceneTabs_newTab is public — see above)
    void sceneTabs_closeTab(int i);  // close tab (with dirty check)
    SceneSnapshot sceneTabs_snapshot();                // capture world + UI
    void          sceneTabs_restore(const SceneSnapshot& s); // restore world

    // ── Draw panels ───────────────────────────────────────────────────────────
    void drawToolbar();
    void drawHierarchy();
    void drawInspector();
    void drawStats();
    void drawCreateMenu();
    SceneType activeSceneType() const;
    void drawUIScene();       // UI Scene editor — drag & drop canvas
    void drawGizmo();        // renders 3D gizmo + handles click-picking

    // ── Picking ───────────────────────────────────────────────────────────────
    // Returns entity under cursor via ray-AABB test, or NULL_ENTITY


    // ── Inspector sections ────────────────────────────────────────────────────
    void inspectTransform (EntityID id);
    void inspectMesh      (EntityID id);
    void inspectMaterial  (EntityID id);
    void inspectRigidBody (EntityID id);
    void inspectAudio     (EntityID id);
    void inspectScript    (EntityID id);
    void inspectTag       (EntityID id);

    // ── Helpers ───────────────────────────────────────────────────────────────
    void handleHotkeys();
    void deleteSelected();
    void duplicateSelected();
    void focusCameraOn(EntityID id);
    void handleMousePick();   // raycast click → select entity
    void pushUndo(UndoAction action);
    void undo();
    void redo();

    std::string entityDisplayName(EntityID id) const;
};

} // namespace eng
