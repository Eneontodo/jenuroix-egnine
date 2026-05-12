#include "render/Mesh.h"
#include <cmath>
#include <numbers>

// ─── Ctor/Dtor ────────────────────────────────────────────────────────────────
Mesh::Mesh(const std::vector<Vertex>& verts,
           const std::vector<unsigned int>& indices,
           GLenum usage)
{
    setup(verts, indices, usage);
}

Mesh::~Mesh() { destroy(); }

Mesh::Mesh(Mesh&& o) noexcept
    : m_vao(o.m_vao), m_vbo(o.m_vbo), m_ebo(o.m_ebo),
      m_count(o.m_count), m_indexed(o.m_indexed)
{
    o.m_vao = o.m_vbo = o.m_ebo = 0;
}

Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this != &o) {
        destroy();
        m_vao = o.m_vao; m_vbo = o.m_vbo; m_ebo = o.m_ebo;
        m_count = o.m_count; m_indexed = o.m_indexed;
        o.m_vao = o.m_vbo = o.m_ebo = 0;
    }
    return *this;
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void Mesh::setup(const std::vector<Vertex>& verts,
                 const std::vector<unsigned int>& indices,
                 GLenum usage)
{
    m_indexed = !indices.empty();
    m_count   = m_indexed ? (GLsizei)indices.size() : (GLsizei)verts.size();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    if (m_indexed) glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
        (GLsizeiptr)(verts.size() * sizeof(Vertex)), verts.data(), usage);

    if (m_indexed) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizeiptr)(indices.size() * sizeof(unsigned int)), indices.data(), usage);
    }

    // layout(location=0) position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, position));
    // layout(location=1) normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, normal));
    // layout(location=2) uv
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, uv));
    // layout(location=3) color
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, color));

    glBindVertexArray(0);
}

void Mesh::update(const std::vector<Vertex>& verts,
                  const std::vector<unsigned int>& indices)
{
    destroy();
    setup(verts, indices, GL_DYNAMIC_DRAW);
}

void Mesh::destroy() {
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao = m_vbo = m_ebo = 0;
}

// ─── Draw ─────────────────────────────────────────────────────────────────────
void Mesh::draw(GLenum mode) const {
    glBindVertexArray(m_vao);
    if (m_indexed)
        glDrawElements(mode, m_count, GL_UNSIGNED_INT, nullptr);
    else
        glDrawArrays(mode, 0, m_count);
    glBindVertexArray(0);
}

// ─── Primitives ──────────────────────────────────────────────────────────────
Mesh Mesh::makeTriangle(float s) {
    return Mesh({
        {{ 0.0f,  s*0.866f, 0}, {0,0,1}, {0.5f,1.0f}, {1,0,0,1}},
        {{-s*0.5f,-s*0.5f, 0}, {0,0,1}, {0.0f,0.0f}, {0,1,0,1}},
        {{ s*0.5f,-s*0.5f, 0}, {0,0,1}, {1.0f,0.0f}, {0,0,1,1}},
    });
}

Mesh Mesh::makeQuad(float s) {
    float h = s * 0.5f;
    std::vector<Vertex> verts = {
        {{-h,  h, 0}, {0,0,1}, {0,1}, {1,1,1,1}},
        {{-h, -h, 0}, {0,0,1}, {0,0}, {1,1,1,1}},
        {{ h, -h, 0}, {0,0,1}, {1,0}, {1,1,1,1}},
        {{ h,  h, 0}, {0,0,1}, {1,1}, {1,1,1,1}},
    };
    std::vector<unsigned int> idx = {0,1,2, 0,2,3};
    return Mesh(verts, idx);
}

Mesh Mesh::makeCube(float s) {
    float h = s * 0.5f;
    // 6 граней * 4 вершины
    std::vector<Vertex> v = {
        // -Z
        {{-h,-h,-h},{0,0,-1},{0,0}}, {{ h,-h,-h},{0,0,-1},{1,0}}, {{ h, h,-h},{0,0,-1},{1,1}}, {{-h, h,-h},{0,0,-1},{0,1}},
        // +Z
        {{ h,-h, h},{0,0, 1},{0,0}}, {{-h,-h, h},{0,0, 1},{1,0}}, {{-h, h, h},{0,0, 1},{1,1}}, {{ h, h, h},{0,0, 1},{0,1}},
        // -X
        {{-h,-h, h},{-1,0,0},{0,0}}, {{-h,-h,-h},{-1,0,0},{1,0}}, {{-h, h,-h},{-1,0,0},{1,1}}, {{-h, h, h},{-1,0,0},{0,1}},
        // +X
        {{ h,-h,-h},{ 1,0,0},{0,0}}, {{ h,-h, h},{ 1,0,0},{1,0}}, {{ h, h, h},{ 1,0,0},{1,1}}, {{ h, h,-h},{ 1,0,0},{0,1}},
        // -Y
        {{-h,-h, h},{0,-1,0},{0,0}}, {{ h,-h, h},{0,-1,0},{1,0}}, {{ h,-h,-h},{0,-1,0},{1,1}}, {{-h,-h,-h},{0,-1,0},{0,1}},
        // +Y
        {{-h, h,-h},{0, 1,0},{0,0}}, {{ h, h,-h},{0, 1,0},{1,0}}, {{ h, h, h},{0, 1,0},{1,1}}, {{-h, h, h},{0, 1,0},{0,1}},
    };
    std::vector<unsigned int> idx;
    for (unsigned int f = 0; f < 6; ++f) {
        unsigned int base = f * 4;
        idx.insert(idx.end(), {base,base+1,base+2, base,base+2,base+3});
    }
    return Mesh(v, idx);
}

