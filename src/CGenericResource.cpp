#include "CGenericResource.h"

std::string CGenericResource::GetType() const
{
    return GetLuaKey();
}
