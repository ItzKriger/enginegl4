#include "CLogger.h"
#include "CEngine.h"
#include "CTerminal.h"
#include "CFunction.h"
#include "CStartupArgsManager.h"
#include "CConVarManager.h"
#include "CWrapable.h"

#include <future>
#include <chrono>
#include <thread>

std::wofstream* CLogger::m_OpenStream(std::string path)
{
	if (streams.find(path) == streams.end())
	{
		std::wofstream wf;
		wf.open(path, std::ios::app | std::ios::out);

		if (!wf.is_open()) { return nullptr; }
		streams[path] = std::move(wf);
	}

	return &streams[path];
}

bool CLogger::V_Init()
{
	std::string outPath, errPath, slowModeDelay;

	bool notasync = false;
	bool forceasync = false;
	bool nofiles = false;

	COMPONENT_CALL_GET(outPath, CStartupArgsManager, GetArgumentValue("-out"));
	COMPONENT_CALL_GET(errPath, CStartupArgsManager, GetArgumentValue("-err"));
	COMPONENT_CALL_GET(slowModeDelay, CStartupArgsManager, GetArgumentValue("-slowloggerdelay"));
	COMPONENT_CALL_GET(notasync, CStartupArgsManager, IsArgumentSet("-synclogger"));
	COMPONENT_CALL_GET(forceasync, CStartupArgsManager, IsArgumentSet("-asynclogger"));
	COMPONENT_CALL_GET(nofiles, CStartupArgsManager, IsArgumentSet("-loggernofiles"));
	COMPONENT_CALL_GET(m_slowMode, CStartupArgsManager, IsArgumentSet("-slowlogger"));

	if(m_slowMode && !slowModeDelay.empty())
	{
		m_slowModeDelay = StringUtils::FromStr<unsigned int>(slowModeDelay);
	}

	if(notasync) { m_isAsync = false; }
	if(forceasync) { m_isAsync = true; }

	if(!nofiles)
	{
		outstream = m_OpenStream(outPath);
		errstream = m_OpenStream(errPath);
	}

	if (m_isAsync)
	{
		outqueue.reserve(128);
		errqueue.reserve(128);

		m_thread = std::make_unique<std::thread>(&CLogger::m_ThreadedWorker, this);
	}
	return true;
}

void CLogger::V_PostInit()
{
	auto mgr = CEngine::GetInstance()->Components.GetComponentTyped<CConVarManager>();

	mgr->AddConVar("time.limit.logger.maxfps", new CWrapable<unsigned int>(20));
	mgr->AddConVar("time.limit.logger.yield", new CWrapable<bool>(true));
	mgr->AddConVar("time.limit.logger.adaptive_sleep", new CWrapable<bool>(true));
	mgr->AddConVar("time.limit.logger.strict_sleep", new CWrapable<bool>(false));
}

void CLogger::V_Update()
{
	if(CClock::now() - lastsync >= std::chrono::duration<float>(sync_datas_interval))
	{
		auto mgr = CEngine::GetInstance()->Components.GetComponentTyped<CConVarManager>();
		async_data data;

		data.adaptive_sleep = mgr->GetConVarValue<unsigned int>("time.limit.logger.maxfps");
		data.yield = mgr->GetConVarValue<bool>("time.limit.logger.yield");
		data.adaptive_sleep = mgr->GetConVarValue<bool>("time.limit.logger.adaptive_sleep");
		data.strict_sleep = mgr->GetConVarValue<bool>("time.limit.logger.strict_sleep");

		{
			std::lock_guard _lock(async_data_mutex);
			threaded_async = data;
		}
		lastsync = CClock::now();
	}
}

CLogger::~CLogger()
{
	if (m_isAsync)
	{
		AskedToStop.store(true, std::memory_order_relaxed);
		while (!Stopped)
		{
			std::this_thread::yield();
		}

		if(m_thread->joinable())
		{
			m_thread->join();
		}
	}

	if (outstream && outstream->is_open()) { outstream->flush(); outstream->close(); }
	if (errstream && errstream->is_open()) { errstream->flush(); errstream->close(); }
}

void CLogger::m_flushOut(const std::wstring& str)
{
	if (outstream && outstream->is_open())
	{
		(*outstream) << str;
		outstream->flush();
	}

	if(!CEngine::GetInstance()->GetStopFlag())
	{
		COMPONENT_CALL(CTerminal, Print(str));
	}
	m_Wait();
}

void CLogger::m_flushErr(const std::wstring& err)
{
	if (errstream && errstream->is_open())
	{
		(*errstream) << err;
		errstream->flush();
	}

	if(!CEngine::GetInstance()->GetStopFlag())
	{
		COMPONENT_CALL(CTerminal, Error(err));
	}
	m_Wait();
}

void CLogger::Out(const std::wstring& str)
{
	if (m_isAsync)
	{
		std::lock_guard lock(mutex);
		outqueue.push_back(str);
	}
	else
	{
		m_flushOut(str);
	}

	OnOut(str);
}

void CLogger::Err(const std::wstring& err)
{
	if (m_isAsync)
	{
		std::lock_guard lock(mutex);
		errqueue.push_back(err);
	}
	else
	{
		m_flushErr(err);
	}

	OnErr(err);
}

void CLogger::Outln(const std::wstring& str)
{
	Out(str + L"\n");
}

void CLogger::Errln(const std::wstring& err)
{
	Err(err + L"\n");
}

void CLogger::SetAsync()
{
	m_isAsync = true;
}

bool CLogger::IsAsync()
{
	return m_isAsync;
}

void CLogger::m_ThreadedWorker()
{
	std::vector<std::wstring> outcopy, errcopy;

	while (true)
	{
		//mgr->AddConVar("time.limit.logger.maxfps", new CWrapable<unsigned int>(20));
		//mgr->AddConVar("time.limit.logger.yield", new CWrapable<bool>(true));
		//mgr->AddConVar("time.limit.logger.adaptive_sleep", new CWrapable<bool>(true));
		//mgr->AddConVar("time.limit.logger.strict_sleep", new CWrapable<bool>(false));

		async_data asdata;
		{
			std::lock_guard _lock(async_data_mutex);
			asdata = threaded_async;
		}

		Sleeper.Start(asdata.maxfps);

		{
			std::lock_guard lock(mutex);

			if(!outqueue.empty()) { outcopy.swap(outqueue); }
			if(!errqueue.empty()) { errcopy.swap(errqueue); }
		}

		for (const auto& s : outcopy) { m_flushOut(s); }
		for (const auto& e : errcopy) { m_flushErr(e); }

		outcopy.clear();
		errcopy.clear();

		if (AskedToStop.load(std::memory_order_acquire)) 
		{
			Stopped.store(true, std::memory_order_release);
			break;
		}

		Sleeper.End(asdata.yield, asdata.adaptive_sleep, asdata.strict_sleep);
	}
}

bool CLogger::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
	table.set_function("out", [this](const std::wstring& text) { Out(text); });
	table.set_function("outln", [this](const std::wstring& text) { Outln(text); });
	table.set_function("err", [this](const std::wstring& err) { Err(err); });
	table.set_function("errln", [this](const std::wstring& err) { Errln(err); });

	return true;
}

void CLogger::m_Wait() const
{
	if(!m_slowMode) { return; }
	std::this_thread::sleep_for(std::chrono::milliseconds(m_slowModeDelay));
}

sol::object CLogger::V_GetScriptUserType(sol::state_view state)
{
	return sol::make_object(state, this);
}

LINK_COMPONENT_TO_CLASS(CLogger, logger);