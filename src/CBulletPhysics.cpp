#include "CBulletPhysics.h"
#include "CEngine.h"
#include "CConVarManager.h"

CBulletWorld::CBulletWorld()
{
    GhostPairCallback = std::make_unique<btGhostPairCallback>();
    Config = std::make_unique<btDefaultCollisionConfiguration>();
    Dispatcher = std::make_unique<btCollisionDispatcher>(Config.get());
    Broadphase = std::make_unique<btDbvtBroadphase>();
    Solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    BulletWorld = std::make_unique<btDiscreteDynamicsWorld>(Dispatcher.get(), Broadphase.get(), Solver.get(), Config.get());

    BulletWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(GhostPairCallback.get());
}

void CBulletWorld::V_SetGravity(const glm::vec3& _gravity)
{
    BulletWorld->setGravity(CBulletPhysics::GetVector(_gravity));
    Log::Instance() << "Set gravity " << _gravity << Log::Endl;
}

glm::vec3 CBulletWorld::GetGravity() const { return CBulletPhysics::GetVector(BulletWorld->getGravity()); }

void CBulletWorld::V_Step(float deltaTime)
{
    int substeps = 10;
    COMPONENT_CALL_GET(substeps, CConVarManager, GetConVarValue<int>("phys.substeps", 10));

    int fixedTimeStepN = 60;
    COMPONENT_CALL_GET(fixedTimeStepN, CConVarManager, GetConVarValue<int>("phys.fixedtimestep", 60));

    float fixedTimeStep = 1.0f / 60.0f;
    if(fixedTimeStepN == 0)
    {
        fixedTimeStep = deltaTime;
    }
    else if(fixedTimeStep > 0)
    {
        fixedTimeStep = 1.0f / static_cast<float>(fixedTimeStepN);
    }

    BulletWorld->stepSimulation(deltaTime, substeps, fixedTimeStep);
}

btVector3 CBulletPhysics::GetVector(const glm::vec3& vec)
{
    return btVector3(vec.x, vec.y, vec.z);
}

glm::vec3 CBulletPhysics::GetVector(const btVector3& vec)
{
    return glm::vec3(vec.x(), vec.y(), vec.z());
}

btQuaternion CBulletPhysics::GetQuat(const glm::quat& qua)
{
    btQuaternion ret;

    ret.setX(qua.x);
    ret.setY(qua.y);
    ret.setZ(qua.z);
    ret.setW(qua.w);

    return ret;
}

glm::quat CBulletPhysics::GetQuat(const btQuaternion& qua)
{
    glm::quat ret;

    ret.x = qua.getX();
    ret.y = qua.getY();
    ret.z = qua.getZ();
    ret.w = qua.getW();

    return ret;
}

btTransform CBulletPhysics::GetTransform(const CTransformBase& trans)
{
    btTransform ret;

    ret.setIdentity();
    ret.setOrigin(GetVector(trans.GetPosition()));
    ret.setRotation(GetQuat(trans.GetRotation()));

    return ret;
}

void CBulletPhysics::GetTransform(CTransformBase& trans, const btTransform& btTrans)
{
    trans.SetPosition(GetVector(btTrans.getOrigin()));
    trans.SetRotation(GetQuat(btTrans.getRotation()));
}

void BulletShapes::CTriMesh::V_Build()
{
    Mesh = std::make_unique<btTriangleMesh>();

    for(auto& tri : Triangles)
    {
        //auto& v1 = Vertices.at(tri.x - 1);
        auto& v1 = Vertices.at(tri.x);
        auto& v2 = Vertices.at(tri.y);
        auto& v3 = Vertices.at(tri.z);

        Mesh->addTriangle
        (
            CBulletPhysics::GetVector(v1),
            CBulletPhysics::GetVector(v2),
            CBulletPhysics::GetVector(v3)
        );
    }

    InternalShape = std::make_unique<btBvhTriangleMeshShape>(Mesh.get(), true);
}

void BulletShapes::CConvex::V_Build()
{
    Log::Instance() << "Convex vertices.size() is " << Vertices.size() << Log::Endl;
    InternalShape = std::make_unique<btConvexHullShape>((float*)Vertices.data(), Vertices.size(), sizeof(glm::vec3));
}

