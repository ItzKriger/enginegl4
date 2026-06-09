#pragma once
#include <string>
#include <map>
#include <unordered_map>

#include "CCachedResourceKeeper.h"

class CResourcesCache
{
public:
    std::unordered_map<std::string, CCachedResourceKeeper> Resources;

    void Add(std::shared_ptr<CResource> res);
    void Add(std::shared_ptr<CResource> res, std::chrono::high_resolution_clock::time_point expires);

    void Remove(CResource* res);

    bool IsResourceCached(CResource* res);
    void RemoveExpired();

    void Clear();
};
