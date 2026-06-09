#pragma once
#include "CPhysicsEngine.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h" //TODO hack

//shapes

namespace BulletShapes
{
    class CBasic : public virtual PhysicsShapes::CBasic
    {
    public:
        std::unique_ptr<btCollisionShape> InternalShape;
    };

    class CTriMesh : public virtual CBasic, public PhysicsShapes::CTriMesh
    {
    public:
        void V_Build() override;
        std::unique_ptr<btTriangleMesh> Mesh;
    };

    class CConvex : public virtual CBasic, public PhysicsShapes::CConvex
    {
    public:
        void V_Build() override;
    };

    class CCompound : public virtual CBasic, public PhysicsShapes::CCompound
    {
    public:
        void V_Build() override;
    };

    class CCapsule : public virtual CBasic, public PhysicsShapes::CCapsule
    {
    public:
        void V_Build() override;
    };

    class CSphere : public virtual CBasic, public PhysicsShapes::CSphere
    {
    public:
        void V_Build() override;
    };

    class CBox : public virtual CBasic, public PhysicsShapes::CBox
    {
    public:
        void V_Build() override;
    };
}

//base for rigid, kinematic, ghost

class CBulletWorld;
class CBulletUnit : public virtual CWorldUnit
{
public:
    static btCollisionShape* GetBulletCollision(std::shared_ptr<CCollision> _col);

    virtual btCollisionObject* GetBulletObject() const = 0;
    CBulletWorld* m_addedToWorld = nullptr;
};

//bodies

class CBulletBody : public virtual CBulletUnit
{
public:
    ~CBulletBody();
    btCollisionObject* GetBulletObject() const override;

    std::unique_ptr<btRigidBody> RigidBody;
    std::unique_ptr<btDefaultMotionState> MotionState;
};

class CBulletRigidBody : public virtual CBulletUnit, public CRigidBody, public CBulletBody
{
public:
    CBulletRigidBody(std::shared_ptr<CCollision> _col, const CTransform& _start_transform, float mass);
    btVector3 LocalInertia = btVector3(0, 0, 0);

    void V_SetMass(float mass) override;
    void V_SetCollision(std::shared_ptr<CCollision> _col) override;

    void SetLinearVelocity(const glm::vec3& vel) override;
    void SetAngularVelocity(const glm::vec3& ang_vel) override;

    glm::vec3 GetLinearVelocity() const override;
    glm::vec3 GetAngularVelocity() const override;

    void Activate() override;

    void SetTransform(const CTransform& trans) override;
    CTransform GetInternalTransform() override;
};

class CBulletKinematicBody : public virtual CBulletUnit, public CKinematicBody, public CBulletBody
{
public:
    CBulletKinematicBody(std::shared_ptr<CCollision> _col, const CTransform& _start_transform);
    void V_SetCollision(std::shared_ptr<CCollision> _col) override;

    void SetTransform(const CTransform& trans) override;
    CTransform GetInternalTransform() override;
};

class CBulletGhostBody : public virtual CBulletUnit, public CGhostBody
{
public:
    CBulletGhostBody(std::shared_ptr<CCollision> _col, const CTransform& _start_transform);
    ~CBulletGhostBody();

    std::unique_ptr<btGhostObject> GhostObject;

    btCollisionObject* GetBulletObject() const override;
    void V_SetCollision(std::shared_ptr<CCollision> _col) override;

    void SetTransform(const CTransform& trans) override;
    CTransform GetInternalTransform() override;
};

//sweep test

class BulletConvexSweepCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
    std::vector<CWorldUnit*> ExcludeList;

    BulletConvexSweepCallback
    (
        const btVector3& from,
        const btVector3& to,
        IConvexSweepCallback* userCb
    );

    btScalar addSingleResult(btCollisionWorld::LocalConvexResult& r, bool normalInWorldSpace) override;
    bool needsCollision(btBroadphaseProxy* proxy0) const override;
private:
    IConvexSweepCallback* m_UserCallback = nullptr;
};

