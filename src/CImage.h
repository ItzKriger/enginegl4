#pragma once
#include "CResource.h"
#include "glm/glm.hpp"

class CImage : public CResource
{
public:
    CImage() = default;
    ~CImage();

    DEFINE_RESOURCE();
    bool V_Load(const std::filesystem::path& path);
    void V_SetupLoadPipeline(CLoadPipeline& pipeline) override;

    glm::ivec2 GetSize() const;
    int GetChannels() const;

    unsigned char* GetRawData();
private:
    int m_Width = 0, m_Height, m_Channels = 0;
    unsigned char* m_Data = nullptr;
};
