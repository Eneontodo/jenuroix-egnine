#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Physics.h — Jolt Physics integration
//
//  Setup (one time):
//    git clone https://github.com/jrouwe/JoltPhysics vendor/JoltPhysics
//    Then CMake picks it up automatically.
//
//  Usage:
//    // Add a rigidbody to a GameObject:
//    player.rigidBody(RigidBodyType::Dynamic)
//          .boxCollider({0.5f, 1.0f, 0.5f})
//          .mass(70.f);
//
//    // Access physics component directly:
//    auto* rb = world.get<RigidBodyComponent>(entity);
//    rb->setVelocity({0, 10, 0});  // jump!
//
//    // Raycast:
//    RayHit hit;
//    if (Physics::raycast(from, dir, 100.f, hit)) { ... }
// ─────────────────────────────────────────────────────────────────────────────
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// ── Forward declarations ──────────────────────────────────────────────────────
class PhysicsWorld;

// ── Enums ─────────────────────────────────────────────────────────────────────
enum class RigidBodyType {
    Static,    // never moves (walls, floors)
    Kinematic, // moved by code, not forces (elevators, moving platforms)
    Dynamic    // fully simulated (crates, balls, ragdolls)
};

enum class ColliderShape {
    Box,
    Sphere,
    Capsule,
    Cylinder,
    ConvexHull,
    Mesh        // static only — concave, for terrain/level geometry
};

// ── Collision filter layers ───────────────────────────────────────────────────
namespace PhysicsLayer {
    constexpr uint16_t NON_MOVING = 0;  // static geometry
    constexpr uint16_t MOVING     = 1;  // dynamic/kinematic bodies
    constexpr uint16_t SENSOR     = 2;  // triggers (no collision response)
    constexpr uint16_t CHARACTER  = 3;  // player/NPC character controllers
    constexpr uint16_t DEBRIS     = 4;  // small debris (doesn't collide with debris)
    constexpr uint16_t RAYCAST    = 5;  // ray-only, no physical collision
}

// ── Raycast result ────────────────────────────────────────────────────────────
struct RayHit {
    glm::vec3  point    = {};
    glm::vec3  normal   = {};
    float      distance = 0.f;
    uint32_t   entityID = 0;    // ECS entity that was hit (0 = none)
    bool       hit      = false;
    operator bool() const { return hit; }
};

// ── Collision event ───────────────────────────────────────────────────────────
struct HitEvent {
    uint32_t  entityA   = 0;
    uint32_t  entityB   = 0;
    glm::vec3 contactPoint = {};
    glm::vec3 normal       = {};
    float     impulse      = 0.f;
    bool      isTrigger    = false;
};

// ── RigidBodyComponent ────────────────────────────────────────────────────────
// Attach to any entity via world.add<RigidBodyComponent>(id)
// But prefer using the PhysicsWorld factory methods or GameObject API.
struct RigidBodyComponent {
    // ── Shape ─────────────────────────────────────────────────────────────────
    ColliderShape shape     = ColliderShape::Box;
    glm::vec3     halfExt   = {0.5f, 0.5f, 0.5f};  // Box / Cylinder half-extents
    float         radius    = 0.5f;                 // Sphere / Capsule
    float         height    = 1.0f;                 // Capsule half-height (excl radius)
    // Custom mesh data (ConvexHull / Mesh shapes)
    std::vector<glm::vec3> meshVerts;

    // ── Body ──────────────────────────────────────────────────────────────────
    RigidBodyType type        = RigidBodyType::Dynamic;
    float         mass        = 1.f;          // kg (ignored for Static)
    float         friction    = 0.5f;         // 0 = ice, 1 = rubber
    float         restitution = 0.2f;         // 0 = no bounce, 1 = perfect bounce
    float         linearDamp  = 0.05f;
    float         angularDamp = 0.05f;
    float         gravityScale= 1.f;          // multiplier for gravity (1 = normal)
    bool          isSensor    = false;        // trigger only — no collision response
    bool          lockRotX    = false;        // prevent rotation on axes
    bool          lockRotY    = false;
    bool          lockRotZ    = false;
    uint16_t      layer       = PhysicsLayer::MOVING;

    // ── Runtime state (set by PhysicsWorld each frame) ────────────────────────
    glm::vec3 velocity        = {};
    glm::vec3 angularVelocity = {};

