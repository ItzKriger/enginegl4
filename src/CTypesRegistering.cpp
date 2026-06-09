#include "CTypesRegistering.h"
#include "CScriptingEngine.h"

#include "CLogger.h"
#include "CWindowManager.h"
#include "CCommand.h"
#include "CCommandProcessor.h"
#include "CResourcesManager.h"
#include "CConVarManager.h"
#include "CWindowManager.h"
#include "CEngine.h"
#include "CScenesManager.h"
#include "CGame.h"
#include "CServer.h"
#include "CClient.h"
#include "CTransformable.h"
#include "CEntity.h"
#include "CPhysicalEntity.h"
#include "CDrawableEntity.h"
#include "CDrawable3D.h"
#include "CDrawableAnimatable.h"

#include "ENT_Generic.h"

#include "U_Scripting.h"
#include "U_ScriptClasses.h"

#include "SDL3/SDL.h"
#include "boost/bimap.hpp"
#include "boost/assign/list_of.hpp"

sol::object entity_index_func(CEntity& _obj, sol::object _key, sol::this_state ts)
{
    sol::state_view lua(ts);
    std::string key;

    if(_key.is<std::string>())
    {
        key = _key.as<std::string>();
    }

    if(!key.empty())
    {
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        auto pres = _obj.Components.IsComponentPresent(key);

        if(pres)
        {
            auto& comp = _obj.Components.GetComponent(key);
            return comp->GetScriptUserType(ts);
        }
    }
    return userdata_index_func<CEntity>(_obj, _key, ts);
}

void CScriptingEngine::RegisterGenericTypes()
{
    State->new_usertype<CGenericComponent>
    (
        "CGenericComponent",
        SOL_COMPONENT_BASE
    );

    State->new_usertype<ENT_Generic>
    (
        "ENT_Generic",
        SOL_ENTITY_BASE
    );
}

template<typename T>
std::function<CWrapableBase*(sol::object)> CvarCreateFunction()
{
    return [](sol::object obj) -> CWrapableBase*
    {
        CWrapable<T>* ret = new CWrapable<T>();

        if(obj.valid())
        {
            ILuaWrapableBase* luawrap = dynamic_cast<ILuaWrapableBase*>(ret);
            luawrap->SetFromLua(obj);
        }
        return ret;
    };
}

enum class CCvarType
{
    Bool,
    Int,
    Uint,
    Float,
    Double,
    Angle,
    Angles,
    Quat,
    Vec2,
    Vec3,
    Vec4,
    String,
    Wstring
};

static std::unordered_map<CCvarType, std::function<CWrapableBase*(sol::object)>> CvarTypesTable =
{
    { CCvarType::Bool, CvarCreateFunction<bool>() },
    { CCvarType::Int, CvarCreateFunction<long long>() },
    { CCvarType::Uint, CvarCreateFunction<unsigned long long>() },
    { CCvarType::Float, CvarCreateFunction<float>() },
    { CCvarType::Double, CvarCreateFunction<double>() },
    { CCvarType::Angle, CvarCreateFunction<CAngle>() },
    { CCvarType::Angles, CvarCreateFunction<CAngles>() },
    { CCvarType::Quat, CvarCreateFunction<glm::quat>() },
    { CCvarType::Vec2, CvarCreateFunction<glm::vec2>() },
    { CCvarType::Vec3, CvarCreateFunction<glm::vec3>() },
    { CCvarType::Vec4, CvarCreateFunction<glm::vec4>() },
    { CCvarType::String, CvarCreateFunction<std::string>() },
    { CCvarType::Wstring, CvarCreateFunction<std::wstring>() }
};

void CScriptingEngine::RegisterMath()
{
    sol::table math = State->create_named_table("Math");

    math.set_function("AngleAxis", [](sol::this_state ts, const CAngle& angle, const glm::vec3& vec)
    {
        return sol::make_object_userdata<glm::quat>(ts, glm::angleAxis(angle.asRadians(), vec));
    });
}

