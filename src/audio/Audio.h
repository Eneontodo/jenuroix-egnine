#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Audio.h  —  Simple audio system built on miniaudio
//
//  Setup (one time):
//    Download miniaudio.h from https://miniaud.io  →  vendor/miniaudio/miniaudio.h
//
//  Usage:
//    Audio::play("assets/sounds/shoot.wav");
//    SoundHandle h = Audio::loop("assets/sounds/music.ogg");
//    Audio::setVolume(h, 0.5f);
//    Audio::stop(h);
//    Audio::setListenerPos(camera.position());  // 3D audio
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <cstdint>
#include <glm/glm.hpp>

using SoundHandle = uint32_t;
constexpr SoundHandle INVALID_SOUND = 0;

class Audio {
public:
    // ── Lifecycle ─────────────────────────────────────────────────────────────
    static void init();
    static void shutdown();
    static void update();          // call once per frame for 3D positional audio

    // ── Playback ──────────────────────────────────────────────────────────────
    // Play once and forget (fire-and-forget)
    static SoundHandle play(const std::string& path, float volume = 1.f);

    // Loop continuously — returns handle to control later
    static SoundHandle loop(const std::string& path, float volume = 1.f);

    // Play at a 3D world position (attenuates with distance from listener)
    static SoundHandle play3D(const std::string& path,
                                glm::vec3         position,
                                float             volume     = 1.f,
                                float             maxDist    = 20.f);

    static SoundHandle loop3D(const std::string& path,
                                glm::vec3         position,
                                float             volume     = 1.f,
                                float             maxDist    = 20.f);

    // ── Control ───────────────────────────────────────────────────────────────
    static void   stop   (SoundHandle h);
    static void   pause  (SoundHandle h);
    static void   resume (SoundHandle h);
    static void   stopAll();

    static void   setVolume  (SoundHandle h, float volume);  // 0.0 – 1.0
    static void   setPitch   (SoundHandle h, float pitch);   // 1.0 = normal
    static void   setPosition(SoundHandle h, glm::vec3 pos); // update 3D pos

    static bool   isPlaying  (SoundHandle h);
    static float  getVolume  (SoundHandle h);

    // ── Master controls ───────────────────────────────────────────────────────
    static void  setMasterVolume(float volume);  // 0.0 – 1.0
    static float getMasterVolume();

    // ── Listener (3D audio camera) ────────────────────────────────────────────
    static void setListenerPos(glm::vec3 position);
    static void setListenerDir(glm::vec3 forward, glm::vec3 up = {0,1,0});

    // ── Preload ───────────────────────────────────────────────────────────────
    // Load into memory now so first play() has no stutter
    static void preload(const std::string& path);
    static void unload (const std::string& path);

    // ── Diagnostics ───────────────────────────────────────────────────────────
    static int  activeSoundCount();
    static bool isInitialized();
};