class BulletConvexSweepResult : public IConvexSweepResult
{
public:
    BulletConvexSweepResult(const btCollisionWorld::ClosestConvexResultCallback& cb);

    bool HasHit() const override;
    const CSweepHit& GetHit() const override;
private:
    bool m_HasHit = false;
    CSweepHit m_Hit;
};

//manifolds

class BulletContactPoint : public IContactPoint
{
public:
    BulletContactPoint(const btManifoldPoint& pt);

    glm::vec3 GetPositionWorldOnA() const override;
    glm::vec3 GetPositionWorldOnB() const override;
    glm::vec3 GetNormalWorldOnB() const override;
    float GetDistance() const override;
private:
    const btManifoldPoint& m_Pt;
};

class BulletContactManifold : public IContactManifold
{
public:
    BulletContactManifold(btPersistentManifold* m);

    int GetNumContacts() const override;
    std::unique_ptr<IContactPoint> GetContact(int index) const override;

    CWorldUnit* GetUnitA() const override;
    CWorldUnit* GetUnitB() const override;
private:
    btPersistentManifold* m_Manifold;
};

class CBulletWorld;
class BulletGhostOverlapResult : public IGhostOverlapResult
{
public:
    BulletGhostOverlapResult
    (
        const btGhostObject* ghost,
        btBroadphaseInterface* broadphase,
        btDispatcher* dispatcher,
        const CBulletWorld* world
    );

    int GetNumOverlappingUnits() const override;
    CWorldUnit* GetOverlappingUnit(int index) const override;
    std::vector<std::unique_ptr<IContactManifold>> GetContactManifolds(CWorldUnit* other) const override;
private:
    const btGhostObject* m_Ghost;
    btBroadphaseInterface* m_Broadphase;
    btDispatcher* m_Dispatcher;
    const CBulletWorld* m_World = nullptr;
};

//world

class CBulletWorld : public CPhysicsWorld
{
public:
    CBulletWorld();
    ~CBulletWorld();

    std::unique_ptr<btGhostPairCallback> GhostPairCallback;
    std::unique_ptr<btBroadphaseInterface> Broadphase;
    std::unique_ptr<btDefaultCollisionConfiguration> Config;
    std::unique_ptr<btCollisionDispatcher> Dispatcher;
    std::unique_ptr<btSequentialImpulseConstraintSolver> Solver;
    std::unique_ptr<btDiscreteDynamicsWorld> BulletWorld;

    std::unique_ptr<CRigidBody> V_CreateRigidBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform, float mass) override;
    std::unique_ptr<CKinematicBody> V_CreateKinematicBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform) override;
    std::unique_ptr<CGhostBody> V_CreateGhostBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform) override;

    void V_AddRigidBody(CRigidBody* rgBody);
    void V_AddKinematicBody(CKinematicBody* knBody);
    void V_AddGhostBody(CGhostBody* ghBody);

    void V_SetGravity(const glm::vec3& _gravity) override;
    glm::vec3 GetGravity() const override;
    void V_Step(float deltaTime) override;

    std::unique_ptr<IConvexSweepResult> V_ConvexSweep
    (
        std::shared_ptr<CCollision> shape,
        const CTransform& from,
        const CTransform& to,
        IConvexSweepCallback* callback,
        std::vector<CWorldUnit*> excludeList
    ) override;
    
    std::unique_ptr<IGhostOverlapResult> V_GetGhostOverlaps(const CGhostBody* ghost) const override;
};

//engine

class CBulletPhysics : public CPhysicsEngine
{
public:
    CBulletPhysics();

    static btVector3 GetVector(const glm::vec3& vec);
    static glm::vec3 GetVector(const btVector3& vec);
    
    static btQuaternion GetQuat(const glm::quat& qua);
    static glm::quat GetQuat(const btQuaternion& qua);

    static btTransform GetTransform(const CTransformBase& trans);
    static void GetTransform(CTransformBase& trans, const btTransform& btTrans);

    std::unique_ptr<CPhysicsWorld> V_CreateWorld() override;
    DEFINE_PHYSICS_ENGINE();
};
