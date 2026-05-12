#pragma once

class Time {
public:
    // Call at the start of each frame
    static void tick();

    static float delta();     // seconds since the last frame
    static float elapsed();   // seconds since the start
    static float fps();       // frames per second (sliding average)

private:
    static float s_delta;
    static float s_elapsed;
    static float s_fps;
    static float s_last;
    static float s_fpsTimer;
    static int   s_frameCount;
};
