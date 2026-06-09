#include "CResourcesManager.h"
#include "CEngine.h"
#include "CLogger.h"
#include "CGenericResource.h"
#include "U_ScriptClasses.h"

#include <algorithm>
#include <stdexcept>
#include <filesystem>

#include "U_Log.h"

CResourcesManager::CResourcesManager()
{
    AsyncLoadingPool.SetThreadsAmount(4); //TODO not hardcoded
}

void CResourcesManager::RegisterResource(const std::shared_ptr<CResource>& res)
{
    if(!res || res->Name.empty()) { COMPONENT_CALL(CLogger, Err(L"Tried to register invalid resource (null/empty name)\n")); return; }
    if(std::find_if(Resources.begin(), Resources.end(), [&res](auto& kv) -> bool
        {
            return kv.second.lock() == res;
        }) != Resources.end()) { throw std::runtime_error("Tried to register resource that is already registered\n"); }

    if(Resources.find({ res->GetType(), res->Name }) != Resources.end()) { COMPONENT_CALL(CLogger, Err(L"Tried to register resource with occupied name\n")); return; }

    Resources.emplace(std::make_pair(res->GetType(), res->Name), res);
    //Log::Instance() << "Registered resource " << res->Name << " type " << res->GetType() << "\n"; LOGLOG
}

void CResourcesManager::V_DeInit()
{
    AsyncLoadingPool.Stop();

    Cache.Clear();
    Resources.clear();
}

void CResourcesManager::StopResource(CResource* res)
{
    if(!res || res->Name.empty()) { COMPONENT_CALL(CLogger, Err(L"Tried to stop invalid resource (null/empty name)\n")); return; }
    auto it = std::find_if(Resources.begin(), Resources.end(), [&res](auto& kv) -> bool
        {
            return kv.second.lock().get() == res && kv.first.second == res->Name && kv.first.first == res->GetType();
        });

    if(it != Resources.end())
    {
        auto ptr = it->second.lock();
        if(ptr)
        {
            Log::Instance() << "Stopped resource " << ptr->Name << "\n";
        }
        
        Resources.erase(it);
    }
}

void CResourcesManager::V_Update()
{
    Cache.RemoveExpired();
    for(auto& _res : Resources)
    {
        if(_res.second.expired()) { continue; }

        auto res = _res.second.lock();
        if(res->LoadingStatus != CResource::CLoadingStatus::None && res->LoadingStatus != CResource::CLoadingStatus::Done)
        {
            res->ProcessPipeline();
        }
    }
}

std::shared_ptr<CResource> CResourcesManager::GetOrCreate(const std::string& type, const std::string& path, bool startPipeline)
{
    std::lock_guard _lock(RetrieveMutex);

    std::shared_ptr<CResource> res;
    bool okget = false;

    std::filesystem::path pp(path);
    try
    {
        res = GetResource(type, pp.stem().string());

        if(res && res->GetType() == type)
        {
            okget = true;
        }
    }
    catch(...) {}

    if(okget)
    {
        return res;
    }

    res = CEngine::GetInstance()->ResourcesFactory.create(type);
    if(!res) { return nullptr; }

    res->Name = pp.stem().string();
    RegisterResource(res);

    if(startPipeline)
    {
        res->StartPipeline(pp);
    }
    return res;
}

std::shared_ptr<CResource> CResourcesManager::LoadResource(const std::string& type, const std::string& path)
{
    std::lock_guard _lock(RetrieveMutex);

    std::shared_ptr<CResource> res;
    bool okget = false;

    std::filesystem::path pp(path);
    try
    {
        res = GetResource(type, pp.stem().string());

        if(res && res->GetType() == type)
        {
            okget = true;
        }
    }
    catch(...) {}

    if(okget)
    {
        return res;
    }
    res = CEngine::GetInstance()->ResourcesFactory.create(type);
    //if(!res) { throw std::runtime_error("No resources with such type: " + type); }
    if(!res) { return nullptr; }

    bool ok = res->Load(path);

    //if(!ok) { throw std::runtime_error("Can't load resource at \"" + path + "\" with type " + type); }
    if(!ok)
    {
        StopResource(res.get());
        return nullptr;
    }
    return res;
}

bool CResourcesManager::IsResourceExist(const std::string& type, const std::string& name)
{
    return std::find_if(Resources.begin(), Resources.end(), [&type, &name](auto& kv) -> bool
        {
            return kv.first.second == name && kv.first.first == type;
        }) != Resources.end();
}

std::shared_ptr<CResource> CResourcesManager::GetResource(const std::string& type, const std::string& name)
{
    //Log::Instance() << "Trying to GetResource \"" << name << "\"\n"; LOGLOG
    auto it = std::find_if(Resources.begin(), Resources.end(), [&type, &name](auto& kv) -> bool
        {
            return kv.first.second == name && kv.first.first == type;
        });

    //if(it == Resources.end()) { throw std::runtime_error("No such named resource"); }
    if(it == Resources.end()) { return nullptr; }

    auto shared = it->second.lock();
    if (!shared)
    {
        //Log::Instance() << "Resource " << name << " expired, removing from manager\n"; LOGLOG
        Resources.erase(it);
        return nullptr;
    }
    //Log::Instance() << "Returned existing\n"; LOGLOG
    return shared;
}

bool CResourcesManager::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    table.set_function("registerNew", InvokeRegisterFunction<CResource, "resource">); //generic or resource
    return true;
}

LINK_SOL_USERTYPE(CResourcesManager);
LINK_COMPONENT_TO_CLASS(CResourcesManager, resourcesmanager);