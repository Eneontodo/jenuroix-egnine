// BlendLoader.cpp
// Parses Blender .blend files to extract mesh geometry + diffuse texture paths.
// See BlendLoader.h for format notes and limitations.

#include "render/BlendLoader.h"
#include "render/Mesh.h"
#include "core/Log.h"

#include <cstring>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <filesystem>

#include <glm/glm.hpp>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
//  Public entry points
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<Model> BlendLoader::load(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open())
        throw std::runtime_error("BlendLoader: cannot open '" + path + "'");

    std::streamsize fsize = f.tellg();
    f.seekg(0);
    std::vector<unsigned char> buf(static_cast<size_t>(fsize));
    f.read(reinterpret_cast<char*>(buf.data()), fsize);

    return loadFromMemory(path, buf.data(), static_cast<uint32_t>(buf.size()));
}

std::unique_ptr<Model> BlendLoader::loadFromMemory(const std::string&   virtualPath,
                                                    const unsigned char* data,
                                                    uint32_t             size)
{
    BlendLoader loader;
    loader.m_data        = data;
    loader.m_size        = size;
    loader.m_virtualPath = virtualPath;

    if (!loader.parseHeader()) {
        // Maybe gzip-compressed?
        if (size > 2 && data[0] == 0x1f && data[1] == 0x8b)
            throw std::runtime_error(
                "BlendLoader: '" + virtualPath + "' is gzip-compressed. "
                "Please save the .blend file with File > Save As and disable "
                "compression (Edit > Preferences > Save & Load > Compress File).");
        throw std::runtime_error("BlendLoader: '" + virtualPath + "' is not a valid .blend file.");
    }
    if (!loader.parseCatalog())
        throw std::runtime_error("BlendLoader: block catalog parse failed: " + virtualPath);
    if (!loader.parseSDNA())
        throw std::runtime_error("BlendLoader: SDNA parse failed: " + virtualPath);

    return loader.buildModel();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Endian-aware reads
// ─────────────────────────────────────────────────────────────────────────────

static uint16_t swap16(uint16_t v) { return (v>>8)|(v<<8); }
static uint32_t swap32(uint32_t v) {
    return ((v>>24)&0xFF)|((v>>8)&0xFF00)|((v<<8)&0xFF0000)|((v<<24)&0xFF000000u);
}
static uint64_t swap64(uint64_t v) {
    return  ((v>>56)&0xFFULL)|((v>>40)&0xFF00ULL)|((v>>24)&0xFF0000ULL)|
            ((v>>8 )&0xFF000000ULL)|((v<<8 )&0xFF00000000ULL)|
            ((v<<24)&0xFF0000000000ULL)|((v<<40)&0xFF000000000000ULL)|
            ((v<<56)&0xFF00000000000000ULL);
}

uint16_t BlendLoader::read16(uint32_t off) const {
    if (off + 2 > m_size) return 0;
    uint16_t v; memcpy(&v, m_data + off, 2);
    return m_bigEndian ? swap16(v) : v;
}
uint32_t BlendLoader::read32(uint32_t off) const {
    if (off + 4 > m_size) return 0;
    uint32_t v; memcpy(&v, m_data + off, 4);
    return m_bigEndian ? swap32(v) : v;
}
uint64_t BlendLoader::read64(uint32_t off) const {
    if (off + 8 > m_size) return 0;
    uint64_t v; memcpy(&v, m_data + off, 8);
    return m_bigEndian ? swap64(v) : v;
}
float BlendLoader::readF32(uint32_t off) const {
    uint32_t u = read32(off);
    float f; memcpy(&f, &u, 4);
    return f;
}
uint64_t BlendLoader::readPtr(uint32_t off) const {
    return m_64bit ? read64(off) : (uint64_t)read32(off);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Header — "BLENDER_v280" style magic
// ─────────────────────────────────────────────────────────────────────────────

bool BlendLoader::parseHeader()
{
    if (m_size < 12) return false;
    // Magic: "BLENDER"
    if (memcmp(m_data, "BLENDER", 7) != 0) return false;

    char ptrChar = static_cast<char>(m_data[7]);
    m_64bit = (ptrChar == '-'); // '-' = 8-byte, '_' = 4-byte

    char endChar = static_cast<char>(m_data[8]);
    m_bigEndian  = (endChar == 'V'); // 'V' = big, 'v' = little

    // Version: 3 digits e.g. "280"
    char ver[4] = {};
    memcpy(ver, m_data + 9, 3);
    m_version = atoi(ver);

    LOG_INFO("BlendLoader: ptr=" << (m_64bit ? 64 : 32)
             << "bit  endian=" << (m_bigEndian ? "BE" : "LE")
             << "  version=" << m_version);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Block catalog — read all block headers
// ─────────────────────────────────────────────────────────────────────────────

bool BlendLoader::parseCatalog()
{
    const int ptrSize = m_64bit ? 8 : 4;
    // Block header layout (after 12-byte file header):
    //   4 bytes  code
    //   4 bytes  size  (data after header)
    //   ptrSize  old pointer
    //   4 bytes  sdna index
    //   4 bytes  count
    const int blkHdrSize = 4 + 4 + ptrSize + 4 + 4;

    uint32_t pos = 12; // skip file header

    while (pos + blkHdrSize <= m_size) {
        char code[5] = {};
        memcpy(code, m_data + pos, 4);
        std::string codeStr(code);
        // Strip trailing spaces/nulls
        while (!codeStr.empty() &&
               (codeStr.back() == ' ' || codeStr.back() == '\0'))
            codeStr.pop_back();

        uint32_t blkDataSize = read32(pos + 4);
        uint64_t oldPtr      = readPtr(pos + 8);
        int32_t  sdnaIdx     = (int32_t)read32(pos + 8 + ptrSize);
        int32_t  count       = (int32_t)read32(pos + 8 + ptrSize + 4);
        uint32_t dataOff     = pos + blkHdrSize;

        FileBlock blk;
        blk.code       = codeStr;
        blk.size       = blkDataSize;
        blk.oldPtr     = oldPtr;
        blk.sdnaIndex  = sdnaIdx;
        blk.count      = count;
        blk.dataOffset = dataOff;

        m_blocks.push_back(blk);
        if (oldPtr != 0)
            m_ptrToBlock[oldPtr] = m_blocks.size() - 1;

        if (codeStr == "ENDB") break;

        pos = dataOff + blkDataSize;
        // Align to 4 bytes (older blends may not, but modern ones do)
        // No alignment padding in standard .blend — skip nothing
    }

    LOG_INFO("BlendLoader: " << m_blocks.size() << " blocks found.");
    return !m_blocks.empty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  SDNA — struct name/type/field database
// ─────────────────────────────────────────────────────────────────────────────

bool BlendLoader::parseSDNA()
{
    // Find the "DNA1" block
    const FileBlock* dnaBlock = nullptr;
    for (auto& b : m_blocks)
        if (b.code == "DNA1") { dnaBlock = &b; break; }
    if (!dnaBlock) { LOG_WARN("BlendLoader: no DNA1 block!"); return false; }

    const uint8_t* d   = m_data + dnaBlock->dataOffset;
    const uint8_t* end = d + dnaBlock->size;

    // SDNA layout:
    //   "SDNA"
    //   "NAME" + int numNames + null-terminated name strings
    //   "TYPE" + int numTypes + null-terminated type strings
    //   "TLEN" + int numTypes + uint16 sizes (aligned to 4)
    //   "STRC" + int numStructs + (typeIdx, numFields, [typeIdx,nameIdx]...)

    uint32_t pos = 0;
    auto peek4 = [&](uint32_t p) -> bool {
        return (p + 4 <= (uint32_t)(end - d)) &&
               memcmp(d + p, nullptr, 0) == 0; // just bounds check
    };
    (void)peek4;

    if ((uint32_t)(end - d) < 4 || memcmp(d, "SDNA", 4) != 0) {
        LOG_WARN("BlendLoader: DNA1 block missing SDNA magic");
        return false;
    }
    pos = 4;

    // Helper: align pos to 4
    auto align4 = [&]() { pos = (pos + 3) & ~3u; };

    // --- NAME section ---
    if (memcmp(d + pos, "NAME", 4) != 0) return false;
    pos += 4;
    uint32_t numNames = 0; memcpy(&numNames, d + pos, 4); if (m_bigEndian) numNames = swap32(numNames);
    pos += 4;
    std::vector<std::string> names(numNames);
    for (uint32_t i = 0; i < numNames; ++i) {
        const char* s = reinterpret_cast<const char*>(d + pos);
        names[i] = s;
        pos += (uint32_t)strlen(s) + 1;
    }
    align4();

    // --- TYPE section ---
    if (memcmp(d + pos, "TYPE", 4) != 0) return false;
    pos += 4;
    uint32_t numTypes = 0; memcpy(&numTypes, d + pos, 4); if (m_bigEndian) numTypes = swap32(numTypes);
    pos += 4;
    std::vector<std::string> types(numTypes);
    for (uint32_t i = 0; i < numTypes; ++i) {
        const char* s = reinterpret_cast<const char*>(d + pos);
        types[i] = s;
        pos += (uint32_t)strlen(s) + 1;
    }
    align4();

    // --- TLEN section ---
    if (memcmp(d + pos, "TLEN", 4) != 0) return false;
    pos += 4;
    std::vector<uint16_t> typeSizes(numTypes);
    for (uint32_t i = 0; i < numTypes; ++i) {
        uint16_t sz; memcpy(&sz, d + pos, 2);
        if (m_bigEndian) sz = swap16(sz);
        typeSizes[i] = sz;
        pos += 2;
    }
    align4();

    // --- STRC section ---
    if (memcmp(d + pos, "STRC", 4) != 0) return false;
    pos += 4;
    uint32_t numStructs = 0; memcpy(&numStructs, d + pos, 4); if (m_bigEndian) numStructs = swap32(numStructs);
    pos += 4;

    m_structs.resize(numStructs);
    for (uint32_t si = 0; si < numStructs; ++si) {
        uint16_t typeIdx; memcpy(&typeIdx, d + pos, 2); if (m_bigEndian) typeIdx = swap16(typeIdx); pos += 2;
        uint16_t numFields; memcpy(&numFields, d + pos, 2); if (m_bigEndian) numFields = swap16(numFields); pos += 2;

        SDNAStruct& st = m_structs[si];
        st.name = (typeIdx < numTypes) ? types[typeIdx] : "?";
        st.size = (typeIdx < typeSizes.size()) ? typeSizes[typeIdx] : 0;
        st.fields.resize(numFields);

        int fieldOff = 0;
        for (uint16_t fi = 0; fi < numFields; ++fi) {
            uint16_t fTypeIdx; memcpy(&fTypeIdx, d + pos, 2); if (m_bigEndian) fTypeIdx = swap16(fTypeIdx); pos += 2;
            uint16_t fNameIdx; memcpy(&fNameIdx, d + pos, 2); if (m_bigEndian) fNameIdx = swap16(fNameIdx); pos += 2;

            SDNAField& fld = st.fields[fi];
            fld.typeName = (fTypeIdx < numTypes) ? types[fTypeIdx] : "?";
            fld.name     = (fNameIdx < numNames)  ? names[fNameIdx] : "?";
            fld.typeSize = (fTypeIdx < typeSizes.size()) ? typeSizes[fTypeIdx] : 0;
            fld.offset   = fieldOff;

            // Compute size of this field (handles pointers and arrays)
            int fieldSize = fld.typeSize;
            const std::string& nm = fld.name;
            // pointer: starts with '*' or "**"
            if (!nm.empty() && nm[0] == '*') {
                fieldSize = m_64bit ? 8 : 4;
                // ** = pointer to pointer → same size
            }
            // array: name contains '[N]', e.g. "co[3]" → 3 × typeSize
            size_t lbrace = nm.find('[');
            if (lbrace != std::string::npos && nm[0] != '*') {
                int arrCount = atoi(nm.c_str() + lbrace + 1);
                if (arrCount > 1) fieldSize = fld.typeSize * arrCount;
            }

            fieldOff += fieldSize;
        }
        m_structByName[st.name] = (int)si;
    }

    LOG_INFO("BlendLoader: SDNA parsed — " << numStructs << " structs.");
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SDNA helpers
// ─────────────────────────────────────────────────────────────────────────────

int BlendLoader::fieldOffset(const std::string& structName,
                              const std::string& fieldName) const
{
    auto it = m_structByName.find(structName);
    if (it == m_structByName.end()) return -1;
    const SDNAStruct& st = m_structs[(size_t)it->second];
    for (auto& f : st.fields) {
        // Strip leading '*' and array suffix for comparison
        std::string nm = f.name;
        while (!nm.empty() && nm[0] == '*') nm.erase(nm.begin());
        size_t lb = nm.find('[');
        if (lb != std::string::npos) nm = nm.substr(0, lb);
        if (nm == fieldName) return f.offset;
    }
    return -1;
}

int BlendLoader::structSize(const std::string& structName) const
{
    auto it = m_structByName.find(structName);
    if (it == m_structByName.end()) return -1;
    return m_structs[(size_t)it->second].size;
}

const BlendLoader::FileBlock* BlendLoader::blockByPtr(uint64_t ptr) const
{
    auto it = m_ptrToBlock.find(ptr);
    if (it == m_ptrToBlock.end()) return nullptr;
    return &m_blocks[it->second];
}

// ─────────────────────────────────────────────────────────────────────────────
//  Build model from all ME (Mesh) blocks
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<Model> BlendLoader::buildModel()
{
    auto model = std::make_unique<Model>();
    model->setPath(m_virtualPath);
    model->boundsMin = glm::vec3( 1e9f,  1e9f,  1e9f);
    model->boundsMax = glm::vec3(-1e9f, -1e9f, -1e9f);

    int meshCount = 0;
    for (auto& blk : m_blocks) {
        if (blk.code != "ME") continue;
        if (blk.dataOffset + blk.size > m_size) continue;

        std::vector<Vertex>       verts;
        std::vector<unsigned int> idx;
        std::string               texPath;
        std::vector<unsigned char> packedData;

        try {
            if (extractMesh(blk, verts, idx, texPath, packedData)) {
                Material mat;
                mat.name = "blend_mesh_" + std::to_string(meshCount);

                // Packed image bytes take priority over external filepath
                if (!packedData.empty()) {
                    mat.packedImageData = std::move(packedData);
                    LOG_INFO("BlendLoader: mesh #" << meshCount
                             << " has packed texture ("
                             << mat.packedImageData.size() << " bytes)");
                } else if (!texPath.empty()) {
                    mat.diffuseTexturePath = texPath;
                    LOG_INFO("BlendLoader: mesh #" << meshCount
                             << " texture path = '" << texPath << "'");
                }

                for (auto& v : verts) {
                    model->boundsMin = glm::min(model->boundsMin, v.position);
                    model->boundsMax = glm::max(model->boundsMax, v.position);
                }

                model->addSubMesh(Mesh(verts, idx), std::move(mat));
                ++meshCount;
                LOG_INFO("BlendLoader: mesh #" << meshCount
                         << " — " << verts.size() << " verts, "
                         << idx.size() / 3 << " tris");
            }
        } catch (const std::exception& e) {
            LOG_WARN("BlendLoader: mesh block error — " << e.what());
        }
    }

    if (meshCount == 0) {
        LOG_WARN("BlendLoader: no meshes found in '" << m_virtualPath << "'");
        LOG_WARN("  Make sure the scene has at least one mesh object.");
        // Return a fallback cube
        model->addSubMesh(Mesh::makeCube(1.f));
    }

    LOG_INFO("BlendLoader: loaded '" << m_virtualPath
             << "' — " << meshCount << " mesh(es).");
    return model;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Extract a single ME block
//
//  Blender internal mesh (post-2.80, pre-4.1 attribute system):
//
//   Mesh {
//     int totvert, totedge, totpoly, totloop;
//     MVert   *mvert;       // positions + normals (short[3])
//     MEdge   *medge;
//     MPoly   *mpoly;       // loopstart + totloop
//     MLoop   *mloop;       // v (vertex index) + e (edge index)
//     CustomDataLayer *ldata; // loop custom data (UVs etc.)
//     ...
//   }
//
//  Blender 4.0+: BMesh-based format, vertex positions in CustomData layers.
//  We handle both via fallback.
// ─────────────────────────────────────────────────────────────────────────────

bool BlendLoader::extractMesh(const FileBlock&           meBlock,
                               std::vector<Vertex>&       outVerts,
                               std::vector<unsigned int>& outIdx,
                               std::string&               outTexPath,
                               std::vector<unsigned char>& outPackedData)
{
    // ── Read Mesh struct fields ───────────────────────────────────────────────
    const uint32_t base = meBlock.dataOffset;

    // totvert / totpoly / totloop
    int offTotvert = fieldOffset("Mesh", "totvert");
    int offTotpoly = fieldOffset("Mesh", "totpoly");
    int offTotloop = fieldOffset("Mesh", "totloop");
    // Blender 4.x renamed totvert → verts_num etc., try both
    if (offTotvert < 0) offTotvert = fieldOffset("Mesh", "verts_num");
    if (offTotpoly < 0) offTotpoly = fieldOffset("Mesh", "faces_num");
    if (offTotloop < 0) offTotloop = fieldOffset("Mesh", "loops_num");

    // Pointer fields
    int offMvert  = fieldOffset("Mesh", "mvert");
    int offMpoly  = fieldOffset("Mesh", "mpoly");
    int offMloop  = fieldOffset("Mesh", "mloop");

    int totvert = (offTotvert >= 0) ? (int)read32(base + offTotvert) : 0;
    int totpoly = (offTotpoly >= 0) ? (int)read32(base + offTotpoly) : 0;
    int totloop = (offTotloop >= 0) ? (int)read32(base + offTotloop) : 0;

    if (totvert <= 0 || totpoly <= 0) return false;

    LOG_INFO("BlendLoader ME: " << totvert << " verts, "
             << totpoly << " polys, " << totloop << " loops");

    // ── Vertex positions (MVert blocks) ───────────────────────────────────────
    // Follow mvert pointer → find the MVert block
    std::vector<glm::vec3> positions(totvert, glm::vec3(0));
    std::vector<glm::vec3> normals(totvert, glm::vec3(0,1,0));

    if (offMvert >= 0) {
        uint64_t mvertPtr = readPtr(base + offMvert);
        const FileBlock* mvertBlk = blockByPtr(mvertPtr);
        if (mvertBlk) {
            int mvertSize = structSize("MVert");
            if (mvertSize <= 0) mvertSize = 20; // fallback: float[3] + short[3] + char[2] = 20 bytes

            int offCo = fieldOffset("MVert", "co");
            int offNo = fieldOffset("MVert", "no");
            if (offCo < 0) offCo = 0; // first field is co[3]
            if (offNo < 0) offNo = 12; // after 3×float

            for (int i = 0; i < totvert && i < mvertBlk->count; ++i) {
                uint32_t vbase = mvertBlk->dataOffset + (uint32_t)(i * mvertSize);
                positions[i] = glm::vec3(
                    readF32(vbase + offCo + 0),
                    readF32(vbase + offCo + 4),
                    readF32(vbase + offCo + 8));

                // Normals stored as signed short[3], range −32767..32767 → −1..1
                if (offNo >= 0) {
                    float nx = (float)(int16_t)read16(vbase + offNo + 0) / 32767.f;
                    float ny = (float)(int16_t)read16(vbase + offNo + 2) / 32767.f;
                    float nz = (float)(int16_t)read16(vbase + offNo + 4) / 32767.f;
                    float len = std::sqrt(nx*nx + ny*ny + nz*nz);
                    if (len > 1e-6f) normals[i] = glm::vec3(nx/len, ny/len, nz/len);
                }
            }
        }
    }

    // ── UV coordinates (MLoopUV) ──────────────────────────────────────────────
    // Blender stores UVs per-loop.  We look for:
    //   1. MLoopUV blocks (legacy: each block corresponds to a UV layer)
    //   2. CustomData layer with type CD_MLOOPUV (=16) inside the ldata field
    std::vector<glm::vec2> loopUVs(totloop, glm::vec2(0));
    bool hasUV = false;

    // Try MLoopUV blocks directly
    int mloopuvSize = structSize("MLoopUV");
    if (mloopuvSize <= 0) mloopuvSize = 12; // uv[2]=8 + flag=4
    int offUV = fieldOffset("MLoopUV", "uv");
    if (offUV < 0) offUV = 0;

    for (auto& blk : m_blocks) {
        if (blk.code != "DATA" && blk.code != "DATB") continue;
        if (blk.sdnaIndex < 0 || blk.sdnaIndex >= (int)m_structs.size()) continue;
        if (m_structs[(size_t)blk.sdnaIndex].name != "MLoopUV") continue;
        if (blk.count < totloop) continue;

        for (int i = 0; i < totloop; ++i) {
            uint32_t uvBase = blk.dataOffset + (uint32_t)(i * mloopuvSize);
            loopUVs[i] = glm::vec2(
                readF32(uvBase + offUV),
                1.f - readF32(uvBase + offUV + 4)); // flip V
        }
        hasUV = true;
        LOG_INFO("BlendLoader: UV layer found (MLoopUV block).");
        break;
    }

    // ── MLoop — loop vertex indices ───────────────────────────────────────────
    std::vector<int> loopVert(totloop, 0);
    if (offMloop >= 0) {
        uint64_t mloopPtr = readPtr(base + offMloop);
        const FileBlock* mloopBlk = blockByPtr(mloopPtr);
        if (mloopBlk) {
            int mloopSize = structSize("MLoop");
            if (mloopSize <= 0) mloopSize = 8; // v(int) + e(int)
            int offV = fieldOffset("MLoop", "v");
            if (offV < 0) offV = 0;

            for (int i = 0; i < totloop && i < mloopBlk->count; ++i) {
                uint32_t lb = mloopBlk->dataOffset + (uint32_t)(i * mloopSize);
                loopVert[i] = (int)read32(lb + offV);
            }
        }
    }

    // ── MPoly — polygon definitions ───────────────────────────────────────────
    struct Poly { int loopstart; int totloop; };
    std::vector<Poly> polys(totpoly);
    if (offMpoly >= 0) {
        uint64_t mpolyPtr = readPtr(base + offMpoly);
        const FileBlock* mpolyBlk = blockByPtr(mpolyPtr);
        if (mpolyBlk) {
            int mpolySize = structSize("MPoly");
            if (mpolySize <= 0) mpolySize = 12; // loopstart(int)+totloop(int)+mat_nr(short)+flag(char)*2
            int offLS = fieldOffset("MPoly", "loopstart");
            int offTL = fieldOffset("MPoly", "totloop");
            if (offLS < 0) offLS = 0;
            if (offTL < 0) offTL = 4;

            for (int i = 0; i < totpoly && i < mpolyBlk->count; ++i) {
                uint32_t pb = mpolyBlk->dataOffset + (uint32_t)(i * mpolySize);
                polys[i].loopstart = (int)read32(pb + offLS);
                polys[i].totloop   = (int)read32(pb + offTL);
            }
        }
    }

    // ── Triangulate polys → output verts + indices ────────────────────────────
    // Each polygon is a fan: (0, 1, 2), (0, 2, 3), (0, 3, 4), …
    outVerts.clear();
    outIdx.clear();
    outVerts.reserve((size_t)(totloop));
    outIdx.reserve((size_t)(totloop * 2));

    for (auto& poly : polys) {
        if (poly.totloop < 3) continue;
        if (poly.loopstart + poly.totloop > totloop) continue;

        // Fan from first loop vertex
        for (int t = 1; t < poly.totloop - 1; ++t) {
            int li[3] = {
                poly.loopstart,
                poly.loopstart + t,
                poly.loopstart + t + 1
            };

            for (int k = 0; k < 3; ++k) {
                Vertex v;
                v.color = glm::vec4(1.f, 1.f, 1.f, 1.f);

                int vi = loopVert[li[k]];
                if (vi >= 0 && vi < (int)positions.size()) {
                    v.position = positions[vi];
                    v.normal   = normals[vi];
                }
                if (hasUV && li[k] < (int)loopUVs.size())
                    v.uv = loopUVs[li[k]];

                outIdx.push_back((unsigned int)outVerts.size());
                outVerts.push_back(v);
            }
        }
    }

    // Generate flat normals if we had none
    if (normals.empty() || normals[0] == glm::vec3(0,1,0)) {
        for (size_t i = 0; i + 2 < outIdx.size(); i += 3) {
            auto& a = outVerts[outIdx[i]];
            auto& b = outVerts[outIdx[i+1]];
            auto& c = outVerts[outIdx[i+2]];
            glm::vec3 n = glm::normalize(
                glm::cross(b.position - a.position,
                           c.position - a.position));
            if (!std::isnan(n.x)) a.normal = b.normal = c.normal = n;
        }
    }

    if (outVerts.empty()) return false;

    // ── Texture — packed image data from inside the .blend ────────────────────
    //
    // When the user packs textures into .blend (File > External Data > Pack All),
    // Blender stores the raw image bytes in a PackedFile struct:
    //
    //   Image {
    //     ID      id;           // 32-byte header with name
    //     char    name[1024];   // filepath (may be "//texture.png" or empty)
    //     int     source;       // 0=file, 1=seq, 2=movie, ...
    //     ...
    //     PackedFile *packedfile;  // pointer → packed data block
    //   }
    //
    //   PackedFile {
    //     int  size;     // byte count
    //     int  seek;
    //     void *data;    // pointer → DATA block with raw image bytes
    //   }
    //
    // Strategy:
    //   1. Find IM (Image) block
    //   2. Read the packedfile pointer from the Image struct
    //   3. Follow it to the PackedFile block → read size + data pointer
    //   4. Follow data pointer to the DATA block → raw PNG/JPG/etc bytes
    //   5. If no packed data → fall back to filepath string

    // SDNA offsets for Image struct
    int offImgName       = fieldOffset("Image", "name");
    int offImgPackedFile = fieldOffset("Image", "packedfile");
    if (offImgName < 0)       offImgName       = 32; // after ID (32 bytes)
    if (offImgPackedFile < 0) {
        // Try "packedfiles" (Blender 3.x uses a ListBase)
        offImgPackedFile = fieldOffset("Image", "packedfiles");
    }

    // SDNA offsets for PackedFile struct
    int offPFSize = fieldOffset("PackedFile", "size");
    int offPFData = fieldOffset("PackedFile", "data");
    if (offPFSize < 0) offPFSize = 0;
    if (offPFData < 0) offPFData = 8; // int size(4) + int seek(4) = 8

    bool foundTexture = false;

    for (auto& blk : m_blocks) {
        if (blk.code != "IM") continue;
        if (blk.dataOffset + blk.size > m_size) continue;

        const uint32_t imgBase = blk.dataOffset;

        // ── Try to read packed image data ────────────────────────────────────
        if (offImgPackedFile >= 0) {
            uint64_t pfPtr = readPtr(imgBase + (uint32_t)offImgPackedFile);

            if (pfPtr != 0) {
                const FileBlock* pfBlk = blockByPtr(pfPtr);
                if (pfBlk && pfBlk->dataOffset + pfBlk->size <= m_size) {

                    // Read PackedFile.size and PackedFile.data pointer
                    int      pfSize = (int)read32(pfBlk->dataOffset + (uint32_t)offPFSize);
                    uint64_t dataPtr = readPtr(pfBlk->dataOffset + (uint32_t)offPFData);

                    if (pfSize > 0 && dataPtr != 0) {
                        const FileBlock* dataBlk = blockByPtr(dataPtr);
                        if (dataBlk &&
                            dataBlk->dataOffset + (uint32_t)pfSize <= m_size)
                        {
                            // Copy raw image bytes into outPackedData
                            const uint8_t* src = m_data + dataBlk->dataOffset;
                            outPackedData.assign(src, src + pfSize);
                            LOG_INFO("BlendLoader: packed image found — "
                                     << pfSize << " bytes");
                            foundTexture = true;
                            break;
                        }
                    }
                }
            }
        }

        // ── Fallback: read filepath string ───────────────────────────────────
        if (!foundTexture) {
            uint32_t nameOff = imgBase + (uint32_t)offImgName;
            if (nameOff < m_size) {
                std::string imgPath;
                for (uint32_t c = nameOff;
                     c < m_size && c < nameOff + 1024; ++c) {
                    char ch = (char)m_data[c];
                    if (ch == '\0') break;
                    imgPath += ch;
                }
                if (!imgPath.empty()) {
                    // Strip Blender relative prefix "//"
                    if (imgPath.size() >= 2 &&
                        imgPath[0] == '/' && imgPath[1] == '/')
                        imgPath = imgPath.substr(2);
                    std::replace(imgPath.begin(), imgPath.end(), '\\', '/');
                    outTexPath = imgPath;
                    LOG_INFO("BlendLoader: image path = '" << imgPath << "'");
                    foundTexture = true;
                    break;
                }
            }
        }
    }

    return true;
}
