#include "CBaseSpace.h"

void CBaseSpace::AddDrawable(std::shared_ptr<CDrawable> dr)
{
    Drawables.push_back(dr);
}

void CBaseSpace::Render(CRendererBase* renderer)
{
    for (auto& d : Drawables) { d->Draw(renderer); }
}

bool CBaseSpace::V_BaseScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    return true;
}

void CBaseSpace::Update()
{
    for(auto& drawable : Drawables)
    {
        drawable->Update();
    }

    V_Update();
    OnUpdate(this);
}

void CBaseSpace::V_Update() {}