void CScriptingEngine::RegisterConVarTypes()
{
    State->new_usertype<CConVar>
    (
        "CConVar",
        sol::no_constructor,
        "Value", sol::property([](CConVar& cvar, sol::this_state ts) -> sol::object
        {
            ILuaWrapableBase* lua = dynamic_cast<ILuaWrapableBase*>(cvar.Wrapable.get());
            if(!lua) { return sol::lua_nil; }

            return lua->ConvertToLua(ts);
        },
        [](CConVar& cvar, sol::object obj, sol::this_state ts) -> void
        {
            ILuaWrapableBase* lua = dynamic_cast<ILuaWrapableBase*>(cvar.Wrapable.get());
            if(!lua || !obj.valid()) { return; }

            lua->SetFromLua(obj);
        }),
        "Description", &CConVar::Description
    );

    State->new_enum<CCvarType>
    (
        "CVarType",
        {
            { "Bool", CCvarType::Bool },
            { "Int", CCvarType::Int },
            { "Uint", CCvarType::Uint },
            { "Float", CCvarType::Float },
            { "Double", CCvarType::Double },
            { "Angle", CCvarType::Angle },
            { "Angles", CCvarType::Angles },
            { "Quat", CCvarType::Quat },
            { "Vec2", CCvarType::Vec2 },
            { "Vec3", CCvarType::Vec3 },
            { "Vec4", CCvarType::Vec4 },
            { "String", CCvarType::String },
            { "Wstring", CCvarType::Wstring }
        }
    );

    State->new_usertype<CConVarManager>
    (
        "CConVarManager",
        sol::no_constructor,
        SOL_COMPONENT_BASE,
        "IsConVarExist", &CConVarManager::IsConVarExist,
        "GetConVar", [](CConVarManager& mgr, const std::string& path, sol::this_state ts) -> sol::object
        {
            auto node = mgr.GetRealConVarNode(path);
            if(!node) { return sol::lua_nil; }

            return sol::make_object(ts, &node->Value);
        },
        "DeleteConVar", &CConVarManager::DeleteConVar,
        "CreateConVar", [](CConVarManager& mgr, const std::string& path, CCvarType type, sol::object obj, sol::this_state ts) -> sol::object
        {
            auto cvar = mgr.GetConVar(path);
            if(cvar) { return sol::lua_nil; }

            auto node = mgr.AddConVar(path, CvarTypesTable.at(type)(obj));
            return sol::make_object(ts, &node->Value);
        }
    );
}

