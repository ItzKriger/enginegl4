#include "CScriptingEngine.h"
#include "CEngine.h"
#include "CScriptingManager.h"

#include "CLuaPreProcessor.h"
#include "CScenesManager.h"
#include "U_Log.h"
#include "CLogger.h"
#include "U_ScriptClasses.h"
#include "U_Scripting.h"

#include <algorithm>

CScriptingEngine::~CScriptingEngine()
{
    COMPONENT_CALL(CScriptingManager, ResetEngineClasses(this));
}

void CScriptingEngine::V_Init() {}

template<typename T>
std::function<sol::table(sol::variadic_args)> QuatVecNewFunction()
{
    return [](sol::variadic_args va) -> sol::table
    {
        T ret;
        for(size_t i = 0; i < ret.length(); i++)
        {
            ret[i] = typename T::value_type{};
        }
        for (size_t i = 0; i < va.size() && i < ret.length(); i++)
        {
            ret[i] = va.get<typename T::value_type>(i);
        }
        return ScriptUtils::ToObject<T>(ret, va.lua_state());
        //return sol::make_object_userdata<T>(va.lua_state(), ret);
    };

    /*return [](sol::variadic_args va) -> sol::table
    {
        auto shared = std::make_shared<T>();

        for (size_t i = 0; i < shared->length(); i++)
        {
            (*shared)[i] = typename T::value_type{};
        }
        for (size_t i = 0; i < va.size() && i < shared->length(); i++)
        {
            (*shared)[i] = va.get<typename T::value_type>(i);
        }

        sol::table tabl = ScriptUtils::ToObject(*shared, va.lua_state());
        tabl["__obj"] = shared;

        return tabl;
    };*/
}

class CLuaDestructor
{
public:
    CCallbackHandler<void> OnDestruct;

    sol::function fn;
    CLuaDestructor(sol::function f) : fn(std::move(f)) {}

    ~CLuaDestructor()
    {
        if (fn.valid())
        {
            fn();
        }
    }
};

template<typename T>
auto VectorCreationFunc()
{
    using VT = T::value_type;
    constexpr size_t L = T::length();

    return sol::overload
    (
        []() -> T
        {
            return T{};
        },
        [](VT _x)
        {
            if constexpr (L == 1) { return T(_x); }
            if constexpr (L == 2) { return T(_x, _x); }
            if constexpr (L == 3) { return T(_x, _x, _x); }
            if constexpr (L == 4) { return T(_x, _x, _x, _x); }
        },
        [](VT _x, VT _y)
        {
            if constexpr (L == 1) { return T(_x); }
            if constexpr (L == 2) { return T(_x, _y); }
            if constexpr (L == 3) { return T(_x, _y, VT{}); }
            if constexpr (L == 4) { return T(_x, _y, VT{}, VT{}); }
        },
        [](VT _x, VT _y, VT _z)
        {
            if constexpr (L == 1) { return T(_x); }
            if constexpr (L == 2) { return T(_x, _y); }
            if constexpr (L == 3) { return T(_x, _y, _z); }
            if constexpr (L == 4) { return T(_x, _y, _z, VT{}); }
        },
        [](VT _x, VT _y, VT _z, VT _w)
        {
            if constexpr (L == 1) { return T(_x); }
            if constexpr (L == 2) { return T(_x, _y); }
            if constexpr (L == 3) { return T(_x, _y, _z); }
            if constexpr (L == 4) { return T(_x, _y, _z, _w); }
        },
        [](const T& v)
        {
            return T(v);
        }
    );
}

