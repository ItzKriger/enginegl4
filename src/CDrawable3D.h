#pragma once
#include "CTransform.h"
#include "CDrawable.h"

class CDrawable3D : public CDrawable
{
public:
    CDrawable3D();

    DEFINE_SOL_USERTYPE();
    CTransform Transform;
};
