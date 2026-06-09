#include "U_Scripting.h"

#include "CEngine.h"
#include "CScriptingManager.h"

sol::state& ScriptUtils::GetStateReference(lua_State* state)
{
    return CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetStateReference(state);
}

std::shared_ptr<sol::state> ScriptUtils::GetStateSharedPtr(lua_State* state)
{
    return CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>()->GetStateSharedPtr(state);
}

CWrapableBase* ScriptUtils::ObjectToWrapable(sol::object obj)
{
    if(obj.get_type() == sol::type::boolean)
    {
        return new CWrapable<bool>(obj.as<bool>());
    }
    else if(obj.get_type() == sol::type::number)
    {
        return new CWrapable<double>(obj.as<double>());
    }
    else if(obj.get_type() == sol::type::string)
    {
        return new CWrapable<std::string>(obj.as<std::string>());
    }
    return nullptr;
}

//num: 90
//string: text
//boolean: true
//table: { name = "frweger", age = 90 }

std::string ScriptUtils::ObjectToString(sol::object obj)
{
    if(obj.get_type() == sol::type::boolean)
    {
        return obj.as<bool>() ? "true" : "false";
    }
    else if(obj.get_type() == sol::type::number)
    {
        std::stringstream ss;
        ss << obj.as<double>();
        return ss.str();
    }
    else if(obj.get_type() == sol::type::string)
    {
        return obj.as<std::string>();
    }
    else if(obj.get_type() == sol::type::table)
    {
        std::string ret = "{ ";
        size_t added = 0;

        for(auto& kv : obj.as<sol::table>())
        {
            ret += ScriptUtils::ObjectToString(kv.first);
            ret += " = ";

            bool isValueStr = kv.second.get_type() == sol::type::string;

            if(isValueStr) { ret += "\""; }
            ret += ScriptUtils::ObjectToString(kv.second);
            if(isValueStr) { ret += "\""; }

            ret += ", ";
            added++;
        }

        //"{ " added 0 -> "{"
        //"{ age = 90, " added 1 -> "{ age = 90"

        ret.pop_back();
        if(added > 0)
        {            
            ret.pop_back();
            ret += " ";
        }

        ret += "}";
    }
    return {};
}

std::wstring ScriptUtils::ObjectToWstring(sol::object obj)
{
    if(obj.get_type() == sol::type::boolean)
    {
        return obj.as<bool>() ? L"true" : L"false";
    }
    else if(obj.get_type() == sol::type::number)
    {
        std::wstringstream ss;
        ss << obj.as<double>();
        return ss.str();
    }
    else if(obj.get_type() == sol::type::string)
    {
        return obj.as<std::wstring>();
    }
    else if(obj.get_type() == sol::type::table)
    {
        std::wstring ret = L"{ ";
        size_t added = 0;

        for(auto& kv : obj.as<sol::table>())
        {
            ret += ScriptUtils::ObjectToWstring(kv.first);
            ret += L" = ";

            bool isValueStr = kv.second.get_type() == sol::type::string;

            if(isValueStr) { ret += L"\""; }
            ret += ScriptUtils::ObjectToWstring(kv.second);
            if(isValueStr) { ret += L"\""; }

            ret += L", ";
            added++;
        }

        ret.pop_back();
        if(added > 0)
        {            
            ret.pop_back();
            ret += L" ";
        }

        ret += L"}";
    }
    return {};
}
