#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
public:
    Shader() = default;

    // Загрузить из файлов
    static Shader fromFiles(const std::string& vertPath, const std::string& fragPath);
    // Загрузить из строк (удобно для встроенных шейдеров)
    static Shader fromSource(const char* vertSrc, const char* fragSrc);

    ~Shader();
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    bool valid() const { return m_id != 0; }

    // Установка юниформов
    void setInt  (const char* name, int v)            const;
    void setFloat(const char* name, float v)           const;
    void setBool (const char* name, bool v)            const;
    void setVec2 (const char* name, const glm::vec2&)  const;
    void setVec3 (const char* name, const glm::vec3&)  const;
    void setVec4 (const char* name, const glm::vec4&)  const;
    void setMat4 (const char* name, const glm::mat4&)  const;
    void setMat3 (const char* name, const glm::mat3&)  const;

    GLuint id() const { return m_id; }

private:
    explicit Shader(GLuint id) : m_id(id) {}

    static GLuint compileStage(GLenum type, const char* src);
    static GLuint linkProgram(GLuint vert, GLuint frag);

    GLuint m_id = 0;
};
