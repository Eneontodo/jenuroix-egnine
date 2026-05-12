#include "render/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera(float fov, float aspect, float nearZ, float farZ)
    : m_fov(fov), m_aspect(aspect), m_near(nearZ), m_far(farZ) {}

void Camera::setRotation(float yaw, float pitch) {
    m_yaw   = yaw;
    m_pitch = std::clamp(pitch, -89.f, 89.f);
}

void Camera::rotate(float dy, float dp) {
    m_yaw  += dy;
    m_pitch = std::clamp(m_pitch + dp, -89.f, 89.f);
}

glm::vec3 Camera::forward() const {
    float yr = glm::radians(m_yaw);
    float pr = glm::radians(m_pitch);
    return glm::normalize(glm::vec3{
        std::cos(pr) * std::cos(yr),
        std::sin(pr),
        std::cos(pr) * std::sin(yr)
    });
}

glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), m_worldUp));
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(m_pos, m_pos + forward(), m_worldUp);
}

glm::mat4 Camera::projectionMatrix() const {
    return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
}

glm::mat4 Camera::vpMatrix() const {
    return projectionMatrix() * viewMatrix();
}

void Camera::processMouse(float dx, float dy, float sensitivity) {
    rotate(dx * sensitivity, -dy * sensitivity);
}

void Camera::processKeyboard(float fwd, float right, float upv, float speed, float dt) {
    m_pos += forward() * fwd   * speed * dt;
    m_pos += Camera::right() * right * speed * dt;
    m_pos += m_worldUp       * upv   * speed * dt;
}
