#include "CStartupArgsManager.h"
#include "CEngine.h"
#include "CCommandProcessor.h"
#include "U_String.h"
#include "CLogger.h"

CStartupArgsManager::CStartupArgsManager()
{
    SetupDefaults();
    Parse();
}

void CStartupArgsManager::V_PostInit()
{
    ProcessCommands();
}

void CStartupArgsManager::SetupDefaults()
{
    Arguments["-out"] = "log.txt";
	Arguments["-err"] = "log.txt";
	Arguments["-renderer"] = "gl430";
}

void CStartupArgsManager::ProcessCommands()
{
    for(auto& s : Commands)
    {
        COMPONENT_CALL(CCommandProcessor, ProcessCommandLine(StringUtils::StrToWstr(s), CCommandSender::Self));
    }
}

bool CStartupArgsManager::IsArgumentSet(const std::string& arg)
{
    return Arguments.find(arg) != Arguments.end();
}

std::string CStartupArgsManager::GetArgumentValue(const std::string& arg)
{
    return IsArgumentSet(arg) ? Arguments[arg] : "";
}

void CStartupArgsManager::Parse() //TODO rewrite with usage of functions
{
    auto splitted = ArgumentsPreprocess::ParseArguments(CEngine::GetInstance()->CommandLine);
	std::stack<std::uint8_t> conditionstack; //0 - none; 1 - true; 2 - false

	bool addingcmd = false;
	std::string name, value;

	for (size_t i = 0U; i < splitted.size(); i++)
	{
		std::string& s = splitted[i];

		if (s[0] == '/' || s[0] == '-' || s[0] == '+')
		{
			if (conditionstack.empty() || conditionstack.top() != 2)
			{
				if (addingcmd)
				{
					if (!value.empty())
					{
						value.pop_back();
						Commands.push_back(value);

						value.clear();
						name.clear();

						continue;
					}
				}
				else
				{
					if (!name.empty())
					{
						if (!value.empty())
						{
							value.pop_back();
						}

						Arguments[name] = value;

						name.clear();
						value.clear();
					}
				}

				if (s[0] == '+')
				{
					addingcmd = true;

					std::string toadd = s;
					toadd.erase(toadd.begin());

					value += toadd + ' ';

					continue;
				}
				else
				{
					addingcmd = false;
				}
			}

			if (s == "-if" || s == "/if")
			{
				if (i + 1 < splitted.size())
				{
					i++;

					s = splitted[i];
					auto it = Arguments.find(s);

					std::string firstarg = s;
					std::string nos = s;
					auto it2 = Arguments.end();
					auto it3 = Arguments.end();

					if (nos[0] == '/' || nos[0] == '-')
					{
						nos.erase(nos.begin());
						it2 = Arguments.find(nos);
					}

					if (it != Arguments.end() || it2 != Arguments.end())
					{
						firstarg = (it != Arguments.end() ? (*it).second : (*it2).second);
					}

					it3 = std::find_if(Arguments.begin(), Arguments.end(), [&s, &firstarg](auto& kv) -> bool
						{
							std::string sd = kv.first;
							if (!sd.empty())
							{
								sd.erase(sd.begin());
								if (sd == s)
								{
									firstarg = kv.second;
									return true;
								}
							}
							return false;
						});

					std::string op, op_arg;

					i++;
					if (i < splitted.size()) { op = splitted[i]; }

					i++;
					if (i < splitted.size()) { op_arg = splitted[i]; }

					if (op == "==" || op == ">" || op == "<" || op == ">=" || op == "<=" || op == "!=")
					{
						bool st = ArgumentsPreprocess::Compare(firstarg, op, op_arg);
						conditionstack.push(st ? 1 : 2);
					}
					else
					{
						bool found = (it != Arguments.end() || it2 != Arguments.end() || it3 != Arguments.end());

						conditionstack.push(found ? 1 : 2);
						i -= 2;
					}
				}
			}
			else if (s == "/endif" || s == "-endif")
			{
				conditionstack.pop();
			}
			else
			{
				if (conditionstack.empty() || conditionstack.top() != 2)
				{
					name = s;
				}
			}
		}
		else if (s[0] == '$')
		{
			std::string tofind = s;
			tofind.erase(tofind.begin());

			auto it = Arguments.find(tofind);
			if (it != Arguments.end())
			{
				value += it->second + ' ';
			}
		}
		else
		{
			if (conditionstack.empty() || conditionstack.top() != 2)
			{
				value += s + ' ';
			}
		}
	}

	if (!value.empty())
	{
		value.pop_back();
	}

	if (!name.empty())
	{
		Arguments[name] = value;
	}
}

LINK_COMPONENT_TO_CLASS(CStartupArgsManager, startupargsmgr);