CScriptingEngine::CScriptingEngine() : ComponentsManager(State)
{
    //COMPONENT_CALL(CScriptingManager, PushCurrentEngine(this));

    State = std::make_shared<sol::state>();
    State->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math);
    
    State->set_function("getFieldsMan", [](sol::table tabl) -> sol::object
    {
        tabl[CScriptFieldsManager::GetDefaultKey()];
        //TODO return script table of field man of table 'tabl'
        return sol::lua_nil;
    });

    auto engine = State->create_named_table("engine");
    auto hook = State->create_named_table("Hook");
    auto components = engine.create_named("components");
    auto entities = engine.create_named("entities");
    auto criticallog = engine.create_named("criticallog");
    auto math = engine.create_named("math");

    State->set_function("CreateVector", VectorCreationFunc<glm::vec3>);
    State->set_function("CreateVector1f", VectorCreationFunc<glm::vec1>);
    State->set_function("CreateVector2f", VectorCreationFunc<glm::vec2>);
    State->set_function("CreateVector3f", VectorCreationFunc<glm::vec3>);
    State->set_function("CreateVector4f", VectorCreationFunc<glm::vec4>);
    State->set_function("CreateVector1", VectorCreationFunc<glm::vec1>);
    State->set_function("CreateVector2", VectorCreationFunc<glm::vec2>);
    State->set_function("CreateVector3", VectorCreationFunc<glm::vec3>);
    State->set_function("CreateVector4", VectorCreationFunc<glm::vec4>);

    State->set_function
    (
        "CreateTransform",
        sol::overload
        (
            []() -> CTransform
            {
                return CTransform();
            },
            [](const CTransformBase& trans) -> CTransform
            {
                CTransform ret;
                ret.SetPRS(trans.GetPRS());
                return ret;
            },
            [](const CTransform& trans) -> CTransform
            {
                CTransform ret;
                ret.SetPRS(trans.GetPRS());
                return ret;
            },
            [](const CTransformBase::CMeasurePack& trans) -> CTransform
            {
                CTransform ret;
                ret.SetPRS(trans);
                return ret;
            },
            [](const glm::vec3& pos) -> CTransform
            {
                CTransform ret;
                ret.SetPosition(pos);
                return ret;
            },
            [](const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scl) -> CTransform
            {
                CTransform ret;
                ret.SetPRS(pos, quat, scl);
                return ret;
            },
            [](const glm::vec3& pos, const glm::quat& quat) -> CTransform
            {
                CTransform ret;
                ret.SetPR(pos, quat);
                return ret;
            },
            [](const glm::vec3& pos, const CAngles& angles) -> CTransform
            {
                CTransform ret;
                ret.SetPosition(pos);
                ret.GetEulerRotation().SetRotation(angles);
                return ret;
            },
            [](const glm::vec3& pos, const CAngles& angles, const glm::vec3& scl) -> CTransform
            {
                CTransform ret;
                ret.SetPS(pos, scl);
                ret.GetEulerRotation().SetRotation(angles);
                return ret;
            }
        )
    );

    State->new_usertype<CLuaDestructor> //TODO destructor using __gc or shared_ptr
    (
        "CLuaDestructor",
        sol::no_constructor
    );

    State->set_function("make_destructor", [](sol::object target, sol::function destructor, sol::this_state ts)
    {
        if(!target.valid() || !destructor.valid()) { return; }

        sol::state_view lua = ts;
        std::shared_ptr<CLuaDestructor> ud = std::make_shared<CLuaDestructor>(destructor);
        if (target.is<sol::table>())
        {
            sol::table t = target.as<sol::table>();
            t["_destructor"] = ud;
            t["_onDestruct"] = ud->OnDestruct.GetScriptTable(ts);
        }
    });

    engine["time"] = CEngine::GetInstance()->Time.GetScriptTable(State);

    auto callback = State->create_named_table("callback");
    callback.set_function("create", [](sol::this_state ts) -> sol::table
    {
        auto obj = std::make_shared<CCallbackHandler<sol::object, sol::variadic_args>>();
        sol::table tabl = obj->GetScriptTable(ts);

        tabl["__obj"] = obj;
        return tabl;
    });

    engine.set_function("getMainCameraTransform", [](sol::this_state ts) -> sol::table //TODO temporary
    {
        auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
        if(!scenesMan) { return sol::lua_nil; }

        auto mainScene = scenesMan->ActiveScene;

        if(!mainScene.lock()) { return sol::lua_nil; }
        auto _mainCamera = mainScene.lock()->CamerasManager.ActiveCamera;

        if(!_mainCamera.lock()) { return sol::lua_nil; }
        auto mainCamera = _mainCamera.lock();

        return mainCamera->Transform.GetScriptTable(ts);
    });

    criticallog.set_function("out", [](const std::wstring& str, sol::object obj)
    {
        std::string path = "critical.txt";

        if(obj.valid() && obj.is<std::string>())
        {
            path = obj.as<std::string>();
        }
        Log::Critical(str, path);
    });

    criticallog.set_function("outln", [](const std::wstring& str, sol::object obj)
    {
        std::string path = "critical.txt";

        if(obj.valid() && obj.is<std::string>())
        {
            path = obj.as<std::string>();
        }
        Log::CriticalLn(str, path);
    });

    math.set_function("angleAxis", [](sol::this_state ts, const CAngle& angle, const glm::vec3& vec) -> auto
    {
        return sol::make_object_userdata<glm::quat>(ts, glm::angleAxis(angle.asRadians(), vec));
    });

    sol::table onComponentAdd = CEngine::GetInstance()->Components.OnComponentAdd.GetScriptTable(State);
    sol::table onComponentInit = CEngine::GetInstance()->Components.OnComponentInit.GetScriptTable(State);
    sol::table onComponentPostInit = CEngine::GetInstance()->Components.OnComponentPostInit.GetScriptTable(State);

    components["onComponentAdd"] = onComponentAdd;
    components["onComponentInit"] = onComponentInit;
    components["onComponentPostInit"] = onComponentPostInit;

    components.set_function("prepareNew", [](sol::object typenmObj, sol::variadic_args va, sol::this_state ts) -> sol::table
    {
        sol::table ret = sol::state_view(ts).create_table();
        ret.set("root_type", "component");

        if(typenmObj.valid() && typenmObj.is<std::string>())
        {
            ret.set("type", typenmObj.as<std::string>());
        }

        std::string nm;
        for (size_t i = 0; i < va.size(); i++)
        {
            //0 - name
            //1 - func
            //2 - name
            //3 - func

            if(i % 2 == 0) //name
            {
                nm = va.get<std::string>(i);
            }
            else //func
            {
                sol::object funcobj = va.get<sol::object>(i);
                if(funcobj.valid() && funcobj.is<sol::function>() && !nm.empty())
                {
                    ret.set(nm, funcobj);
                }
            }
        }

        //constructor
        //destructor
        //init
        //postinit
        //update

        return ret;
    });

    components.set_function("registerNew", InvokeRegisterFunction<CComponent, "component">);
    entities.set_function("registerNew", InvokeRegisterFunction<CEntity, "entity">);

    components.set_function("create", [this](const std::string& type, sol::object instantInitObj) -> sol::object
    {
        bool instantInit = true;
        if(instantInitObj.valid() && instantInitObj.is<bool>())
        {
            instantInit = instantInitObj.as<bool>();
        }

        auto& comps = CEngine::GetInstance()->Components;
        if(!comps.IsComponentPresent(type))
        {
            comps.CreateComponent(type, instantInit);
            if(!comps.IsComponentPresent(type)) { return sol::lua_nil; }
        }

        return comps.GetComponent(type)->GetScriptTable(*this->State); //TODO problem, if not initted table might not be created properly
    });

    components.set_function("ispresent", [this](const std::string& type, sol::this_state ts) -> bool
    {
        return CEngine::GetInstance()->Components.IsComponentPresent(type);
    });

    /*auto angle = State->create_named_table("angle");
    angle.set_function("new", [](sol::variadic_args va) -> sol::table
    {
        float def = 0.0f;
        if(va.size() > 0)
        {
            def = va.get<float>(0); 
        }

        std::shared_ptr<CAngle> angl = std::make_shared<CAngle>();
        angl->setRadians(def);

        sol::table tabl = angl->GetScriptTable(va.lua_state());
        tabl["__obj"] = angl;

        return tabl;
    });*/

    //TODO more types! unify types with cvar.type

    auto angles = State->create_named_table("angles");
    angles.set_function("new", QuatVecNewFunction<CAngles>());

    auto quat = State->create_named_table("quat");
    quat.set_function("new", QuatVecNewFunction<glm::quat>());

    quat.set_function("from_euler", [](sol::object vec) -> sol::table //TODO temp func
    {
        glm::vec3 euler = ScriptUtils::FromObject<glm::vec3>(vec);
        glm::quat ret = glm::quat(euler);

        return ScriptUtils::ToObject<glm::quat>(ret, vec.lua_state());
    });

    quat.set_function("from_euler_degrees", [](sol::object vec) -> sol::table //TODO temp func
    {
        glm::vec3 euler = ScriptUtils::FromObject<glm::vec3>(vec);

        euler.x = glm::radians(euler.x);
        euler.y = glm::radians(euler.y);
        euler.z = glm::radians(euler.z);

        glm::quat ret = glm::quat(euler);

        return ScriptUtils::ToObject<glm::quat>(ret, vec.lua_state());
    });

    auto vec2 = State->create_named_table("vec2");
    vec2.set_function("new", QuatVecNewFunction<glm::vec2>());

    auto vec3 = State->create_named_table("vec3");
    vec3.set_function("new", QuatVecNewFunction<glm::vec3>());

    auto vec4 = State->create_named_table("vec4");
    vec4.set_function("new", QuatVecNewFunction<glm::vec4>());

    auto transform = State->create_named_table("transform");
    transform.set_function("new", [](sol::variadic_args va, sol::this_state ts) -> sol::table
    {
        auto trans = std::make_shared<CTransform>();
        sol::table ret = trans->GetScriptTable(ts);
        
        ret.raw_set("__obj", trans);
        ret.raw_set("__type", std::string("transform"));
        return ret;
    });

    hook.set_function("Set", [this](const std::string& str, sol::object arg1, sol::object arg2)
    {
        if(arg1.valid() && arg1.is<sol::function>())
        {
            AddHook(str, arg1.as<sol::function>());
        }
        else if((arg1.valid() && arg2.valid()) && arg1.is<std::string>() && arg2.is<sol::function>())
        {
            AddHook(str, arg2.as<sol::function>(), arg1.as<std::string>());
        }
    });

    hook.set_function("Call", [this](const std::string& str, sol::variadic_args va)
    {
        COMPONENT_CALL(CScriptingManager, CallHook(str, va));
    });

    hook.set_function("CallSingle", [this](const std::string& str, const std::string& hookName, sol::variadic_args va)
    {
        COMPONENT_CALL(CScriptingManager, CallSingleHook(str, hookName, va));
    });

    hook.set_function("Delete", [this](const std::string& str, sol::object func)
    {
        if(!func.valid()) { return; }

        if(func.is<std::string>())
        {
            DeleteHook(str, func.as<std::string>());
        }
        else if(func.is<sol::function>())
        {
            DeleteHook(str, func.as<sol::function>());
        }
    });

    auto engine_meta = State->create_table();

    engine_meta["__index"] = [this](sol::table table, std::string key, sol::this_state s) -> sol::object
    {
        bool ispr = CEngine::GetInstance()->Components.IsComponentPresent(key);
        if(ispr)
        {
            return CEngine::GetInstance()->Components.GetComponent(key)->GetScriptTable(s);
        }

        return sol::lua_nil;
        //return ComponentsCache.GatedDeleteOrGet(key, ispr);
    };

    /*engine_meta["__pairs"] = [this](sol::table self, sol::this_state s) -> sol::object
    {
        sol::state_view lua(s);
        sol::table merged = lua.create_table();

        for(const auto& [k, v] : CEngine::GetInstance()->Components.Items)
        {
            merged.set(k, ComponentsCache.Get(k));
        }

        return lua["pairs"](merged);
    };

    engine_meta["__len"] = [this](sol::table self, sol::this_state s) -> int
    {
        int len = 0;
        while (true)
        {
            std::string key = std::to_string(len + 1);
            bool exists_in_fields = CEngine::GetInstance()->Components.IsComponentPresent(key);
            bool exists_in_lua = self[static_cast<int>(len + 1)].valid();

            if (exists_in_fields || exists_in_lua)
            {
                ++len;
            }
            else
            {
                break;
            }
        }
        return len;
    };

    engine_meta["__ipairs"] = [this](sol::table self, sol::this_state s) -> std::tuple<sol::function, sol::object, sol::object>
    {
        sol::state_view lua(s);

        auto iter = [this](sol::table tbl, int index) -> std::tuple<sol::object, sol::object>
        {
            int next_index = index + 1;
            std::string key = std::to_string(next_index);

            bool exists = CEngine::GetInstance()->Components.IsComponentPresent(key);
            if (exists)
            {
                sol::object val = ComponentsCache.Get(key);
                return
                {
                    sol::make_object(tbl.lua_state(), next_index),
                    val
                };
            }

            sol::object lua_val = tbl[next_index];
            if (lua_val.valid())
            {
                return
                {
                    sol::make_object(tbl.lua_state(), next_index),
                    lua_val
                };
            }

            return { sol::lua_nil, sol::lua_nil };
        };

        return std::make_tuple(sol::make_object(lua, iter), sol::make_object(lua, self), sol::make_object(lua, 0));
    };*/

    engine[sol::metatable_key] = engine_meta;
    //COMPONENT_CALL(CScriptingManager, PopCurrentEngine());
}

