#pragma once
#include <fstream>
#include <vector>
#include <filesystem>
#include <memory>
#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <shared_mutex>

#include "CComponent.h"
#include "CCallbackHandler.h"
#include "CThreadedValue.h"
#include "U_String.h"
#include "CSingleton.h"
#include "CThreadSleeper.h"
#include "CTime.h"

class CLogger : public CComponent
{
public:
	bool V_Init() override;
	void V_PostInit() override;

	void V_Update() override;

	~CLogger();

	void Out(const std::wstring& str);
	void Outln(const std::wstring& str);
	void Err(const std::wstring& err);
	void Errln(const std::wstring& err);

	sol::object V_GetScriptUserType(sol::state_view state) override;
	bool V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table) override;
	
	CCallbackHandler<void, const std::wstring&> OnOut;
	CCallbackHandler<void, const std::wstring&> OnErr;

	template<typename T>
	CLogger& operator<<(const T& val)
	{
		if constexpr (std::is_same_v<T, char>)
		{
			Out(std::wstring((wchar_t)val, 1));
		}
		else if constexpr (std::is_same_v<T, wchar_t>)
		{
			Out(std::wstring(val, 1));
		}
		else
		{
			Out(StringUtils::ToStr<std::wstring>(val));
		}
		return *this;
	}

	void SetAsync();
	bool IsAsync();

	DEFINE_COMPONENT();
private:
	std::atomic_bool AskedToStop = false;
	std::atomic_bool Stopped = false;

	bool m_isAsync = true;
	bool m_slowMode = false;

	unsigned int m_slowModeDelay = 200;
	
	std::wofstream* m_OpenStream(std::string path);

	void m_Wait() const;
	void m_flushOut(const std::wstring& str);
	void m_flushErr(const std::wstring& err);
	void m_ThreadedWorker();

	CThreadSleeper Sleeper;

	std::mutex mutex;
	std::vector<std::wstring> outqueue, errqueue;
	std::unique_ptr<std::thread> m_thread;

	std::map<std::filesystem::path, std::wofstream> streams;

	std::wofstream* outstream = nullptr, *errstream = nullptr;

	struct async_data
	{
		unsigned int maxfps = 20;
		bool yield = true;
		bool adaptive_sleep = true;
		bool strict_sleep = false;
	};

	async_data threaded_async;

	std::mutex async_data_mutex;

	float sync_datas_interval = 0.1f;
	CTimePoint lastsync;
};
