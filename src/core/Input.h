#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>

// Use: Input::init(window.handle()) — once.
// Then anywhere: Input::isKeyDown(GLFW_KEY_W)

class Input {
public:
    static void init(GLFWwindow* window);

    // Keyboard
    static bool isKeyDown(int key);      // held down
    static bool isKeyPressed(int key);   // pressed this frame
    static bool isKeyReleased(int key);  // released this frame

    // Mouse buttons
    static bool isMouseDown(int button);
    static bool isMousePressed(int button);
    static bool isMouseReleased(int button);

    static glm::vec2 mousePos();
    static glm::vec2 mouseDelta();  // displacement per frame
    static float     scrollDelta(); // scroll wheel per frame

    // Called automatically at the end of the frame (Window::pollEvents already done)
    static void endFrame();

private:
    static void keyCallback(GLFWwindow*, int key, int, int action, int);
    static void mouseButtonCallback(GLFWwindow*, int btn, int action, int);
    static void cursorPosCallback(GLFWwindow*, double x, double y);
    static void scrollCallback(GLFWwindow*, double, double yoff);

    static constexpr int KEY_COUNT  = GLFW_KEY_LAST + 1;
    static constexpr int BTN_COUNT  = GLFW_MOUSE_BUTTON_LAST + 1;

    static std::array<bool, KEY_COUNT> s_keys;
    static std::array<bool, KEY_COUNT> s_keysPressed;
    static std::array<bool, KEY_COUNT> s_keysReleased;

    static std::array<bool, BTN_COUNT> s_buttons;
    static std::array<bool, BTN_COUNT> s_buttonsPressed;
    static std::array<bool, BTN_COUNT> s_buttonsReleased;

    static glm::vec2 s_mousePos;
    static glm::vec2 s_mouseLast;
    static glm::vec2 s_mouseDelta;
    static float     s_scrollDelta;
    static bool      s_firstMouse;
};
