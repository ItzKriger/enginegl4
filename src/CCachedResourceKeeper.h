#pragma once
#include "CResource.h"

#include <chrono>
#include <memory>

class CCachedResourceKeeper
{
public:
    CCachedResourceKeeper(std::shared_ptr<CResource> res);
    CCachedResourceKeeper(std::shared_ptr<CResource> res, std::chrono::high_resolution_clock::time_point expires);

    bool IsExpired() const;
    std::shared_ptr<CResource> Resource;
    
    bool Timed = false;
    std::chrono::high_resolution_clock::time_point Expires;
};
