// ════════════════════════════════════════════════════════════════════
//  UI.cpp  —  Реализация UI системы с поддержкой кастомных тем
//  Зависимости: ImGui, GLM
// ════════════════════════════════════════════════════════════════════
#include "UI.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace eng {
namespace UI {

// ════════════════════════════════════════════════════════════════════
//  Activity Theme
// ════════════════════════════════════════════════════════════════════

static Theme g_theme;   // Current active theme (default initialized)

const Theme& GetTheme() { return g_theme; }

// ════════════════════════════════════════════════════════════════════
//  Embedded Themes
// ════════════════════════════════════════════════════════════════════

namespace Themes {

// ── Cyberpunk 2077 ───────────────────────────────────────────────
Theme Cyberpunk()
{
    Theme t;
    t.bgDeep        = {0.06f,0.06f,0.08f,1.f};
    t.bgBase        = {0.09f,0.09f,0.12f,1.f};
    t.bgMid         = {0.12f,0.12f,0.16f,1.f};
    t.bgHigh        = {0.16f,0.16f,0.22f,1.f};
    t.bgFrame       = {0.10f,0.10f,0.14f,1.f};
    t.accent        = {1.00f,0.85f,0.00f,1.f};   // жёлтый #FFDA00
    t.accent2       = {0.00f,0.95f,1.00f,1.f};   // циан #00F2FF
    t.danger        = {1.00f,0.10f,0.20f,1.f};   // красный
    t.textMain      = {0.95f,0.92f,0.85f,1.f};   // тёплый белый
    t.textDim       = {0.55f,0.53f,0.48f,1.f};
    t.rounding      = 0.f;                        // острые углы
    t.windowBorder  = 1.f;
    t.frameBorder   = 1.f;
    t.buttonHeight  = 36.f;
    t.topBarColor   = t.accent;
    t.topBarHeight  = 3.f;
    t.sectionLeftBar= true;
    t.buttonNeonEdge= true;
    return t;
}

// ── Dark (редакторный, фиолетовый) ──────────────────────────────
Theme Dark()
{
    Theme t;
    t.bgDeep        = {0.08f,0.07f,0.10f,1.f};
    t.bgBase        = {0.12f,0.11f,0.15f,1.f};
    t.bgMid         = {0.17f,0.15f,0.21f,1.f};
    t.bgHigh        = {0.22f,0.19f,0.28f,1.f};
    t.bgFrame       = {0.13f,0.12f,0.17f,1.f};
    t.accent        = {0.68f,0.46f,1.00f,1.f};   // лавандовый
    t.accent2       = {0.40f,0.80f,1.00f,1.f};   // голубой
    t.danger        = {1.00f,0.35f,0.35f,1.f};
    t.textMain      = {0.92f,0.90f,0.95f,1.f};   // холодный белый
    t.textDim       = {0.50f,0.48f,0.55f,1.f};
    t.rounding      = 5.f;                        // мягкие углы
    t.windowBorder  = 1.f;
    t.frameBorder   = 0.f;                        // без рамки у полей
    t.buttonHeight  = 34.f;
    t.topBarColor   = t.accent;
    t.topBarHeight  = 2.f;
    t.sectionLeftBar= true;
    t.buttonNeonEdge= false;
    return t;
}

// ── Light ─────────────────────────────────
Theme Light()
{
    Theme t;
    t.bgDeep        = {0.82f,0.83f,0.85f,1.f};
    t.bgBase        = {0.88f,0.89f,0.91f,1.f};
    t.bgMid         = {0.80f,0.81f,0.83f,1.f};
    t.bgHigh        = {0.75f,0.76f,0.80f,1.f};
    t.bgFrame       = {0.96f,0.97f,0.98f,1.f};
    t.accent        = {0.18f,0.48f,0.90f,1.f};   // синий
    t.accent2       = {0.00f,0.65f,0.55f,1.f};   // бирюзовый
    t.danger        = {0.85f,0.20f,0.20f,1.f};
    t.textMain      = {0.10f,0.10f,0.12f,1.f};   // почти чёрный
    t.textDim       = {0.45f,0.46f,0.50f,1.f};
    t.rounding      = 4.f;
    t.windowBorder  = 1.f;
    t.frameBorder   = 1.f;
    t.buttonHeight  = 32.f;
    t.topBarColor   = t.accent;
    t.topBarHeight  = 2.f;
    t.sectionLeftBar= true;
    t.buttonNeonEdge= false;
    return t;
}

// ── Blood (тёмный красный) ───────────────────────────────────────
Theme Blood()
{
    Theme t;
    t.bgDeep        = {0.06f,0.04f,0.04f,1.f};
    t.bgBase        = {0.10f,0.06f,0.06f,1.f};
    t.bgMid         = {0.15f,0.08f,0.08f,1.f};
    t.bgHigh        = {0.20f,0.10f,0.10f,1.f};
    t.bgFrame       = {0.12f,0.07f,0.07f,1.f};
    t.accent        = {1.00f,0.20f,0.10f,1.f};   // алый
    t.accent2       = {1.00f,0.55f,0.10f,1.f};   // оранжевый
    t.danger        = {1.00f,0.80f,0.00f,1.f};   // жёлтое предупреждение
    t.textMain      = {0.98f,0.88f,0.85f,1.f};   // тёплый белый
    t.textDim       = {0.55f,0.40f,0.38f,1.f};
    t.rounding      = 0.f;
    t.windowBorder  = 1.f;
    t.frameBorder   = 1.f;
    t.buttonHeight  = 36.f;
    t.topBarColor   = t.accent;
    t.topBarHeight  = 3.f;
    t.sectionLeftBar= true;
    t.buttonNeonEdge= true;
    return t;
}

// ── Matrix (зелёный / чёрный) ────────────────────────────────────
Theme Matrix()
{
    Theme t;
    t.bgDeep        = {0.02f,0.04f,0.02f,1.f};
    t.bgBase        = {0.04f,0.07f,0.04f,1.f};
    t.bgMid         = {0.06f,0.10f,0.06f,1.f};
    t.bgHigh        = {0.08f,0.14f,0.08f,1.f};
    t.bgFrame       = {0.03f,0.06f,0.03f,1.f};
    t.accent        = {0.15f,1.00f,0.30f,1.f};   // неоново-зелёный
    t.accent2       = {0.50f,1.00f,0.60f,1.f};   // светло-зелёный
    t.danger        = {1.00f,0.30f,0.10f,1.f};
    t.textMain      = {0.70f,1.00f,0.70f,1.f};   // зелёный текст
    t.textDim       = {0.30f,0.55f,0.30f,1.f};
    t.rounding      = 0.f;
    t.windowBorder  = 1.f;
    t.frameBorder   = 1.f;
    t.buttonHeight  = 36.f;
    t.topBarColor   = t.accent;
    t.topBarHeight  = 2.f;
    t.sectionLeftBar= true;
    t.buttonNeonEdge= true;
    return t;
}

} // namespace Themes

// ════════════════════════════════════════════════════════════════════
//  ПРИМЕНЕНИЕ ТЕМЫ  →  ImGuiStyle
// ════════════════════════════════════════════════════════════════════

void ApplyTheme(const Theme& theme)
{
    g_theme = theme;   // сохраняем как активную

    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c     = s.Colors;
    const Theme& t = g_theme;

    // ── Геометрия ───────────────────────────────────────────────
    s.WindowPadding     = {14.f, 12.f};
    s.FramePadding      = {10.f,  6.f};
    s.ItemSpacing       = { 8.f,  7.f};
    s.ItemInnerSpacing  = { 6.f,  4.f};
    s.IndentSpacing     = 18.f;
    s.ScrollbarSize     = 10.f;
    s.GrabMinSize       = 10.f;

    s.WindowRounding    = t.rounding;
    s.ChildRounding     = t.rounding;
    s.FrameRounding     = t.rounding;
    s.PopupRounding     = t.rounding;
    s.ScrollbarRounding = t.rounding;
    s.GrabRounding      = t.rounding;
    s.TabRounding       = t.rounding;
    s.WindowBorderSize  = t.windowBorder;
    s.FrameBorderSize   = t.frameBorder;
    s.PopupBorderSize   = t.windowBorder;

    // ── Вспомогательные вычисляемые цвета ───────────────────────
    auto withAlpha = [](ImVec4 col, float a) -> ImVec4 {
        return {col.x, col.y, col.z, a};
    };
    auto brighter = [](ImVec4 col, float k) -> ImVec4 {
        return {
            col.x + (1.f - col.x) * k,
            col.y + (1.f - col.y) * k,
            col.z + (1.f - col.z) * k,
            col.w
        };
    };

    const ImVec4 accentDim  = withAlpha(t.accent,  0.55f);
    const ImVec4 accentH    = brighter(t.accent,   0.15f);
    const ImVec4 border     = withAlpha(t.accent,  0.30f);
    const ImVec4 borderH    = withAlpha(t.accent,  0.80f);

    // ── Цвета ────────────────────────────────────────────────────
    c[ImGuiCol_Text]                  = t.textMain;
    c[ImGuiCol_TextDisabled]          = t.textDim;
    c[ImGuiCol_TextSelectedBg]        = withAlpha(t.accent, 0.25f);

    c[ImGuiCol_WindowBg]              = t.bgBase;
    c[ImGuiCol_ChildBg]               = t.bgDeep;
    c[ImGuiCol_PopupBg]               = {t.bgDeep.x+0.01f, t.bgDeep.y+0.01f, t.bgDeep.z+0.02f, 0.97f};

    c[ImGuiCol_Border]                = border;
    c[ImGuiCol_BorderShadow]          = {0,0,0,0};

    c[ImGuiCol_FrameBg]               = t.bgFrame;
    c[ImGuiCol_FrameBgHovered]        = t.bgMid;
    c[ImGuiCol_FrameBgActive]         = t.bgHigh;

    c[ImGuiCol_TitleBg]               = t.bgDeep;
    c[ImGuiCol_TitleBgActive]         = t.bgMid;
    c[ImGuiCol_TitleBgCollapsed]      = t.bgDeep;

    c[ImGuiCol_MenuBarBg]             = t.bgDeep;

    c[ImGuiCol_ScrollbarBg]           = t.bgDeep;
    c[ImGuiCol_ScrollbarGrab]         = withAlpha(t.textDim, 0.8f);
    c[ImGuiCol_ScrollbarGrabHovered]  = accentDim;
    c[ImGuiCol_ScrollbarGrabActive]   = t.accent;

    c[ImGuiCol_CheckMark]             = t.accent;

    c[ImGuiCol_SliderGrab]            = t.accent;
    c[ImGuiCol_SliderGrabActive]      = accentH;

    c[ImGuiCol_Button]                = t.bgMid;
    c[ImGuiCol_ButtonHovered]         = withAlpha(t.accent, 0.15f);
    c[ImGuiCol_ButtonActive]          = withAlpha(t.accent, 0.30f);

    c[ImGuiCol_Header]                = withAlpha(t.accent, 0.12f);
    c[ImGuiCol_HeaderHovered]         = withAlpha(t.accent, 0.22f);
    c[ImGuiCol_HeaderActive]          = withAlpha(t.accent, 0.35f);

    c[ImGuiCol_Separator]             = withAlpha(t.accent, 0.25f);
    c[ImGuiCol_SeparatorHovered]      = accentDim;
    c[ImGuiCol_SeparatorActive]       = t.accent;

    c[ImGuiCol_ResizeGrip]            = accentDim;
    c[ImGuiCol_ResizeGripHovered]     = t.accent;
    c[ImGuiCol_ResizeGripActive]      = accentH;

    c[ImGuiCol_Tab]                   = t.bgBase;
    c[ImGuiCol_TabHovered]            = withAlpha(t.accent, 0.25f);
    c[ImGuiCol_TabActive]             = t.bgMid;
    c[ImGuiCol_TabUnfocused]          = t.bgDeep;
    c[ImGuiCol_TabUnfocusedActive]    = t.bgMid;

    c[ImGuiCol_TableHeaderBg]         = t.bgMid;
    c[ImGuiCol_TableBorderStrong]     = borderH;
    c[ImGuiCol_TableBorderLight]      = border;
    c[ImGuiCol_TableRowBg]            = {0,0,0,0};
    c[ImGuiCol_TableRowBgAlt]         = {1,1,1,0.03f};

    c[ImGuiCol_DragDropTarget]        = t.accent;
    c[ImGuiCol_NavHighlight]          = t.accent;
    c[ImGuiCol_NavWindowingHighlight] = withAlpha(t.accent, 0.70f);
    c[ImGuiCol_NavWindowingDimBg]     = withAlpha(t.bgDeep, 0.70f);
    c[ImGuiCol_ModalWindowDimBg]      = withAlpha(t.bgDeep, 0.75f);
}

// ════════════════════════════════════════════════════════════════════
//  ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ════════════════════════════════════════════════════════════════════

static ImU32 ToU32(ImVec4 c)
{
    return ImGui::ColorConvertFloat4ToU32(c);
}

static void DrawNeonUnderline(ImVec4 color, float thickness = 1.5f)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    dl->AddLine({min.x, max.y}, {max.x, max.y}, ToU32(color), thickness);
}