void BulletShapes::CCompound::V_Build()
{
    InternalShape = std::make_unique<btCompoundShape>();
    auto compound = dynamic_cast<btCompoundShape*>(InternalShape.get());

    for(auto& ch : Childs)
    {
        auto bulletShape = std::dynamic_pointer_cast<BulletShapes::CBasic>(ch.Shape);
        if(!bulletShape) { continue; }

        compound->addChildShape(CBulletPhysics::GetTransform(ch.Transform), bulletShape->InternalShape.get());
    }
}

void BulletShapes::CCapsule::V_Build()
{
    InternalShape = std::make_unique<btCapsuleShape>(Radius, Height);
}

void BulletShapes::CSphere::V_Build()
{
    InternalShape = std::make_unique<btSphereShape>(Radius);
}

void BulletShapes::CBox::V_Build()
{
    InternalShape = std::make_unique<btBoxShape>(CBulletPhysics::GetVector(HalfExtents));
}

CBulletPhysics::CBulletPhysics()
{
    ShapesFactory.replace<BulletShapes::CTriMesh>("trimesh");
    ShapesFactory.replace<BulletShapes::CConvex>("convex");
    ShapesFactory.replace<BulletShapes::CCompound>("compound");
    ShapesFactory.replace<BulletShapes::CCapsule>("capsule");
    ShapesFactory.replace<BulletShapes::CSphere>("sphere");
    ShapesFactory.replace<BulletShapes::CBox>("box");
}

std::unique_ptr<CPhysicsWorld> CBulletPhysics::V_CreateWorld()
{
    return std::make_unique<CBulletWorld>();
}

btCollisionShape* CBulletUnit::GetBulletCollision(std::shared_ptr<CCollision> _col)
{
    Log::ErrInstance() << "CBulletUnit::GetCollision\n";
    if(!_col) { Log::ErrInstance() << "NULL _col\n"; }
    auto __shape = _col->Shape.get();
    if(!__shape) { Log::ErrInstance() << "NULL __shape\n"; }
    auto _shape = dynamic_cast<BulletShapes::CBasic*>(__shape);
    if(!_shape) { Log::ErrInstance() << "NULL _shape\n"; }
    auto& shape = _shape->InternalShape;
    if(!shape) { Log::ErrInstance() << "NULL shape\n"; }

    return shape.get();
}

CBulletRigidBody::CBulletRigidBody(std::shared_ptr<CCollision> _col, const CTransform& _start_transform, float mass)
{
    m_setCollisionPtr(_col);
    auto shape = CBulletUnit::GetBulletCollision(_col);

    if (mass != 0.0f)
    {
        shape->calculateLocalInertia(mass, LocalInertia);
    }

    auto start_transform = CBulletPhysics::GetTransform(_start_transform);
    MotionState = std::make_unique<btDefaultMotionState>(start_transform);

    btRigidBody::btRigidBodyConstructionInfo rbInfo
    (
        mass,
        MotionState.get(),
        shape,
        LocalInertia
    );

    Log::Instance() << "CBulletRigidBody::CBulletRigidBody mass is " << mass << Log::Endl;

    RigidBody = std::make_unique<btRigidBody>(rbInfo);
    RigidBody->setUserPointer(static_cast<CWorldUnit*>(this));

    Log::Instance() << "[WORLD] Setting user pointer " << this << Log::Endl;

    RigidBody->setWorldTransform(start_transform);
    //world->addRigidBody(body);
}

void CBulletRigidBody::V_SetMass(float mass)
{
    btVector3 localInertia(0, 0, 0);
	RigidBody->getCollisionShape()->calculateLocalInertia(mass, localInertia);
	RigidBody->setMassProps(mass, localInertia);
}

void CBulletRigidBody::SetLinearVelocity(const glm::vec3& vel) { RigidBody->setLinearVelocity(CBulletPhysics::GetVector(vel)); }
void CBulletRigidBody::SetAngularVelocity(const glm::vec3& ang_vel) { RigidBody->setAngularVelocity(CBulletPhysics::GetVector(ang_vel)); }

