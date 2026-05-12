// Physics.cpp — Jolt Physics v5+ backend
// Rewritten with correct v5 API.

#include "physics/Physics.h"
#include "core/Log.h"

#if __has_include(<Jolt/Jolt.h>)
#  define JOLT_AVAILABLE 1
#else
#  define JOLT_AVAILABLE 0
#  pragma message("JoltPhysics not found — Physics system is a stub.")
#endif

#if JOLT_AVAILABLE

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <thread>
#include <unordered_map>

JPH_SUPPRESS_WARNINGS
using namespace JPH;

static inline uint32_t bodyKey(const BodyID& id) {
    return id.GetIndexAndSequenceNumber();
}

// ── Broad-phase layers ────────────────────────────────────────────────────────
namespace BPLayers {
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING    (1);
    static constexpr uint            NUM_LAYERS = 2;
}

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        for (auto& e : m_map) e = BPLayers::MOVING;
        m_map[PhysicsLayer::NON_MOVING] = BPLayers::NON_MOVING;
        m_map[PhysicsLayer::RAYCAST]    = BPLayers::NON_MOVING;
    }
    uint GetNumBroadPhaseLayers() const override { return BPLayers::NUM_LAYERS; }
    BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer l) const override {
        return (l < 16) ? m_map[l] : BPLayers::MOVING;
    }
    const char* GetBroadPhaseLayerName(BroadPhaseLayer layer) const override {
        return layer == BPLayers::NON_MOVING ? "NON_MOVING" : "MOVING";
    }
private:
    BroadPhaseLayer m_map[16] = {};
};

class OVBPLayerFilter final : public ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(ObjectLayer obj, BroadPhaseLayer bp) const override {
        if (obj == PhysicsLayer::NON_MOVING) return bp == BPLayers::MOVING;
        return true;
    }
};

class OVOLayerFilter final : public ObjectLayerPairFilter {
public:
    bool ShouldCollide(ObjectLayer a, ObjectLayer b) const override {
        if (a == PhysicsLayer::DEBRIS   && b == PhysicsLayer::DEBRIS)  return false;
        if (a == PhysicsLayer::SENSOR   || b == PhysicsLayer::SENSOR)  return false;
        return true;
    }
};

// ── Contact listener ──────────────────────────────────────────────────────────
class ContactListenerImpl final : public ContactListener {
public:
    std::function<void(const HitEvent&)> onEnter, onExit;
    std::unordered_map<uint32_t,uint32_t>* bodyToEntity = nullptr;

    ValidateResult OnContactValidate(const Body&, const Body&,
        RVec3Arg, const CollideShapeResult&) override {
        return ValidateResult::AcceptAllContactsForThisBodyPair;
    }
    void OnContactAdded(const Body& a, const Body& b,
        const ContactManifold& m, ContactSettings&) override
    {
        if (!onEnter) return;
        HitEvent ev;
        ev.entityA = getEntity(a.GetID());
        ev.entityB = getEntity(b.GetID());
        RVec3 cp   = m.GetWorldSpaceContactPointOn1(0);
        ev.contactPoint = { (float)cp.GetX(), (float)cp.GetY(), (float)cp.GetZ() };
        Vec3 n = m.mWorldSpaceNormal;
        ev.normal = { n.GetX(), n.GetY(), n.GetZ() };
        onEnter(ev);
    }
    void OnContactPersisted(const Body&, const Body&,
        const ContactManifold&, ContactSettings&) override {}
    void OnContactRemoved(const SubShapeIDPair& p) override {
        if (!onExit) return;
        HitEvent ev;
        ev.entityA = getEntity(p.GetBody1ID());
        ev.entityB = getEntity(p.GetBody2ID());
        onExit(ev);
    }
private:
    uint32_t getEntity(const BodyID& id) const {
        if (!bodyToEntity) return 0;
        auto it = bodyToEntity->find(bodyKey(id));
        return (it != bodyToEntity->end()) ? it->second : 0;
    }
};

