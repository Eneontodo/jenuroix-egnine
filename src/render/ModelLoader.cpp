// ModelLoader.cpp
// ALL header-only library implementations live here — compiled EXACTLY ONCE.
//
// Include order matters:
//   1. stb_image        (used by both us and tinygltf)
//   2. stb_image_write  (required by tinygltf even if we never export)
//   3. tinyobjloader    (v1.x API — LoadObj with out-params, NOT ObjReader)
//   4. json.hpp         (used by tinygltf)
//   5. tinygltf         (after all of the above)
//   6. our own headers

// ── 1. stb_image ─────────────────────────────────────────────────────────────
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ── 2. stb_image_write ───────────────────────────────────────────────────────
// tinygltf internally includes it; suppress its warnings.
#ifdef _MSC_VER
#  pragma warning(push, 0)
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#ifdef _MSC_VER
#  pragma warning(pop)
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

// ── 3. tinyobjloader v1.x ────────────────────────────────────────────────────
// Uses LoadObj() with out-params.  ObjReader/ObjReaderConfig belong to v2+.
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// ── 4. json.hpp ──────────────────────────────────────────────────────────────
#include <json.hpp>

// ── 5. tinygltf ──────────────────────────────────────────────────────────────
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE           // already included above
#define TINYGLTF_NO_INCLUDE_STB_IMAGE   // don't include again
#include <tiny_gltf.h>

// ── 6. Engine headers ─────────────────────────────────────────────────────────
#include "render/ModelLoader.h"
#include "render/BlendLoader.h"
#include "render/Mesh.h"
#include "core/Log.h"
#include "res/EmbeddedRegistry.h"
#include <sstream>

#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "engine.h"

namespace fs = std::filesystem;

