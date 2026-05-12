// Audio.cpp — miniaudio backend
//
// miniaudio is a single-header C library. Define the implementation ONCE here.
// Get it from: https://miniaud.io  (vendor/miniaudio/miniaudio.h)

#if __has_include(<miniaudio/miniaudio.h>)
#  define MINIAUDIO_IMPLEMENTATION
#  include <miniaudio/miniaudio.h>
#  define AUDIO_AVAILABLE 1
#else
#  define AUDIO_AVAILABLE 0
#  pragma message("miniaudio.h not found — Audio system is a stub. Get it from https://miniaud.io")
#endif

#include "audio/Audio.h"
#include "core/Log.h"
#include "res/EmbeddedRegistry.h"

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;


//  Internal state
// <---------------------------->
namespace {

#if AUDIO_AVAILABLE

struct SoundEntry {
    ma_sound   sound;
    bool       valid   = false;
    bool       is3D    = false;
    glm::vec3  pos     = {};
};

struct AudioState {
    ma_engine                                    engine;
    bool                                         ready = false;
    std::unordered_map<SoundHandle, SoundEntry>  sounds;
    std::unordered_map<std::string, ma_decoder*> cache;   // preloaded decoders
    SoundHandle                                  nextId = 1;
    float                                        masterVolume = 1.f;
    std::mutex                                   mtx;
};

static AudioState* g_audio = nullptr;

// Resolve path: embedded registry first, then disk
static std::string resolveAudioPath(const std::string& path) {
    const EmbeddedEntry* e = EmbeddedRegistry::get().find(path);
    if (e) return path;  // embedded — return as-is, handled below
    if (fs::exists(path)) return path;
    // Try assets/sounds/ subfolder
    std::string alt = "assets/sounds/" + fs::path(path).filename().string();
    if (fs::exists(alt)) return alt;
    return path;
}

// Helper: init a ma_sound from path (embedded or disk)
static bool initSound(ma_sound& s, const std::string& path,
                        bool loop, bool spatial)
{
    ma_uint32 flags = 0;
    if (spatial) flags |= MA_SOUND_FLAG_NO_SPATIALIZATION; // we handle manually
    if (!spatial) flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

    // Try embedded first
    const EmbeddedEntry* e = EmbeddedRegistry::get().find(path);
    if (e) {
        ma_result r = ma_sound_init_from_data_source(
            &g_audio->engine, nullptr, flags, nullptr, &s);
        // For embedded audio we use a memory decoder
        // miniaudio supports init from memory via ma_decoder + ma_sound_init_from_data_source
        // Use the full pipeline:
        ma_decoder_config cfg = ma_decoder_config_init_default();
        ma_decoder* dec = new ma_decoder;
        if (ma_decoder_init_memory(e->data, e->size, &cfg, dec) != MA_SUCCESS) {
            delete dec;
            LOG_WARN("Audio: failed to decode embedded: " << path);
            return false;
        }
        if (ma_sound_init_from_data_source(&g_audio->engine,
            dec, flags, nullptr, &s) != MA_SUCCESS)
        {
            ma_decoder_uninit(dec);
            delete dec;
            return false;
        }
        return true;
    }

    // Disk file
    ma_result r = ma_sound_init_from_file(
        &g_audio->engine, resolveAudioPath(path).c_str(),
        flags, nullptr, nullptr, &s);

    if (r != MA_SUCCESS) {
        LOG_WARN("Audio: cannot load '" << path << "' (ma error " << r << ")");
        return false;
    }
    ma_sound_set_looping(&s, loop ? MA_TRUE : MA_FALSE);
    if (spatial) {
        ma_sound_set_spatialization_enabled(&s, MA_TRUE);
        ma_sound_set_attenuation_model(&s, ma_attenuation_model_inverse_distance);
    }
    return true;
}

static SoundHandle addSound(const std::string& path, bool loop,
                            bool spatial, float volume,
                            glm::vec3 pos = {}, float maxDist = 20.f)
{
    if (!g_audio || !g_audio->ready) return INVALID_SOUND;

    std::lock_guard<std::mutex> lk(g_audio->mtx);
    SoundHandle id = g_audio->nextId++;
    SoundEntry& e  = g_audio->sounds[id];

    if (!initSound(e.sound, path, loop, spatial)) {
        g_audio->sounds.erase(id);
        return INVALID_SOUND;
    }

    e.valid = true;
    e.is3D  = spatial;
    e.pos   = pos;

    ma_sound_set_volume(&e.sound, volume * g_audio->masterVolume);
    if (spatial) {
        ma_sound_set_position(&e.sound, pos.x, pos.y, pos.z);
        ma_sound_set_max_distance(&e.sound, maxDist);
    }
    ma_sound_start(&e.sound);
    return id;
}

#endif // AUDIO_AVAILABLE

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────

void Audio::init() {
#if AUDIO_AVAILABLE
    g_audio = new AudioState();
    ma_engine_config cfg = ma_engine_config_init();
    cfg.listenerCount    = 1;

    if (ma_engine_init(&cfg, &g_audio->engine) != MA_SUCCESS) {
        LOG_ERROR("Audio: failed to initialise miniaudio engine");
        delete g_audio;
        g_audio = nullptr;
        return;
    }
    g_audio->ready = true;
    LOG_INFO("Audio: miniaudio engine ready");
#else
    LOG_WARN("Audio: miniaudio not available — place miniaudio.h in vendor/miniaudio/");
#endif
}

void Audio::shutdown() {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    stopAll();
    // Flush cached decoders
    for (auto& [k,dec] : g_audio->cache) {
        ma_decoder_uninit(dec);
        delete dec;
    }
    ma_engine_uninit(&g_audio->engine);
    delete g_audio;
    g_audio = nullptr;
    LOG_INFO("Audio: shutdown");
#endif
}

void Audio::update() {
#if AUDIO_AVAILABLE
    if (!g_audio || !g_audio->ready) return;
    // Auto-remove finished one-shot sounds
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    std::vector<SoundHandle> toRemove;
    for (auto& [id, e] : g_audio->sounds) {
        if (e.valid && ma_sound_at_end(&e.sound)) {
            if (!ma_sound_is_looping(&e.sound)) {
                ma_sound_uninit(&e.sound);
                toRemove.push_back(id);
            }
        }
    }
    for (auto id : toRemove) g_audio->sounds.erase(id);
#endif
}

SoundHandle Audio::play(const std::string& path, float volume) {
#if AUDIO_AVAILABLE
    return addSound(path, false, false, volume);
#else
    return INVALID_SOUND;
#endif
}

SoundHandle Audio::loop(const std::string& path, float volume) {
#if AUDIO_AVAILABLE
    return addSound(path, true, false, volume);
#else
    return INVALID_SOUND;
#endif
}

SoundHandle Audio::play3D(const std::string& path, glm::vec3 pos,
                            float volume, float maxDist) {
#if AUDIO_AVAILABLE
    return addSound(path, false, true, volume, pos, maxDist);
#else
    return INVALID_SOUND;
#endif
}

SoundHandle Audio::loop3D(const std::string& path, glm::vec3 pos,
                            float volume, float maxDist) {
#if AUDIO_AVAILABLE
    return addSound(path, true, true, volume, pos, maxDist);
#else
    return INVALID_SOUND;
#endif
}

void Audio::stop(SoundHandle h) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    auto it = g_audio->sounds.find(h);
    if (it == g_audio->sounds.end() || !it->second.valid) return;
    ma_sound_stop(&it->second.sound);
    ma_sound_uninit(&it->second.sound);
    g_audio->sounds.erase(it);
#endif
}

