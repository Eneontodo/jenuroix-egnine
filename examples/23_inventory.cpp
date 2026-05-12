// ============================================================
//  Example 23 — Inventory UI System
//
//  Shows a classic grid-based inventory with:
//    - Clickable item slots (pick up / place)
//    - Item categories with color coding
//    - Tooltip with item description on hover
//    - Hotbar (slots 1-5 mapped to keyboard)
//    - Simple drag simulation (click to pick, click to place)
//
//  Controls:
//    I / Esc  — open / close inventory
//    1-5      — use hotbar slot
//    LMB      — pick up / place item
// ============================================================
#include "engine.h"
#include <vector>
#include <string>
#include <optional>
using namespace eng;

// ── Item definition ───────────────────────────────────────────────────────
enum class ItemType { None, Weapon, Armor, Potion, Misc };

struct Item {
    std::string name;
    std::string desc;
    ItemType    type   = ItemType::None;
    int         count  = 1;
    glm::vec3   color  = {0.7f, 0.7f, 0.7f};
};

// ── Inventory state ───────────────────────────────────────────────────────
constexpr int INV_COLS = 6;
constexpr int INV_ROWS = 4;
constexpr int HOT_SLOTS = 5;

struct Inventory {
    std::vector<std::optional<Item>> slots;    // INV_COLS * INV_ROWS
    std::vector<std::optional<Item>> hotbar;   // HOT_SLOTS
    std::optional<Item>              held;      // currently "dragged"

    Inventory() {
        slots .resize(INV_COLS * INV_ROWS);
        hotbar.resize(HOT_SLOTS);
    }

    void put(int idx, Item item) { slots[idx] = item; }
    void putHot(int idx, Item item) { hotbar[idx] = item; }
};

// ── Helper: draw one inventory slot ──────────────────────────────────────
// Returns true if clicked.
static bool DrawSlot(const std::optional<Item>& item,
                     float size, bool highlight = false)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    auto*  dl  = ImGui::GetWindowDrawList();

    // Slot background
    ImU32 bg = highlight
        ? IM_COL32(80, 120, 200, 180)
        : IM_COL32(40, 40, 55, 200);
    dl->AddRectFilled(pos, {pos.x + size, pos.y + size}, bg, 4.f);
    dl->AddRect      (pos, {pos.x + size, pos.y + size},
                      IM_COL32(100, 100, 130, 200), 4.f);

    bool clicked = false;
    ImGui::InvisibleButton("##slot", {size, size});
    if (ImGui::IsItemClicked()) clicked = true;

    if (item) {
        // Color square representing the item type
        ImVec2 ic = {pos.x + 6, pos.y + 6};
        dl->AddRectFilled(ic, {ic.x + size - 12, ic.y + size - 12},
            IM_COL32((int)(item->color.r * 255),
                     (int)(item->color.g * 255),
                     (int)(item->color.b * 255), 230), 3.f);

        // Item name (abbreviated)
        std::string label = item->name.size() > 6
            ? item->name.substr(0, 5) + "…"
            : item->name;
        dl->AddText({pos.x + 4, pos.y + size - 16},
                    IM_COL32(255, 255, 255, 220), label.c_str());

        // Stack count
        if (item->count > 1) {
            std::string cnt = std::to_string(item->count);
            dl->AddText({pos.x + size - 12 - (float)cnt.size() * 4, pos.y + 4},
                        IM_COL32(255, 220, 80, 255), cnt.c_str());
        }

        // Tooltip on hover
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextColored({item->color.r, item->color.g, item->color.b, 1},
                               "%s", item->name.c_str());
            ImGui::TextDisabled("%s", item->desc.c_str());
            const char* cats[] = {"?","Weapon","Armor","Potion","Misc"};
            ImGui::TextDisabled("Type: %s  x%d",
                cats[(int)item->type], item->count);
            ImGui::EndTooltip();
        }
    }

    return clicked;
}

