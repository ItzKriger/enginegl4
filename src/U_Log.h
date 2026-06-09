#pragma once
#include <string>
#include <conio.h>

#include "U_String.h"

namespace Log
{
    void Out(const std::wstring& str);
	void Outln(const std::wstring& str);
	void Err(const std::wstring& err);
	void Errln(const std::wstring& err);

    void Out(const std::string& str);
	void Outln(const std::string& str);
	void Err(const std::string& err);
	void Errln(const std::string& err);

    void Critical(const std::wstring& msg, const std::string& filename = "critical.txt");
    void CriticalLn(const std::wstring& msg, const std::string& filename = "critical.txt");

    static class CLoggerEndl {} Endl;
    static class CLoggerPauser {} Pause;

    class CLoggerProxy
    {
    public:
        template<typename T>
        CLoggerProxy& operator<<(const T& val)
        {
            if constexpr (std::is_same_v<T, char>)
            {
                Out(std::wstring((wchar_t)val, 1));
            }
            else if constexpr (std::is_same_v<T, wchar_t>)
            {
                Out(std::wstring(val, 1));
            }
            else if constexpr (std::is_same_v<T, CLoggerEndl>)
            {
                Out(L"\n");
            }
            else if constexpr (std::is_same_v<T, CLoggerPauser>)
            {
                _getch();
            }
            else
            {
                Out(StringUtils::ToStr<std::wstring>(val));
            }
            return *this;
        }
    };

    class CLoggerErrProxy
    {
    public:
        template<typename T>
        CLoggerErrProxy& operator<<(const T& val)
        {
            if constexpr (std::is_same_v<T, char>)
            {
                Err(std::wstring((wchar_t)val, 1));
            }
            else if constexpr (std::is_same_v<T, wchar_t>)
            {
                Err(std::wstring(val, 1));
            }
            else if constexpr (std::is_same_v<T, CLoggerEndl>)
            {
                Err(L"\n");
            }
            else if constexpr (std::is_same_v<T, CLoggerPauser>)
            {
                _getch();
            }
            else
            {
                Err(StringUtils::ToStr<std::wstring>(val));
            }
            return *this;
        }
    };

    class CCriticalLoggerProxy
    {
    public:
        CCriticalLoggerProxy();
        CCriticalLoggerProxy(const std::string& flnm);

        template<typename T>
        CCriticalLoggerProxy& operator<<(const T& val)
        {
            if constexpr (std::is_same_v<T, char>)
            {
                Critical(std::wstring((wchar_t)val, 1), fileName);
            }
            else if constexpr (std::is_same_v<T, wchar_t>)
            {
                Critical(std::wstring(val, 1), fileName);
            }
            else if constexpr (std::is_same_v<T, CLoggerEndl>)
            {
                Critical(L"\n", fileName);
            }
            else if constexpr (std::is_same_v<T, CLoggerPauser>)
            {
                _getch();
            }
            else
            {
                Critical(StringUtils::ToStr<std::wstring>(val), fileName);
            }
            return *this;
        }

        std::string fileName = "critical.txt";
    };

    CLoggerProxy Instance();
    CLoggerErrProxy ErrInstance();
    CCriticalLoggerProxy CriticalInstance(const std::string& fileName = "critical.txt");
}