void Audio::pause(SoundHandle h) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    auto it = g_audio->sounds.find(h);
    if (it != g_audio->sounds.end() && it->second.valid)
        ma_sound_stop(&it->second.sound);
#endif
}

void Audio::resume(SoundHandle h) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    auto it = g_audio->sounds.find(h);
    if (it != g_audio->sounds.end() && it->second.valid)
        ma_sound_start(&it->second.sound);
#endif
}

void Audio::stopAll() {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    for (auto& [id, e] : g_audio->sounds) {
        if (e.valid) {
            ma_sound_stop(&e.sound);
            ma_sound_uninit(&e.sound);
        }
    }
    g_audio->sounds.clear();
#endif
}

void Audio::setVolume(SoundHandle h, float volume) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    auto it = g_audio->sounds.find(h);
    if (it != g_audio->sounds.end() && it->second.valid)
        ma_sound_set_volume(&it->second.sound, volume * g_audio->masterVolume);
#endif
}

void Audio::setPitch(SoundHandle h, float pitch) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    auto it = g_audio->sounds.find(h);
    if (it != g_audio->sounds.end() && it->second.valid)
        ma_sound_set_pitch(&it->second.sound, pitch);
#endif
}

void Audio::setPosition(SoundHandle h, glm::vec3 pos) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    auto it = g_audio->sounds.find(h);
    if (it != g_audio->sounds.end() && it->second.valid && it->second.is3D) {
        it->second.pos = pos;
        ma_sound_set_position(&it->second.sound, pos.x, pos.y, pos.z);
    }
