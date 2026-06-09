#pragma once
#include "CComponent.h"
#include "CScriptClass.h"

class CGenericComponent : public CComponent, public CScriptClass<std::string>
{
public:
    using CScriptClass<std::string>::CScriptClass;
    ~CGenericComponent();

    DEFINE_SOL_USERTYPE();

    bool V_Init() override;
    void V_PostInit() override;
    void V_DeInit() override;

    void V_Update() override;
    std::string GetType() const override;

    sol::table GetCurrentTable();
    sol::object GetCurrentUserType();
};
