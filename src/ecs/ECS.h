#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <functional>
#include <cassert>
#include <string>

// Entity just a uint32_t ID 
using EntityID = uint32_t;
constexpr EntityID NULL_ENTITY = 0;

// ComponentPool — stores components of a single type
class IComponentPool { public: virtual ~IComponentPool() = default; virtual void remove(EntityID) = 0; };

template<typename T>
class ComponentPool : public IComponentPool {
public:
    T* add(EntityID id) {
        assert(m_index.find(id) == m_index.end() && "Component already exists");
        m_index[id] = (uint32_t)m_data.size();
        m_ids.push_back(id);
        return &m_data.emplace_back();
    }

    T* get(EntityID id) {
        auto it = m_index.find(id);
        return it != m_index.end() ? &m_data[it->second] : nullptr;
    }

    void remove(EntityID id) override {
        auto it = m_index.find(id);
        if (it == m_index.end()) return;
        uint32_t idx  = it->second;
        uint32_t last = (uint32_t)m_data.size() - 1;
        if (idx != last) {
            m_data[idx]  = std::move(m_data[last]);
            m_ids[idx]   = m_ids[last];
            m_index[m_ids[idx]] = idx;
        }
        m_data.pop_back();
        m_ids.pop_back();
        m_index.erase(id);
    }

    bool has(EntityID id) const { return m_index.count(id) > 0; }

    // Internal access for iteration (used by World::each)
    std::vector<T>&        data() { return m_data; }
    const std::vector<T>&  data() const { return m_data; }
    std::vector<EntityID>& ids()  { return m_ids; }

private:
    std::vector<T>        m_data;
    std::vector<EntityID> m_ids;
    std::unordered_map<EntityID, uint32_t> m_index;
};

// World Represents the entire game state: entities + components
class World {
public:
    // Lifecycle
    EntityID create(const std::string& name = "") {
        EntityID id = ++m_nextId;
        m_alive.push_back(id);
        m_names[id] = name.empty() ? ("Entity_" + std::to_string(id)) : name;
        return id;
    }

    void destroy(EntityID id) {
        for (auto& [type, pool] : m_pools) pool->remove(id);
        m_alive.erase(std::remove(m_alive.begin(), m_alive.end(), id), m_alive.end());
        m_names.erase(id);
    }

    bool alive(EntityID id) const {
        return std::find(m_alive.begin(), m_alive.end(), id) != m_alive.end();
    }

    const std::string& name(EntityID id) const {
        static std::string empty;
        auto it = m_names.find(id);
        return it != m_names.end() ? it->second : empty;
    }

    const std::vector<EntityID>& entities() const { return m_alive; }

    // Components
    template<typename T>
    T& add(EntityID id) {
        return *pool<T>().add(id);
    }

    template<typename T>
    T* get(EntityID id) {
        return pool<T>().get(id);
    }

    template<typename T>
    bool has(EntityID id) {
        return pool<T>().has(id);
    }

    template<typename T>
    void remove(EntityID id) {
        pool<T>().remove(id);
    }

    // View: iterate all entities with component T
    template<typename T>
    void each(std::function<void(EntityID, T&)> fn) {
        auto& p = pool<T>();
        for (size_t i = 0; i < p.ids().size(); ++i)
            fn(p.ids()[i], p.data()[i]);
    }

    // View: iterate entities with BOTH T and U
    template<typename T, typename U>
    void each(std::function<void(EntityID, T&, U&)> fn) {
        auto& p = pool<T>();
        for (size_t i = 0; i < p.ids().size(); ++i) {
            EntityID id = p.ids()[i];
            U* u = get<U>(id);
            if (u) fn(id, p.data()[i], *u);
        }
    }

    // View: T, U, V
    template<typename T, typename U, typename V>
    void each(std::function<void(EntityID, T&, U&, V&)> fn) {
        auto& p = pool<T>();
        for (size_t i = 0; i < p.ids().size(); ++i) {
            EntityID id = p.ids()[i];
            U* u = get<U>(id); V* v = get<V>(id);
            if (u && v) fn(id, p.data()[i], *u, *v);
        }
    }

    // Search by name
    EntityID findByName(const std::string& name) const {
        for (auto& [id, n] : m_names)
            if (n == name) return id;
        return NULL_ENTITY;
    }

private:
    template<typename T>
    ComponentPool<T>& pool() {
        auto key = std::type_index(typeid(T));
        auto it  = m_pools.find(key);
        if (it == m_pools.end())
            m_pools[key] = std::make_unique<ComponentPool<T>>();
        return *static_cast<ComponentPool<T>*>(m_pools[key].get());
    }

    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
    std::vector<EntityID>                   m_alive;
    std::unordered_map<EntityID,std::string> m_names;
    EntityID m_nextId = 0;
};

// Default components
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct TransformComponent {
    glm::vec3 position = {0,0,0};
    glm::vec3 rotation = {0,0,0}; // Euler degrees
    glm::vec3 scale    = {1,1,1};

    glm::mat4 matrix() const {
        glm::mat4 m = glm::translate(glm::mat4(1.f), position);
        m = glm::rotate(m, glm::radians(rotation.x), {1,0,0});
        m = glm::rotate(m, glm::radians(rotation.y), {0,1,0});
        m = glm::rotate(m, glm::radians(rotation.z), {0,0,1});
        return glm::scale(m, scale);
    }
};

struct TagComponent {
    std::string tag;
};

struct ActiveComponent {}; // marker: entity is active

// ISystem — base class for systems
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void onStart (World&) {}
    virtual void onUpdate(World&, float dt) = 0;
    virtual void onRender(World&) {}
};
