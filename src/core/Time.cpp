#include "core/Time.h"
#include <GLFW/glfw3.h>

float Time::s_delta      = 0.f;
float Time::s_elapsed    = 0.f;
float Time::s_fps        = 0.f;
float Time::s_last       = 0.f;
float Time::s_fpsTimer   = 0.f;
int   Time::s_frameCount = 0;

void Time::tick() {
    float now  = (float)glfwGetTime();
    s_delta    = now - s_last;
    s_last     = now;
    s_elapsed  = now;

    s_fpsTimer += s_delta;
    ++s_frameCount;
    if (s_fpsTimer >= 0.5f) {
        s_fps        = (float)s_frameCount / s_fpsTimer;
        s_fpsTimer   = 0.f;
        s_frameCount = 0;
    }
}

float Time::delta()   { return s_delta; }
float Time::elapsed() { return s_elapsed; }
float Time::fps()     { return s_fps; }
