#pragma once
#include "CTime.h"

class CThreadSleeper
{
public:
    CThreadSleeper();

    void Start(unsigned int maxfps);
    void End(bool yield, bool adaptive_sleep, bool strict_sleep);
private:
    CTimePoint m_LastTick;
    CTimePoint m_NextTick;
    CTimePoint m_CurrentTick;

    double m_WaitTime = 0.0f;
    double m_DeltaTime = 0.0f;

    unsigned int m_MaxFps = 0;
};
