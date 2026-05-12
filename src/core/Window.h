#pragma once
#include <string>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Callback for window resize events
using ResizeCallback = std::function<void(int w, int h)>;

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool   isOpen()   const;
    void   pollEvents();
    void   clear(float r = 0.05f, float g = 0.05f, float b = 0.10f, float a = 1.0f);
    void   display();

    int    width()  const { return m_width; }
    int    height() const { return m_height; }
    float  aspect() const { return (float)m_width / (float)m_height; }

    GLFWwindow* handle() const { return m_handle; }

    void setVSync(bool enabled);
    void setTitle(const std::string& title);
    void setResizeCallback(ResizeCallback cb) { m_resizeCb = cb; }

    // Block/unblock cursor (for FPS camera)
    void captureCursor(bool captured);
    bool isCursorCaptured() const { return m_cursorCaptured; }

private:
    static void framebufferResizeCallback(GLFWwindow* wnd, int w, int h);
    static void errorCallback(int code, const char* desc);

    GLFWwindow*    m_handle        = nullptr;
    int            m_width         = 0;
    int            m_height        = 0;
    bool           m_cursorCaptured = false;
    ResizeCallback m_resizeCb;
};