void CScriptingEngine::AddHook(const std::string& hooksGroup, sol::function func, const std::string& hookName)
{
    Hooks.insert({ hooksGroup, { hookName, func } });
}

void CScriptingEngine::DeleteHook(const std::string& hooksGroup, sol::function func)
{
    std::erase_if(Hooks, [&hooksGroup, &func](const auto& kv)
    {
        return kv.first == hooksGroup && kv.second.second == func;
    });
}

void CScriptingEngine::DeleteHook(const std::string& hooksGroup, const std::string& hookName)
{
    std::erase_if(Hooks, [&hooksGroup, &hookName](const auto& kv)
    {
        return kv.first == hooksGroup && kv.second.first == hookName;
    });
}

void CScriptingEngine::ScriptString(const std::wstring& code)
{
    COMPONENT_CALL(CScriptingManager, PushCurrentEngine(this));

    CLuaPreProcessor Processor;
    Processor.ProcessText(code);

    if(!code.empty())
    {
        sol::protected_function_result result = State->safe_script(StringUtils::WstrToStr(Processor.CodeString), sol::script_pass_on_error);

        if (!result.valid())
        {
            sol::error err = result;
            Log::ErrInstance() << "Lua error: " << err.what() << Log::Endl;
        }
    }

    COMPONENT_CALL(CScriptingManager, PopCurrentEngine());
}

