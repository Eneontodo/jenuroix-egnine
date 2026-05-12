#include "core/Window.h"
#include <stdexcept>
#include <cstdio>

void Window::errorCallback(int code, const char* desc) {
    fprintf(stderr, "[GLFW Error %d] %s\n", code, desc);
}

void Window::framebufferResizeCallback(GLFWwindow* wnd, int w, int h) {
    glViewport(0, 0, w, h);
    auto* self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(wnd));
    self->m_width  = w;
    self->m_height = h;
    if (self->m_resizeCb) self->m_resizeCb(w, h);
}

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height)
{
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA x4

    m_handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_handle);
    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, framebufferResizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setVSync(false); // default: no vsync (engine.h controls this)
}

Window::~Window() {
    if (m_handle) glfwDestroyWindow(m_handle);
    glfwTerminate();
}

bool Window::isOpen() const {
    return !glfwWindowShouldClose(m_handle);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::display() {
    glfwSwapBuffers(m_handle);
}

void Window::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}

void Window::setTitle(const std::string& title) {
    glfwSetWindowTitle(m_handle, title.c_str());
}

void Window::captureCursor(bool captured) {
    m_cursorCaptured = captured;
    glfwSetInputMode(m_handle, GLFW_CURSOR,
        captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}