// ── Helpers ───────────────────────────────────────────────────────────────────
static Vec3  toJ (glm::vec3 v) { return Vec3(v.x, v.y, v.z); }
static RVec3 toJR(glm::vec3 v) { return RVec3(v.x, v.y, v.z); }
static Quat  toJQ(glm::quat q) { return Quat(q.x, q.y, q.z, q.w); }
static glm::vec3 fJ (Vec3  v)  { return { v.GetX(),        v.GetY(),        v.GetZ()        }; }
static glm::vec3 fJR(RVec3 v)  { return { (float)v.GetX(),(float)v.GetY(),(float)v.GetZ()  }; }
static glm::quat fJQ(Quat  q)  { return { q.GetW(), q.GetX(), q.GetY(), q.GetZ() }; }

static ShapeRefC makeShape(const RigidBodyComponent& rb) {
    switch (rb.shape) {
    case ColliderShape::Sphere:   return new SphereShape(rb.radius);
    case ColliderShape::Capsule:  return new CapsuleShape(rb.height * 0.5f, rb.radius);
    case ColliderShape::Cylinder: return new CylinderShape(rb.height * 0.5f, rb.radius);
    case ColliderShape::ConvexHull: {
        Array<Vec3> pts;
        for (auto& v : rb.meshVerts) pts.push_back(toJ(v));
        auto res = ConvexHullShapeSettings(pts).Create();
        if (res.HasError()) return new BoxShape(toJ(rb.halfExt));
        return res.Get();
    }
    case ColliderShape::Mesh: {
        TriangleList tris;
        for (size_t i = 0; i + 2 < rb.meshVerts.size(); i += 3)
            tris.push_back(Triangle(toJ(rb.meshVerts[i]),
                                    toJ(rb.meshVerts[i+1]),
                                    toJ(rb.meshVerts[i+2])));
        auto res = MeshShapeSettings(tris).Create();
        if (res.HasError()) return new BoxShape(toJ(rb.halfExt));
        return res.Get();
    }
    default:
        return new BoxShape(toJ(rb.halfExt));
    }
}

#endif // JOLT_AVAILABLE

// ── Impl struct ───────────────────────────────────────────────────────────────
struct PhysicsWorld::Impl {
#if JOLT_AVAILABLE
    TempAllocatorImpl*   tempAlloc = nullptr;
    JobSystemThreadPool* jobSystem = nullptr;
    PhysicsSystem*       phys      = nullptr;
    BPLayerInterfaceImpl bpInterface;
    OVBPLayerFilter      ovbpFilter;
    OVOLayerFilter       ovoFilter;
    ContactListenerImpl  contactListener;
    BodyInterface*       bodyIface = nullptr;

    std::unordered_map<uint32_t, BodyID>   entityToBody;
    std::unordered_map<uint32_t, uint32_t> bodyToEntity;

    std::function<void(const HitEvent&)> cbEnter, cbExit, cbTrigEnter, cbTrigExit;
#endif
    bool ready = false;
};

// ── Public API ────────────────────────────────────────────────────────────────
PhysicsWorld::PhysicsWorld()  { m_impl = new Impl; }
PhysicsWorld::~PhysicsWorld() { shutdown(); delete m_impl; }

void PhysicsWorld::init() {
#if JOLT_AVAILABLE
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    m_impl->tempAlloc = new TempAllocatorImpl(32 * 1024 * 1024);
    m_impl->jobSystem = new JobSystemThreadPool(
        cMaxPhysicsJobs, cMaxPhysicsBarriers,
        (int)std::thread::hardware_concurrency() - 1);

    m_impl->phys = new PhysicsSystem();
    m_impl->phys->Init(65536, 0, 65536, 32768,
                       m_impl->bpInterface,
                       m_impl->ovbpFilter,
                       m_impl->ovoFilter);

    m_impl->contactListener.bodyToEntity = &m_impl->bodyToEntity;
    m_impl->contactListener.onEnter = [this](const HitEvent& e){
        if (m_impl->cbEnter) m_impl->cbEnter(e);
    };
    m_impl->contactListener.onExit = [this](const HitEvent& e){
        if (m_impl->cbExit) m_impl->cbExit(e);
    };
    m_impl->phys->SetContactListener(&m_impl->contactListener);
    m_impl->phys->SetGravity(Vec3(0, -9.81f, 0));
    m_impl->bodyIface = &m_impl->phys->GetBodyInterface();
    m_impl->ready = true;
    LOG_INFO("Physics: Jolt v5 initialised");
#else
    LOG_WARN("Physics: Jolt not available");
#endif
}

