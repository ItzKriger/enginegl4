#pragma once

template<typename Class>
class CSingleton
{
public:
	CSingleton()
	{
		m_Instance = (Class*)this;
	}

	~CSingleton()
	{
		m_Instance = nullptr;
	}

	static Class* GetInstance()
	{
		return m_Instance;
	}
private:
	static inline Class* m_Instance = nullptr;
};
