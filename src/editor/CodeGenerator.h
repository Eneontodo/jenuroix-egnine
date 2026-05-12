#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  CodeGenerator.h  —  Generates main.cpp from the current scene
//
//  Панели:
//    Code     — просмотр и редактирование сгенерированного main.cpp
//    Compile  — компилировать + запустить
//    Files    — экспортировать / импортировать main.cpp
//
//  Live preview: при правке кода в редакторе — через kLiveMs мс сцена
//  обновляется (парсятся spawn/cube/sphere/pos/color/scale/rot/model/addRigidBody).
//  Скрипты (onUpdate/onStart/etc.) требуют компиляции — в сцене не отражаются.
// ─────────────────────────────────────────────────────────────────────────────
#include "engine.h"
#include "editor/SceneEditor.h"
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

namespace eng {

class CodeGenerator {
public:
    explicit CodeGenerator(App& app);
    ~CodeGenerator();

    // Вызывать каждый кадр внутри onRender
    void drawPanel();

    // Provide access to UI Scene widgets so they appear in generated code
    void setUIWidgets(const std::vector<UIWidget>* widgets) { m_uiWidgets = widgets; }

    // Provide all scene tabs for multi-scene code generation
    void setSceneTabs(const std::vector<SceneTab>* tabs,
                      const SceneSnapshot*         activeSnap,
                      int                          activeIndex)
    {
        m_tabs        = tabs;
        m_activeSnap  = activeSnap;
        m_activeIndex = activeIndex;
    }

    // Сгенерировать код из текущей сцены
    std::string generate() const;

    // Сгенерировать multi-scene main.cpp (used when >1 tab)
    std::string generateMultiScene() const;

    // Скомпилировать m_codeBuf (записывает main.cpp + запускает build)
    // ВНИМАНИЕ: вызывается из фонового потока
    bool compile(std::string& outLog);

    // Запустить скомпилированный exe
    bool run(std::string& outLog);

    // Экспорт m_codeBuf на диск
    bool exportTo(const std::string& path) const;

    // Импорт main.cpp с диска
    bool importFrom(const std::string& path);

    // Очистить сцену до пустой
    void clearScene();

    // Разобрать код в m_codeBuf и применить к сцене (live preview).
    // Парсит: spawn, cube/sphere/quad/plane/cylinder/model,
    //         pos/rot/scale, color/metallic/roughness,
    //         addRigidBody(Static/Dynamic/Kinematic, Box/Sphere/Capsule)
    // НЕ парсит: скрипты onUpdate/onStart — нужна компиляция
    void applyCodeToScene();

private:
    App&        m_app;
    World&      m_world;

    // Panel state
    bool        m_open       = true;
    int         m_tab        = 0;
    char*       m_codeBuf    = nullptr;
    int         m_codeBufSz  = 512 * 1024;
    bool        m_dirty      = true;

    // onRender UI block — редактируется отдельно, не перезаписывается Regenerate
    char*       m_renderBuf  = nullptr;
    int         m_renderBufSz = 64 * 1024;
    const std::vector<UIWidget>*  m_uiWidgets   = nullptr;
    const std::vector<SceneTab>*  m_tabs        = nullptr;
    const SceneSnapshot*          m_activeSnap  = nullptr;
    int                           m_activeIndex = 0;
    bool        m_renderDirty = false;

    // Build state (доступ из потока — под мьютексом)
    std::string         m_compileLog;
    std::mutex          m_logMutex;
    std::atomic<bool>   m_building{false};
    std::atomic<bool>   m_compiled{false};
    std::thread         m_buildThread;

    // Live preview debounce
    using Clock = std::chrono::steady_clock;
    Clock::time_point   m_lastEditTime{};
    bool                m_pendingApply  = false;
    static constexpr int kLiveMs        = 800; // мс тишины → применяем к сцене

    // Live preview status
    std::string m_liveStatus; // "" | "✓ N objects applied" | "⚠ ..."

    // File dialog
    char        m_exportPath[512] = "exported_main.cpp";
    char        m_importPath[512] = "";

    // Internal
    void        regenerateIfDirty();
    void        startBuildAsync();
    void        tickLivePreview();   // вызывается каждый кадр из drawPanel

    std::string buildCommand() const;
    std::string runCommand()   const;

    // Code generation helpers
    std::string genHeader    ()            const;
    std::string genAppSetup  ()            const;
    std::string genObjects   ()            const;
    std::string genObject    (EntityID id) const;
    std::string genRigidBody (EntityID id) const;
    std::string genRenderBlock()           const;
    std::string genFooter    ()            const;

    // Low-level parsing helpers
    static void        skipWS      (const std::string& s, size_t& p);
    static float       parseFloat  (const std::string& s, size_t& p);
    static std::string parseString (const std::string& s, size_t& p); // "..."
    static bool        skipTo      (const std::string& s, size_t& p, char c);

    // Misc string helpers
    std::string tagName (EntityID id) const;
    std::string varName (EntityID id) const;
    std::string col3    (glm::vec3 c) const;
    std::string vec3    (glm::vec3 v) const;
    std::string flt     (float f)     const;
};

} // namespace eng
