#pragma once
#include "render/Texture.h"
#include "render/Shader.h"
#include "render/Model.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <fstream>

class ResourceManager {
public:
    // Returns raw const pointer — lifetime tied to the ResourceManager cache.
    // Material::albedoMap expects const Texture*.
    const Texture* texture(const std::string& path);
    const Texture* colorTex(unsigned char r, unsigned char g,
                             unsigned char b, unsigned char a = 255);
    // Load texture directly from a raw image buffer (PNG/JPG bytes).
    // 'key' is just a unique cache key string (e.g. "blend://MyImage").
    const Texture* textureFromMemory(const std::string& key,
                                      const unsigned char* data,
                                      unsigned int         size);
    // Upload already-decoded pixels (from tinygltf img.image)
    const Texture* textureFromMemory(const std::string& key,
                                      const unsigned char* pixels,
                                      int width, int height, int channels);

    // Shaders
    Shader* shader(const std::string& vertPath, const std::string& fragPath);

    // Models — raw pointer into cache
    Model* model(const std::string& path);

    // Cache management
    void clearTextures() { m_textures.clear(); }
    void clearShaders()  { m_shaders.clear();  }
    void clearModels()   { m_models.clear();   }
    void clearAll()      { clearTextures(); clearShaders(); clearModels(); }

private:
    std::unique_ptr<Model>  loadModelFromFile(const std::string& path);
    std::string             loadShaderSource (const std::string& path);

    // Owned by cache, exposed as raw pointers
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
    std::unordered_map<std::string, std::unique_ptr<Shader>>  m_shaders;
    std::unordered_map<std::string, std::unique_ptr<Model>>   m_models;
};