glm::vec3 CBulletRigidBody::GetLinearVelocity() const { return CBulletPhysics::GetVector(RigidBody->getLinearVelocity()); }
glm::vec3 CBulletRigidBody::GetAngularVelocity() const { return CBulletPhysics::GetVector(RigidBody->getAngularVelocity()); }

void CBulletRigidBody::Activate() { return RigidBody->activate(); }

void CBulletRigidBody::SetTransform(const CTransform& trans)
{
    auto btTrans = CBulletPhysics::GetTransform(trans);

    RigidBody->getMotionState()->setWorldTransform(btTrans);
    RigidBody->setWorldTransform(btTrans);
}

CTransform CBulletRigidBody::GetInternalTransform()
{
    CTransform ret;
    CBulletPhysics::GetTransform(ret, RigidBody->getWorldTransform());

    return ret;
}

void CBulletKinematicBody::SetTransform(const CTransform& trans)
{
    auto btTrans = CBulletPhysics::GetTransform(trans);

    RigidBody->getMotionState()->setWorldTransform(btTrans);
    RigidBody->setWorldTransform(btTrans);
}

CTransform CBulletKinematicBody::GetInternalTransform()
{
    CTransform ret;
    CBulletPhysics::GetTransform(ret, RigidBody->getWorldTransform());

    return ret;
}

void CBulletGhostBody::SetTransform(const CTransform& trans)
{
    auto btTrans = CBulletPhysics::GetTransform(trans);
    GhostObject->setWorldTransform(btTrans);
}

CTransform CBulletGhostBody::GetInternalTransform()
{
    CTransform ret;
    CBulletPhysics::GetTransform(ret, GhostObject->getWorldTransform());

    return ret;
}