// Normalize separators (Windows uses backslash in .mtl paths)
static std::string normSep(std::string s) {
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

// ═════════════════════════════════════════════════════════════════════════════
//  Dispatch
// ═════════════════════════════════════════════════════════════════════════════
std::unique_ptr<Model> ModelLoader::load(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    LOG_INFO("ModelLoader: loading '" << path << "'");
    if (ext == ".obj")                    return loadOBJ (path);
    if (ext == ".gltf" || ext == ".glb") return loadGLTF(path);
    if (ext == ".blend")                 return BlendLoader::load(path);
    throw std::runtime_error(
        "ModelLoader: unsupported format '" + ext
        + "'. Supported: .obj  .gltf  .glb  .blend");
}

// ═════════════════════════════════════════════════════════════════════════════
//  OBJ  (tinyobjloader v1.x)
// ═════════════════════════════════════════════════════════════════════════════
std::unique_ptr<Model> ModelLoader::loadOBJ(const std::string& path) {
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      err;

    // baseDir is where the .obj lives — tinyobj uses it to find the .mtl
    std::string baseDir = normSep(fs::path(path).parent_path().string());
    if (baseDir.empty()) baseDir = ".";
    std::string baseDirSlash = baseDir + "/";

    bool ok = tinyobj::LoadObj(
        &attrib, &shapes, &materials,
        &err,
        path.c_str(),
        baseDirSlash.c_str(),   // mtl search path
        /*triangulate=*/true);

    // tinyobj puts both warnings and errors into 'err'
    if (!err.empty()) {
        // Only treat it as a fatal error if ok==false
        if (!ok) throw std::runtime_error("OBJ load failed [" + path + "]: " + err);
        LOG_WARN("OBJ [" << path << "]: " << err);
    }
    if (shapes.empty())
        throw std::runtime_error("OBJ has no shapes: " + path);

    // Log what .mtl files were found
    if (materials.empty())
        LOG_WARN("OBJ: no materials loaded for '" << path
                 << "' — check that the .mtl file is next to the .obj");
    else
        LOG_INFO("OBJ: " << materials.size() << " material(s) loaded");

    auto model       = std::make_unique<Model>();
    model->setPath(path);
    model->boundsMin = glm::vec3( 1e9f,  1e9f,  1e9f);
    model->boundsMax = glm::vec3(-1e9f, -1e9f, -1e9f);

    for (auto& shape : shapes) {
        std::vector<Vertex>       verts;
        std::vector<unsigned int> indices;
        verts.reserve(shape.mesh.indices.size());

        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            for (int v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];

                Vertex vert;
                vert.color = glm::vec4(1.f, 1.f, 1.f, 1.f);

                // Position
                vert.position = glm::vec3(
                    attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 2]);

                // Normal (optional)
                if (idx.normal_index >= 0)
                    vert.normal = glm::vec3(
                        attrib.normals[3 * size_t(idx.normal_index) + 0],
                        attrib.normals[3 * size_t(idx.normal_index) + 1],
                        attrib.normals[3 * size_t(idx.normal_index) + 2]);

                // UV — flip V so bottom-left origin matches OpenGL top-left
                if (idx.texcoord_index >= 0)
                    vert.uv = glm::vec2(
                        attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                        1.f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]);

                indices.push_back((unsigned int)verts.size());
                verts.push_back(vert);

                model->boundsMin = glm::min(model->boundsMin, vert.position);
                model->boundsMax = glm::max(model->boundsMax, vert.position);
            }
            indexOffset += fv;
        }

        // Generate flat normals when the .obj has none
        if (attrib.normals.empty()) {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                auto& a = verts[indices[i]];
                auto& b = verts[indices[i + 1]];
                auto& c = verts[indices[i + 2]];
                glm::vec3 n = glm::normalize(
                    glm::cross(b.position - a.position,
                               c.position - a.position));
                a.normal = b.normal = c.normal = n;
            }
        }

        // ── Material ─────────────────────────────────────────────────────
        Material mat;
        if (!shape.mesh.material_ids.empty()
            && shape.mesh.material_ids[0] >= 0
            && shape.mesh.material_ids[0] < (int)materials.size())
        {
            const auto& m = materials[(size_t)shape.mesh.material_ids[0]];
            mat.name      = m.name;

            // Diffuse color from .mtl  (Kd r g b)
            mat.albedo = glm::vec4(
                (float)m.diffuse[0],
                (float)m.diffuse[1],
                (float)m.diffuse[2], 1.f);

            // Shininess → roughness approximation
            mat.roughness = 1.f - glm::clamp(
                (float)m.shininess / 500.f, 0.f, 1.f);

            // Diffuse texture  (map_Kd)
            // Record raw path — ResourceManager will resolve + upload it.
            if (!m.diffuse_texname.empty()) {
                // tinyobj may give us a backslash path on Windows
                std::string rawTex = normSep(m.diffuse_texname);
                mat.diffuseTexturePath = rawTex;
                LOG_INFO("  mtl '" << m.name
                         << "' map_Kd = '" << rawTex << "'");
            } else {
                LOG_INFO("  mtl '" << m.name
                         << "' has no map_Kd (diffuse texture)");
            }
        } else {
            LOG_WARN("  shape '" << shape.name
                     << "' has no material assigned");
        }

        model->addSubMesh(Mesh(verts, indices), std::move(mat));
        LOG_INFO("  shape '" << shape.name
                 << "': " << verts.size() << " verts");
    }

    LOG_INFO("OBJ loaded: " << path
             << " (" << shapes.size() << " shapes, "
             << materials.size() << " materials)");
    return model;
}

// ═════════════════════════════════════════════════════════════════════════════
//  GLTF / GLB
// ═════════════════════════════════════════════════════════════════════════════

// Upload one tinygltf::Image into an OpenGL texture and return it.
// Results are cached by imageIndex so each unique image is uploaded only once.
static const Texture* uploadGLTFImage(
    const tinygltf::Image&                  img,
    int                                     imageIndex,
    const std::string&                      modelPath,
    std::unordered_map<int,const Texture*>& cache)
{
    auto it = cache.find(imageIndex);
    if (it != cache.end()) return it->second;

    const Texture* tex = nullptr;

    if (!eng::g_app) { cache[imageIndex] = nullptr; return nullptr; }
    ResourceManager& res = eng::g_app->resources();

    // ── Case 1: decoded pixels in img.image (GLB / data-URI) ────────────────
    if (!img.image.empty() && img.width > 0 && img.height > 0) {
        std::string key = "gltf://" + modelPath + "/img/" + std::to_string(imageIndex);
        tex = res.textureFromMemory(key,
                                    img.image.data(),
                                    img.width, img.height, img.component);
        LOG_INFO("  GLTF img[" << imageIndex << "] "
                 << img.width << "x" << img.height << " ch=" << img.component);
    }
    // ── Case 2: external URI ─────────────────────────────────────────────────
    else if (!img.uri.empty() && img.uri.find("data:") != 0) {
        std::string resolved = normSep(
            (fs::path(modelPath).parent_path() / img.uri).string());
        tex = res.texture(resolved);
        LOG_INFO("  GLTF img[" << imageIndex << "] external: " << resolved);
    }

    cache[imageIndex] = tex;
    return tex;
}

