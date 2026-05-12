#pragma once
// ╔══════════════════════════════════════════════════════════════════════════╗
// ║  EmbeddedRegistry.h  —  Virtual in-memory filesystem                    ║
// ║                                                                          ║
// ║  Stores pointers to byte arrays compiled into the .exe                  ║
// ║  via embed_assets.py.                                                    ║
// ║                                                                          ║
// ║  Usage:                                                                  ║
// ║    // At startup (before any resource load):                             ║
// ║    registerEmbeddedAssets();                                             ║
// ║                                                                          ║
// ║    // In ResourceManager / anywhere:                                     ║
// ║    auto* e = EmbeddedRegistry::get().find("assets/textures/wall.png");  ║
// ║    if (e) { /* load from e->data, e->size */ }                          ║
// ╚══════════════════════════════════════════════════════════════════════════╝
#include <string>
#include <unordered_map>
#include <cstddef>

struct EmbeddedEntry {
    const unsigned char* data = nullptr;
    unsigned int         size = 0;
};

class EmbeddedRegistry {
public:
    static EmbeddedRegistry& get() {
        static EmbeddedRegistry inst;
        return inst;
    }

    // Called by registerEmbeddedAssets() — adds one file
    void add(const std::string& virtualPath,
             const unsigned char* data,
             unsigned int         size)
    {
        m_entries[virtualPath] = { data, size };
        // Also register without "assets/" prefix so both forms work:
        //   "assets/textures/wall.png"  and  "textures/wall.png"
        if (virtualPath.rfind("assets/", 0) == 0)
            m_entries[virtualPath.substr(7)] = { data, size };
    }

    // Returns nullptr if not found
    const EmbeddedEntry* find(const std::string& path) const {
        // Normalize backslashes
        std::string key = path;
        for (char& c : key) if (c == '\\') c = '/';

        auto it = m_entries.find(key);
        if (it != m_entries.end()) return &it->second;

        // Try with "assets/" prefix added
        it = m_entries.find("assets/" + key);
        if (it != m_entries.end()) return &it->second;

        return nullptr;
    }

    bool has(const std::string& path) const {
        return find(path) != nullptr;
    }

    // List all registered paths (debug)
    void dump() const {
        for (auto& [k, v] : m_entries)
            printf("[embed] %s  (%u bytes)\n", k.c_str(), v.size);
    }

    size_t count() const { return m_entries.size(); }

private:
    std::unordered_map<std::string, EmbeddedEntry> m_entries;
};
