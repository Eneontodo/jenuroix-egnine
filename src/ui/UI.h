#pragma once
// ════════════════════════════════════════════════════════════════════
//  UI.h  —  Engine UI system with custom theme support
//
//  Quick start:
//      UI::ApplyTheme(UI::Themes::Cyberpunk());   // built-in theme
//      UI::ApplyTheme(UI::Themes::Dark());        // classic dark
//      UI::ApplyTheme(UI::Themes::Light());       // light editor style
//
//  Custom theme:
//      UI::Theme t;
//      t.accent        = {1.f, 0.3f, 0.f, 1.f};  // orange accent
//      t.accent2       = {1.f, 0.8f, 0.f, 1.f};  // yellow secondary
//      t.bgBase        = {0.1f,0.08f,0.06f,1.f};  // warm dark background
//      t.rounding      = 4.f;                      // rounded corners
//      t.topBarColor   = t.accent;
//      UI::ApplyTheme(t);
//
//  Switch theme at runtime:
//      UI::ApplyTheme(newTheme);  // call at any time
// ════════════════════════════════════════════════════════════════════
#include <imgui.h>
#include <string>
#include <glm/glm.hpp>

namespace eng {
namespace UI {

// ════════════════════════════════════════════════════════════════════
//  THEME STRUCTURE
// ════════════════════════════════════════════════════════════════════
struct Theme {
    // ── Background colors ─────────────────────────────────────────
    ImVec4 bgDeep    = {0.06f,0.06f,0.08f,1.f};  ///< Darkest (child windows)
    ImVec4 bgBase    = {0.09f,0.09f,0.12f,1.f};  ///< Main window background
    ImVec4 bgMid     = {0.12f,0.12f,0.16f,1.f};  ///< Hover background
    ImVec4 bgHigh    = {0.16f,0.16f,0.22f,1.f};  ///< Active background
    ImVec4 bgFrame   = {0.10f,0.10f,0.14f,1.f};  ///< Input field background

    // ── Accent colors ─────────────────────────────────────────────
    ImVec4 accent    = {1.f,0.85f,0.f,1.f};      ///< Primary accent
    ImVec4 accent2   = {0.f,0.95f,1.f,1.f};      ///< Secondary accent (sections)
    ImVec4 danger    = {1.f,0.10f,0.20f,1.f};    ///< Danger / destructive

    // ── Text ──────────────────────────────────────────────────────
    ImVec4 textMain  = {0.95f,0.92f,0.85f,1.f};  ///< Primary text
    ImVec4 textDim   = {0.55f,0.53f,0.48f,1.f};  ///< Secondary/hints

    // ── Widget style ──────────────────────────────────────────────
    float  rounding       = 0.f;   ///< Rounding (0 = sharp, 4+ = soft)
    float  windowBorder   = 1.f;   ///< Window border thickness
    float  frameBorder    = 1.f;   ///< Frame border thickness
    float  buttonHeight   = 36.f;  ///< Large button height