    // ── Opaque Jolt handle (set internally — do not touch) ────────────────────
    uint32_t _bodyId = UINT32_MAX;

    // ── Callbacks ─────────────────────────────────────────────────────────────
    std::function<void(const HitEvent&)> onCollisionEnter;
    std::function<void(const HitEvent&)> onCollisionExit;
    std::function<void(const HitEvent&)> onTriggerEnter;
    std::function<void(const HitEvent&)> onTriggerExit;
};

// ── CharacterComponent ────────────────────────────────────────────────────────
// High-level character controller built on Jolt's CharacterVirtual
struct CharacterComponent {
    float height          = 1.8f;
    float radius          = 0.4f;
    float mass            = 70.f;
    float maxSlopeAngle   = 50.f;  // degrees
    float stepHeight      = 0.35f; // how tall a step the character can climb
    float maxSpeed        = 10.f;
    glm::vec3 desiredVel  = {};    // set this each frame to move

    bool  isGrounded      = false; // read-only — set by physics
    glm::vec3 velocity    = {};    // current velocity — read-only

    uint32_t _handle = UINT32_MAX;
};

// ── PhysicsWorld ──────────────────────────────────────────────────────────────
// One per scene. Managed by App.
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    void init();
    void shutdown();
    void step(float dt);          // advance simulation (call once per frame)

    // ── Body management ───────────────────────────────────────────────────────
    // Create a body for an entity. Call after adding RigidBodyComponent.
    void addBody   (uint32_t entityId, RigidBodyComponent& rb,
                    glm::vec3 position, glm::quat rotation = glm::quat{1,0,0,0});
    void removeBody(uint32_t entityId);

    // Add a character controller
    void addCharacter   (uint32_t entityId, CharacterComponent& ch,
                         glm::vec3 startPos);
    void removeCharacter(uint32_t entityId);

    // ── Per-frame sync ────────────────────────────────────────────────────────
    // After step(), call to write Jolt positions back into transform components
    // Signature: (entityId, newPosition, newRotation)
    void syncTransforms(
        std::function<void(uint32_t, glm::vec3, glm::quat)> writeFn);

    // ── Impulse / force ───────────────────────────────────────────────────────
    void applyImpulse      (uint32_t entityId, glm::vec3 impulse);
    void applyForce        (uint32_t entityId, glm::vec3 force);
    void applyTorque       (uint32_t entityId, glm::vec3 torque);
    void setVelocity       (uint32_t entityId, glm::vec3 vel);
    void setAngularVelocity(uint32_t entityId, glm::vec3 vel);
    void setPosition       (uint32_t entityId, glm::vec3 pos);
    void setGravityScale   (uint32_t entityId, float scale);
    void setRotation       (uint32_t entityId, glm::quat rot);

    // ── Character movement ────────────────────────────────────────────────────
    // Set desiredVelocity on CharacterComponent, then call this each frame
    void updateCharacter(uint32_t entityId, CharacterComponent& ch,
                         float dt, glm::vec3 gravity = {0,-9.81f,0});

    // ── Queries ───────────────────────────────────────────────────────────────
    RayHit  raycast(glm::vec3 origin, glm::vec3 dir, float maxDist,
                    uint16_t layerMask = 0xFFFF) const;

    // Returns all entities whose colliders overlap the sphere
    std::vector<uint32_t> sphereOverlap(glm::vec3 center, float radius,
                                         uint16_t layerMask = 0xFFFF) const;

    // Returns all entities whose colliders overlap the box
    std::vector<uint32_t> boxOverlap(glm::vec3 center, glm::vec3 halfExt,
                                      uint16_t layerMask = 0xFFFF) const;

    // ── Global settings ───────────────────────────────────────────────────────
    void setGravity(glm::vec3 g = {0, -9.81f, 0});

    // ── Collision callbacks ───────────────────────────────────────────────────
    void onCollisionEnter(std::function<void(const HitEvent&)> fn);
    void onCollisionExit (std::function<void(const HitEvent&)> fn);
    void onTriggerEnter  (std::function<void(const HitEvent&)> fn);
    void onTriggerExit   (std::function<void(const HitEvent&)> fn);

    // ── Debug ─────────────────────────────────────────────────────────────────
    int  bodyCount() const;
    bool isInitialized() const;

private:
    struct Impl;
    Impl* m_impl = nullptr;
};
