#pragma once
#include "CComponent.h"
#include "CCallbackHandler.h"

#include <memory>
#include "SDL3/SDL.h"

class CImgui : public CComponent
{
public:
    DEFINE_COMPONENT();

    bool V_Init() override;
    void V_DeInit() override;
    bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    std::unique_ptr<CSmartCallback> DrawDeleter;
    std::unique_ptr<CSmartCallback> EventDeleter;

    CCallbackHandler<void> OnDraw;
};
