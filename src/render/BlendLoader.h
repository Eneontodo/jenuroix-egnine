#pragma once
// BlendLoader.h
// Reads Blender .blend files and extracts meshes + diffuse texture paths.
//
// Supported Blender versions: 2.80 - 4.x  (pointer size 4 or 8, little/big endian)
//
// What we parse:
//   - File header   : magic, pointer size, endian, version
//   - SDNA block    : struct/field names and types (needed to locate field offsets)
//   - ME blocks     : Mesh data-blocks
//   - MVert         : vertex positions + normals  (legacy format, Blender ≤ 3.5)
//   - MLoop         : loop vertex indices
//   - MLoopUV       : loop UV coordinates         (legacy format)
//   - MPoly         : polygon (face) definitions
//   - CD_MLOOPUV    : CustomData layer for UVs    (Blender ≥ 3.x)
//   - image paths   : diffuse texture filenames embedded in material Image blocks
//
// Limitations (not fatal — the loader degrades gracefully):
//   - Only the *first* UV layer is read
//   - Only the *first* material's diffuse image path is extracted
//   - Armature / shape-key / hair / curves / lights are ignored
//   - Compressed .blend files (gzip) are NOT supported — export uncompressed

#include "render/Model.h"
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <unordered_map>

// ─── Public API ──────────────────────────────────────────────────────────────

class BlendLoader {
public:
    // Load from disk
    static std::unique_ptr<Model> load(const std::string& path);

    // Load from in-memory buffer
    static std::unique_ptr<Model> loadFromMemory(const std::string& virtualPath,
                                                  const unsigned char* data,
                                                  uint32_t             size);
private:
    // ── Internal parser state ────────────────────────────────────────────────
    struct FileBlock {
        std::string  code;      // 4-byte block code ("OB","ME","MA","IM", …)
        uint32_t     size;      // size of data after header
        uint64_t     oldPtr;    // old memory address (4 or 8 bytes in file)
        int32_t      sdnaIndex; // index into SDNA struct array
        int32_t      count;     // number of structs in this block
        uint32_t     dataOffset;// byte offset of block's data in the raw buffer
    };

    struct SDNAField {
        std::string typeName;
        std::string name;       // may start with * (pointer) or [] (array)
        uint16_t    typeSize;
        int         offset;     // byte offset from start of struct
    };

    struct SDNAStruct {
        std::string             name;
        int                     size;
        std::vector<SDNAField>  fields;
    };

    // Main internal state
    const unsigned char*                   m_data    = nullptr;
    uint32_t                               m_size    = 0;
    bool                                   m_64bit   = true;
    bool                                   m_bigEndian = false;
    int                                    m_version = 0;
    std::string                            m_virtualPath;

    std::vector<FileBlock>                 m_blocks;
    std::vector<SDNAStruct>                m_structs;
    std::unordered_map<std::string,int>    m_structByName;
    // old-pointer → block index, for pointer-following
    std::unordered_map<uint64_t,size_t>    m_ptrToBlock;

    // ── Parse pipeline ───────────────────────────────────────────────────────
    bool parseHeader();
    bool parseCatalog();   // first pass: collect all block headers
    bool parseSDNA();      // parse the SDNA block
    std::unique_ptr<Model> buildModel();

    // ── Mesh extraction ──────────────────────────────────────────────────────
    bool extractMesh(const FileBlock&             meBlock,
                     std::vector<Vertex>&         outVerts,
                     std::vector<unsigned int>&   outIdx,
                     std::string&                 outTexPath,
                     std::vector<unsigned char>&  outPackedData);

    // Helpers that read from arbitrary SDNA structs in a block
    int   fieldOffset(const std::string& structName,
                      const std::string& fieldName) const;
    int   structSize (const std::string& structName) const;

    // Endian-aware integer reads
    uint16_t read16(uint32_t off) const;
    uint32_t read32(uint32_t off) const;
    uint64_t read64(uint32_t off) const;
    float    readF32(uint32_t off) const;
    uint64_t readPtr(uint32_t off) const; // reads 4 or 8 bytes based on m_64bit

    // Block lookup by old pointer
    const FileBlock* blockByPtr(uint64_t ptr) const;
};
