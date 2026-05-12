#pragma once
#include <string_view>

// ─────────────────────────────────────────────────────────────────────────────
//  configure.h — single source of truth for engine/editor metadata
//  Change version here — it propagates everywhere automatically.
// ─────────────────────────────────────────────────────────────────────────────

namespace eng {

struct EngineConfig {
    // ── Change these ──────────────────────────────────────────────────────────
    static constexpr std::string_view name          = "Jenuroix Engine";
    static constexpr std::string_view editorName    = "Jenuroix Editor";
    static constexpr std::string_view version       = "1.6.21";
    static constexpr std::string_view author        = "Eneontodo";
    static constexpr std::string_view firstGameName = "Game";

    // ── Derived (do not edit) ─────────────────────────────────────────────────
    static constexpr std::string_view editorTitle() {
        // Concatenation at runtime; constexpr concat requires C++20 tricks,
        // so we expose a helper used wherever the full "Editor vX.Y" string is needed.
        return editorName; // base — full title built in editorTitleFull()
    }

    // Returns e.g. "Jenuroix Editor  v1.2"
    static std::string editorTitleFull() {
        return std::string(editorName) + "  v" + std::string(version);
    }

    // Returns e.g. "Jenuroix Engine v1.2"
    static std::string nameFull() {
        return std::string(name) + " v" + std::string(version);
    }
};

} // namespace eng