void PhysicsWorld::shutdown() {
#if JOLT_AVAILABLE
    if (!m_impl->ready) return;
    for (auto& [eid, bid] : m_impl->entityToBody)
        m_impl->bodyIface->RemoveBody(bid);
    m_impl->entityToBody.clear();
    m_impl->bodyToEntity.clear();
    delete m_impl->phys;
    delete m_impl->jobSystem;
    delete m_impl->tempAlloc;
    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
    m_impl->ready = false;
#endif
}

void PhysicsWorld::step(float dt) {
#if JOLT_AVAILABLE
    if (m_impl->ready)
        m_impl->phys->Update(dt, 1, m_impl->tempAlloc, m_impl->jobSystem);
#endif
}

void PhysicsWorld::addBody(uint32_t eid, RigidBodyComponent& rb,
                            glm::vec3 pos, glm::quat rot)
{
#if JOLT_AVAILABLE
    if (!m_impl->ready) return;
    ShapeRefC shape = makeShape(rb);

    EMotionType mt = EMotionType::Static;
    ObjectLayer ol = PhysicsLayer::NON_MOVING;
    switch (rb.type) {
    case RigidBodyType::Dynamic:
        mt = EMotionType::Dynamic;
        ol = rb.isSensor ? (ObjectLayer)PhysicsLayer::SENSOR : (ObjectLayer)PhysicsLayer::MOVING;
        break;
    case RigidBodyType::Kinematic:
        mt = EMotionType::Kinematic;
        ol = PhysicsLayer::MOVING;
        break;
    default: break;
    }

    BodyCreationSettings bcs(shape, toJR(pos), toJQ(rot), mt, ol);
    if (rb.type == RigidBodyType::Dynamic) {
        bcs.mMassPropertiesOverride.mMass = rb.mass;
        bcs.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
        bcs.mLinearDamping          = rb.linearDamp;
        bcs.mAngularDamping         = rb.angularDamp;
    }
    bcs.mFriction    = rb.friction;
    bcs.mRestitution = rb.restitution;
    bcs.mIsSensor    = rb.isSensor;
    bcs.mUserData    = (uint64_t)eid;

    Body* body = m_impl->bodyIface->CreateBody(bcs);
    if (!body) { LOG_ERROR("Physics: CreateBody failed for entity " << eid); return; }

    BodyID bid = body->GetID();
    m_impl->bodyIface->AddBody(bid, EActivation::Activate);
    m_impl->entityToBody[eid]          = bid;
    m_impl->bodyToEntity[bodyKey(bid)] = eid;
    rb._bodyId = bodyKey(bid);
    LOG_INFO("Physics: body added entity=" << eid);
#endif
}

void PhysicsWorld::removeBody(uint32_t eid) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it == m_impl->entityToBody.end()) return;
    m_impl->bodyIface->RemoveBody(it->second);
    m_impl->bodyToEntity.erase(bodyKey(it->second));
    m_impl->entityToBody.erase(it);
#endif
}

void PhysicsWorld::syncTransforms(
    std::function<void(uint32_t, glm::vec3, glm::quat)> writeFn)
{
#if JOLT_AVAILABLE
    if (!m_impl->ready) return;
    for (auto& [eid, bid] : m_impl->entityToBody)
        if (m_impl->bodyIface->IsActive(bid))
            writeFn(eid,
                fJR(m_impl->bodyIface->GetPosition(bid)),
                fJQ(m_impl->bodyIface->GetRotation(bid)));
#endif
}

void PhysicsWorld::applyImpulse(uint32_t eid, glm::vec3 v) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it != m_impl->entityToBody.end())
        m_impl->bodyIface->AddImpulse(it->second, toJ(v));
#endif
}
void PhysicsWorld::applyForce(uint32_t eid, glm::vec3 v) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it != m_impl->entityToBody.end())
        m_impl->bodyIface->AddForce(it->second, toJ(v));
