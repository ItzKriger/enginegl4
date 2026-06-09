#pragma once
#include <vector>
#include <memory>

#include "CBaseSpace.h"

class CSpacesManager
{
public:
    std::vector<std::shared_ptr<CBaseSpace>> Spaces;

    void AddSpace(std::shared_ptr<CBaseSpace> space);
    void Render(CRendererBase* renderer);
    void Update();

    int GetSpaceID(CBaseSpace* _space) const;
};