std::unique_ptr<Model> ModelLoader::loadGLTF(const std::string& path) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model    gltf;
    std::string        err, warn;

    // Tell tinygltf to decode images via stb_image (required for embedded textures)
    loader.SetImageLoader(
        [](tinygltf::Image* img, const int, std::string* err,
           std::string*, int, int,
           const unsigned char* bytes, int size, void*) -> bool {
            int w, h, ch;
            stbi_set_flip_vertically_on_load(0);
            unsigned char* px = stbi_load_from_memory(bytes, size, &w, &h, &ch, 0);
            if (!px) {
                if (err) *err = stbi_failure_reason();
                return false;
            }
            img->width     = w;
            img->height    = h;
            img->component = ch;
            img->bits      = 8;
            img->image.assign(px, px + w * h * ch);
            stbi_image_free(px);
            return true;
        }, nullptr);

    bool ok = (fs::path(path).extension() == ".glb")
        ? loader.LoadBinaryFromFile(&gltf, &err, &warn, path)
        : loader.LoadASCIIFromFile (&gltf, &err, &warn, path);

    if (!warn.empty()) LOG_WARN("GLTF [" << path << "]: " << warn);
    if (!ok) throw std::runtime_error("GLTF load failed [" + path + "]: " + err);

    auto model       = std::make_unique<Model>();
    model->setPath(path);
    model->boundsMin = glm::vec3( 1e9f,  1e9f,  1e9f);
    model->boundsMax = glm::vec3(-1e9f, -1e9f, -1e9f);

    // Per-model image cache — each unique image uploaded to GPU exactly once
    std::unordered_map<int,const Texture*> imageCache;

    // Returns a typed pointer into a buffer-view's data
    auto accessorPtr = [&](int accIdx) -> const uint8_t* {
        const auto& acc = gltf.accessors [(size_t)accIdx];
        const auto& bv  = gltf.bufferViews[(size_t)acc.bufferView];
        const auto& buf = gltf.buffers   [(size_t)bv.buffer];
        return buf.data.data() + bv.byteOffset + acc.byteOffset;
    };

    for (auto& gMesh : gltf.meshes) {
        for (auto& prim : gMesh.primitives) {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES && prim.mode != -1)
                continue;
            if (!prim.attributes.count("POSITION")) continue;

            std::vector<Vertex>       verts;
            std::vector<unsigned int> indices;

            // Positions — GLTF is right-handed Y-up, flip Z to match engine
            {
                const auto& acc  = gltf.accessors[(size_t)
                    prim.attributes.at("POSITION")];
                const float* d   = reinterpret_cast<const float*>(
                    accessorPtr(prim.attributes.at("POSITION")));
                verts.resize(acc.count);
                for (size_t i = 0; i < acc.count; ++i) {
                    verts[i].position = glm::vec3(d[i*3], d[i*3+1], -d[i*3+2]);
                    verts[i].color    = glm::vec4(1.f, 1.f, 1.f, 1.f);
                    model->boundsMin  = glm::min(model->boundsMin, verts[i].position);
                    model->boundsMax  = glm::max(model->boundsMax, verts[i].position);
                }
            }
            if (verts.empty()) continue;

            // Normals — flip Z to match
            if (prim.attributes.count("NORMAL")) {
                const float* d = reinterpret_cast<const float*>(
                    accessorPtr(prim.attributes.at("NORMAL")));
                for (size_t i = 0; i < verts.size(); ++i)
                    verts[i].normal = glm::vec3(d[i*3], d[i*3+1], -d[i*3+2]);
            }

            // UVs — GLTF UV origin is top-left, OpenGL is bottom-left → flip V
            if (prim.attributes.count("TEXCOORD_0")) {
                const float* d = reinterpret_cast<const float*>(
                    accessorPtr(prim.attributes.at("TEXCOORD_0")));
                for (size_t i = 0; i < verts.size(); ++i)
                    verts[i].uv = glm::vec2(d[i*2], 1.f - d[i*2+1]);
            }

            // Index buffer
            if (prim.indices >= 0) {
                const auto& acc = gltf.accessors[(size_t)prim.indices];
                const uint8_t* raw = accessorPtr(prim.indices);
                indices.reserve(acc.count);
                for (size_t i = 0; i < acc.count; ++i) {
                    switch (acc.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        indices.push_back(
                            reinterpret_cast<const uint32_t*>(raw)[i]); break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        indices.push_back(
                            reinterpret_cast<const uint16_t*>(raw)[i]); break;
                    default:
                        indices.push_back(
                            reinterpret_cast<const uint8_t*>(raw)[i]);  break;
                    }
                }
            } else {
                for (unsigned int i = 0; i < (unsigned int)verts.size(); ++i)
                    indices.push_back(i);
            }

            // Flipping Z mirrors the geometry, which reverses triangle winding
            // CCW → CW. Swap index 1 and 2 of each triangle to restore CCW.
            for (size_t i = 0; i + 2 < indices.size(); i += 3)
                std::swap(indices[i + 1], indices[i + 2]);

            // Material
            Material mat;
            if (prim.material >= 0
                && prim.material < (int)gltf.materials.size())
            {
                const auto& gm  = gltf.materials[(size_t)prim.material];
                mat.name        = gm.name;
                const auto& pbr = gm.pbrMetallicRoughness;
                mat.albedo = glm::vec4(
                    (float)pbr.baseColorFactor[0],
                    (float)pbr.baseColorFactor[1],
                    (float)pbr.baseColorFactor[2],
                    (float)pbr.baseColorFactor[3]);
                mat.metallic  = (float)pbr.metallicFactor;
                mat.roughness = (float)pbr.roughnessFactor;

                // Base-color texture — handles both embedded pixels and URIs
                if (pbr.baseColorTexture.index >= 0) {
                    const auto& tex = gltf.textures[
                        (size_t)pbr.baseColorTexture.index];
                    if (tex.source >= 0 &&
                        tex.source < (int)gltf.images.size())
                        mat.albedoMap = uploadGLTFImage(
                            gltf.images[(size_t)tex.source],
                            tex.source, path, imageCache);
                }
            }

            model->addSubMesh(Mesh(verts, indices), std::move(mat));
        }
    }

    if (model->subMeshCount() == 0)
        throw std::runtime_error("GLTF has no usable meshes: " + path);

    LOG_INFO("GLTF loaded: " << path
             << " (" << gltf.meshes.size() << " meshes, "
             << model->subMeshCount() << " submeshes)");
    return model;
}

