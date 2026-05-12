// ProjectManager.cpp
#include "editor/ProjectManager.h"
#include "editor/configure.h"
#include "editor/CodeGenerator.h"
#include "core/Log.h"
#include "ui/UI.h"
#include <imgui.h>
#include <json.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using json   = nlohmann::json;

namespace eng {

ProjectManager::ProjectManager(App& app)
    : m_app(app), m_world(app.world())
{}

// ─────────────────────────────────────────────────────────────────────────────
//  Serialize scene to JSON
// ─────────────────────────────────────────────────────────────────────────────
std::string ProjectManager::serializeScene() const {
    json root;
    root["version"] = 1;

    json entities = json::array();
    const_cast<World&>(m_world).each<TagComp>(
        [&](EntityID id, TagComp& tag) {
            json ent;
            ent["id"]   = id;
            ent["name"] = tag.value;

            // Transform
            if (auto* t = m_world.get<TransformComponent>(id)) {
                ent["pos"]   = { t->position.x, t->position.y, t->position.z };
                ent["rot"]   = { t->rotation.x, t->rotation.y, t->rotation.z };
                ent["scale"] = { t->scale.x,    t->scale.y,    t->scale.z    };
            }

            // Mesh renderer
            if (auto* mr = m_world.get<MeshRendererComponent>(id)) {
                ent["visible"] = mr->visible;
                ent["color"]   = { mr->material.albedo.r,
                                    mr->material.albedo.g,
                                    mr->material.albedo.b };
                ent["metallic"]  = mr->material.metallic;
                ent["roughness"] = mr->material.roughness;
                if (mr->model && !mr->model->path().empty())
                    ent["model"] = mr->model->path();
                else
                    ent["shape"] = "cube"; // default
            }

            // RigidBody
            if (auto* rb = m_world.get<RigidBodyComponent>(id)) {
                ent["rb_type"] = (int)rb->type;
                ent["rb_shape"]= (int)rb->shape;
                ent["rb_mass"] = rb->mass;
                ent["rb_restitution"] = rb->restitution;
                ent["rb_friction"]    = rb->friction;
                ent["rb_sensor"]      = rb->isSensor;
            }

            entities.push_back(ent);
        });

    root["entities"] = entities;
    return root.dump(2);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Parse scene from JSON
// ─────────────────────────────────────────────────────────────────────────────
bool ProjectManager::parseScene(const std::string& src) {
    try {
        json root = json::parse(src);

        // Clear existing scene (keep Floor/DirLight)
        std::vector<EntityID> toDelete;
        m_world.each<TagComp>([&](EntityID id, TagComp& t) {
            if (t.value != "Floor" && t.value != "DirectionalLight")
                toDelete.push_back(id);
        });
        for (EntityID id : toDelete) {
            m_app.physics().removeBody(id);
            m_world.destroy(id);
        }

        // Recreate entities
        for (auto& ent : root["entities"]) {
            std::string name = ent.value("name", "Object");
            if (name == "Floor" || name == "DirectionalLight") continue;

            auto obj = m_app.spawn(name);
            EntityID id = obj.id();

            // Transform
            if (ent.contains("pos")) {
                auto& p = ent["pos"];
                auto& r = ent["rot"];
                auto& s = ent["scale"];
                obj.pos((float)p[0], (float)p[1], (float)p[2]);
                if (auto* t = m_world.get<TransformComponent>(id)) {
                    t->rotation = { (float)r[0], (float)r[1], (float)r[2] };
                    t->scale    = { (float)s[0], (float)s[1], (float)s[2] };
                }
            }

            // Mesh
            bool hasMesh = false;
            if (ent.contains("model")) {
                obj.model(ent["model"].get<std::string>());
                hasMesh = true;
            } else if (ent.value("shape", "cube") == "sphere") {
                obj.sphere();
                hasMesh = true;
            } else {
                obj.cube();
                hasMesh = true;
            }

            // Color
            if (hasMesh && ent.contains("color")) {
                auto& c = ent["color"];
                obj.color((float)c[0], (float)c[1], (float)c[2]);
            }

            // Material
            if (auto* mr = m_world.get<MeshRendererComponent>(id)) {
                mr->material.metallic  = ent.value("metallic",  0.f);
                mr->material.roughness = ent.value("roughness", 0.5f);
            }

            // RigidBody
            if (ent.contains("rb_type")) {
                m_app.addRigidBody(id,
                    (RigidBodyType)ent["rb_type"].get<int>(),
                    (ColliderShape)ent["rb_shape"].get<int>());
                if (auto* rb = m_world.get<RigidBodyComponent>(id)) {
                    rb->mass        = ent.value("rb_mass",        1.f);
                    rb->restitution = ent.value("rb_restitution", 0.2f);
                    rb->friction    = ent.value("rb_friction",    0.5f);
                    rb->isSensor    = ent.value("rb_sensor",      false);
                }
            }
        }

        LOG_INFO("ProjectManager: loaded " << root["entities"].size() << " entities");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("ProjectManager: JSON parse error: " << e.what());
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Save / Load
// ─────────────────────────────────────────────────────────────────────────────
bool ProjectManager::save(const std::string& path) {
    std::ofstream f(path);
    if (!f) { LOG_ERROR("ProjectManager: cannot write " << path); return false; }
    f << serializeScene();
    m_path  = path;
    m_dirty = false;
    LOG_INFO("ProjectManager: saved to " << path);
    return true;
}

bool ProjectManager::load(const std::string& path) {
    if (!fs::exists(path)) return false;
    std::ifstream f(path);
    if (!f) return false;
    std::string src((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    if (!parseScene(src)) return false;
    m_path  = path;
    m_dirty = false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Menu Bar
// ─────────────────────────────────────────────────────────────────────────────
void ProjectManager::drawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    // ── File ──────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
            if (m_onNewScene) {
                // Delegate to SceneEditor tab system — creates a new tab
                m_onNewScene();
            } else {
                // Fallback: clear world directly
                std::vector<EntityID> del;
                m_world.each<TagComp>([&](EntityID id, TagComp& t){
                    if (t.value != "Floor" && t.value != "DirectionalLight")
                        del.push_back(id);
                });
                for (EntityID id : del) {
                    m_app.physics().removeBody(id);
                    m_world.destroy(id);
                }
                m_path.clear(); m_dirty = false;
                LOG_INFO("ProjectManager: new scene");
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            if (m_path.empty()) m_showSaveAs = true;
            else save(m_path);
        }
        if (ImGui::MenuItem("Save As...")) {
            m_showSaveAs = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Open...", "Ctrl+O")) {
            // Simple: ask for path via save-as dialog reused for open
            m_showSaveAs = true;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            glfwSetWindowShouldClose(m_app.window().handle(), 1);
        }
        ImGui::EndMenu();
    }

    // ── Edit ──────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
            // Delegated to SceneEditor via hotkeys
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
        ImGui::Separator();
        if (ImGui::MenuItem("Select All", "Ctrl+A")) {}
        if (ImGui::MenuItem("Delete Selected", "Del")) {}
        ImGui::EndMenu();
    }

    // ── View ──────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Toggle Wireframe", "F")) {}
        if (ImGui::MenuItem("Toggle Free Cam", "Tab")) {}
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Camera")) {
            m_app.camPos(0, 4, 10).camRot(-90.f, -20.f);
        }
        ImGui::Separator();

        // ── Theme submenu ─────────────────────────────────────────────────────
        /*
        if (ImGui::BeginMenu("Theme")) {
            struct ThemeEntry { const char* name; std::function<UI::Theme()> fn; };
            static const ThemeEntry themes[] = {
                { "Cyberpunk  (default)", UI::Themes::Cyberpunk },
                { "Dark",                UI::Themes::Dark       },
                { "Light",               UI::Themes::Light      },
                { "Blood",               UI::Themes::Blood      },
                { "Matrix",              UI::Themes::Matrix     },
            };
            const UI::Theme& cur = UI::GetTheme();
            for (auto& t : themes) {
                UI::Theme candidate = t.fn();
                // highlight active theme
                bool active = (candidate.accent.x == cur.accent.x &&
                               candidate.accent.y == cur.accent.y);
                if (active) ImGui::PushStyleColor(ImGuiCol_Text, UI::ACCENT());
                if (ImGui::MenuItem(t.name, nullptr, active))
                    UI::ApplyTheme(t.fn());
                if (active) ImGui::PopStyleColor();
            }
            ImGui::EndMenu();
        }
        */
        ImGui::EndMenu();
    }
        

    // ── Help ──────────────────────────────────────────────────────────────────
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About Jenuroix Engine"))
            m_showAbout = true;
        if (ImGui::MenuItem("Controls")) {
            LOG_INFO(
                "Controls:\n"
                "  Tab        - Toggle mouse capture\n"
                "  WASD+QE    - Camera movement\n"
                "  LMB click  - Select object\n"
                "  W/E/R      - Gizmo mode (Move/Rotate/Scale)\n"
                "  F          - Focus camera on selected\n"
                "  Del        - Delete selected\n"
                "  Ctrl+D     - Duplicate selected\n"
                "  Ctrl+Z/Y   - Undo/Redo\n"
                "  F1         - Toggle editor panels");
        }
        ImGui::EndMenu();
    }

    // ── Right side: project name + dirty indicator ────────────────────────────
    float rightW = ImGui::GetContentRegionAvail().x;
    std::string title = m_path.empty() ? "Untitled Scene" : fs::path(m_path).stem().string();
    if (m_dirty) title += "  *";
    float tw = ImGui::CalcTextSize(title.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + rightW - tw - 12);
    ImGui::TextDisabled("%s", title.c_str());

    ImGui::EndMainMenuBar();

    // ── Save As dialog ────────────────────────────────────────────────────────
    if (m_showSaveAs) {
        ImGui::SetNextWindowSize({420, 120}, ImGuiCond_Always);
        ImGui::SetNextWindowPos(
            { (float)m_app.window().width() * 0.5f,
              (float)m_app.window().height() * 0.5f },
            ImGuiCond_Always, {0.5f, 0.5f});

        if (ImGui::Begin("Save / Open Project##dialog",
                            &m_showSaveAs,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("File path:");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##savepath", m_saveAsBuf, sizeof(m_saveAsBuf));

            ImGui::Spacing();
            if (ImGui::Button("Save", {90, 0})) {
                save(m_saveAsBuf);
                m_showSaveAs = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Load", {90, 0})) {
                load(m_saveAsBuf);
                m_showSaveAs = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", {90, 0}))
                m_showSaveAs = false;
        }
        ImGui::End();
    }

    // ── About dialog ──────────────────────────────────────────────────────────
    if (m_showAbout) {
        ImGui::SetNextWindowSize({340, 180}, ImGuiCond_Always);
        ImGui::SetNextWindowPos(
            { (float)m_app.window().width()  * 0.5f,
              (float)m_app.window().height() * 0.5f },
            ImGuiCond_Always, {0.5f, 0.5f});

        std::string aboutTitle = std::string("About ") + std::string(EngineConfig::name) + "##about";
        if (ImGui::Begin(aboutTitle.c_str(), &m_showAbout,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Spacing();
            UI::LabelColored(UI::YELLOW, "%s  v%s",
                std::string(EngineConfig::name).c_str(),
                std::string(EngineConfig::version).c_str());
            ImGui::Spacing();
            ImGui::TextWrapped(
                "Author:    %s\n"
                "Version:   %s\n"
                "  Physics:   Jolt Physics\n"
                "  Audio:     miniaudio\n"
                "  Renderer:  OpenGL 3.3\n"
                "  UI:        Dear ImGui",
                std::string(EngineConfig::author).c_str(),
                std::string(EngineConfig::version).c_str());
            ImGui::Spacing();
            if (ImGui::Button("Close", {80, 0})) m_showAbout = false;
        }
        ImGui::End();
    }
}

} // namespace eng
