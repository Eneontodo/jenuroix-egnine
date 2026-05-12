#include "core/Input.h"
#include <algorithm>

// Static storage
std::array<bool, Input::KEY_COUNT> Input::s_keys        = {};
std::array<bool, Input::KEY_COUNT> Input::s_keysPressed  = {};
std::array<bool, Input::KEY_COUNT> Input::s_keysReleased = {};

std::array<bool, Input::BTN_COUNT> Input::s_buttons         = {};
std::array<bool, Input::BTN_COUNT> Input::s_buttonsPressed  = {};
std::array<bool, Input::BTN_COUNT> Input::s_buttonsReleased = {};

glm::vec2 Input::s_mousePos   = {};
glm::vec2 Input::s_mouseLast  = {};
glm::vec2 Input::s_mouseDelta = {};
float     Input::s_scrollDelta = 0.f;
bool      Input::s_firstMouse  = true;

// Init
void Input::init(GLFWwindow* window) {
    glfwSetKeyCallback        (window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback  (window, cursorPosCallback);
    glfwSetScrollCallback     (window, scrollCallback);
}

// Keyboard
void Input::keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (key < 0 || key >= KEY_COUNT) return;
    if (action == GLFW_PRESS) {
        s_keys[key]        = true;
        s_keysPressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        s_keys[key]         = false;
        s_keysReleased[key] = true;
    }
}

bool Input::isKeyDown    (int key) { return key >= 0 && key < KEY_COUNT && s_keys[key]; }
bool Input::isKeyPressed (int key) { return key >= 0 && key < KEY_COUNT && s_keysPressed[key]; }
bool Input::isKeyReleased(int key) { return key >= 0 && key < KEY_COUNT && s_keysReleased[key]; }

// Mouse buttons
void Input::mouseButtonCallback(GLFWwindow*, int btn, int action, int) {
    if (btn < 0 || btn >= BTN_COUNT) return;
    if (action == GLFW_PRESS) {
        s_buttons[btn]        = true;
        s_buttonsPressed[btn] = true;
    } else if (action == GLFW_RELEASE) {
        s_buttons[btn]         = false;
        s_buttonsReleased[btn] = true;
    }
}

bool Input::isMouseDown    (int b) { return b >= 0 && b < BTN_COUNT && s_buttons[b]; }
bool Input::isMousePressed (int b) { return b >= 0 && b < BTN_COUNT && s_buttonsPressed[b]; }
bool Input::isMouseReleased(int b) { return b >= 0 && b < BTN_COUNT && s_buttonsReleased[b]; }

// Cursor
void Input::cursorPosCallback(GLFWwindow*, double x, double y) {
    glm::vec2 pos((float)x, (float)y);
    if (s_firstMouse) { s_mouseLast = pos; s_firstMouse = false; }
    s_mouseDelta += pos - s_mouseLast;
    s_mouseLast   = pos;
    s_mousePos    = pos;
}

glm::vec2 Input::mousePos()   { return s_mousePos; }
glm::vec2 Input::mouseDelta() { return s_mouseDelta; }

// Scroll
void Input::scrollCallback(GLFWwindow*, double, double yoff) {
    s_scrollDelta += (float)yoff;
}
float Input::scrollDelta() { return s_scrollDelta; }

// End of frame
void Input::endFrame() {
    s_keysPressed.fill(false);
    s_keysReleased.fill(false);
    s_buttonsPressed.fill(false);
    s_buttonsReleased.fill(false);
    s_mouseDelta  = {};
    s_scrollDelta = 0.f;
}
