#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    Camera(float fov   = 60.f,
           float aspect = 16.f/9.f,
           float nearZ  = 0.05f,
           float farZ   = 1000.f);

    // ─── Трансформ ────────────────────────────────────────────────────────
    void setPosition(const glm::vec3& pos)  { m_pos = pos; }
    void move(const glm::vec3& delta)        { m_pos += delta; }

    // yaw = поворот вокруг Y, pitch = вверх-вниз (градусы)
    void setRotation(float yaw, float pitch);
    void rotate(float dyaw, float dpitch);   // добавляет к текущим

    glm::vec3 position() const { return m_pos; }
    float yaw()   const { return m_yaw; }
    float pitch() const { return m_pitch; }

    // Направления (вычисляются из yaw/pitch)
    glm::vec3 forward() const;
    glm::vec3 right()   const;
    glm::vec3 up()      const { return m_worldUp; }

    // ─── Матрицы ─────────────────────────────────────────────────────────
    glm::mat4 viewMatrix()       const;
    glm::mat4 projectionMatrix() const;
    // VP сразу
    glm::mat4 vpMatrix()         const;

    // ─── Проекция ─────────────────────────────────────────────────────────
    void setFov   (float fov)    { m_fov    = fov; }
    void setAspect(float aspect)  { m_aspect = aspect; }
    void setClip  (float n, float f) { m_near = n; m_far = f; }

    float fov()    const { return m_fov; }
    float aspect() const { return m_aspect; }

    // ─── FPS-управление ───────────────────────────────────────────────────
    // Sensitivity: пиксели → градусы
    void processMouse   (float dx, float dy, float sensitivity = 0.1f);
    void processKeyboard(float forward, float right, float up, float speed, float dt);

private:
    glm::vec3 m_pos     = {0, 0, 3};
    float     m_yaw     = -90.f;
    float     m_pitch   = 0.f;
    float     m_fov     = 60.f;
    float     m_aspect  = 16.f/9.f;
    float     m_near    = 0.05f;
    float     m_far     = 1000.f;
    glm::vec3 m_worldUp = {0,1,0};
};
