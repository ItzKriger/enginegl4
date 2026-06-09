#include "CResourcesCache.h"
#include <algorithm>

#include "U_Log.h"

void CResourcesCache::Clear()
{
    Resources.clear();
}

void CResourcesCache::Add(std::shared_ptr<CResource> res)
{
    if(!res || IsResourceCached(res.get()) || res->Name.empty()) { return; }
    Resources.insert({ res->Name, CCachedResourceKeeper(res) });

    Log::Instance() << "Cached resource (non-expiring) " << res->Name << " type " << res->GetType() << "\n";
}

void CResourcesCache::Add(std::shared_ptr<CResource> res, std::chrono::high_resolution_clock::time_point expires)
{
    if(!res || res->Name.empty()) { return; }

    if(IsResourceCached(res.get()))
    {
        auto it = std::find_if(Resources.begin(), Resources.end(), [&res](auto& kv) -> bool
        {
            return kv.second.Resource == res && kv.first == res->Name;
        });

        it->second.Expires = expires;
        it->second.Timed = true;
        Log::Instance() << "Set expiration time for resource " << res->Name << " type " << res->GetType() << "\n";
    }
    else
    {
        Resources.insert({ res->Name, CCachedResourceKeeper(res, expires) });
        Log::Instance() << "Cached resource (expiring) " << res->Name << " type " << res->GetType() << "\n";
    }
}

void CResourcesCache::Remove(CResource* res)
{
    std::erase_if(Resources, [&res](auto& kv) -> bool
    {
        if(kv.second.Resource.get() == res)
        {
            Log::Instance() << "Resource " << kv.second.Resource->Name << " was removed\n";
        }
        return kv.second.Resource.get() == res;
    });
}

void CResourcesCache::RemoveExpired()
{
    std::erase_if(Resources, [](auto& kv) -> bool
    {
        if(kv.second.IsExpired())
        {
            Log::Instance() << "Resource " << kv.second.Resource->Name << " was expired\n";
        }
        return kv.second.IsExpired();
    });
}

bool CResourcesCache::IsResourceCached(CResource* res)
{
    if(!res) { return false; }

    return std::find_if(Resources.begin(), Resources.end(), [&res](auto& kv) -> bool
        {
            return kv.second.Resource.get() == res && kv.first == res->Name;
        }) != Resources.end();
}