CBulletKinematicBody::CBulletKinematicBody(std::shared_ptr<CCollision> _col, const CTransform& _start_transform)
{
    m_setCollisionPtr(_col);
    auto shape = CBulletUnit::GetBulletCollision(_col);

    auto start_transform = CBulletPhysics::GetTransform(_start_transform);
    MotionState = std::make_unique<btDefaultMotionState>(start_transform);

    btRigidBody::btRigidBodyConstructionInfo rbInfo
    (
        0.0f, //mass MUST be zero
        MotionState.get(),
        shape
    );

    RigidBody = std::make_unique<btRigidBody>(rbInfo);
    RigidBody->setUserPointer(static_cast<CWorldUnit*>(this));
    RigidBody->setCollisionFlags(RigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

    Log::Instance() << "[WORLD] Setting user pointer " << this << Log::Endl;

    RigidBody->setActivationState(DISABLE_DEACTIVATION);
    //world->addRigidBody(body);
}

CBulletBody::~CBulletBody()
{
    if(m_addedToWorld && RigidBody)
    {
        Log::Instance() << "[WORLD] RemoveRigidBody " << GetCollision()->Name << Log::Endl;
        m_addedToWorld->BulletWorld->removeRigidBody(RigidBody.get());
    }
}

CBulletGhostBody::~CBulletGhostBody()
{
    if(m_addedToWorld && GhostObject)
    {
        Log::Instance() << "[WORLD] RemoveGhostBody " << GetCollision()->Name << Log::Endl;
        m_addedToWorld->BulletWorld->removeCollisionObject(GhostObject.get());
    }
}

CBulletWorld::~CBulletWorld()
{
    Units.clear(); //TODO manual deletion?
}

CBulletGhostBody::CBulletGhostBody(std::shared_ptr<CCollision> _col, const CTransform& _start_transform)
{
    m_setCollisionPtr(_col);
    auto shape = CBulletUnit::GetBulletCollision(_col);
    auto start_transform = CBulletPhysics::GetTransform(_start_transform);

    /*
    btCollisionWorld::ClosestConvexResultCallback cb;
    cb.hasHit();
    cb.m_closestHitFraction;
    cb.m_collisionFilterGroup;
    cb.m_collisionFilterMask;
    cb.m_convexFromWorld;
    cb.m_convexToWorld;
    cb.m_hitCollisionObject;
    cb.m_hitNormalWorld;
    cb.m_hitPointWorld;
    */

    GhostObject = std::make_unique<btPairCachingGhostObject>(); //TODO defaulting to btPairCachingGhostObject?
    GhostObject->setUserPointer(static_cast<CWorldUnit*>(this));

    Log::Instance() << "[WORLD] Setting user pointer " << this << Log::Endl;

    GhostObject->setCollisionShape(shape);
    GhostObject->setWorldTransform(start_transform);

    GhostObject->setCollisionFlags(GhostObject->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
    //world->addCollisionObject(GhostObject, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::AllFilter);
}

void CBulletWorld::V_AddRigidBody(CRigidBody* rgBody)
{
    Log::Instance() << "[WORLD] V_AddRigidBody " << rgBody->GetCollision()->Name << Log::Endl;

    auto btBody = dynamic_cast<CBulletRigidBody*>(rgBody);
    BulletWorld->addRigidBody(btBody->RigidBody.get());

    btBody->m_addedToWorld = this;
}

void CBulletWorld::V_AddKinematicBody(CKinematicBody* knBody)
{
    Log::Instance() << "[WORLD] V_AddKinematicBody " << knBody->GetCollision()->Name << Log::Endl;

    auto btBody = dynamic_cast<CBulletKinematicBody*>(knBody);
    BulletWorld->addRigidBody(btBody->RigidBody.get());

    btBody->m_addedToWorld = this;
}

void CBulletWorld::V_AddGhostBody(CGhostBody* ghBody)
{
    Log::Instance() << "[WORLD] V_AddGhostBody " << ghBody->GetCollision()->Name << Log::Endl;

    auto btBody = dynamic_cast<CBulletGhostBody*>(ghBody);
    BulletWorld->addCollisionObject(btBody->GhostObject.get(), btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::AllFilter);

    btBody->m_addedToWorld = this;
}

void CBulletRigidBody::V_SetCollision(std::shared_ptr<CCollision> _col)
{
    auto shape = CBulletUnit::GetBulletCollision(_col);
    RigidBody->setCollisionShape(shape);
}

void CBulletKinematicBody::V_SetCollision(std::shared_ptr<CCollision> _col)
{
    auto shape = CBulletUnit::GetBulletCollision(_col);
    RigidBody->setCollisionShape(shape);
}

void CBulletGhostBody::V_SetCollision(std::shared_ptr<CCollision> _col)
{
    auto shape = CBulletUnit::GetBulletCollision(_col);
    GhostObject->setCollisionShape(shape);
}

std::unique_ptr<CRigidBody> CBulletWorld::V_CreateRigidBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform, float mass)
{
    return std::make_unique<CBulletRigidBody>(_collision, startTransform, mass);
}

std::unique_ptr<CKinematicBody> CBulletWorld::V_CreateKinematicBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform)
{
    return std::make_unique<CBulletKinematicBody>(_collision, startTransform);
}

std::unique_ptr<CGhostBody> CBulletWorld::V_CreateGhostBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform)
{
    return std::make_unique<CBulletGhostBody>(_collision, startTransform);
}

LINK_PHYSICS_ENGINE_TO_CLASS(CBulletPhysics, bulletphysics);

BulletConvexSweepCallback::BulletConvexSweepCallback
(
    const btVector3& from,
    const btVector3& to,
    IConvexSweepCallback* userCb
) : btCollisionWorld::ClosestConvexResultCallback(from, to), m_UserCallback(userCb) {}

bool BulletConvexSweepCallback::needsCollision(btBroadphaseProxy* proxy0) const
{
    auto proxyPtr = proxy0->m_clientObject;
    auto it = std::find_if(ExcludeList.begin(), ExcludeList.end(), [proxyPtr](CWorldUnit* unit) -> bool
    {
        auto btbody = dynamic_cast<CBulletBody*>(unit);
        auto btghost = dynamic_cast<CBulletGhostBody*>(unit);

        if(btbody)
        {
            return static_cast<btCollisionObject*>(btbody->RigidBody.get()) == proxyPtr;
        }
        else if(btghost)
        {
            return static_cast<btCollisionObject*>(btghost->GhostObject.get()) == proxyPtr;
        }
        return false;
    });

    if (it != ExcludeList.end())
    {
        return false;
    }
    return ClosestConvexResultCallback::needsCollision(proxy0);
}

