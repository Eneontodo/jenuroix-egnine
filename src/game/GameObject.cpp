#include "game/GameObject.h"
#include "engine.h"
#include "ecs/ECS.h"
#include "audio/Audio.h"
#include "physics/Physics.h"
#include <stdexcept>
#include <cmath>
#include <unordered_map>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace eng;

// ─────────────────────────────────────────────────────────────────────────────
//  Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
TransformComponent& GameObject::transform() const {
    auto* t = m_world->get<TransformComponent>(m_id);
    if (!t) throw std::runtime_error("GameObject: no TransformComponent");
    return *t;
}

static void ensureRenderer(const GameObject& go, World* w) {
    if (!w->has<MeshRendererComponent>(go.id()))
        w->add<MeshRendererComponent>(go.id());
}

static MeshRendererComponent& renderer(const GameObject& go, World* w) {
    ensureRenderer(go, w);
    return *w->get<MeshRendererComponent>(go.id());
}

// ─────────────────────────────────────────────────────────────────────────────
//  TRANSFORM
// ─────────────────────────────────────────────────────────────────────────────
GameObject& GameObject::pos(float x, float y, float z) {
    transform().position = {x, y, z}; return *this;
}
GameObject& GameObject::pos(glm::vec3 p) {
    transform().position = p; return *this;
}
glm::vec3 GameObject::pos() const { return transform().position; }

GameObject& GameObject::x(float v) { transform().position.x = v; return *this; }
GameObject& GameObject::y(float v) { transform().position.y = v; return *this; }
GameObject& GameObject::z(float v) { transform().position.z = v; return *this; }
float GameObject::x() const { return transform().position.x; }
float GameObject::y() const { return transform().position.y; }
float GameObject::z() const { return transform().position.z; }

GameObject& GameObject::move(float dx, float dy, float dz) {
    transform().position += glm::vec3(dx, dy, dz); return *this;
}
GameObject& GameObject::move(glm::vec3 d) {
    transform().position += d; return *this;
}

GameObject& GameObject::rot(float rx, float ry, float rz) {
    transform().rotation = {rx, ry, rz}; return *this;
}
GameObject& GameObject::rotX(float d) { transform().rotation.x = d; return *this; }
GameObject& GameObject::rotY(float d) { transform().rotation.y = d; return *this; }
GameObject& GameObject::rotZ(float d) { transform().rotation.z = d; return *this; }
GameObject& GameObject::addRot(float rx, float ry, float rz) {
    transform().rotation += glm::vec3(rx, ry, rz); return *this;
}
glm::vec3 GameObject::rot() const { return transform().rotation; }

GameObject& GameObject::scale(float s) {
    transform().scale = {s, s, s}; return *this;
}
GameObject& GameObject::scale(float sx, float sy, float sz) {
    transform().scale = {sx, sy, sz}; return *this;
}
GameObject& GameObject::scaleX(float v) { transform().scale.x = v; return *this; }
GameObject& GameObject::scaleY(float v) { transform().scale.y = v; return *this; }
GameObject& GameObject::scaleZ(float v) { transform().scale.z = v; return *this; }
glm::vec3 GameObject::scale() const { return transform().scale; }

GameObject& GameObject::lookAt(float tx, float ty, float tz) {
    return lookAt(glm::vec3(tx, ty, tz));
}
GameObject& GameObject::lookAt(glm::vec3 target) {
    glm::vec3 dir = glm::normalize(target - transform().position);
    float yaw   = glm::degrees(std::atan2(dir.x, dir.z));
    float pitch = glm::degrees(std::asin(-dir.y));
    transform().rotation = {pitch, yaw, 0.f};
    return *this;
}
glm::vec3 GameObject::forward() const {
    glm::vec3 r = glm::radians(transform().rotation);
    return glm::normalize(glm::vec3(
        std::sin(r.y) * std::cos(r.x),
       -std::sin(r.x),
        std::cos(r.y) * std::cos(r.x)));
}
glm::vec3 GameObject::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3(0,1,0)));
}
glm::vec3 GameObject::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