// ════════════════════════════════════════════════════════════════════
//  ОВЕРЛЕЙ
// ════════════════════════════════════════════════════════════════════

void DarkOverlay(float alpha)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   {0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::Begin("##overlay", nullptr,
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize      |
        ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoScrollbar   |
        ImGuiWindowFlags_NoInputs     | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNav        | ImGuiWindowFlags_NoDecoration  |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::End();
    ImGui::PopStyleVar(2);
}

// ════════════════════════════════════════════════════════════════════
//  ГЛАВНОЕ МЕНЮ
// ════════════════════════════════════════════════════════════════════

bool BeginMainMenu(const char* id)
{
    const Theme& t = g_theme;
    ImGuiIO& io    = ImGui::GetIO();
    const float W  = 340.f;

    ImGui::SetNextWindowPos({(io.DisplaySize.x - W) * 0.5f,
                             io.DisplaySize.y * 0.22f}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({W, 0.f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.96f);

    ImGui::PushStyleColor(ImGuiCol_Border,     {t.accent.x,t.accent.y,t.accent.z,0.85f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   {24.f, 22.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,     { 8.f, 10.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);

    bool open = ImGui::Begin(id, nullptr,
        ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize  |
        ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_NoSavedSettings);

    // Декоративная полоска сверху
    if (t.topBarHeight > 0.f) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wpos    = ImGui::GetWindowPos();
        float  ww      = ImGui::GetWindowWidth();
        dl->AddRectFilled(wpos, {wpos.x + ww, wpos.y + t.topBarHeight},
                          ToU32(t.topBarColor));
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
    return open;
}

void EndMainMenu() { ImGui::End(); }

// ════════════════════════════════════════════════════════════════════
//  ФИКСИРОВАННОЕ ОКНО
// ════════════════════════════════════════════════════════════════════

bool BeginFixed(const char* label, float x, float y, float w, float h,
                ImGuiWindowFlags extra_flags)
{
    ImGui::SetNextWindowPos({x, y}, ImGuiCond_Always);
    if (w > 0 && h > 0)
        ImGui::SetNextWindowSize({w, h}, ImGuiCond_Always);
    return ImGui::Begin(label, nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        extra_flags);
}

void End() { ImGui::End(); }

// ════════════════════════════════════════════════════════════════════
//  ТЕКСТ
// ════════════════════════════════════════════════════════════════════

void Title(const char* text)
{
    const Theme& t = g_theme;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.f);

    float textW = ImGui::CalcTextSize(text).x;
    float winW  = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (winW - textW) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, t.accent);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();

    // Двойное неон-подчёркивание
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 min     = ImGui::GetItemRectMin();
        ImVec2 max     = ImGui::GetItemRectMax();
        float  ox      = ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x;
        float  ex      = ox + ImGui::GetContentRegionAvail().x + ImGui::GetStyle().WindowPadding.x;
        dl->AddLine({ox, max.y + 4.f}, {ex, max.y + 4.f},
                    ToU32({t.accent.x, t.accent.y, t.accent.z, 0.75f}), 1.f);
        dl->AddLine({ox, max.y + 6.f}, {ex, max.y + 6.f},
                    ToU32({t.accent.x, t.accent.y, t.accent.z, 0.25f}), 1.f);
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
}

void Header(const char* text)
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
    ImGui::PushStyleColor(ImGuiCol_Text, g_theme.accent);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    DrawNeonUnderline({g_theme.accent.x, g_theme.accent.y, g_theme.accent.z, 0.5f});
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
}

void Label(const char* fmt, ...)
{
    char buf[512];
    va_list args; va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ImGui::TextUnformatted(buf);
}

void LabelColored(ImVec4 color, const char* fmt, ...)
{
    char buf[512];
    va_list args; va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ImGui::TextColored(color, "%s", buf);
}

void ReadOnly(const char* label, const char* value)
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, g_theme.bgDeep);
    ImGui::PushStyleColor(ImGuiCol_Text,    g_theme.textDim);
    char buf[256];
    std::strncpy(buf, value, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    ImGui::InputText(label, buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor(2);
}

// ════════════════════════════════════════════════════════════════════
//  РАЗДЕЛИТЕЛИ
// ════════════════════════════════════════════════════════════════════

void Separator()
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
}

void SameLine(float spacing)
{
    spacing < 0.f ? ImGui::SameLine() : ImGui::SameLine(0.f, spacing);
}

// ════════════════════════════════════════════════════════════════════
//  КНОПКИ
// ════════════════════════════════════════════════════════════════════

bool Button(const char* label)
{
    const Theme& t = g_theme;
    float w = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleColor(ImGuiCol_Button,        t.bgDeep);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {t.accent.x,t.accent.y,t.accent.z,0.18f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {t.accent.x,t.accent.y,t.accent.z,0.35f});
    ImGui::PushStyleColor(ImGuiCol_Text,          t.accent);
    ImGui::PushStyleColor(ImGuiCol_Border,        {t.accent.x,t.accent.y,t.accent.z,0.55f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   t.rounding);

    bool pressed = ImGui::Button(label, {w, t.buttonHeight});

    if (t.buttonNeonEdge && ImGui::IsItemHovered()) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        dl->AddRect(min, max, ToU32({t.accent.x,t.accent.y,t.accent.z,0.80f}),
                    t.rounding, 0, 1.5f);
        dl->AddLine({min.x, max.y - 1.f}, {max.x, max.y - 1.f},
                    ToU32(t.accent), 2.f);
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);
    return pressed;
}

bool SmallButton(const char* label)
{
    const Theme& t = g_theme;
    ImGui::PushStyleColor(ImGuiCol_Button,        t.bgMid);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {t.accent.x,t.accent.y,t.accent.z,0.20f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {t.accent.x,t.accent.y,t.accent.z,0.38f});
    ImGui::PushStyleColor(ImGuiCol_Text,          {t.accent.x,t.accent.y,t.accent.z,0.90f});
    ImGui::PushStyleColor(ImGuiCol_Border,        {t.accent.x,t.accent.y,t.accent.z,0.40f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    bool pressed = ImGui::SmallButton(label);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
    return pressed;
}

bool IconButton(const char* icon_label)
{
    const Theme& t = g_theme;
    ImGui::PushStyleColor(ImGuiCol_Button,        {0,0,0,0});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {t.accent2.x,t.accent2.y,t.accent2.z,0.20f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {t.accent2.x,t.accent2.y,t.accent2.z,0.40f});
    ImGui::PushStyleColor(ImGuiCol_Text,          t.accent2);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    float sz     = ImGui::GetFrameHeight();
    bool pressed = ImGui::Button(icon_label, {sz, sz});
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    return pressed;
}

// ════════════════════════════════════════════════════════════════════
//  СЕКЦИИ
// ════════════════════════════════════════════════════════════════════

bool Section(const char* label)
{
    const Theme& t = g_theme;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

    ImGui::PushStyleColor(ImGuiCol_Header,        {t.accent2.x,t.accent2.y,t.accent2.z,0.10f});
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {t.accent2.x,t.accent2.y,t.accent2.z,0.20f});
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  {t.accent2.x,t.accent2.y,t.accent2.z,0.30f});
    ImGui::PushStyleColor(ImGuiCol_Text,          t.accent2);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    bool open = ImGui::TreeNodeEx(label,
        ImGuiTreeNodeFlags_DefaultOpen    |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_Framed         |
        ImGuiTreeNodeFlags_NoTreePushOnOpen);

    if (t.sectionLeftBar) {
        ImVec2 imin = ImGui::GetItemRectMin();
        ImVec2 imax = ImGui::GetItemRectMax();
        dl->AddRectFilled({imin.x, imin.y}, {imin.x + 3.f, imax.y},
                          ToU32({t.accent2.x, t.accent2.y, t.accent2.z, 0.90f}));
    }

    ImGui::PopStyleColor(4);
    return open;
}

// ════════════════════════════════════════════════════════════════════
//  КОНТРОЛЫ
// ════════════════════════════════════════════════════════════════════

bool Bool(const char* label, bool& value)
{
    const Theme& t = g_theme;
    ImGui::PushStyleColor(ImGuiCol_CheckMark,      t.accent);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  {t.accent.x,t.accent.y,t.accent.z,0.30f});
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {t.accent.x,t.accent.y,t.accent.z,0.15f});
    bool changed = ImGui::Checkbox(label, &value);
    ImGui::PopStyleColor(3);
    return changed;
}

