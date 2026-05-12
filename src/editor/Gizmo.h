#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Gizmo.h  —  3D transform handles (Translate / Rotate / Scale)
//
//  Рисует стрелки/дуги/кубики прямо во вьюпорте.
//  Поддерживает перетаскивание мышью + клавишные шорткаты X/Y/Z для lock оси.
//
//  Подключение:
//    Gizmo::draw(app, selectedEntityId, gizmoMode);
//    // вызывать ПОСЛЕ renderScene(), ДО ImGui::Render()
//
//  Режимы:
//    GizmoMode::Translate  — стрелки по осям (красная=X, зелёная=Y, синяя=Z)
//    GizmoMode::Rotate     — дуги (кольца) по осям
//    GizmoMode::Scale      — кубики на концах осей
// ─────────────────────────────────────────────────────────────────────────────

#include <glm/glm.hpp>
#include <cstdint>

namespace eng { class App; }
using EntityID = uint32_t;

enum class GizmoMode { Translate, Rotate, Scale };

class Gizmo {
public:
    // Главный метод — вызывать каждый кадр в onRender
    static bool draw(eng::App&   app,
                     EntityID    selectedEntity,
                     GizmoMode   mode,
                     bool        localSpace = false);

    // Возвращает true если гизмо сейчас перетаскивается
    static bool isDragging();

    // Сброс состояния (при смене выбранного объекта)
    static void reset();
};