void CScriptingEngine::ScriptFile(const std::filesystem::path& path)
{
    std::wifstream file(path);
    if(!file.is_open())
    {
        COMPONENT_CALL(CLogger, Errln(L"Couldn't open file " + StringUtils::StrToWstr(path.string())));
        return;
    }

    std::wstringstream stream;

    stream << std::noskipws << file.rdbuf();
    std::wstring code = stream.str();

    file.close();
    ScriptString(code);
}

void CScriptingEngine::ScriptDirectory(const std::filesystem::path& dir)
{
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) { return; }
    std::vector<std::filesystem::path> lua_files;
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".lua")
        {
            lua_files.push_back(entry.path());
        }
    }

    for(auto& file : lua_files)
    {
        ScriptFile(file);
    }
}

void CScriptingEngine::Init()
{
    RegisterGeneralTypes();
    RegisterOtherTypes();

    V_Init();
    OnInit(this);
}

bool CScriptingEngine::IsTypeRegistered(size_t hash)
{
    return std::find_if(RegisteredTypes.begin(), RegisteredTypes.end(), [hash](const auto& kv) -> bool { return kv.second == hash; }) != RegisteredTypes.end();
}

bool CScriptingEngine::IsTypeRegistered(const std::string& _name)
{
    return RegisteredTypes.find(_name) != RegisteredTypes.end();
}

