#pragma once
#include "CMaterialBase.h"
#include "CColor.h"
#include "GL/glew.h"

class CGL430MaterialLoadingContext : public CLoadingContext
{
public:
    using CLoadingContext::CLoadingContext;
    std::filesystem::path DiffusePath;
};

class CGL430Material : public CMaterialBase
{
public:
    CGL430Material();
    ~CGL430Material();

    bool V_Load(const std::filesystem::path& path) override;
    void V_SetupLoadPipeline(CLoadPipeline& pipeline) override;
    std::shared_ptr<CLoadingContext> CreatePipelineContext(const std::filesystem::path& path) override;
    
    DEFINE_RESOURCE();

    GLuint64 DiffuseTextureHandle = 0;
    CColor DiffuseColor;
    float Shininess = 32.0f;

    GLuint DiffuseTextureNativeHandle = 0;
};
