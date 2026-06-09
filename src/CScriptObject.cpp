#include "CScriptObject.h"

bool CScriptObject::m_internalInit(std::shared_ptr<sol::state> state, sol::table table)
{
    auto succ_vec = OnScriptInit.CallWithReturns(state, table);
    return std::find(succ_vec.begin(), succ_vec.end(), false) == succ_vec.end();
}