void CScriptingEngine::RegisterType(const std::string& _name, size_t hash)
{
    if(IsTypeRegistered(_name) || IsTypeRegistered(hash)) { return; }
    RegisteredTypes.emplace(_name, hash);
}

void CScriptingEngine::RegisterGeneralTypes()
{
    ScriptUtils::RegisterType<glm::vec1>(*State);
    ScriptUtils::RegisterType<glm::vec2>(*State);
    ScriptUtils::RegisterType<glm::vec3>(*State);
    ScriptUtils::RegisterType<glm::vec4>(*State);

    ScriptUtils::RegisterType<glm::mat2x2>(*State);
    ScriptUtils::RegisterType<glm::mat2x3>(*State);
    ScriptUtils::RegisterType<glm::mat2x4>(*State);

    ScriptUtils::RegisterType<glm::mat3x2>(*State);
    ScriptUtils::RegisterType<glm::mat3x3>(*State);
    ScriptUtils::RegisterType<glm::mat3x4>(*State);

    ScriptUtils::RegisterType<glm::mat3x2>(*State);
    ScriptUtils::RegisterType<glm::mat3x3>(*State);
    ScriptUtils::RegisterType<glm::mat3x4>(*State);

    ScriptUtils::RegisterType<glm::mat4x2>(*State);
    ScriptUtils::RegisterType<glm::mat4x3>(*State);
    ScriptUtils::RegisterType<glm::mat4x4>(*State);

    //ScriptUtils::RegisterType<glm::mat2>(*State);
    //ScriptUtils::RegisterType<glm::mat3>(*State);
    //ScriptUtils::RegisterType<glm::mat4>(*State);

    ScriptUtils::RegisterType<glm::quat>(*State);
    ScriptUtils::RegisterType<CAngle>(*State);
    ScriptUtils::RegisterType<CAngles>(*State);

    ScriptUtils::RegisterType<SDL_Event>(*State);
}
