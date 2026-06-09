#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "nlohmann/json.hpp"

#include "CResource.h"
#include "CObjectFactory.h"
#include "CTransform.h"
#include "CScriptObject.h"

#define LINK_PHYSICS_ENGINE_TO_CLASS(_class, name) std::string _class::GetType() const { return #name; }; CFactoryInitter<_class, CPhysicsEngine> __entity_initter_ ## name = CFactoryInitter<_class, CPhysicsEngine>(#name, std::function<void(CFactoryInitter<_class, CPhysicsEngine>*)>([](CFactoryInitter<_class, CPhysicsEngine>* tter) -> void { tter->m_factory = &CEngine::GetInstance()->PhysicsEnginesFactory; }));
#define DEFINE_PHYSICS_ENGINE() std::string GetType() const override

class CTriangleMesh : public CResource
{
public:
    struct CTriangle
    {
        std::uint16_t Material;
        glm::u64vec3 Index;
    };

    void V_SetupLoadPipeline(CLoadPipeline& pipeline) override;

    std::vector<glm::vec3> Vertices;
    std::vector<CTriangle> Triangles;

    DEFINE_RESOURCE();
};

namespace PhysicsShapes
{
    class CBasic
    {
    public:
        virtual ~CBasic() = default;

        virtual std::string GetType() const;

        void Load(const nlohmann::json& _json);
        virtual void V_Load(const nlohmann::json& _json);

        void Build();
        virtual void V_BaseBuild();
        virtual void V_Build();

        virtual float GetMargin() const;
        virtual void SetMargin(float _margin);
    };

    class CTriMesh : public virtual CBasic
    {
    public:
        std::vector<glm::vec3> Vertices;
        std::vector<glm::uvec3> Triangles;

        void LoadTrimesh(const std::filesystem::path& path);

        void V_Load(const nlohmann::json& _json) override;
        void Merge(float threshold = 0.001f);

        std::string GetType() const override;
    };

    class CConvex : public CTriMesh
    {
    public:
        void BuildConvex();
        void V_BaseBuild() override;
        void V_Load(const nlohmann::json& _json) override;

        std::string GetType() const override;

        bool NeedVHACD = true;
    };

    class CCompound : public virtual CBasic
    {
    public:
        struct CChild
        {
            CTransform Transform;
            std::shared_ptr<CBasic> Shape;
        };

        std::string GetType() const override;

        void V_Load(const nlohmann::json& _json) override;
        void V_BaseBuild() override;
        std::vector<CChild> Childs;
    };

    class CCapsule : public virtual CBasic
    {
    public:
        void V_Load(const nlohmann::json& _json) override;
        std::string GetType() const override;

        float Radius = 2.0f;
        float Height = 4.0f;
    };

    class CSphere : public virtual CBasic
    {
    public:
        void V_Load(const nlohmann::json& _json) override;
        std::string GetType() const override;

        float Radius = 1.0f;
    };

    class CBox : public virtual CBasic
    {
    public:
        void V_Load(const nlohmann::json& _json) override;
        std::string GetType() const override;

        glm::vec3 HalfExtents = glm::vec3(1.0f, 1.0f, 1.0f);
    };
}

class CCollision : public CResource
{
public:
    static std::unique_ptr<PhysicsShapes::CBasic> ParseShape(const nlohmann::json& _json, CTransformBase* _trans = nullptr); //TODO parent transforms
    static std::unique_ptr<PhysicsShapes::CBasic> BuildConvexCompound(const nlohmann::json& _json); //TODO parent transforms
    static std::unique_ptr<PhysicsShapes::CBasic> LoadNativeConvex(const nlohmann::json& _json); //TODO parent transforms
    static void CompileNativeConvex(const std::filesystem::path& _mesh_path, std::vector<std::filesystem::path> _configs_paths = {}, bool vhacd = false);

    void V_SetupLoadPipeline(CLoadPipeline& pipeline) override;
    std::unique_ptr<PhysicsShapes::CBasic> Shape;
    DEFINE_RESOURCE();
};

//base for rigid, kinematic, ghost

enum class EUnitType
{
    Rigid,
    Kinematic,
    Ghost
};

class CPhysicsWorld;
class CWorldUnit : public CScriptObject
{
public:
    virtual ~CWorldUnit() = default;

    void OverridePersistent(CPhysicsWorld* _world);

    void SetCollision(std::shared_ptr<CCollision> _col);
    virtual void V_SetCollision(std::shared_ptr<CCollision> _col);

    std::shared_ptr<CCollision> GetCollision() const;
    virtual void SetTransform(const CTransform& trans);
    virtual void UpdateInternalTransform();

    virtual CTransform GetInternalTransform();
    CTransform GetTransform() const;

    CCallbackHandler<void, CTransformBase::CMeasurePack, CTransformBase*>& GetTransformCallback();
    CPhysicsWorld* GetWorld() const;

    bool IsUpdatingInternalTransform() const;
    void m_setCollisionPtr(std::shared_ptr<CCollision> _col); //TODO HACK

    bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;
    virtual EUnitType GetType() const = 0;
private:
    bool m_updatingInternalTransform = false;
    CPhysicsWorld* m_World = nullptr;

    CTransform Transform;
    std::shared_ptr<CCollision> m_Collision;
};

//bodies

class CRigidBody : public virtual CWorldUnit
{
public:
    void SetMass(float mass);
    virtual void V_SetMass(float mass);

    virtual void SetLinearVelocity(const glm::vec3& vel);
    virtual void SetAngularVelocity(const glm::vec3& ang_vel);

    virtual glm::vec3 GetLinearVelocity() const;
    virtual glm::vec3 GetAngularVelocity() const;

    virtual void Activate();

    float GetMass() const;
    EUnitType GetType() const override;
private:
    float m_Mass = 100.0f;
};

class CKinematicBody : public virtual CWorldUnit
{
public:
    EUnitType GetType() const override;
};

class CGhostBody : public virtual CWorldUnit
{
public:
    EUnitType GetType() const override;
};

//sweep test

struct CSweepHit
{
    CWorldUnit* HitUnit = nullptr;
    glm::vec3 HitPointWorld;
    glm::vec3 HitNormalWorld;
    float HitFraction = 1.0f;
};

class IConvexSweepResult
{
public:
    virtual ~IConvexSweepResult() = default;

    virtual bool HasHit() const = 0;
    virtual const CSweepHit& GetHit() const = 0;
};

class IConvexSweepCallback
{
public:
    virtual ~IConvexSweepCallback() = default;
    virtual bool OnHit(const CSweepHit& hit) = 0;
};

//manifolds

class IContactPoint
{
public:
    virtual ~IContactPoint() = default;

    virtual glm::vec3 GetPositionWorldOnA() const = 0;
    virtual glm::vec3 GetPositionWorldOnB() const = 0;
    virtual glm::vec3 GetNormalWorldOnB() const = 0;
    virtual float GetDistance() const = 0;
};

class IContactManifold
{
public:
    virtual ~IContactManifold() = default;

    virtual int GetNumContacts() const = 0;
    virtual std::unique_ptr<IContactPoint> GetContact(int index) const = 0;
    virtual CWorldUnit* GetUnitA() const = 0;
    virtual CWorldUnit* GetUnitB() const = 0;
};

class IGhostOverlapResult
{
public:
    virtual ~IGhostOverlapResult() = default;

    virtual int GetNumOverlappingUnits() const = 0;
    virtual CWorldUnit* GetOverlappingUnit(int index) const = 0;

    virtual std::vector<std::unique_ptr<IContactManifold>> GetContactManifolds(CWorldUnit* other) const = 0;
};

//world

class CPhysicsEngine;
class CPhysicsWorld : public CScriptObject
{
public:
    virtual ~CPhysicsWorld() = default;

    void OverridePersistent(CPhysicsEngine* _engine);
    void Step(float deltaTime);

    virtual void V_Step(float deltaTime);

    void SetGravity(const glm::vec3& _gravity);
    
    virtual glm::vec3 GetGravity() const;
    virtual void V_SetGravity(const glm::vec3& _gravity);

    virtual std::unique_ptr<CRigidBody> V_CreateRigidBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform, float mass);
    virtual std::unique_ptr<CKinematicBody> V_CreateKinematicBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform);
    virtual std::unique_ptr<CGhostBody> V_CreateGhostBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform);

    virtual void V_AddRigidBody(CRigidBody* rgBody);
    virtual void V_AddKinematicBody(CKinematicBody* knBody);
    virtual void V_AddGhostBody(CGhostBody* ghBody);

    CRigidBody* CreateRigidBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform, float mass);
    CKinematicBody* CreateKinematicBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform);
    CGhostBody* CreateGhostBody(std::shared_ptr<CCollision> _collision, const CTransform& startTransform);

    virtual std::unique_ptr<IConvexSweepResult> V_ConvexSweep
    (
        std::shared_ptr<CCollision> shape,
        const CTransform& from,
        const CTransform& to,
        IConvexSweepCallback* callback = nullptr,
        std::vector<CWorldUnit*> excludeList = {}
    ) = 0;
    virtual std::unique_ptr<IGhostOverlapResult> V_GetGhostOverlaps(const CGhostBody* ghost) const = 0;
    bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    std::vector<std::unique_ptr<CWorldUnit>> Units;
private:
    CPhysicsEngine* m_linkedEngine = nullptr;
};

//engine

class CPhysicsEngine : public CScriptObject
{
public:
    CPhysicsEngine();
    virtual ~CPhysicsEngine() = default;
    virtual std::string GetType() const;

    CPhysicsWorld* CreateWorld();
    virtual std::unique_ptr<CPhysicsWorld> V_CreateWorld();

    CPhysicsWorld* GetWorld(int id);
    void Trace(std::function<void(CPhysicsWorld*)> func);

    bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    CObjectFactory<PhysicsShapes::CBasic, std::string> ShapesFactory;
private:
    std::vector<std::unique_ptr<CPhysicsWorld>> m_Worlds;
};
