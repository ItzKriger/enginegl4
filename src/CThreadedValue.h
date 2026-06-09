#pragma once
#include <mutex>

template<typename T>
class CThreadedValue
{
public:
	CThreadedValue(const T& val)
	{
		m_Value = val;
	}

	T Get() const
	{
		T ret{};
		{
			std::lock_guard lock(m_Mutex);
			ret = m_Value;
		}
		return ret;
	}
	
	void Set(const T& newval)
	{
		std::lock_guard lock(m_Mutex);
		m_Value = newval;
	}
private:
	T m_Value{};
	mutable std::mutex m_Mutex;
};
