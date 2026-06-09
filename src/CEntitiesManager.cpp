#include "CEntitiesManager.h"
#include "CEngine.h"
#include "CGame.h"
#include "CServer.h"
#include "CClient.h"

#include <limits>
#include <algorithm>

CEntitiesManager::CEntitiesManager(worldid_t worldid) : WorldID(worldid) {}

bool CEntitiesManager::m_isValidWorld(worldid_t worldid)
{
    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto world = game->Worlds.GetWorld(GetWorldID());

    return world;
}

worldid_t CEntitiesManager::GetWorldID() const
{
    return WorldID;
}

size_t CEntitiesManager::Count() const
{
    return Items.size();
}

CEntityHandle CEntitiesManager::CreateEntity(const std::string& type, bool sync)
{
    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto world = game->Worlds.GetWorld(GetWorldID());

    if(!world) { return CEntityHandle(); }

    std::unique_ptr<CEntity> ent = CEngine::GetInstance()->EntitiesFactory.create<std::unique_ptr<CEntity>>(type);
    if(!ent) { return CEntityHandle(); }

    return m_addEntity(std::move(ent), sync);
}

CEntityHandle CEntitiesManager::m_addEntity(std::unique_ptr<CEntity>&& ent, bool sync)
{
    entityid_t id = GetNextFreeID();
    Items.insert({ id, std::move(ent) });

    Items[id]->OverridePersistent(random::random_static::get<serial_t>(), id, GetWorldID());
    Items[id]->SetNetSync(sync, false);

    for(auto& kv : initParams)
    {
        Items[id]->AddInitParam(kv.first, kv.second);
        Log::Instance() << "Passing " << kv.first << " to entity " << Items[id]->GetType() << Log::Endl;
    }
    ClearNextEntityParams();

    Items[id]->Init();

    auto server = CEngine::GetInstance()->Components.GetComponentTyped<CServer>();
    if(server && sync)
    {
        auto msg = std::make_shared<CNetEntityStart>();

        msg->Entity = CEntityHandle(Items[id].get());
        server->SendReliableMessageToAll(msg); //TODO should be reliable
    }

    Items[id]->PostInit();
    return CEntityHandle(Items[id].get());
}

void CEntitiesManager::DeleteEntity(const CEntityHandle& handle)
{
    auto it = std::find_if(Items.begin(), Items.end(), [&handle](auto& kv)
    {
        return handle == kv.second.get();
    });

    if(it != Items.end())
    {
        auto server = CEngine::GetInstance()->Components.GetComponentTyped<CServer>();
        if(server && handle.Get()->IsNetSync())
        {
            auto msg = std::make_shared<CNetEntityStop>();

            msg->Entity = CEntityHandle(it->second.get());
            server->SendReliableMessageToAll(msg); //TODO should be reliable
        }
        Items.erase(it);
    }
}

void CEntitiesManager::Trace(const std::function<void(CEntity* ent)>& func)
{
    for(auto& kv : Items)
    {
        func(kv.second.get());
    }
}

entityid_t CEntitiesManager::GetNextFreeID() const
{
    for(entityid_t i = 0; i < std::numeric_limits<entityid_t>::max(); i++)
    {
        if (Items.find(i) == Items.end())
        {
            return i;
        }
    }
    return std::numeric_limits<entityid_t>::max();
}

void CNetEntityStart::V_Write(CBufferWrapper& wrapper)
{
    auto entity = Entity.Get();
    auto& entfactory = CEngine::GetInstance()->EntitiesFactory;
    auto& cmpfactory = CEngine::GetInstance()->EntityComponentsFactory;
    
    auto entityType = entfactory.GetIndex(entity->GetType());

    wrapper.Write<std::uint16_t>(entity->GetUUID());
    wrapper.Write<std::uint16_t>(entityType);

    std::vector<std::uint8_t> _ent_data;
    CBufferWrapper _ent_wrapper(_ent_data);

    entity->FullPack(_ent_wrapper);

    wrapper.Write<std::uint16_t>(_ent_data.size());
    wrapper.WriteData(_ent_data.data(), _ent_data.size());

    std::vector<CEntityComponent*> toWrite;
    for(auto& kv : entity->Components.Items)
    {
        if(kv.second->NetSync)
        {
            toWrite.push_back(kv.second.get());
        }
    }

    wrapper.Write<std::uint16_t>(toWrite.size());

    for(auto comp : toWrite)
    {
        auto compType = cmpfactory.GetIndex(comp->GetType());

        wrapper.Write<std::uint16_t>(compType);

        std::vector<std::uint8_t> _comp_data;
        CBufferWrapper _comp_wrapper(_comp_data);

        comp->FullPack(_comp_wrapper);

        wrapper.Write<std::uint16_t>(_comp_data.size());
        wrapper.WriteData(_comp_data.data(), _comp_data.size());
    }
}

