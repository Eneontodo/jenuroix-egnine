#include "game/Game.h"
#include "core/Input.h"

Game::Game(Window& window)
    : m_window(window),
      m_camera(60.f, window.aspect(), 0.05f, 1000.f)
{
    Input::init(window.handle());

    // Update camera aspect on window resize
    window.setResizeCallback([this](int w, int h){
        m_camera.setAspect((float)w / (float)h);
    });
}
