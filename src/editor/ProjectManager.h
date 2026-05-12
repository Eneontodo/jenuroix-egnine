#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  ProjectManager.h  —  Save / Load project, Menu Bar
//
//  Сохраняет сцену в JSON файл (.jenuroix) и загружает обратно.
//  Рисует верхний Menu Bar: File | Edit | View | Help
// ─────────────────────────────────────────────────────────────────────────────
#include "engine.h"
#include <string>

namespace eng {

class ProjectManager {
public:
    explicit ProjectManager(App& app);

    // Вызывать в onRender — рисует menu bar поверх всего
    void drawMenuBar();

    // Optional callback: called when user picks File > New Scene
    // Connect to SceneEditor::sceneTabs_newTab()
    void onNewScene(std::function<void()> cb) { m_onNewScene = cb; }

    // Сохранить проект
    bool save(const std::string& path);
    bool saveAs();   // открывает диалог (упрощённо — просит имя)

    // Загрузить проект
    bool load(const std::string& path);

    // Текущий путь к проекту
    const std::string& currentPath() const { return m_path; }
    bool hasUnsavedChanges() const { return m_dirty; }
    void markDirty() { m_dirty = true; }

private:
    App&        m_app;
    World&      m_world;
    std::string m_path;
    bool        m_dirty      = false;
    bool        m_showAbout  = false;
    bool        m_showSaveAs = false;
    char        m_saveAsBuf[512] = "project.jenuroix";
    std::function<void()> m_onNewScene;

    // Serialize / deserialize scene
    std::string serializeScene()  const;
    bool        parseScene(const std::string& json);

    // Helpers
    std::string escapeJson(const std::string& s) const;
};

} // namespace eng
