#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include <functional>
#include <vector>
#include <unordered_map>
#include <future>
#include <semaphore>

#include "CCallbackHandler.h"

#define LINK_RESOURCE_TO_CLASS(_class, name) std::string _class::GetType() const { return #name; }; CFactoryInitter<_class, CResource> __resource_initter_ ## name = CFactoryInitter<_class, CResource>(#name, std::function<void(CFactoryInitter<_class, CResource>*)>([](CFactoryInitter<_class, CResource>* tter) -> void { tter->m_factory = &CEngine::GetInstance()->ResourcesFactory; }));
#define DEFINE_RESOURCE() std::string GetType() const override

class CResource;
class CLoadingContext
{
public:
    CLoadingContext(CResource* _current_resource = nullptr, const std::filesystem::path& _path = {});
    virtual ~CLoadingContext() = default;

    CResource* CurrentResource = nullptr;
    std::filesystem::path LoadPath;

    void LoadRequired();
    std::vector<std::shared_ptr<CResource>> RequiredResources;

    std::shared_future<void> CurrentAsyncFuture;
};

using CPipelineFunction = std::function<bool(std::shared_ptr<CLoadingContext>)>;

class CPipelineNode
{
public:
    CPipelineFunction Function;
    bool RunAsync = false;
};

using CLoadPipeline = std::vector<CPipelineNode>;
using CLoadCallback = CCallbackHandler<void, CLoadPipeline&>;

class CLoadPipelinesCache
{
public:
    std::unordered_map<std::string, CLoadPipeline> Cache;
    std::unordered_map<std::string, CLoadCallback> CacheCallbacks;
};

class CResource : public std::enable_shared_from_this<CResource>
{
public:
    enum class CLoadingStatus
    {
        None,
        Waiting,
        Sync,
        WaitingAsync,
        StartedAsync,
        CompletedAsync,
        WaitingForRequired,
        Done
    };

    CResource();
    virtual ~CResource();
    virtual std::string GetType() const = 0;

    void SetupLoadPipeline(CLoadPipeline& pipeline);
    void V_BaseSetupLoadPipeline(CLoadPipeline& pipeline);

    virtual void V_SetupLoadPipeline(CLoadPipeline& pipeline);

    void StartPipeline(const std::filesystem::path& path);
    void ProcessPipeline();
    void PipelineWaitForRequired();
    void PipelineCheckRequired();

    void Wait();
    bool IsReady() const;

    virtual std::shared_ptr<CLoadingContext> CreatePipelineContext(const std::filesystem::path& path);

    CLoadPipeline& GetLoadPipeline();
    CLoadCallback& GetLoadPipelineSetupCallback();

    bool Load(const std::filesystem::path& path);
    virtual bool V_Load(const std::filesystem::path& path);
    virtual bool V_SynchronisedLoad();

    CLoadingStatus LoadingStatus = CLoadingStatus::None;

    std::string Name;
    std::vector<CPipelineNode> LoadPipeline;

    std::shared_ptr<CLoadingContext> LoadingContext;
    int LoadingFunctionIndex = -1;

    CCallbackHandler<void, CResource*> OnDoneLoading;

    std::shared_future<void> WaitingFuture;
};
