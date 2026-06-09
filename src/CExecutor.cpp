#include "CExecutor.h"
#include "CEngine.h"
#include "CLogger.h"
#include "U_String.h"
#include "CCommandProcessor.h"
#include "CStartupArgsManager.h"
#include "CConVarManager.h"

#include "U_Files.h"

#include <sstream>
#include <locale>
#include <fstream>

bool CExecutor::V_Init()
{
    bool NoAutoExec = false;
    COMPONENT_CALL_GET(NoAutoExec, CStartupArgsManager, IsArgumentSet("-noautoexec"));

    ExecuteFile((FileUtils::get_executable_path() / "resources" / "cfg" / "e_aliases.cfg").string()); //as in "early_config"
    ExecuteFile((FileUtils::get_executable_path() / "resources" / "cfg" / "e_config.cfg").string()); //as in "early_config"
    if(!NoAutoExec)
    {
        ExecuteFile((FileUtils::get_executable_path() / "resources" / "cfg" / "init.cfg").string());
        //ExecuteFile(".\\resources\\cfg\\init.cfg"); //TODO not the right way (hardcoded)
    }
    return true;
}

void CExecutor::V_PostInit()
{
    bool NoAutoExec = false;
    COMPONENT_CALL_GET(NoAutoExec, CStartupArgsManager, IsArgumentSet("-noautoexec"));

    ExecuteFile((FileUtils::get_executable_path() / "resources" / "cfg" / "aliases.cfg").string());
    ExecuteFile((FileUtils::get_executable_path() / "resources" / "cfg" / "config.cfg").string());
    if(!NoAutoExec)
    {
        ExecuteFile((FileUtils::get_executable_path() / "resources" / "cfg" / "postinit.cfg").string());
        //ExecuteFile(".\\resources\\cfg\\postinit.cfg"); //TODO not the right way (hardcoded)
    }
}

void CExecutor::ExecuteFile(const std::string& file)
{
    if(!FileUtils::is_within_root(FileUtils::get_executable_path() / "resources" / "cfg", file))
    {
        Log::ErrInstance() << "Can't execute config \"" << file << "\". Unacceptable path\n";
        return;
    }

    std::wifstream filestream;
    filestream.open(file);

    if(!filestream.is_open())
    {
        COMPONENT_CALL(CLogger, Err(L"Can't open file \"" + StringUtils::StrToWstr(file) + L"\"\n"));
        return;
    }

    std::wstringstream wss;
    wss << std::noskipws << filestream.rdbuf();

    std::wstring m_cfg = wss.str();
    filestream.close();

    StringUtils::replace<std::wstring>(m_cfg, L"\r\n", L"\n");

    if (StringUtils::IsEmpty(m_cfg)) { return; }
    std::vector<std::wstring> cmds;

    StringUtils::split_str(m_cfg, L'\n', cmds);
    if (cmds.empty()) { return; }

    for (std::wstring& cmd : cmds)
    {
        COMPONENT_CALL(CCommandProcessor, ProcessCommandLine(cmd, CCommandSender::Self));
    }
}

void CExecutor::V_DeInit()
{
    WriteAliases((FileUtils::get_executable_path() / "resources" / "cfg" / "aliases.cfg").string());
    WriteConfig((FileUtils::get_executable_path() / "resources" / "cfg" / "config.cfg").string());
}

void CExecutor::WriteConfig(const std::string& file)
{
    auto cvarmgr = CEngine::GetInstance()->Components.GetComponentTyped<CConVarManager>();
    if(!cvarmgr) { return; }

    std::wofstream f(file);
    if(!f.is_open())
    {
        return;
    }

    std::wstringstream buf;
    cvarmgr->Root.Trace([&buf](const CConVarManager::CVarNode* nod)
    {
        if(nod->Value.Wrapable)
        {
            buf << L"\"" << StringUtils::StrToWstr(nod->GetPath()) << L"\" \"" << nod->Value.Wrapable->GetValueStr() << L"\"\n";
        }
    });

    f << std::noskipws << buf.str();
    f.close();
}

void CExecutor::WriteAliases(const std::string& file)
{
    auto cmdproc = CEngine::GetInstance()->Components.GetComponentTyped<CCommandProcessor>();
    if(!cmdproc) { return; }

    std::wofstream f(file);
    if(!f.is_open())
    {
        return;
    }

    std::wstringstream buf;
    for(auto& kv : cmdproc->Aliases)
    {
        std::wstring cmd = StringUtils::StrToWstr(kv.first);
        cmd = CCommandProcessor::ProcessSpecialChars(cmd);

        std::wstring cmdarg = kv.second;
        cmdarg = CCommandProcessor::ProcessSpecialChars(cmdarg);

        buf << L"alias \"" << cmd << L"\" \"" << cmdarg << L"\"\n";
    }

    f << std::noskipws << buf.str();
    f.close();
}

LINK_COMPONENT_TO_CLASS(CExecutor, executor);