#pragma once
#include "CDrawableAnimatable.h"
#include "GL/glew.h"

#include <map>
#include <unordered_map>

class CGL430Drawable : public CDrawableAnimatable
{
public:
    CGL430Drawable();
    ~CGL430Drawable();

    bool IsReadyToDraw() const;
    void ReuploadBonesData();

    GLuint bonesSSBO = 0;
private:
    bool m_readyToDraw = false;
    bool m_needToUpdateBones = false;

    std::map<CTransformBase*, size_t> bones_offsets;
    std::map<CTransformBase*, CJoint*> trans_joint;
    std::vector<std::uint8_t> bones_data;
};
