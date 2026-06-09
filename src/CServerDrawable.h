#pragma once
#include "CDrawableAnimatable.h"

#include <map>
#include <unordered_map>

class CServerDrawable : public CDrawableAnimatable
{
public:
    bool IsReadyToDraw() const;
private:
    bool m_readyToDraw = false;
    bool m_needToUpdateBones = false;

    std::map<CTransformBase*, size_t> bones_offsets;
    std::map<CTransformBase*, CJoint*> trans_joint;
    std::vector<std::uint8_t> bones_data;
};
