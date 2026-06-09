#include "CTransformable.h"
#include "CEngine.h"
#include "CServer.h"
#include "CBitFlags8.h"
#include "CGame.h"
#include "CConVarManager.h"
#include "CClient.h"
#include "U_General.h"
#include "CPhysicalEntity.h"

#include "glm/glm.hpp"
#include "glm/gtx/compatibility.hpp"

void CTransformable::V_Init()
{
    Transform.OnTransformChanged += [this](CTransformBase::CMeasurePack msp, CTransformBase* _this)
    {
        if(!NetSync) { return; }

        auto server = CEngine::GetInstance()->Components.GetComponentTyped<CServer>();
        if(!server) { return; }

        auto msg = std::make_shared<CNetTransformChanged>();

        msg->Entity = CEntityHandle(GetEntity());
        msg->ChangePosition = msp.Position != _this->GetPosition();
        msg->ChangeRotation = msp.Rotation != _this->GetRotation();
        msg->ChangeScale = msp.Scale != _this->GetScale();
        msg->RemoteChanged = CEngine::GetInstance()->Time.GetCurrent(); //TODO time differences?

        msg->Pack = _this->GetPRS();

        //TODO also take in the count stream distance and space culling
        //TODO lerp
        server->SendUnreliableMessageToAll(msg); //TODO should be reliable
    };
}

void CNetTransformChanged::V_Write(CBufferWrapper& wrapper)
{
    CBitFlags8 flags;

    flags.set(0, ChangePosition);
    flags.set(1, ChangeRotation);
    flags.set(2, ChangeScale);

    //auto dur = std::chrono::duration_cast<std::chrono::duration<double>>(RemoteChanged.time_since_epoch());

    wrapper.Write<std::uint16_t>(Entity.Get()->GetUUID());
    wrapper.Write<CTimePoint>(RemoteChanged); //TODO don't write/read it raw. Do the serialization
    wrapper.Write(flags);

    if(ChangePosition) { wrapper.Write(Pack.Position); }
    if(ChangeRotation) { wrapper.Write(Pack.Rotation); }
    if(ChangeScale) { wrapper.Write(Pack.Scale); }
}

void CNetTransformChanged::V_Read(CBufferWrapper& wrapper)
{
    auto client = CEngine::GetInstance()->Components.GetComponentTyped<CClient>();
    if(!client) { return; }

    auto _entid = wrapper.Read<std::uint16_t>();
    RemoteChanged = wrapper.Read<CTimePoint>();
    auto flags = wrapper.Read<CBitFlags8>();

    std::uint16_t entid = client->ServerEntityMap[_entid];

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto world = game->Worlds.GetWorld();

    auto it = std::find_if(world->Entities.Items.begin(), world->Entities.Items.end(), [entid](auto& kv) -> bool
    {
        return kv.second->GetUUID() == entid;
    });

    if(it == world->Entities.Items.end())
    {
        Log::ErrInstance() << "No such entity\n"; //TODO disconnect -- fatal!
    }

    ChangePosition = flags[0];
    ChangeRotation = flags[1];
    ChangeScale = flags[2];

    if(ChangePosition) { wrapper.Read(Pack.Position); }
    if(ChangeRotation) { wrapper.Read(Pack.Rotation); }
    if(ChangeScale) { wrapper.Read(Pack.Scale); }

    if(it != world->Entities.Items.end()) //TODO HACK
    {
        Entity = CEntityHandle(it->second.get());
    }
}

void CNetTransformChanged::Process()
{
    if(!Entity.IsValid()) { return; } //TODO HACK

    bool Interpolation = true;
    std::uint8_t InterpFrames = 1;

    COMPONENT_CALL_GET(Interpolation, CConVarManager, GetConVarValue<bool>("cl.interp", true));
    COMPONENT_CALL_GET(InterpFrames, CConVarManager, GetConVarValue<std::uint8_t>("cl.interp.frames", 1));

    auto entity = Entity.Get();
    if(!entity) { return; }

    auto transformable = entity->Components.GetComponentTyped<CTransformable>();
    if(!transformable) { return; }

    transformable->m_remoteTransformable = true;

    if(!ChangePosition) { Pack.Position = transformable->Transform.GetPosition(); }
    if(!ChangeRotation) { Pack.Rotation = transformable->Transform.GetRotation(); }
    if(!ChangeScale) { Pack.Scale = transformable->Transform.GetScale(); }

    if(!Interpolation)
    {
        transformable->Transform.SetPRS(Pack);
    }
    else
    {
        auto clientNow = CEngine::GetInstance()->Time.GetCurrent();
        static std::chrono::duration<float> serverToClientOffset = std::chrono::duration<float>(0);

        auto measuredOffset = clientNow - NetTime;

        serverToClientOffset =
            serverToClientOffset * 0.9f +
            measuredOffset * 0.1f;

        CTimePoint clientDomainTime = TIME_EXPR(NetTime + serverToClientOffset);
        transformable->InterpQueue.push_back({ RemoteChanged, Pack });

        //Log::Instance() << "RemoteChanged: " << RemoteChanged.time_since_epoch().count() << "; Client time: " << CEngine::GetInstance()->Time.GetCurrent().time_since_epoch() << Log::Endl;
    }
}

