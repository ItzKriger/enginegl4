#pragma once
#include "CSpacesManager.h"
#include "CCamerasManager.h"

class CScene
{
public:
    CSpacesManager SpacesManager;
    CCamerasManager CamerasManager;

    void Update();
    void Render(CRendererBase* renderer);
};

