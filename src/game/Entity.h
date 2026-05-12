#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <memory>
#include <vector>
#include <functional>

    // Базовый класс «объект на сцене».
    // Используй как есть или наследуй.

class Entity {
public:
    explicit Entity(std::string name = "Entity")
        : m_name(std::move(name)) {}
    virtual ~Entity() = default;

    // Non-copyable
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    // Transform
    glm::vec3& position()       { return m_pos; }
    glm::vec3& rotation()       { return m_rot; } // Euler degrees XYZ
    glm::vec3& scale()          { return m_scale; }

    const glm::vec3& position() const { return m_pos; }
    const glm::vec3& rotation() const { return m_rot; }
    const glm::vec3& scale()    const { return m_scale; }

    glm::mat4 modelMatrix() const {
        glm::mat4 m = glm::translate(glm::mat4(1.f), m_pos);
        m = glm::rotate(m, glm::radians(m_rot.x), {1,0,0});
        m = glm::rotate(m, glm::radians(m_rot.y), {0,1,0});
        m = glm::rotate(m, glm::radians(m_rot.z), {0,0,1});
        m = glm::scale (m, m_scale);
        return m;
    }

    // Metadata
    const std::string& name()    const { return m_name; }
    bool               active()  const { return m_active; }
    void               setActive(bool v) { m_active = v; }

    // Hierarchy
    void addChild(std::shared_ptr<Entity> child) {
        child->m_parent = this;
        m_children.push_back(std::move(child));
    }
    const std::vector<std::shared_ptr<Entity>>& children() const { return m_children; }

    // Lifecycle callbacks (override in derived classes)
    virtual void onStart()                  {}
    virtual void onUpdate(float /*dt*/)     {}
    virtual void onRender()                 {}

protected:
    std::string m_name;
    bool        m_active = true;
    glm::vec3   m_pos   = {0,0,0};
    glm::vec3   m_rot   = {0,0,0};
    glm::vec3   m_scale = {1,1,1};

    Entity*     m_parent = nullptr;
    std::vector<std::shared_ptr<Entity>> m_children;
};
