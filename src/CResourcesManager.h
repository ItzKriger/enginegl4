#pragma once
#include "CComponent.h"
#include "CResource.h"
#include "CResourcesCache.h"
#include "CThreadPool.h"

#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <mutex>

class CResourcesManager : public CComponent
{
public:
    using IdentUnit = std::pair<std::string, std::string>; //Type, Name

    struct PairHash
    {
        size_t operator()(const std::pair<std::string, std::string>& p) const noexcept
        {
            size_t h1 = std::hash<std::string>{}(p.first);
            size_t h2 = std::hash<std::string>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };

    std::unordered_map<IdentUnit, std::weak_ptr<CResource>, PairHash> Resources;

    CResourcesManager();

    void V_Update() override;
    void RegisterResource(const std::shared_ptr<CResource>& res);
    void StopResource(CResource* res);
    void V_DeInit() override;

    std::shared_ptr<CResource> GetOrCreate(const std::string& type, const std::string& path, bool startPipeline = true);
    std::shared_ptr<CResource> LoadResource(const std::string& type, const std::string& path);

    bool IsResourceExist(const std::string& type, const std::string& name);
    std::shared_ptr<CResource> GetResource(const std::string& type, const std::string& name);

    template<typename T>
    std::shared_ptr<T> GetResourceTyped(const std::string& type, const std::string& name)
    {
        return std::dynamic_pointer_cast<T>(GetResource(type, name));
    }

    bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    CResourcesCache Cache;
    CThreadPool AsyncLoadingPool;

    CLoadPipelinesCache PipelinesCache;

    DEFINE_SOL_USERTYPE();
    DEFINE_COMPONENT();
    std::mutex RetrieveMutex;
};
