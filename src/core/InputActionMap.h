#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <GLFW/glfw3.h>
#include "core/Input.h"

// One-to-many mapping: action -> bindings (keys, mouse buttons, axes)
struct Binding {
    enum class Type { Key, MouseButton, MouseAxis };
    Type type   = Type::Key;
    int  code   = 0;    // GLFW_KEY_* или GLFW_MOUSE_BUTTON_*
    float scale = 1.0f; // для осей: инвертирование направления
};

// Map of actions to their bindings
//  Usage:
//    auto& im = InputActionMap::get();
//    im.bind("Jump",  Binding{Binding::Type::Key, GLFW_KEY_SPACE});
//    im.bind("Shoot", Binding{Binding::Type::MouseButton, GLFW_MOUSE_BUTTON_LEFT});
//
//    if (im.isPressed("Jump"))  { ... }
//    float x = im.axis("MoveRight");

class InputActionMap {
public:
    static InputActionMap& get() { static InputActionMap inst; return inst; }

    // Add / replace binding
    void bind(const std::string& action, Binding binding) {
        m_bindings[action].push_back(binding);
    }

    // Reset all bindings for an action
    void unbind(const std::string& action) {
        m_bindings.erase(action);
    }

    // Is any key for the action held down
    bool isDown(const std::string& action) const {
        for (auto& b : get_bindings(action))
            if (check_down(b)) return true;
        return false;
    }

    // Only pressed this frame
    bool isPressed(const std::string& action) const {
        for (auto& b : get_bindings(action))
            if (check_pressed(b)) return true;
        return false;
    }

    // Only released this frame
    bool isReleased(const std::string& action) const {
        for (auto& b : get_bindings(action))
            if (check_released(b)) return true;
        return false;
    }

    // Axis value: sum of scale of active bindings (usually −1..+1)
    float axis(const std::string& action) const {
        float v = 0.f;
        for (auto& b : get_bindings(action))
            if (check_down(b)) v += b.scale;
        return v;
    }

    // ─── Load default bindings ────────────────────────────────────────────
    // Typical set of "out of the box" bindings
    void loadDefaults() {
        using T = Binding::Type;
        // Movement
        bind("MoveForward",  {T::Key, GLFW_KEY_W,            1.f});
        bind("MoveForward",  {T::Key, GLFW_KEY_UP,           1.f});
        bind("MoveBackward", {T::Key, GLFW_KEY_S,           -1.f});
        bind("MoveBackward", {T::Key, GLFW_KEY_DOWN,        -1.f});
        bind("MoveRight",    {T::Key, GLFW_KEY_D,            1.f});
        bind("MoveRight",    {T::Key, GLFW_KEY_RIGHT,        1.f});
        bind("MoveLeft",     {T::Key, GLFW_KEY_A,           -1.f});
        bind("MoveLeft",     {T::Key, GLFW_KEY_LEFT,        -1.f});
        bind("MoveUp",       {T::Key, GLFW_KEY_SPACE,        1.f});
        bind("MoveDown",     {T::Key, GLFW_KEY_LEFT_SHIFT,  -1.f});
        // Actions
        bind("Jump",         {T::Key, GLFW_KEY_SPACE});
        bind("Interact",     {T::Key, GLFW_KEY_E});
        bind("Sprint",       {T::Key, GLFW_KEY_LEFT_CONTROL});
        bind("Shoot",        {T::MouseButton, GLFW_MOUSE_BUTTON_LEFT});
        bind("ADS",          {T::MouseButton, GLFW_MOUSE_BUTTON_RIGHT});
        // UI
        bind("Pause",        {T::Key, GLFW_KEY_ESCAPE});
        bind("Console",      {T::Key, GLFW_KEY_GRAVE_ACCENT});
    }

private:
    std::unordered_map<std::string, std::vector<Binding>> m_bindings;

    const std::vector<Binding>& get_bindings(const std::string& action) const {
        static const std::vector<Binding> empty;
        auto it = m_bindings.find(action);
        return it != m_bindings.end() ? it->second : empty;
    }

    static bool check_down(const Binding& b) {
        if (b.type == Binding::Type::Key)
            return Input::isKeyDown(b.code);
        if (b.type == Binding::Type::MouseButton)
            return Input::isMouseDown(b.code);
        return false;
    }

    static bool check_pressed(const Binding& b) {
        if (b.type == Binding::Type::Key)
            return Input::isKeyPressed(b.code);
        if (b.type == Binding::Type::MouseButton)
            return Input::isMousePressed(b.code);
        return false;
    }

    static bool check_released(const Binding& b) {
        if (b.type == Binding::Type::Key)
            return Input::isKeyReleased(b.code);
        if (b.type == Binding::Type::MouseButton)
            return Input::isMouseReleased(b.code);
        return false;
    }
};

// Shortcuts
inline InputActionMap& Actions() { return InputActionMap::get(); }
