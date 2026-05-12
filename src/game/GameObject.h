#pragma once
#include <string>
#include <functional>
#include <glm/glm.hpp>

class World;
using EntityID = uint32_t;
struct TransformComponent;

// ─────────────────────────────────────────────────────────────────────────────
//  GameObject — lightweight handle (ID + World*). Pass by value freely.
//  Every method returns *this for chaining: obj.pos(0,1,0).scale(2).color(1,0,0)
// ─────────────────────────────────────────────────────────────────────────────
class GameObject {
public:
    GameObject() = default;
    GameObject(EntityID id, World* world) : m_id(id), m_world(world) {}

    bool        valid() const { return m_id != 0 && m_world != nullptr; }
    EntityID    id()    const { return m_id; }
    World*      world()       { return m_world; }

    // ── TRANSFORM ─────────────────────────────────────────────────────────────
    GameObject& pos   (float x, float y, float z = 0.f);
    GameObject& pos   (glm::vec3 p);
    glm::vec3   pos   () const;

    GameObject& x     (float v);
    GameObject& y     (float v);
    GameObject& z     (float v);
    float       x     () const;
    float       y     () const;
    float       z     () const;

    GameObject& move  (float dx, float dy, float dz = 0.f);
    GameObject& move  (glm::vec3 d);

    GameObject& rot   (float rx, float ry, float rz = 0.f);
    GameObject& rotX  (float deg);
    GameObject& rotY  (float deg);
    GameObject& rotZ  (float deg);
    GameObject& addRot(float rx, float ry, float rz = 0.f); // increment rotation
    glm::vec3   rot   () const;

    GameObject& scale (float s);
    GameObject& scale (float sx, float sy, float sz);
    GameObject& scaleX(float v);
    GameObject& scaleY(float v);
    GameObject& scaleZ(float v);
    glm::vec3   scale () const;

    GameObject& lookAt(float tx, float ty, float tz);
    GameObject& lookAt(glm::vec3 target);
    glm::vec3   forward() const;
    glm::vec3   right  () const;
    glm::vec3   up     () const;

    // ── MESH / SHAPE ──────────────────────────────────────────────────────────
    GameObject& cube    (float size = 1.f);
    GameObject& sphere  (float radius = 0.5f, int detail = 24);
    GameObject& quad    (float size = 1.f);
    GameObject& grid    (int cols = 10, int rows = 10, float cell = 1.f);
    GameObject& cylinder(float radius = 0.5f, float height = 1.f, int segs = 16);
    GameObject& plane   (float w = 10.f, float d = 10.f);
    GameObject& model   (const std::string& path);

    // ── MATERIAL ──────────────────────────────────────────────────────────────
    GameObject& color      (float r, float g, float b, float a = 1.f);
    GameObject& color      (glm::vec3 c);
    GameObject& alpha      (float a);
    GameObject& metallic   (float v);
    GameObject& roughness  (float v);
    GameObject& emissive   (float r, float g, float b);
    GameObject& emissive   (float intensity);
    GameObject& texture    (const std::string& path);
    GameObject& tiling     (float u, float v);
    GameObject& wireframe  (bool on = true);
    GameObject& doubleSided(bool on = true);

    // ── VISIBILITY ────────────────────────────────────────────────────────────
    GameObject& hide();
    GameObject& show();
    GameObject& toggle();
    GameObject& visible(bool on);
    bool        visible() const;

    // ── PHYSICS ───────────────────────────────────────────────────────────────
    GameObject& physStatic   ();
    GameObject& physDynamic  (float mass = 1.f);
    GameObject& physKinematic();
    GameObject& physSensor   ();
    GameObject& physBox      (glm::vec3 halfExt = {0.5f,0.5f,0.5f});
    GameObject& physSphere   (float radius = 0.5f);
    GameObject& physCapsule  (float radius = 0.4f, float height = 1.f);
    GameObject& mass         (float kg);
    GameObject& friction     (float f);
    GameObject& bounce       (float b);
    GameObject& gravityScale (float s);
    GameObject& velocity     (float x, float y, float z);
    GameObject& velocity     (glm::vec3 v);
    glm::vec3   velocity     () const;
    GameObject& impulse      (float x, float y, float z);
    GameObject& impulse      (glm::vec3 v);
    GameObject& addForce     (glm::vec3 f);
    GameObject& lockRotX     (bool on = true);
    GameObject& lockRotY     (bool on = true);
    GameObject& lockRotZ     (bool on = true);

    // ── AUDIO ─────────────────────────────────────────────────────────────────
    GameObject& playSound   (const std::string& path, float vol = 1.f);
    GameObject& loopSound   (const std::string& path, float vol = 1.f);
    GameObject& stopSound   ();
    GameObject& soundVolume (float v);
    GameObject& soundPitch  (float v);
    GameObject& sound3D     (bool on = true);

    // ── SCRIPTS ───────────────────────────────────────────────────────────────
    GameObject& onUpdate (std::function<void(GameObject&, float dt)> fn);
    GameObject& onStart  (std::function<void(GameObject&)> fn);
    GameObject& onDestroy(std::function<void(GameObject&)> fn);
    GameObject& onCollide(std::function<void(GameObject&, GameObject)> fn);

    // ── COLLIDER (legacy AABB) ────────────────────────────────────────────────
    GameObject& addCollider();
    bool        overlaps(const GameObject& other) const;

    // ── TAG / NAME ────────────────────────────────────────────────────────────
    GameObject& tag    (const std::string& t);
    std::string tag    () const;
    std::string name   () const;
    bool        hasTag (const std::string& t) const;

    // ── UTILITY ───────────────────────────────────────────────────────────────
    float distanceTo(const GameObject& other) const;
    float distanceTo(glm::vec3 point)         const;
    void  destroy   ();

    bool operator==(const GameObject& o) const { return m_id == o.m_id; }
    bool operator!=(const GameObject& o) const { return m_id != o.m_id; }

private:
    EntityID m_id    = 0;
    World*   m_world = nullptr;
    TransformComponent& transform() const;
};