Mesh Mesh::makeGrid(int cols, int rows, float cell) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;
    float ox = cols * cell * 0.5f;
    float oz = rows * cell * 0.5f;
    for (int r = 0; r <= rows; ++r)
    for (int c = 0; c <= cols; ++c) {
        float x = c * cell - ox;
        float z = r * cell - oz;
        verts.push_back({{x, 0, z}, {0,1,0},
            {(float)c/cols, (float)r/rows}, {1,1,1,1}});
    }
    for (int r = 0; r < rows; ++r)
    for (int c = 0; c < cols; ++c) {
        unsigned int tl = r*(cols+1)+c;
        idx.insert(idx.end(), {tl, tl+1, tl+(unsigned)(cols+1),
                                tl+1, tl+(unsigned)(cols+1)+1, tl+(unsigned)(cols+1)});
    }
    return Mesh(verts, idx);
}

Mesh Mesh::makeSphere(float radius, int sectors, int stacks) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;
    const float pi = std::numbers::pi_v<float>;

    for (int st = 0; st <= stacks; ++st) {
        float phi = pi * 0.5f - pi * (float)st / stacks;
        float y   = radius * std::sin(phi);
        float r   = radius * std::cos(phi);
        for (int sc = 0; sc <= sectors; ++sc) {
            float theta = 2.f * pi * (float)sc / sectors;
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);
            glm::vec3 pos = {x, y, z};
            glm::vec3 norm = glm::normalize(pos);
            glm::vec2 uv  = {(float)sc/sectors, (float)st/stacks};
            verts.push_back({pos, norm, uv, {1,1,1,1}});
        }
    }
    for (int st = 0; st < stacks; ++st)
    for (int sc = 0; sc < sectors; ++sc) {
        unsigned int cur  = st*(sectors+1)+sc;
        unsigned int next = cur + sectors + 1;
        if (st != 0)        idx.insert(idx.end(), {cur, next, cur+1});
        if (st != stacks-1) idx.insert(idx.end(), {cur+1, next, next+1});
    }
    return Mesh(verts, idx);
}

Mesh Mesh::makeCylinder(float radius, float height, int sectors) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> idx;
    const float pi = std::numbers::pi_v<float>;
    const float half = height * 0.5f;

    // ── Боковая поверхность ─────────────────────────────────────────────────
    // sectors+1 столбцов (первая и последняя совпадают по позиции, но разные uv)
    for (int s = 0; s <= sectors; ++s) {
        float theta = 2.f * pi * (float)s / sectors;
        float cx = std::cos(theta);
        float cz = std::sin(theta);
        float u  = (float)s / sectors;

        // нижняя вершина боковой грани
        verts.push_back({{radius*cx, -half, radius*cz},
                         {cx, 0.f, cz},
                         {u, 0.f}, {1,1,1,1}});
        // верхняя вершина боковой грани
        verts.push_back({{radius*cx,  half, radius*cz},
                         {cx, 0.f, cz},
                         {u, 1.f}, {1,1,1,1}});
    }
    // каждый столбец даёт 2 вершины: bottom = s*2, top = s*2+1
    for (int s = 0; s < sectors; ++s) {
        unsigned int b0 = s*2,   t0 = s*2+1;
        unsigned int b1 = b0+2,  t1 = t0+2;
        idx.insert(idx.end(), {b0, b1, t0,  t0, b1, t1});
    }

    // ── Крышки ─────────────────────────────────────────────────────────────
    // Генерируем крышки с отдельными вершинами (плоские нормали)
    auto addCap = [&](float y, float ny) {
        unsigned int centerIdx = (unsigned int)verts.size();
        // центр
        verts.push_back({{0.f, y, 0.f}, {0.f, ny, 0.f}, {0.5f, 0.5f}, {1,1,1,1}});
        unsigned int rimStart = centerIdx + 1;
        for (int s = 0; s <= sectors; ++s) {
            float theta = 2.f * pi * (float)s / sectors;
            float cx = std::cos(theta);
            float cz = std::sin(theta);
            float u  = cx * 0.5f + 0.5f;
            float v  = cz * 0.5f + 0.5f;
            verts.push_back({{radius*cx, y, radius*cz},
                             {0.f, ny, 0.f},
                             {u, v}, {1,1,1,1}});
        }
        for (int s = 0; s < sectors; ++s) {
            unsigned int a = rimStart + s;
            unsigned int b = rimStart + s + 1;
            if (ny > 0.f)
                idx.insert(idx.end(), {centerIdx, a, b});
            else
                idx.insert(idx.end(), {centerIdx, b, a});
        }
    };

    addCap(-half, -1.f); // нижняя крышка
    addCap( half,  1.f); // верхняя крышка

    return Mesh(verts, idx);
}
