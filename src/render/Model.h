#pragma once
#include <vector>
#include <string>
#include <memory>
#include "render/Mesh.h"
#include "render/Material.h"
#include "render/Shader.h"
#include <glm/glm.hpp>

// One sub-mesh = geometry + material.
struct SubMesh {
    Mesh     mesh;
    Material material;
};

// A model is a collection of sub-meshes, usually loaded from a .obj or .gltf file.
// Load via ResourceManager::model("assets/models/thing.obj").
class Model {
public:
    Model() = default;
    Model(Model&&)            = default;
    Model& operator=(Model&&) = default;
    Model(const Model&)       = delete;
    Model& operator=(const Model&) = delete;

    // Draw all sub-meshes, binding each material before drawing
    void draw(Shader& shader) const {
        for (auto& sm : m_subMeshes) {
            sm.material.bind(shader);
            sm.mesh.draw();
        }
    }

    // Draw raw geometry without touching any material state
    void drawRaw() const {
        for (auto& sm : m_subMeshes)
            sm.mesh.draw();
    }

    void addSubMesh(Mesh mesh, Material mat = {}) {
        m_subMeshes.push_back({std::move(mesh), std::move(mat)});
    }

    size_t subMeshCount() const { return m_subMeshes.size(); }

    // Direct access to a material — used by ResourceManager to set textures
    // Access a sub-mesh by index (used by the renderer)
    SubMesh&       subMesh(size_t i)       { return m_subMeshes[i]; }
    const SubMesh& subMesh(size_t i) const { return m_subMeshes[i]; }

    Material&       material(size_t i)       { return m_subMeshes[i].material; }
    const Material& material(size_t i) const { return m_subMeshes[i].material; }

    // Axis-aligned bounding box (approximate, set by ModelLoader)
    glm::vec3 boundsMin = {-1, -1, -1};
    glm::vec3 boundsMax = { 1,  1,  1};

    const std::string& path() const { return m_path; }
    void setPath(std::string p)     { m_path = std::move(p); }

private:
    std::vector<SubMesh> m_subMeshes;
    std::string          m_path;
};
