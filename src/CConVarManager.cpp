#include "CConVarManager.h"
#include "CEngine.h"
#include "CScriptingManager.h"
#include "ILuaWrapableBase.h"
#include "CWrapable.h"
#include "CAngle.h"

#include <stdexcept>
#include <functional>
#include <map>
#include <vector>
#include <string>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>

template<typename T>
std::function<CWrapableBase*(sol::object)> CVarCreateFunction()
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

std::vector<std::pair<std::string, std::function<CWrapableBase*(sol::object)>>> CVarTypesTable = //TODO more types! unify types with QuatVecNewFunction (CScriptingEngine)
{
    { "bool", CVarCreateFunction<bool>() },
    { "int", CVarCreateFunction<long long>() },
    { "uint", CVarCreateFunction<unsigned long long>() },
    { "float", CVarCreateFunction<float>() },
    { "double", CVarCreateFunction<double>() },
    { "angle", CVarCreateFunction<CAngle>() },
    { "angles", CVarCreateFunction<CAngles>() },
    { "quat", CVarCreateFunction<glm::quat>() },
    { "vec2", CVarCreateFunction<glm::vec2>() },
    { "vec3", CVarCreateFunction<glm::vec3>() },
    { "vec4", CVarCreateFunction<glm::vec4>() },
    { "string", CVarCreateFunction<std::wstring>() },
    { "string8", CVarCreateFunction<std::string>() },
    { "wstring", CVarCreateFunction<std::wstring>() }
};

std::map<std::string, std::pair<int, std::function<CWrapableBase*(sol::object)>>> CVarTypesTabl = 
{
    { "int", { 0, CVarCreateFunction<long long>() } },
    { "uint", { 1, CVarCreateFunction<unsigned long long>() } },
    { "float", { 2, CVarCreateFunction<float>() } },
    { "double", { 3, CVarCreateFunction<double>() } },
    { "angle", { 4, CVarCreateFunction<CAngle>() } },
    { "angles", { 5, CVarCreateFunction<CAngles>() } },
    { "quat", { 6, CVarCreateFunction<glm::quat>() } },
    { "vec2", { 7, CVarCreateFunction<glm::vec2>() } },
    { "vec3", { 8, CVarCreateFunction<glm::vec3>() } },
    { "vec4", { 9, CVarCreateFunction<glm::vec4>() } },
    { "string", { 10, CVarCreateFunction<std::wstring>() } },
    { "string8", { 11, CVarCreateFunction<std::string>() } },
    { "wstring", { 12, CVarCreateFunction<std::wstring>() } },
};

bool CConVarManager::V_Init()
{
    return true;
}

bool CConVarManager::IsConVarExist(const std::string& path)
{
    return GetConVar(path);
}

CConVarManager::CVarRawPtr CConVarManager::GetConVar(const std::string& path)
{
    //std::shared_lock guard(m_Mutex); //TODO is required?

    CVarNode* nod = Root.GetNode(path);
    if(!nod) { return nullptr; }

    return nod->Value.Wrapable ? nod->Value.Wrapable.get() : nullptr;
}

CConVarManager::CVarNode* CConVarManager::AddConVar(const std::string& path, CConVarManager::CVarRawPtr ptr)
{
    if(!ptr) { return nullptr; }

    //std::lock_guard guard(m_Mutex); //TODO is required?

    CVarNode* nod = Root.CreateNode(path);
    if(nod->Value.Wrapable) { throw std::runtime_error("Convar already exist"); }

    nod->Value.Wrapable = CVarPtr(ptr);
    return nod;
}

bool CConVarManager::DeleteConVar(const std::string& path)
{
    //std::lock_guard guard(m_Mutex); //TODO is required?

    CVarNode* nod = Root.GetNode(path);
    if(!nod) { return false; }

    Root.DeleteNode(nod);
    return true;
}

CConVarManager::CVarNode* CConVarManager::GetRealConVarNode(const std::string& path)
{
    //std::shared_lock guard(m_Mutex); //TODO is required?
    return Root.GetNode(path);
}

CConVarManager::RealCVar& CConVarManager::GetRealConVar(const std::string& path)
{
    //std::shared_lock guard(m_Mutex); //TODO is required?

    CVarNode* nod = Root.GetNode(path);
    if(!nod) { throw std::runtime_error("No such convar"); }

    return nod->Value;
}

bool CConVarManager::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    if(state->get<sol::object>("cvar") == sol::lua_nil)
    {
        sol::table cvar = state->create_named_table("cvar");
        sol::table type = cvar.create_named("type");

        size_t index = 0;
        for(auto& kv : CVarTypesTable)
        {
            type.set(kv.first, index);
            index++;
        }
    }

    table.set_function("getcvar", [this](const std::string& path, sol::this_state ts) -> sol::object
    {
        if(!IsConVarExist(path)) { return sol::lua_nil; }
        auto engine = CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetEngine(ts);

        auto cvar = GetRealConVarNode(path);
        sol::table ret = cvar->Value.GetScriptTable(ts);

        //sol::table ret = engine->ConvarCache.GatedDeleteOrGet(cvar ? cvar->GetPath() : "", cvar);
        return ret;
    });

    table.set_function("createcvar", [this](const std::string& path, int type, sol::object obj, sol::this_state ts) -> sol::object
    {
        auto cvar = GetConVar(path);
        if(cvar) { return sol::lua_nil; }

        if(type < 0 || type >= CVarTypesTable.size()) { return sol::lua_nil; }

        auto node = AddConVar(path, CVarTypesTable.at(type).second(obj));
        auto engine = CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetEngine(ts);

        //return engine->ConvarCache.Get(node->GetPath());
        return node->Value.GetScriptTable(ts);
    });

    table.set_function("deletecvar", [this](const std::string& path) -> int
    {
        return DeleteConVar(path) ? 0 : 1;
    });
    return true;
}

LINK_SOL_USERTYPE(CConVarManager);
LINK_COMPONENT_TO_CLASS(CConVarManager, convarmanager);