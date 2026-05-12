#include "render/Shader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

// ─── Compile helpers ─────────────────────────────────────────────────────────
GLuint Shader::compileStage(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        glDeleteShader(id);
        throw std::runtime_error(
            std::string("Shader compile error [")
            + (type == GL_VERTEX_SHADER ? "VERT" : "FRAG")
            + "]:\n" + log);
    }
    return id;
}

GLuint Shader::linkProgram(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        glDeleteProgram(prog);
        throw std::runtime_error(std::string("Shader link error:\n") + log);
    }
    return prog;
}

// ─── Factory ─────────────────────────────────────────────────────────────────
Shader Shader::fromSource(const char* vertSrc, const char* fragSrc) {
    GLuint vert = compileStage(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileStage(GL_FRAGMENT_SHADER, fragSrc);
    return Shader(linkProgram(vert, frag));
}

Shader Shader::fromFiles(const std::string& vertPath, const std::string& fragPath) {
    auto readFile = [](const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Cannot open shader file: " + path);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };
    auto vs = readFile(vertPath);
    auto fs = readFile(fragPath);
    return fromSource(vs.c_str(), fs.c_str());
}

// ─── Move ────────────────────────────────────────────────────────────────────
Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}
Shader::Shader(Shader&& o) noexcept : m_id(o.m_id) { o.m_id = 0; }
Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) { if (m_id) glDeleteProgram(m_id); m_id = o.m_id; o.m_id = 0; }
    return *this;
}

// ─── Use ─────────────────────────────────────────────────────────────────────
void Shader::use() const { glUseProgram(m_id); }

// ─── Uniforms ─────────────────────────────────────────────────────────────────
void Shader::setInt  (const char* n, int v)            const { glUniform1i (glGetUniformLocation(m_id,n), v); }
void Shader::setFloat(const char* n, float v)           const { glUniform1f (glGetUniformLocation(m_id,n), v); }
void Shader::setBool (const char* n, bool v)            const { glUniform1i (glGetUniformLocation(m_id,n),(int)v); }
void Shader::setVec2 (const char* n, const glm::vec2& v)const { glUniform2fv(glGetUniformLocation(m_id,n),1,glm::value_ptr(v)); }
void Shader::setVec3 (const char* n, const glm::vec3& v)const { glUniform3fv(glGetUniformLocation(m_id,n),1,glm::value_ptr(v)); }
void Shader::setVec4 (const char* n, const glm::vec4& v)const { glUniform4fv(glGetUniformLocation(m_id,n),1,glm::value_ptr(v)); }
void Shader::setMat4 (const char* n, const glm::mat4& m)const { glUniformMatrix4fv(glGetUniformLocation(m_id,n),1,GL_FALSE,glm::value_ptr(m)); }
void Shader::setMat3 (const char* n, const glm::mat3& m)const { glUniformMatrix3fv(glGetUniformLocation(m_id,n),1,GL_FALSE,glm::value_ptr(m)); }
