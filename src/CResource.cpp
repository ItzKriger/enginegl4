#include "CResource.h"
#include "CEngine.h"
#include "CResourcesManager.h"

#include <filesystem>

CLoadingContext::CLoadingContext(CResource* _current_resource, const std::filesystem::path& _path) : CurrentResource(_current_resource), LoadPath(_path)
{
    //Log::Instance() << "CLoadingContext::CLoadingContext\n"; LOGLOG
}

CResource::CResource()
{
}

CResource::~CResource()
{
}

bool CResource::Load(const std::filesystem::path& path)
{
    std::filesystem::path pp(path);
    Name = pp.stem().string();

    bool loaded = V_Load(path);
    if(!loaded) { Name.clear(); return false; }

    COMPONENT_CALL(CResourcesManager, RegisterResource(shared_from_this()));
    return true;
}

bool CResource::V_Load(const std::filesystem::path& path)
{
    return true;
}

bool CResource::V_SynchronisedLoad()
{
    return true;
}

bool CResource::IsReady() const
{
    return LoadingStatus == CResource::CLoadingStatus::Done;
}

void CResource::SetupLoadPipeline(CLoadPipeline& pipeline)
{
    V_BaseSetupLoadPipeline(pipeline);
    V_SetupLoadPipeline(pipeline);
    GetLoadPipelineSetupCallback()(pipeline);
}

void CResource::V_SetupLoadPipeline(CLoadPipeline& pipeline)
{

}

void CResource::V_BaseSetupLoadPipeline(CLoadPipeline& pipeline)
{

}

std::shared_ptr<CLoadingContext> CResource::CreatePipelineContext(const std::filesystem::path& path)
{
    return std::make_shared<CLoadingContext>(this, path);
}

void CResource::StartPipeline(const std::filesystem::path& path)
{
    LoadingContext = CreatePipelineContext(path);
    LoadingFunctionIndex = 0;
    LoadingStatus = CResource::CLoadingStatus::Waiting;
}

void CResource::PipelineWaitForRequired()
{
    LoadingStatus = CResource::CLoadingStatus::WaitingForRequired;
}

void CResource::PipelineCheckRequired()
{
    if(LoadingContext->RequiredResources.empty()) { return; }

    //Log::Instance() << "For " << GetType() << " " << Name << ":\n";

    bool allOk = true;
    for(auto res : LoadingContext->RequiredResources)
    {
        //Log::Instance() << "Checking required " << res->GetType() << " " << res->Name << " status " << (int)res->LoadingStatus << Log::Endl; 
        if(res->LoadingStatus != CResource::CLoadingStatus::Done)
        {
            allOk = false;
            break;
        }
    }

    if(!allOk && LoadingStatus != CResource::CLoadingStatus::WaitingForRequired)
    {
        LoadingStatus = CResource::CLoadingStatus::WaitingForRequired;
    }
    else if(allOk)
    {
        LoadingStatus = CResource::CLoadingStatus::Waiting;
    }
    
    //Log::Instance() << "PipelineCheckRequired set status to " << (int)LoadingStatus << " with allok " << allOk << Log::Endl;
}

