#include "CEntity.h"
#include "CEntityHandle.h"
#include "U_ShortAPI.h"

CEntity::CEntity() : Components(this) {}

CEntity::~CEntity() { OnDestruct(this); }
std::string CEntity::GetType() const { return {}; }

CEntityHandle CEntity::GetHandle() const
{
    return CEntityHandle(this);
}

void CEntity::V_Update() {}

void CEntity::OverridePersistent(serial_t serial, entityid_t id, worldid_t worldid)
{
    SerialNumber = serial;
    UUID = id;
    WorldID = worldid;
}

void CEntity::AddInitParam(const std::string& name, CWrapableBase* wrapable)
{
    if(!wrapable) { return; }
    InitParams.emplace(name, std::unique_ptr<CWrapableBase>(wrapable));
}

void CEntity::FullPack(CBufferWrapper& packet) {}
void CEntity::FullUnpack(CBufferWrapper& packet) {}

serial_t CEntity::GetSerialNumber() const
{
    return SerialNumber;
}

entityid_t CEntity::GetUUID() const
{
    return UUID;
}

worldid_t CEntity::GetWorldID() const
{
    return WorldID;
}

void CEntity::Update()
{
    Components.Trace([](CEntityComponent* _comp) { _comp->Update(); });

    V_Update();
    OnUpdate(this);
}

void CEntity::Init()
{
    V_Init();
    OnInit(this);
}

void CEntity::SetNetSync(bool sync, bool sendMessage)
{
    if(sendMessage)
    {
        if(NetSync && !sync)
        {
            //TODO stop net entity message
        }
        else if(!NetSync && sync)
        {
            //TODO start net entity message
        }
    }
    NetSync = sync;
}

bool CEntity::IsNetSync() const
{
    return NetSync;
}

void CEntity::V_Init() {}

void CEntity::PostInit()
{
    Log::Instance() << "CEntity::PostInit\n";
    Components.Trace([](CEntityComponent* comp)
    {
        comp->Init();
    });

    V_PostInit();
    OnPostInit(this);
}

void CEntity::V_PostInit() {}

bool CEntity::V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    auto components = table.create_named("components");

    components.set_function("create", [this](const std::string& _typename, sol::object instantInit)
    {
        bool instantInitBool = true;
        if(instantInit.valid() && instantInit.is<bool>())
        {
            instantInitBool = instantInit.as<bool>();
        }
        Components.CreateComponent(_typename, instantInitBool);
    });

    components.set_function("isPresent", [this](const std::string& _typename) -> bool
    {
        return Components.IsComponentPresent(_typename);
    });

    SetFieldsManager(components, [this](FieldsManPtr fieldsMan)
    {
        //using Optional = std::optional<sol::object>;
        //using IndexFunc = std::function<Optional(sol::table, sol::object, sol::this_state)>;

        fieldsMan->AddIndexChecker([this](sol::table tabl, sol::object obj, sol::this_state ts) -> std::optional<sol::object>
        {
            if(obj.is<std::string>())
            {
                auto keystr = obj.as<std::string>();
                if(this->Components.IsComponentPresent(keystr))
                {
                    auto& comp = this->Components.GetComponent(keystr);
                    return comp->GetScriptTable(ts);
                }
            }
            return std::nullopt;
        });
    });

    table.set_function("init", [this]() { Init(); });
    table.set_function("postinit", [this]() { Init(); });
	table.set_function("update", [this]() { Update(); });

	table.set_function("getInitParam", [this](const std::string& name, sol::this_state ts) -> sol::object
    {
        auto param = GetInitParamRaw(name);
        if(!param.has_value()) { return sol::lua_nil; }
        auto luawrap = dynamic_cast<ILuaWrapableBase*>(param.value());
        if(!luawrap) { return sol::lua_nil; }
        return luawrap->ConvertToLua(ts);
    });

    table.set_function("setInitParam", [this](const std::string& name, sol::object obj, sol::this_state ts) -> int
    {
        if(obj.get_type() == sol::type::boolean) { AddInitParam(name, new CWrapable<bool>(obj.as<bool>())); }
        else if(obj.get_type() == sol::type::number) { AddInitParam(name, new CWrapable<double>(obj.as<double>())); }
        else if(obj.get_type() == sol::type::string) { AddInitParam(name, new CWrapable<std::wstring>(obj.as<std::wstring>())); }
        else { return 1; }
        return 0;
    });

    SetFieldsManager(table, [this](FieldsManPtr fieldsMan)
    {
        fieldsMan->AddField("type", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return sol::make_object(ts, this->GetType()); }));
        fieldsMan->AddField("serialNumber", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return sol::make_object<size_t>(ts, this->GetSerialNumber()); }));
        fieldsMan->AddField("uuid", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return sol::make_object<size_t>(ts, this->GetUUID()); }));
        fieldsMan->AddField("worldId", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return sol::make_object<size_t>(ts, this->GetWorldID()); }));
        fieldsMan->AddField("oninit", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnInit.GetScriptTable(ts); }));
        fieldsMan->AddField("onpostinit", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnPostInit.GetScriptTable(ts); }));
        fieldsMan->AddField("onupdate", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnUpdate.GetScriptTable(ts); }));
        fieldsMan->AddField("ondestruct", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object { return OnDestruct.GetScriptTable(ts); }));
    });
    return true;
}

std::optional<CWrapableBase*> CEntity::GetInitParamRaw(const std::string& name)
{
    auto it = std::find_if(InitParams.begin(), InitParams.end(), [&name](auto& kv) -> bool { return kv.first == name; });
    if(it == InitParams.end()) { return std::nullopt; }
    return it->second.get();
}
