#pragma once
// ═══════════════════════════════════════════════════════════════════════════════
//  ImGuiLayer.h  —  ImGui module for the engine
//  
//  Usage:
//    #include "ui/ImGuiLayer.h"
//    
//    ImGuiLayer ui(app.window());   // once in onStart
//
//    app.onRender([&](){
//        ui.begin();
//
//        if (UI::BeginMainMenu("Main Menu")) {
//            if (UI::Button("Play"))   state = GAME;
//            if (UI::Button("Quit"))   app.quit();
//        }
//
//        ui.end();
//    });
// ═══════════════════════════════════════════════════════════════════════════════

// ── Check ImGui is in vendor/ ─────────────────────────────────────────────────
#if __has_include(<imgui.h>)
  #include <imgui.h>
  #include <backends/imgui_impl_glfw.h>
  #include <backends/imgui_impl_opengl3.h>
  #define ENGINE_IMGUI_AVAILABLE 1
#else
  #error "ImGui not found in vendor/imgui/. Download from https://github.com/ocornut/imgui and put in vendor/imgui/"
#endif

#include "core/Window.h"
#include <string>
#include <functional>
#include <glm/glm.hpp>

// ─────────────────────────────────────────────────────────────────────────────
//  ImGuiLayer — init/begin/end lifecycle
// ─────────────────────────────────────────────────────────────────────────────
class ImGuiLayer {
public:
    ImGuiLayer() = default;

    explicit ImGuiLayer(Window& window) { init(window); }

    void init(Window& window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // ── Style: dark ───────────────────────────────────────
        applyStyle();

        ImGui_ImplGlfw_InitForOpenGL(window.handle(), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        // Загружаем шрифт с поддержкой кириллицы сразу при инициализации
        setFontSize(15.0f);

        m_ready = true;
    }

    ~ImGuiLayer() {
        if (!m_ready) return;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    // Call at the START of onRender, before your UI code
    void begin() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // Call at the END of onRender, after all UI code
    void end() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    bool ready() const { return m_ready; }

    // ── Font ──────────────────────────────────────────────────────────────
    // Call before init() or in onStart before first frame
    void setFontSize(float size) {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        // Собираем диапазон глифов: Latin + Cyrillic + прочие Unicode
        ImFontConfig cfg;
        cfg.SizePixels = size;

        // Пробуем загрузить системный шрифт с поддержкой кириллицы
        static const ImWchar ranges[] = {
            0x0020, 0x00FF, // Latin + Basic Latin Extended
            0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
            0x2000, 0x206F, // General Punctuation
            0x2DE0, 0x2DFF, // Cyrillic Extended-A
            0xA640, 0xA69F, // Cyrillic Extended-B
            0,
        };

        bool loaded = tryLoadSystemFont(io, size, ranges);

        if (!loaded) {
            // Fallback: дефолтный шрифт (ASCII only, но хотя бы не крашится)
            io.Fonts->AddFontDefault(&cfg);
        }

        io.Fonts->Build();
        ImGui_ImplOpenGL3_DestroyDeviceObjects();
        ImGui_ImplOpenGL3_CreateDeviceObjects();
    }

private:
    // Ищет системный TTF-шрифт с кириллицей и загружает его
    static bool tryLoadSystemFont(ImGuiIO& io, float size, const ImWchar* ranges) {
        // Список кандидатов: Windows, Linux (Ubuntu/Debian), macOS
        static const char* candidates[] = {
            // Windows
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/tahoma.ttf",
            "C:/Windows/Fonts/verdana.ttf",
            // Linux
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            // macOS
            "/System/Library/Fonts/Helvetica.ttc",
            "/System/Library/Fonts/Arial.ttf",
            nullptr
        };

        ImFontConfig cfg;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;

        for (int i = 0; candidates[i]; ++i) {
            if (FILE* f = fopen(candidates[i], "rb")) {
                fclose(f);
                ImFont* font = io.Fonts->AddFontFromFileTTF(candidates[i], size, &cfg, ranges);
                if (font) return true;
            }
        }
        return false;
    }

    bool m_ready = false;

    static void applyStyle() {
        ImGui::StyleColorsDark();
        ImGuiStyle& s = ImGui::GetStyle();

        // Roundness
        s.WindowRounding    = 4.f;
        s.FrameRounding     = 3.f;
        s.GrabRounding      = 3.f;
        s.PopupRounding     = 3.f;
        s.ScrollbarRounding = 3.f;
        s.TabRounding       = 3.f;

        // Spacing
        s.WindowPadding     = {10, 10};
        s.FramePadding      = {6, 4};
        s.ItemSpacing       = {8, 6};
        s.IndentSpacing     = 18.f;
        s.ScrollbarSize     = 13.f;
        s.GrabMinSize       = 10.f;

        // Colors (dark theme)
        auto* c = s.Colors;
        c[ImGuiCol_WindowBg]         = {0.192f, 0.192f, 0.192f, 1.f};
        c[ImGuiCol_ChildBg]          = {0.157f, 0.157f, 0.157f, 1.f};
        c[ImGuiCol_PopupBg]          = {0.18f,  0.18f,  0.18f,  0.95f};
        c[ImGuiCol_Border]           = {0.08f,  0.08f,  0.08f,  1.f};
        c[ImGuiCol_FrameBg]          = {0.11f,  0.11f,  0.11f,  1.f};
        c[ImGuiCol_FrameBgHovered]   = {0.20f,  0.20f,  0.20f,  1.f};
        c[ImGuiCol_FrameBgActive]    = {0.27f,  0.27f,  0.27f,  1.f};
        c[ImGuiCol_TitleBg]          = {0.10f,  0.10f,  0.10f,  1.f};
        c[ImGuiCol_TitleBgActive]    = {0.14f,  0.14f,  0.14f,  1.f};
        c[ImGuiCol_MenuBarBg]        = {0.14f,  0.14f,  0.14f,  1.f};
        c[ImGuiCol_Header]           = {0.22f,  0.22f,  0.22f,  1.f};
        c[ImGuiCol_HeaderHovered]    = {0.27f,  0.27f,  0.27f,  1.f};
        c[ImGuiCol_HeaderActive]     = {0.31f,  0.31f,  0.31f,  1.f};
        c[ImGuiCol_Button]           = {0.26f,  0.26f,  0.26f,  1.f};
        c[ImGuiCol_ButtonHovered]    = {0.34f,  0.34f,  0.34f,  1.f};
        c[ImGuiCol_ButtonActive]     = {0.18f,  0.45f,  0.78f,  1.f}; // blue
        c[ImGuiCol_Tab]              = {0.18f,  0.18f,  0.18f,  1.f};
        c[ImGuiCol_TabHovered]       = {0.27f,  0.27f,  0.27f,  1.f};
        c[ImGuiCol_TabActive]        = {0.22f,  0.22f,  0.22f,  1.f};
        c[ImGuiCol_SliderGrab]       = {0.26f,  0.59f,  0.98f,  0.8f};
        c[ImGuiCol_SliderGrabActive] = {0.26f,  0.59f,  0.98f,  1.f};
        c[ImGuiCol_CheckMark]        = {0.26f,  0.59f,  0.98f,  1.f};
        c[ImGuiCol_Separator]        = {0.10f,  0.10f,  0.10f,  1.f};
        c[ImGuiCol_Text]             = {0.86f,  0.86f,  0.86f,  1.f};
        c[ImGuiCol_TextDisabled]     = {0.50f,  0.50f,  0.50f,  1.f};
    }
};

