#pragma once
#include "CEntity.h"
#include "CEntityHandle.h"
#include "CNetMessage.h"
#include "CServer.h"
#include "CWrapable.h"

#include "U_Random.h"

#include <memory>
#include <map>
#include <unordered_map>
#include <functional>

class CNetEntityStart : public CNetMessage
{
public:
    CEntityHandle Entity;

    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;

    void Process() override;

    struct CData
    {
        std::string Type;
        std::vector<std::uint8_t> Data;
    };

    entityid_t ServerEntityID;

    CData EntityData;
    std::vector<CData> ComponentsData;

    DEFINE_NET_MESSAGE();
};

class CNetEntityStop : public CNetMessage
{
public:
    CEntityHandle Entity;

    void V_Write(CBufferWrapper& wrapper) override;
    void V_Read(CBufferWrapper& wrapper) override;

    void Process() override;

    DEFINE_NET_MESSAGE();
};

class CEntitiesManager
{
public:
    CEntitiesManager() = delete;
    CEntitiesManager(worldid_t worldid);

    void SetNextEntityInitParam(const std::string& name, CWrapableBase* wrapable);
    void ClearNextEntityParams();

    /*template<typename T>
    void SetNextEntityInitParam(const std::string& name, const T& value)
    {
        initParams.emplace(name, new CWrapable<T>(value));
    }*/

    template<typename EntityType>
    CEntityHandle CreateEntity()
    {
        if(!m_isValidWorld(GetWorldID())) { return CEntityHandle(); }

        std::unique_ptr<EntityType> ent = std::make_unique<EntityType>();
        if(!ent) { return CEntityHandle(); }

        return m_addEntity(std::move(ent));
    }

    CEntityHandle CreateEntity(const std::string& type, bool sync = true);
    void DeleteEntity(const CEntityHandle& handle);

    CEntityHandle GetEntityByID(entityid_t id);

    entityid_t GetNextFreeID() const;
    worldid_t GetWorldID() const;

    size_t Count() const;

    void Trace(const std::function<void(CEntity* ent)>& func);
    std::unordered_map<entityid_t, std::unique_ptr<CEntity>> Items;
private:
    bool m_isValidWorld(worldid_t worldid);
    CEntityHandle m_addEntity(std::unique_ptr<CEntity>&& ent, bool sync = true);

    std::unordered_map<std::string, CWrapableBase*> initParams;
    worldid_t WorldID;
};