void CScriptingEngine::RegisterEntityTypes()
{
    State->new_usertype<CEntityComponent>
    (
        "CEntityComponent",
        sol::no_constructor,
        "Init", &CEntityComponent::Init,
        "Update", &CEntityComponent::Update,
        "GetType", &CEntityComponent::GetType,
        "GetEntity", &CEntityComponent::GetEntity,
        "NetSync", &CEntityComponent::NetSync //TODO on change events
    );

    State->new_usertype<CEntityHandle>
    (
        "CEntityHandle",
        sol::no_constructor,
        "Get", &CEntityHandle::Get,
        "IsValid", &CEntityHandle::IsValid,
        "CmpHandle", [](const CEntityHandle& handle, const CEntityHandle& other) { return handle == other; },
        "CmpEntity", [](const CEntityHandle& handle, CEntity* ent) { return handle == ent; }
    );

    void Add(std::unique_ptr<CEntityComponent>&& comp);
	bool IsComponentPresent(const std::string& type);
	std::unique_ptr<CEntityComponent>& GetComponent(const std::string& type);
	void CreateComponent(const std::string& type, bool instantInit = true);
	void Trace(const std::function<void(CEntityComponent* nod)>& func);

    State->new_usertype<CEntityComponentsManager>
    (
        "CEntityComponentsManager",
        sol::no_constructor,
        "GetEntity", &CEntityComponentsManager::GetEntity,
        "IsComponentPresent", [](CEntityComponentsManager& mgr, const std::string& type) -> bool { return mgr.IsComponentPresent(type); },
        "GetComponent", [](CEntityComponentsManager& mgr, const std::string& type) -> CEntityComponent* { return mgr.GetComponent(type).get(); },
        "CreateComponent", [](CEntityComponentsManager& mgr, const std::string& type, sol::object _instantInit) -> CEntityComponent*
        {
            bool instantInit = true;
            if(_instantInit.valid() && _instantInit.is<bool>()) { instantInit = _instantInit.as<bool>(); }

            mgr.CreateComponent(type, instantInit);
            return mgr.GetComponent(type).get();
        },
        "Trace", [](CEntityComponentsManager& mgr, sol::function func)
        {
            mgr.Trace([&func](CEntityComponent* comp) { func(comp); });
        }
    );

    State->new_usertype<CEntity>
    (
        "CEntity",
        sol::no_constructor,
        "Init", &CEntity::Init,
        "PostInit", &CEntity::PostInit,
        "Update", &CEntity::Update,
        "GetSerialNumber", &CEntity::GetSerialNumber,
        "GetUUID", &CEntity::GetUUID,
        "GetWorldID", &CEntity::GetWorldID,
        "IsNetSync", &CEntity::IsNetSync,
        "GetHandle", &CEntity::GetHandle,
        "GetType", &CEntity::GetType,
        "SetNetSync", [](CEntity& ent, sol::object _sync, sol::object _sendmsg)
        {
            bool sync = true;
            bool sendmsg = true;

            if(_sync.valid() && _sync.is<bool>()) { sync = _sync.as<bool>(); }
            if(_sendmsg.valid() && _sendmsg.is<bool>()) { sendmsg = _sendmsg.as<bool>(); }

            ent.SetNetSync(sync, sendmsg);
        },
        "Components", sol::property([](CEntity& entity) -> CEntityComponentsManager&
        {
            return entity.Components;
        }),
        "AddInitParam", [](CEntity& entity, const std::string& name, sol::object obj)
        {
            auto wrap = ScriptUtils::ObjectToWrapable(obj);
            if(!wrap) { return; }

            entity.AddInitParam(name, wrap);
        },
        "GetInitParam", [](CEntity& entity, const std::string& name, sol::this_state ts) -> sol::object
        {
            auto _wrap = entity.GetInitParamRaw(name);
            if(!_wrap.has_value() || !_wrap.value()) { return sol::lua_nil; }

            auto luawrap = dynamic_cast<ILuaWrapableBase*>(_wrap.value());
            if(!luawrap) { return sol::lua_nil; }

            return luawrap->ConvertToLua(ts);
        },
        sol::meta_function::index,
        &entity_index_func,
        sol::meta_function::new_index,
        &userdata_new_index_func<CEntity>
    );
}

void CScriptingEngine::RegisterGameTypes()
{
    State->new_usertype<CGame>
    (
        "CGame",
        sol::no_constructor,
        SOL_COMPONENT_BASE,
        "Worlds", &CGame::Worlds,
        "Physics", sol::property([](CGame& game) { return game.PhysicsEngine.get(); })
    );

    State->new_usertype<CEntitiesManager>
    (
        "CEntitiesManager",
        sol::no_constructor,
        "Count", &CEntitiesManager::Count,
        "GetNextFreeID", &CEntitiesManager::GetNextFreeID,
        "GetWorldID", &CEntitiesManager::GetWorldID,
        "DeleteEntity", &CEntitiesManager::DeleteEntity,
        "GetEntityByID", &CEntitiesManager::GetEntityByID,
        "CreateEntity", [](CEntitiesManager& mgr, const std::string& type, sol::object _sync)
        {
            bool sync = true;
            if(_sync.valid() && _sync.is<bool>())
            {
                sync = _sync.as<bool>();
            }

            return mgr.CreateEntity(type, sync);
        },
        "Trace", [](CEntitiesManager& mgr, sol::function func)
        {
            mgr.Trace([&func](CEntity* ent) { func(ent); });
        },
        "ClearNextEntityParams", &CEntitiesManager::ClearNextEntityParams,
        "SetNextEntityInitParam", [](CEntitiesManager& mgr, const std::string& name, sol::object obj)
        {
            auto wrap = ScriptUtils::ObjectToWrapable(obj);
            if(!wrap) { return; }

            mgr.SetNextEntityInitParam(name, wrap);
        }
    );

    State->new_usertype<CWorld>
    (
        "CWorld",
        sol::no_constructor,
        "GetUUID", &CWorld::GetUUID,
        "GetID", &CWorld::GetUUID,
        "Id", sol::property([](const CWorld& world) { return world.GetUUID(); }),
        "Update", &CWorld::Update,
        "Entities", sol::property([](CWorld& world) -> CEntitiesManager&
        {
            return world.Entities;
        })
    );

    State->new_usertype<CWorldsManager>
    (
        "CWorldsManager",
        sol::no_constructor,
        "CreateWorld", [](CWorldsManager& mann) -> CWorld*
        {
            auto id = mann.CreateWorld();
            return mann.GetWorld(id);
        },
        "GetWorld", [](CWorldsManager& mann, sol::object id) -> CWorld*
        {
            worldid_t _id = 0;
            if(id.valid() && id.is<int>())
            {
                _id = id.as<int>();
            }

            return mann.GetWorld(_id);
        },
        "IsWorldExist", [](CWorldsManager& mann, sol::object id) -> bool
        {
            worldid_t _id = 0;
            if(id.valid() && id.is<int>())
            {
                _id = id.as<int>();
            }

            return mann.IsWorldExist(_id);
        },
        "Count", &CWorldsManager::Count,
        "DeleteWorld", sol::overload
        (
            [](CWorldsManager& mann)
            {
                mann.DeleteWorld(0);
            },
            [](CWorldsManager& mann, int id)
            {
                mann.DeleteWorld(id);
            },
            [](CWorldsManager& mann, CWorld& world)
            {
                mann.DeleteWorld(world.GetUUID());
            }
        ),
        "GetNextFreeID", &CWorldsManager::GetNextFreeID,
        "Update", &CWorldsManager::Update,
        "Trace", [](CWorldsManager& mann, sol::function func)
        {
            for(auto& world : mann.Items)
            {
                func(world.second.get());
            }
        }
    );
}

