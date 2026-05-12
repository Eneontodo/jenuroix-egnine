#pragma once
#include "render/Model.h"
#include <string>
#include <memory>

class ModelLoader {
public:
    // Load from file path (auto-detects .obj / .gltf / .glb / .blend)
    static std::unique_ptr<Model> load(const std::string& path);

    // Load from in-memory buffer (for embedded assets)
    // 'virtualPath' is only used to detect the format from extension.
    static std::unique_ptr<Model> loadFromMemory(const std::string& virtualPath,
                                                  const unsigned char* data,
                                                  unsigned int         size);
private:
    static std::unique_ptr<Model> loadOBJ (const std::string& path);
    static std::unique_ptr<Model> loadGLTF(const std::string& path);

    static std::unique_ptr<Model> loadOBJFromMemory (const std::string& name,
                                                      const unsigned char* data,
                                                      unsigned int size);
    static std::unique_ptr<Model> loadGLTFFromMemory(const std::string& name,
                                                      const unsigned char* data,
                                                      unsigned int size);
};