btScalar BulletConvexSweepCallback::addSingleResult(btCollisionWorld::LocalConvexResult& r, bool normalInWorldSpace)
{
    if (!m_UserCallback)
    {
        return ClosestConvexResultCallback::addSingleResult(r, normalInWorldSpace);
    }

    CSweepHit hit;
    hit.HitFraction = r.m_hitFraction;
    hit.HitPointWorld = CBulletPhysics::GetVector(r.m_hitPointLocal);
    hit.HitNormalWorld = CBulletPhysics::GetVector
    (
        normalInWorldSpace ? r.m_hitNormalLocal
        : r.m_hitCollisionObject->getWorldTransform().getBasis() * r.m_hitNormalLocal
    );

    hit.HitUnit = static_cast<CWorldUnit*>(r.m_hitCollisionObject->getUserPointer());

    if (!m_UserCallback->OnHit(hit))
    {
        return 1.0f; //ignore this hit
    }
    return ClosestConvexResultCallback::addSingleResult(r, normalInWorldSpace);
}

BulletConvexSweepResult::BulletConvexSweepResult(const btCollisionWorld::ClosestConvexResultCallback& cb)
{
    if (cb.hasHit())
    {
        m_HasHit = true;
        m_Hit.HitFraction = cb.m_closestHitFraction;
        m_Hit.HitPointWorld = CBulletPhysics::GetVector(cb.m_hitPointWorld);
        m_Hit.HitNormalWorld = CBulletPhysics::GetVector(cb.m_hitNormalWorld);
        m_Hit.HitUnit = static_cast<CWorldUnit*>(cb.m_hitCollisionObject->getUserPointer());
    }
}

bool BulletConvexSweepResult::HasHit() const { return m_HasHit; }
const CSweepHit& BulletConvexSweepResult::GetHit() const { return m_Hit; }

BulletContactPoint::BulletContactPoint(const btManifoldPoint& pt) : m_Pt(pt) {}

glm::vec3 BulletContactPoint::GetPositionWorldOnA() const
{
    return CBulletPhysics::GetVector(m_Pt.getPositionWorldOnA());
}

glm::vec3 BulletContactPoint::GetPositionWorldOnB() const
{
    return CBulletPhysics::GetVector(m_Pt.getPositionWorldOnB());
}

glm::vec3 BulletContactPoint::GetNormalWorldOnB() const
{
    return CBulletPhysics::GetVector(m_Pt.m_normalWorldOnB);
}

float BulletContactPoint::GetDistance() const
{
    return m_Pt.getDistance();
}

BulletContactManifold::BulletContactManifold(btPersistentManifold* m) : m_Manifold(m) {}

int BulletContactManifold::GetNumContacts() const
{
    return m_Manifold->getNumContacts();
}

std::unique_ptr<IContactPoint> BulletContactManifold::GetContact(int index) const
{
    return std::make_unique<BulletContactPoint>(m_Manifold->getContactPoint(index));
}

CWorldUnit* BulletContactManifold::GetUnitA() const
{
    return static_cast<CWorldUnit*>(m_Manifold->getBody0()->getUserPointer());
}

CWorldUnit* BulletContactManifold::GetUnitB() const
{
    return static_cast<CWorldUnit*>(m_Manifold->getBody1()->getUserPointer());
}

BulletGhostOverlapResult::BulletGhostOverlapResult
(
    const btGhostObject* ghost,
    btBroadphaseInterface* broadphase,
    btDispatcher* dispatcher,
    const CBulletWorld* world
) : m_Ghost(ghost), m_Broadphase(broadphase), m_Dispatcher(dispatcher), m_World(world) {}

int BulletGhostOverlapResult::GetNumOverlappingUnits() const
{
    return m_Ghost->getNumOverlappingObjects();
}

CWorldUnit* BulletGhostOverlapResult::GetOverlappingUnit(int index) const
{
    auto* obj = m_Ghost->getOverlappingObject(index);
    return static_cast<CWorldUnit*>(obj->getUserPointer());
}

btCollisionObject* CBulletBody::GetBulletObject() const
{
    return static_cast<btCollisionObject*>(RigidBody.get());
}

btCollisionObject* CBulletGhostBody::GetBulletObject() const
{
    return static_cast<btCollisionObject*>(GhostObject.get());
}

