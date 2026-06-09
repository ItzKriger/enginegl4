#include "CThreadSleeper.h"

CThreadSleeper::CThreadSleeper()
{
    m_LastTick = m_NextTick = m_CurrentTick = CClock::now();
}

void CThreadSleeper::Start(unsigned int maxfps)
{
    m_MaxFps = maxfps;
    m_CurrentTick = CClock::now();

    std::chrono::duration<double> delta = m_CurrentTick - m_LastTick;
    m_DeltaTime = delta.count();

    m_LastTick = m_CurrentTick;

    if (m_MaxFps)
    {
        m_WaitTime = 1.0 / static_cast<double>(m_MaxFps);
        auto interval = std::chrono::duration<double>(m_WaitTime);

        m_NextTick = std::chrono::time_point_cast<CClock::duration>(m_CurrentTick + interval);
    }
    else
    {
        m_NextTick = m_CurrentTick;
    }
}

void CThreadSleeper::End(bool yield, bool adaptive_sleep, bool strict_sleep)
{
    auto now = CClock::now();

    if (m_MaxFps)
    {
        if(adaptive_sleep)
        {
            if (now < m_NextTick)
            {
                std::this_thread::sleep_until(m_NextTick);
            }
            else
            {
                //too late!
                m_NextTick = now;
            }
        }

        if(strict_sleep)
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(m_WaitTime));
        }

        while (CClock::now() < m_NextTick)
        {
            if(yield)
            {
                std::this_thread::yield();
            }
        }
    }

    now = CClock::now();
}