// ═════════════════════════════════════════════════════════════════════════════
//  loadFromMemory — dispatch by extension of virtual path
// ═════════════════════════════════════════════════════════════════════════════
std::unique_ptr<Model> ModelLoader::loadFromMemory(const std::string&   virtualPath,
                                                    const unsigned char* data,
                                                    unsigned int         size)
{
    std::string ext = fs::path(virtualPath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    LOG_INFO("ModelLoader [embed]: '" << virtualPath << "'  (" << size << " bytes)");

    if (ext == ".obj")                    return loadOBJFromMemory (virtualPath, data, size);
    if (ext == ".gltf" || ext == ".glb") return loadGLTFFromMemory(virtualPath, data, size);
    if (ext == ".blend")                 return BlendLoader::loadFromMemory(virtualPath, data, size);

    throw std::runtime_error(
        "ModelLoader: unsupported format '" + ext + "' in embedded asset");
}

// ── OBJ from memory ───────────────────────────────────────────────────────────
// tinyobjloader v1.x supports parsing from a string via LoadObj overload.
std::unique_ptr<Model> ModelLoader::loadOBJFromMemory(const std::string&   name,
                                                       const unsigned char* data,
                                                       unsigned int         size)
{
    // Convert OBJ buffer to string stream
    std::string        objStr(reinterpret_cast<const char*>(data), size);
    std::istringstream objStream(objStr);

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      err;

    // ── Find embedded .mtl file ───────────────────────────────────────────
    // The OBJ says "mtllib Untitled.mtl" — we look for that file in the
    // embedded registry using the same directory as the OBJ.
    //
    // Strategy:
    //   1. Derive the .mtl path: same dir as OBJ, same stem, ".mtl" ext.
    //   2. Also try every "assets/.../*.mtl" in the registry (fuzzy match).
    //   3. If found → create a MaterialStreamReader from its data.
    //   4. If not found → parse without materials (colors will be white).

    auto& reg = EmbeddedRegistry::get();

    // Build candidate MTL virtual paths
    std::string objDir  = normSep(fs::path(name).parent_path().string());
    std::string objStem = fs::path(name).stem().string();

    std::vector<std::string> mtlCandidates = {
        // Same stem as OBJ: "assets/models/Untitled.mtl"
        objDir + "/" + objStem + ".mtl",
        // Try reading "mtllib" line from the OBJ text to get actual name
    };

    // Also parse "mtllib <filename>" from the OBJ text to get the real name
    {
        std::istringstream scan(objStr);
        std::string line;
        while (std::getline(scan, line)) {
            if (line.size() > 7 && line.substr(0, 7) == "mtllib ") {
                std::string mtlFile = line.substr(7);
                // strip trailing whitespace/CR
                while (!mtlFile.empty() &&
                       (mtlFile.back() == '\r' || mtlFile.back() == '\n' ||
                        mtlFile.back() == ' '))
                    mtlFile.pop_back();
                // Candidate: same dir as OBJ + filename from mtllib line
                mtlCandidates.insert(mtlCandidates.begin(),
                    objDir + "/" + normSep(mtlFile));
                LOG_INFO("OBJ [embed] mtllib line: '" << mtlFile << "'");
                break; // only need the first mtllib
            }
        }
    }

    // Search registry for each candidate
    std::string mtlStr;
    for (auto& c : mtlCandidates) {
        if (const EmbeddedEntry* me = reg.find(c)) {
            mtlStr = std::string(
                reinterpret_cast<const char*>(me->data), me->size);
            LOG_INFO("OBJ [embed] MTL found: " << c
                     << " (" << me->size << " bytes)");
            break;
        }
    }

    if (mtlStr.empty()) {
        LOG_WARN("OBJ [embed] no MTL found for: " << name);
        LOG_WARN("  Make sure the .mtl file is in assets/ next to the .obj");
    }

    // Build material reader (or nullptr if no MTL)
    std::istringstream                    mtlStream(mtlStr);
    std::unique_ptr<tinyobj::MaterialStreamReader> matReaderOwned;
    tinyobj::MaterialStreamReader*        matReader = nullptr;
    if (!mtlStr.empty()) {
        matReaderOwned = std::make_unique<tinyobj::MaterialStreamReader>(mtlStream);
        matReader = matReaderOwned.get();
    }

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials,
                                &err, &objStream, matReader, true);

    if (!err.empty()) {
        if (!ok) throw std::runtime_error(
            "OBJ [embed] parse failed [" + name + "]: " + err);
        LOG_WARN("OBJ [embed] warning: " << err);
    }
    if (shapes.empty())
        throw std::runtime_error("OBJ [embed] has no shapes: " + name);

    auto model       = std::make_unique<Model>();
    model->setPath(name);
    model->boundsMin = glm::vec3( 1e9f,  1e9f,  1e9f);
    model->boundsMax = glm::vec3(-1e9f, -1e9f, -1e9f);

    for (auto& shape : shapes) {
        std::vector<Vertex>       verts;
        std::vector<unsigned int> indices;
        verts.reserve(shape.mesh.indices.size());

        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            for (int v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                Vertex vert;
                vert.color    = glm::vec4(1.f, 1.f, 1.f, 1.f);
                vert.position = glm::vec3(
                    attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 2]);
                if (idx.normal_index >= 0)
                    vert.normal = glm::vec3(
                        attrib.normals[3 * size_t(idx.normal_index) + 0],
                        attrib.normals[3 * size_t(idx.normal_index) + 1],
                        attrib.normals[3 * size_t(idx.normal_index) + 2]);
                if (idx.texcoord_index >= 0)
                    vert.uv = glm::vec2(
                        attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                        1.f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]);
                indices.push_back((unsigned int)verts.size());
                verts.push_back(vert);
                model->boundsMin = glm::min(model->boundsMin, vert.position);
                model->boundsMax = glm::max(model->boundsMax, vert.position);
            }
            indexOffset += fv;
        }

        // Flat normals if missing
        if (attrib.normals.empty()) {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                auto& a = verts[indices[i]];
                auto& b = verts[indices[i+1]];
                auto& c = verts[indices[i+2]];
                glm::vec3 n = glm::normalize(
                    glm::cross(b.position - a.position,
                               c.position - a.position));
                a.normal = b.normal = c.normal = n;
            }
        }

        Material mat;
        if (!shape.mesh.material_ids.empty()
            && shape.mesh.material_ids[0] >= 0
            && shape.mesh.material_ids[0] < (int)materials.size())
        {
            const auto& m = materials[(size_t)shape.mesh.material_ids[0]];
            mat.name  = m.name;
            mat.albedo = glm::vec4(
                (float)m.diffuse[0], (float)m.diffuse[1],
                (float)m.diffuse[2], 1.f);
            if (!m.diffuse_texname.empty())
                mat.diffuseTexturePath = normSep(m.diffuse_texname);
        }

        model->addSubMesh(Mesh(verts, indices), std::move(mat));
    }

    LOG_INFO("OBJ [embed] loaded: " << name
             << " (" << shapes.size() << " shapes)");
    return model;
}

