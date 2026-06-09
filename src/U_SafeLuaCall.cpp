#include "U_SafeLuaCall.h"
#include "U_Log.h"

void OutError(const std::wstring& err)
{
    Log::ErrInstance() << err;
}

void OutError(const std::string& err)
{
    Log::ErrInstance() << err;
}
