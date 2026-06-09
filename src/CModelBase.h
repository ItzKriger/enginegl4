#pragma once
#include "CModelLoadInfo.h"
#include "CResource.h"

#include <filesystem>

class CModelLoadingContext : public CLoadingContext
{
public:
    CModelLoadingContext(CResource* _current_resource = nullptr, const std::filesystem::path& _path = {});
    //using CLoadingContext::CLoadingContext;
    CModelLoadInfo LoadInfo;
};

class CModelBase : public CResource
{
public:
    virtual ~CModelBase();
    virtual void V_Load(const CModelLoadInfo& loadinfo);
    virtual void V_Draw();

    void V_SetupLoadPipeline(CLoadPipeline& pipeline) override;
    std::shared_ptr<CLoadingContext> CreatePipelineContext(const std::filesystem::path& path) override;

    bool V_Load(const std::filesystem::path& path) override;
    std::vector<CModelLoadInfo::CBone> BonesData;
};