void CNetEntityStart::V_Read(CBufferWrapper& wrapper)
{
    std::uint16_t entid = wrapper.Read<std::uint16_t>();
    std::uint16_t enttype = wrapper.Read<std::uint16_t>();

    ServerEntityID = entid;

    auto& entfactory = CEngine::GetInstance()->EntitiesFactory;
    auto& cmpfactory = CEngine::GetInstance()->EntityComponentsFactory;

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto world = game->Worlds.GetWorld();

    auto it = std::find_if(world->Entities.Items.begin(), world->Entities.Items.end(), [entid](auto& kv) -> bool
    {
        return kv.second->GetUUID() == entid;
    });

    if(it != world->Entities.Items.end())
    {
        Log::ErrInstance() << "Entity ID duplicate\n"; //TODO disconnect -- fatal!
    }

    auto entstype = entfactory.GetByID(enttype);
    if(entstype.empty())
    {
        Log::ErrInstance() << "Entity invalid type\n"; //TODO disconnect -- fatal!
    }

    auto entSize = wrapper.Read<std::uint16_t>();
    auto _entData = wrapper.ReadData(entSize);

    std::vector<std::uint8_t> entData(_entData.get(), _entData.get() + entSize);

    EntityData.Type = entstype;
    EntityData.Data = std::move(entData);

    std::uint16_t compsCount = wrapper.Read<std::uint16_t>();

    for(std::uint16_t i = 0; i < compsCount; i++)
    {
        std::uint16_t compType = wrapper.Read<std::uint16_t>();

        auto compstype = cmpfactory.GetByID(compType);
        if(compstype.empty())
        {
            Log::ErrInstance() << "Component invalid type\n"; //TODO disconnect -- fatal!
        }

        auto compSize = wrapper.Read<std::uint16_t>();
        auto _compData = wrapper.ReadData(compSize);

        std::vector<std::uint8_t> compData(_compData.get(), _compData.get() + compSize);
        CData ccompData;

        ccompData.Type = compstype;
        ccompData.Data = std::move(compData);

        ComponentsData.push_back(std::move(ccompData));
    }
}

void CNetEntityStart::Process()
{
    auto client = CEngine::GetInstance()->Components.GetComponentTyped<CClient>();
    if(!client) { return; }

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto world = game->Worlds.GetWorld();

    auto enth = world->Entities.CreateEntity(EntityData.Type);
    auto ent = enth.Get();

    client->ServerEntityMap.emplace(ServerEntityID, ent->GetUUID());

    CBufferWrapper entData(EntityData.Data);
    ent->FullUnpack(entData);

    for(auto& _comp : ComponentsData)
    {
        ent->Components.CreateComponent(_comp.Type, false);
        auto& comp = ent->Components.GetComponent(_comp.Type);

        CBufferWrapper compData(_comp.Data);
        comp->FullUnpack(compData);
        
        comp->Init();
    }
}

void CNetEntityStop::V_Write(CBufferWrapper& wrapper)
{
    auto entity = Entity.Get();
    auto& entfactory = CEngine::GetInstance()->EntitiesFactory;
    auto& cmpfactory = CEngine::GetInstance()->EntityComponentsFactory;
    
    auto entityType = entfactory.GetIndex(entity->GetType());

    wrapper.Write<std::uint16_t>(entity->GetUUID());
    wrapper.Write<std::uint16_t>(entityType);
}

void CNetEntityStop::V_Read(CBufferWrapper& wrapper)
{
    auto client = CEngine::GetInstance()->Components.GetComponentTyped<CClient>();
    if(!client) { return; }

    std::uint16_t _entid = wrapper.Read<std::uint16_t>();
    std::uint16_t enttype = wrapper.Read<std::uint16_t>();

    std::uint16_t entid = client->ServerEntityMap[_entid];

    auto& entfactory = CEngine::GetInstance()->EntitiesFactory;
    auto& cmpfactory = CEngine::GetInstance()->EntityComponentsFactory;

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

    auto entstype = entfactory.GetByID(enttype);
    if(entstype.empty())
    {
        Log::ErrInstance() << "Entity invalid type\n"; //TODO disconnect -- fatal!
    }

    auto& entity = it->second;
    if(entity->GetType() != entstype)
    {
        Log::ErrInstance() << "Entity type mismatch\n"; //TODO disconnect -- fatal!
    }

    Entity = CEntityHandle(entity.get());
}

void CNetEntityStop::Process()
{
    if(!Entity.IsValid()) { return; }

    auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
    auto world = game->Worlds.GetWorld();

    world->Entities.DeleteEntity(Entity);
}

void CEntitiesManager::SetNextEntityInitParam(const std::string& name, CWrapableBase* wrapable)
{
    initParams.emplace(name, wrapable);
}

void CEntitiesManager::ClearNextEntityParams()
{
    initParams.clear();
}

CEntityHandle CEntitiesManager::GetEntityByID(entityid_t id)
{
    auto it = std::find_if(Items.begin(), Items.end(), [id](auto& kv) { return kv.first == id && kv.second && kv.second->GetUUID() == id; });
    if(it == Items.end()) { return CEntityHandle(); }
    return CEntityHandle(it->second.get());
}

LINK_NET_MESSAGE_TO_CLASS(CNetEntityStart, entitystart);
LINK_NET_MESSAGE_TO_CLASS(CNetEntityStop, entitystop);