bool SliderFloat(const char* label, float& value, float vmin, float vmax)
{
    const Theme& t = g_theme;
    ImVec4 accentH = {t.accent.x+(1-t.accent.x)*.15f,
                      t.accent.y+(1-t.accent.y)*.15f,
                      t.accent.z+(1-t.accent.z)*.15f, 1.f};
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,       t.accent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, accentH);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          t.bgFrame);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   t.bgMid);
    bool changed = ImGui::SliderFloat(label, &value, vmin, vmax);
    ImGui::PopStyleColor(4);
    return changed;
}

bool SliderInt(const char* label, int& value, int vmin, int vmax)
{
    const Theme& t = g_theme;
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,       t.accent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, t.accent);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          t.bgFrame);
    bool changed = ImGui::SliderInt(label, &value, vmin, vmax);
    ImGui::PopStyleColor(3);
    return changed;
}

bool InputFloat(const char* label, float& value, float step)
{
    return ImGui::InputFloat(label, &value, step);
}

bool InputText(const char* label, char* buf, size_t buf_size)
{
    return ImGui::InputText(label, buf, buf_size);
}

bool Vec3(const char* label, glm::vec3& v, float speed)
{
    float tmp[3] = {v.x, v.y, v.z};
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        g_theme.bgFrame);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, g_theme.bgMid);
    bool changed = ImGui::DragFloat3(label, tmp, speed);
    ImGui::PopStyleColor(2);
    if (changed) { v.x = tmp[0]; v.y = tmp[1]; v.z = tmp[2]; }
    return changed;
}