#endif
}

bool Audio::isPlaying(SoundHandle h) {
#if AUDIO_AVAILABLE
    if (!g_audio) return false;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    auto it = g_audio->sounds.find(h);
    return it != g_audio->sounds.end() && it->second.valid && ma_sound_is_playing(&it->second.sound);
#else
    return false;
#endif
}

float Audio::getVolume(SoundHandle h) {
#if AUDIO_AVAILABLE
    if (!g_audio) return 0.f;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    auto it = g_audio->sounds.find(h);
    if (it != g_audio->sounds.end() && it->second.valid)
        return ma_sound_get_volume(&it->second.sound);
#endif
    return 0.f;
}

void Audio::setMasterVolume(float volume) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    g_audio->masterVolume = volume;
    ma_engine_set_volume(&g_audio->engine, volume);
#endif
}

float Audio::getMasterVolume() {
#if AUDIO_AVAILABLE
    return g_audio ? g_audio->masterVolume : 1.f;
#else
    return 1.f;
#endif
}

void Audio::setListenerPos(glm::vec3 pos) {
#if AUDIO_AVAILABLE
    if (!g_audio || !g_audio->ready) return;
    ma_engine_listener_set_position(&g_audio->engine, 0, pos.x, pos.y, pos.z);
#endif
}

void Audio::setListenerDir(glm::vec3 forward, glm::vec3 up) {
#if AUDIO_AVAILABLE
    if (!g_audio || !g_audio->ready) return;
    ma_engine_listener_set_direction(&g_audio->engine, 0,
        forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&g_audio->engine, 0,
        up.x, up.y, up.z);
#endif
}

void Audio::preload(const std::string& path) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    if (g_audio->cache.count(path)) return;
    ma_decoder_config cfg = ma_decoder_config_init_default();
    ma_decoder* dec = new ma_decoder;
    if (ma_decoder_init_file(resolveAudioPath(path).c_str(), &cfg, dec) == MA_SUCCESS)
        g_audio->cache[path] = dec;
    else {
        delete dec;
        LOG_WARN("Audio::preload: cannot load '" << path << "'");
    }
#endif
}

void Audio::unload(const std::string& path) {
#if AUDIO_AVAILABLE
    if (!g_audio) return;
    auto it = g_audio->cache.find(path);
    if (it == g_audio->cache.end()) return;
    ma_decoder_uninit(it->second);
    delete it->second;
    g_audio->cache.erase(it);
#endif
}

int Audio::activeSoundCount() {
#if AUDIO_AVAILABLE
    if (!g_audio) return 0;
    std::lock_guard<std::mutex> lk(g_audio->mtx);
    return (int)g_audio->sounds.size();
#else
    return 0;
#endif
}

bool Audio::isInitialized() {
#if AUDIO_AVAILABLE
    return g_audio && g_audio->ready;
#else
    return false;
#endif
}
