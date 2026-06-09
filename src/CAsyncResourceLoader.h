#pragma once
#include "CResourceTicket.h"

#include <vector>
#include <thread>

class CAsyncResourceLoader
{
public:

private:
    size_t m_maxThreads = 4;
    std::vector<std::thread> m_threadPool;
};