bool Color3(const char* label, glm::vec3& color)
{
    float tmp[3] = {color.r, color.g, color.b};
    bool changed = ImGui::ColorEdit3(label, tmp,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);
    if (changed) { color.r = tmp[0]; color.g = tmp[1]; color.b = tmp[2]; }
    return changed;
}

bool Dropdown(const char* label, int& current, const char* const items[], int count)
{
    const Theme& t = g_theme;
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        t.bgFrame);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, t.bgMid);
    ImGui::PushStyleColor(ImGuiCol_PopupBg,        t.bgDeep);
    ImGui::PushStyleColor(ImGuiCol_Header,         {t.accent.x,t.accent.y,t.accent.z,0.18f});
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  {t.accent.x,t.accent.y,t.accent.z,0.28f});

    const char* preview = (current >= 0 && current < count) ? items[current] : "---";
    bool changed = false;
    if (ImGui::BeginCombo(label, preview)) {
        for (int i = 0; i < count; i++) {
            bool sel = (i == current);
            if (ImGui::Selectable(items[i], sel)) { current = i; changed = true; }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleColor(5);
    return changed;
}

// ════════════════════════════════════════════════════════════════════
//  ТАБЛИЦЫ
// ════════════════════════════════════════════════════════════════════

bool BeginTable(const char* id, int columns, ImGuiTableFlags flags)
{
    return ImGui::BeginTable(id, columns, flags);
}
void EndTable()        { ImGui::EndTable(); }
void TableNextRow()    { ImGui::TableNextRow(); }
void TableNextColumn() { ImGui::TableNextColumn(); }

// ════════════════════════════════════════════════════════════════════
//  ВКЛАДКИ
// ════════════════════════════════════════════════════════════════════

bool BeginTabBar(const char* id)
{
    const Theme& t = g_theme;
    ImGui::PushStyleColor(ImGuiCol_Tab,          t.bgBase);
    ImGui::PushStyleColor(ImGuiCol_TabHovered,   {t.accent.x,t.accent.y,t.accent.z,0.25f});
    ImGui::PushStyleColor(ImGuiCol_TabActive,    t.bgMid);
    ImGui::PushStyleColor(ImGuiCol_TabUnfocused, t.bgDeep);
    bool open = ImGui::BeginTabBar(id);
    ImGui::PopStyleColor(4);
    return open;
}
void EndTabBar()                    { ImGui::EndTabBar(); }
bool BeginTab(const char* label)    { return ImGui::BeginTabItem(label); }
void EndTab()                       { ImGui::EndTabItem(); }

// ════════════════════════════════════════════════════════════════════
//  ДОПОЛНИТЕЛЬНЫЕ ФУНКЦИИ (обратная совместимость с ImGuiLayer)
// ════════════════════════════════════════════════════════════════════

void Tooltip(const char* text)
{
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushStyleColor(ImGuiCol_Text, g_theme.textMain);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        ImGui::EndTooltip();
    }
}

