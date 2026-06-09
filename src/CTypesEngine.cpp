#include "CTypesEngine.h"
#include "CTypesRegistering.h"

#include "CEngine.h"
#include "U_ScriptClasses.h"
#include "CScriptingEngine.h"
#include "CScenesManager.h"

void CScriptingEngine::RegisterEngineType()
{
    State->new_usertype<CComponentsManager>
    (
        "CComponentsManager",
        sol::no_constructor,
        "IsComponentPresent", [](CComponentsManager& mgr, const std::string& _name)
        {
            std::string name = _name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            return mgr.IsComponentPresent(name);
        },
        "GetComponent", [](CComponentsManager& mgr, const std::string& _name, sol::this_state ts) -> sol::object
        {
            std::string name = _name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            auto comp = mgr.GetComponentRaw(name);
            
            if(comp)
            {
                return comp->GetScriptUserType(ts);
            }
            return sol::lua_nil;
        },
        "Create", [](CComponentsManager& mgr, const std::string& _type, sol::object instantInitObj, sol::this_state ts) -> sol::object
        {
            std::string type = _type;
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);

            bool instantInit = true;
            if(instantInitObj.valid() && instantInitObj.is<bool>())
            {
                instantInit = instantInitObj.as<bool>();
            }

            if(!mgr.IsComponentPresent(type))
            {
                mgr.CreateComponent(type, instantInit);
                if(!mgr.IsComponentPresent(type)) { return sol::lua_nil; }
            }
            return mgr.GetComponent(type)->GetScriptUserType(ts); //TODO problem, if not initted table might not be created properly
        },
        "Register", [](CComponentsManager& mgr, sol::table functions, sol::this_state ts)
        {
            InvokeRegisterFunction<CComponent, "component">(functions, ts);
        },
        "Trace", [](CComponentsManager& mgr, sol::function func)
        {
            mgr.Trace([&func](CComponent* comp) { func(comp); });
        },
        "ReverseTrace", [](CComponentsManager& mgr, sol::function func)
        {
            mgr.ReverseTrace([&func](CComponent* comp) { func(comp); });
        }
    );

    State->new_usertype<CTime>
    (
        "CTime",
        "Init", &CTime::Init,
        "NewTick", &CTime::NewTick,
        "EndTick", &CTime::EndTick,
        "GetCurrent", &CTime::GetCurrent,
        "GetFPS", &CTime::GetFPS,
        "GetPrevFPS", &CTime::GetPrevFPS,
        "ResetDeltaTime", [](CTime& _time, sol::object obj)
        {
            double toSet = 0.0;
            if(obj.valid() && obj.is<double>())
            {
                toSet = obj.as<double>();
            }
            _time.ResetDeltaTime(toSet);
        },
        "DeltaTime", sol::property([](CTime& _time) { return _time.GetDeltaTime(); }),
        "DeltaTimePrecise", sol::property([](CTime& _time) { return _time.GetDeltaTimePrecise(); }),
        "GetDeltaTime", &CTime::GetDeltaTime,
        "GetDeltaTimePrecise", &CTime::GetDeltaTimePrecise,
        "OnNewTick", sol::property([](CTime& _time) -> HANDLER
        {
            return _time.OnNewTick;
        }),
        "OnEndTick", sol::property([](CTime& _time) -> HANDLER
        {
            return _time.OnEndTick;
        })
    );

    State->new_usertype<CBuildInfo>
    (
        "CBuildInfo",
        "Date", sol::property([](CBuildInfo& b) { return CBuildInfo::GetBuildDate(); }),
        "Time", sol::property([](CBuildInfo& b) { return CBuildInfo::GetBuildTime(); }),
        "Compiler", sol::property([](CBuildInfo& b) { return CBuildInfo::GetCompiler(); }),
        "OperatingSystem", sol::property([](CBuildInfo& b) { return CBuildInfo::GetOperatingSystemName(); })
    );

    State->new_usertype<CEngine>
    (
        "CEngine",
        sol::meta_function::index,
        [](CEngine& engine, const std::string& _key, sol::this_state ts) -> sol::object
        {
            sol::state_view lua(ts);
            std::string key = _key;

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            auto* comp = engine.Components.GetComponentRaw(key);
            if (comp)
            {
                return comp->GetScriptUserType(ts);
                //return sol::make_object(lua, comp);
            }
            return sol::nil;
        },
        "Init", &CEngine::Init,
        "DeInit", &CEngine::DeInit,
        "Update", &CEngine::Update,
        "Quit", &CEngine::Quit,
        "GetStopFlag", &CEngine::GetStopFlag,
        "BuildInfo", &CEngine::BuildInfo,
        "Time", &CEngine::Time,
        "CommandLine", &CEngine::CommandLine,
        "Components", &CEngine::Components,
        "GetMainCameraTransform", [](CEngine& engine, sol::this_state ts) -> sol::object //TODO temporary
        {
            auto scenesMan = engine.Components.GetComponentTyped<CScenesManager>();
            if(!scenesMan) { return sol::lua_nil; }

            auto mainScene = scenesMan->ActiveScene;

            if(!mainScene.lock()) { return sol::lua_nil; }
            auto _mainCamera = mainScene.lock()->CamerasManager.ActiveCamera;

            if(!_mainCamera.lock()) { return sol::lua_nil; }
            auto mainCamera = _mainCamera.lock();

            return sol::make_object(ts, &mainCamera->Transform);
        },
        "Register", [](CEngine& engine, sol::table tabl, sol::this_state ts) -> int
        {
            sol::object root_typenm = tabl["root_type"];

            if(!root_typenm.valid() || !root_typenm.is<std::string>()) { return 6; }
            std::string _strroottypenm = root_typenm.as<std::string>();

            std::string strroottypenm = _strroottypenm;
            std::transform(strroottypenm.begin(), strroottypenm.end(), strroottypenm.begin(), ::tolower);

            if(strroottypenm == "component")
            {
                return InvokeRegisterFunction<CComponent, "component">(tabl, ts);
            }
            else if(strroottypenm == "entity")
            {
                return InvokeRegisterFunction<CEntity, "entity">(tabl, ts);
            }
            else if(strroottypenm == "resource")
            {
                return InvokeRegisterFunction<CResource, "resource">(tabl, ts);
            }
            return 7;
        }
    );
}
