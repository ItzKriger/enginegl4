#include "CDrawable3D.h"

CDrawable3D::CDrawable3D()
{
    OnScriptInit += [this](std::shared_ptr<sol::state> state, sol::table table) -> bool
    {
        table["transform"] = Transform.GetScriptTable(state);
        return true;
    };
}

LINK_SOL_USERTYPE(CDrawable3D);