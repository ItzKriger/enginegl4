#pragma once
#include <thread>
#include <chrono>

#include "CCallbackHandler.h"
#include "CScriptObject.h"

using CClock = std::chrono::high_resolution_clock;
using CTimePoint = std::chrono::time_point<CClock>;

#define TIME_EXPR(a) std::chrono::time_point_cast<CClock::duration>(a)

class CTime : public CScriptObject
{
public:
    void Init();
    bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;

    void NewTick();
    void EndTick();
    CTimePoint GetCurrent() const;

    unsigned int GetFPS() const;
    unsigned int GetPrevFPS() const;

    void ResetDeltaTime(double resetValue = 0.0);

    float GetDeltaTime() const;
    double GetDeltaTimePrecise() const;

    CCallbackHandler<void> OnNewTick;
    CCallbackHandler<void> OnEndTick;
private:
    CTimePoint m_LastTick;
    CTimePoint m_NextTick;
    CTimePoint m_CurrentTick;
    CTimePoint m_LastFpsCheck;

    unsigned int m_MaxFps = 0;

    double m_WaitTime = 0.0f;
    double m_DeltaTime = 0.0f;

    unsigned int m_PrevFPS = 0;
    unsigned int m_FPS = 0;
};
