// SceneEditor.cpp
#include "editor/SceneEditor.h"
#include "editor/Gizmo.h"
#include "core/Log.h"
#include <imgui.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace eng {

// ─────────────────────────────────────────────────────────────────────────────
//  Ray-AABB intersection helper
// ─────────────────────────────────────────────────────────────────────────────
static bool rayAABB(const glm::vec3& ro, const glm::vec3& rd,
                    const glm::vec3& bmin, const glm::vec3& bmax,
                    float& tOut)
{
    glm::vec3 invD = 1.f / rd;
    glm::vec3 t0   = (bmin - ro) * invD;
    glm::vec3 t1   = (bmax - ro) * invD;
    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);
    float tEnter = std::max({tmin.x, tmin.y, tmin.z});
    float tExit  = std::min({tmax.x, tmax.y, tmax.z});
    if (tExit < 0 || tEnter > tExit) return false;
    tOut = tEnter > 0 ? tEnter : tExit;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
SceneEditor::SceneEditor(App& app)
    : m_app(app), m_world(app.world())
{
    sceneTabs_init();
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Helper: get widget type name ─────────────────────────────────────────────
static const char* widgetTypeName(UIWidget::Type t) {
    switch (t) {
    case UIWidget::Type::Button:      return "Button";
    case UIWidget::Type::Label:       return "Label";
    case UIWidget::Type::Slider:      return "Slider";
    case UIWidget::Type::TextField:   return "TextField";
    case UIWidget::Type::ColoredText: return "ColoredText";
    case UIWidget::Type::Panel:       return "Panel";
    case UIWidget::Type::Image:       return "Image";
    case UIWidget::Type::Toggle:      return "Toggle";
    case UIWidget::Type::ProgressBar: return "ProgressBar";
    case UIWidget::Type::Dropdown:    return "Dropdown";
    }
    return "Unknown";
}

// ── Helper: default widget size ───────────────────────────────────────────────
static ImVec2 defaultWidgetSize(UIWidget::Type t) {
    switch (t) {
    case UIWidget::Type::Button:      return {140.f, 36.f};
    case UIWidget::Type::Label:       return {140.f, 22.f};
    case UIWidget::Type::Slider:      return {200.f, 24.f};
    case UIWidget::Type::TextField:   return {200.f, 28.f};
    case UIWidget::Type::ColoredText: return {160.f, 22.f};
    case UIWidget::Type::Panel:       return {200.f, 120.f};
    case UIWidget::Type::Image:       return {100.f, 100.f};
    case UIWidget::Type::Toggle:      return {140.f, 24.f};
    case UIWidget::Type::ProgressBar: return {200.f, 20.f};
    case UIWidget::Type::Dropdown:    return {180.f, 28.f};
    }
    return {120.f, 28.f};
}

// ── Helper: initialize widget defaults ────────────────────────────────────────
static void initWidgetDefaults(UIWidget& w) {
    w.size = defaultWidgetSize(w.type);
    snprintf(w.name, sizeof(w.name), "%s", widgetTypeName(w.type));
    switch (w.type) {
    case UIWidget::Type::Button:
        snprintf(w.text, sizeof(w.text), "Button");
        w.color     = {1.f, 0.85f, 0.f, 1.f};
        w.bgColor   = {0.10f, 0.10f, 0.16f, 1.f};
        w.rounding  = 4.f;
        break;
    case UIWidget::Type::Label:
        snprintf(w.text, sizeof(w.text), "Label");
        w.color     = {0.95f, 0.92f, 0.85f, 1.f};
        break;
    case UIWidget::Type::Slider:
        snprintf(w.text, sizeof(w.text), "Slider");
        w.sliderVal = 0.5f;
        w.color     = {1.f, 0.85f, 0.f, 1.f};
        break;
    case UIWidget::Type::TextField:
        snprintf(w.text, sizeof(w.text), "Placeholder...");
        w.color     = {0.95f, 0.92f, 0.85f, 1.f};
        w.bgColor   = {0.08f, 0.08f, 0.12f, 1.f};
        break;
    case UIWidget::Type::ColoredText:
        snprintf(w.text, sizeof(w.text), "Colored Text");
        w.color     = {0.f, 0.95f, 1.f, 1.f};
        break;
    case UIWidget::Type::Panel:
        snprintf(w.text, sizeof(w.text), "Panel");
        w.bgColor   = {0.09f, 0.09f, 0.14f, 0.9f};
        w.borderColor = {1.f, 0.85f, 0.f, 0.4f};
        w.rounding  = 6.f;
        break;
    case UIWidget::Type::Image:
        snprintf(w.text, sizeof(w.text), "Image");
        w.bgColor   = {0.15f, 0.15f, 0.20f, 1.f};
        w.color     = {1.f, 1.f, 1.f, 1.f};
        w.rounding  = 0.f;
        break;
    case UIWidget::Type::Toggle:
        snprintf(w.text, sizeof(w.text), "Toggle");
        w.toggled   = false;
        w.color     = {1.f, 0.85f, 0.f, 1.f};
        break;
    case UIWidget::Type::ProgressBar:
        snprintf(w.text, sizeof(w.text), "Progress");
        w.progress  = 0.6f;
        w.color     = {1.f, 0.85f, 0.f, 1.f};
        w.bgColor   = {0.10f, 0.10f, 0.15f, 1.f};
        break;
    case UIWidget::Type::Dropdown:
        snprintf(w.text, sizeof(w.text), "Option 1");
        w.color     = {0.95f, 0.92f, 0.85f, 1.f};
        w.bgColor   = {0.10f, 0.10f, 0.16f, 1.f};
        w.dropdownIdx = 0;
        break;
    }
}


void SceneEditor::draw() {
    handleHotkeys();
    handleMousePick(); // ray-cast click → select entity

    if (!m_visible) return;

    UI::EnableDockspace();
    drawToolbar();
    drawSceneTabs();
    drawHierarchy();
    drawInspector();
    drawStats();
    drawGizmo();       // 3D handles + selection overlay (inside)
    if (m_showCreateMenu) drawCreateMenu();
    // UI canvas is shown automatically for UI scene tabs — no toggle needed
    if (activeSceneType() == SceneType::SceneUI) drawUIScene();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Toolbar
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::drawToolbar() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    // Sit below the main menu bar (BeginMainMenuBar reserves one frame-height row)
    const float MENUBAR_H = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos({vp->Pos.x, vp->Pos.y + MENUBAR_H});
    ImGui::SetNextWindowSize({vp->Size.x, 42});
    ImGui::SetNextWindowBgAlpha(0.95f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8, 6});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::Begin("##toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar(2);

    // ── Play / Pause / Stop ───────────────────────────────────────────────────
    bool isPlaying = m_playState == PlayState::Playing;
    bool isPaused  = m_playState == PlayState::Paused;

    if (isPlaying || isPaused)
        ImGui::PushStyleColor(ImGuiCol_Button, {0.18f, 0.55f, 0.18f, 1.f});

    if (ImGui::Button(isPlaying ? "  ||  " : "  >  ")) {
        if (m_playState == PlayState::Editing) {
            m_playState = PlayState::Playing;
            LOG_INFO("Editor: Play");
        } else if (m_playState == PlayState::Playing) {
            m_playState = PlayState::Paused;
            LOG_INFO("Editor: Paused");
        } else {
            m_playState = PlayState::Playing;
            LOG_INFO("Editor: Resumed");
        }
    }
    UI::Tooltip("Play / Pause  (Space)");

    if (isPlaying || isPaused) ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::BeginDisabled(m_playState == PlayState::Editing);
    if (ImGui::Button("  []  ")) {
        m_playState = PlayState::Editing;
        LOG_INFO("Editor: Stop");
    }
    UI::Tooltip("Stop");
    ImGui::EndDisabled();

    ImGui::SameLine(0, 20);
    ImGui::Text("|");
    ImGui::SameLine(0, 20);

    bool isUIScene = (activeSceneType() == SceneType::SceneUI);

    // ── Gizmo mode — hidden in UI scene ──────────────────────────────────────
    if (!isUIScene) {
        auto gizmoBtn = [&](const char* label, GizmoMode mode) {
            bool active = m_gizmoMode == mode;
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, {0.18f,0.45f,0.78f,1.f});
            if (ImGui::Button(label)) m_gizmoMode = mode;
            if (active) ImGui::PopStyleColor();
            ImGui::SameLine();
        };
        gizmoBtn("  T  ", GizmoMode::Translate);  UI::Tooltip("Translate (W)");
        gizmoBtn("  R  ", GizmoMode::Rotate);      UI::Tooltip("Rotate (E)");
        gizmoBtn("  S  ", GizmoMode::Scale);       UI::Tooltip("Scale (R)");
        ImGui::SameLine(0, 20);
        ImGui::Text("|");
        ImGui::SameLine(0, 20);
    } else {
        // Scene type badge
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.40f,0.18f,0.55f,0.80f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.40f,0.18f,0.55f,0.80f});
        ImGui::Button(" UI Scene ");
        ImGui::PopStyleColor(2);
        UI::Tooltip("This is a UI Scene — only 2D widgets allowed.");
        ImGui::SameLine(0, 20);
        ImGui::Text("|");
        ImGui::SameLine(0, 20);
    }

    // ── Create objects ────────────────────────────────────────────────────────
    if (ImGui::Button("  + Object  "))
        m_showCreateMenu = !m_showCreateMenu;
    UI::Tooltip("Create new object (Ctrl+N)");

    ImGui::SameLine(0, 20);
    ImGui::Text("|");
    ImGui::SameLine(0, 20);

    // ── Undo/Redo ─────────────────────────────────────────────────────────────
    ImGui::BeginDisabled(m_undoIdx < 0);
    if (ImGui::Button(" Undo ")) undo();
    UI::Tooltip("Undo (Ctrl+Z)");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(m_undoIdx >= (int)m_undoStack.size() - 1);
    if (ImGui::Button(" Redo ")) redo();
    UI::Tooltip("Redo (Ctrl+Y)");
    ImGui::EndDisabled();

    // ── Play state label ──────────────────────────────────────────────────────
    ImGui::SameLine();
    float rightX = ImGui::GetContentRegionMax().x - 80;
    ImGui::SetCursorPosX(rightX);
    switch (m_playState) {
    case PlayState::Editing: UI::LabelColored(UI::GRAY,   "EDITING"); break;
    case PlayState::Playing: UI::LabelColored(UI::GREEN,  "PLAYING"); break;
    case PlayState::Paused:  UI::LabelColored(UI::YELLOW, "PAUSED");  break;
    }

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hierarchy
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::drawHierarchy() {
    ImGui::SetNextWindowPos({4, ImGui::GetFrameHeight() + 72.f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({240, 500}, ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Hierarchy")) { ImGui::End(); return; }

    bool isUIScene = (activeSceneType() == SceneType::SceneUI);

    if (isUIScene) {
        // ── UI Scene hierarchy: list of UIWidgets ─────────────────────────────

        // Header with add button
        ImGui::PushStyleColor(ImGuiCol_Text, {0.f,0.95f,1.f,0.85f});
        ImGui::TextUnformatted("UI Widgets");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        float addBtnX = ImGui::GetContentRegionAvail().x - 20.f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + addBtnX);
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.38f,0.10f,1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.15f,0.55f,0.15f,1.f});
        if (ImGui::SmallButton(" + ")) ImGui::OpenPopup("##hier_add_widget");
        ImGui::PopStyleColor(2);
        UI::Tooltip("Add widget");
        ImGui::Separator();

        // Add widget popup
        if (ImGui::BeginPopup("##hier_add_widget")) {
            ImGui::PushStyleColor(ImGuiCol_Text, {1.f,0.85f,0.f,1.f});
            ImGui::TextUnformatted("Add Widget");
            ImGui::PopStyleColor();
            ImGui::Separator();
            struct WEntry { const char* label; UIWidget::Type type; };
            static const WEntry wEntries[] = {
                {"Button",       UIWidget::Type::Button},
                {"Label",        UIWidget::Type::Label},
                {"Colored Text", UIWidget::Type::ColoredText},
                {"Text Field",   UIWidget::Type::TextField},
                {"Slider",       UIWidget::Type::Slider},
                {"Toggle",       UIWidget::Type::Toggle},
                {"Progress Bar", UIWidget::Type::ProgressBar},
                {"Panel",        UIWidget::Type::Panel},
                {"Image",        UIWidget::Type::Image},
                {"Dropdown",     UIWidget::Type::Dropdown},
            };
            for (auto& e : wEntries) {
                if (ImGui::MenuItem(e.label)) {
                    UIWidget w;
                    w.type = e.type;
                    initWidgetDefaults(w);
                    w.pos = {80.f + m_uiWidgets.size() * 12.f,
                             60.f + m_uiWidgets.size() * 12.f};
                    w.zOrder = (int)m_uiWidgets.size();
                    snprintf(w.name, sizeof(w.name), "%s_%d",
                                widgetTypeName(w.type), (int)m_uiWidgets.size()+1);
                    m_uiSelIdx = (int)m_uiWidgets.size();
                    m_uiWidgets.push_back(w);
                }
            }
            ImGui::EndPopup();
        }

        // Widget list sorted by z-order (front to back = top to bottom)
        std::vector<int> order;
        order.reserve(m_uiWidgets.size());
        for (int i=0; i<(int)m_uiWidgets.size(); i++) order.push_back(i);
        std::stable_sort(order.begin(), order.end(),
            [&](int a, int b){ return m_uiWidgets[a].zOrder > m_uiWidgets[b].zOrder; });

        static int  s_renamingWidget = -1;
        static char s_renameWidgetBuf[64] = {};

        for (int oi = 0; oi < (int)order.size(); oi++) {
            int i = order[oi];
            UIWidget& w = m_uiWidgets[i];
            bool isSel = (m_uiSelIdx == i);

            const char* icon = "[ ]";
            switch (w.type) {
            case UIWidget::Type::Button:      icon = "[B]"; break;
            case UIWidget::Type::Label:       icon = "[L]"; break;
            case UIWidget::Type::Slider:      icon = "[~]"; break;
            case UIWidget::Type::TextField:   icon = "[T]"; break;
            case UIWidget::Type::ColoredText: icon = "[C]"; break;
            case UIWidget::Type::Panel:       icon = "[P]"; break;
            case UIWidget::Type::Image:       icon = "[I]"; break;
            case UIWidget::Type::Toggle:      icon = "[X]"; break;
            case UIWidget::Type::ProgressBar: icon = "[%]"; break;
            case UIWidget::Type::Dropdown:    icon = "[V]"; break;
            }

            if (s_renamingWidget == i) {
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##wren", s_renameWidgetBuf, sizeof(s_renameWidgetBuf),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                    strncpy(w.name, s_renameWidgetBuf, sizeof(w.name)-1);
                    s_renamingWidget = -1;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) s_renamingWidget = -1;
                continue;
            }

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Leaf |
                ImGuiTreeNodeFlags_SpanFullWidth |
                ImGuiTreeNodeFlags_FramePadding;
            if (isSel) flags |= ImGuiTreeNodeFlags_Selected;

            if (!w.visible)
                ImGui::PushStyleColor(ImGuiCol_Text, {0.45f,0.44f,0.40f,1.f});

            char nodeLabel[96];
            snprintf(nodeLabel, sizeof(nodeLabel), "%s %s", icon, w.name);
            bool open = ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s", nodeLabel);

            if (!w.visible) ImGui::PopStyleColor();

            if (ImGui::IsItemClicked())         m_uiSelIdx = i;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                s_renamingWidget = i;
                strncpy(s_renameWidgetBuf, w.name, sizeof(s_renameWidgetBuf)-1);
            }

            if (ImGui::BeginPopupContextItem()) {
                m_uiSelIdx = i;
                if (ImGui::MenuItem("Rename", "F2")) {
                    s_renamingWidget = i;
                    strncpy(s_renameWidgetBuf, w.name, sizeof(s_renameWidgetBuf)-1);
                }
                if (ImGui::MenuItem(w.visible ? "Hide" : "Show")) w.visible = !w.visible;
                ImGui::Separator();
                if (ImGui::MenuItem("Move Up"))   w.zOrder++;
                if (ImGui::MenuItem("Move Down")) w.zOrder--;
                ImGui::Separator();
                if (ImGui::MenuItem("Duplicate")) {
                    UIWidget copy = w;
                    copy.pos.x += 16.f; copy.pos.y += 16.f;
                    copy.zOrder = (int)m_uiWidgets.size();
                    snprintf(copy.name, sizeof(copy.name), "%s_copy", w.name);
                    m_uiSelIdx = (int)m_uiWidgets.size();
                    m_uiWidgets.push_back(copy);
                }
                if (ImGui::MenuItem("Delete")) {
                    ImGui::EndPopup();
                    if (open) ImGui::TreePop();
                    m_uiWidgets.erase(m_uiWidgets.begin() + i);
                    if (m_uiSelIdx >= (int)m_uiWidgets.size())
                        m_uiSelIdx = (int)m_uiWidgets.size() - 1;
                    ImGui::End();
                    return;
                }
                ImGui::EndPopup();
            }

            if (open) ImGui::TreePop();
        }

        if (m_uiWidgets.empty()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, {0.45f,0.44f,0.40f,1.f});
            ImGui::TextWrapped("No widgets yet.\nClick [+] above to add one,\nor drag from the palette on the canvas.");
            ImGui::PopStyleColor();
        }

    } else {
        // ── 3D Scene hierarchy: ECS entities ─────────────────────────────────
        static char filter[64] = {};
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##filter", "Search...", filter, sizeof(filter));
        ImGui::Separator();

        std::vector<EntityID> entities;
        m_world.each<TagComp>([&](EntityID id, TagComp&) { entities.push_back(id); });
        std::sort(entities.begin(), entities.end());

        for (EntityID id : entities) {
            auto* tag = m_world.get<TagComp>(id);
            std::string name = tag ? tag->value : ("Entity " + std::to_string(id));

            if (filter[0] != '\0') {
                std::string lower = name, f = filter;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                std::transform(f.begin(),     f.end(),     f.begin(),     ::tolower);
                if (lower.find(f) == std::string::npos) continue;
            }

            if (m_renamingId == id) {
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                    if (tag) {
                        std::string oldName = tag->value, newName = m_renameBuffer;
                        pushUndo({"Rename " + oldName,
                            [this, oldName, id]() mutable { if (auto* t = m_world.get<TagComp>(id)) t->value = oldName; },
                            [this, newName, id]() mutable { if (auto* t = m_world.get<TagComp>(id)) t->value = newName; }});
                        tag->value = newName;
                    }
                    m_renamingId = NULL_ENTITY;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_renamingId = NULL_ENTITY;
                continue;
            }

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;
            if (m_selected == id) flags |= ImGuiTreeNodeFlags_Selected;

            bool open = ImGui::TreeNodeEx((void*)(uintptr_t)id, flags, "%s", name.c_str());
            if (ImGui::IsItemClicked()) { m_selected = id; if (m_onSelect) m_onSelect(id); }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                m_renamingId = id;
                strncpy(m_renameBuffer, name.c_str(), sizeof(m_renameBuffer) - 1);
            }
            if (ImGui::BeginPopupContextItem()) {
                m_selected = id;
                if (ImGui::MenuItem("Rename",    "F2"))     { m_renamingId = id; strncpy(m_renameBuffer, name.c_str(), sizeof(m_renameBuffer)-1); }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D")) duplicateSelected();
                if (ImGui::MenuItem("Focus",     "F"))      focusCameraOn(id);
                ImGui::Separator();
                if (ImGui::MenuItem("Delete",    "Del")) {
                    ImGui::EndPopup(); if (open) ImGui::TreePop();
                    deleteSelected(); ImGui::End(); return;
                }
                ImGui::EndPopup();
            }
            if (open) ImGui::TreePop();
        }

        if (ImGui::BeginPopupContextWindow("##hier_ctx",
            ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Create Empty")) {
                auto obj = m_app.spawn("Empty");
                m_selected = obj.id();
                if (m_onCreate) m_onCreate(m_selected);
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}


// ─────────────────────────────────────────────────────────────────────────────
//  Inspector
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::drawInspector() {
    ImGui::SetNextWindowPos({(float)m_app.window().width() - 264, ImGui::GetFrameHeight() + 72.f},
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({260, 600}, ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Inspector")) { ImGui::End(); return; }

    bool isUIScene = (activeSceneType() == SceneType::SceneUI);

    if (isUIScene) {
        // ── UI Scene: inspect selected UIWidget ───────────────────────────────
        if (m_uiSelIdx < 0 || m_uiSelIdx >= (int)m_uiWidgets.size()) {
            ImGui::TextDisabled("No widget selected");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, {0.45f,0.44f,0.40f,1.f});
            ImGui::TextWrapped("Select a widget in the Hierarchy or click it on the canvas.");
            ImGui::PopStyleColor();
            ImGui::End();
            return;
        }

        UIWidget& w = m_uiWidgets[m_uiSelIdx];

        // Header
        ImGui::PushStyleColor(ImGuiCol_Text, {0.f,0.95f,1.f,1.f});
        ImGui::Text("%s", widgetTypeName(w.type));
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, {0.55f,0.53f,0.48f,1.f});
        ImGui::Text("(Widget)");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        float labelW = 68.f;
        auto propLabel = [&](const char* lbl) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.55f,0.53f,0.48f,1.f});
            ImGui::TextUnformatted(lbl);
            ImGui::PopStyleColor();
            ImGui::SameLine(labelW);
            ImGui::SetNextItemWidth(-1.f);
        };

        // ── Identity ──────────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, {1.f,0.85f,0.f,0.85f});
        ImGui::TextUnformatted("Identity");
        ImGui::PopStyleColor();
        propLabel("Name");   ImGui::InputText("##wn",  w.name,  sizeof(w.name));
        propLabel("Text");   ImGui::InputText("##wt",  w.text,  sizeof(w.text));
        ImGui::Spacing();

        // ── Rect Transform ────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, {1.f,0.85f,0.f,0.85f});
        ImGui::TextUnformatted("Rect Transform");
        ImGui::PopStyleColor();

        float hw = (ImGui::GetContentRegionAvail().x - labelW - 6.f) * 0.5f;
        ImGui::PushStyleColor(ImGuiCol_Text, {0.55f,0.53f,0.48f,1.f});
        ImGui::TextUnformatted("Pos");
        ImGui::PopStyleColor();
        ImGui::SameLine(labelW);
        ImGui::SetNextItemWidth(hw);
        ImGui::DragFloat("##wpx", &w.pos.x, 1.f, 0.f, 0.f, "X:%.0f");
        ImGui::SameLine(0, 4);
        ImGui::SetNextItemWidth(hw);
        ImGui::DragFloat("##wpy", &w.pos.y, 1.f, 0.f, 0.f, "Y:%.0f");

        ImGui::PushStyleColor(ImGuiCol_Text, {0.55f,0.53f,0.48f,1.f});
        ImGui::TextUnformatted("Size");
        ImGui::PopStyleColor();
        ImGui::SameLine(labelW);
        ImGui::SetNextItemWidth(hw);
        ImGui::DragFloat("##wsw", &w.size.x, 1.f, 10.f, 2000.f, "W:%.0f");
        ImGui::SameLine(0, 4);
        ImGui::SetNextItemWidth(hw);
        ImGui::DragFloat("##wsh", &w.size.y, 1.f, 10.f, 2000.f, "H:%.0f");

        propLabel("Z-Order");
        ImGui::DragInt("##wz", &w.zOrder, 1.f, -100, 100);
        ImGui::Spacing();

        // ── Appearance ────────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, {1.f,0.85f,0.f,0.85f});
        ImGui::TextUnformatted("Appearance");
        ImGui::PopStyleColor();

        propLabel("Color");
        ImGui::ColorEdit4("##wc", &w.color.x,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);

        switch (w.type) {
        case UIWidget::Type::Panel:
        case UIWidget::Type::Button:
        case UIWidget::Type::Image:
        case UIWidget::Type::Dropdown:
        case UIWidget::Type::TextField:
            propLabel("BG Color");
            ImGui::ColorEdit4("##wbg", &w.bgColor.x,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);
            propLabel("Border");
            ImGui::ColorEdit4("##wbc", &w.borderColor.x,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);
            propLabel("Rounding");
            ImGui::SliderFloat("##wr", &w.rounding, 0.f, 20.f, "%.1f");
            break;
        default: break;
        }

        if (w.type == UIWidget::Type::Slider) {
            ImGui::Spacing();
            propLabel("Value");
            ImGui::SliderFloat("##wv", &w.sliderVal, 0.f, 1.f, "%.2f");
        }
        if (w.type == UIWidget::Type::Toggle) {
            ImGui::Spacing();
            propLabel("Checked");
            ImGui::Checkbox("##wtog", &w.toggled);
        }
        if (w.type == UIWidget::Type::ProgressBar) {
            ImGui::Spacing();
            propLabel("Progress");
            ImGui::SliderFloat("##wprog", &w.progress, 0.f, 1.f, "%.2f");
        }
        if (w.type == UIWidget::Type::Dropdown) {
            ImGui::Spacing();
            propLabel("Items");
            ImGui::InputText("##wdi", w.dropdownItems, sizeof(w.dropdownItems));
            ImGui::PushStyleColor(ImGuiCol_Text, {0.45f,0.44f,0.40f,1.f});
            ImGui::TextUnformatted("  comma separated");
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ── Layer ──────────────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, {1.f,0.85f,0.f,0.85f});
        ImGui::TextUnformatted("Layer");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        float bw2 = (ImGui::GetContentRegionAvail().x - 4.f) * 0.5f;
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.10f,0.16f,1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.f,0.85f,0.f,0.15f});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
        if (ImGui::Button("Bring Forward", {bw2, 24})) w.zOrder++;
        ImGui::SameLine(0, 4);
        if (ImGui::Button("Send Backward", {bw2, 24})) w.zOrder--;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        propLabel("Visible");
        ImGui::Checkbox("##wvis", &w.visible);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ── Delete ────────────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Button,        {0.45f,0.06f,0.06f,1.f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.70f,0.08f,0.08f,1.f});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
        if (ImGui::Button("Delete Widget", {-1, 28})) {
            m_uiWidgets.erase(m_uiWidgets.begin() + m_uiSelIdx);
            m_uiSelIdx = -1;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

    } else {
        // ── 3D Scene: inspect selected ECS entity ─────────────────────────────
        if (m_selected == NULL_ENTITY) {
            ImGui::TextDisabled("No object selected");
            ImGui::End();
            return;
        }

        auto* tag = m_world.get<TagComp>(m_selected);
        std::string name = tag ? tag->value : ("Entity " + std::to_string(m_selected));

        char nameBuf[128] = {};
        strncpy(nameBuf, name.c_str(), sizeof(nameBuf) - 1);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf),
            ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (tag) tag->value = nameBuf;
        }
        ImGui::TextDisabled("ID: %u", m_selected);
        ImGui::Separator();

        inspectTransform(m_selected);
        inspectMesh     (m_selected);
        inspectMaterial (m_selected);
        inspectRigidBody(m_selected);
        //inspectAudio    (m_selected);
        inspectScript   (m_selected);

        ImGui::Spacing();
        ImGui::Separator();
        float bw = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((bw - 160) * 0.5f);
        if (ImGui::Button("+ Add Component", {160, 28}))
            ImGui::OpenPopup("add_comp_popup");

        if (ImGui::BeginPopup("add_comp_popup")) {
            if (!m_world.has<RigidBodyComponent>(m_selected)) {
                if (ImGui::MenuItem("RigidBody")) {
                    m_world.add<RigidBodyComponent>(m_selected);
                    LOG_INFO("Editor: added RigidBodyComponent to " << m_selected);
                }
            }
            if (!m_world.has<ScriptComponent>(m_selected)) {
                if (ImGui::MenuItem("Script"))
                    m_world.add<ScriptComponent>(m_selected);
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inspector — Transform
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::inspectTransform(EntityID id) {
    auto* t = m_world.get<TransformComponent>(id);
    if (!t) return;

    if (!ImGui::CollapsingHeader("  ⊞  Transform", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    glm::vec3 pos   = t->position;
    glm::vec3 euler = t->rotation;
    glm::vec3 scl   = t->scale;
    bool changed    = false;

    // Column layout: label | X | Y | Z | reset
    ImGui::PushItemWidth(72.f);

    // ── Position ──────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Position");
    ImGui::SameLine(72);
    ImGui::PushStyleColor(ImGuiCol_Text, {0.95f,0.35f,0.35f,1.f});
    ImGui::TextUnformatted("X"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##px", &pos.x, 0.05f)) changed = true;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {0.35f,0.90f,0.35f,1.f});
    ImGui::TextUnformatted("Y"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##py", &pos.y, 0.05f)) changed = true;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {0.35f,0.55f,0.95f,1.f});
    ImGui::TextUnformatted("Z"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##pz", &pos.z, 0.05f)) changed = true;
    ImGui::SameLine();
    if (ImGui::SmallButton("R##rp")) { pos = {0,0,0}; changed = true; }
    UI::Tooltip("Reset position to (0, 0, 0)");

    // ── Rotation ──────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Rotation");
    ImGui::SameLine(72);
    ImGui::PushStyleColor(ImGuiCol_Text, {0.95f,0.35f,0.35f,1.f});
    ImGui::TextUnformatted("X"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##rx", &euler.x, 0.5f, -360.f, 360.f)) changed = true;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {0.35f,0.90f,0.35f,1.f});
    ImGui::TextUnformatted("Y"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##ry", &euler.y, 0.5f, -360.f, 360.f)) changed = true;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {0.35f,0.55f,0.95f,1.f});
    ImGui::TextUnformatted("Z"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##rz", &euler.z, 0.5f, -360.f, 360.f)) changed = true;
    ImGui::SameLine();
    if (ImGui::SmallButton("R##rr")) { euler = {0,0,0}; changed = true; }
    UI::Tooltip("Reset rotation to (0, 0, 0)");

    // ── Scale ─────────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Scale   ");
    ImGui::SameLine(72);
    ImGui::PushStyleColor(ImGuiCol_Text, {0.95f,0.35f,0.35f,1.f});
    ImGui::TextUnformatted("X"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##sx", &scl.x, 0.01f, 0.001f, 1000.f)) changed = true;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {0.35f,0.90f,0.35f,1.f});
    ImGui::TextUnformatted("Y"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##sy", &scl.y, 0.01f, 0.001f, 1000.f)) changed = true;
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {0.35f,0.55f,0.95f,1.f});
    ImGui::TextUnformatted("Z"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##sz", &scl.z, 0.01f, 0.001f, 1000.f)) changed = true;
    ImGui::SameLine();
    if (ImGui::SmallButton("R##rs")) { scl = {1,1,1}; changed = true; }
    UI::Tooltip("Reset scale to (1, 1, 1)");

    ImGui::PopItemWidth();

    if (changed) {
        t->position = pos;
        t->rotation = euler;
        t->scale    = scl;
        if (m_world.has<RigidBodyComponent>(id))
            m_app.physics().setPosition(id, pos);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inspector — Mesh
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::inspectMesh(EntityID id) {
    auto* mr = m_world.get<MeshRendererComponent>(id);
    if (!mr) return;

    if (!ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Separator();

    // Current model path
    std::string curPath = (mr->model && !mr->model->path().empty())
                        ? mr->model->path() : "";

    ImGui::TextDisabled("Model:");
    ImGui::SameLine();
    ImGui::TextUnformatted(curPath.empty() ? "(built-in)" : curPath.c_str());

    // Load model from path
    static char modelPathBuf[512] = {};
    ImGui::SetNextItemWidth(-80);
    ImGui::InputTextWithHint("##modelpath", "assets/models/file.glb  (Enter to load)",
                              modelPathBuf, sizeof(modelPathBuf));
    ImGui::SameLine();
    bool loadClicked = ImGui::Button("Load##model");

    // Load on Enter or button click
    bool enterPressed = ImGui::IsItemDeactivatedAfterEdit() ||
                        (ImGui::IsItemFocused() &&
                         ImGui::IsKeyPressed(ImGuiKey_Enter));

    if ((loadClicked || enterPressed) && modelPathBuf[0] != '\0') {
        try {
            // ResourceManager::model() returns Model* — wrap in shared_ptr with no-op deleter
            // since ResourceManager owns the lifetime
            Model* raw = m_app.resources().model(modelPathBuf);
            if (raw) {
                mr->model = std::shared_ptr<Model>(raw, [](Model*){});
                LOG_INFO("Inspector: loaded model '" << modelPathBuf << "'");
                modelPathBuf[0] = '\0'; // clear after successful load
            } else {
                LOG_WARN("Inspector: model not found: " << modelPathBuf);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Inspector: model load error: " << e.what());
        }
    }

    // Quick shape buttons
    ImGui::Spacing();
    ImGui::TextDisabled("Quick shapes:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Cube"))     { GameObject{id, &m_world}.cube(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Sphere"))   { GameObject{id, &m_world}.sphere(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Cylinder")) { GameObject{id, &m_world}.cylinder(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Quad"))     { GameObject{id, &m_world}.quad(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Plane"))    { GameObject{id, &m_world}.plane(); }

    // Visibility toggle
    ImGui::Spacing();
    bool vis = mr->visible;
    if (ImGui::Checkbox("Visible", &vis))
        GameObject{id, &m_world}.visible(vis);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inspector — Material
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::inspectMaterial(EntityID id) {
    auto* mr = m_world.get<MeshRendererComponent>(id);
    if (!mr) return;

    if (!ImGui::CollapsingHeader("  ◈  Material", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    Material& mat = mr->material;

    // ── Albedo ────────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Albedo  ");
    ImGui::SameLine(72);
    glm::vec3 col = {mat.albedo.r, mat.albedo.g, mat.albedo.b};
    ImGui::PushItemWidth(-1);
    if (ImGui::ColorEdit3("##albedo", glm::value_ptr(col),
        ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB))
        mat.albedo = {col.r, col.g, col.b, mat.albedo.a};

    // ── Alpha ─────────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Alpha   ");
    ImGui::SameLine(72);
    if (ImGui::SliderFloat("##alpha", &mat.albedo.a, 0.f, 1.f, "%.2f"))
        mat.transparent = (mat.albedo.a < 0.999f);

    // ── Emissive ──────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Emissive");
    ImGui::SameLine(72);
    ImGui::ColorEdit3("##emissive", glm::value_ptr(mat.emissive),
        ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_HDR |
        ImGuiColorEditFlags_Float);

    // ── Metallic ──────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Metallic");
    ImGui::SameLine(72);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, {0.6f, 0.7f, 0.8f, 1.f});
    ImGui::SliderFloat("##metallic", &mat.metallic, 0.f, 1.f, "%.2f");
    ImGui::PopStyleColor();

    // ── Roughness ─────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Roughness");
    ImGui::SameLine(72);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, {0.7f, 0.55f, 0.4f, 1.f});
    ImGui::SliderFloat("##roughness", &mat.roughness, 0.f, 1.f, "%.2f");
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    // ── Tiling ────────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::TextDisabled("Tiling");
    ImGui::SameLine(72);
    ImGui::PushItemWidth(80.f);
    ImGui::DragFloat("##tilingU", &mat.tilingU, 0.05f, 0.01f, 64.f, "U:%.2f");
    ImGui::SameLine();
    ImGui::DragFloat("##tilingV", &mat.tilingV, 0.05f, 0.01f, 64.f, "V:%.2f");
    ImGui::PopItemWidth();

    // ── Flags ─────────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Checkbox("Wireframe",   &mat.wireframe);
    ImGui::SameLine(110);
    ImGui::Checkbox("Double Sided", &mat.doubleSided);
    ImGui::Checkbox("Cast Shadow",  &mat.castShadow);
    ImGui::SameLine(110);
    ImGui::Checkbox("Transparent",  &mat.transparent);

    // ── Texture ───────────────────────────────────────────────────────────────
    ImGui::Spacing();
    if (mat.albedoMap)
        UI::LabelColored(UI::GREEN, "  \u2713 Texture loaded");
    else
        ImGui::TextDisabled("  No texture");

    // Texture path loader
    static char texPathBuf[512] = {};
    ImGui::SetNextItemWidth(-80);
    ImGui::InputTextWithHint("##texpath", "assets/textures/file.png",
                              texPathBuf, sizeof(texPathBuf));
    ImGui::SameLine();
    if (ImGui::Button("Load##tex") && texPathBuf[0] != '\0') {
        GameObject{id, &m_world}.texture(texPathBuf);
        texPathBuf[0] = '\0';
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inspector — RigidBody
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::inspectRigidBody(EntityID id) {
    auto* rb = m_world.get<RigidBodyComponent>(id);
    if (!rb) return;

    if (!ImGui::CollapsingHeader("  \u25A6  Rigid Body", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // ── Type & Shape ──────────────────────────────────────────────────────────
    static const char* types[]  = {"Static", "Kinematic", "Dynamic"};
    static const char* shapes[] = {"Box", "Sphere", "Capsule", "Cylinder", "ConvexHull", "Mesh"};
    int t = (int)rb->type;
    int s = (int)rb->shape;
    ImGui::PushItemWidth(-1);
    if (ImGui::Combo("##rbtype",  &t, types,  3)) rb->type  = (RigidBodyType)t;
    if (ImGui::Combo("##rbshape", &s, shapes, 6)) rb->shape = (ColliderShape)s;
    ImGui::PopItemWidth();

    // quick-type buttons
    ImGui::Spacing();
    ImGui::TextDisabled("Quick:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Static"))    { GameObject{id,&m_world}.physStatic(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Dynamic"))   { GameObject{id,&m_world}.physDynamic(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Kinematic")) { GameObject{id,&m_world}.physKinematic(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Sensor"))    { GameObject{id,&m_world}.physSensor(); }

    // ── Collider dims ─────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::TextDisabled("Collider");
    ImGui::SameLine();
    if (ImGui::SmallButton("Box##cs"))     { GameObject{id,&m_world}.physBox(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Sphere##cs"))  { GameObject{id,&m_world}.physSphere(); }
    ImGui::SameLine();
    if (ImGui::SmallButton("Capsule##cs")) { GameObject{id,&m_world}.physCapsule(); }

    ImGui::DragFloat3("Half Ext", glm::value_ptr(rb->halfExt), 0.01f, 0.001f, 100.f);
    ImGui::DragFloat ("Radius",   &rb->radius, 0.01f, 0.001f, 50.f);
    ImGui::DragFloat ("Height",   &rb->height, 0.01f, 0.001f, 50.f);

    // ── Physical params ───────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::DragFloat("Mass",         &rb->mass,        0.1f,  0.001f, 10000.f, "%.2f kg");
    ImGui::DragFloat("Friction",     &rb->friction,    0.01f, 0.f,    1.f,     "%.2f");
    ImGui::DragFloat("Restitution",  &rb->restitution, 0.01f, 0.f,    1.f,     "%.2f");
    ImGui::DragFloat("Linear Damp",  &rb->linearDamp,  0.01f, 0.f,    1.f,     "%.2f");
    ImGui::DragFloat("Angular Damp", &rb->angularDamp, 0.01f, 0.f,    1.f,     "%.2f");

    // gravity scale via GameObject - read current value from component
    float gravScale = rb->gravityScale;
    if (ImGui::DragFloat("Gravity Scale", &gravScale, 0.05f, -5.f, 10.f, "%.2f"))
        GameObject{id, &m_world}.gravityScale(gravScale);

    // ── Velocity ──────────────────────────────────────────────────────────────
    ImGui::Separator();
    glm::vec3 vel = rb->velocity;
    ImGui::TextDisabled("Velocity");
    ImGui::SameLine(72);
    ImGui::PushItemWidth(55.f);
    bool velChanged = false;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.95f,0.35f,0.35f,1.f}); ImGui::TextUnformatted("X"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##vx", &vel.x, 0.1f)) velChanged = true; ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {0.35f,0.90f,0.35f,1.f}); ImGui::TextUnformatted("Y"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##vy", &vel.y, 0.1f)) velChanged = true; ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, {0.35f,0.55f,0.95f,1.f}); ImGui::TextUnformatted("Z"); ImGui::SameLine(); ImGui::PopStyleColor();
    if (ImGui::DragFloat("##vz", &vel.z, 0.1f)) velChanged = true;
    ImGui::PopItemWidth();
    if (velChanged) GameObject{id, &m_world}.velocity(vel);

    // Impulse / Force buttons
    ImGui::Spacing();
    glm::vec3 s_impulseVec = {};
    ImGui::PushItemWidth(-80.f);
    ImGui::DragFloat3("##impvec", glm::value_ptr(s_impulseVec), 0.1f, -1000.f, 1000.f);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::SmallButton("Impulse")) GameObject{id,&m_world}.impulse(s_impulseVec);
    ImGui::SameLine();
    if (ImGui::SmallButton("Force"))   GameObject{id,&m_world}.addForce(s_impulseVec);

    // ── Lock rotation ─────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextDisabled("Lock Rot:");
    ImGui::SameLine();
    bool lx = rb->lockRotX, ly = rb->lockRotY, lz = rb->lockRotZ;
    if (ImGui::Checkbox("X##lrx", &lx)) { rb->lockRotX = lx; GameObject{id,&m_world}.lockRotX(lx); }
    ImGui::SameLine();
    if (ImGui::Checkbox("Y##lry", &ly)) { rb->lockRotY = ly; GameObject{id,&m_world}.lockRotY(ly); }
    ImGui::SameLine();
    if (ImGui::Checkbox("Z##lrz", &lz)) { rb->lockRotZ = lz; GameObject{id,&m_world}.lockRotZ(lz); }

    // ── Flags ─────────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::Checkbox("Is Sensor", &rb->isSensor);

    // ── Remove ────────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, {0.6f,0.1f,0.1f,1.f});
    if (ImGui::SmallButton("Remove##rb")) {
        m_app.physics().removeBody(id);
        m_world.remove<RigidBodyComponent>(id);
    }
    ImGui::PopStyleColor();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inspector — Audio(TODO)
// ─────────────────────────────────────────────────────────────────────────────
/*
void SceneEditor::inspectAudio(EntityID id) {
    // Audio uses a static map inside GameObject — always show the section
    if (!ImGui::CollapsingHeader("  \u266A  Audio"))
        return;

    static std::unordered_map<uint32_t, char[256]> s_pathBufs;
    static std::unordered_map<uint32_t, float>     s_vol;
    static std::unordered_map<uint32_t, float>     s_pitch;
    static std::unordered_map<uint32_t, bool>      s_3d;

    auto& pathBuf = s_pathBufs[id];
    float& vol    = s_vol.emplace(id, 1.f).first->second;
    float& pitch  = s_pitch.emplace(id, 1.f).first->second;
    bool&  use3D  = s_3d.emplace(id, false).first->second;

    // Sound path
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##sndpath", "assets/sounds/file.wav",
                                pathBuf, sizeof(pathBuf));

    // Play / Loop / Stop buttons
    ImGui::Spacing();
    GameObject go{id, &m_world};
    if (ImGui::Button("Play##snd",  {60, 0}) && pathBuf[0])
        go.playSound(pathBuf, vol);
    ImGui::SameLine();
    if (ImGui::Button("Loop##snd",  {60, 0}) && pathBuf[0])
        go.loopSound(pathBuf, vol);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, {0.5f,0.1f,0.1f,1.f});
    if (ImGui::Button("Stop##snd",  {60, 0}))
        go.stopSound();
    ImGui::PopStyleColor();

    // Volume
    ImGui::TextDisabled("Volume ");
    ImGui::SameLine(72);
    ImGui::PushItemWidth(-1);
    if (ImGui::SliderFloat("##svol", &vol, 0.f, 1.f, "%.2f"))
        go.soundVolume(vol);

    // Pitch
    ImGui::TextDisabled("Pitch  ");
    ImGui::SameLine(72);
    if (ImGui::SliderFloat("##spitch", &pitch, 0.1f, 3.f, "%.2f"))
        go.soundPitch(pitch);
    ImGui::PopItemWidth();

    // 3D toggle
    if (ImGui::Checkbox("3D Positional", &use3D))
        go.sound3D(use3D);
}
*/
// ─────────────────────────────────────────────────────────────────────────────
//  Inspector — Script
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::inspectScript(EntityID id) {
    auto* sc = m_world.get<ScriptComponent>(id);
    if (!sc) return;

    if (!ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::TextDisabled("onUpdate: %s", sc->onUpdate ? "bound (C++)" : "none");

    ImGui::Spacing();
    ImGui::TextDisabled("Script code (pseudocode / notes):");

    // Per-entity script text buffer stored in a static map
    static std::unordered_map<uint32_t, std::string> s_scriptText;
    auto& text = s_scriptText[id];
    if (text.empty())
        text = "// onUpdate example:\n"
               "// float t = Time::total();\n"
               "// self.rotY(t * 45.f);\n"
               "// self.pos(sin(t), 0, cos(t));\n";

    // Editable multiline code box — 6 lines tall
    char buf[2048];
    strncpy(buf, text.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.08f, 0.09f, 0.11f, 1.f});
    if (ImGui::InputTextMultiline("##script", buf, sizeof(buf),
        {-1, ImGui::GetTextLineHeight() * 7},
        ImGuiInputTextFlags_AllowTabInput))
    {
        text = buf;
    }
    ImGui::PopStyleColor();

    ImGui::TextDisabled("(Notes only — bind real logic in C++ via sc->onUpdate)");

    // Remove script button
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, {0.5f, 0.1f, 0.1f, 1.f});
    if (ImGui::SmallButton("Remove Script")) {
        m_world.remove<ScriptComponent>(id);
        s_scriptText.erase(id);
    }
    ImGui::PopStyleColor();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Stats overlay
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::drawStats() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos({vp->Size.x - 200.f, vp->Size.y - 110.f},
                             ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::SetNextWindowSize({196, 0}, ImGuiCond_Always);

    // Count entities before entering UI block
    int entityCount = 0;
    m_world.each<TagComp>([&](EntityID, TagComp&){ entityCount++; });

    ImGui::Begin("##stats", nullptr,
        ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoInputs    | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    UI::LabelColored(UI::WHITE,  "FPS:     %.0f", Time::fps());
    UI::LabelColored(UI::GRAY,   "Entities:%d", entityCount);
    UI::LabelColored(UI::GRAY,   "Physics: %d bodies",
        m_app.physics().bodyCount());
    UI::LabelColored(UI::GRAY,   "Audio:   %d sounds",
        Audio::activeSoundCount());
    UI::LabelColored(m_playState == PlayState::Playing ? UI::GREEN :
                     m_playState == PlayState::Paused  ? UI::YELLOW : UI::GRAY,
                     "State:  %s",
                     m_playState == PlayState::Playing ? "Playing" :
                     m_playState == PlayState::Paused  ? "Paused"  : "Editing");

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Create menu popup
// ─────────────────────────────────────────────────────────────────────────────
// ── Safe category label used inside Create Object window ─────────────────────
// NOTE: UI::Header() calls SetCursorPosY which triggers the ImGui assert
//       "Code uses SetCursorPos() to extend window/parent boundaries" when the
//       window uses AlwaysAutoResize.  We use a plain TextColored + Separator
//       here instead — no cursor repositioning, no assert.
static void CreateMenuCategory(const char* label)
{
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f,0.85f,0.f,1.f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Separator();
}

SceneType SceneEditor::activeSceneType() const {
    if (m_activeTab >= 0 && m_activeTab < (int)m_tabs.size())
        return m_tabs[m_activeTab].type;
    return SceneType::Scene3D;
}

void SceneEditor::drawCreateMenu() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos({vp->Size.x * 0.5f, ImGui::GetFrameHeight() + 72.f + 4.f},
                             ImGuiCond_Always, {0.5f, 0.f});
    ImGui::SetNextWindowSizeConstraints({220, 0}, {220, 600});
    ImGui::SetNextWindowBgAlpha(0.97f);

    if (!ImGui::Begin("Create Object", &m_showCreateMenu,
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoCollapse       |
        ImGuiWindowFlags_NoMove           |
        ImGuiWindowFlags_NoSavedSettings))
    { ImGui::End(); return; }

    // Helper: full-width button that spawns an object and closes the menu
    auto create = [&](const char* label, std::function<GameObject()> fn) {
        float w = ImGui::GetContentRegionAvail().x;
        if (ImGui::Button(label, {w, 28.f})) {
            auto obj = fn();
            m_selected = obj.id();
            if (m_onCreate) m_onCreate(m_selected);
            m_showCreateMenu = false;
        }
    };

    bool isUI = (activeSceneType() == SceneType::SceneUI);

    // ── 3D section — hidden in UI scenes ─────────────────────────────────────
    if (!isUI) {
        CreateMenuCategory("3D Primitives");
        create("Cube",     [&]{ return m_app.spawn("Cube").cube(); });
        create("Sphere",   [&]{ return m_app.spawn("Sphere").sphere(); });
        create("Cylinder", [&]{ return m_app.spawn("Cylinder").cylinder(); });
        create("Plane",    [&]{ return m_app.spawn("Plane").plane(); });
        create("Quad",     [&]{ return m_app.spawn("Quad").quad(); });

        CreateMenuCategory("Lights & Camera");
        create("Directional Light", [&]{
            auto o = m_app.spawn("DirLight");
            o.color(1.f, 0.95f, 0.85f);
            return o;
        });
    }

    // ── UI Widgets ────────────────────────────────────────────────────────────
    if (isUI) {
        // In UI scene: add directly to canvas widget list
        CreateMenuCategory("Add Widget to Canvas");

        auto addWidget = [&](UIWidget::Type type) {
            UIWidget w;
            w.type = type;
            initWidgetDefaults(w);
            w.pos    = {80.f + m_uiWidgets.size() * 12.f,
                        60.f + m_uiWidgets.size() * 12.f};
            w.zOrder = (int)m_uiWidgets.size();
            snprintf(w.name, sizeof(w.name), "%s_%d",
                     widgetTypeName(type), (int)m_uiWidgets.size()+1);
            m_uiSelIdx = (int)m_uiWidgets.size();
            m_uiWidgets.push_back(w);
            m_showCreateMenu = false;
        };

        float bw = ImGui::GetContentRegionAvail().x;
        struct E { const char* label; UIWidget::Type type; };
        static const E entries[] = {
            {"Button",       UIWidget::Type::Button},
            {"Label",        UIWidget::Type::Label},
            {"Colored Text", UIWidget::Type::ColoredText},
            {"Text Field",   UIWidget::Type::TextField},
            {"Slider",       UIWidget::Type::Slider},
            {"Toggle",       UIWidget::Type::Toggle},
            {"Progress Bar", UIWidget::Type::ProgressBar},
            {"Panel",        UIWidget::Type::Panel},
            {"Image",        UIWidget::Type::Image},
            {"Dropdown",     UIWidget::Type::Dropdown},
        };
        for (auto& e : entries) {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.10f,0.15f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {1.f,0.85f,0.f,0.12f});
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
            if (ImGui::Button(e.label, {bw, 28.f})) addWidget(e.type);
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
        }

    } else {
        // In 3D scene — UI objects have world-space transform
        CreateMenuCategory("UI Widgets (world-space)");
        create("Button (3D)",       [&]{ return m_app.spawn("UI_Button"); });
        create("Label (3D)",        [&]{ return m_app.spawn("UI_Label"); });
        create("Slider (3D)",       [&]{ return m_app.spawn("UI_Slider"); });
        create("Text Field (3D)",   [&]{ return m_app.spawn("UI_TextField"); });
        create("Colored Text (3D)", [&]{ return m_app.spawn("UI_ColoredText"); });
        create("Image (3D)",        [&]{ return m_app.spawn("UI_Image"); });

        // ── Empty ─────────────────────────────────────────────────────────────
        CreateMenuCategory("Empty");
        create("Empty Object", [&]{ return m_app.spawn("Empty"); });
    }

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hotkeys
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::handleHotkeys() {
    ImGuiIO& io = ImGui::GetIO();

    // NEVER fire hotkeys while user is typing in any ImGui widget
    if (io.WantCaptureKeyboard) return;

    // Also don't fire if cursor is captured (camera mode)
    // — except F1 which always works
    bool camMode = m_app.window().isCursorCaptured();

    if (ImGui::IsKeyPressed(ImGuiKey_F1))
        toggle();

    if (camMode) return; // rest of hotkeys only in editor mode

    if (!m_visible) return;

    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && m_selected)
        deleteSelected();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && m_selected)
        duplicateSelected();

    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))
        undo();

    if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) ||
        (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))))
        redo();

    if (ImGui::IsKeyPressed(ImGuiKey_F) && m_selected)
        focusCameraOn(m_selected);

    // Gizmo mode — W/E/R only when not in camera mode
    if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmoMode = GizmoMode::Translate;
    if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmoMode = GizmoMode::Rotate;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmoMode = GizmoMode::Scale;

    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        if (m_playState == PlayState::Editing)       m_playState = PlayState::Playing;
        else if (m_playState == PlayState::Playing)  m_playState = PlayState::Paused;
        else                                         m_playState = PlayState::Playing;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Actions
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::deleteSelected() {
    if (m_selected == NULL_ENTITY) return;

    EntityID id = m_selected;
    std::string name = entityDisplayName(id);

    // Store component snapshot for undo
    TransformComponent* t  = m_world.get<TransformComponent>(id);
    TransformComponent  tc = t ? *t : TransformComponent{};

    pushUndo({"Delete " + name,
        [this, name, tc]() mutable {
            m_app.spawn(name).pos(tc.position.x, tc.position.y, tc.position.z);
            LOG_INFO("Undo: recreated " << name);
        },
        [this, id]() mutable {
            m_world.destroy(id);
        }
    });

    if (m_onDelete) m_onDelete(id);
    m_app.physics().removeBody(id);
    m_world.destroy(id);
    m_selected = NULL_ENTITY;
    LOG_INFO("Editor: deleted entity " << id);
}

void SceneEditor::duplicateSelected() {
    if (m_selected == NULL_ENTITY) return;

    std::string name = entityDisplayName(m_selected) + " (copy)";
    auto newObj = m_app.spawn(name);
    EntityID newId = newObj.id();

    // Copy transform
    if (auto* t = m_world.get<TransformComponent>(m_selected)) {
        if (auto* nt = m_world.get<TransformComponent>(newId)) {
            *nt = *t;
            nt->position += glm::vec3(0.5f, 0, 0);
        }
    }

    // Copy mesh renderer — cache BEFORE add() which may reallocate the pool
    if (m_world.has<MeshRendererComponent>(m_selected)) {
        MeshRendererComponent mrCopy = *m_world.get<MeshRendererComponent>(m_selected);
        if (!m_world.has<MeshRendererComponent>(newId))
            m_world.add<MeshRendererComponent>(newId);
        *m_world.get<MeshRendererComponent>(newId) = mrCopy;
    }

    // Copy RigidBody — re-register body with physics
    if (m_world.has<RigidBodyComponent>(m_selected)) {
        RigidBodyComponent rbCopy = *m_world.get<RigidBodyComponent>(m_selected);
        rbCopy._bodyId = UINT32_MAX; // must get fresh Jolt handle
        if (!m_world.has<RigidBodyComponent>(newId))
            m_world.add<RigidBodyComponent>(newId);
        *m_world.get<RigidBodyComponent>(newId) = rbCopy;
        auto* nt = m_world.get<TransformComponent>(newId);
        glm::vec3 pos = nt ? nt->position : glm::vec3(0);
        glm::quat rot = nt ? glm::quat(glm::radians(nt->rotation)) : glm::quat{1,0,0,0};
        m_app.physics().addBody(newId, *m_world.get<RigidBodyComponent>(newId), pos, rot);
    }

    m_selected = newId;
    if (m_onCreate) m_onCreate(m_selected);
    LOG_INFO("Editor: duplicated → entity " << m_selected);
}

void SceneEditor::focusCameraOn(EntityID id) {
    auto* t = m_world.get<TransformComponent>(id);
    if (!t) return;
    glm::vec3 pos  = t->position;
    float     dist = glm::length(t->scale) * 2.5f + 3.f;
    m_app.camPos(pos.x, pos.y + dist * 0.35f, pos.z + dist);
    m_app.camRot(-90.f, -20.f);
    LOG_INFO("Editor: camera focused on entity " << id);
}

void SceneEditor::pushUndo(UndoAction action) {
    // Truncate redo history
    if (m_undoIdx < (int)m_undoStack.size() - 1)
        m_undoStack.erase(m_undoStack.begin() + m_undoIdx + 1, m_undoStack.end());
    m_undoStack.push_back(std::move(action));
    if (m_undoStack.size() > 50) m_undoStack.erase(m_undoStack.begin());
    m_undoIdx = (int)m_undoStack.size() - 1;
    // Mark current scene tab as modified
    if (m_activeTab >= 0 && m_activeTab < (int)m_tabs.size())
        m_tabs[m_activeTab].dirty = true;
}

void SceneEditor::undo() {
    if (m_undoIdx < 0) return;
    m_undoStack[m_undoIdx].undo();
    LOG_INFO("Undo: " << m_undoStack[m_undoIdx].description);
    --m_undoIdx;
}

void SceneEditor::redo() {
    if (m_undoIdx >= (int)m_undoStack.size() - 1) return;
    ++m_undoIdx;
    m_undoStack[m_undoIdx].redo();
    LOG_INFO("Redo: " << m_undoStack[m_undoIdx].description);
}

std::string SceneEditor::entityDisplayName(EntityID id) const {
    auto* t = m_world.get<TagComp>(id);
    return t ? t->value : ("Entity " + std::to_string(id));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pick entity under cursor
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
//  drawGizmo — 3D handles + selection overlay
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::drawGizmo()
{
    if (m_playState == PlayState::Playing) return;
    // No 3D gizmo in UI scenes — objects are positioned via inspector only
    if (activeSceneType() == SceneType::SceneUI) return;

    // ── Draw 3D gizmo for selected entity ────────────────────────────────────
    if (m_selected != NULL_ENTITY)
        Gizmo::draw(m_app, m_selected, m_gizmoMode);

    // ── Selection info overlay (top-center of viewport) ───────────────────────
    if (m_selected == NULL_ENTITY) return;

    auto* t  = m_world.get<TransformComponent>(m_selected);
    if (!t) return;

    ImGui::SetNextWindowPos(
        { (float)m_app.window().width() * 0.5f, 52.f },
        ImGuiCond_Always, { 0.5f, 0.f });
    ImGui::SetNextWindowBgAlpha(0.60f);
    ImGui::SetNextWindowSize({0, 0}); // auto-size

    ImGui::Begin("##selinfo", nullptr,
        ImGuiWindowFlags_NoTitleBar     |
        ImGuiWindowFlags_NoResize       |
        ImGuiWindowFlags_NoMove         |
        ImGuiWindowFlags_NoScrollbar    |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoInputs       |
        ImGuiWindowFlags_NoNav          |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    std::string name = entityDisplayName(m_selected);
    UI::LabelColored(UI::YELLOW, "%s  [ID:%u]", name.c_str(), m_selected);
    UI::LabelColored(UI::WHITE,
        "P: %.2f  %.2f  %.2f",
        t->position.x, t->position.y, t->position.z);
    UI::LabelColored(UI::WHITE,
        "R: %.1f  %.1f  %.1f",
        t->rotation.x, t->rotation.y, t->rotation.z);
    UI::LabelColored(UI::WHITE,
        "S: %.2f  %.2f  %.2f",
        t->scale.x, t->scale.y, t->scale.z);

    ImGui::Spacing();
    UI::LabelColored(
        m_gizmoMode == GizmoMode::Translate ? UI::CYAN :
        m_gizmoMode == GizmoMode::Rotate    ? UI::ORANGE : UI::GREEN,
        m_gizmoMode == GizmoMode::Translate ? "W  Move" :
        m_gizmoMode == GizmoMode::Rotate    ? "E  Rotate" : "R  Scale");
    UI::LabelColored(UI::GRAY, "Drag axis arrow  |  F = focus");

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Mouse picking — click on object in viewport to select it
// ─────────────────────────────────────────────────────────────────────────────
void SceneEditor::handleMousePick() {
    // Only pick in Editing mode
    if (m_playState != PlayState::Editing) return;

    // Don't pick if ImGui is capturing mouse
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    // Only on fresh left-click, not drag
    static bool s_prevLMB = false;
    bool lmbDown  = m_app.mouse(0);
    bool lmbClick = lmbDown && !s_prevLMB;
    s_prevLMB = lmbDown;

    if (!lmbClick) return;

    // Don't pick if gizmo is being dragged
    if (Gizmo::isDragging()) return;

    // Only pick inside the 3D viewport area (not over panels)
    glm::vec2 mouse = m_app.mousePos();
    int W = m_app.window().width();
    int H = m_app.window().height();

    // Panels: Hierarchy on left (~248px), Inspector on right (~264px), Toolbar top (46px)
    bool inViewport = (mouse.x > 248 &&
                        mouse.x < W - 264 &&
                        mouse.y > (ImGui::GetFrameHeight() + 72.f) &&
                        mouse.y < H);
    if (!inViewport) return;

    // Build ray from mouse position
    glm::mat4 view   = m_app.camera().viewMatrix();
    glm::mat4 proj   = m_app.camera().projectionMatrix();
    glm::vec3 camPos = m_app.camera().position();

    float nx = (2.f * mouse.x) / W - 1.f;
    float ny = 1.f - (2.f * mouse.y) / H;
    glm::vec4 clip(nx, ny, -1.f, 1.f);
    glm::vec4 eye = glm::inverse(proj) * clip;
    eye.z = -1.f; eye.w = 0.f;
    glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(view) * eye));

    // Ray-AABB test against all visible mesh entities
    EntityID bestId   = NULL_ENTITY;
    float    bestDist = 1e9f;

    m_world.each<TransformComponent, MeshRendererComponent>(
        [&](EntityID id, TransformComponent& tr, MeshRendererComponent& mr) {
            if (!mr.visible) return;

            // Use model bounds if available, otherwise unit cube
            glm::vec3 bmin = mr.model ? mr.model->boundsMin : glm::vec3(-0.5f);
            glm::vec3 bmax = mr.model ? mr.model->boundsMax : glm::vec3( 0.5f);

            // Transform 8 AABB corners to world space → rebuild world AABB
            glm::mat4 mat = tr.matrix();
            glm::vec3 wmin(1e9f), wmax(-1e9f);
            for (int cx = 0; cx < 2; ++cx)
            for (int cy = 0; cy < 2; ++cy)
            for (int cz = 0; cz < 2; ++cz) {
                glm::vec3 corner(cx ? bmax.x : bmin.x,
                                cy ? bmax.y : bmin.y,
                                cz ? bmax.z : bmin.z);
                glm::vec3 w = glm::vec3(mat * glm::vec4(corner, 1.f));
                wmin = glm::min(wmin, w);
                wmax = glm::max(wmax, w);
            }

            float t;
            if (rayAABB(camPos, rayDir, wmin, wmax, t) && t < bestDist) {
                bestDist = t;
                bestId   = id;
            }
        });

    if (bestId != NULL_ENTITY) {
        m_selected = bestId;
        if (m_onSelect) m_onSelect(bestId);
        Gizmo::reset();
        LOG_INFO("Editor: picked entity " << bestId
                << " (" << entityDisplayName(bestId) << ")"
                << " dist=" << bestDist);
    } else {
        m_selected = NULL_ENTITY;
        Gizmo::reset();
    }
}


// ─────────────────────────────────────────────────────────────────────────────
//  UI Scene Editor
// ─────────────────────────────────────────────────────────────────────────────

// UIWidget struct is defined in SceneEditor.h


void SceneEditor::drawUIScene()
{
    // ── Per-frame shorthand refs ───────────────────────────────────────────────
    auto& s_widgets    = m_uiWidgets;
    auto& s_dragIdx    = m_uiDragIdx;
    auto& s_selIdx     = m_uiSelIdx;
    auto& s_dragOffset = m_uiDragOffset;

    static bool    s_snapToGrid   = true;
    static float   s_gridStep     = 8.f;
    static bool    s_showGrid     = true;
    static int     s_resizeCorner = -1;
    static ImVec2  s_resizeStart  = {0,0};
    static ImVec2  s_resizePosStart   = {0,0};
    static ImVec2  s_resizeSizeStart  = {0,0};
    static bool    s_isResizing   = false;

    const float WW = (float)m_app.window().width();
    const float WH = (float)m_app.window().height();
    const float TOOLBAR_H = ImGui::GetFrameHeight() + 72.f;   // menubar + toolbar + tabs

    // Hierarchy and Inspector keep their normal positions.
    // Canvas fills the space between them.
    const float HIER_W  = 248.f;
    const float INSP_W  = 264.f;
    const float CANVAS_X = HIER_W + 4.f;
    const float CANVAS_W = WW - CANVAS_X - INSP_W - 4.f;
    const float CANVAS_H = WH - TOOLBAR_H - 2.f;

    ImGuiIO& io = ImGui::GetIO();

    // ─────────────────────────────────────────────────────────────────────────
    //  CANVAS WINDOW
    // ─────────────────────────────────────────────────────────────────────────
    ImGui::SetNextWindowPos ({CANVAS_X, TOOLBAR_H + 2.f}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({CANVAS_W, CANVAS_H},         ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    {0.f, 0.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.05f,0.05f,0.08f,1.f});
    ImGui::PushStyleColor(ImGuiCol_Border,   {1.f,0.85f,0.f,0.30f});

    ImGui::Begin("##UICanvas", nullptr,
        ImGuiWindowFlags_NoMove             |
        ImGuiWindowFlags_NoResize           |
        ImGuiWindowFlags_NoCollapse         |
        ImGuiWindowFlags_NoScrollbar        |
        ImGuiWindowFlags_NoScrollWithMouse  |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings    |
        ImGuiWindowFlags_NoTitleBar);

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);

    ImDrawList* dl       = ImGui::GetWindowDrawList();
    ImVec2 canvasOrigin  = ImGui::GetWindowPos();

    // ── Title / toolbar bar ───────────────────────────────────────────────────
    const float TB_H = 28.f;
    dl->AddRectFilled(canvasOrigin, {canvasOrigin.x + CANVAS_W, canvasOrigin.y + TB_H},
                      IM_COL32(8,8,12,245));
    dl->AddLine({canvasOrigin.x, canvasOrigin.y + TB_H},
                {canvasOrigin.x + CANVAS_W, canvasOrigin.y + TB_H},
                IM_COL32(255,218,0,50), 1.f);

    // Toolbar widgets (snap, grid toggles)
    ImGui::SetCursorPos({8.f, 6.f});
    ImGui::PushStyleColor(ImGuiCol_Text, {1.f,0.85f,0.f,0.85f});
    ImGui::TextUnformatted("UI Canvas");
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 20.f);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, {1.f,0.85f,0.f,1.f});
    ImGui::PushStyleColor(ImGuiCol_Text,      {0.65f,0.63f,0.58f,1.f});
    ImGui::Checkbox("Snap", &s_snapToGrid);
    ImGui::SameLine(0, 12.f);
    ImGui::Checkbox("Grid", &s_showGrid);
    ImGui::PopStyleColor(2);

    // Widget count right-aligned
    {
        char cnt[32]; snprintf(cnt, sizeof(cnt), "%d widget(s)", (int)s_widgets.size());
        ImVec2 ts = ImGui::CalcTextSize(cnt);
        float rx = CANVAS_W - ts.x - 10.f;
        dl->AddText({canvasOrigin.x + rx, canvasOrigin.y + 7.f},
                    IM_COL32(100,98,90,180), cnt);
    }

    // ── Content area ──────────────────────────────────────────────────────────
    const float SB_H = 20.f;  // status bar
    ImVec2 contentOrigin = {canvasOrigin.x, canvasOrigin.y + TB_H};
    ImVec2 contentSize   = {CANVAS_W, CANVAS_H - TB_H - SB_H};

    dl->AddRectFilled(contentOrigin,
        {contentOrigin.x + contentSize.x, contentOrigin.y + contentSize.y},
        IM_COL32(10,10,15,255));

    // ── Grid ─────────────────────────────────────────────────────────────────
    if (s_showGrid) {
        float g = s_gridStep * 4.f;
        for (float x = 0; x < contentSize.x; x += g)
            dl->AddLine({contentOrigin.x+x, contentOrigin.y},
                        {contentOrigin.x+x, contentOrigin.y+contentSize.y},
                        IM_COL32(26,26,38,255), 1.f);
        for (float y = 0; y < contentSize.y; y += g)
            dl->AddLine({contentOrigin.x, contentOrigin.y+y},
                        {contentOrigin.x+contentSize.x, contentOrigin.y+y},
                        IM_COL32(26,26,38,255), 1.f);
        for (float x = 0; x < contentSize.x; x += s_gridStep)
            for (float y = 0; y < contentSize.y; y += s_gridStep)
                dl->AddRectFilled(
                    {contentOrigin.x+x-0.5f, contentOrigin.y+y-0.5f},
                    {contentOrigin.x+x+0.5f, contentOrigin.y+y+0.5f},
                    IM_COL32(38,38,52,180));
    }

    // Drop target for drag from palette / hierarchy add
    ImGui::SetCursorScreenPos(contentOrigin);
    ImGui::InvisibleButton("##cvdrop", contentSize, ImGuiButtonFlags_MouseButtonLeft);
    bool canvasHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("UI_WIDGET_TYPE")) {
            int t = *(const int*)payload->Data;
            ImVec2 mp = ImGui::GetMousePos();
            UIWidget w;
            w.type = (UIWidget::Type)t;
            initWidgetDefaults(w);
            float cx = mp.x - contentOrigin.x - w.size.x * 0.5f;
            float cy = mp.y - contentOrigin.y - w.size.y * 0.5f;
            if (s_snapToGrid) {
                cx = std::round(cx / s_gridStep) * s_gridStep;
                cy = std::round(cy / s_gridStep) * s_gridStep;
            }
            w.pos.x = std::max(0.f, std::min(cx, contentSize.x - w.size.x));
            w.pos.y = std::max(0.f, std::min(cy, contentSize.y - w.size.y));
            snprintf(w.name, sizeof(w.name), "%s_%d", widgetTypeName(w.type), (int)s_widgets.size()+1);
            w.zOrder = (int)s_widgets.size();
            s_selIdx = (int)s_widgets.size();
            s_widgets.push_back(w);
        }
        ImGui::EndDragDropTarget();
    }

    // ── Sorted render order ───────────────────────────────────────────────────
    std::vector<int> zSorted;
    zSorted.reserve(s_widgets.size());
    for (int i = 0; i < (int)s_widgets.size(); i++) zSorted.push_back(i);
    std::stable_sort(zSorted.begin(), zSorted.end(),
        [&](int a, int b){ return s_widgets[a].zOrder < s_widgets[b].zOrder; });

    auto snap = [&](float v) -> float {
        return s_snapToGrid ? std::round(v / s_gridStep) * s_gridStep : v;
    };

    // ── Render widgets (back → front) ─────────────────────────────────────────
    for (int i : zSorted) {
        UIWidget& w = s_widgets[i];
        if (!w.visible) continue;

        ImVec2 wMin = {contentOrigin.x + w.pos.x, contentOrigin.y + w.pos.y};
        ImVec2 wMax = {wMin.x + w.size.x, wMin.y + w.size.y};
        bool isSel = (s_selIdx == i);
        bool hov   = io.MousePos.x >= wMin.x && io.MousePos.x <= wMax.x &&
                     io.MousePos.y >= wMin.y && io.MousePos.y <= wMax.y && canvasHovered;

        dl->PushClipRect(contentOrigin,
            {contentOrigin.x + contentSize.x, contentOrigin.y + contentSize.y}, true);

        switch (w.type) {
        case UIWidget::Type::Panel: {
            ImU32 bgCol = ImGui::ColorConvertFloat4ToU32(w.bgColor);
            ImU32 bdCol = ImGui::ColorConvertFloat4ToU32(w.borderColor);
            dl->AddRectFilled(wMin, wMax, bgCol, w.rounding);
            dl->AddRectFilled(wMin, {wMax.x, wMin.y+18.f}, IM_COL32(0,0,0,80), w.rounding, ImDrawFlags_RoundCornersTop);
            dl->AddRect(wMin, wMax, bdCol, w.rounding, 0, w.borderWidth);
            ImVec2 ts = ImGui::CalcTextSize(w.text);
            dl->AddText({wMin.x+6.f, wMin.y+(18.f-ts.y)*0.5f}, IM_COL32(200,195,185,200), w.text);
            if (isSel||hov) dl->AddRect(wMin,wMax, isSel?IM_COL32(255,218,0,220):IM_COL32(255,218,0,90), w.rounding, 0, isSel?2.f:1.f);
            break;
        }
        case UIWidget::Type::Button: {
            ImU32 bgCol = hov ? IM_COL32(20,20,30,240) : ImGui::ColorConvertFloat4ToU32(w.bgColor);
            dl->AddRectFilled(wMin, wMax, bgCol, w.rounding);
            if (hov) dl->AddRectFilled(wMin, wMax,
                IM_COL32((int)(w.color.x*255),(int)(w.color.y*255),(int)(w.color.z*255),22), w.rounding);
            ImU32 bdCol = isSel ? ImGui::ColorConvertFloat4ToU32(w.color) : (hov ? IM_COL32_WHITE : ImGui::ColorConvertFloat4ToU32(w.borderColor));
            dl->AddRect(wMin, wMax, bdCol, w.rounding, 0, isSel?2.f:1.f);
            if (hov||isSel) dl->AddLine({wMin.x+w.rounding,wMax.y-1.f},{wMax.x-w.rounding,wMax.y-1.f}, ImGui::ColorConvertFloat4ToU32(w.color), 2.f);
            ImVec2 ts = ImGui::CalcTextSize(w.text);
            dl->AddText({wMin.x+(w.size.x-ts.x)*0.5f, wMin.y+(w.size.y-ts.y)*0.5f}, ImGui::ColorConvertFloat4ToU32(w.color), w.text);
            break;
        }
        case UIWidget::Type::Label: {
            if (isSel||hov) dl->AddRect({wMin.x-2,wMin.y-2},{wMax.x+2,wMax.y+2}, IM_COL32(255,218,0,isSel?160:60), 2.f, 0, 1.f);
            ImVec2 ts = ImGui::CalcTextSize(w.text);
            dl->AddText({wMin.x, wMin.y+(w.size.y-ts.y)*0.5f}, ImGui::ColorConvertFloat4ToU32(w.color), w.text);
            break;
        }
        case UIWidget::Type::ColoredText: {
            if (isSel||hov) dl->AddRect({wMin.x-2,wMin.y-2},{wMax.x+2,wMax.y+2}, IM_COL32(255,218,0,isSel?160:60), 2.f, 0, 1.f);
            dl->AddText({wMin.x+1,wMin.y+1}, IM_COL32(0,0,0,80), w.text);
            dl->AddText(wMin, ImGui::ColorConvertFloat4ToU32(w.color), w.text);
            break;
        }
        case UIWidget::Type::Slider: {
            dl->AddText({wMin.x, wMin.y-15.f}, IM_COL32(150,148,140,200), w.text);
            float trackY = wMin.y+w.size.y*0.5f;
            float tx0=wMin.x+8.f, tx1=wMax.x-8.f;
            float grabX = tx0+(tx1-tx0)*w.sliderVal;
            dl->AddRectFilled({tx0,trackY-2.f},{tx1,trackY+2.f}, IM_COL32(30,30,42,255), 2.f);
            dl->AddRectFilled({tx0,trackY-2.f},{grabX,trackY+2.f}, ImGui::ColorConvertFloat4ToU32(w.color), 2.f);
            dl->AddCircleFilled({grabX,trackY}, 8.f, ImGui::ColorConvertFloat4ToU32(w.color));
            dl->AddCircle({grabX,trackY}, 8.f, IM_COL32(255,255,255,120), 0, 1.5f);
            char vb[16]; snprintf(vb,sizeof(vb),"%.2f",w.sliderVal);
            ImVec2 vts=ImGui::CalcTextSize(vb);
            dl->AddText({wMax.x-vts.x-2.f, wMin.y-15.f}, IM_COL32(255,218,0,180), vb);
            if (isSel) dl->AddRect({wMin.x-2,wMin.y-19},{wMax.x+2,wMax.y+2}, IM_COL32(255,218,0,160), 0.f, 0, 1.f);
            bool grabHov = std::abs(io.MousePos.x-grabX)<10.f && std::abs(io.MousePos.y-trackY)<10.f;
            if (grabHov && ImGui::IsMouseClicked(0)) { s_selIdx=i; s_dragIdx=-999; }
            if (s_dragIdx==-999 && s_selIdx==i && ImGui::IsMouseDown(0)) {
                float t = (io.MousePos.x-tx0)/(tx1-tx0);
                w.sliderVal = std::max(0.f, std::min(1.f, t));
            }
            if (!ImGui::IsMouseDown(0) && s_dragIdx==-999) s_dragIdx=-1;
            break;
        }
        case UIWidget::Type::TextField: {
            dl->AddRectFilled(wMin, wMax, ImGui::ColorConvertFloat4ToU32(w.bgColor), w.rounding);
            dl->AddRect(wMin, wMax, isSel?IM_COL32(255,218,0,200):IM_COL32(70,72,90,200), w.rounding, 0, isSel?1.5f:1.f);
            const char* txt = w.inputBuf[0] ? w.inputBuf : w.text;
            ImU32 tc = w.inputBuf[0] ? ImGui::ColorConvertFloat4ToU32(w.color) : IM_COL32(90,92,110,200);
            ImVec2 ts=ImGui::CalcTextSize(txt);
            dl->AddText({wMin.x+7.f, wMin.y+(w.size.y-ts.y)*0.5f}, tc, txt);
            if (isSel && (int)(ImGui::GetTime()*2.0)%2==0) {
                float cx2=wMin.x+7.f+ts.x+2.f;
                dl->AddLine({cx2,wMin.y+4.f},{cx2,wMax.y-4.f}, IM_COL32(255,218,0,200), 1.f);
            }
            break;
        }
        case UIWidget::Type::Image: {
            dl->AddRectFilled(wMin, wMax, ImGui::ColorConvertFloat4ToU32(w.bgColor), w.rounding);
            dl->AddRect(wMin, wMax, IM_COL32(80,82,100,200), w.rounding, 0, 1.f);
            dl->AddLine({wMin.x+4,wMin.y+4},{wMax.x-4,wMax.y-4}, IM_COL32(80,82,100,160), 1.f);
            dl->AddLine({wMax.x-4,wMin.y+4},{wMin.x+4,wMax.y-4}, IM_COL32(80,82,100,160), 1.f);
            ImVec2 center={wMin.x+w.size.x*0.5f,wMin.y+w.size.y*0.5f};
            ImVec2 ts=ImGui::CalcTextSize(w.text);
            dl->AddText({center.x-ts.x*0.5f,center.y-ts.y*0.5f}, IM_COL32(120,122,140,200), w.text);
            if (isSel||hov) dl->AddRect(wMin,wMax, isSel?IM_COL32(255,218,0,200):IM_COL32(255,218,0,80), w.rounding, 0, isSel?2.f:1.f);
            break;
        }
        case UIWidget::Type::Toggle: {
            if (isSel||hov) dl->AddRect({wMin.x-2,wMin.y-2},{wMax.x+2,wMax.y+2}, IM_COL32(255,218,0,isSel?160:60), 2.f, 0, 1.f);
            float boxSz=w.size.y-4.f;
            ImVec2 boxMin={wMin.x+2.f,wMin.y+2.f};
            float tW=boxSz*1.8f;
            dl->AddRectFilled(boxMin,{boxMin.x+tW,boxMin.y+boxSz}, IM_COL32(30,30,42,255), boxSz*0.5f);
            if (w.toggled) dl->AddRectFilled(boxMin,{boxMin.x+tW,boxMin.y+boxSz}, ImGui::ColorConvertFloat4ToU32(w.color), boxSz*0.5f);
            float kx=w.toggled ? boxMin.x+tW-boxSz*0.5f : boxMin.x+boxSz*0.5f;
            float ky=boxMin.y+boxSz*0.5f;
            dl->AddCircleFilled({kx,ky}, boxSz*0.44f, IM_COL32(230,228,220,240));
            ImVec2 ts=ImGui::CalcTextSize(w.text);
            dl->AddText({boxMin.x+tW+8.f, wMin.y+(w.size.y-ts.y)*0.5f}, ImGui::ColorConvertFloat4ToU32(w.color), w.text);
            break;
        }
        case UIWidget::Type::ProgressBar: {
            dl->AddRectFilled(wMin, wMax, ImGui::ColorConvertFloat4ToU32(w.bgColor), w.rounding);
            float fw=w.size.x*std::max(0.f,std::min(1.f,w.progress));
            if (fw>0.f) {
                ImVec4 dimC={w.color.x*0.8f,w.color.y*0.8f,w.color.z*0.8f,1.f};
                dl->AddRectFilledMultiColor(wMin,{wMin.x+fw,wMax.y},
                    ImGui::ColorConvertFloat4ToU32(dimC), ImGui::ColorConvertFloat4ToU32(w.color),
                    ImGui::ColorConvertFloat4ToU32(w.color), ImGui::ColorConvertFloat4ToU32(dimC));
                dl->AddRectFilled(wMin,{wMin.x+fw,wMin.y+w.size.y*0.4f}, IM_COL32(255,255,255,16));
            }
            dl->AddRect(wMin,wMax, ImGui::ColorConvertFloat4ToU32(w.borderColor), w.rounding, 0, 1.f);
            char pct[16]; snprintf(pct,sizeof(pct),"%.0f%%",w.progress*100.f);
            ImVec2 ts=ImGui::CalcTextSize(pct);
            dl->AddText({wMin.x+(w.size.x-ts.x)*0.5f,wMin.y+(w.size.y-ts.y)*0.5f}, IM_COL32(235,230,220,200), pct);
            if (isSel||hov) dl->AddRect(wMin,wMax, isSel?IM_COL32(255,218,0,200):IM_COL32(255,218,0,80), w.rounding, 0, isSel?2.f:1.f);
            break;
        }
        case UIWidget::Type::Dropdown: {
            dl->AddRectFilled(wMin, wMax, ImGui::ColorConvertFloat4ToU32(w.bgColor), w.rounding);
            dl->AddRect(wMin, wMax, isSel?IM_COL32(255,218,0,200):IM_COL32(70,72,90,180), w.rounding, 0, isSel?1.5f:1.f);
            float arW=w.size.y; float ax=wMax.x-arW*0.5f, ay=wMin.y+w.size.y*0.5f;
            dl->AddLine({ax-4.f,ay-2.f},{ax,ay+3.f}, IM_COL32(180,178,168,200), 1.5f);
            dl->AddLine({ax+4.f,ay-2.f},{ax,ay+3.f}, IM_COL32(180,178,168,200), 1.5f);
            dl->AddLine({wMax.x-arW,wMin.y+3.f},{wMax.x-arW,wMax.y-3.f}, IM_COL32(70,72,90,150), 1.f);
            ImVec2 ts=ImGui::CalcTextSize(w.text);
            dl->AddText({wMin.x+8.f,wMin.y+(w.size.y-ts.y)*0.5f}, ImGui::ColorConvertFloat4ToU32(w.color), w.text);
            break;
        }
        }

        dl->PopClipRect();
    }

    // ── Interaction: pick top-most widget ─────────────────────────────────────
    if (!s_isResizing && s_dragIdx != -999) {
        for (int zi = (int)zSorted.size()-1; zi >= 0; zi--) {
            int i = zSorted[zi];
            if (!s_widgets[i].visible) continue;
            ImVec2 mn={contentOrigin.x+s_widgets[i].pos.x, contentOrigin.y+s_widgets[i].pos.y};
            ImVec2 mx={mn.x+s_widgets[i].size.x, mn.y+s_widgets[i].size.y};
            if (io.MousePos.x>=mn.x && io.MousePos.x<=mx.x &&
                io.MousePos.y>=mn.y && io.MousePos.y<=mx.y && canvasHovered)
            {
                if (ImGui::IsMouseClicked(0)) {
                    s_selIdx     = i;
                    s_dragIdx    = i;
                    s_dragOffset = {io.MousePos.x-mn.x, io.MousePos.y-mn.y};
                }
                break;
            }
        }
    }

    // ── Drag to move ──────────────────────────────────────────────────────────
    if (s_dragIdx >= 0 && s_dragIdx < (int)s_widgets.size() && ImGui::IsMouseDown(0) && !s_isResizing) {
        UIWidget& w = s_widgets[s_dragIdx];
        float nx = io.MousePos.x - contentOrigin.x - s_dragOffset.x;
        float ny = io.MousePos.y - contentOrigin.y - s_dragOffset.y;
        w.pos.x = snap(std::max(0.f, std::min(nx, contentSize.x - w.size.x)));
        w.pos.y = snap(std::max(0.f, std::min(ny, contentSize.y - w.size.y)));
    }
    if (!ImGui::IsMouseDown(0) && s_dragIdx >= 0) s_dragIdx = -1;

    // ── Selection highlight + resize handles ──────────────────────────────────
    if (s_selIdx >= 0 && s_selIdx < (int)s_widgets.size()) {
        UIWidget& w = s_widgets[s_selIdx];
        ImVec2 wMin={contentOrigin.x+w.pos.x, contentOrigin.y+w.pos.y};
        ImVec2 wMax={wMin.x+w.size.x, wMin.y+w.size.y};

        dl->AddRect(wMin, wMax, IM_COL32(255,218,0,255), 0.f, 0, 1.5f);

        // Corner handles
        float hSz = 6.f;
        ImVec2 corners[4] = {{wMin.x,wMin.y},{wMax.x,wMin.y},{wMin.x,wMax.y},{wMax.x,wMax.y}};
        for (int c=0; c<4; c++) {
            dl->AddRectFilled(
                {corners[c].x-hSz*0.5f,corners[c].y-hSz*0.5f},
                {corners[c].x+hSz*0.5f,corners[c].y+hSz*0.5f},
                IM_COL32(255,218,0,240));
            dl->AddRect(
                {corners[c].x-hSz*0.5f,corners[c].y-hSz*0.5f},
                {corners[c].x+hSz*0.5f,corners[c].y+hSz*0.5f},
                IM_COL32(20,18,10,200));
            bool hHov = std::abs(io.MousePos.x-corners[c].x)<=hSz &&
                        std::abs(io.MousePos.y-corners[c].y)<=hSz && canvasHovered;
            if (hHov && ImGui::IsMouseClicked(0)) {
                s_isResizing     = true;
                s_resizeCorner   = c;
                s_resizeStart    = io.MousePos;
                s_resizePosStart = w.pos;
                s_resizeSizeStart= w.size;
                s_dragIdx = -1;
            }
        }

        // Ruler lines
        ImU32 rc = IM_COL32(255,218,0,25);
        dl->AddLine({contentOrigin.x,wMin.y},{wMin.x,wMin.y},rc);
        dl->AddLine({contentOrigin.x,wMax.y},{wMin.x,wMax.y},rc);
        dl->AddLine({wMin.x,contentOrigin.y},{wMin.x,wMin.y},rc);
        dl->AddLine({wMax.x,contentOrigin.y},{wMax.x,wMin.y},rc);

        // Size label
        char szLbl[32]; snprintf(szLbl,sizeof(szLbl),"%.0fx%.0f",w.size.x,w.size.y);
        ImVec2 szTs=ImGui::CalcTextSize(szLbl);
        float lx=wMin.x+(w.size.x-szTs.x)*0.5f, ly=wMax.y+4.f;
        if (ly+szTs.y < contentOrigin.y+contentSize.y)
            dl->AddText({lx,ly}, IM_COL32(255,218,0,130), szLbl);
    }

    // ── Resize logic ──────────────────────────────────────────────────────────
    if (s_isResizing && s_selIdx >= 0 && s_selIdx < (int)s_widgets.size()) {
        if (ImGui::IsMouseDown(0)) {
            UIWidget& w = s_widgets[s_selIdx];
            float dx = io.MousePos.x - s_resizeStart.x;
            float dy = io.MousePos.y - s_resizeStart.y;
            switch (s_resizeCorner) {
            case 0: w.pos.x=snap(std::max(0.f,s_resizePosStart.x+dx)); w.pos.y=snap(std::max(0.f,s_resizePosStart.y+dy)); w.size.x=snap(std::max(20.f,s_resizeSizeStart.x-dx)); w.size.y=snap(std::max(10.f,s_resizeSizeStart.y-dy)); break;
            case 1: w.pos.y=snap(std::max(0.f,s_resizePosStart.y+dy)); w.size.x=snap(std::max(20.f,s_resizeSizeStart.x+dx)); w.size.y=snap(std::max(10.f,s_resizeSizeStart.y-dy)); break;
            case 2: w.pos.x=snap(std::max(0.f,s_resizePosStart.x+dx)); w.size.x=snap(std::max(20.f,s_resizeSizeStart.x-dx)); w.size.y=snap(std::max(10.f,s_resizeSizeStart.y+dy)); break;
            case 3: w.size.x=snap(std::max(20.f,s_resizeSizeStart.x+dx)); w.size.y=snap(std::max(10.f,s_resizeSizeStart.y+dy)); break;
            }
        } else {
            s_isResizing = false;
        }
    }

    // Click empty → deselect
    if (canvasHovered && ImGui::IsMouseClicked(0) && !s_isResizing) {
        bool hit = false;
        for (int zi=(int)zSorted.size()-1; zi>=0; zi--) {
            int i=zSorted[zi];
            if (!s_widgets[i].visible) continue;
            ImVec2 mn={contentOrigin.x+s_widgets[i].pos.x,contentOrigin.y+s_widgets[i].pos.y};
            ImVec2 mx={mn.x+s_widgets[i].size.x,mn.y+s_widgets[i].size.y};
            if (io.MousePos.x>=mn.x&&io.MousePos.x<=mx.x&&io.MousePos.y>=mn.y&&io.MousePos.y<=mx.y)
            { hit=true; break; }
        }
        if (!hit) s_selIdx = -1;
    }

    // ── Status bar ────────────────────────────────────────────────────────────
    {
        ImVec2 sbPos={contentOrigin.x, contentOrigin.y+contentSize.y};
        dl->AddRectFilled(sbPos,{sbPos.x+contentSize.x,sbPos.y+SB_H}, IM_COL32(7,7,11,210));
        char sb[160];
        if (s_selIdx>=0 && s_selIdx<(int)s_widgets.size()) {
            UIWidget& sw=s_widgets[s_selIdx];
            snprintf(sb,sizeof(sb),"selected: %s  |  pos: %.0f, %.0f  |  size: %.0f x %.0f  |  z: %d",
                sw.name, sw.pos.x, sw.pos.y, sw.size.x, sw.size.y, sw.zOrder);
        } else {
            snprintf(sb,sizeof(sb),"Click widget to select  |  Drag to move  |  Corner handles to resize  |  Add via Hierarchy [+]");
        }
        dl->AddText({sbPos.x+8.f,sbPos.y+3.f}, IM_COL32(110,108,100,200), sb);
    }

    ImGui::End();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Scene Tabs
// ═════════════════════════════════════════════════════════════════════════════

// ── Snapshot helpers ──────────────────────────────────────────────────────────
SceneSnapshot SceneEditor::sceneTabs_snapshot() {
    SceneSnapshot snap;
    const_cast<World&>(m_world).each<TagComp>([&](EntityID id, TagComp& tag) {
        if (tag.value == "Floor" || tag.value == "DirectionalLight") return;

        SceneSnapshot::EntitySnap es;
        es.name = tag.value;

        if (auto* t = m_world.get<TransformComponent>(id)) {
            es.pos   = t->position;
            es.rot   = t->rotation;
            es.scale = t->scale;
        }
        if (auto* mr = m_world.get<MeshRendererComponent>(id)) {
            es.hasMesh  = true;
            es.color    = { mr->material.albedo.r, mr->material.albedo.g, mr->material.albedo.b };
            if (mr->model && !mr->model->path().empty()) {
                es.modelPath = mr->model->path();
                // detect primitive type from path tag set during spawn
                const std::string& p = mr->model->path();
                es.isSphere = (p.find("__sphere") != std::string::npos ||
                               p.find("sphere")   != std::string::npos);
            } else {
                es.isSphere = false; // default cube
            }
        }
        snap.entities.push_back(es);
    });
    snap.uiWidgets = m_uiWidgets;   // save UI canvas widgets
    return snap;
}

void SceneEditor::sceneTabs_restore(const SceneSnapshot& snap) {
    // Destroy all user entities (keep Floor / DirectionalLight)
    std::vector<EntityID> toDelete;
    m_world.each<TagComp>([&](EntityID id, TagComp& t) {
        if (t.value != "Floor" && t.value != "DirectionalLight")
            toDelete.push_back(id);
    });
    for (EntityID id : toDelete) {
        m_app.physics().removeBody(id);
        m_world.destroy(id);
    }
    m_selected = NULL_ENTITY;

    // Restore UI widgets
    m_uiWidgets = snap.uiWidgets;
    m_uiSelIdx  = -1;
    m_uiDragIdx = -1;

    // Recreate
    for (const auto& es : snap.entities) {
        auto obj = m_app.spawn(es.name);
        EntityID id = obj.id();
        obj.pos(es.pos.x, es.pos.y, es.pos.z);
        if (auto* t = m_world.get<TransformComponent>(id)) {
            t->rotation = es.rot;
            t->scale    = es.scale;
        }
        if (es.hasMesh) {
            if (!es.modelPath.empty()) obj.model(es.modelPath);
            else if (es.isSphere)      obj.sphere();
            else                       obj.cube();
            obj.color(es.color.r, es.color.g, es.color.b);
        }
    }
}

// ── Init / new tab ────────────────────────────────────────────────────────────
void SceneEditor::sceneTabs_init() {
    SceneTab tab;
    tab.name = "Scene 1";
    tab.snap = sceneTabs_snapshot(); // empty world
    m_tabs.push_back(tab);
    m_activeTab = 0;
}

void SceneEditor::sceneTabs_saveActive() {
    if (m_activeTab < 0 || m_activeTab >= (int)m_tabs.size()) return;
    m_tabs[m_activeTab].snap = sceneTabs_snapshot();
}

void SceneEditor::sceneTabs_loadActive() {
    if (m_activeTab < 0 || m_activeTab >= (int)m_tabs.size()) return;
    sceneTabs_restore(m_tabs[m_activeTab].snap);
    m_undoStack.clear();
    m_undoIdx = -1;
}

void SceneEditor::sceneTabs_newTab() {
    // Called directly to create a 3D scene (e.g. from file menu)
    // The + button uses m_newTabPopup instead.
    sceneTabs_addTab(SceneType::Scene3D);
}

void SceneEditor::sceneTabs_addTab(SceneType type) {
    sceneTabs_saveActive();
    SceneTab tab;
    tab.type  = type;
    tab.name  = (type == SceneType::SceneUI ? "UI Scene " : "Scene ")
                + std::to_string((int)m_tabs.size() + 1);
    tab.snap  = {};
    m_tabs.push_back(tab);
    m_activeTab = (int)m_tabs.size() - 1;
    m_pendingTab = m_activeTab;
    sceneTabs_restore(tab.snap);
    m_undoStack.clear();
    m_undoIdx = -1;
    LOG_INFO("SceneTabs: new " << (type == SceneType::SceneUI ? "UI" : "3D")
             << " tab '" << tab.name << "'");
}

void SceneEditor::sceneTabs_closeTab(int i) {
    if (m_tabs.size() <= 1) {
        // Can't close last tab — just clear it
        sceneTabs_restore({});
        m_tabs[0].snap  = {};
        m_tabs[0].dirty = false;
        m_tabs[0].name  = "Scene 1";
        return;
    }
    m_tabs.erase(m_tabs.begin() + i);
    m_activeTab = std::max(0, std::min(m_activeTab, (int)m_tabs.size() - 1));
    sceneTabs_loadActive();
}

// ── Draw tab bar ──────────────────────────────────────────────────────────────
void SceneEditor::drawSceneTabs() {
    ImGuiViewport* vp  = ImGui::GetMainViewport();
    const float MENUBAR_H  = ImGui::GetFrameHeight();
    const float TOOLBAR_H  = 42.f;
    const float TAB_BAR_H  = 28.f;
    const float TOP        = MENUBAR_H + TOOLBAR_H;

    ImGui::SetNextWindowPos ({vp->Pos.x, vp->Pos.y + TOP});
    ImGui::SetNextWindowSize({vp->Size.x, TAB_BAR_H});
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  {4.f, 2.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding,    4.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.10f,0.10f,0.13f,1.f});

    ImGui::Begin("##sceneTabs", nullptr,
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    if (ImGui::BeginTabBar("##sceneTabBar",
            ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {

        int closeRequest = -1;

        for (int i = 0; i < (int)m_tabs.size(); i++) {
            auto& tab = m_tabs[i];
            // Type icon prefix: [3D] or [UI]
            const char* typeTag = (tab.type == SceneType::SceneUI) ? "[UI] " : "[3D] ";
            std::string label = typeTag + tab.name + (tab.dirty ? " *" : "") + "##tab" + std::to_string(i);

            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            // SetSelected only on the frame we explicitly switch — not every frame
            if (i == m_pendingTab) flags |= ImGuiTabItemFlags_SetSelected;

            bool open = true;
            bool tabVisible = ImGui::BeginTabItem(label.c_str(), &open, flags);

            if (!open) closeRequest = i;

            if (tabVisible) {
                if (i != m_activeTab) {
                    // Save old, switch, load new
                    sceneTabs_saveActive();
                    m_activeTab = i;
                    sceneTabs_loadActive();
                }
                m_pendingTab = -1; // clear one-shot flag
                ImGui::EndTabItem();
            }
        }

        // "+" button — opens scene type chooser popup
        ImGui::PushStyleColor(ImGuiCol_Tab,       {0.08f,0.35f,0.08f,1.f});
        ImGui::PushStyleColor(ImGuiCol_TabHovered, {0.12f,0.50f,0.12f,1.f});
        ImGui::PushStyleColor(ImGuiCol_TabActive,  {0.10f,0.42f,0.10f,1.f});
        if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing))
            m_newTabPopup = true;
        ImGui::PopStyleColor(3);
        UI::Tooltip("Add new scene tab");

        ImGui::EndTabBar();

        // ── New scene type chooser popup ──────────────────────────────────────
        if (m_newTabPopup) {
            ImGui::OpenPopup("##newScenePopup");
            m_newTabPopup = false;
        }
        ImGuiViewport* vp2 = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(
            {vp2->Size.x * 0.5f, vp2->Size.y * 0.5f},
            ImGuiCond_Always, {0.5f, 0.5f});
        ImGui::SetNextWindowSize({300, 0}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.97f);
        if (ImGui::BeginPopup("##newScenePopup",
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f,0.85f,0.f,1.f));
            ImGui::TextUnformatted("  Choose scene type");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Spacing();

            float w = ImGui::GetContentRegionAvail().x;

            // 3D Scene button
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f,0.30f,0.55f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.20f,0.40f,0.70f,1.f});
            if (ImGui::Button("[3D]  3D Scene", {w, 40.f})) {
                sceneTabs_addTab(SceneType::Scene3D);
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(2);
            ImGui::TextDisabled("  Cubes, spheres, lights, physics,\n  UI widgets with world transform.");

            ImGui::Spacing();

            // UI Scene button
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.40f,0.18f,0.55f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.55f,0.25f,0.70f,1.f});
            if (ImGui::Button("[UI]  UI Scene", {w, 40.f})) {
                sceneTabs_addTab(SceneType::SceneUI);
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(2);
            ImGui::TextDisabled("  Buttons, labels, sliders, images\n  in screen-space canvas. No 3D.");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Cancel", {w, 26.f})) ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        if (closeRequest >= 0)
            sceneTabs_closeTab(closeRequest);
    }

    ImGui::End();
}


} // namespace eng