namespace sol
{
    template <>
    struct is_container<std::filesystem::path> : std::false_type {};
}

void CScriptingEngine::RegisterResourceTypes()
{
    State->new_enum<CResource::CLoadingStatus>
    (
        "CLoadingStatus",
        {
            { "None", CResource::CLoadingStatus::None },
            { "Waiting", CResource::CLoadingStatus::Waiting },
            { "Sync", CResource::CLoadingStatus::Sync },
            { "WaitingAsync", CResource::CLoadingStatus::WaitingAsync },
            { "StartedAsync", CResource::CLoadingStatus::StartedAsync },
            { "CompletedAsync", CResource::CLoadingStatus::CompletedAsync },
            { "WaitingForRequired", CResource::CLoadingStatus::WaitingForRequired },
            { "Done", CResource::CLoadingStatus::Done }
        }
    );

    State->new_usertype<CLoadingContext>
    (
        "CLoadingContext",
        sol::no_constructor,
        "CurrentResource", &CLoadingContext::CurrentResource,
        "LoadPath", &CLoadingContext::LoadPath,
        "RequiredResources", &CLoadingContext::RequiredResources
    );

    State->new_usertype<CResource>
    (
        "CResource",
        sol::no_constructor,
        "GetType", &CResource::GetType,
        "StartPipeline", &CResource::StartPipeline,
        "ProcessPipeline", &CResource::ProcessPipeline,
        "PipelineWaitForRequired", &CResource::PipelineWaitForRequired,
        "PipelineCheckRequired", &CResource::PipelineCheckRequired,
        "Wait", &CResource::Wait,
        "IsReady", &CResource::IsReady,
        "Load", &CResource::Load,
        "LoadingStatus", &CResource::LoadingStatus,
        "Name", &CResource::Name,
        "LoadingContext", &CResource::LoadingContext,
        "LoadingFunctionIndex", &CResource::LoadingFunctionIndex,
        "OnDoneLoading", sol::property([](CResource& res) -> HANDLER
        {
            return res.OnDoneLoading;
        })
    );
}

void CScriptingEngine::RegisterCallbackHandlerType()
{
    State->new_usertype<CSmartCallback>
    (
        "CSmartCallback",
        sol::no_constructor,
        "Invalidate", &CSmartCallback::Invalidate,
        "ForceDelete", &CSmartCallback::ForceDelete,
        "IsValid", &CSmartCallback::IsValid,
        "Handler", sol::property([](CSmartCallback& callback) -> CCallbackHandlerBase& { return callback.Handler; })
    );

    State->new_usertype<CCallbackHandlerBase>
    (
        "CCallbackHandler",
        sol::no_constructor,
        "GetLastFunctionHandle", &CCallbackHandlerBase::GetLastFunctionHandleShared,
        "Add", &CCallbackHandlerBase::AddLuaFunction,
        "Call", &CCallbackHandlerBase::CallLua,
        "IsEnabled", &CCallbackHandlerBase::IsEnabled,
        "SetState", &CCallbackHandlerBase::SetEnabledState,
        "Delete", &CCallbackHandlerBase::DeleteFunctionsByName
    );
}

