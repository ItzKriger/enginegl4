#include "CTypesPhysics.h"

#include "CScriptingEngine.h"
#include "CTypesRegistering.h"
#include "CPhysicsEngine.h"

void CScriptingEngine::RegisterPhysicsTypes()
{
    State->new_enum<EUnitType>
    (
        "EUnitType",
        {
            { "Rigid", EUnitType::Rigid },
            { "Kinematic", EUnitType::Kinematic },
            { "Ghost", EUnitType::Ghost }
        }
    );

    State->new_usertype<PhysicsShapes::CBasic>
    (
        "PhysicsShapes_CBasic",
        sol::no_constructor,
        "GetType", &PhysicsShapes::CBasic::GetType,
        "Load", &PhysicsShapes::CBasic::Load,
        "Build", &PhysicsShapes::CBasic::Build,
        "GetMargin", &PhysicsShapes::CBasic::GetMargin,
        "SetMargin", &PhysicsShapes::CBasic::SetMargin
    );

    State->new_usertype<CCollision>
    (
        "CCollision",
        sol::no_constructor,
        SOL_RESOURCE_BASE,
        "Shape", sol::property([](CCollision& col) -> PhysicsShapes::CBasic* { return col.Shape.get(); })
    );

    State->new_usertype<CWorldUnit>
    (
        "CWorldUnit",
        sol::no_constructor,
        "SetCollision", &CWorldUnit::SetCollision,
        "GetCollision", &CWorldUnit::GetCollision,
        "SetTransform", &CWorldUnit::SetTransform,
        "UpdateInternalTransform", &CWorldUnit::UpdateInternalTransform,
        "GetInternalTransform", &CWorldUnit::GetInternalTransform,
        "GetTransform", &CWorldUnit::GetTransform,
        "GetTransformCallback", [](CWorldUnit& unit, sol::this_state ts) { return unit.GetTransformCallback().GetScriptTable(ts); },
        "GetWorld", &CWorldUnit::GetWorld,
        "World", sol::property([](CWorldUnit& world) { return world.GetWorld(); }),
        "IsUpdatingInternalTransform", &CWorldUnit::IsUpdatingInternalTransform,
        "GetType", &CWorldUnit::GetType
    );

    State->new_usertype<CRigidBody>
    (
        "CRigidBody",
        sol::no_constructor,
        sol::base_classes, sol::bases<CWorldUnit>(),
        "SetMass", &CRigidBody::SetMass,
        "GetMass", &CRigidBody::GetMass,
        "SetLinearVelocity", &CRigidBody::SetLinearVelocity,
        "SetAngularVelocity", &CRigidBody::SetAngularVelocity,
        "GetLinearVelocity", &CRigidBody::GetLinearVelocity,
        "GetAngularVelocity", &CRigidBody::GetAngularVelocity,
        "Activate", &CRigidBody::Activate
    );

    State->new_usertype<CKinematicBody>
    (
        "CKinematicBody",
        sol::no_constructor,
        sol::base_classes, sol::bases<CWorldUnit>()
    );

    State->new_usertype<CGhostBody>
    (
        "CGhostBody",
        sol::no_constructor,
        sol::base_classes, sol::bases<CWorldUnit>()
    );

    State->new_usertype<CSweepHit>
    (
        "CSweepHit",
        sol::no_constructor,
        "HitUnit", &CSweepHit::HitUnit,
        "HitPointWorld", &CSweepHit::HitPointWorld,
        "HitNormalWorld", &CSweepHit::HitNormalWorld,
        "HitFraction", &CSweepHit::HitFraction
    );

    State->new_usertype<IConvexSweepResult>
    (
        "IConvexSweepResult",
        sol::no_constructor,
        "HasHit", &IConvexSweepResult::HasHit,
        "GetHit", &IConvexSweepResult::GetHit
    );

    State->new_usertype<IConvexSweepCallback>
    (
        "IConvexSweepCallback",
        sol::no_constructor,
        "OnHit", &IConvexSweepCallback::OnHit
    );

    State->new_usertype<IContactPoint>
    (
        "IContactPoint",
        sol::no_constructor,
        "GetPositionWorldOnA", &IContactPoint::GetPositionWorldOnA,
        "GetPositionWorldOnB", &IContactPoint::GetPositionWorldOnB,
        "GetNormalWorldOnB", &IContactPoint::GetNormalWorldOnB,
        "GetDistance", &IContactPoint::GetDistance
    );

    State->new_usertype<IContactManifold>
    (
        "IContactManifold",
        sol::no_constructor,
        "GetNumContacts", &IContactManifold::GetNumContacts,
        "GetUnitA", &IContactManifold::GetUnitA,
        "GetUnitB", &IContactManifold::GetUnitB,
        "UnitA", sol::property([](IContactManifold& mnf) -> CWorldUnit* { return mnf.GetUnitA(); }),
        "UnitB", sol::property([](IContactManifold& mnf) -> CWorldUnit* { return mnf.GetUnitB(); }),
        "GetContact", [](const IContactManifold& mnf, int index) -> IContactPoint*
        {
            return mnf.GetContact(index).get();
        }
    );

    //std::vector<std::unique_ptr<IContactManifold>> GetContactManifolds(CWorldUnit* other)

    State->new_usertype<IGhostOverlapResult>
    (
        "IGhostOverlapResult",
        sol::no_constructor,
        "GetNumOverlappingUnits", &IGhostOverlapResult::GetNumOverlappingUnits,
        "GetOverlappingUnit", &IGhostOverlapResult::GetOverlappingUnit,
        //TODO might just reinterpret_cast since unique_ptr is just a single ptr in memory?
        "GetContactManifolds", [](const IGhostOverlapResult& overlap, CWorldUnit* other) -> std::vector<IContactManifold*> 
        {
            auto _ret = overlap.GetContactManifolds(other);

            std::vector<IContactManifold*> ret;
            ret.reserve(_ret.size());

            for(const auto& c : _ret)
            {
                ret.push_back(c.get());
            }
            return ret;
        }
    );

    State->new_usertype<CPhysicsEngine>
    (
        "CPhysicsEngine",
        sol::no_constructor,
        "GetType", &CPhysicsEngine::GetType,
        "CreateWorld", &CPhysicsEngine::CreateWorld,
        "GetWorld", [](CPhysicsEngine& eng, sol::object _id) -> CPhysicsWorld*
        {
            int id = 0;
            if(_id.valid() && _id.is<int>()) { id = _id.as<int>(); }

            return eng.GetWorld(id);
        },
        "Trace", [](CPhysicsEngine& eng, sol::function func)
        {
            eng.Trace([&func](CPhysicsWorld* world) { func(world); });
        }
    );

    State->new_usertype<CPhysicsWorld>
    (
        "CPhysicsWorld",
        sol::no_constructor,
        "Step", &CPhysicsWorld::Step,
        "SetGravity", &CPhysicsWorld::SetGravity,
        "GetGravity", &CPhysicsWorld::GetGravity,
        "CreateRigidBody", &CPhysicsWorld::CreateRigidBody,
        "CreateKinematicBody", &CPhysicsWorld::CreateKinematicBody,
        "CreateGhostBody", &CPhysicsWorld::CreateGhostBody,
        "GetGhostOverlaps",
        sol::overload
        (
            [](CPhysicsWorld& world, const CGhostBody* ghost) -> IGhostOverlapResult*
            {
                return world.V_GetGhostOverlaps(ghost).get();
            },
            [](CPhysicsWorld& world, const CWorldUnit* _ghost) -> IGhostOverlapResult*
            {
                auto* ghost = dynamic_cast<const CGhostBody*>(_ghost);

                if(!ghost) { return nullptr; }
                return world.V_GetGhostOverlaps(ghost).get();
            }
        ),
        "ConvexSweep",
        sol::overload
        (
            []
            (
                CPhysicsWorld& world,
                std::shared_ptr<CCollision> shape,
                const CTransform& from,
                const CTransform& to
            )
            {
                return world.V_ConvexSweep(shape, from, to).get();
            },
            []
            (
                CPhysicsWorld& world,
                std::shared_ptr<CCollision> shape,
                const CTransform& from,
                const CTransform& to,
                std::vector<CWorldUnit*> excludeList
            )
            {
                return world.V_ConvexSweep(shape, from, to, nullptr, excludeList).get();
            },
            []
            (
                CPhysicsWorld& world,
                std::shared_ptr<CCollision> shape,
                const CTransform& from,
                const CTransform& to,
                std::vector<CWorldUnit*> excludeList,
                IConvexSweepCallback* callback
            )
            {
                return world.V_ConvexSweep(shape, from, to, callback, excludeList).get();
            }
        )
    );

    
}
