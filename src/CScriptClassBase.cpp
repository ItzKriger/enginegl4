#include "CScriptClassBase.h"

CScriptClassBase::CScriptClassBase(const std::map<std::string, sol::function>& _Functions, std::shared_ptr<sol::state> _State) : Functions(_Functions), State(_State) {}