void Spacing()  { ImGui::Spacing(); }
void NewLine()  { ImGui::NewLine(); }

bool ButtonW(const char* label, float width, float height)
{
    const Theme& t = g_theme;
    ImGui::PushStyleColor(ImGuiCol_Button,        t.bgMid);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {t.accent.x,t.accent.y,t.accent.z,0.18f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {t.accent.x,t.accent.y,t.accent.z,0.35f});
    ImGui::PushStyleColor(ImGuiCol_Text,          t.accent);
    ImGui::PushStyleColor(ImGuiCol_Border,        {t.accent.x,t.accent.y,t.accent.z,0.45f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    bool pressed = ImGui::Button(label, {width, height});
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
    return pressed;
}

bool Float(const char* label, float& v, float step, float stepFast)
{
    return ImGui::InputFloat(label, &v, step, stepFast);
}

bool Int(const char* label, int& v, int min, int max)
{
    if (min == 0 && max == 0)
        return ImGui::InputInt(label, &v);
    return ImGui::SliderInt(label, &v, min, max);
}

bool Text(const char* label, char* buf, size_t bufSize)
{
    return ImGui::InputText(label, buf, bufSize);
}

bool Color4(const char* label, glm::vec4& color)
{
    float tmp[4] = {color.r, color.g, color.b, color.a};
    bool changed = ImGui::ColorEdit4(label, tmp,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);
    if (changed) { color.r = tmp[0]; color.g = tmp[1]; color.b = tmp[2]; color.a = tmp[3]; }
    return changed;
}

bool BeginMenuBar() { return ImGui::BeginMenuBar(); }
void EndMenuBar()   { ImGui::EndMenuBar(); }
bool BeginMenu(const char* label) { return ImGui::BeginMenu(label); }
void EndMenu()      { ImGui::EndMenu(); }
bool MenuItem(const char* label, const char* shortcut)
{
    return ImGui::MenuItem(label, shortcut);
}

void OpenPopup(const char* id)  { ImGui::OpenPopup(id); }
bool BeginPopup(const char* id)
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_theme.bgDeep);
    bool open = ImGui::BeginPopup(id);
    if (!open) ImGui::PopStyleColor();
    return open;
}
void EndPopup()   { ImGui::PopStyleColor(); ImGui::EndPopup(); }
void ClosePopup() { ImGui::CloseCurrentPopup(); }

void ProgressBar(float fraction, const char* overlay)
{
    const Theme& t = g_theme;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, t.accent);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       t.bgFrame);
    ImGui::ProgressBar(fraction, {-1, 0}, overlay);
    ImGui::PopStyleColor(2);
}

void EnableDockspace()
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::Begin("##DockSpace", nullptr,
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar(3);
    ImGui::DockSpace(ImGui::GetID("MainDockspace"), {0,0},
        ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

} // namespace UI
} // namespace eng