static std::vector<std::string> Priorities = 
{
    "default",
    "indexed",
    "first",
    "last",
    "after"
};

void setPriorityFunc(CComponent& comp, int _type, sol::variadic_args va)
{
    if(_type < 0 || _type >= Priorities.size()) { return; }

    std::string type = Priorities.at(_type);
    if(type == "default")
    {
        comp.SetPriority(std::make_unique<CDefaultPriority>());
    }
    else if(type == "indexed")
    {
        if(va.size() == 0) { return; }
        if(va.get_type(0) != sol::type::number) { return; }

        int index = va.get<int>(0);

        comp.SetPriority(std::make_unique<CIndexedPriority>(index));
    }
    else if(type == "first")
    {
        comp.SetPriority(std::make_unique<CFirstPriority>());
    }
    else if(type == "last")
    {
        comp.SetPriority(std::make_unique<CLastPriority>());
    }
    else if(type == "after")
    {
        std::vector<std::string> afterComps;
        for(auto arg : va)
        {
            if(arg.get_type() != sol::type::string) { continue; }
            afterComps.push_back(arg.get<std::string>());
        }

        if(!afterComps.empty())
        {
            comp.SetPriority(std::make_unique<CAfterPriority>(afterComps));
        }
    }
};

int getPriorityFunc(CComponent& comp)
{
    auto prio = comp.GetPriority();
    if(!prio) { return -1; }

    if(dynamic_cast<CDefaultPriority*>(prio))
    {
        return 0;
    }
    else if(dynamic_cast<CIndexedPriority*>(prio))
    {
        return 1;
    }
    else if(dynamic_cast<CFirstPriority*>(prio))
    {
        return 2;
    }
    else if(dynamic_cast<CLastPriority*>(prio))
    {
        return 3;
    }
    else if(dynamic_cast<CAfterPriority*>(prio))
    {
        return 4;
    }

    return -1;
};

void CScriptingEngine::RegisterComponentType()
{
    sol::table priority = State->create_named_table("priority");

    size_t index = 0;
    for(auto& pr : Priorities)
    {
        priority.set(pr, index);
        index++;
    }

    State->new_usertype<CComponent>
    (
        "CComponent",
        "GetType", &CComponent::GetType,
        "Init", &CComponent::Init,
        "DeInit", &CComponent::DeInit,
        "PostInit", &CComponent::PostInit,
        "Update", &CComponent::Update,
        "SetPriority", &setPriorityFunc,
        "ResetPriority", &CComponent::ResetPriority,
        "GetPriority", &getPriorityFunc,
        "OnInit", sol::property([](CComponent& _comp) -> HANDLER { return _comp.OnInit; }),
        "OnPostInit", sol::property([](CComponent& _comp) -> HANDLER { return _comp.OnPostInit; }),
        "OnUpdate", sol::property([](CComponent& _comp) -> HANDLER { return _comp.OnUpdate; }),
        "OnDestruct", sol::property([](CComponent& _comp) -> HANDLER { return _comp.OnDestruct; }),
        "OnDeInit", sol::property([](CComponent& _comp) -> HANDLER { return _comp.OnDeInit; }),
        sol::meta_function::index,
        &userdata_index_func<CComponent>,
        sol::meta_function::new_index,
        &userdata_new_index_func<CComponent>
    );
}

