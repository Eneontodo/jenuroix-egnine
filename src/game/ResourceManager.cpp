#include "game/ResourceManager.h"
#include "render/ModelLoader.h"
#include "render/Mesh.h"
#include "res/EmbeddedRegistry.h"
#include "core/Log.h"
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstring>

namespace fs = std::filesystem;

static std::string normPath(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

static std::string resolveTexturePath(const std::string& raw,
                                       const std::string& modelDir)
{
    auto& reg  = EmbeddedRegistry::get();
    auto  norm = normPath(raw);

    std::vector<std::string> candidates = {
        norm,
        normPath((fs::path(modelDir) / norm).string()),
        normPath((fs::path(modelDir) / fs::path(norm).filename()).string()),
        normPath(("assets/textures" / fs::path(norm).filename()).string()),
    };

    for (auto& c : candidates) {
        if (reg.find(c))    return c;  // embedded
        if (fs::exists(c))  return c;  // disk
    }
    return "";
}

// ── texture() ────────────────────────────────────────────────────────────────
const Texture* ResourceManager::texture(const std::string& path) {
    std::string key = normPath(path);
    auto it = m_textures.find(key);
    if (it != m_textures.end()) return it->second.get();

    std::unique_ptr<Texture> tex;
    try {
        tex = std::make_unique<Texture>(Texture::fromFile(key));
    } catch (const std::exception& e) {
        LOG_ERROR("ResourceManager::texture [" << key << "]: " << e.what());
        tex = std::make_unique<Texture>(Texture::fromColor(255, 0, 255)); // magenta fallback
    }

    const Texture* raw = tex.get();
    m_textures[key] = std::move(tex);
    return raw;
}

// ── colorTex() ────────────────────────────────────────────────────────────────
const Texture* ResourceManager::colorTex(unsigned char r, unsigned char g,
                                          unsigned char b, unsigned char a)
{
    char key[32];
    snprintf(key, sizeof(key), "__color_%02X%02X%02X%02X", r, g, b, a);
    auto it = m_textures.find(key);
    if (it != m_textures.end()) return it->second.get();

    auto tex = std::make_unique<Texture>(Texture::fromColor(r, g, b, a));
    const Texture* raw = tex.get();
    m_textures[key] = std::move(tex);
    return raw;
}

// ── loadShaderSource() ────────────────────────────────────────────────────────
std::string ResourceManager::loadShaderSource(const std::string& path) {
    std::string key = normPath(path);

    if (const EmbeddedEntry* e = EmbeddedRegistry::get().find(key))
        return std::string(reinterpret_cast<const char*>(e->data), e->size);

    std::ifstream f(key);
    if (!f.is_open())
        throw std::runtime_error("Shader source not found: " + key);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ── shader() ──────────────────────────────────────────────────────────────────
Shader* ResourceManager::shader(const std::string& vertPath,
                                 const std::string& fragPath)
{
    std::string key = normPath(vertPath) + "|" + normPath(fragPath);
    auto it = m_shaders.find(key);
    if (it != m_shaders.end()) return it->second.get();

    std::string vert = loadShaderSource(vertPath);
    std::string frag = loadShaderSource(fragPath);

    // Shader::fromSource expects const char* — use .c_str()
    auto sh  = std::make_unique<Shader>(
        Shader::fromSource(vert.c_str(), frag.c_str()));
    Shader* raw = sh.get();
    m_shaders[key] = std::move(sh);
    return raw;
}

// ── model() ───────────────────────────────────────────────────────────────────
Model* ResourceManager::model(const std::string& path) {
    std::string key = normPath(path);
    auto it = m_models.find(key);
    if (it != m_models.end()) return it->second.get();

    auto mdl = loadModelFromFile(key);
    Model* raw = mdl.get();
    m_models[key] = std::move(mdl);
    return raw;
}

// ── textureFromMemory() ───────────────────────────────────────────────────────
const Texture* ResourceManager::textureFromMemory(const std::string&   key,
                                                   const unsigned char* data,
                                                   unsigned int         size)
{
    auto it = m_textures.find(key);
    if (it != m_textures.end()) return it->second.get();

    std::unique_ptr<Texture> tex;
    try {
        // Check for our RAWPX header — already-decoded pixels from tinygltf.
        // Layout: "RAWPX\0\0\0" (8) + w(4 LE) + h(4 LE) + ch(1) + pixels
        const unsigned int RAWPX_HDR = 17;
        if (size > RAWPX_HDR && memcmp(data, "RAWPX", 5) == 0) {
            int w  = (int)(data[8]  | (data[9]  << 8) |
                           (data[10] << 16) | (data[11] << 24));
            int h  = (int)(data[12] | (data[13] << 8) |
                           (data[14] << 16) | (data[15] << 24));
            int ch = (int)data[16];
            const unsigned char* px = data + RAWPX_HDR;

            if (w > 0 && h > 0 && ch > 0 &&
                (unsigned int)(w * h * ch) <= size - RAWPX_HDR)
            {
                tex = std::make_unique<Texture>(
                    Texture::fromPixels(px, w, h, ch));
                LOG_INFO("ResourceManager: uploaded RAWPX texture "
                         << w << "x" << h << " ch=" << ch
                         << " [" << key << "]");
            } else {
                throw std::runtime_error("RAWPX header/size mismatch");
            }
        } else {
            // Compressed image (PNG / JPG / etc) — let stb_image decode it
            tex = std::make_unique<Texture>(
                Texture::fromMemory(data, size));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("ResourceManager::textureFromMemory [" << key << "]: "
                  << e.what());
        tex = std::make_unique<Texture>(Texture::fromColor(255, 0, 255));
    }

    const Texture* raw = tex.get();
    m_textures[key] = std::move(tex);
    return raw;
}

// ── textureFromMemory() — already-decoded pixels overload ────────────────────
const Texture* ResourceManager::textureFromMemory(const std::string&   key,
                                                   const unsigned char* pixels,
                                                   int w, int h, int ch)
{
    auto it = m_textures.find(key);
    if (it != m_textures.end()) return it->second.get();

    std::unique_ptr<Texture> tex;
    try {
        tex = std::make_unique<Texture>(Texture::fromPixels(pixels, w, h, ch));
    } catch (const std::exception& e) {
        LOG_ERROR("ResourceManager::textureFromMemory(pixels) ["
                  << key << "]: " << e.what());
        tex = std::make_unique<Texture>(Texture::fromColor(255, 0, 255));
    }
    const Texture* raw = tex.get();
    m_textures[key] = std::move(tex);
    return raw;
}

// ── loadModelFromFile() ───────────────────────────────────────────────────────
std::unique_ptr<Model> ResourceManager::loadModelFromFile(const std::string& path) {
    const EmbeddedEntry* embedded = EmbeddedRegistry::get().find(path);

    if (!embedded && !fs::exists(path)) {
        LOG_ERROR("Model not found: " << path);
        LOG_ERROR("  Working dir: " << fs::current_path().string());
        auto fb = std::make_unique<Model>();
        fb->addSubMesh(Mesh::makeCube(1.f));
        fb->setPath(path);
        return fb;
    }

    std::unique_ptr<Model> mdl;
    try {
        mdl = embedded
            ? ModelLoader::loadFromMemory(path, embedded->data, embedded->size)
            : ModelLoader::load(path);
        if (!mdl) throw std::runtime_error("loader returned nullptr");
    } catch (const std::exception& e) {
        LOG_ERROR("Model load error [" << path << "]: " << e.what());
        auto fb = std::make_unique<Model>();
        fb->addSubMesh(Mesh::makeCube(1.f));
        fb->setPath(path);
        return fb;
    }

    // Resolve + assign textures to each submesh
    std::string modelDir = fs::path(path).parent_path().string();
    if (modelDir.empty()) modelDir = ".";

    for (size_t i = 0; i < mdl->subMeshCount(); ++i) {
        Material& mat = mdl->material(i);
        if (mat.albedoMap) continue;

        // ── Priority 1: packed image bytes from inside a .blend ──────────
        // BlendLoader fills packedImageData when the image is packed into
        // the .blend (File > External Data > Pack All Into .blend).
        if (!mat.packedImageData.empty()) {
            std::string key = "blend://packed/" + path + "/" + std::to_string(i);
            mat.albedoMap = textureFromMemory(
                key,
                mat.packedImageData.data(),
                (unsigned int)mat.packedImageData.size());
            if (mat.albedoMap)
                LOG_INFO("  Packed texture loaded for submesh " << i);
            continue; // packed wins — skip external file search
        }

        // ── Priority 2: external file path stored by the loader ──────────
        if (mat.diffuseTexturePath.empty()) continue;

        std::string resolved = resolveTexturePath(
            normPath(mat.diffuseTexturePath), modelDir);

        if (resolved.empty()) {
            LOG_WARN("  Texture not found: '" << mat.diffuseTexturePath << "'");
            LOG_WARN("  Put it next to the .blend / .obj or in assets/textures/");
            continue;
        }

        mat.albedoMap = texture(resolved);
        if (mat.albedoMap)
            LOG_INFO("  Texture OK: " << resolved);
    }

    return mdl;
}
