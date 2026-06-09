#pragma once
#include "CEngine.h"
#include "CScriptingManager.h"

#include "U_Scripting.h"
#include "U_FixedString.h"
#include <string_view>

template<typename Base, FixedString RootType = FixedString("component")>
int InvokeRegisterFunction(sol::table tabl, sol::this_state ts)
{
    std::string_view roottype_sv(RootType);

    sol::object typenm = tabl["type"];
    sol::object root_typenm = tabl["root_type"];

    if(!typenm.valid() || !typenm.is<std::string>()) { return 1; }
    std::string _strtypenm = typenm.as<std::string>();

    std::string strtypenm = _strtypenm;
    std::transform(strtypenm.begin(), strtypenm.end(), strtypenm.begin(), ::tolower);

    if(!root_typenm.valid() || !root_typenm.is<std::string>()) { return 2; }
    std::string _strroottypenm = root_typenm.as<std::string>();

    std::string strroottypenm = _strroottypenm;
    std::transform(strroottypenm.begin(), strroottypenm.end(), strroottypenm.begin(), ::tolower);

    if(strroottypenm != roottype_sv) { return 3; }
    if(strtypenm.empty()) { return 4; }

    auto man = CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>();
    auto engine = man->GetEngine(ts);

    bool isRegistered = man->ScriptClassesFactory.IsClassRegistered<Base>(strtypenm);
    if(isRegistered) { return 5; }

    std::map<std::string, sol::function> _functions;
    for(auto& kv : tabl)
    {
        if(kv.second.valid() && kv.second.is<sol::function>())
        {
            _functions.insert({ kv.first.as<std::string>(), kv.second.as<sol::function>() });
        }
    }

    auto State = ScriptUtils::GetStateSharedPtr(ts);
    man->ScriptClassesFactory.RegisterClassInfo<Base>({ strtypenm, State, _functions, engine });
    return 0;
}