void CScriptingEngine::RegisterDrawableTypes()
{
    State->new_usertype<CDrawable>
    (
        "CDrawable",
        sol::no_constructor,
        "Update", &CDrawable::Update,
        "Draw", &CDrawable::Draw,
        "OnUpdate", sol::property([](CDrawable& drawable) -> HANDLER { return drawable.OnUpdate; })
    );

    State->new_usertype<CDrawable3D>
    (
        "CDrawable3D",
        sol::no_constructor,
        sol::base_classes, sol::bases<CDrawable>(),
        "Transform", &CDrawable3D::Transform
    );

    State->new_usertype<CDrawableModel>
    (
        "CDrawableModel",
        sol::no_constructor,
        sol::base_classes, sol::bases<CDrawable3D>(),
        "GetModel", &CDrawableModel::GetModel,
        "SetModel", sol::overload
        (
            [](CDrawableModel& drawable, std::shared_ptr<CResource> model)
            {
                return drawable.SetModel(model);
            },
            [](CDrawableModel& drawable, std::shared_ptr<CModelBase> model)
            {
                auto casted = std::dynamic_pointer_cast<CResource>(model);
                return drawable.SetModel(casted);
            }
        ),
        "OnSetModel", sol::property([](CDrawableModel& drawable) -> HANDLER { return drawable.OnSetModel; }),
        "OnPreDraw", sol::property([](CDrawableModel& drawable) -> HANDLER { return drawable.OnPreDraw; }),
        "OnPostDraw", sol::property([](CDrawableModel& drawable) -> HANDLER { return drawable.OnPostDraw; })
    );

    State->new_usertype<CDrawableAnimatable>
    (
        "CDrawableAnimatable",
        sol::no_constructor,
        sol::base_classes, sol::bases<CDrawableModel>() //TODO
    );
}

