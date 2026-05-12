#pragma once
#include <string>
#include <glad/glad.h>

class Texture {
public:
    Texture() = default;

    // ── Load from file (tries embedded registry first, then disk) ──────────
    static Texture fromFile(const std::string& path,
                            bool    flipVertically = true,
                            GLenum  wrapS      = GL_REPEAT,
                            GLenum  wrapT      = GL_REPEAT,
                            GLenum  minFilter  = GL_LINEAR_MIPMAP_LINEAR,
                            GLenum  magFilter  = GL_LINEAR);

    // ── Load from raw memory buffer (compressed image — PNG/JPG/etc) ──────────
    static Texture fromMemory(const unsigned char* buf, unsigned int len,
                              bool    flipVertically = true,
                              GLenum  wrapS      = GL_REPEAT,
                              GLenum  wrapT      = GL_REPEAT,
                              GLenum  minFilter  = GL_LINEAR_MIPMAP_LINEAR,
                              GLenum  magFilter  = GL_LINEAR);

    // ── Upload already-decoded pixels (e.g. from tinygltf) ────────────────────
    // pixels: tightly-packed rows, channels = 3 (RGB) or 4 (RGBA)
    static Texture fromPixels(const unsigned char* pixels,
                              int width, int height, int channels,
                              GLenum  wrapS     = GL_REPEAT,
                              GLenum  wrapT     = GL_REPEAT,
                              GLenum  minFilter = GL_LINEAR_MIPMAP_LINEAR,
                              GLenum  magFilter = GL_LINEAR);

    // ── Solid-color helpers ────────────────────────────────────────────────
    static Texture white();
    static Texture fromColor(unsigned char r, unsigned char g,
                             unsigned char b, unsigned char a = 255);

    // ── RAII ──────────────────────────────────────────────────────────────
    ~Texture();
    Texture(Texture&& o) noexcept;
    Texture& operator=(Texture&& o) noexcept;
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    // ── API ───────────────────────────────────────────────────────────────
    void bind  (unsigned int slot = 0) const;
    void unbind() const;

    bool   valid()  const { return m_id != 0; }
    GLuint id()     const { return m_id; }
    int    width()  const { return m_w; }
    int    height() const { return m_h; }

private:
    friend Texture uploadPixels(const unsigned char*, int, int, int, GLenum, GLenum, GLenum, GLenum);
    explicit Texture(GLuint id, int w, int h) : m_id(id), m_w(w), m_h(h) {}

    GLuint m_id = 0;
    int    m_w  = 0;
    int    m_h  = 0;
};
