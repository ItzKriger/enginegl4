#pragma once
#include "CComponent.h"
#include "CResource.h"

#include "CScriptFieldsManager.h"
#include "U_Scripting.h"
#include "CTransform.h"

class CTestRender : public CComponent
{
public:
    bool V_Init() override;
    void V_Update() override;

    void Draw();

    bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    DEFINE_COMPONENT();
    std::shared_ptr<CResource> Model;
    CTransform ModelTransform;
    CTransform CameraTransform;

    float CameraFOV = 110.0f;
    CAngles CameraAngles;

    bool Forward = false, Back = false, Right = false, Left = false, Shift = false, Ctrl = false;
};
