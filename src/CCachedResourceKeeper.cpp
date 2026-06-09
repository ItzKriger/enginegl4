#include "CCachedResourceKeeper.h"

CCachedResourceKeeper::CCachedResourceKeeper(std::shared_ptr<CResource> res) : Resource(res), Timed(false) {}
CCachedResourceKeeper::CCachedResourceKeeper(std::shared_ptr<CResource> res, std::chrono::high_resolution_clock::time_point expires) : Resource(res), Timed(true), Expires(expires)  {}

bool CCachedResourceKeeper::IsExpired() const
{
    if(!Timed) { return false; }
    return std::chrono::high_resolution_clock::now() >= Expires;
}