// ─────────────────────────────────────────────────────────────────────────────
//  MESH / SHAPE
// ─────────────────────────────────────────────────────────────────────────────
GameObject& GameObject::cube(float size) {
    auto& mr = renderer(*this, m_world);
    auto mdl = std::make_shared<Model>();
    mdl->addSubMesh(Mesh::makeCube(size));
    mr.model = mdl;
    return *this;
}
GameObject& GameObject::sphere(float radius, int detail) {
    auto& mr = renderer(*this, m_world);
    auto mdl = std::make_shared<Model>();
    mdl->addSubMesh(Mesh::makeSphere(radius, detail));
    mr.model = mdl;
    return *this;
}
GameObject& GameObject::quad(float size) {
    auto& mr = renderer(*this, m_world);
    auto mdl = std::make_shared<Model>();
    mdl->addSubMesh(Mesh::makeQuad(size));
    mr.model = mdl;
    return *this;
}
GameObject& GameObject::grid(int cols, int rows, float cell) {
    auto& mr = renderer(*this, m_world);
    auto mdl = std::make_shared<Model>();
    mdl->addSubMesh(Mesh::makeGrid(cols, rows, cell));
    mr.model = mdl;
    return *this;
}
GameObject& GameObject::cylinder(float radius, float height, int segs) {
    auto& mr = renderer(*this, m_world);
    auto mdl = std::make_shared<Model>();
    mdl->addSubMesh(Mesh::makeCylinder(radius, height, segs));
    mr.model = mdl;
    return *this;
}
GameObject& GameObject::plane(float w, float d) {
    return scale(w, 1.f, d).quad(1.f);
}
GameObject& GameObject::model(const std::string& path) {
    auto& mr = renderer(*this, m_world);
    if (g_app) {
        Model* raw = g_app->resources().model(path);
        if (raw) mr.model = std::shared_ptr<Model>(raw, [](Model*){});
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
//  MATERIAL
// ─────────────────────────────────────────────────────────────────────────────
GameObject& GameObject::color(float r, float g, float b, float a) {
    auto& mr = renderer(*this, m_world);
    mr.material.albedo = {r, g, b, a};
    if (g_app)
        mr.material.albedoMap = g_app->resources().colorTex(
            (uint8_t)(r*255),(uint8_t)(g*255),(uint8_t)(b*255),(uint8_t)(a*255));
    return *this;
}
GameObject& GameObject::color(glm::vec3 c) { return color(c.r, c.g, c.b); }

GameObject& GameObject::alpha(float a) {
    auto& mr = renderer(*this, m_world);
    mr.material.albedo.a = a;
    return *this;
}
GameObject& GameObject::metallic(float v) {
    renderer(*this, m_world).material.metallic = v; return *this;
}
GameObject& GameObject::roughness(float v) {
    renderer(*this, m_world).material.roughness = v; return *this;
}
GameObject& GameObject::emissive(float r, float g, float b) {
    renderer(*this, m_world).material.emissive = {r, g, b}; return *this;
}
GameObject& GameObject::emissive(float intensity) {
    auto& m = renderer(*this, m_world).material;
    glm::vec3 col = glm::vec3(m.albedo) * intensity;
    m.emissive = col;
    return *this;
}
GameObject& GameObject::texture(const std::string& path) {
    auto& mr = renderer(*this, m_world);
    if (g_app) mr.material.albedoMap = g_app->resources().texture(path);
    return *this;
}
GameObject& GameObject::tiling(float u, float v) {
    renderer(*this, m_world).material.tilingU = u;
    renderer(*this, m_world).material.tilingV = v;
    return *this;
}
GameObject& GameObject::wireframe(bool on) {
    renderer(*this, m_world).material.wireframe = on; return *this;
}
GameObject& GameObject::doubleSided(bool on) {
    renderer(*this, m_world).material.doubleSided = on; return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
//  VISIBILITY
// ─────────────────────────────────────────────────────────────────────────────
GameObject& GameObject::hide() {
    ensureRenderer(*this, m_world);
    m_world->get<MeshRendererComponent>(m_id)->visible = false;
    return *this;
}
GameObject& GameObject::show() {
    ensureRenderer(*this, m_world);
    m_world->get<MeshRendererComponent>(m_id)->visible = true;
    return *this;
}
GameObject& GameObject::toggle() {
    ensureRenderer(*this, m_world);
    auto* mr = m_world->get<MeshRendererComponent>(m_id);
    mr->visible = !mr->visible;
    return *this;
}
GameObject& GameObject::visible(bool on) { return on ? show() : hide(); }
bool        GameObject::visible() const {
    auto* mr = m_world->get<MeshRendererComponent>(m_id);
    return mr ? mr->visible : true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  PHYSICS
// ─────────────────────────────────────────────────────────────────────────────
static void ensurePhys(GameObject& go, World* w,
                       RigidBodyType type, ColliderShape shape,
                       glm::vec3 halfExt = {0.5f,0.5f,0.5f}, float mass = 1.f)
{
    if (!g_app) return;
    if (!w->has<RigidBodyComponent>(go.id()))
        w->add<RigidBodyComponent>(go.id());
    auto* rb = w->get<RigidBodyComponent>(go.id());
    rb->type    = type;
    rb->shape   = shape;
    rb->halfExt = halfExt;
    rb->mass    = mass;
    auto* t = w->get<TransformComponent>(go.id());
    glm::vec3 pos = t ? t->position : glm::vec3(0);
    glm::quat qrot = t ? glm::quat(glm::radians(t->rotation)) : glm::quat{1,0,0,0};
    g_app->physics().addBody(go.id(), *rb, pos, qrot);
}

GameObject& GameObject::physStatic() {
    ensurePhys(*this, m_world, RigidBodyType::Static, ColliderShape::Box);
    return *this;
}
GameObject& GameObject::physDynamic(float m) {
    ensurePhys(*this, m_world, RigidBodyType::Dynamic, ColliderShape::Box, {0.5f,0.5f,0.5f}, m);
    return *this;
}
GameObject& GameObject::physKinematic() {
    ensurePhys(*this, m_world, RigidBodyType::Kinematic, ColliderShape::Box);
    return *this;
}
GameObject& GameObject::physSensor() {
    if (!g_app) return *this;
    if (!m_world->has<RigidBodyComponent>(m_id))
        m_world->add<RigidBodyComponent>(m_id);
    auto* rb = m_world->get<RigidBodyComponent>(m_id);
    rb->isSensor = true;
    rb->type     = RigidBodyType::Dynamic;
    rb->shape    = ColliderShape::Box;
    auto* t = m_world->get<TransformComponent>(m_id);
    g_app->physics().addBody(m_id, *rb, t ? t->position : glm::vec3(0));
    return *this;
}
GameObject& GameObject::physBox(glm::vec3 he) {
    ensurePhys(*this, m_world, RigidBodyType::Dynamic, ColliderShape::Box, he);
    return *this;
}
GameObject& GameObject::physSphere(float r) {
    if (!g_app) return *this;
    if (!m_world->has<RigidBodyComponent>(m_id))
        m_world->add<RigidBodyComponent>(m_id);
    auto* rb = m_world->get<RigidBodyComponent>(m_id);
    rb->shape  = ColliderShape::Sphere;
    rb->radius = r;
    rb->type   = RigidBodyType::Dynamic;
    auto* t = m_world->get<TransformComponent>(m_id);
    g_app->physics().addBody(m_id, *rb, t ? t->position : glm::vec3(0));
    return *this;
}
GameObject& GameObject::physCapsule(float r, float h) {
    if (!g_app) return *this;
    if (!m_world->has<RigidBodyComponent>(m_id))
        m_world->add<RigidBodyComponent>(m_id);
    auto* rb = m_world->get<RigidBodyComponent>(m_id);
    rb->shape  = ColliderShape::Capsule;
    rb->radius = r;
    rb->height = h;
    rb->type   = RigidBodyType::Dynamic;
    auto* t = m_world->get<TransformComponent>(m_id);
    g_app->physics().addBody(m_id, *rb, t ? t->position : glm::vec3(0));
    return *this;
}
GameObject& GameObject::mass(float kg) {
    if (auto* rb = m_world->get<RigidBodyComponent>(m_id)) rb->mass = kg;
    return *this;
}
GameObject& GameObject::friction(float f) {
    if (auto* rb = m_world->get<RigidBodyComponent>(m_id)) rb->friction = f;
    return *this;
}
GameObject& GameObject::bounce(float b) {
    if (auto* rb = m_world->get<RigidBodyComponent>(m_id)) rb->restitution = b;
    return *this;
}
GameObject& GameObject::gravityScale(float s) {
    if (auto* rb = m_world->get<RigidBodyComponent>(m_id)) {
        rb->gravityScale = s;
        if (g_app) g_app->physics().setGravityScale(m_id, s);
    }
    return *this;
}
GameObject& GameObject::velocity(float x, float y, float z) {
    if (g_app) g_app->physics().setVelocity(m_id, {x,y,z});
    return *this;
}
GameObject& GameObject::velocity(glm::vec3 v) {
    if (g_app) g_app->physics().setVelocity(m_id, v);
    return *this;
}
glm::vec3 GameObject::velocity() const {
    if (auto* rb = m_world->get<RigidBodyComponent>(m_id)) return rb->velocity;
    return {};
}
GameObject& GameObject::impulse(float x, float y, float z) {
    if (g_app) g_app->physics().applyImpulse(m_id, {x,y,z});
    return *this;
}
GameObject& GameObject::impulse(glm::vec3 v) {
    if (g_app) g_app->physics().applyImpulse(m_id, v);
    return *this;
}
GameObject& GameObject::addForce(glm::vec3 f) {
    if (g_app) g_app->physics().applyForce(m_id, f);
    return *this;
}
GameObject& GameObject::lockRotX(bool on) {
    if (auto* rb = m_world->get<RigidBodyComponent>(m_id)) rb->lockRotX = on;
    return *this;
}
GameObject& GameObject::lockRotY(bool on) {
    if (auto* rb = m_world->get<RigidBodyComponent>(m_id)) rb->lockRotY = on;
    return *this;
}
GameObject& GameObject::lockRotZ(bool on) {
    if (auto* rb = m_world->get<RigidBodyComponent>(m_id)) rb->lockRotZ = on;
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
//  AUDIO
// ─────────────────────────────────────────────────────────────────────────────
// Store one sound handle per entity in a static map
static std::unordered_map<uint32_t, SoundHandle> s_sounds;

GameObject& GameObject::playSound(const std::string& path, float vol) {
    auto h = Audio::play(path, vol);
    s_sounds[m_id] = h;
    return *this;
}
GameObject& GameObject::loopSound(const std::string& path, float vol) {
    auto h = Audio::loop(path, vol);
    s_sounds[m_id] = h;
    return *this;
}
GameObject& GameObject::stopSound() {
    auto it = s_sounds.find(m_id);
    if (it != s_sounds.end()) { Audio::stop(it->second); s_sounds.erase(it); }
    return *this;
}
GameObject& GameObject::soundVolume(float v) {
    auto it = s_sounds.find(m_id);
    if (it != s_sounds.end()) Audio::setVolume(it->second, v);
    return *this;
}
GameObject& GameObject::soundPitch(float v) {
    auto it = s_sounds.find(m_id);
    if (it != s_sounds.end()) Audio::setPitch(it->second, v);
    return *this;
}
GameObject& GameObject::sound3D(bool on) {
    // Re-play as 3D at object position if on
    if (on) {
        auto it = s_sounds.find(m_id);
        if (it != s_sounds.end())
            Audio::setPosition(it->second, transform().position);
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SCRIPTS
// ─────────────────────────────────────────────────────────────────────────────
GameObject& GameObject::onUpdate(std::function<void(GameObject&, float)> fn) {
    if (!m_world->has<ScriptComponent>(m_id))
        m_world->add<ScriptComponent>(m_id);
    m_world->get<ScriptComponent>(m_id)->onUpdate =
        [fn](GameObject self, float dt) { fn(self, dt); };
    return *this;
}
GameObject& GameObject::onStart(std::function<void(GameObject&)> fn) {
    // Store in script; App calls it once before first frame
    if (!m_world->has<ScriptComponent>(m_id))
        m_world->add<ScriptComponent>(m_id);
    // Wrap: call immediately if game already started, else defer via engine
    if (g_app) fn(*this);
    return *this;
}
GameObject& GameObject::onDestroy(std::function<void(GameObject&)> fn) {
    // Register in ScriptComponent — engine calls on destroy
    if (!m_world->has<ScriptComponent>(m_id))
        m_world->add<ScriptComponent>(m_id);
    m_world->get<ScriptComponent>(m_id)->onDestroy =
        [fn](GameObject self) { fn(self); };
    return *this;
}
GameObject& GameObject::onCollide(std::function<void(GameObject&, GameObject)> fn) {
    if (!m_world->has<ScriptComponent>(m_id))
        m_world->add<ScriptComponent>(m_id);
    m_world->get<ScriptComponent>(m_id)->onCollide =
        [fn](GameObject self, GameObject other) { fn(self, other); };
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
//  COLLIDER (legacy)
// ─────────────────────────────────────────────────────────────────────────────
GameObject& GameObject::addCollider() {
    if (!m_world->has<ColliderComponent>(m_id))
        m_world->add<ColliderComponent>(m_id);
    auto* t = m_world->get<TransformComponent>(m_id);
    if (t) m_world->get<ColliderComponent>(m_id)->halfSize = t->scale * 0.5f;
    return *this;
}
bool GameObject::overlaps(const GameObject& other) const {
    auto* ca = m_world->get<ColliderComponent>(m_id);
    auto* cb = m_world->get<ColliderComponent>(other.m_id);
    if (!ca || !cb) return false;
    auto* ta = m_world->get<TransformComponent>(m_id);
    auto* tb = m_world->get<TransformComponent>(other.m_id);
    if (!ta || !tb) return false;
    glm::vec3 pa = ta->position, pb = tb->position;
    glm::vec3 ha = ca->halfSize, hb = cb->halfSize;
    return std::abs(pa.x-pb.x) < ha.x+hb.x &&
           std::abs(pa.y-pb.y) < ha.y+hb.y &&
           std::abs(pa.z-pb.z) < ha.z+hb.z;
}

// ─────────────────────────────────────────────────────────────────────────────
//  TAG / NAME
// ─────────────────────────────────────────────────────────────────────────────
GameObject& GameObject::tag(const std::string& t) {
    if (!m_world->has<TagComp>(m_id)) m_world->add<TagComp>(m_id);
    m_world->get<TagComp>(m_id)->value = t;
    return *this;
}
std::string GameObject::tag() const {
    auto* tc = m_world->get<TagComp>(m_id);
    return tc ? tc->value : "";
}
std::string GameObject::name() const { return tag(); }
bool        GameObject::hasTag(const std::string& t) const { return tag() == t; }

// ─────────────────────────────────────────────────────────────────────────────
//  UTILITY
// ─────────────────────────────────────────────────────────────────────────────
float GameObject::distanceTo(const GameObject& other) const {
    auto* ta = m_world->get<TransformComponent>(m_id);
    auto* tb = m_world->get<TransformComponent>(other.m_id);
    if (!ta || !tb) return 0.f;
    return glm::length(ta->position - tb->position);
}
float GameObject::distanceTo(glm::vec3 point) const {
    auto* t = m_world->get<TransformComponent>(m_id);
    return t ? glm::length(t->position - point) : 0.f;
}

void GameObject::destroy() {
    if (!m_world || !m_id) return;
    // Call onDestroy if present
    if (auto* sc = m_world->get<ScriptComponent>(m_id))
        if (sc->onDestroy) sc->onDestroy(*this);
    // Stop sounds
    stopSound();
    // Remove physics body
    if (g_app && m_world->has<RigidBodyComponent>(m_id))
        g_app->physics().removeBody(m_id);
    m_world->destroy(m_id);
    m_id = 0;
}
