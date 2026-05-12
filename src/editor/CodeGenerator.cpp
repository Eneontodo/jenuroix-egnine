// CodeGenerator.cpp
#include "editor/CodeGenerator.h"
#include "core/Log.h"
#include "editor/configure.h"
#include <imgui.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#  include <windows.h>
#  include <shellapi.h>
#  define POPEN  _popen
#  define PCLOSE _pclose
#else
#  include <unistd.h>
#  define POPEN  popen
#  define PCLOSE pclose
#endif

namespace fs = std::filesystem;

namespace eng {

// ─────────────────────────────────────────────────────────────────────────────
CodeGenerator::CodeGenerator(App& app)
    : m_app(app), m_world(app.world())
{
    m_codeBuf = new char[m_codeBufSz];
    m_codeBuf[0] = '\0';
    m_renderBuf = new char[m_renderBufSz];
    // default UI block
    const char* defaultRender =
        "        if (UI::BeginFixed(\"##hud\", 8, 8, 0, 0,\n"
        "            ImGuiWindowFlags_NoTitleBar |\n"
        "            ImGuiWindowFlags_NoBackground |\n"
        "            ImGuiWindowFlags_AlwaysAutoResize))\n"
        "        {\n"
        "            UI::LabelColored(UI::WHITE, \"FPS: %.0f\", Time::fps());\n"
        "        }\n"
        "        UI::End();\n";
    strncpy(m_renderBuf, defaultRender, m_renderBufSz - 1);
    m_renderBuf[m_renderBufSz - 1] = '\0';
}

CodeGenerator::~CodeGenerator() {
    if (m_buildThread.joinable())
        m_buildThread.join();
    delete[] m_codeBuf;
    delete[] m_renderBuf;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Code generation
// ─────────────────────────────────────────────────────────────────────────────
std::string CodeGenerator::generate() const {
    // If we have tabs info, generate multi-scene code
    if (m_tabs && m_tabs->size() > 1) {
        return generateMultiScene();
    }
    // Single scene (legacy path)
    std::ostringstream ss;
    ss << genHeader() << genAppSetup() << genObjects() << genRenderBlock() << genFooter();
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Multi-scene code generation
// ─────────────────────────────────────────────────────────────────────────────
static std::string sanitizeIdent(const std::string& s) {
    std::string out;
    for (char c : s) out += (std::isalnum((unsigned char)c) ? c : '_');
    if (out.empty() || std::isdigit((unsigned char)out[0])) out = "scene_" + out;
    return out;
}

static std::string genUIWidgetsForTab(const std::vector<UIWidget>& widgets) {
    if (widgets.empty()) return "";
    std::ostringstream ss;
    ss << "        // UI Canvas widgets\n";
    ss << "        ImGui::SetNextWindowPos({0,0});\n";
    ss << "        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);\n";
    ss << "        ImGui::Begin(\"##canvas\", nullptr,\n";
    ss << "            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |\n";
    ss << "            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |\n";
    ss << "            ImGuiWindowFlags_NoBringToFrontOnFocus);\n";
    char line[512];
    for (const auto& w : widgets) {
        std::string n(w.name);
        std::string sid = sanitizeIdent(n);
        ss << "        ImGui::SetCursorPos({" << w.pos.x << "f, " << w.pos.y << "f});\n";
        switch (w.type) {
        case UIWidget::Type::Button:
            snprintf(line,sizeof(line),"        if (ImGui::Button(\"%s\", {%.0ff,%.0ff})) { /* TODO */ }\n",w.text,w.size.x,w.size.y);
            ss<<line; break;
        case UIWidget::Type::Label:
            snprintf(line,sizeof(line),"        ImGui::Text(\"%s\");\n",w.text);
            ss<<line; break;
        case UIWidget::Type::ColoredText:
            snprintf(line,sizeof(line),"        ImGui::TextColored(ImVec4(%.3ff,%.3ff,%.3ff,1.f), \"%s\");\n",w.color.x,w.color.y,w.color.z,w.text);
            ss<<line; break;
        case UIWidget::Type::Slider:
            snprintf(line,sizeof(line),"        static float %s_val = %.2ff;\n        ImGui::SetNextItemWidth(%.0ff);\n        ImGui::SliderFloat(\"##%s\", &%s_val, 0.f, 1.f);\n",sid.c_str(),w.sliderVal,w.size.x,sid.c_str(),sid.c_str());
            ss<<line; break;
        case UIWidget::Type::TextField:
            snprintf(line,sizeof(line),"        static char %s_buf[256] = {};\n        ImGui::SetNextItemWidth(%.0ff);\n        ImGui::InputText(\"##%s\", %s_buf, sizeof(%s_buf));\n",sid.c_str(),w.size.x,sid.c_str(),sid.c_str(),sid.c_str());
            ss<<line; break;
        case UIWidget::Type::Toggle:
            snprintf(line,sizeof(line),"        static bool %s_val = %s;\n        ImGui::Checkbox(\"%s\", &%s_val);\n",sid.c_str(),w.toggled?"true":"false",w.text,sid.c_str());
            ss<<line; break;
        case UIWidget::Type::ProgressBar:
            snprintf(line,sizeof(line),"        static float %s_val = %.2ff;\n        ImGui::ProgressBar(%s_val, ImVec2(%.0ff, %.0ff));\n",sid.c_str(),w.progress,sid.c_str(),w.size.x,w.size.y);
            ss<<line; break;
        case UIWidget::Type::Panel:
            snprintf(line,sizeof(line),"        ImGui::BeginChild(\"%s\", ImVec2(%.0ff,%.0ff), true);\n        // TODO: panel content\n        ImGui::EndChild();\n",sid.c_str(),w.size.x,w.size.y);
            ss<<line; break;
        default:
            snprintf(line,sizeof(line),"        // Widget: %s\n",w.name);
            ss<<line; break;
        }
    }
    ss << "        ImGui::End();\n";
    return ss.str();
}

std::string CodeGenerator::generateMultiScene() const {
    std::ostringstream ss;
    ss << "// Generated by Jenuroix Editor — Multi-Scene\n"
       << "// Edit freely — use Regenerate to sync from scene.\n\n"
       << "#include \"engine.h\"\n"
       << "#include <cmath>\n"
       << "using namespace eng;\n\n"
       << "int main() {\n"
       << "    App app(\"" << std::string(EngineConfig::name) << "\", 1280, 720);\n"
       << "    app.skyColor(0.13f, 0.14f, 0.18f)\n"
       << "       .lightDir(0.4f, 1.f, 0.5f)\n"
       << "       .lightColor(1.f, 0.97f, 0.9f)\n"
       << "       .ambient(0.18f, 0.18f, 0.22f);\n"
       << "    app.camPos(0, 4, 10).camRot(-90.f, -20.f).freeCam(true);\n\n"
       << "    SceneManager scenes(app);\n\n";

    // Save the currently active tab first (it has the live world state)
    // For each tab, generate its setup lambda
    for (int ti = 0; ti < (int)m_tabs->size(); ti++) {
        const SceneTab& tab = (*m_tabs)[ti];
        // Use the saved snapshot for all tabs; for the active one use activeSnap
        const SceneSnapshot* snap = (ti == m_activeIndex && m_activeSnap)
                                  ? m_activeSnap : &tab.snap;

        std::string fnName = sanitizeIdent(tab.name);
        bool isUI = (tab.type == SceneType::SceneUI);

        ss << "    // ── Scene: " << tab.name << " (" << (isUI?"UI":"3D") << ") ──────────────\n";
        ss << "    scenes.add(\"" << tab.name << "\", [](App& app) {\n";

        if (!isUI) {
            // 3D: spawn entities from snapshot
            if (!snap->entities.empty()) {
                ss << "        // Scene objects\n";
                for (const auto& e : snap->entities) {
                    if (e.name == "Floor" || e.name == "DirectionalLight") continue;
                    std::string vn = sanitizeIdent(e.name) + "_obj";
                    ss << "        auto " << vn << " = app.spawn(\"" << e.name << "\")";
                    if (e.hasMesh) {
                        if (!e.modelPath.empty())
                            ss << "\n            .model(\"" << e.modelPath << "\")";
                        else if (e.isSphere)
                            ss << "\n            .sphere()";
                        else
                            ss << "\n            .cube()";
                        ss << "\n            .color(" << flt(e.color.r) << "f," << flt(e.color.g) << "f," << flt(e.color.b) << "f)";
                    }
                    if (e.pos.x!=0||e.pos.y!=0||e.pos.z!=0)
                        ss << "\n            .pos(" << flt(e.pos.x) << "f," << flt(e.pos.y) << "f," << flt(e.pos.z) << "f)";
                    ss << ";\n";
                }
                ss << "\n";
            }
            ss << "        app.onUpdate([&](float dt) {\n"
               << "            // TODO: game logic for " << tab.name << "\n"
               << "        });\n"
               << "        app.onRender([&]() {\n"
               << "            // TODO: HUD / UI for " << tab.name << "\n"
               << "        });\n";
        } else {
            // UI Scene: emit ImGui calls at absolute positions
            ss << "        app.onRender([&]() {\n";
            ss << genUIWidgetsForTab(snap->uiWidgets);
            ss << "        });\n";
        }

        ss << "    });\n\n";
    }

    // Determine first scene name
    std::string firstName = m_tabs->empty() ? "Scene" : (*m_tabs)[0].name;
    ss << "    scenes.loadFirst(); // starts with \"" << firstName << "\"\n\n";
    ss << "    // ── Example: switch scenes with keyboard ─────────────────────\n";
    ss << "    // app.onUpdate([&](float dt) {\n";
    ss << "    //     if (app.keyDown(KEY_N)) scenes.loadNext();\n";
    ss << "    //     if (app.keyDown(KEY_P)) scenes.loadPrev();\n";
    ss << "    //     if (app.keyDown(KEY_1)) scenes.load(\"" << firstName << "\");\n";
    if (m_tabs->size() > 1)
        ss << "    //     if (app.keyDown(KEY_2)) scenes.load(\"" << (*m_tabs)[1].name << "\");\n";
    ss << "    // });\n\n";
    ss << "    return app.run();\n}\n";
    return ss.str();
}

std::string CodeGenerator::genHeader() const {
    return
        "// Generated by Jenuroix Editor\n"
        "// Edit freely — use Regenerate to sync from scene, or edit and watch live preview.\n"
        "\n"
        "#include \"engine.h\"\n"
        "#include <cmath>\n"
        "using namespace eng;\n"
        "\n"
        "int main() {\n";
}

std::string CodeGenerator::genAppSetup() const {
    std::ostringstream ss;
    ss << "    App app(\"" << std::string(EngineConfig::name) << "\", 1280, 720);\n";
    ss << "    app.skyColor(0.13f, 0.14f, 0.18f)\n"
       << "       .lightDir(0.4f, 1.f, 0.5f)\n"
       << "       .lightColor(1.f, 0.97f, 0.9f)\n"
       << "       .ambient(0.18f, 0.18f, 0.22f);\n";
    ss << "    app.camPos(0, 4, 10).camRot(-90.f, -20.f).freeCam(true);\n";
    ss << "\n";
    return ss.str();
}

std::string CodeGenerator::genObjects() const {
    std::ostringstream ss;

    std::vector<EntityID> entities;
    const_cast<World&>(m_world).each<TagComp>(
        [&](EntityID id, TagComp&) { entities.push_back(id); });
    std::sort(entities.begin(), entities.end());

    if (!entities.empty()) {
        ss << "    // ── Scene objects ──────────────────────────────\n";
        for (EntityID id : entities)
            ss << genObject(id);
        ss << "\n";
    }

    ss << "    app.onUpdate([&](float dt) {\n"
       << "        // Game logic here\n"
       << "    });\n\n";

    return ss.str();
}

std::string CodeGenerator::genRenderBlock() const {
    std::ostringstream ss;
    ss << "    app.onRender([&]() {\n";
    ss << m_renderBuf;
    // ensure newline before closing brace
    std::string body(m_renderBuf);
    if (!body.empty() && body.back() != '\n') ss << "\n";

    // ── Emit UI Scene widgets ─────────────────────────────────────────────
    if (m_uiWidgets && !m_uiWidgets->empty()) {
        ss << "\n        // --- UI Scene widgets ---\n";
        ss << "        if (ImGui::Begin(\"UI Scene\")) {\n";
        char line[512];
        for (const auto& w : *m_uiWidgets) {
            std::string n(w.text);
            switch (w.type) {
            case UIWidget::Type::Button:
                snprintf(line, sizeof(line),
                    "            if (ImGui::Button(\"%s\")) { /* TODO */ }\n", n.c_str());
                ss << line; break;
            case UIWidget::Type::Label:
                snprintf(line, sizeof(line),
                    "            ImGui::Text(\"%s\");\n", n.c_str());
                ss << line; break;
            case UIWidget::Type::Slider:
                snprintf(line, sizeof(line),
                    "            static float %s_val = %.2ff;\n"
                    "            ImGui::SliderFloat(\"%s\", &%s_val, 0.f, 1.f);\n",
                    n.c_str(), w.sliderVal, n.c_str(), n.c_str());
                ss << line; break;
            case UIWidget::Type::TextField:
                snprintf(line, sizeof(line),
                    "            static char %s_buf[256] = {};\n"
                    "            ImGui::InputText(\"%s\", %s_buf, sizeof(%s_buf));\n",
                    n.c_str(), n.c_str(), n.c_str(), n.c_str());
                ss << line; break;
            case UIWidget::Type::ColoredText:
                snprintf(line, sizeof(line),
                    "            ImGui::TextColored(ImVec4(%.3ff,%.3ff,%.3ff,1.f), \"%s\");\n",
                    w.color.x, w.color.y, w.color.z, n.c_str());
                ss << line; break;
            case UIWidget::Type::Panel: {
                std::string sid(w.name); for (char& c : sid) if (!isalnum(c)) c = '_';
                snprintf(line, sizeof(line),
                    "            ImGui::BeginChild(\"%s\", ImVec2(%.0f, %.0f), true);\n"
                    "            // TODO: add content here\n"
                    "            ImGui::EndChild();\n",
                    sid.c_str(), w.size.x, w.size.y);
                ss << line; break;
            }
            case UIWidget::Type::Toggle: {
                std::string sid(w.name); for (char& c : sid) if (!isalnum(c)) c = '_';
                snprintf(line, sizeof(line),
                    "            static bool %s_val = %s;\n"
                    "            ImGui::Checkbox(\"%s\", &%s_val);\n",
                    sid.c_str(), w.toggled ? "true" : "false", n.c_str(), sid.c_str());
                ss << line; break;
            }
            case UIWidget::Type::ProgressBar: {
                std::string sid(w.name); for (char& c : sid) if (!isalnum(c)) c = '_';
                snprintf(line, sizeof(line),
                    "            static float %s_val = %.2ff;\n"
                    "            ImGui::ProgressBar(%s_val, ImVec2(-1, 0));\n",
                    sid.c_str(), w.progress, sid.c_str());
                ss << line; break;
            }
            case UIWidget::Type::Dropdown: {
                std::string sid(w.name); for (char& c : sid) if (!isalnum(c)) c = '_';
                snprintf(line, sizeof(line),
                    "            static int %s_idx = 0;\n"
                    "            const char* %s_items[] = {\"Item 1\", \"Item 2\", \"Item 3\"};\n"
                    "            ImGui::Combo(\"%s\", &%s_idx, %s_items, 3);\n",
                    sid.c_str(), sid.c_str(), n.c_str(), sid.c_str(), sid.c_str());
                ss << line; break;
            }
            case UIWidget::Type::Image:
                snprintf(line, sizeof(line),
                    "            // Image placeholder: %s (load texture and use ImGui::Image)\n",
                    n.c_str());
                ss << line; break;
            default:
                snprintf(line, sizeof(line), "            // Widget: %s\n", n.c_str());
                ss << line; break;
            }
        }
        ss << "        }\n";
        ss << "        ImGui::End();\n";
    }

    ss << "    });\n\n";
    return ss.str();
}

std::string CodeGenerator::genObject(EntityID id) const {
    std::ostringstream ss;
    auto* tag = m_world.get<TagComp>(id);
    std::string name = tag ? tag->value : ("Entity_" + std::to_string(id));
    std::string var  = varName(id);

    if (name == "Floor" && id <= 2) return "";
    if (name == "DirectionalLight")  return "";

    ss << "    auto " << var << " = app.spawn(\"" << name << "\")";

    auto* mr = m_world.get<MeshRendererComponent>(id);
    if (mr) {
        if (mr->model && !mr->model->path().empty()) {
            ss << "\n        .model(\"" << mr->model->path() << "\")";
        } else {
            glm::vec3 sz = (mr->model ? mr->model->boundsMax - mr->model->boundsMin
                                      : glm::vec3(1.f));
            bool isSphere = std::abs(sz.x - sz.y) < 0.1f && std::abs(sz.y - sz.z) < 0.1f;
            ss << (isSphere ? "\n        .sphere()" : "\n        .cube()");
        }
        glm::vec3 c = mr->material.albedo;
        if (!(c.r > 0.99f && c.g > 0.99f && c.b > 0.99f))
            ss << "\n        .color(" << col3(c) << ")";
        if (mr->material.metallic > 0.01f)
            ss << "\n        .metallic(" << flt(mr->material.metallic) << "f)";
        if (std::abs(mr->material.roughness - 0.5f) > 0.05f)
            ss << "\n        .roughness(" << flt(mr->material.roughness) << "f)";
    }

    auto* t = m_world.get<TransformComponent>(id);
    if (t) {
        if (glm::length(t->position) > 0.001f)
            ss << "\n        .pos(" << vec3(t->position) << ")";
        if (glm::length(t->scale - glm::vec3(1.f)) > 0.001f) {
            bool uni = std::abs(t->scale.x - t->scale.y) < 0.001f &&
                       std::abs(t->scale.y - t->scale.z) < 0.001f;
            ss << (uni ? "\n        .scale(" + flt(t->scale.x) + "f)"
                       : "\n        .scale(" + vec3(t->scale) + ")");
        }
        if (glm::length(t->rotation) > 0.01f)
            ss << "\n        .rot(" << vec3(t->rotation) << ")";
    }

    ss << ";\n";

    auto* rb = m_world.get<RigidBodyComponent>(id);
    if (rb) {
        const char* type  = rb->type == RigidBodyType::Static    ? "Static"
                          : rb->type == RigidBodyType::Kinematic ? "Kinematic"
                          :                                        "Dynamic";
        const char* shape = rb->shape == ColliderShape::Sphere  ? "Sphere"
                          : rb->shape == ColliderShape::Capsule ? "Capsule"
                          :                                       "Box";
        ss << "    app.addRigidBody(" << var << ".id()"
           << ", RigidBodyType::" << type
           << ", ColliderShape::"  << shape;
        glm::vec3 he = rb->halfExt;
        if (!(std::abs(he.x-0.5f)<0.01f && std::abs(he.y-0.5f)<0.01f && std::abs(he.z-0.5f)<0.01f))
            ss << ", {" << flt(he.x) << "f, " << flt(he.y) << "f, " << flt(he.z) << "f}";
        if (std::abs(rb->mass - 1.f) > 0.01f)
            ss << ", " << flt(rb->mass) << "f";
        ss << ");\n";
    }

    ss << "\n";
    return ss.str();
}

std::string CodeGenerator::genRigidBody(EntityID) const { return ""; }

std::string CodeGenerator::genFooter() const {
    return "    return app.run();\n}\n";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Low-level parsing helpers
// ─────────────────────────────────────────────────────────────────────────────
void CodeGenerator::skipWS(const std::string& s, size_t& p) {
    while (p < s.size() && (s[p]==' '||s[p]=='\t'||s[p]=='\n'||s[p]=='\r')) ++p;
}

float CodeGenerator::parseFloat(const std::string& s, size_t& p) {
    skipWS(s, p);
    size_t start = p;
    if (p < s.size() && (s[p]=='-'||s[p]=='+')) ++p;
    while (p < s.size() && (std::isdigit((unsigned char)s[p])||s[p]=='.')) ++p;
    if (p < s.size() && (s[p]=='f'||s[p]=='F')) ++p; // trailing 'f'
    if (p == start) return 0.f;
    return std::stof(s.substr(start, p - start));
}

std::string CodeGenerator::parseString(const std::string& s, size_t& p) {
    skipWS(s, p);
    if (p >= s.size() || s[p] != '"') return "";
    ++p; // skip opening "
    size_t start = p;
    while (p < s.size() && s[p] != '"') ++p;
    std::string result = s.substr(start, p - start);
    if (p < s.size()) ++p; // skip closing "
    return result;
}

bool CodeGenerator::skipTo(const std::string& s, size_t& p, char c) {
    while (p < s.size() && s[p] != c) ++p;
    if (p >= s.size()) return false;
    ++p; // skip the char
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  applyCodeToScene  —  live preview
// ─────────────────────────────────────────────────────────────────────────────
//
//  Algorithm:
//    1. Find all app.spawn("Name") calls in code.
//    2. For each spawn, collect the method chain (.cube/.pos/.color/...).
//    3. Remove scene objects not found in code (except Floor/DirLight).
//    4. Create or update objects.
//    5. Handle app.addRigidBody(...) separately.
//
void CodeGenerator::applyCodeToScene() {
    std::string code(m_codeBuf);

    // ── Parsed object structure ────────────────────────────────────────────────
    struct ParsedObj {
        std::string name;
        // mesh
        enum class Mesh { None, Cube, Sphere, Quad, Plane, Cylinder, Model } mesh = Mesh::None;
        std::string modelPath;
        // transform
        bool hasPosVal = false;  glm::vec3 posVal{0,0,0};
        bool hasRotVal = false;  glm::vec3 rotVal{0,0,0};
        bool hasScaleVal = false; glm::vec3 scaleVal{1,1,1};
        bool scaleUniform = false; float scaleUni = 1.f;
        // material
        bool hasColor = false;   glm::vec3 colorVal{1,1,1};
        bool hasMetallic = false; float metallicVal = 0.f;
        bool hasRoughness = false; float roughnessVal = 0.5f;
        // rigidbody (from addRigidBody)
        bool hasRB = false;
        RigidBodyType rbType   = RigidBodyType::Dynamic;
        ColliderShape rbShape  = ColliderShape::Box;
        glm::vec3     rbHalfExt{0.5f,0.5f,0.5f};
        float         rbMass   = 1.f;
        // variable name used in code (for addRigidBody matching)
        std::string varInCode;
    };

    std::vector<ParsedObj> parsed;

    // ── Step 1: collect all spawn blocks ──────────────────────────────────────
    size_t pos = 0;
    while (true) {
        // Find app.spawn( or .spawn(
        size_t spawnPos = code.find(".spawn(", pos);
        if (spawnPos == std::string::npos) break;

        ParsedObj obj;

        // Name: .spawn("Name")
        size_t p = spawnPos + 7; // after .spawn(
        obj.name = parseString(code, p);
        if (obj.name.empty()) { pos = p; continue; }

        // Skip ) and whitespace
        skipTo(code, p, ')');

        // Find variable name: "auto varname = ..." before spawn
        // Search backward from spawnPos to 'auto' or '='
        {
            size_t back = spawnPos;
            while (back > 0 && code[back-1] != '\n') --back; // start of line
            std::string line = code.substr(back, spawnPos - back);
            // find "auto VAR =" or "VAR ="
            size_t eq = line.rfind('=');
            if (eq != std::string::npos) {
                // everything before = (without auto)
                std::string lhs = line.substr(0, eq);
                // strip whitespace and 'auto'
                size_t lp = 0;
                skipWS(lhs, lp);
                if (lhs.substr(lp,4) == "auto") lp += 4;
                skipWS(lhs, lp);
                size_t varEnd = lp;
                while (varEnd < lhs.size() && (std::isalnum((unsigned char)lhs[varEnd])||lhs[varEnd]=='_')) ++varEnd;
                obj.varInCode = lhs.substr(lp, varEnd - lp);
            }
        }

        // ── Step 2: read method chain after spawn ──────────────────────────────
        // Chain ends at ';'
        while (p < code.size() && code[p] != ';') {
            // skip whitespace/newlines/\n/\t
            skipWS(code, p);
            if (p >= code.size() || code[p] == ';') break;

            // expect '.'
            if (code[p] != '.') { ++p; continue; }
            ++p;

            // read method name
            size_t nameStart = p;
            while (p < code.size() && (std::isalnum((unsigned char)code[p])||code[p]=='_')) ++p;
            std::string method = code.substr(nameStart, p - nameStart);

            // read arguments ( ... )
            skipWS(code, p);
            if (p >= code.size() || code[p] != '(') continue;
            ++p; // skip (

            if (method == "cube") {
                obj.mesh = ParsedObj::Mesh::Cube;
                skipTo(code, p, ')');
            } else if (method == "sphere") {
                obj.mesh = ParsedObj::Mesh::Sphere;
                skipTo(code, p, ')');
            } else if (method == "quad") {
                obj.mesh = ParsedObj::Mesh::Quad;
                skipTo(code, p, ')');
            } else if (method == "plane") {
                obj.mesh = ParsedObj::Mesh::Plane;
                skipTo(code, p, ')');
            } else if (method == "cylinder") {
                obj.mesh = ParsedObj::Mesh::Cylinder;
                skipTo(code, p, ')');
            } else if (method == "model") {
                obj.mesh = ParsedObj::Mesh::Model;
                obj.modelPath = parseString(code, p);
                skipTo(code, p, ')');
            } else if (method == "pos") {
                obj.hasPosVal = true;
                obj.posVal.x = parseFloat(code, p); skipTo(code, p, ',');
                obj.posVal.y = parseFloat(code, p);
                // optional z
                size_t pp = p;
                skipWS(code, pp);
                if (pp < code.size() && code[pp] == ',') {
                    ++pp; obj.posVal.z = parseFloat(code, pp); p = pp;
                }
                skipTo(code, p, ')');
            } else if (method == "rot") {
                obj.hasRotVal = true;
                obj.rotVal.x = parseFloat(code, p); skipTo(code, p, ',');
                obj.rotVal.y = parseFloat(code, p);
                size_t pp = p; skipWS(code, pp);
                if (pp < code.size() && code[pp] == ',') { ++pp; obj.rotVal.z = parseFloat(code, pp); p = pp; }
                skipTo(code, p, ')');
            } else if (method == "scale") {
                obj.hasScaleVal = true;
                // can be scale(f) or scale(x,y,z)
                float x = parseFloat(code, p);
                size_t pp = p; skipWS(code, pp);
                if (pp < code.size() && code[pp] == ',') {
                    ++pp; float y = parseFloat(code, pp);
                    skipWS(code, pp);
                    if (pp < code.size() && code[pp] == ',') { ++pp; float z = parseFloat(code, pp); p = pp; obj.scaleVal = {x,y,z}; }
                    else { obj.scaleVal = {x,y,x}; p = pp; }
                    obj.scaleUniform = false;
                } else {
                    obj.scaleUniform = true;
                    obj.scaleUni = x;
                    obj.scaleVal = {x,x,x};
                }
                skipTo(code, p, ')');
            } else if (method == "color") {
                obj.hasColor = true;
                obj.colorVal.r = parseFloat(code, p); skipTo(code, p, ',');
                obj.colorVal.g = parseFloat(code, p); skipTo(code, p, ',');
                obj.colorVal.b = parseFloat(code, p);
                skipTo(code, p, ')');
            } else if (method == "metallic") {
                obj.hasMetallic = true;
                obj.metallicVal = parseFloat(code, p);
                skipTo(code, p, ')');
            } else if (method == "roughness") {
                obj.hasRoughness = true;
                obj.roughnessVal = parseFloat(code, p);
                skipTo(code, p, ')');
            } else {
                // unknown method — skip to ')'
                skipTo(code, p, ')');
            }
        }

        if (!obj.name.empty())
            parsed.push_back(obj);

        pos = p + 1;
    }

    // ── Step 3: parse addRigidBody(var.id(), Type, Shape, ...) ──────────────
    {
        size_t p = 0;
        while (true) {
            size_t rbPos = code.find("addRigidBody(", p);
            if (rbPos == std::string::npos) break;
            p = rbPos + 13; // after addRigidBody(

            // first argument: varname.id()
            skipWS(code, p);
            size_t varStart = p;
            while (p < code.size() && (std::isalnum((unsigned char)code[p])||code[p]=='_')) ++p;
            std::string varRef = code.substr(varStart, p - varStart);

            // find corresponding ParsedObj by varInCode
            ParsedObj* target = nullptr;
            for (auto& o : parsed)
                if (o.varInCode == varRef) { target = &o; break; }

            // skip .id()
            skipTo(code, p, ',');

            // RigidBodyType::XXX
            skipWS(code, p);
            // skip "RigidBodyType::"
            size_t tp = code.find("::", p);
            if (tp != std::string::npos && tp < p + 30) p = tp + 2;
            size_t typeStart = p;
            while (p < code.size() && std::isalpha((unsigned char)code[p])) ++p;
            std::string typeStr = code.substr(typeStart, p - typeStart);

            skipTo(code, p, ',');

            // ColliderShape::XXX
            skipWS(code, p);
            size_t sp2 = code.find("::", p);
            if (sp2 != std::string::npos && sp2 < p + 30) p = sp2 + 2;
            size_t shapeStart = p;
            while (p < code.size() && std::isalpha((unsigned char)code[p])) ++p;
            std::string shapeStr = code.substr(shapeStart, p - shapeStart);

            if (target) {
                target->hasRB = true;
                target->rbType  = (typeStr == "Static")    ? RigidBodyType::Static
                                : (typeStr == "Kinematic") ? RigidBodyType::Kinematic
                                :                            RigidBodyType::Dynamic;
                target->rbShape = (shapeStr == "Sphere")  ? ColliderShape::Sphere
                                : (shapeStr == "Capsule") ? ColliderShape::Capsule
                                :                           ColliderShape::Box;

                // optional halfExt and mass
                skipWS(code, p);
                if (p < code.size() && code[p] == ',') {
                    ++p; skipWS(code, p);
                    if (p < code.size() && code[p] == '{') {
                        ++p;
                        target->rbHalfExt.x = parseFloat(code, p); skipTo(code, p, ',');
                        target->rbHalfExt.y = parseFloat(code, p); skipTo(code, p, ',');
                        target->rbHalfExt.z = parseFloat(code, p); skipTo(code, p, '}');
                        // optional mass
                        skipWS(code, p);
                        if (p < code.size() && code[p] == ',') {
                            ++p; target->rbMass = parseFloat(code, p);
                        }
                    } else {
                        // directly a mass float
                        target->rbMass = parseFloat(code, p);
                    }
                }
            }
            skipTo(code, p, ')');
        }
    }

    // ── Step 4: synchronize scene ──────────────────────────────────────────────

    // Object names from code
    std::vector<std::string> codeNames;
    for (auto& o : parsed) codeNames.push_back(o.name);

    // Remove from scene what is not in code
    std::vector<EntityID> toDelete;
    m_world.each<TagComp>([&](EntityID id, TagComp& t) {
        if (t.value == "Floor" || t.value == "DirectionalLight") return;
        bool found = std::find(codeNames.begin(), codeNames.end(), t.value) != codeNames.end();
        if (!found) toDelete.push_back(id);
    });
    for (EntityID id : toDelete) {
        m_app.physics().removeBody(id);
        m_world.destroy(id);
    }

    int applied = 0;

    for (auto& obj : parsed) {
        if (obj.name == "Floor" || obj.name == "DirectionalLight") continue;

        // find existing object with this name
        EntityID existId = m_world.findByName(obj.name);
        GameObject go = (existId != 0)
            ? GameObject(existId, &m_world)
            : m_app.spawn(obj.name);

        // Mesh
        switch (obj.mesh) {
            case ParsedObj::Mesh::Cube:     go.cube();     break;
            case ParsedObj::Mesh::Sphere:   go.sphere();   break;
            case ParsedObj::Mesh::Quad:     go.quad();     break;
            case ParsedObj::Mesh::Plane:    go.plane();    break;
            case ParsedObj::Mesh::Cylinder: go.cylinder(); break;
            case ParsedObj::Mesh::Model:    if (!obj.modelPath.empty()) go.model(obj.modelPath); break;
            default: break;
        }

        // Transform
        if (obj.hasPosVal)   go.pos(obj.posVal.x, obj.posVal.y, obj.posVal.z);
        if (obj.hasRotVal)   go.rot(obj.rotVal.x, obj.rotVal.y, obj.rotVal.z);
        if (obj.hasScaleVal) go.scale(obj.scaleVal.x, obj.scaleVal.y, obj.scaleVal.z);

        // Material
        if (obj.hasColor)     go.color(obj.colorVal.r, obj.colorVal.g, obj.colorVal.b);
        if (obj.hasMetallic)  go.metallic(obj.metallicVal);
        if (obj.hasRoughness) go.roughness(obj.roughnessVal);

        // RigidBody — remove old if type changed, add new
        if (obj.hasRB) {
            if (m_world.has<RigidBodyComponent>(go.id()))
                m_app.physics().removeBody(go.id());
            m_app.addRigidBody(go.id(), obj.rbType, obj.rbShape, obj.rbHalfExt, obj.rbMass);
        }

        ++applied;
    }

    m_liveStatus = "✓ " + std::to_string(applied) + " obj" +
                   (applied == 1 ? "" : "s") + " applied";
    LOG_INFO("CodeGenerator live: " << m_liveStatus);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Compile & Run
// ─────────────────────────────────────────────────────────────────────────────
std::string CodeGenerator::buildCommand() const {
#ifdef _WIN32
    return "cd /d \"" + fs::current_path().parent_path().string()
         + "\" && scripts\\build.bat 2>&1";
#else
    return "cd \"" + fs::current_path().parent_path().string()
         + "\" && ./scripts/build.sh 2>&1";
#endif
}

std::string CodeGenerator::runCommand() const {
#ifdef _WIN32
    return "start \"\" \""
         + (fs::current_path().parent_path() / "build" / "game.exe").string()
         + "\"";
#else
    return (fs::current_path().parent_path() / "build" / "game").string() + " &";
#endif
}

// FIX 1: read m_codeBuf, not generate()
bool CodeGenerator::compile(std::string& outLog) {
    std::string src(m_codeBuf);
    std::string mainPath = (fs::current_path().parent_path() / "src" / "main.cpp").string();
    {
        std::ofstream f(mainPath);
        if (!f) { outLog = "ERROR: cannot write " + mainPath; return false; }
        f << src;
    }

    outLog.clear();
    FILE* pipe = POPEN(buildCommand().c_str(), "r");
    if (!pipe) { outLog = "ERROR: failed to start build process"; return false; }

    char buf[256];
    while (fgets(buf, sizeof(buf), pipe))
        outLog += buf;

    int ret = PCLOSE(pipe);
    bool ok = (ret == 0);
    m_compiled.store(ok);
    return ok;
}

// FIX 2: build in a separate thread
void CodeGenerator::startBuildAsync() {
    if (m_building.load()) return;
    if (m_buildThread.joinable()) m_buildThread.join();

    m_building.store(true);
    m_compiled.store(false);
    { std::lock_guard<std::mutex> lk(m_logMutex); m_compileLog = "[ Building... ]\n\n"; }

    m_buildThread = std::thread([this]() {
        std::string log;
        bool ok = compile(log);
        std::lock_guard<std::mutex> lk(m_logMutex);
        m_compileLog = log + (ok ? "\n[ BUILD SUCCEEDED ]" : "\n[ BUILD FAILED ]");
        m_building.store(false);
    });
}

bool CodeGenerator::run(std::string& outLog) {
    if (!m_compiled.load()) { outLog = "Compile first!"; return false; }
#ifdef _WIN32
    std::string exe = (fs::current_path().parent_path() / "build" / "game.exe").string();
    ShellExecuteA(nullptr, "open", exe.c_str(), nullptr, nullptr, SW_SHOW);
    return true;
#else
    int ret = std::system(runCommand().c_str());
    return ret == 0;
#endif
}

bool CodeGenerator::exportTo(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << m_codeBuf;
    LOG_INFO("CodeGenerator: exported to " << path);
    return true;
}

bool CodeGenerator::importFrom(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string src((std::istreambuf_iterator<char>(f)), {});
    strncpy(m_codeBuf, src.c_str(), m_codeBufSz - 1);
    m_codeBuf[m_codeBufSz - 1] = '\0';
    m_dirty = false;
    LOG_INFO("CodeGenerator: imported " << path);
    return true;
}

void CodeGenerator::clearScene() {
    std::vector<EntityID> toDelete;
    m_world.each<TagComp>([&](EntityID id, TagComp& t) {
        if (t.value != "Floor" && t.value != "DirectionalLight")
            toDelete.push_back(id);
    });
    for (EntityID id : toDelete) {
        m_app.physics().removeBody(id);
        m_world.destroy(id);
    }
    LOG_INFO("CodeGenerator: cleared " << toDelete.size() << " objects");
}

void CodeGenerator::regenerateIfDirty() {
    std::string code = generate();
    strncpy(m_codeBuf, code.c_str(), m_codeBufSz - 1);
    m_codeBuf[m_codeBufSz - 1] = '\0';
    m_dirty = false;
    m_pendingApply = false;
    m_liveStatus.clear();
}

// FIX 3: live preview debounce
void CodeGenerator::tickLivePreview() {
    if (!m_pendingApply) return;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  Clock::now() - m_lastEditTime).count();
    if (ms < kLiveMs) return;
    m_pendingApply = false;
    applyCodeToScene();
}

// ─────────────────────────────────────────────────────────────────────────────
//  ImGui Panel
// ─────────────────────────────────────────────────────────────────────────────
void CodeGenerator::drawPanel() {
    if (!m_open) return;

    tickLivePreview();

    int W = m_app.window().width();
    int H = m_app.window().height();

    ImGui::SetNextWindowPos ({0, (float)(H-300)}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({(float)W, 300},     ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.97f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8,8});
    bool open = ImGui::Begin("##codepanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar(2);
    if (!open) { ImGui::End(); return; }

    if (ImGui::BeginTabBar("##codetabs")) {

        // ── Code tab ─────────────────────────────────────────────────────────
        if (ImGui::BeginTabItem("  < >  Code  ")) {
            m_tab = 0;

            ImGui::PushStyleColor(ImGuiCol_Button, {0.15f,0.35f,0.55f,1.f});
            if (ImGui::Button("  ↺  Regenerate  ")) regenerateIfDirty();
            ImGui::PopStyleColor();
            UI::Tooltip("Rebuild code from current scene state");
            ImGui::SameLine();

            if (ImGui::Button("  ⟳  Apply to Scene  ")) {
                applyCodeToScene();
            }
            UI::Tooltip("Parse code and update 3D scene immediately");
            ImGui::SameLine();

            if (ImGui::Button("  Copy  ")) ImGui::SetClipboardText(m_codeBuf);
            UI::Tooltip("Copy all code to clipboard");
            ImGui::SameLine();

            if (ImGui::Button("  Clear  ")) { if (m_codeBuf) m_codeBuf[0] = '\0'; }
            UI::Tooltip("Clear the code buffer");
            ImGui::SameLine();

            ImGui::TextDisabled("|");
            ImGui::SameLine();

            // Stats
            int lines = 0, chars = 0;
            for (char* p = m_codeBuf; *p; ++p) { chars++; if (*p=='\n') lines++; }
            int entityCount = 0;
            const_cast<World&>(m_world).each<TagComp>(
                [&](EntityID, TagComp& t) {
                    if (t.value!="Floor"&&t.value!="DirectionalLight") entityCount++;
                });
            UI::LabelColored(UI::GRAY, "%d lines  %d chars  %d objects",
                             lines+1, chars, entityCount);
            ImGui::SameLine();

            ImGui::TextDisabled("|");
            ImGui::SameLine();

            // Live status
            if (m_pendingApply)
                UI::LabelColored(UI::YELLOW, "⟳ applying...");
            else if (!m_liveStatus.empty())
                UI::LabelColored(UI::GREEN, "%s", m_liveStatus.c_str());
            else if (m_dirty)
                UI::LabelColored(UI::YELLOW, "● unsaved changes");
            else
                UI::LabelColored(UI::GREEN, "✓ up to date");

            ImGui::Separator();

            float avail = ImGui::GetContentRegionAvail().y;

            ImGui::PushStyleColor(ImGuiCol_FrameBg,        {0.07f,0.08f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {0.09f,0.10f,0.13f,1.f});
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  {0.07f,0.08f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_Text,           {0.85f,0.90f,0.95f,1.f});
            ImGui::InputTextMultiline("##code", m_codeBuf, (size_t)m_codeBufSz,
                {-1, avail}, ImGuiInputTextFlags_AllowTabInput);
            ImGui::PopStyleColor(4);

            if (ImGui::IsItemEdited()) {
                m_dirty = true;
                m_pendingApply  = true;
                m_lastEditTime  = Clock::now();
                m_liveStatus.clear();
            }

            ImGui::EndTabItem();
        }

        // ── Build tab ─────────────────────────────────────────────────────────
        if (ImGui::BeginTabItem("  ▶  Build  ")) {
            m_tab = 1;
            ImGui::Spacing();

            bool building = m_building.load();
            bool compiled = m_compiled.load();

            ImGui::BeginChild("##status_row", {-1,32}, false);
            if (building)      UI::LabelColored(UI::YELLOW, "  ⟳  Building...");
            else if (compiled) UI::LabelColored(UI::GREEN,  "  ✓  Last build succeeded");
            else if (!m_compileLog.empty()) UI::LabelColored(UI::RED, "  ✗  Last build failed — see log below");
            else               UI::LabelColored(UI::GRAY,   "  ○  Not compiled yet");
            ImGui::EndChild();

            ImGui::Separator(); ImGui::Spacing();

            float bw = 170.f;

            ImGui::BeginDisabled(building);
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.42f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.14f,0.56f,0.14f,1.f});
            if (ImGui::Button("  ▶  Compile & Save  ", {bw,38})) {
                regenerateIfDirty();
                startBuildAsync();
            }
            ImGui::PopStyleColor(2);
            ImGui::EndDisabled();
            UI::Tooltip("Writes main.cpp → runs build script → updates log");

            if (building) { ImGui::SameLine(0,12); ImGui::TextDisabled("[ building... ]"); }

            ImGui::SameLine(0,10);

            ImGui::BeginDisabled(!compiled || building);
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.30f,0.60f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.14f,0.40f,0.75f,1.f});
            if (ImGui::Button("  ▷  Run Game  ", {bw,38})) {
                std::string log; run(log);
                if (!log.empty()) { std::lock_guard<std::mutex> lk(m_logMutex); m_compileLog += "\n"+log; }
            }
            ImGui::PopStyleColor(2);
            ImGui::EndDisabled();
            UI::Tooltip("Launch compiled game.exe");

            ImGui::SameLine(0,10);

            ImGui::BeginDisabled(building);
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.48f,0.10f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.62f,0.14f,0.14f,1.f});
            if (ImGui::Button("  ✕  Clear Scene  ", {bw,38})) {
                clearScene(); regenerateIfDirty();
                m_compiled.store(false);
                std::lock_guard<std::mutex> lk(m_logMutex); m_compileLog.clear();
            }
            ImGui::PopStyleColor(2);
            ImGui::EndDisabled();
            UI::Tooltip("Remove all objects and reset to empty scene");

            ImGui::Spacing(); ImGui::Separator();
            ImGui::TextDisabled("Build output:");
            float logH = ImGui::GetContentRegionAvail().y - 4;

            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.04f,0.05f,0.06f,1.f});
            ImGui::BeginChild("##buildlog", {-1,logH}, false,
                ImGuiWindowFlags_HorizontalScrollbar);

            std::string logSnap;
            { std::lock_guard<std::mutex> lk(m_logMutex); logSnap = m_compileLog; }

            std::istringstream iss(logSnap);
            std::string line;
            while (std::getline(iss, line)) {
                bool isErr  = line.find("error")  !=std::string::npos||line.find("FAILED")!=std::string::npos||line.find("failed")!=std::string::npos;
                bool isWarn = line.find("warning")!=std::string::npos||line.find("note:") !=std::string::npos;
                bool isOk   = line.find("SUCCEEDED")!=std::string::npos||line.find("[OK]")!=std::string::npos;
                ImVec4 col = isErr  ? ImVec4{0.95f,0.35f,0.35f,1.f}
                           : isWarn ? ImVec4{0.95f,0.80f,0.20f,1.f}
                           : isOk   ? ImVec4{0.30f,0.90f,0.30f,1.f}
                           :          ImVec4{0.72f,0.75f,0.80f,1.f};
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                ImGui::TextUnformatted(line.c_str());
                ImGui::PopStyleColor();
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()-20)
                ImGui::SetScrollHereY(1.f);
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::EndTabItem();
        }

