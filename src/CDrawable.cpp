#include "CDrawable.h"

bool CDrawable::V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    //table.set_function("draw", [this]() { return this->Draw(); });
    //table.set_function("draw", [this]() { return this->Draw(); });
    return true;
}

void CDrawable::Update()
{
    V_Update();
    OnUpdate(this);
}

void CDrawable::V_Update() {}
LINK_SOL_USERTYPE(CDrawable);