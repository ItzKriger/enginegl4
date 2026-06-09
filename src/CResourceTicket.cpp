#include "CResourceTicket.h"

bool CResourceTicket::IsReady() const
{
    return resource && state.load() == CState::Ready;
}

void CResourceTicket::Join()
{
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]{ return state.load() != CState::Loading; });
}

std::shared_ptr<CResource> CResourceTicket::Get()
{
    if(state != CState::Ready)
    {
        Join();
    }
    return resource;
}