        // ── Files tab ─────────────────────────────────────────────────────────
        if (ImGui::BeginTabItem("  ⇅  Files  ")) {
            m_tab = 2;
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.6f,0.85f,1.f,1.f});
            ImGui::TextUnformatted("  Export main.cpp");
            ImGui::PopStyleColor();
            ImGui::Separator(); ImGui::Spacing();

            ImGui::TextDisabled("Save path:");
            ImGui::SetNextItemWidth(-110);
            ImGui::InputText("##exportpath", m_exportPath, sizeof(m_exportPath));
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.42f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.14f,0.56f,0.14f,1.f});
            if (ImGui::Button("Export##btn", {100,0})) {
                bool ok = exportTo(m_exportPath);
                std::lock_guard<std::mutex> lk(m_logMutex);
                m_compileLog = ok ? "✓ Exported to: "+std::string(m_exportPath)
                                  : "✗ ERROR: cannot write file";
            }
            ImGui::PopStyleColor(2);
            UI::Tooltip("Save current code to a .cpp file");

            ImGui::Spacing(); ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.6f,0.85f,1.f,1.f});
            ImGui::TextUnformatted("  Import main.cpp");
            ImGui::PopStyleColor();
            ImGui::Separator(); ImGui::Spacing();

            ImGui::TextDisabled("File path:");
            ImGui::SetNextItemWidth(-110);
            ImGui::InputText("##importpath", m_importPath, sizeof(m_importPath));
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f,0.30f,0.60f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.14f,0.40f,0.75f,1.f});
            if (ImGui::Button("Import##btn", {100,0})) {
                bool ok = importFrom(m_importPath);
                std::lock_guard<std::mutex> lk(m_logMutex);
                m_compileLog = ok ? "✓ Imported: "+std::string(m_importPath)
                                  : "✗ ERROR: file not found";
            }
            ImGui::PopStyleColor(2);
            UI::Tooltip("Load a .cpp file into the code editor");

            ImGui::Spacing();
            ImGui::TextWrapped(
                "Import loads the file text into the Code tab. "
                "The 3D scene is not automatically reconstructed — "
                "press 'Apply to Scene' after importing.");

            ImGui::Spacing(); ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.6f,0.85f,1.f,1.f});
            ImGui::TextUnformatted("  Scene");
            ImGui::PopStyleColor();
            ImGui::Separator(); ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button,        {0.48f,0.10f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.62f,0.14f,0.14f,1.f});
            if (ImGui::Button("  ✕  Clear Scene  ", {180,32})) {
                clearScene(); regenerateIfDirty(); m_compiled.store(false);
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
            ImGui::TextDisabled("Remove all objects and start fresh");

            std::string logSnap2;
            { std::lock_guard<std::mutex> lk(m_logMutex); logSnap2 = m_compileLog; }
            if (!logSnap2.empty() && m_tab == 2) {
                ImGui::Spacing(); ImGui::Separator();
                bool isErr = logSnap2.find("ERROR")!=std::string::npos||logSnap2.find("✗")!=std::string::npos;
                UI::LabelColored(isErr ? UI::RED : UI::GREEN, "%s", logSnap2.c_str());
            }

            ImGui::EndTabItem();
        }

        // ── UI / onRender tab ─────────────────────────────────────────────────
        if (ImGui::BeginTabItem("  ◈  UI / onRender  ")) {
            m_tab = 3;

            // button
            ImGui::PushStyleColor(ImGuiCol_Button, {0.15f,0.35f,0.55f,1.f});
            if (ImGui::Button("  Copy  "))
                ImGui::SetClipboardText(m_renderBuf);
            ImGui::PopStyleColor();
            UI::Tooltip("Copy UI code to clipboard");
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, {0.48f,0.10f,0.10f,1.f});
            if (ImGui::Button("  Reset to default  ")) {
                const char* def =
                    "        if (UI::BeginFixed(\"##hud\", 8, 8, 0, 0,\n"
                    "            ImGuiWindowFlags_NoTitleBar |\n"
                    "            ImGuiWindowFlags_NoBackground |\n"
                    "            ImGuiWindowFlags_AlwaysAutoResize))\n"
                    "        {\n"
                    "            UI::LabelColored(UI::WHITE, \"FPS: %.0f\", Time::fps());\n"
                    "        }\n"
                    "        UI::End();\n";
                strncpy(m_renderBuf, def, m_renderBufSz - 1);
                m_renderBuf[m_renderBufSz - 1] = '\0';
                m_renderDirty = true;
            }
            ImGui::PopStyleColor();
            UI::Tooltip("Restore default FPS HUD");

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            UI::LabelColored(UI::GRAY,
                "Редактируй тело onRender — Regenerate не затрёт эти изменения");

            ImGui::Separator();

            // API hint
            if (ImGui::TreeNodeEx("##uihelp", ImGuiTreeNodeFlags_None, "  ℹ  UI API — краткий справочник")) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.65f,0.85f,1.f,1.f});
                ImGui::TextUnformatted(
                    "UI::BeginFixed(id, x, y, w, h, flags)  /  UI::End()\n"
                    "UI::Label(fmt, ...)   UI::LabelColored(color, fmt, ...)\n"
                    "UI::Button(label)     UI::SmallButton(label)\n"
                    "UI::SliderFloat(label, val, min, max)\n"
                    "UI::SliderInt / UI::InputFloat / UI::InputText / UI::Vec3\n"
                    "UI::Section(label)    UI::Separator()    UI::Spacing()\n"
                    "Colors: UI::WHITE  UI::YELLOW  UI::CYAN  UI::RED  UI::GREEN  UI::ACCENT()\n"
                    "Time::fps()   Time::dt()   Time::elapsed()");
                ImGui::PopStyleColor();
                ImGui::TreePop();
            }

            ImGui::Spacing();

            float avail = ImGui::GetContentRegionAvail().y;

            ImGui::PushStyleColor(ImGuiCol_FrameBg,        {0.07f,0.08f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {0.09f,0.10f,0.13f,1.f});
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  {0.07f,0.08f,0.10f,1.f});
            ImGui::PushStyleColor(ImGuiCol_Text,           {0.85f,0.90f,0.95f,1.f});
            ImGui::InputTextMultiline("##rendercode", m_renderBuf, (size_t)m_renderBufSz,
                {-1, avail}, ImGuiInputTextFlags_AllowTabInput);
            ImGui::PopStyleColor(4);

            if (ImGui::IsItemEdited())
                m_renderDirty = true;

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
std::string CodeGenerator::tagName(EntityID id) const {
    auto* t = m_world.get<TagComp>(id);
    return t ? t->value : ("Entity_" + std::to_string(id));
}
std::string CodeGenerator::varName(EntityID id) const {
    std::string n = tagName(id), v;
    bool first = true;
    for (char c : n) {
        if (std::isalnum((unsigned char)c))     { v += (char)std::tolower((unsigned char)c); first=false; }
        else if (!first && (c==' '||c=='_'))     v += '_';
    }
    if (v.empty()||std::isdigit((unsigned char)v[0])) v = "obj_"+v;
    v += "_" + std::to_string(id);
    return v;
}
std::string CodeGenerator::col3(glm::vec3 c) const {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(2)
        <<c.r<<"f, "<<c.g<<"f, "<<c.b<<"f"; return ss.str();
}
std::string CodeGenerator::vec3(glm::vec3 v) const {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(3)
        <<v.x<<"f, "<<v.y<<"f, "<<v.z<<"f"; return ss.str();
}
std::string CodeGenerator::flt(float f) const {
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(3)<<f; return ss.str();
}

} // namespace eng
