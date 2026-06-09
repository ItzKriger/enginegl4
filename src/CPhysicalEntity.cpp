#include "CPhysicalEntity.h"
#include "CEngine.h"
#include "CGame.h"
#include "CResourcesManager.h"
#include "CTransformable.h"

void CPhysicalEntity::V_Init()
{
    if(!WorldUnit)
    {
        auto opt_worldId = GetEntity()->GetInitParam<int>("phys_worldid");
        auto opt_collision = GetEntity()->GetInitParam<std::string>("phys_collision");
        auto opt_type = GetEntity()->GetInitParam<std::string>("phys_type");

        if(!opt_worldId.has_value() || !opt_collision.has_value() || !opt_type.has_value()) { return; }

        auto opt_mass = GetEntity()->GetInitParam<float>("phys_mass");

        CTransform start_transform;
        float mass = 100.0f;
        if(opt_mass.has_value()) { mass = opt_mass.value(); }

        auto opt_start_position = GetEntity()->GetInitParam<glm::vec3>("phys_position");
        auto opt_start_quat = GetEntity()->GetInitParam<glm::quat>("phys_rotation");
        auto opt_start_euler = GetEntity()->GetInitParam<CAngles>("phys_euler");

        if(opt_start_position.has_value())
        {
            start_transform.SetPosition(opt_start_position.value());
        }
        else
        {
            start_transform.SetPosition(glm::vec3(0, 0, 0));
        }

        if(opt_start_quat.has_value())
        {
            start_transform.SetRotation(opt_start_quat.value());
        }

        if(opt_start_euler.has_value())
        {
            start_transform.GetEulerRotation().SetRotation(opt_start_euler.value());
        }

        if(!opt_start_quat.has_value() && !opt_start_euler.has_value())
        {
            start_transform.SetRotation(glm::identity<glm::quat>());
        }

        start_transform.SetScale(glm::vec3(1, 1, 1));

        auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
        auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
        auto world = game->PhysicsEngine->GetWorld(opt_worldId.value());

        if(!world)
        {
            Log::ErrInstance() << "No world for physical entity\n";
            return;
        }

        auto _col = resman->GetOrCreate("collision", opt_collision.value());

        if(!_col)
        {
            Log::ErrInstance() << "No collision for physical entity\n";
            return;
        }

        _col->Wait();
        auto col = std::dynamic_pointer_cast<CCollision>(_col);

        if(!_col)
        {
            Log::ErrInstance() << "No collision for physical entity (2)\n";
            return;
        }

        std::string type = opt_type.value();
        if(type == "rigid")
        {
            WorldUnit = world->CreateRigidBody(col, start_transform, mass); //TODO remove on destructor
        }
        else if(type == "kinematic")
        {
            WorldUnit = world->CreateKinematicBody(col, start_transform); //TODO remove on destructor
        }
        else if(type == "ghost")
        {
            WorldUnit = world->CreateGhostBody(col, start_transform); //TODO remove on destructor
        }
        else
        {
            Log::ErrInstance() << "Unknown physical type\n";
        }
        Log::Instance() << "Created local physical with mass of " << mass << Log::Endl;
    }

    WorldUnit->GetTransformCallback() += [this](CTransformBase::CMeasurePack oldpack, CTransformBase* _this) -> void
    {
        auto transformable = GetEntity()->Components.GetComponentTyped<CTransformable>();
        if(transformable && WorldUnit->IsUpdatingInternalTransform())
        {
            transformable->Transform.SetPRS(_this->GetPRS());
        }
    };

    auto entity = GetEntity();
    auto transformable = entity->Components.GetComponentTyped<CTransformable>();
    if(!transformable)
    {
        entity->Components.CreateComponent<CTransformable>();
        transformable = entity->Components.GetComponentTyped<CTransformable>();
    }

    transformable->Transform.OnTransformChanged += [this](CTransformBase::CMeasurePack oldpack, CTransformBase* _this) -> void
    {
        auto transformable = GetEntity()->Components.GetComponentTyped<CTransformable>();
        if(transformable && !WorldUnit->IsUpdatingInternalTransform())
        {
            WorldUnit->SetTransform(transformable->Transform);
            auto rigidBody = dynamic_cast<CRigidBody*>(WorldUnit);
            if(rigidBody)
            {
                rigidBody->Activate();
            }
        }
    };
}

#pragma pack(push, 1)
struct PhysicalFlags
{
    unsigned int Type : 2;
    unsigned int __unused : 6;
};
#pragma pack(pop)

void CPhysicalEntity::FullPack(CBufferWrapper& packet)
{
    auto colName = WorldUnit->GetCollision()->Name;
    auto rigid = dynamic_cast<CRigidBody*>(WorldUnit);

    float mass = 0.0f;
    if(rigid)
    {
        mass = rigid->GetMass();
    }

    auto trans = WorldUnit->GetTransform();

    auto rigidBody = dynamic_cast<CRigidBody*>(WorldUnit);
    auto kinematicBody = dynamic_cast<CKinematicBody*>(WorldUnit);
    auto ghostBody = dynamic_cast<CGhostBody*>(WorldUnit);

    PhysicalFlags flags;
    if(rigidBody) { flags.Type = 0; }
    else if(kinematicBody) { flags.Type = 1; }
    else if(ghostBody) { flags.Type = 2; }

    Log::Instance() << "Write Physical Flag: " << flags.Type << Log::Endl;

    packet.WriteLenString<std::uint8_t, std::string>(colName);
    packet.Write<float>(mass);
    packet.Write<PhysicalFlags>(flags);
    packet.Write<glm::vec3>(trans.GetPosition());
    packet.Write<glm::quat>(trans.GetRotation());
}

void CPhysicalEntity::FullUnpack(CBufferWrapper& packet)
{
    auto colName = packet.ReadLenString<std::uint8_t, std::string>();
    auto mass = packet.Read<float>();
    auto flags = packet.Read<PhysicalFlags>();
    auto pos = packet.Read<glm::vec3>();
    auto rot = packet.Read<glm::quat>();

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto world = game->PhysicsEngine->GetWorld(0); //TODO not zero

    CTransform start_transform;
    start_transform.SetPRS
    (
        {
            pos,
            rot,
            glm::vec3(1, 1, 1)
        }
    );

    if(!world)
    {
        Log::ErrInstance() << "No world for physical entity\n";
        return;
    }

    auto _col = resman->GetOrCreate("collision", colName + ".ecol");

    if(!_col)
    {
        Log::ErrInstance() << "No collision for physical entity\n";
        return;
    }

    auto col = std::dynamic_pointer_cast<CCollision>(_col);
    //col->Wait();

    if(!col)
    {
        Log::ErrInstance() << "No collision for physical entity (2)\n";
        return;
    }

    Log::Instance() << "Read Physical Flag: " << flags.Type << Log::Endl;

    if(flags.Type == 0)
    {
        WorldUnit = world->CreateRigidBody(col, start_transform, mass); //TODO remove on destructor
    }
    else if(flags.Type == 1)
    {
        WorldUnit = world->CreateKinematicBody(col, start_transform); //TODO remove on destructor
    }
    else if(flags.Type == 2)
    {
        WorldUnit = world->CreateGhostBody(col, start_transform); //TODO remove on destructor
    }
    else
    {
        Log::ErrInstance() << "Unknown physical type\n";
    }

    //WorldUnit = world->CreateRigidBody(col, start_transform, mass); //TODO remove on destructor
}

LINK_SOL_USERTYPE(CPhysicalEntity);
LINK_ENTITY_COMPONENT_TO_CLASS(CPhysicalEntity, physical);