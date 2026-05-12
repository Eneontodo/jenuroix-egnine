#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 color = {1,1,1,1};
};

class Mesh {
public:
    Mesh() = default;
    Mesh(const std::vector<Vertex>& verts,
         const std::vector<unsigned int>& indices = {},
         GLenum usage = GL_STATIC_DRAW);

    ~Mesh();
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw(GLenum mode = GL_TRIANGLES) const;

    // Обновить данные на GPU (для динамических мешей)
    void update(const std::vector<Vertex>& verts,
                const std::vector<unsigned int>& indices = {});

    // ─── Готовые примитивы ────────────────────────────────────────────────
    static Mesh makeTriangle(float size = 1.0f);
    static Mesh makeQuad    (float size = 1.0f);  // единичный квадрат XY
    static Mesh makeCube    (float size = 1.0f);
    static Mesh makeGrid    (int cols, int rows, float cellSize = 1.0f);
    static Mesh makeSphere  (float radius = 1.0f, int sectors = 32, int stacks = 16);
    static Mesh makeCylinder(float radius = 0.5f, float height = 1.0f, int sectors = 32);

    bool valid() const { return m_vao != 0; }

private:
    void setup(const std::vector<Vertex>& verts,
               const std::vector<unsigned int>& indices,
               GLenum usage);
    void destroy();

    GLuint m_vao       = 0;
    GLuint m_vbo       = 0;
    GLuint m_ebo       = 0;
    GLsizei m_count    = 0;   // число индексов или вершин
    bool    m_indexed  = false;
};
