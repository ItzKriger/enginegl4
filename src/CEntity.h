#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

#include "CEntityComponentsManager.h"
#include "U_Typedefs.h"
#include "CCallbackHandler.h"
#include "CScriptObject.h"
#include "CWrapableBase.h"
#include "CWrapable.h"

#define LINK_ENTITY_TO_CLASS(_class, name) std::string _class::GetType() const { return #name; }; CFactoryInitter<_class, CEntity> __entity_initter_ ## name = CFactoryInitter<_class, CEntity>(#name, std::function<void(CFactoryInitter<_class, CEntity>*)>([](CFactoryInitter<_class, CEntity>* tter) -> void { tter->m_factory = &CEngine::GetInstance()->EntitiesFactory; }));
#define DEFINE_ENTITY() std::string GetType() const override

class CEntityHandle;
class CEntity : public CScriptObject
{
public:
    CEntity();

    virtual ~CEntity();
    virtual std::string GetType() const;

    void Init();
    virtual void V_Init();

    void PostInit();
    virtual void V_PostInit();

    void Update();
    virtual void V_Update();

    virtual void FullPack(CBufferWrapper& packet);
	virtual void FullUnpack(CBufferWrapper& packet);

    void OverridePersistent(serial_t serial, entityid_t id, worldid_t worldid);
    bool V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    serial_t GetSerialNumber() const;
    entityid_t GetUUID() const;
    worldid_t GetWorldID() const;

    CEntityComponentsManager Components;

    CCallbackHandler<void, CEntity*> OnInit; //ret:nothing this_entity
    CCallbackHandler<void, CEntity*> OnPostInit; //ret:nothing this_entity
    CCallbackHandler<void, CEntity*> OnUpdate; //ret:nothing this_entity
    CCallbackHandler<void, CEntity*> OnDestruct; //ret:nothing this_entity

    void SetNetSync(bool sync = true, bool sendMessage = true);
    bool IsNetSync() const;

    CEntityHandle GetHandle() const;

    std::unordered_map<std::string, std::unique_ptr<CWrapableBase>> InitParams;

    void AddInitParam(const std::string& name, CWrapableBase* wrapable);

    /*template<typename T>
    void AddInitParam(const std::string& name, const T& value)
    {
        InitParams.emplace(name, std::make_unique<CWrapable<T>>(value));
    }*/

    template<typename T>
    std::optional<T> GetInitParam(const std::string& name)
    {
        auto it = std::find_if(InitParams.begin(), InitParams.end(), [&name](auto& kv) -> bool { return kv.first == name && kv.second->GetTypeInfo() == typeid(T); });
        if(it == InitParams.end()) { return std::nullopt; }
        auto wrp = dynamic_cast<CWrapable<T>*>(it->second.get());
        if(!wrp) { return std::nullopt; }
        return wrp->GetValue();
    }

    std::optional<CWrapableBase*> GetInitParamRaw(const std::string& name);
    sol::table UserData;
private:
    serial_t SerialNumber = 0;
    entityid_t UUID = 0;
    worldid_t WorldID = 0;

    bool NetSync = true;
};
