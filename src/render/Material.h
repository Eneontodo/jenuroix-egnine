#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "render/Texture.h"
#include "render/Shader.h"

// Surface material — passed to the shader before every draw call.
// albedoMap is set by ResourceManager after the model is loaded.
struct Material {
    std::string name = "Default";

    // Base colour / tint (multiplied with the texture if one is set)
    glm::vec4 albedo    = {1, 1, 1, 1};
    glm::vec3 emissive  = {0, 0, 0};
    float     metallic  = 0.0f;
    float     roughness = 0.8f;
    float     ao        = 1.0f;

    // Path recorded by ModelLoader so ResourceManager can load the texture later.
    // You don't need to set this yourself.
    std::string diffuseTexturePath;

    // Raw image bytes packed inside a .blend file (File > External Data > Pack).
    // When set, ResourceManager loads the texture directly from this buffer
    // without needing any external file.  Set automatically by BlendLoader.
    std::vector<unsigned char> packedImageData;

    // GPU texture pointers — owned by ResourceManager, not by Material.
    const Texture* albedoMap     = nullptr;
    const Texture* normalMap     = nullptr;
    const Texture* metallicMap   = nullptr;
    const Texture* roughnessMap  = nullptr;
    const Texture* aoMap         = nullptr;
    const Texture* emissiveMap   = nullptr;

    bool twoSided    = false;
    bool transparent = false;
    bool castShadow  = true;
    bool wireframe   = false;
    bool doubleSided = false;
    float tilingU    = 1.f;
    float tilingV    = 1.f;

    // Bind all textures and push uniforms to the shader
    void bind(Shader& shader) const {
        shader.setVec4 ("uMat.albedo",    albedo);
        shader.setVec3 ("uMat.emissive",  emissive);
        shader.setFloat("uMat.metallic",  metallic);
        shader.setFloat("uMat.roughness", roughness);
        shader.setFloat("uMat.ao",        ao);

        auto bindSlot = [&](const Texture* tex, unsigned int slot, const char* flag) {
            shader.setBool(flag, tex != nullptr);
            if (tex) tex->bind(slot);
        };
        bindSlot(albedoMap,   0, "uMat.hasAlbedoMap");
        bindSlot(normalMap,   1, "uMat.hasNormalMap");
        bindSlot(metallicMap, 2, "uMat.hasMetallicMap");
        bindSlot(roughnessMap,3, "uMat.hasRoughnessMap");
        bindSlot(aoMap,       4, "uMat.hasAOMap");
        bindSlot(emissiveMap, 5, "uMat.hasEmissiveMap");

        if (twoSided) glDisable(GL_CULL_FACE);
        else          glEnable (GL_CULL_FACE);
    }
};
