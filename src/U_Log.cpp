#include "U_Log.h"
#include "CEngine.h"
#include "CLogger.h"

#include <fstream>

Log::CLoggerProxy Log::Instance() { return CLoggerProxy(); }
Log::CLoggerErrProxy Log::ErrInstance() { return CLoggerErrProxy(); }

Log::CCriticalLoggerProxy::CCriticalLoggerProxy() {}
Log::CCriticalLoggerProxy::CCriticalLoggerProxy(const std::string& flnm) : fileName(flnm) {}

Log::CCriticalLoggerProxy Log::CriticalInstance(const std::string& fileName)
{
    return CCriticalLoggerProxy(fileName);
}

void Log::Out(const std::wstring& str)
{
    COMPONENT_CALL(CLogger, Out(str));
}

void Log::Outln(const std::wstring& str)
{
    COMPONENT_CALL(CLogger, Outln(str));
}
void Log::Err(const std::wstring& err)
{
    COMPONENT_CALL(CLogger, Err(err));
}

void Log::Errln(const std::wstring& err)
{
    COMPONENT_CALL(CLogger, Errln(err));
}

void Log::Out(const std::string& str)
{
    Out(StringUtils::StrToWstr(str));
}

void Log::Outln(const std::string& str)
{
    Outln(StringUtils::StrToWstr(str));
}

void Log::Err(const std::string& err)
{
    Err(StringUtils::StrToWstr(err));
}

void Log::Errln(const std::string& err)
{
    Errln(StringUtils::StrToWstr(err));
}

void Log::Critical(const std::wstring& msg, const std::string& filename)
{
    std::wofstream file(filename, std::ios::app | std::ios::out);
    file << msg << std::flush;
    file.close();
}

void Log::CriticalLn(const std::wstring& msg, const std::string& filename)
{
    Critical(msg + L"\n", filename);
}
