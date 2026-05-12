#pragma once
#include "core/Window.h"
#include "core/Console.h"
#include "render/Camera.h"
#include "game/Scene.h"
#include <memory>
#include "core/Input.h"

// Let's you write your game by inheriting from Game and overriding these methods:
//   void onStart()
//   void onUpdate(float dt)
//   void onRender()
//
// After inheriting from Game and overriding these methods, pass your game class to GameRunner::run<YourGame>(argc, argv).

class Game {
public:
    explicit Game(Window& window);
    virtual ~Game() = default;

    // Called once at the start
    virtual void onStart() {}
    // dt — seconds since the last frame
    virtual void onUpdate(float dt) {}
    // Render the scene here
    virtual void onRender() {}

    Window& window() { return m_window; }
    Camera& camera() { return m_camera; }

    bool running()     const { return m_running; }
    void quit()              { m_running = false; }

protected:
    Window&  m_window;
    Camera   m_camera;
    Scene    m_scene;
    bool     m_running = true;
};

// Runner
struct GameConfig {
    int         width   = 1280;
    int         height  = 720;
    const char* title   = "My Game";
    bool        vsync   = true;
};

class GameRunner {
public:
    template<typename G>
    static int run(GameConfig cfg = {}) {
        try {
            Window window(cfg.width, cfg.height, cfg.title);
            window.setVSync(cfg.vsync);

            G game(window);

            // Register basic console commands
            Console::get().addCommand("quit",
                [&](auto&){ game.quit(); }, "Quit the game");
            Console::get().addCommand("fov",
                [&](const std::vector<std::string>& a){
                    if (a.size() > 1) game.camera().setFov(std::stof(a[1]));
                }, "fov <degrees>");

            game.onStart();

            while (window.isOpen() && game.running()) {
                // Input and events
                window.pollEvents();

                // Logic update
                float dt = (float)glfwGetTime();          // Time since start
                static float lastTime = 0.f;
                float delta = dt - lastTime;
                lastTime    = dt;

                game.onUpdate(delta);

                // Render
                window.clear(0.08f, 0.08f, 0.12f);
                game.onRender();
                window.display();

                Input::endFrame();
            }
            return 0;
        } catch (const std::exception& e) {
            fprintf(stderr, "Fatal: %s\n", e.what());
            return 1;
        }
    }
};
