#pragma once
#include "CResource.h"
#include "CScriptClass.h"

class CGenericResource : public CResource, public CScriptClass<std::string>
{
public:
    using CScriptClass<std::string>::CScriptClass;
    std::string GetType() const override;
};