int main() {
    App app("Inventory System", 1280, 720);
    app.freeCam(false)
       .skyColor(0.08f, 0.1f, 0.14f)
       .lightDir(0.4f, 1.f, 0.5f)
       .ambient(0.15f, 0.15f, 0.2f);
    app.camPos(0, 3, 10).camRot(-90, -15);

    // Background scene
    app.spawn("Ground").grid(20, 20).color(0.18f, 0.2f, 0.18f).pos(0, -0.5f, 0);
    for (int i = 0; i < 5; ++i) {
        float a = i * 1.2566f;
        app.spawn("Obj_" + std::to_string(i))
           .cube().scale(0.8f)
           .color(0.3f + i * 0.1f, 0.4f, 0.6f)
           .pos(std::cos(a) * 4.f, 0, std::sin(a) * 4.f)
           .onUpdate([](GameObject& s, float dt){ s.rotY(s.rot().y + 20.f * dt); });
    }

    // ── Populate inventory ────────────────────────────────────────────────
    Inventory inv;
    // Weapons
    inv.put(0,  {"Sword",    "A sharp steel sword",      ItemType::Weapon, 1, {0.7f,0.7f,0.9f}});
    inv.put(1,  {"Axe",      "Heavy two-handed axe",     ItemType::Weapon, 1, {0.8f,0.5f,0.3f}});
    inv.put(2,  {"Bow",      "Elven longbow",            ItemType::Weapon, 1, {0.6f,0.8f,0.4f}});
    // Armor
    inv.put(6,  {"Helmet",   "Iron helmet +5 defense",   ItemType::Armor,  1, {0.6f,0.6f,0.7f}});
    inv.put(7,  {"Chestpl.", "Steel chestplate",         ItemType::Armor,  1, {0.55f,0.6f,0.7f}});
    // Potions
    inv.put(12, {"HP Pot",   "Restores 50 HP",           ItemType::Potion, 5, {0.9f,0.2f,0.25f}});
    inv.put(13, {"MP Pot",   "Restores 30 MP",           ItemType::Potion, 3, {0.3f,0.3f,0.9f}});
    inv.put(14, {"Speed",    "Move faster for 30s",      ItemType::Potion, 2, {0.2f,0.9f,0.5f}});
    // Misc
    inv.put(18, {"Gold",     "100 gold coins",           ItemType::Misc,   100,{0.95f,0.8f,0.1f}});
    inv.put(19, {"Key",      "Opens a locked chest",     ItemType::Misc,   1,  {0.9f,0.7f,0.2f}});
    // Hotbar
    inv.putHot(0, {"Sword", "A sharp steel sword",      ItemType::Weapon, 1, {0.7f,0.7f,0.9f}});
    inv.putHot(1, {"HP Pot","Restores 50 HP",           ItemType::Potion, 5, {0.9f,0.2f,0.25f}});
    inv.putHot(2, {"Gold",  "100 gold coins",           ItemType::Misc,  100,{0.95f,0.8f,0.1f}});

    bool    showInv    = false;
    int     activeHot  = 0;

    app.onUpdate([&](float dt) {
        if (app.keyDown(GLFW_KEY_I) || (showInv && app.keyDown(KEY_ESC)))
            showInv = !showInv;
        for (int i = 0; i < HOT_SLOTS; ++i)
            if (app.keyDown(GLFW_KEY_1 + i)) activeHot = i;
    });

    app.onRender([&]() {
        float SW = ImGui::GetIO().DisplaySize.x;
        float SH = ImGui::GetIO().DisplaySize.y;

        // ── Hotbar ────────────────────────────────────────────────────────
        const float SLOT_SZ = 54.f;
        const float PAD     = 6.f;
        float hotW = (SLOT_SZ + PAD) * HOT_SLOTS + PAD;
        float hotX = (SW - hotW) * 0.5f;
        float hotY = SH - SLOT_SZ - 16.f;

        ImGui::SetNextWindowPos({hotX, hotY}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({hotW, SLOT_SZ + 2 * PAD}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.f);
        ImGui::Begin("##hotbar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoDecoration);

        for (int i = 0; i < HOT_SLOTS; ++i) {
            if (i > 0) ImGui::SameLine(0, PAD);
            ImGui::PushID(i);
            if (DrawSlot(inv.hotbar[i], SLOT_SZ, i == activeHot))
                activeHot = i;
            ImGui::PopID();
        }
        ImGui::End();

        // ── HUD hint ──────────────────────────────────────────────────────
        if (UI::BeginFixed("##hud", 8, 8, 0, 0,
            ImGuiWindowFlags_NoTitleBar    |
            ImGuiWindowFlags_NoBackground  |
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::LabelColored(UI::WHITE, "FPS: %.0f", Time::fps());
            UI::LabelColored(UI::GRAY,  "I — open inventory");
            UI::LabelColored(UI::GRAY,  "1-5 — hotbar");
            if (inv.held)
                UI::LabelColored(UI::YELLOW, "Holding: %s", inv.held->name.c_str());
        }
        UI::End();

        if (!showInv) return;

        // ── Inventory window ──────────────────────────────────────────────
        float invW = (SLOT_SZ + PAD) * INV_COLS + PAD * 2 + 16.f;
        float invH = (SLOT_SZ + PAD) * INV_ROWS + PAD * 2 + 50.f;
        ImGui::SetNextWindowPos({(SW - invW) * 0.5f, (SH - invH) * 0.5f},
                                ImGuiCond_Appearing);
        ImGui::SetNextWindowSize({invW, invH}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.88f);

        if (ImGui::Begin("Inventory", &showInv, ImGuiWindowFlags_NoResize)) {
            ImGui::TextColored({0.8f,0.7f,0.3f,1}, "Held: %s",
                inv.held ? inv.held->name.c_str() : "(none)");
            ImGui::Separator();

            for (int row = 0; row < INV_ROWS; ++row) {
                for (int col = 0; col < INV_COLS; ++col) {
                    int idx = row * INV_COLS + col;
                    if (col > 0) ImGui::SameLine(0, PAD);
                    ImGui::PushID(idx);

                    if (DrawSlot(inv.slots[idx], SLOT_SZ)) {
                        // Toggle: pick up or place
                        if (inv.held) {
                            std::swap(inv.slots[idx], inv.held);
                        } else if (inv.slots[idx]) {
                            inv.held = inv.slots[idx];
                            inv.slots[idx].reset();
                        }
                    }
                    ImGui::PopID();
                }
            }
        }
        ImGui::End();
    });

    return app.run();
}
