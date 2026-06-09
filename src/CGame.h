#pragma once
#include "CComponent.h"
#include "CWorldsManager.h"
#include "CPhysicsEngine.h"

class CGame : public CComponent
{
public:
    CGame();

    void V_Update() override;
    void V_DeInit() override;
    bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    DEFINE_SOL_USERTYPE();

    CWorldsManager Worlds;
    std::unique_ptr<CPhysicsEngine> PhysicsEngine;

    DEFINE_COMPONENT();
};
