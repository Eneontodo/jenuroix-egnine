// ════════════════════════════════════════════════════════════════════
//  Example 11 — UI Inspector
//  Live object inspector, tabs, tables, popups
// ════════════════════════════════════════════════════════════════════
#include "engine.h"
#include <cmath>
#include <vector>
#include <string>
using namespace eng;

int main() {
    App app("UI Inspector", 1280, 720);
    app.skyColor(0.1f, 0.1f, 0.14f)
        .lightDir(0.5f, 1.f, 0.3f)
        .ambient(0.12f, 0.12f, 0.18f);

    // Per-object editable state
    struct ObjData {
        std::string  name;
        glm::vec3    pos    = {0,0,0};
        glm::vec3    rot    = {0,0,0};
        glm::vec3    scl    = {1,1,1};
        glm::vec3    color  = {1,1,1};
        bool         visible= true;
        bool         spin   = false;
        float        spinSpd= 60.f;
        GameObject   obj;
    };

    std::vector<ObjData> objects;
    int selIdx = 0;

    // Lights
    glm::vec3 lightDir   = {0.5f, 1.f, 0.3f};
    glm::vec3 lightColor = {1.f, 0.95f, 0.85f};
    glm::vec3 ambient    = {0.12f, 0.12f, 0.18f};
    glm::vec3 skyCol     = {0.1f, 0.1f, 0.14f};
    float     fovVal     = 60.f;

    // Log
    std::vector<std::string> logs;
    auto addLog = [&](const std::string& s) {
        logs.push_back(s);
        if (logs.size() > 20) logs.erase(logs.begin());
    };

    app.onStart([&]() {
        // Ground
        app.spawn("Ground").grid(20,20).color(0.18f,0.18f,0.22f).pos(0,-0.5f,0);

        // Initial objects
        auto add = [&](const char* name, glm::vec3 pos, glm::vec3 col, bool cube) {
            ObjData d;
            d.name  = name;
            d.pos   = pos;
            d.color = col;
            d.obj   = cube
                ? app.spawn(name).cube().color(col.r,col.g,col.b).pos(pos)
                : app.spawn(name).sphere().color(col.r,col.g,col.b).pos(pos);
            objects.push_back(d);
        };

        add("RedCube",    {-3, 0, 0}, {0.9f,0.2f,0.2f}, true);
        add("GreenSphere",{ 0, 0, 0}, {0.2f,0.8f,0.3f}, false);
        add("BlueCube",  { 3, 0, 0}, {0.2f,0.5f,0.9f}, true);
        add("YellowSphere",{0, 0,-3},{0.9f,0.8f,0.1f}, false);

        addLog("Scene initialised with 4 objects.");
    });

    app.onUpdate([&](float dt) {
        for (auto& d : objects) {
            if (!d.obj.valid()) continue;
            if (d.spin)
                d.rot.y += d.spinSpd * dt;
            d.obj.rot(d.rot.x, d.rot.y, d.rot.z);
            d.obj.pos(d.pos);
            d.obj.scale(d.scl.x, d.scl.y, d.scl.z);
            if (d.visible) d.obj.show(); else d.obj.hide();
            d.obj.color(d.color.r, d.color.g, d.color.b);
        }

        app.lightDir(lightDir.x, lightDir.y, lightDir.z);
        app.lightColor(lightColor.r, lightColor.g, lightColor.b);
        app.ambient(ambient.r, ambient.g, ambient.b);
        app.skyColor(skyCol.r, skyCol.g, skyCol.b);
        app.fov(fovVal);
    });

    app.onRender([&]() {
        float SW = ImGui::GetIO().DisplaySize.x;
        float SH = ImGui::GetIO().DisplaySize.y;

        // ── Hierarchy panel (left) ─────────────────────────────────
        if (UI::BeginFixed("Hierarchy", 8, 8, 200, SH-16)) {
            UI::Header("Scene");
            for (int i = 0; i < (int)objects.size(); i++) {
                bool sel = (i == selIdx);
                if (sel) ImGui::PushStyleColor(ImGuiCol_Text,
                    ImVec4(0.26f,0.59f,0.98f,1.f));
                if (ImGui::Selectable(objects[i].name.c_str(), sel))
                    selIdx = i;
                if (sel) ImGui::PopStyleColor();
            }
            UI::Separator();
            if (UI::SmallButton("+ Cube")) {
                ObjData d;
                d.name  = "Cube_" + std::to_string(objects.size());
                d.color = {0.7f,0.7f,0.7f};
                d.obj   = app.spawn(d.name).cube()
                    .color(d.color.r,d.color.g,d.color.b);
                objects.push_back(d);
                selIdx = (int)objects.size()-1;
                addLog("Created: " + d.name);
            }
            UI::SameLine();
            if (UI::SmallButton("- Remove") && !objects.empty()) {
                addLog("Removed: " + objects[selIdx].name);
                objects[selIdx].obj.destroy();
                objects.erase(objects.begin()+selIdx);
                selIdx = (selIdx > 0) ? selIdx - 1 : 0;
            }
        }
        UI::End();

        // ── Inspector panel (right) ────────────────────────────────
        if (UI::BeginFixed("Inspector", SW-280, 8, 272, SH*0.65f)) {
            if (!objects.empty() && selIdx < (int)objects.size()) {
                auto& d = objects[selIdx];
                UI::Header(d.name.c_str());

                if (UI::Section("Transform")) {
                    if (UI::Vec3("Position", d.pos, 0.05f))
                        addLog(d.name + ": pos changed");
                    if (UI::Vec3("Rotation", d.rot, 1.f))
                        addLog(d.name + ": rot changed");
                    if (UI::Vec3("Scale",    d.scl, 0.05f))
                        addLog(d.name + ": scale changed");
                    if (UI::SmallButton("Reset Transform")) {
                        d.pos = {0,0,0};
                        d.rot = {0,0,0};
                        d.scl = {1,1,1};
                    }
                }

                if (UI::Section("Appearance")) {
                    if (UI::Color3("Color", d.color))
                        addLog(d.name + ": color changed");
                    UI::Bool("Visible", d.visible);
                }

                if (UI::Section("Behaviour")) {
                    UI::Bool("Auto Spin",  d.spin);
                    if (d.spin)
                        UI::SliderFloat("Spin Speed", d.spinSpd, -360, 360);
                }

                UI::Separator();
                UI::ReadOnly("Name",  d.name.c_str());
            } else {
                UI::LabelColored(UI::GRAY, "No object selected");
            }
        }
        UI::End();

        // ── Tabs: Console + Scene Settings ─────────────────────────
        if (UI::BeginFixed("##bottom", SW-280, SH*0.65f+16, 272, SH*0.35f-24)) {
            if (UI::BeginTabBar("##tabs")) {

                if (UI::BeginTab("Console")) {
                    ImGui::BeginChild("##log", {0, -28}, false);
                    for (auto& l : logs)
                        ImGui::TextUnformatted(l.c_str());
                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                        ImGui::SetScrollHereY(1.f);
                    ImGui::EndChild();
                    if (UI::SmallButton("Clear")) logs.clear();
                    UI::EndTab();
                }

                if (UI::BeginTab("Lighting")) {
                    UI::Vec3("Direction",  lightDir,   0.01f);
                    UI::Color3("Light Color", lightColor);
                    UI::Color3("Ambient",  ambient);
                    UI::Color3("Sky",      skyCol);
                    UI::SliderFloat("FOV", fovVal, 30, 120);
                    UI::EndTab();
                }

                if (UI::BeginTab("Stats")) {
                    UI::LabelColored(UI::GREEN,  "FPS:     %.0f", Time::fps());
                    UI::LabelColored(UI::WHITE,  "Objects: %d",   (int)objects.size());
                    UI::LabelColored(UI::WHITE,  "DeltaT:  %.2fms", Time::delta()*1000);
                    UI::LabelColored(UI::WHITE,  "Uptime:  %.0fs", Time::elapsed());
                    UI::EndTab();
                }

                UI::EndTabBar();
            }
        }
        UI::End();

        // ── Object list table (center-bottom) ─────────────────────
        if (UI::BeginFixed("##table", 216, SH-150, SW-216-288, 142,
            ImGuiWindowFlags_NoTitleBar)) {
            UI::Header("All Objects");
            if (UI::BeginTable("##objtbl", 5)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("X");
                ImGui::TableSetupColumn("Y");
                ImGui::TableSetupColumn("Z");
                ImGui::TableSetupColumn("Vis");
                ImGui::TableHeadersRow();
                for (auto& d : objects) {
                    UI::TableNextRow();
                    UI::TableNextColumn(); ImGui::TextUnformatted(d.name.c_str());
                    UI::TableNextColumn(); ImGui::Text("%.1f", d.pos.x);
                    UI::TableNextColumn(); ImGui::Text("%.1f", d.pos.y);
                    UI::TableNextColumn(); ImGui::Text("%.1f", d.pos.z);
                    UI::TableNextColumn(); ImGui::TextUnformatted(d.visible?"Y":"N");
                }
                UI::EndTable();
            }
        }
        UI::End();
    });

    return app.run();
}
