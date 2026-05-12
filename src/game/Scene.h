#pragma once
#include "game/Entity.h"
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

class Scene {
public:
    explicit Scene(std::string name = "Scene") : m_name(std::move(name)) {}

    // Create and add entity
    template<typename T, typename... Args>
    T* spawn(Args&&... args) {
        auto e = std::make_shared<T>(std::forward<Args>(args)...);
        T* ptr = e.get();
        m_entities.push_back(std::move(e));
        ptr->onStart();
        return ptr;
    }

    void remove(Entity* e) {
        m_entities.erase(
            std::remove_if(m_entities.begin(), m_entities.end(),
                [e](const auto& p){ return p.get() == e; }),
            m_entities.end());
    }

    void update(float dt) {
        for (auto& e : m_entities)
            if (e->active()) e->onUpdate(dt);
    }

    void render() {
        for (auto& e : m_entities)
            if (e->active()) e->onRender();
    }

    const std::vector<std::shared_ptr<Entity>>& entities() const { return m_entities; }
    const std::string& name() const { return m_name; }

    // Find the first entity by name
    Entity* find(const std::string& name) {
        for (auto& e : m_entities)
            if (e->name() == name) return e.get();
        return nullptr;
    }

private:
    std::string m_name;
    std::vector<std::shared_ptr<Entity>> m_entities;
};
