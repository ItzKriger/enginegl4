#include "CTime.h"
#include "CConVarManager.h"
#include "CEngine.h"
#include "CLogger.h"
#include "CScriptFieldsManager.h"
#include "U_Scripting.h"

void CTime::Init()
{
    m_LastTick = m_NextTick = m_CurrentTick = m_LastFpsCheck = CClock::now();
}

void CTime::NewTick()
{
    m_CurrentTick = CClock::now();

    std::chrono::duration<double> delta = m_CurrentTick - m_LastTick;
    m_DeltaTime = delta.count();

    m_LastTick = m_CurrentTick;
    COMPONENT_CALL_GET(m_MaxFps, CConVarManager, GetConVarValue<unsigned int>("fps.max"));

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
    OnNewTick();
}

void CTime::EndTick()
{
    auto now = CClock::now();
    auto mgr = CEngine::GetInstance()->Components.GetComponentTyped<CConVarManager>();

    bool yield = mgr->GetConVarValue<bool>("time.limit.yield");
    bool adaptive_sleep = mgr->GetConVarValue<bool>("time.limit.adaptive_sleep");
    bool strict_sleep = mgr->GetConVarValue<bool>("time.limit.strict_sleep");

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

    if(now - m_LastFpsCheck >= std::chrono::seconds(1))
    {
        bool dump = false;
        COMPONENT_CALL_GET(dump, CConVarManager, GetConVarValue<bool>("fps.dump"));

        if(dump)
        {
            COMPONENT_CALL(CLogger, Outln(L"FPS is " + StringUtils::ToStr<std::wstring>(m_FPS) + L" with deltatime " + StringUtils::ToStr<std::wstring>(m_DeltaTime) + 
                                            L" and maxfps is " + StringUtils::ToStr<std::wstring>(m_MaxFps)));
        }

        m_PrevFPS = m_FPS;
        m_FPS = 0;

        m_LastFpsCheck = now;
    }

    m_FPS++;
    OnEndTick();
}

unsigned int CTime::GetFPS() const
{
    return m_FPS;
}

unsigned int CTime::GetPrevFPS() const
{
    return m_PrevFPS;
}

float CTime::GetDeltaTime() const
{
    return static_cast<float>(m_DeltaTime);
}

double CTime::GetDeltaTimePrecise() const
{
    return m_DeltaTime;
}

void CTime::ResetDeltaTime(double resetValue)
{
    m_DeltaTime = resetValue;
}

CTimePoint CTime::GetCurrent() const
{
    return CClock::now();
}

bool CTime::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
    sol::table onNewTick = OnNewTick.GetScriptTable(state);
    sol::table onEndTick = OnEndTick.GetScriptTable(state);

    table["onNewTick"] = onNewTick;
    table["onEndTick"] = onEndTick;

    static std::unordered_map<std::string, CTimePoint> measures;

    table.set_function("start_measure", [](const std::string& name)
    {
        measures.emplace(name, CEngine::GetInstance()->Time.GetCurrent());
    });

    table.set_function("stop_measure", [](const std::string& name) -> float
    {
        auto it = measures.find(name);
        if(it == measures.end()) { return 0.0f; }

        auto now = CEngine::GetInstance()->Time.GetCurrent();
        auto past = it->second;

        measures.erase(it);
        return std::chrono::duration_cast<std::chrono::duration<float>>(now - past).count();
    });

    auto fieldsMan = CScriptFieldsManager::CreateFieldsManager(table);

    auto scDeltaTime = std::make_unique<CFunctionalScriptField>([this](sol::state_view st) -> sol::object
    { return ScriptUtils::ToObject<float>(GetDeltaTime(), st); });

    auto scDeltaTimePrecise = std::make_unique<CFunctionalScriptField>([this](sol::state_view st) -> sol::object
    { return ScriptUtils::ToObject<double>(GetDeltaTimePrecise(), st); });

    auto scFps = std::make_unique<CFunctionalScriptField>([this](sol::state_view st) -> sol::object
    { return ScriptUtils::ToObject<unsigned int>(GetPrevFPS(), st); });

    auto scCurrentFps = std::make_unique<CFunctionalScriptField>([this](sol::state_view st) -> sol::object
    { return ScriptUtils::ToObject<unsigned int>(GetFPS(), st); });

    fieldsMan->AddField("deltaTime", std::move(scDeltaTime));
    fieldsMan->AddField("deltaTimePrecise", std::move(scDeltaTimePrecise));
    fieldsMan->AddField("fps", std::move(scFps));
    fieldsMan->AddField("fpsCurrent", std::move(scCurrentFps));

    fieldsMan->CreateMetaTable(table);
    return true;
}