// ── GLTF/GLB from memory ──────────────────────────────────────────────────────
std::unique_ptr<Model> ModelLoader::loadGLTFFromMemory(const std::string&   name,
                                                        const unsigned char* data,
                                                        unsigned int         size)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model    gltf;
    std::string        err, warn;
    bool ok;

    // Tell tinygltf to decode images via stb_image (required for embedded textures)
    loader.SetImageLoader(
        [](tinygltf::Image* img, const int, std::string* err,
           std::string*, int, int,
           const unsigned char* bytes, int size, void*) -> bool {
            int w, h, ch;
            stbi_set_flip_vertically_on_load(0);
            unsigned char* px = stbi_load_from_memory(bytes, size, &w, &h, &ch, 0);
            if (!px) {
                if (err) *err = stbi_failure_reason();
                return false;
            }
            img->width     = w;
            img->height    = h;
            img->component = ch;
            img->bits      = 8;
            img->image.assign(px, px + w * h * ch);
            stbi_image_free(px);
            return true;
        }, nullptr);

    // GLB = binary; GLTF = text JSON
    std::string ext = fs::path(name).extension().string();
    if (ext == ".glb")
        ok = loader.LoadBinaryFromMemory(&gltf, &err, &warn, data, size, ".");
    else
        ok = loader.LoadASCIIFromString (&gltf, &err, &warn,
             reinterpret_cast<const char*>(data), size, ".");

    if (!warn.empty()) LOG_WARN("GLTF [embed]: " << warn);
    if (!ok) throw std::runtime_error(
        "GLTF [embed] load failed [" + name + "]: " + err);

    auto model       = std::make_unique<Model>();
    model->setPath(name);
    model->boundsMin = glm::vec3( 1e9f,  1e9f,  1e9f);
    model->boundsMax = glm::vec3(-1e9f, -1e9f, -1e9f);

    std::unordered_map<int,const Texture*> imageCache;

    auto accessorPtr = [&](int accIdx) -> const uint8_t* {
        const auto& acc = gltf.accessors [(size_t)accIdx];
        const auto& bv  = gltf.bufferViews[(size_t)acc.bufferView];
        const auto& buf = gltf.buffers   [(size_t)bv.buffer];
        return buf.data.data() + bv.byteOffset + acc.byteOffset;
    };

    for (auto& gMesh : gltf.meshes) {
        for (auto& prim : gMesh.primitives) {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES && prim.mode != -1) continue;
            if (!prim.attributes.count("POSITION")) continue;

            std::vector<Vertex>       verts;
            std::vector<unsigned int> indices;

            // Positions — flip Z (GLTF right-handed → engine left-handed)
            {
                const auto& acc = gltf.accessors[(size_t)prim.attributes.at("POSITION")];
                const float* d  = reinterpret_cast<const float*>(
                    accessorPtr(prim.attributes.at("POSITION")));
                verts.resize(acc.count);
                for (size_t i = 0; i < acc.count; ++i) {
                    verts[i].position = glm::vec3(d[i*3], d[i*3+1], -d[i*3+2]);
                    verts[i].color    = glm::vec4(1.f, 1.f, 1.f, 1.f);
                    model->boundsMin  = glm::min(model->boundsMin, verts[i].position);
                    model->boundsMax  = glm::max(model->boundsMax, verts[i].position);
                }
            }
            if (verts.empty()) continue;

            // Normals — flip Z to match
            if (prim.attributes.count("NORMAL")) {
                const float* d = reinterpret_cast<const float*>(
                    accessorPtr(prim.attributes.at("NORMAL")));
                for (size_t i = 0; i < verts.size(); ++i)
                    verts[i].normal = glm::vec3(d[i*3], d[i*3+1], -d[i*3+2]);
            }

            // UVs — GLTF top-left origin → OpenGL bottom-left → flip V
            if (prim.attributes.count("TEXCOORD_0")) {
                const float* d = reinterpret_cast<const float*>(
                    accessorPtr(prim.attributes.at("TEXCOORD_0")));
                for (size_t i = 0; i < verts.size(); ++i)
                    verts[i].uv = glm::vec2(d[i*2], 1.f - d[i*2+1]);
            }

            if (prim.indices >= 0) {
                const auto& acc = gltf.accessors[(size_t)prim.indices];
                const uint8_t* raw = accessorPtr(prim.indices);
                indices.reserve(acc.count);
                for (size_t i = 0; i < acc.count; ++i) {
                    switch (acc.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        indices.push_back(reinterpret_cast<const uint32_t*>(raw)[i]); break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        indices.push_back(reinterpret_cast<const uint16_t*>(raw)[i]); break;
                    default:
                        indices.push_back(reinterpret_cast<const uint8_t*>(raw)[i]);  break;
                    }
                }
            } else {
                for (unsigned int i = 0; i < (unsigned int)verts.size(); ++i)
                    indices.push_back(i);
            }

            // Z-flip reverses winding CCW→CW; swap to restore CCW
            for (size_t i = 0; i + 2 < indices.size(); i += 3)
                std::swap(indices[i + 1], indices[i + 2]);

            Material mat;
            if (prim.material >= 0 && prim.material < (int)gltf.materials.size()) {
                const auto& gm  = gltf.materials[(size_t)prim.material];
                mat.name        = gm.name;
                const auto& pbr = gm.pbrMetallicRoughness;
                mat.albedo = glm::vec4(
                    (float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1],
                    (float)pbr.baseColorFactor[2], (float)pbr.baseColorFactor[3]);
                mat.metallic  = (float)pbr.metallicFactor;
                mat.roughness = (float)pbr.roughnessFactor;

                // Base-color texture — handles embedded pixels and URIs
                if (pbr.baseColorTexture.index >= 0) {
                    const auto& tex = gltf.textures[
                        (size_t)pbr.baseColorTexture.index];
                    if (tex.source >= 0 &&
                        tex.source < (int)gltf.images.size())
                        mat.albedoMap = uploadGLTFImage(
                            gltf.images[(size_t)tex.source],
                            tex.source, name, imageCache);
                }
            }

            model->addSubMesh(Mesh(verts, indices), std::move(mat));
        }
    }

    if (model->subMeshCount() == 0)
        throw std::runtime_error("GLTF [embed] has no usable meshes: " + name);

    LOG_INFO("GLTF [embed] loaded: " << name
             << " (" << model->subMeshCount() << " submeshes)");
    return model;
}