void CResource::ProcessPipeline()
{
    if(LoadingStatus == CResource::CLoadingStatus::CompletedAsync && LoadingFunctionIndex < 0 && !LoadingContext)
    {
        LoadingStatus = CResource::CLoadingStatus::Done;
        return;
    }

    if(!LoadingContext) { return; }
    PipelineCheckRequired();

    if(LoadingStatus == CResource::CLoadingStatus::WaitingForRequired
        || LoadingStatus == CResource::CLoadingStatus::WaitingAsync
        || LoadingStatus == CResource::CLoadingStatus::StartedAsync) { return; }
    if(LoadingFunctionIndex < 0) { return; }

    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto& pipeline = GetLoadPipeline();

    for(; LoadingFunctionIndex < pipeline.size(); )
    {
        auto& func = pipeline.at(LoadingFunctionIndex);

        if(!func.RunAsync)
        {
            if (LoadingContext->CurrentAsyncFuture.valid())
            {
                LoadingContext->CurrentAsyncFuture.wait();
                LoadingContext->CurrentAsyncFuture = {};
            }

            LoadingStatus = CResource::CLoadingStatus::Sync;
            func.Function(LoadingContext);
            LoadingFunctionIndex++;

            PipelineCheckRequired();
            if(LoadingStatus == CResource::CLoadingStatus::WaitingForRequired
                || LoadingStatus == CResource::CLoadingStatus::WaitingAsync
                || LoadingStatus == CResource::CLoadingStatus::StartedAsync) { return; }
        }
        else
        {
            LoadingStatus = CResource::CLoadingStatus::WaitingAsync;

            auto promise = std::make_shared<std::promise<void>>();
            LoadingContext->CurrentAsyncFuture = promise->get_future().share();

            auto waitingpromise = std::make_shared<std::promise<void>>();
            WaitingFuture = waitingpromise->get_future().share();

            auto thisResource = shared_from_this();

            resman->AsyncLoadingPool.AddTask([func, this, promise, waitingpromise, thisResource]()
            {
                this->LoadingStatus = CResource::CLoadingStatus::StartedAsync;
                func.Function(this->LoadingContext);

                if(this->LoadingStatus != CResource::CLoadingStatus::Done)
                {
                    this->LoadingStatus = CResource::CLoadingStatus::CompletedAsync;
                }

                promise->set_value();
                waitingpromise->set_value();
            });

            LoadingFunctionIndex++;
            break;
        }
    }

    if
    (
        LoadingFunctionIndex >= pipeline.size() - 1 &&
        LoadingStatus != CResource::CLoadingStatus::WaitingForRequired &&
        LoadingStatus != CResource::CLoadingStatus::WaitingAsync &&
        LoadingStatus != CResource::CLoadingStatus::StartedAsync
    )
    {
        LoadingFunctionIndex = -1;
        LoadingStatus = CResource::CLoadingStatus::Done;
        LoadingContext.reset();
        OnDoneLoading(this);
    }
}

//this entire function doesn't make any sense
//we should update all the resources so they'll all load as
//requirements for others
//
//also wait can be called from other threads which is invalid behaviour
void CResource::Wait()
{
    if(!LoadingContext) { Log::Instance() << "Can't wait\n"; return; }
    while(LoadingStatus != CResource::CLoadingStatus::Done)
    {
        if(LoadingStatus == CResource::CLoadingStatus::WaitingAsync ||
            LoadingStatus == CResource::CLoadingStatus::StartedAsync)
        {
            if (WaitingFuture.valid())
            {
                WaitingFuture.wait();
                WaitingFuture = {};
            }
        }
        else
        {
            for(auto res : LoadingContext->RequiredResources)
            {
                if(res->LoadingStatus != CResource::CLoadingStatus::Done)
                {
                    res->ProcessPipeline();
                    break;
                }
            }
            ProcessPipeline();
            //TODO don't do active waiting
        }
    }
    Log::Instance() << "Wait end\n";
    WaitingFuture = {};
}

CLoadPipeline& CResource::GetLoadPipeline()
{
    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto& cache = resman->PipelinesCache.Cache;

    auto it = cache.find(GetType());

    if(it == cache.end())
    {
        CLoadPipeline pipeline;

        SetupLoadPipeline(pipeline);
        return cache.emplace(GetType(), std::move(pipeline)).first->second;
    }
    return it->second;
}

CLoadCallback& CResource::GetLoadPipelineSetupCallback()
{
    auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
    auto& cache = resman->PipelinesCache.CacheCallbacks;

    auto it = cache.find(GetType());

    if(it == cache.end())
    {
        return cache.emplace(GetType(), CLoadCallback()).first->second;
    }
    return it->second;
}
