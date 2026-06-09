#include "CGame.h"
#include "CEngine.h"
#include "CScriptingManager.h"
#include "CGameScriptingEngine.h"
#include "U_General.h"
#include "U_ShortAPI.h"

CGame::CGame()
{
    
}

void CGame::V_DeInit()
{
    PhysicsEngine.reset();
    //TODO delete worlds
}

void CGame::V_Update()
{
    if(PhysicsEngine && !IsConnectedClient())
    {
        auto deltaTime = CEngine::GetInstance()->Time.GetDeltaTime();
        PhysicsEngine->Trace([deltaTime](CPhysicsWorld* world) { world->Step(deltaTime); });
    }
    Worlds.Update();
}

bool CGame::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    table["worlds"] = Worlds.GetScriptTable(state);

    SetFieldsManager(table, [this](FieldsManPtr fieldsMan)
    {
        fieldsMan->AddField("physics", std::make_unique<CFunctionalScriptField>([this](sol::state_view ts) -> sol::object
        { return PhysicsEngine ? PhysicsEngine->GetScriptTable(ts) : sol::lua_nil; }));
    });
    return true;
}

LINK_SOL_USERTYPE(CGame);
LINK_COMPONENT_TO_CLASS(CGame, game);