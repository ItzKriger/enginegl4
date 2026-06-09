#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <functional>

class CThreadPool
{
public:
    using Unit = std::function<void()>;

    CThreadPool(size_t numThreads = 0) : stop(false)
    {
        for (size_t i = 0; i < numThreads; ++i)
        {
            workers.emplace_back(&CThreadPool::m_threadedFunction, this);
        };
    }

    ~CThreadPool()
    {
        Stop();
    }

    void Stop()
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();

        for (auto& worker : workers)
        {
            worker.join();
        }
        workers.clear();
    }

    void SetThreadsAmount(size_t newAmount)
    {
        int difference = static_cast<int>(newAmount) - static_cast<int>(workers.size());
        if(difference <= 0) { return; }

        for (int i = 0; i < difference; ++i)
        {
            workers.emplace_back(&CThreadPool::m_threadedFunction, this);
        };
    }

    void AddTask(Unit _func)
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push(std::move(_func));
        }
        condition.notify_one();
    }

    void m_threadedFunction()
    {
        for(;;)
        {
            Unit task;

            {
                std::unique_lock<std::mutex> lock(queueMutex);

                condition.wait(lock, [this]
                {
                    return stop || !tasks.empty();
                });

                if (stop && tasks.empty())
                {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }

            task();
        }
    }

    std::vector<std::thread> workers;
    std::queue<Unit> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};
