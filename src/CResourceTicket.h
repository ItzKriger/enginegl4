#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "CResource.h"

class CResourceTicket
{
public:
    CResourceTicket() = default;
    CResourceTicket(std::shared_ptr<CResource> res);

    bool IsReady() const;
    void Join();
    std::shared_ptr<CResource> Get();
private:
    enum class CState { Loading, Synchronising, Ready, Failed };

    std::atomic<CState> state { CState::Loading };
    std::shared_ptr<CResource> resource;
    std::mutex mtx;
    std::condition_variable cv;

    friend class CResourcesManager;
};
