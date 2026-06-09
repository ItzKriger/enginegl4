#include "CSpacesManager.h"

void CSpacesManager::AddSpace(std::shared_ptr<CBaseSpace> space)
{
    Spaces.push_back(space);
}

void CSpacesManager::Render(CRendererBase* renderer)
{
    for (auto& sp : Spaces) { sp->Render(renderer); }
}

void CSpacesManager::Update()
{
    for(auto& space : Spaces)
    {
        space->Update();
    }
}

int CSpacesManager::GetSpaceID(CBaseSpace* _space) const
{
    auto it = std::find_if(Spaces.begin(), Spaces.end(), [_space](auto& sp) -> bool { return sp.get() == _space; });
    if(it != Spaces.end())
    {
        return std::distance(Spaces.begin(), it);
    }
    return -1;
}