    // ── Decorative elements ──────────────────────────────────────
    ImVec4 topBarColor    = {1.f,0.85f,0.f,1.f}; ///< Top decorative bar color
    float  topBarHeight   = 3.f;   ///< Top bar height (0 = disabled)
    bool   sectionLeftBar = true;  ///< Left bar on Section()
    bool   buttonNeonEdge = true;  ///< Bottom neon edge on Button() hover
};

// ════════════════════════════════════════════════════════════════════
//  BUILT-IN THEMES
// ════════════════════════════════════════════════════════════════════
namespace Themes {
    Theme Cyberpunk();  ///< Yellow / cyan, sharp edges, neon
    Theme Dark();       ///< Purple accent, soft corners
    Theme Light();      ///< Light, blue accent (Unity-style)
    Theme Blood();      ///< Red / orange, dark charcoal background
    Theme Matrix();     ///< Green / black, monochrome
} // namespace Themes

// ════════════════════════════════════════════════════════════════════
//  APPLY AND ACCESS
// ════════════════════════════════════════════════════════════════════

/// Applies the theme to all of ImGui. Can be called at runtime.
void ApplyTheme(const Theme& theme);

/// Returns the currently active theme.
const Theme& GetTheme();

/// Backwards compatibility: ApplyTheme() with no args = Cyberpunk
inline void ApplyTheme() { ApplyTheme(Themes::Cyberpunk()); }

// ════════════════════════════════════════════════════════════════════
//  COLORS
// ════════════════════════════════════════════════════════════════════

// Dynamic — taken from active theme:
inline ImVec4 ACCENT()   { return GetTheme().accent;  }
inline ImVec4 ACCENT2()  { return GetTheme().accent2; }
inline ImVec4 DANGER()   { return GetTheme().danger;  }

// Static utility colors:
inline constexpr ImVec4 WHITE   = {1.f,  1.f,  1.f,  1.f};
inline constexpr ImVec4 GRAY    = {0.55f,0.55f,0.60f,1.f};
inline constexpr ImVec4 BLACK   = {0.f,  0.f,  0.f,  1.f};
// For LabelColored and backwards compatibility:
inline constexpr ImVec4 YELLOW  = {1.f,  0.85f,0.f,  1.f};
inline constexpr ImVec4 CYAN    = {0.f,  0.95f,1.f,  1.f};
inline constexpr ImVec4 RED     = {1.f,  0.10f,0.20f,1.f};
inline constexpr ImVec4 GREEN   = {0.20f,1.f,  0.50f,1.f};
inline constexpr ImVec4 ORANGE  = {1.f,  0.55f,0.05f,1.f};
inline constexpr ImVec4 MAGENTA = {1.f,  0.20f,0.80f,1.f};

// ════════════════════════════════════════════════════════════════════
//  WIDGETS  (API fully compatible with previous version)
// ════════════════════════════════════════════════════════════════════

void DarkOverlay(float alpha = 0.6f);

bool BeginMainMenu(const char* id);
void EndMainMenu();

bool BeginFixed(const char* label, float x, float y, float w, float h,
                ImGuiWindowFlags extra_flags = 0);
void End();

void Title(const char* text);
void Header(const char* text);
void Label(const char* fmt, ...);
void LabelColored(ImVec4 color, const char* fmt, ...);
void ReadOnly(const char* label, const char* value);

void Separator();
void SameLine(float spacing = -1.f);

bool Button(const char* label);
bool SmallButton(const char* label);
bool IconButton(const char* icon_label);

bool Section(const char* label);

bool Bool(const char* label, bool& value);
bool SliderFloat(const char* label, float& value, float vmin, float vmax);
bool SliderInt(const char* label, int& value, int vmin, int vmax);
bool InputFloat(const char* label, float& value, float step = 0.1f);
bool InputText(const char* label, char* buf, size_t buf_size);
bool Vec3(const char* label, glm::vec3& v, float speed = 0.1f);
bool Color3(const char* label, glm::vec3& color);
bool Dropdown(const char* label, int& current, const char* const items[], int count);

bool BeginTable(const char* id, int columns,
                ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH |
                                        ImGuiTableFlags_RowBg);
void EndTable();
void TableNextRow();
void TableNextColumn();

bool BeginTabBar(const char* id);
void EndTabBar();
bool BeginTab(const char* label);
void EndTab();

void Tooltip(const char* text);

void Spacing();
void NewLine();

bool ButtonW(const char* label, float width = 120.f, float height = 28.f);
bool Float(const char* label, float& v, float step = 0.1f, float stepFast = 1.f);
bool Int(const char* label, int& v, int min = 0, int max = 0);
bool Text(const char* label, char* buf, size_t bufSize);
bool Color4(const char* label, glm::vec4& color);

bool BeginMenuBar();
void EndMenuBar();
bool BeginMenu(const char* label);
void EndMenu();
bool MenuItem(const char* label, const char* shortcut = nullptr);

void OpenPopup(const char* id);
bool BeginPopup(const char* id);
void EndPopup();
void ClosePopup();

void ProgressBar(float fraction, const char* overlay = nullptr);
void EnableDockspace();

} // namespace UI
} // namespace eng
