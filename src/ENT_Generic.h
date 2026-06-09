#pragma once
#include "CEntity.h"
#include "CScriptClass.h"

class ENT_Generic : public CEntity, public CScriptClass<std::string>
{
public:
    using CScriptClass<std::string>::CScriptClass;

    std::string GetType() const override;
    void V_Init() override;
    void V_PostInit() override;
    void V_Update() override;

    void FullPack(CBufferWrapper& packet) override;
	void FullUnpack(CBufferWrapper& packet) override;

    DEFINE_SOL_USERTYPE();

    sol::table GetCurrentTable();
};
