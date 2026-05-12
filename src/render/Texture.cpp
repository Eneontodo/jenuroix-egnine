#include "render/Texture.h"
#include "res/EmbeddedRegistry.h"
#include "core/Log.h"
#include <stdexcept>
#include <filesystem>

// STB_IMAGE_IMPLEMENTATION lives in ModelLoader.cpp (compiled once).
// Here we only use the header (declarations + inline helpers).
#include <stb_image.h>

namespace fs = std::filesystem;

// ── Internal helper: upload raw pixel data to OpenGL ─────────────────────────
Texture uploadPixels(const unsigned char* data, int w, int h, int ch,
                             GLenum wrapS, GLenum wrapT,
                             GLenum minFilter, GLenum magFilter)
{
    GLenum internalFmt, fmt;
    switch (ch) {
    case 1: internalFmt = GL_R8;    fmt = GL_RED;  break;
    case 2: internalFmt = GL_RG8;   fmt = GL_RG;   break;
    case 3: internalFmt = GL_RGB8;  fmt = GL_RGB;  break;
    default:internalFmt = GL_RGBA8; fmt = GL_RGBA; break;
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    // Default row alignment is 4 bytes — RGB textures with width not divisible
    // by 4 will be corrupt or cause a crash without this:
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)internalFmt,
                 w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restore default
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     (GLint)wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     (GLint)wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)magFilter);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Force GL_RED to display as white (not red) using swizzle
    if (ch == 1) {
        glBindTexture(GL_TEXTURE_2D, id);
        GLint sw[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, sw);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    return Texture(id, w, h);
}

// ── fromMemory — load from a raw byte buffer (embedded asset) ────────────────
Texture Texture::fromMemory(const unsigned char* buf, unsigned int len,
                              bool flip,
                              GLenum wrapS, GLenum wrapT,
                              GLenum minFilter, GLenum magFilter)
{
    stbi_set_flip_vertically_on_load(flip ? 1 : 0);

    int w = 0, h = 0, ch = 0;
    unsigned char* px = stbi_load_from_memory(buf, (int)len, &w, &h, &ch, 0);
    if (!px) {
        throw std::runtime_error(
            std::string("Texture::fromMemory stb_image error: ")
            + stbi_failure_reason());
    }

    Texture tex = uploadPixels(px, w, h, ch, wrapS, wrapT, minFilter, magFilter);
    stbi_image_free(px);
    return tex;
}

// ── fromPixels — upload already-decoded pixel data (e.g. from tinygltf) ─────
Texture Texture::fromPixels(const unsigned char* pixels,
                             int w, int h, int ch,
                             GLenum wrapS, GLenum wrapT,
                             GLenum minFilter, GLenum magFilter)
{
    if (!pixels || w <= 0 || h <= 0 || ch <= 0)
        throw std::runtime_error("Texture::fromPixels: invalid pixel data");
    return uploadPixels(pixels, w, h, ch, wrapS, wrapT, minFilter, magFilter);
}

// ── fromFile — try embedded first, then disk ──────────────────────────────────
Texture Texture::fromFile(const std::string& path,
                           bool     flip,
                           GLenum   wrapS,     GLenum wrapT,
                           GLenum   minFilter, GLenum magFilter)
{
    // 1. Try embedded registry (compiled into the .exe)
    if (const EmbeddedEntry* e = EmbeddedRegistry::get().find(path)) {
        LOG_INFO("Texture [embed]: " << path
                 << "  (" << e->size << " bytes)");
        try {
            return fromMemory(e->data, e->size, flip,
                              wrapS, wrapT, minFilter, magFilter);
        } catch (const std::exception& ex) {
            LOG_ERROR("Texture [embed] decode failed: " << ex.what());
            // fall through to disk
        }
    }

    // 2. Fall back to disk
    if (!fs::exists(path)) {
        LOG_ERROR("Texture not found (embedded nor on disk): " << path);
        LOG_ERROR("  Working dir: " << fs::current_path().string());
        throw std::runtime_error("Texture not found: " + path);
    }

    stbi_set_flip_vertically_on_load(flip ? 1 : 0);
    int w = 0, h = 0, ch = 0;
    unsigned char* px = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!px) {
        LOG_ERROR("Texture stb_image error [" << path << "]: "
                  << stbi_failure_reason());
        throw std::runtime_error(
            std::string("stb_image: ") + stbi_failure_reason()
            + "  [" + path + "]");
    }

    Texture tex = uploadPixels(px, w, h, ch, wrapS, wrapT, minFilter, magFilter);
    stbi_image_free(px);
    LOG_INFO("Texture [disk]: " << w << "x" << h << " ch=" << ch
             << "  " << fs::path(path).filename().string());
    return tex;
}

// ── Convenience constructors ─────────────────────────────────────────────────
Texture Texture::white() {
    return fromColor(255, 255, 255, 255);
}

Texture Texture::fromColor(unsigned char r, unsigned char g,
                             unsigned char b, unsigned char a)
{
    unsigned char px[4] = {r, g, b, a};
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Texture(id, 1, 1);
}

// ── RAII ─────────────────────────────────────────────────────────────────────
Texture::~Texture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

Texture::Texture(Texture&& o) noexcept
    : m_id(o.m_id), m_w(o.m_w), m_h(o.m_h) { o.m_id = 0; }

Texture& Texture::operator=(Texture&& o) noexcept {
    if (this != &o) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id = o.m_id; m_w = o.m_w; m_h = o.m_h;
        o.m_id = 0;
    }
    return *this;
}

// ── Bind ─────────────────────────────────────────────────────────────────────
void Texture::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}