std::vector<std::unique_ptr<IContactManifold>> BulletGhostOverlapResult::GetContactManifolds(CWorldUnit* other) const
{
    auto* ghost = static_cast<btPairCachingGhostObject*>
    (
        const_cast<btGhostObject*>(m_Ghost)
    );

    btDispatcher* dispatcher = m_World->BulletWorld->getDispatcher();
    const btDispatcherInfo& dispatchInfo = m_World->BulletWorld->getDispatchInfo();

    dispatcher->dispatchAllCollisionPairs
    (
        ghost->getOverlappingPairCache(),
        dispatchInfo,
        dispatcher
    );

    std::vector<std::unique_ptr<IContactManifold>> result;

    if (!m_Ghost || !other)
    {
        Log::ErrInstance() << "GetContactManifolds ERR#0\n";
        return result;
    }

    auto* otherBullet = dynamic_cast<CBulletUnit*>(other);
    if (!otherBullet)
    {
        Log::ErrInstance() << "GetContactManifolds ERR#1\n";
        return result;
    }

    btCollisionObject* otherObj = otherBullet->GetBulletObject();
    if (!otherObj)
    {
        Log::ErrInstance() << "GetContactManifolds ERR#2\n";
        return result;
    }

    auto* pairCache = static_cast<btPairCachingGhostObject*>(const_cast<btGhostObject*>(m_Ghost))->getOverlappingPairCache();
    const auto& pairs = pairCache->getOverlappingPairArray();

    for (int i = 0; i < pairs.size(); ++i)
    {
        const btBroadphasePair& pair = pairs[i];

        btCollisionObject* a = static_cast<btCollisionObject*>(pair.m_pProxy0->m_clientObject);
        btCollisionObject* b = static_cast<btCollisionObject*>(pair.m_pProxy1->m_clientObject);

        if (!((a == m_Ghost && b == otherObj) || (b == m_Ghost && a == otherObj)))
        {
            Log::ErrInstance() << "GetContactManifolds ERR#3:1\n";
            continue;
        }

        if (!pair.m_algorithm)
        {
            Log::ErrInstance() << "GetContactManifolds ERR#3:2\n";
            continue;
        }

        btManifoldArray manifolds;
        pair.m_algorithm->getAllContactManifolds(manifolds);

        for (int m = 0; m < manifolds.size(); ++m)
        {
            btPersistentManifold* manifold = manifolds[m];

            if (manifold->getNumContacts() > 0)
            {
                result.push_back(std::make_unique<BulletContactManifold>(manifold));
            }
        }
    }
    return result;
}

std::unique_ptr<IConvexSweepResult> CBulletWorld::V_ConvexSweep
(
    std::shared_ptr<CCollision> shape,
    const CTransform& from,
    const CTransform& to,
    IConvexSweepCallback* callback,
    std::vector<CWorldUnit*> excludeList
)
{
    btTransform btFrom = CBulletPhysics::GetTransform(from);
    btTransform btTo = CBulletPhysics::GetTransform(to);

    BulletConvexSweepCallback cb
    (
        btFrom.getOrigin(),
        btTo.getOrigin(),
        callback
    );

    cb.ExcludeList = std::move(excludeList);

    auto _btcol = dynamic_cast<BulletShapes::CBasic*>(shape->Shape.get());
    if(!_btcol)
    {
        Log::ErrInstance() << "NULL _btcol typed " << shape->GetType() << " with collision " << shape->Shape->GetType() << Log::Endl;
    }

    auto btcol = _btcol->InternalShape.get();

    BulletWorld->convexSweepTest
    (
        static_cast<btConvexShape*>(btcol),
        btFrom,
        btTo,
        cb
    );

    return std::make_unique<BulletConvexSweepResult>(cb);
}

std::unique_ptr<IGhostOverlapResult> CBulletWorld::V_GetGhostOverlaps(const CGhostBody* ghost) const
{
    auto _btcol = dynamic_cast<const CBulletGhostBody*>(ghost);
    auto btGhost = _btcol->GhostObject.get();

    return std::make_unique<BulletGhostOverlapResult>
    (
        btGhost,
        Broadphase.get(),
        Dispatcher.get(),
        this
    );
}