#endif
}
void PhysicsWorld::applyTorque(uint32_t eid, glm::vec3 v) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it != m_impl->entityToBody.end())
        m_impl->bodyIface->AddTorque(it->second, toJ(v));
#endif
}
void PhysicsWorld::setVelocity(uint32_t eid, glm::vec3 v) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it != m_impl->entityToBody.end())
        m_impl->bodyIface->SetLinearVelocity(it->second, toJ(v));
#endif
}
void PhysicsWorld::setGravityScale(uint32_t eid, float s) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it != m_impl->entityToBody.end())
        m_impl->bodyIface->SetGravityFactor(it->second, s);
#endif
}
void PhysicsWorld::setAngularVelocity(uint32_t eid, glm::vec3 v) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it != m_impl->entityToBody.end())
        m_impl->bodyIface->SetAngularVelocity(it->second, toJ(v));
#endif
}
void PhysicsWorld::setPosition(uint32_t eid, glm::vec3 v) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it != m_impl->entityToBody.end())
        m_impl->bodyIface->SetPosition(it->second, toJR(v), EActivation::Activate);
#endif
}
void PhysicsWorld::setRotation(uint32_t eid, glm::quat q) {
#if JOLT_AVAILABLE
    auto it = m_impl->entityToBody.find(eid);
    if (it != m_impl->entityToBody.end())
        m_impl->bodyIface->SetRotation(it->second, toJQ(q), EActivation::Activate);
#endif
}

RayHit PhysicsWorld::raycast(glm::vec3 origin, glm::vec3 dir,
                               float maxDist, uint16_t) const
{
    RayHit result;
#if JOLT_AVAILABLE
    if (!m_impl->ready) return result;
    glm::vec3 nd = glm::length(dir) > 0 ? glm::normalize(dir) : dir;
    RRayCast ray(toJR(origin), toJ(nd * maxDist));
    RayCastResult hit;
    if (m_impl->phys->GetNarrowPhaseQuery().CastRay(ray, hit)) {
        BodyID bid     = hit.mBodyID;
        result.hit     = true;
        result.distance = hit.mFraction * maxDist;
        result.point   = fJR(ray.GetPointOnRay(hit.mFraction));

        BodyLockRead lock(m_impl->phys->GetBodyLockInterface(), bid);
        if (lock.Succeeded())
            result.normal = fJ(lock.GetBody().GetWorldSpaceSurfaceNormal(
                hit.mSubShapeID2, ray.GetPointOnRay(hit.mFraction)));

        auto it = m_impl->bodyToEntity.find(bodyKey(bid));
        if (it != m_impl->bodyToEntity.end()) result.entityID = it->second;
    }
#endif
    return result;
}

void PhysicsWorld::setGravity(glm::vec3 g) {
#if JOLT_AVAILABLE
    if (m_impl->ready) m_impl->phys->SetGravity(toJ(g));
#endif
}

void PhysicsWorld::onCollisionEnter(std::function<void(const HitEvent&)> fn) { m_impl->cbEnter     = fn; }
void PhysicsWorld::onCollisionExit (std::function<void(const HitEvent&)> fn) { m_impl->cbExit      = fn; }
void PhysicsWorld::onTriggerEnter  (std::function<void(const HitEvent&)> fn) { m_impl->cbTrigEnter = fn; }
void PhysicsWorld::onTriggerExit   (std::function<void(const HitEvent&)> fn) { m_impl->cbTrigExit  = fn; }

int  PhysicsWorld::bodyCount()     const { return (int)m_impl->entityToBody.size(); }
bool PhysicsWorld::isInitialized() const { return m_impl->ready; }

std::vector<uint32_t> PhysicsWorld::sphereOverlap(glm::vec3, float,   uint16_t) const { return {}; }
std::vector<uint32_t> PhysicsWorld::boxOverlap   (glm::vec3, glm::vec3, uint16_t) const { return {}; }
void PhysicsWorld::addCharacter   (uint32_t, CharacterComponent&, glm::vec3) {}
void PhysicsWorld::removeCharacter(uint32_t) {}
void PhysicsWorld::updateCharacter(uint32_t, CharacterComponent&, float, glm::vec3) {}