void CTransformable::V_Update()
{
    //dump.trans.timestep
    int timesPerSecond = 100;
    size_t maxmem = 1000000;

    COMPONENT_CALL_GET(timesPerSecond, CConVarManager, GetConVarValue<int>("dump.trans.timestep", 100));
    COMPONENT_CALL_GET(maxmem, CConVarManager, GetConVarValue<std::uint64_t>("dump.trans.maxmem", 1000000));

    constexpr size_t singleSize = sizeof(CTimePoint) + sizeof(CTransformBase::CMeasurePack);
    constexpr size_t totalSize = singleSize * 1000000;

    auto now = CEngine::GetInstance()->Time.GetCurrent();
    if(now - LastDump >= std::chrono::duration<float>(1.0f / static_cast<float>(timesPerSecond)))
    {
        DumpQueue.push_back({ now, Transform.GetPRS() });
        LastDump = now;
    }

    if(DumpQueue.size() >= maxmem)
    {
        DumpQueue.erase(DumpQueue.begin()); //inefficient limit
    }

    //Log::Instance() << "CTransformable::V_Update()\n";
    if(!m_remoteTransformable) { return; }

    bool Interpolation = true;
    std::uint8_t InterpFrames = 1;

    COMPONENT_CALL_GET(Interpolation, CConVarManager, GetConVarValue<bool>("cl.interp", true));
    COMPONENT_CALL_GET(InterpFrames, CConVarManager, GetConVarValue<std::uint8_t>("cl.interp.frames", 1));

    if(!Interpolation) { return; }
    //if(InterpQueue.size() < InterpFrames) { return; }

    //sv_rate = 1.0 / 50.0 = 0.02
    //pack_1 = 1.02
    //pack_2 = 1.04
    //cl_time = 1.045
    //pack_1_pred + (frames_i * sv_rate) = 1.02 + (1 * 0.02) = 1.02 + 0.02 = 1.04
    //pack_2_pred + (frames_i * sv_rate) = 1.02 + (2 * 0.02) = 1.02 + 0.04 = 1.06

    //pack_1 = 1.02
    //cl_time = 1.023
    //pack_1_pred = 1.02 + (1 * 0.02) = 1.04

    float sv_rate = 33.0f;
    COMPONENT_CALL_GET(sv_rate, CClient, GetServerRate());

    float sv_delay = 1.0f / sv_rate;

    float MaxDifference = (InterpFrames + 2) * sv_delay; //TODO not hardcoded threshold
    CTimePoint curTime = CEngine::GetInstance()->Time.GetCurrent();

    int lerpIndex = -1;
    float minDiff = 9999.0f;

    auto lerpPoint = CEngine::GetInstance()->Time.GetCurrent() - std::chrono::duration<float>(sv_delay * InterpFrames);

    for(int i = 0; i < InterpQueue.size(); i++)
    {
        auto& pair = InterpQueue.at(i);

        if(pair.first > curTime) { continue; }

        if(pair.first <= lerpPoint && 
            (
                (i == InterpQueue.size() - 1) ||
                (InterpQueue.at(i + 1).first > lerpPoint)
            )
        )
        {
            lerpIndex = i;
            break;
        }
    }

    if(lerpIndex != -1)
    {
        //cl_time = 1.023

        auto& lerp_unit = InterpQueue.at(lerpIndex);
        auto t_pack_pred_time = lerp_unit.first + std::chrono::duration<float>(sv_delay); //1.04
        auto t_pack_plus_client_diff = CEngine::GetInstance()->Time.GetCurrent() - lerp_unit.first; //0.003
        
        float client_diff = std::chrono::duration<float>(t_pack_plus_client_diff).count(); //0.003
        float max_diff = sv_delay; //0.02

        //0.003 / 0.02 = 0.15
        float lerpFactor = client_diff / max_diff;
        lerpFactor = glm::clamp(lerpFactor, 0.0f, 1.0f);

        CTransformBase::CMeasurePack lerpedPack;

        lerpedPack.Position = glm::lerp(Transform.GetPosition(), lerp_unit.second.Position, lerpFactor);
        lerpedPack.Rotation = glm::slerp(Transform.GetRotation(), lerp_unit.second.Rotation, lerpFactor);
        lerpedPack.Scale = glm::lerp(Transform.GetScale(), lerp_unit.second.Scale, lerpFactor);

        Transform.SetPRS(lerpedPack);
    }
    else if(!InterpQueue.empty())
    {
        if(CEngine::GetInstance()->Time.GetCurrent() >= InterpQueue.front().first)
        {
            Transform.SetPRS(InterpQueue.back().second);
        }
    }

    std::erase_if(InterpQueue, [&curTime, MaxDifference](auto& pair) -> bool
    {
        return (curTime - pair.first) >= std::chrono::duration<float>(MaxDifference);
    });

    //Log::Instance() << "CTransformable::Interp()\n";
}

void CTransformable::FullPack(CBufferWrapper& packet)
{
    packet.Write(Transform.GetPRS());
}

void CTransformable::FullUnpack(CBufferWrapper& packet)
{
    Transform.SetPRS(packet.Read<CTransformBase::CMeasurePack>());
}

LINK_SOL_USERTYPE(CTransformable);
LINK_NET_MESSAGE_TO_CLASS(CNetTransformChanged, transform);
LINK_ENTITY_COMPONENT_TO_CLASS(CTransformable, transformable);