void CScriptingEngine::RegisterOtherTypes()
{
    std::cout << "RegisterOtherTypes()\n";

    RegisterCallbackHandlerType();

    State->new_usertype<nlohmann::json>
    (
        "CJson",
        sol::no_constructor,
        "IsArray", [](const nlohmann::json& _json) -> bool { return _json.is_array(); },
        "IsBoolean", [](const nlohmann::json& _json) -> bool { return _json.is_boolean(); },
        "IsString", [](const nlohmann::json& _json) -> bool { return _json.is_string(); },
        "IsNumber", [](const nlohmann::json& _json) -> bool { return _json.is_number(); },
        "Contains", [](const nlohmann::json& _json, const std::string& name) -> bool { return _json.contains(name); },
        sol::meta_function::index,
        [](const nlohmann::json& _json, sol::object key, sol::this_state ts)
        {
            //TODO
        }
    );

    State->new_usertype<std::filesystem::path>
    (
        "CPath",
        sol::constructors<std::filesystem::path(), std::filesystem::path(const std::string&)>(),
        "string", [](const std::filesystem::path& p)
        {
            return p.string();
        },
        "generic_string", [](const std::filesystem::path& p)
        {
            return p.generic_string();
        },
        "filename", [](const std::filesystem::path& p) { return p.filename().string(); }
    );

    /*State->new_usertype<std::filesystem::path>
    (
        "path",
        sol::constructors
        <
            std::filesystem::path(),
            std::filesystem::path(const std::string&)
        >(),
        "string", [](const std::filesystem::path& p)
        {
            return p.string();
        }
    );*/

    RegisterDrawableTypes();
    RegisterComponentType();

    RegisterEntityTypes();
    RegisterGameTypes();
    RegisterMath();
    RegisterConVarTypes();
    RegisterCommandsTypes();
    RegisterGenericTypes();
    RegisterTransformTypes();
    RegisterPhysicsTypes();

    State->new_usertype<CResourcesManager>
    (
        "CResourcesManager",
        SOL_COMPONENT_BASE
    );

    State->new_usertype<CLogger>
    (
        "CLogger",
        SOL_COMPONENT_BASE,
        "Out", &CLogger::Out,
        "Outln", &CLogger::Outln,
        "Err", &CLogger::Err,
        "Errln", &CLogger::Errln,
        "SetAsync", &CLogger::SetAsync,
        "IsAsync", &CLogger::IsAsync,
        "OnOut", sol::property([](CLogger& _logger) -> HANDLER { return _logger.OnOut; }),
        "OnErr", sol::property([](CLogger& _logger) -> HANDLER { return _logger.OnErr; })
    );

    State->new_usertype<CServer>
    (
        "CServer",
        SOL_COMPONENT_BASE,
        "GetFreeID", &CServer::GetFreeID,
        "SetRate", &CServer::SetRate,
        "GetRate", &CServer::GetRate
    );

    State->new_usertype<CClient>
    (
        "CClient",
        SOL_COMPONENT_BASE,
        "IsTransceiverValid", [](CClient& cl) -> bool { return cl.Transceiver.operator bool(); },
        "SetServerRate", &CClient::SetServerRate,
        "GetServerRate", &CClient::GetServerRate,
        "Reset", &CClient::Reset
    );

    State->new_usertype<CPhysicalEntity>
    (
        "CPhysicalEntity",
        SOL_ENTITY_COMPONENT_BASE,
        "WorldUnit", &CPhysicalEntity::WorldUnit,
        "Unit", &CPhysicalEntity::WorldUnit,
        "SetUnit", [](CPhysicalEntity& ent, CWorldUnit* newUnit) { ent.WorldUnit = newUnit; }
    );

    State->new_usertype<CTransformable>
    (
        "CTransformable",
        SOL_ENTITY_COMPONENT_BASE,
        "Transform", &CTransformable::Transform
    );

    State->new_usertype<CDrawableEntity>
    (
        "CDrawableEntity",
        SOL_ENTITY_COMPONENT_BASE,
        "Drawable", &CDrawableEntity::Drawable
    );
    
    static boost::bimap<std::string, SDL_Scancode> stringToCodes =
        boost::assign::list_of<boost::bimap<std::string, SDL_Scancode>::value_type>
        ("a", SDL_SCANCODE_A)
        ("b", SDL_SCANCODE_B)
        ("c", SDL_SCANCODE_C)
        ("d", SDL_SCANCODE_D)
        ("e", SDL_SCANCODE_E)
        ("f", SDL_SCANCODE_F)
        ("g", SDL_SCANCODE_G)
        ("h", SDL_SCANCODE_H)
        ("i", SDL_SCANCODE_I)
        ("j", SDL_SCANCODE_J)
        ("k", SDL_SCANCODE_K)
        ("l", SDL_SCANCODE_L)
        ("m", SDL_SCANCODE_M)
        ("n", SDL_SCANCODE_N)
        ("o", SDL_SCANCODE_O)
        ("p", SDL_SCANCODE_P)
        ("q", SDL_SCANCODE_Q)
        ("r", SDL_SCANCODE_R)
        ("s", SDL_SCANCODE_S)
        ("t", SDL_SCANCODE_T)
        ("u", SDL_SCANCODE_U)
        ("v", SDL_SCANCODE_V)
        ("w", SDL_SCANCODE_W)
        ("x", SDL_SCANCODE_X)
        ("y", SDL_SCANCODE_Y)
        ("z", SDL_SCANCODE_Z)
        ("lshift", SDL_SCANCODE_LSHIFT)
        ("lctrl", SDL_SCANCODE_LCTRL)
        ("lalt", SDL_SCANCODE_LALT)
        ("rshift", SDL_SCANCODE_RSHIFT)
        ("rctrl", SDL_SCANCODE_RCTRL)
        ("ralt", SDL_SCANCODE_RALT);

    State->new_usertype<CWindowManager>
    (
        "CWindowManager",
        SOL_COMPONENT_BASE,
        "OnEventHandle", sol::property([](CWindowManager& winman) -> HANDLER { return winman.OnEventHandle; }),
        "GetRelativeMouseState", [](CWindowManager& winman) -> glm::vec2
        {
            float dx, dy;
            SDL_GetRelativeMouseState(&dx, &dy);

            return { dx, dy };
        },
        "SetRelativeMouseMode", [](CWindowManager& winman, bool enable) -> void { SDL_SetWindowRelativeMouseMode(winman.Window, enable); },
	    "GetRelativeMouseMode", [](CWindowManager& winman) -> bool { return SDL_GetWindowRelativeMouseMode(winman.Window); },
	    "GetKeyState", [](CWindowManager& winman, const std::string& name) -> bool
        {
            const bool* keyboard = SDL_GetKeyboardState(NULL);
            auto it = stringToCodes.left.find(name);

            if(it == stringToCodes.left.end()) { return false; }
            return keyboard[it->second];
        },
        "PumpEvents", [](CWindowManager& winman)
        {
            SDL_PumpEvents();
        }
    );

    RegisterEngineType();
    (*State)["Engine"] = sol::make_object(*State, CEngine::GetInstance());
    std::cout << "RegisterOtherTypes() end\n";
}
