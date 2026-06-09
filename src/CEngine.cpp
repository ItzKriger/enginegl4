#include <memory>

#include "CEngine.h"
#include "CFactoryInitterBase.h"
#include "CConversionRegistry.h"

#include "CLogger.h"
#include "CTerminal.h"
#include "CCommandProcessor.h"
#include "CConVarManager.h"
#include "CStartupArgsManager.h"
#include "CWindowManager.h"
#include "CWrapable.h"
#include "CExecutor.h"
#include "CResourcesManager.h"
#include "CGame.h"
#include "CScriptingManager.h"
#include "CBinaryFile.h"
#include "CModelCompiler.h"
#include "CAnimationCompiler.h"
#include "CShapeCompiler.h"
#include "U_Log.h"
#include "U_Files.h"
#include "U_General.h"
#include "CLuaPreProcessor.h"
#include "CImgui.h"
#include "CScenesManager.h"
#include "CDrawableModel.h"
#include "CDrawableAnimatable.h"
#include "CGL430Model.h"
#include "CClient.h"
#include "CServer.h"
#include "U_Automatic.h"
#include "CNetAlert.h"
#include "CTransformable.h"
#include "CBulletPhysics.h"
#include "CPhysicalEntity.h"
#include "CPhysicsEngine.h"
#include "CAngelScriptEngine.h"

#include <filesystem>
#include "boost/filesystem.hpp"
#include "boost/crc.hpp"

//#include "CFunction.h" //temporary

#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <future>
#include <variant>

CEngine::CEngine(const std::string& cmdline) : CommandLine(cmdline)
{
	
}

CEngine::~CEngine()
{

}

void CEngine::ProcessEarlyInitters()
{
	for (auto& i : EarlyInitters)
	{
		i->Process();
	}
}

void CEngine::ProcessInitters()
{
	for (auto& i : Initters)
	{
		i->Process();
	}
}

class CFunctionalSetter
{
public:
	std::function<bool(const CCommandArgsWrapper&)> Setter;
	std::function<std::variant<std::string, std::wstring, void*>(const CCommandArgsWrapper&)> Getter;
};

void CEngine::Init()
{
	ProcessInitters();
	
	Components.Add(std::make_unique<CStartupArgsManager>());
	Components.Add(std::make_unique<CConVarManager>()); //scripted

	bool NoLogger = false;
	COMPONENT_CALL_GET(NoLogger, CStartupArgsManager, IsArgumentSet("-nologger"));

	if(!NoLogger) { Components.Add(std::make_unique<CLogger>()); } //scripted
	Components.Add(std::make_unique<CCommandProcessor>());
	Components.Add(std::make_unique<CExecutor>());
	Components.Add(std::make_unique<CTerminal>());
	Components.Add(std::make_unique<CResourcesManager>());

	bool IsServer = false;
	bool NoVideo = false;
	bool NoScripting = false;

	COMPONENT_CALL_GET(IsServer, CStartupArgsManager, IsArgumentSet("-server"));
	COMPONENT_CALL_GET(NoVideo, CStartupArgsManager, IsArgumentSet("-novideo"));
	COMPONENT_CALL_GET(NoScripting, CStartupArgsManager, IsArgumentSet("-noscripting"));

	if(!IsServer && !NoVideo)
	{
		Components.Add(std::make_unique<CWindowManager>());
	}

	if(!NoScripting)
	{
		Components.Add(std::make_unique<CScriptingManager>());
	}
	//Components.Add(std::make_unique<CImgui>());

	//COMPONENT_CALL(CImgui, SetPriority(std::make_unique<CAfterPriority>(std::vector<std::string>({ "windowmanager" }))));
	COMPONENT_CALL(CWindowManager, SetPriority(std::make_unique<CLastPriority>()));
	COMPONENT_CALL(CScriptingManager, AddScriptEngine("main", std::make_unique<CScriptingEngine>()));

	auto man = Components.GetComponentTyped<CScriptingManager>();
	CScriptingEngine* engine = nullptr;

	if(man)
	{
		engine = man->GetEngine("main");
	}

	if(engine)
	{
		engine->ScriptDirectory(".\\resources\\lua\\util");
		engine->ScriptDirectory(".\\resources\\lua\\preinit");
	}

	COMPONENT_CALL(CScriptingManager, CallHook("preInit"));
	Components.Trace([](CComponent* comp) { comp->Init(); });
	
	if(man && engine)
	{
		engine->ScriptDirectory(".\\resources\\lua\\postinit");
		man->CallHook("postInit");
	}

	if (Components.IsComponentPresent("commandprocessor"))
	{
		auto& echo = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("echo",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				std::wstring out = args.Merge(0, args.size() - 1);
				COMPONENT_CALL(CLogger, Out(out + L"\n"));
				return CMD_OK;
			});

		echo->Description = "It prints!";
		echo->Aliases.insert("print");

		auto& quit = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("quit",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				CEngine::GetInstance()->Quit();
				return CMD_OK;
			});

		quit->Description = "Shutdown engine";
		quit->Aliases.insert("exit");

		auto& err = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("err",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				std::wstring err = args.Merge(0, args.size() - 1);
				COMPONENT_CALL(CLogger, Err(err + L"\n"));
				return CMD_OK;
			});

		auto& typestable = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("typestable",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				std::stringstream ss;
				for(auto& kv : TableStringTypes)
				{
					ss << kv.first << " : " << kv.second << '\n';
				}

				COMPONENT_CALL(CLogger, Out(StringUtils::StrToWstr(ss.str())));
				return CMD_OK;
			});

		auto& exec = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("exec",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				std::wstring path = args.Merge(0, args.size() - 1);
				COMPONENT_CALL(CExecutor, ExecuteFile((FileUtils::get_executable_path() / "resources" / "cfg" / StringUtils::WstrToStr(path)).string()));
				return CMD_OK;
			});

		auto& compilemdl = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("compilemdl",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				for(size_t i = 0; i < args.size(); i++)
				{
					Log::Instance() << "Compiling model \"" << args[i] << "\"\n";
					bool status = CModelCompiler::CompileModel(args[i]);

					if(status) { Log::Outln("Compiled successfully"); }
					else { Log::Errln("Compilation error occured"); }
				}
				return CMD_OK;
			});

		auto& compileanim = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("compileanim",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				for(size_t i = 0; i < args.size(); i++)
				{
					Log::Instance() << "Compiling animation \"" << args[i] << "\"\n";
					bool status = CAnimationCompiler::CompileAnimation(args[i]);

					if(status) { Log::Outln("Compiled successfully"); }
					else { Log::Errln("Compilation error occured"); }
				}
				return CMD_OK;
			});

		auto& compileshape = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("compileshape",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				for(size_t i = 0; i < args.size(); i++)
				{
					Log::Instance() << "Compiling shape \"" << args[i] << "\"\n";
					bool status = CShapeCompiler::CompileShape(args[i]);

					if(status) { Log::Outln("Compiled successfully"); }
					else { Log::Errln("Compilation error occured"); }
				}
				return CMD_OK;
			});

		//    compileconvex mesh_path is_vhacd {configs}
		auto& compileconvex = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("compileconvex", 
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto _mesh_path = args.WrapArgument<std::string>(0);
				auto mesh_path = std::filesystem::path(_mesh_path);

				auto is_vhacd = args.WrapArgumentOr<bool>(1, true);
				
				std::vector<std::filesystem::path> configs;
				for(size_t i = 2; i < args.size(); i++)
				{
					auto _cfg_path = args.WrapArgumentOr<std::string>(i, "");
					if(!_cfg_path.empty())
					{
						configs.push_back(std::filesystem::path(_cfg_path + ".json"));
						Log::Instance() << "Added config " << _cfg_path << Log::Endl;
					}
				}

				if(configs.empty() && is_vhacd)
				{
					configs.push_back(std::filesystem::path("vhacd_no_holes.json"));
					Log::Instance() << "Defaulted config to vhacd_no_holes\n";
				}

				if(is_vhacd)
				{
					Log::Instance() << "Using VHACD\n";
				}
				Log::Instance() << "Mesh path is " << _mesh_path << Log::Endl;

				CCollision::CompileNativeConvex(mesh_path, configs, is_vhacd);
				return CMD_OK;
			});

		auto& alias = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("alias",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				if(args.size() < 1)
				{
					Log::ErrInstance() << "Usage: \"alias name \"cmd\"\"\n";
					return CMD_INC;
				}

				auto cmdproc = CEngine::GetInstance()->Components.GetComponentTyped<CCommandProcessor>();
				
				auto cmd = args.WrapArgument<std::string>(0);
				auto cmdarg = args.Merge(1);

				auto it = cmdproc->Aliases.find(cmd);

				if(args.size() < 2)
				{
					if(it != cmdproc->Aliases.end())
					{
						Log::Instance() << it->first << " = \"" << it->second << "\"\n";
					}
					else
					{
						Log::ErrInstance() << "No such alias\n";
					}
					return CMD_OK;
				}
				
				if(it != cmdproc->Aliases.end())
				{
					cmdproc->Aliases.erase(it);
				}
				cmdproc->Aliases.emplace(cmd, cmdarg);
				return CMD_OK;
			}
		);

		auto& trace = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("trace",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				std::wstring path = args.Merge(0, args.size() - 1);

				CConVarManager::CVarNode* Node = nullptr;
				COMPONENT_CALL_GET(Node, CConVarManager, Root.GetNode(StringUtils::WstrToStr(path)));

				if(!Node)
				{
					COMPONENT_CALL(CLogger, Err(L"No such node\n"));
					return CMD_ERR;
				}

				if(Node->Children.empty())
				{
					COMPONENT_CALL(CLogger, Err(L"Node is empty\n"));
					return CMD_OK;
				}

				Node->Trace([](const CConVarManager::CVarNode* nod)
				{
					COMPONENT_CALL(CLogger, Out(StringUtils::StrToWstr(nod->GetPath()) + L"\n"));
				});
				
				return CMD_OK;
			});
		
		auto& component = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("component",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				if(!args.empty())
				{
					if(args[0] == L"list")
					{
						std::wstringstream ws;
						ws << L"There is " << CEngine::GetInstance()->Components.Items.size() << L" components in engine:\n";
						
						for(auto& kv : CEngine::GetInstance()->Components.Items)
						{
							ws << StringUtils::StrToWstr(kv.first) << L": " << (kv.second ? L"valid" : L"invalid") << '\n';
						}

						COMPONENT_CALL(CLogger, Out(ws.str()));
					}
				}
				return CMD_OK;
			});

		auto& creategame = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("creategame",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				CEngine::GetInstance()->Components.CreateComponent<CGame>();
				COMPONENT_CALL(CLogger, Out(L"Game created\n"));

				auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
				if(!game->PhysicsEngine)
				{
					game->PhysicsEngine = std::make_unique<CBulletPhysics>();
				}
				return CMD_OK;
			});

		auto& game = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("game",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				if(!CEngine::GetInstance()->Components.IsComponentPresent<CGame>())
				{
					CEngine::GetInstance()->Components.Add(std::make_unique<CGame>());
					auto _game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();

					_game->Init();
					_game->PostInit();

					COMPONENT_CALL(CLogger, Out(L"Game created\n"));
				}

				if(!CEngine::GetInstance()->Components.IsComponentPresent<CScenesManager>())
				{
					CEngine::GetInstance()->Components.Add(std::make_unique<CScenesManager>());
					auto _scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();

					_scenesMan->Init();
					_scenesMan->PostInit();

					COMPONENT_CALL(CLogger, Out(L"Scenes manager created\n"));
				}

				auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
				auto mainScene = std::make_shared<CScene>();
				auto mainCamera = std::make_shared<CCamera>();

				auto mainSpace = std::make_shared<CBaseSpace>();

				scenesMan->AddScene(mainScene);
				mainScene->CamerasManager.AddCamera(mainCamera);

				mainScene->CamerasManager.SetActiveCamera(mainCamera);
				mainScene->SpacesManager.AddSpace(mainSpace);

				scenesMan->SetActiveScene(mainScene);

				COMPONENT_CALL(CLogger, Out(L"Scene and space created\n"));

				auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
				auto worldid = game->Worlds.CreateWorld();

				if(!game->PhysicsEngine)
				{
					game->PhysicsEngine = std::make_unique<CBulletPhysics>();
				}
				game->PhysicsEngine->CreateWorld();

				Log::Instance() << "Created world " << worldid << Log::Endl;
				return CMD_OK;
			});

		auto& fps = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("fps",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				if(args.at(0) == L"current")
				{
					Log::Instance() << "Current FPS: " << CEngine::GetInstance()->Time.GetFPS() << Log::Endl;
				}
				else
				{
					Log::Instance() << "FPS: " << CEngine::GetInstance()->Time.GetPrevFPS() << Log::Endl;
				}
				return CMD_OK;
			});

		auto& testbinfile = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("testbinfile",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				TestBinaryFile();
				return CMD_OK;
			});

		auto& stem = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("stem",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				std::wstring data = args.Merge(0, args.size() - 1);

				std::filesystem::path stdpath(data);
				boost::filesystem::path boostpath(data);

				std::wstringstream ws;
				ws << "Std path: \"" << stdpath.wstring() << "\" stem: \"" << stdpath.stem().wstring() << "\" has_stem: " << stdpath.has_stem() << " has_filename: " << stdpath.has_filename()
					<< " has_extension: " << stdpath.has_extension() << '\n';

				ws << "Boost path: \"" << boostpath.wstring() << "\" stem: \"" << boostpath.stem().wstring() << "\" has_stem: " << boostpath.has_stem() << " has_filename: " << boostpath.has_filename()
					<< " has_extension: " << boostpath.has_extension() << '\n';

				COMPONENT_CALL(CLogger, Out(ws.str()));
				return CMD_OK;
			});

		auto& factoryid = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("factoryid",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto arg = args.WrapArgument<std::string>(0);
				auto id = CEngine::GetInstance()->ComponentsFactory.GetIndex(arg);

				Log::Instance() << "Component ID is " << id << Log::Endl;
				return CMD_OK;
			}
		);

		auto& clmsg = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("clmsg",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto client = CEngine::GetInstance()->Components.GetComponentTyped<CClient>();

				auto netalert = std::make_shared<CNetAlert>();
				netalert->BackgroundColor = CColorInt::Black;
				netalert->TextColor = CColorInt::White;
				netalert->String = args.Merge() + L"\n";

				client->SendReliableMessage(netalert);
				Log::Instance() << "Clmsg got the command\n";
				return CMD_OK;
			}
		);

		auto& svmsg = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("svmsg",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto server = CEngine::GetInstance()->Components.GetComponentTyped<CServer>();

				auto netalert = std::make_shared<CNetAlert>();
				netalert->BackgroundColor = CColorInt::Black;
				netalert->TextColor = CColorInt::White;
				netalert->String = args.Merge() + L"\n";

				server->SendReliableMessageToAll(netalert);
				Log::Instance() << "Svmsg got the command\n";
				return CMD_OK;
			}
		);

		auto& factorybyid = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("factorybyid",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto arg = args.WrapArgument<size_t>(0);
				auto id = CEngine::GetInstance()->ComponentsFactory.GetByID(arg);

				Log::Instance() << "Component IdType is " << id << Log::Endl;
				return CMD_OK;
			}
		);

		auto& crc = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("crc",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				std::wstring data = args.Merge(0, args.size() - 1);

				boost::crc_32_type result;
				result.process_bytes(data.data(), data.size() * sizeof(std::wstring::value_type));
				
				std::wstringstream ws;
				ws << "Crc32 checksum is: " << std::hex << result.checksum() << '\n';

				COMPONENT_CALL(CLogger, Out(ws.str()));

				return CMD_OK;
			});

		auto& version = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("version",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				std::stringstream ws;

				ws << "Compiled using " << CBuildInfo::GetCompiler() << " at " << CBuildInfo::GetBuildDate() << ' ' << CBuildInfo::GetBuildTime() << " for " << CBuildInfo::GetOperatingSystemName() << '\n';
				ws << "Build #" << Automatic::BuildNumber << '\n';
				
				COMPONENT_CALL(CLogger, Out(StringUtils::StrToWstr(ws.str())));

				return CMD_OK;
			});

		auto& cvlua = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("cvlua",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				if(args.empty()) { return CMD_INC; }

				std::string filename = StringUtils::WstrToStr(args[0]);
				std::wstring outname = args[1];

				CLuaPreProcessor Processor;

				std::wifstream stream(filename);
				if(!stream.is_open()) { return CMD_ERR; }

				std::wstringstream codestream;
				codestream << std::noskipws << stream.rdbuf();

				Processor.ProcessText(codestream.str());
				stream.close();

				Processor.Dump(outname);

				COMPONENT_CALL(CLogger, Outln(L"Written"));
				return CMD_OK;
			});

		auto& dolua = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("dolua",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				bool gameexist = CEngine::GetInstance()->Components.IsComponentPresent<CGame>();
				if(!gameexist) { COMPONENT_CALL(CLogger, Err(L"Game doesn't exist\n")); return CMD_OK; }

				bool scriptexist = CEngine::GetInstance()->Components.IsComponentPresent<CScriptingManager>();
				if(!gameexist) { COMPONENT_CALL(CLogger, Err(L"Scripting manager doesn't exist\n")); return CMD_OK; }

				auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
				auto scrman = CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>();

				if(!args.empty())
				{
					bool engexist = scrman->IsScriptEngineExist(StringUtils::WstrToStr(args[0]));
					if(!engexist) { COMPONENT_CALL(CLogger, Err(L"Scripting engine doesn't exist\n")); return CMD_OK; }

					auto& screng = scrman->ScriptingEngines[StringUtils::WstrToStr(args[0])];

					if(args.size() < 2)
					{
						COMPONENT_CALL(CLogger, Err(L"Specify action or script file path\n"));
						return CMD_OK;
					}

					if(args[1] == L"callhook")
					{
						if(args.size() < 3)
						{
							COMPONENT_CALL(CLogger, Err(L"Specify hook name\n"));
							return CMD_OK;
						}

						scrman->CallHook(StringUtils::WstrToStr(args[2]));
					}
					else if(args[1] == L"code")
					{
						if(args.size() < 3)
						{
							COMPONENT_CALL(CLogger, Err(L"Write some code\n"));
							return CMD_OK;
						}

						std::wstring code = StringUtils::merge_arg(args.RealArgs, 2, args.size() - 1);
						screng->ScriptString(code);
					}
					else
					{
						screng->ScriptFile(StringUtils::WstrToStr(args[1]));
					}
				}
				return CMD_OK;
			});
		
		auto& res = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("res",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				bool resmanexist = CEngine::GetInstance()->Components.IsComponentPresent<CResourcesManager>();
				if(!resmanexist) { COMPONENT_CALL(CLogger, Err(L"Resources manager doesn't exist\n")); return CMD_OK; }

				auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
				
				if(args.size() < 1)
				{
					COMPONENT_CALL(CLogger, Err(L"Specify action\n"));
					return CMD_OK;
				}

				if(args[0] == L"load")
				{
					if(args.size() < 3)
					{
						COMPONENT_CALL(CLogger, Err(L"Specify type and path\n"));
						return CMD_OK;
					}

					std::string type = StringUtils::WstrToStr(args[1]);
					std::string path = StringUtils::WstrToStr(args[2]);
					
					bool isWait = false;

					for(size_t i = 3; i < args.size(); i++)
					{
						std::string flag = StringUtils::WstrToStr(args[i]);
						if(flag == "wait")
						{
							isWait = true;
							Log::Instance() << "Will be waiting\n";
						}
					}

					auto res = resman->GetOrCreate(type, path);
					if(!res)
					{
						COMPONENT_CALL(CLogger, Err(L"Resource wasn't created\n"));
						return CMD_OK;
					}

					//CTimePoint now = CEngine::GetInstance()->Time.GetCurrent();
					//CTimePoint expires = TIME_EXPR(now + std::chrono::seconds(10));

					//resman->Cache.Add(res, expires);
					resman->Cache.Add(res);
					if(isWait)
					{
						Log::Instance() << "Waiting for resource...\n";
						res->Wait();
						Log::Instance() << "Done!\n";
					}
				}
				else if(args[0] == L"list")
				{
					for(auto& kv : resman->Resources)
					{
						if(kv.second.expired()) { continue; }
						auto res = kv.second.lock();

						std::map<CResource::CLoadingStatus, std::string> statusString = 
						{
							{ CResource::CLoadingStatus::None, "None" },
							{ CResource::CLoadingStatus::Waiting, "Waiting" },
							{ CResource::CLoadingStatus::Sync, "Sync" },
							{ CResource::CLoadingStatus::WaitingAsync, "WaitingAsync" },
							{ CResource::CLoadingStatus::StartedAsync, "StartedAsync" },
							{ CResource::CLoadingStatus::CompletedAsync, "CompletedAsync" },
							{ CResource::CLoadingStatus::WaitingForRequired, "WaitingForRequired" },
							{ CResource::CLoadingStatus::Done, "Done" },
						};

						Log::Instance() << "[" << res->GetType() << "] " << res->Name << " " << statusString[res->LoadingStatus] << 
						" (current/pipeline " << res->LoadingFunctionIndex << "/" << res->GetLoadPipeline().size() << ")" << Log::Endl;
					}
				}
				return CMD_OK;
			});

		CConversionRegistry::GetInstance()->SetTestCommand("convtest");
		
		static auto addModel = [](CBaseSpace* space, const std::string& mdlname) -> void
		{
			auto _resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
			auto _winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
			auto& _renderer = _winman->Renderer;

			auto drawable = _renderer->CreateAnimatedDrawable();

			auto _model = _resman->GetOrCreate(_renderer->GetModelType(), mdlname);
			auto model = std::dynamic_pointer_cast<CModelBase>(_model);

			auto mdl_drawable = std::dynamic_pointer_cast<CDrawableModel>(drawable);

			mdl_drawable->SetModel(model);
			space->AddDrawable(drawable);
		};

		auto& setanim = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("setanim",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
				auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
				if(!scenesMan) { return CCommand::CmdStatus::Error; }

				auto _mainScene = scenesMan->ActiveScene;

				if(!_mainScene.lock()) { return CCommand::CmdStatus::Error; }
				auto mainScene = _mainScene.lock();

				int spaceIndex = args.WrapArgument<int>(0);
				int drawableIndex = args.WrapArgument<int>(1);
				auto animName = args.WrapArgument<std::string>(2);
				auto rate = args.WrapArgumentOr<float>(3, 1.0f);
				auto oldinterp = args.WrapArgumentOr<bool>(4, 0);
				auto animtype = args.WrapArgumentOr<std::string>(5, "linear");

				std::map<std::string, InterpolationType> interpTypes = 
				{
					{ "linear", InterpolationType::Linear },
					{ "cubic", InterpolationType::Cubic },
					{ "cosine", InterpolationType::Cosine }
				};

				std::map<InterpolationType, std::string> interpTypesStr = 
				{
					{ InterpolationType::Linear, "linear" },
					{ InterpolationType::Cubic, "cubic" },
					{ InterpolationType::Cosine, "cosine" }
				};

				if(StringUtils::IsEmpty(animName))
				{
					Log::ErrInstance() << "Animation name can't be empty\n";
					return CCommand::CmdStatus::Error;
				}

				if(spaceIndex < 0 || spaceIndex >= mainScene->SpacesManager.Spaces.size())
				{
					Log::ErrInstance() << "Space index is out of range [0, " << mainScene->SpacesManager.Spaces.size() << ")\n";
					return CCommand::CmdStatus::Error;
				}

				auto space = mainScene->SpacesManager.Spaces.at(spaceIndex).get();

				if(drawableIndex < 0 || drawableIndex >= space->Drawables.size())
				{
					Log::ErrInstance() << "Drawable index is out of range [0, " << space->Drawables.size() << ")\n";
					return CCommand::CmdStatus::Error;
				}

				auto drawable = space->Drawables.at(drawableIndex);
				auto animatable = std::dynamic_pointer_cast<CDrawableAnimatable>(drawable);

				auto anim = resman->GetOrCreate("animation", animName, true);
				animatable->AddAnimation(anim, rate);

				animatable->Animations.back().InterpType = interpTypes[animtype];
				animatable->Animations.back().OldInterp = oldinterp;

				Log::Instance() << "Rate is " << rate << Log::Endl;
				Log::Instance() << "Oldinterp flag is " << animatable->Animations.back().OldInterp << Log::Endl;
				Log::Instance() << "Interpolation type is " << interpTypesStr[animatable->Animations.back().InterpType] << Log::Endl;

				return CCommand::CmdStatus::OK;
			}
		);

		auto& connect = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("connect",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto exist = CEngine::GetInstance()->Components.IsComponentPresent<CClient>();
				if(!exist)
				{
					CEngine::GetInstance()->Components.CreateComponent<CClient>();
				}
				
				auto client = CEngine::GetInstance()->Components.GetComponentTyped<CClient>();
				auto hostname = args.WrapArgument<std::string>(0);
				
				client->Connect(hostname);
				return CCommand::CmdStatus::OK;
			}
		);

		auto& server = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("server",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto exist = CEngine::GetInstance()->Components.IsComponentPresent<CServer>();
				if(!exist)
				{
					CEngine::GetInstance()->Components.CreateComponent<CServer>();
					Log::Instance() << "Server created\n";
				}
				else
				{
					CEngine::GetInstance()->Components.DeleteComponent<CServer>();
					Log::Instance() << "Server killed\n";
				}
				return CCommand::CmdStatus::OK;
			}
		);

		auto& trans = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("trans",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
				if(!scenesMan) { return CCommand::CmdStatus::Error; }

				auto _mainScene = scenesMan->ActiveScene;

				if(!_mainScene.lock()) { return CCommand::CmdStatus::Error; }
				auto mainScene = _mainScene.lock();

				//vec3(pos) quat(rot) vec3(scale)
				//setbone 0 0 root pqs
				
				//vec3(pos) euler_degrees(rot) vec3(scale)
				//setbone 0 0 root prs

				int spaceIndex = args.WrapArgument<int>(0);
				int drawableIndex = args.WrapArgument<int>(1);
				auto format = args.WrapArgument<std::string>(2);

				if(StringUtils::IsEmpty(format))
				{
					Log::ErrInstance() << "Format can't be empty\n";
					return CCommand::CmdStatus::Error;
				}

				if(spaceIndex < 0 || spaceIndex >= mainScene->SpacesManager.Spaces.size())
				{
					Log::ErrInstance() << "Space index is out of range [0, " << mainScene->SpacesManager.Spaces.size() << ")\n";
					return CCommand::CmdStatus::Error;
				}

				auto space = mainScene->SpacesManager.Spaces.at(spaceIndex).get();

				if(drawableIndex < 0 || drawableIndex >= space->Drawables.size())
				{
					Log::ErrInstance() << "Drawable index is out of range [0, " << space->Drawables.size() << ")\n";
					return CCommand::CmdStatus::Error;
				}

				auto drawable = space->Drawables.at(drawableIndex);
				auto drawable3d = std::dynamic_pointer_cast<CDrawable3D>(drawable);

				if(!drawable3d)
				{
					Log::ErrInstance() << "Drawable isn't 3D\n";
					return CCommand::CmdStatus::Error;
				}

				CTransformBase::CMeasurePack mp = drawable3d->Transform.GetPRS();

				size_t argIndex = 3;
				for(size_t i = 0; i < format.size(); i++)
				{
					auto arg = args.WrapArgument<std::string>(argIndex);
					if(format[i] == 'p')
					{
						mp.Position = StringUtils::FromStr<glm::vec3>(arg);
						Log::Instance() << "Set drawable position " << StringUtils::ToStr(mp.Position) << Log::Endl;
					}
					else if(format[i] == 's')
					{
						mp.Scale = StringUtils::FromStr<glm::vec3>(arg);
						Log::Instance() << "Set drawable scale " << StringUtils::ToStr(mp.Scale) << Log::Endl;
					}
					else if(format[i] == 'r')
					{
						auto euler = mp.GetEulerRotation();
						euler.SetRotation(AngleUtils::FromStrDegrees(arg));

						Log::Instance() << "Set drawable euler " << AngleUtils::ToStrDegrees(euler.GetRotation());
						Log::Instance() << " as quaternion " << StringUtils::ToStr(mp.Rotation) << Log::Endl;
					}
					else if(format[i] == 'q')
					{
						mp.Rotation = StringUtils::FromStr<glm::quat>(arg);
						auto euler = mp.GetEulerRotation();

						Log::Instance() << "Set drawable quaternion " << StringUtils::ToStr(mp.Rotation);
						Log::Instance() << " as euler " << AngleUtils::ToStrDegrees(euler.GetRotation()) << Log::Endl;
					}
					else if(format[i] == 'z')
					{
						mp = CTransformBase::CMeasurePack();
						Log::Instance() << "Zeroed drawable transform\n";
					}
					else
					{
						Log::ErrInstance() << "Unknown char in format" << Log::Endl;
					}
					argIndex++;
				}

				drawable3d->Transform.SetPRS(mp);
				return CCommand::CmdStatus::OK;
			}
		);

		auto& setbone = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("setbone",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
				if(!scenesMan) { return CCommand::CmdStatus::Error; }

				auto _mainScene = scenesMan->ActiveScene;

				if(!_mainScene.lock()) { return CCommand::CmdStatus::Error; }
				auto mainScene = _mainScene.lock();

				//vec3(pos) quat(rot) vec3(scale)
				//setbone 0 0 root pqs
				
				//vec3(pos) euler_degrees(rot) vec3(scale)
				//setbone 0 0 root prs

				int spaceIndex = args.WrapArgument<int>(0);
				int drawableIndex = args.WrapArgument<int>(1);
				auto boneName = args.WrapArgument<std::string>(2);
				auto format = args.WrapArgument<std::string>(3);

				if(StringUtils::IsEmpty(boneName))
				{
					Log::ErrInstance() << "Bone name can't be empty\n";
					return CCommand::CmdStatus::Error;
				}

				if(StringUtils::IsEmpty(format))
				{
					Log::ErrInstance() << "Format can't be empty\n";
					return CCommand::CmdStatus::Error;
				}

				if(spaceIndex < 0 || spaceIndex >= mainScene->SpacesManager.Spaces.size())
				{
					Log::ErrInstance() << "Space index is out of range [0, " << mainScene->SpacesManager.Spaces.size() << ")\n";
					return CCommand::CmdStatus::Error;
				}

				auto space = mainScene->SpacesManager.Spaces.at(spaceIndex).get();

				if(drawableIndex < 0 || drawableIndex >= space->Drawables.size())
				{
					Log::ErrInstance() << "Drawable index is out of range [0, " << space->Drawables.size() << ")\n";
					return CCommand::CmdStatus::Error;
				}

				auto drawable = space->Drawables.at(drawableIndex);
				auto animatable = std::dynamic_pointer_cast<CDrawableAnimatable>(drawable);

				if(!animatable)
				{
					Log::ErrInstance() << "Drawable isn't animatable\n";
					return CCommand::CmdStatus::Error;
				}

				auto it = std::find_if(animatable->Joints.begin(), animatable->Joints.end(), [boneName](auto& j) -> bool { return j.Name == boneName; });
				if(it == animatable->Joints.end())
				{
					Log::ErrInstance() << "No bone named " << boneName << Log::Endl;
					return CCommand::CmdStatus::Error;
				}

				CTransformBase::CMeasurePack mp = it->Transform.GetPRS();

				size_t argIndex = 4;
				for(size_t i = 0; i < format.size(); i++)
				{
					auto arg = args.WrapArgument<std::string>(argIndex);
					if(format[i] == 'p')
					{
						mp.Position = StringUtils::FromStr<glm::vec3>(arg);
						Log::Instance() << "Set bone position " << StringUtils::ToStr(mp.Position) << Log::Endl;
					}
					else if(format[i] == 's')
					{
						mp.Scale = StringUtils::FromStr<glm::vec3>(arg);
						Log::Instance() << "Set bone scale " << StringUtils::ToStr(mp.Scale) << Log::Endl;
					}
					else if(format[i] == 'r')
					{
						auto euler = mp.GetEulerRotation();
						euler.SetRotation(AngleUtils::FromStrDegrees(arg));

						Log::Instance() << "Set bone euler " << AngleUtils::ToStrDegrees(euler.GetRotation());
						Log::Instance() << " as quaternion " << StringUtils::ToStr(mp.Rotation) << Log::Endl;
					}
					else if(format[i] == 'q')
					{
						mp.Rotation = StringUtils::FromStr<glm::quat>(arg);
						auto euler = mp.GetEulerRotation();

						Log::Instance() << "Set bone quaternion " << StringUtils::ToStr(mp.Rotation);
						Log::Instance() << " as euler " << AngleUtils::ToStrDegrees(euler.GetRotation()) << Log::Endl;
					}
					else if(format[i] == 'z')
					{
						mp = CTransformBase::CMeasurePack();
						Log::Instance() << "Zeroed bone transform\n";
					}
					else
					{
						Log::ErrInstance() << "Unknown char in format" << Log::Endl;
					}
					argIndex++;
				}

				it->Transform.SetPRS(mp);
				return CCommand::CmdStatus::OK;
			}
		);

		auto& matrix = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("matrix",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				//  0  1  2  3  4
				//0 0  1  2  3  4
				//1 5  6  7  8  9
				//2 10 11 12 13 14
				//3 15 16 17 18 19
				//4 20 21 22 23 24

				//[0][2] = 10
				//x + (y * width) = 0 + 2 * 5 = 10
				//[3][3] = 18
				//x + (y * width) = 3 + 3 * 5 = 3 + 15 = 18

				//18 = 3, 3
				//y = ind / width
				//x = ind % width

				//y = 18 / 4 = 

				//1 2 3 1 2 3
				//0 1 2 3 4 5

				//0 + 3 = 3

				auto op = args.WrapArgument<std::string>(0);

				auto printMatrix = [](const glm::mat4& mat)
				{
					for(size_t y = 0; y < glm::mat4::length(); y++)
					{
						for(size_t x = 0; x < glm::mat4::length(); x++)
						{
							Log::Instance() << mat[x][y];
							if(x != glm::mat4::length() - 1) { Log::Instance() << " "; }
						}
						Log::Instance() << Log::Endl;
					}

					Log::Instance() << Log::Endl;

					for(size_t y = 0; y < glm::mat4::length(); y++)
					{
						for(size_t x = 0; x < glm::mat4::length(); x++)
						{
							Log::Instance() << mat[x][y] << " ";
						}
					}

					Log::Instance() << Log::Endl;
					Log::Instance() << Log::Endl;
				};

				if(op == "mul")
				{
					glm::mat4 mat1, mat2;
					for(size_t y = 0; y < glm::mat4::length(); y++)
					{
						for(size_t x = 0; x < glm::mat4::length(); x++)
						{
							auto num1 = args.WrapArgument<float>((x + (y * glm::mat4::length())) + 1);
							auto num2 = args.WrapArgument<float>(((x + (y * glm::mat4::length())) + (glm::mat4::length() * glm::mat4::length())) + 1);

							mat1[x][y] = num1;
							mat2[x][y] = num2;
						}
					}

					glm::mat4 result = mat1 * mat2;
					printMatrix(result);
				}
				else if(op == "help")
				{
					Log::Instance() << "matrix mul {mat1} {mat2}\n";
					Log::Instance() << "matrix inv {mat}\n";
					Log::Instance() << "matrix prs2m {format} (prqs) {args...}\n";
					Log::Instance() << "matrix m2prs {mat}\n";
					Log::Instance() << "matrix ident\n";
					Log::Instance() << "matrix help\n";
				}
				else if(op == "inv")
				{
					glm::mat4 mat;
					for(size_t y = 0; y < glm::mat4::length(); y++)
					{
						for(size_t x = 0; x < glm::mat4::length(); x++)
						{
							auto num = args.WrapArgument<float>((x + (y * glm::mat4::length())) + 1);

							mat[x][y] = num;
						}
					}

					glm::mat4 result = glm::inverse(mat);
					printMatrix(result);
				}
				else if(op == "prs2m")
				{
					CTransformBase::CMeasurePack mp;

					auto format = args.WrapArgument<std::string>(1);

					size_t argIndex = 2;
					for(size_t i = 0; i < format.size(); i++)
					{
						auto arg = args.WrapArgument<std::string>(argIndex);
						if(format[i] == 'p')
						{
							mp.Position = StringUtils::FromStr<glm::vec3>(arg);
							Log::Instance() << "Set bone position " << StringUtils::ToStr(mp.Position) << Log::Endl;
						}
						else if(format[i] == 's')
						{
							mp.Scale = StringUtils::FromStr<glm::vec3>(arg);
							Log::Instance() << "Set bone scale " << StringUtils::ToStr(mp.Scale) << Log::Endl;
						}
						else if(format[i] == 'r')
						{
							auto euler = mp.GetEulerRotation();
							euler.SetRotation(AngleUtils::FromStrDegrees(arg));

							Log::Instance() << "Set bone euler " << AngleUtils::ToStrDegrees(euler.GetRotation());
							Log::Instance() << " as quaternion " << StringUtils::ToStr(mp.Rotation) << Log::Endl;
						}
						else if(format[i] == 'q')
						{
							mp.Rotation = StringUtils::FromStr<glm::quat>(arg);
							auto euler = mp.GetEulerRotation();

							Log::Instance() << "Set bone quaternion " << StringUtils::ToStr(mp.Rotation);
							Log::Instance() << " as euler " << AngleUtils::ToStrDegrees(euler.GetRotation()) << Log::Endl;
						}
						else if(format[i] == 'z')
						{
							mp = CTransformBase::CMeasurePack();
							Log::Instance() << "Zeroed bone transform\n";
						}
						else
						{
							Log::ErrInstance() << "Unknown char in format" << Log::Endl;
						}
						argIndex++;
					}

					CTransform trans;
					trans.SetPRS(mp);

					glm::mat4 result = trans.GetModelMatrix();
					printMatrix(result);
				}
				else if(op == "m2prs")
				{
					glm::mat4 mat;
					for(size_t y = 0; y < glm::mat4::length(); y++)
					{
						for(size_t x = 0; x < glm::mat4::length(); x++)
						{
							auto num = args.WrapArgument<float>((x + (y * glm::mat4::length())) + 1);

							mat[x][y] = num;
						}
					}

					CTransform trans;
					trans.SetFromMatrix(mat);

					auto pos = trans.GetPosition();
					auto qua = trans.GetRotation();
					auto ang = trans.GetEulerRotation().GetRotation();
					auto scl = trans.GetScale();

					Log::Instance() << "Pos  : " << pos.x << ", " << pos.y << ", " << pos.z << Log::Endl;
					Log::Instance() << "Euler: " << ang.x.asDegrees() << ", " << ang.y.asDegrees() << ", " << ang.z.asDegrees() << Log::Endl;
					Log::Instance() << "Scale: " << scl.x << ", " << scl.y << ", " << scl.z << Log::Endl;
					Log::Instance() << "Quat : " << qua.x << ", " << qua.y << ", " << qua.z << ", " << qua.w << Log::Endl;
					
				}
				else if(op == "ident")
				{
					glm::mat4 ident = glm::mat4(1.0f);
					printMatrix(ident);
				}
				return CCommand::CmdStatus::OK;
			}
		);

		auto& bonesdbg = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("bonesdbg",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
				if(!scenesMan) { return CCommand::CmdStatus::Error; }

				auto _mainScene = scenesMan->ActiveScene;

				if(!_mainScene.lock()) { return CCommand::CmdStatus::Error; }
				auto mainScene = _mainScene.lock();

				int spaceIndex = args.WrapArgument<int>(0);
				if(spaceIndex < 0 || spaceIndex >= mainScene->SpacesManager.Spaces.size())
				{
					Log::ErrInstance() << "Index is out of range [0, " << mainScene->SpacesManager.Spaces.size() << ")\n";
					return CCommand::CmdStatus::Error;
				}

				auto space = mainScene->SpacesManager.Spaces.at(spaceIndex).get();
				for(auto drawable : space->Drawables)
				{
					auto mdl = std::dynamic_pointer_cast<CDrawableModel>(drawable);
					auto animatable = std::dynamic_pointer_cast<CDrawableAnimatable>(drawable);

					auto model = std::dynamic_pointer_cast<CGL430Model>(mdl->GetModel());

					for(auto& joint : animatable->Joints)
					{
						Log::Instance() << "Bone " << joint.Name;
						if(joint.Parent)
						{
							Log::Instance() << " has parent of " << joint.Parent->Name << Log::Endl;
						}
						else
						{
							Log::Instance() << " has no parent\n";
						}

						Log::Instance() << Log::Endl;

						Log::Instance() << "Bindpose:\n";
						for(size_t y = 0; y < glm::mat4::length(); y++)
						{
							for(size_t x = 0; x < glm::mat4::length(); x++)
							{
								Log::Instance() << joint.BindPose[x][y] << " ";
							}
						}

						Log::Instance() << Log::Endl << Log::Endl;

						Log::Instance() << "GlobalBind:\n";
						for(size_t y = 0; y < glm::mat4::length(); y++)
						{
							for(size_t x = 0; x < glm::mat4::length(); x++)
							{
								Log::Instance() << joint.GlobalBind[x][y] << " ";
							}
						}

						Log::Instance() << Log::Endl << Log::Endl;

						glm::mat4 trans = joint.Transform.GetModelMatrix();
						glm::mat4 getmatrix = joint.GetMatrix();
						auto matrix = joint.GetMatrix() * glm::inverse(joint.GlobalBind);

						Log::Instance() << "Transform:\n";
						for(size_t y = 0; y < glm::mat4::length(); y++)
						{
							for(size_t x = 0; x < glm::mat4::length(); x++)
							{
								Log::Instance() << trans[x][y] << " ";
							}
						}

						Log::Instance() << Log::Endl << Log::Endl;

						Log::Instance() << "GetMatrix:\n";
						for(size_t y = 0; y < glm::mat4::length(); y++)
						{
							for(size_t x = 0; x < glm::mat4::length(); x++)
							{
								Log::Instance() << getmatrix[x][y] << " ";
							}
						}

						Log::Instance() << Log::Endl << Log::Endl;

						Log::Instance() << "GetMatrix * inverse(GlobalBind):\n";
						for(size_t y = 0; y < glm::mat4::length(); y++)
						{
							for(size_t x = 0; x < glm::mat4::length(); x++)
							{
								Log::Instance() << matrix[x][y] << " ";
							}
						}

						Log::Instance() << Log::Endl << Log::Endl;
					}
				}

				return CCommand::CmdStatus::OK;
			}
		);

		auto& addmodel = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("addmodel",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
				if(!scenesMan) { return CCommand::CmdStatus::Error; }

				auto _mainScene = scenesMan->ActiveScene;

				if(!_mainScene.lock()) { return CCommand::CmdStatus::Error; }
				auto mainScene = _mainScene.lock();

				std::string modelName = args.WrapArgument<std::string>(0);
				int spaceIndex = args.WrapArgument<int>(1);

				if(StringUtils::IsEmpty(modelName))
				{
					Log::ErrInstance() << "Model name can't be empty\n";
					return CCommand::CmdStatus::Error;
				}

				if(spaceIndex < 0 || spaceIndex >= mainScene->SpacesManager.Spaces.size())
				{
					Log::ErrInstance() << "Index is out of range [0, " << mainScene->SpacesManager.Spaces.size() << ")\n";
					return CCommand::CmdStatus::Error;
				}

				auto space = mainScene->SpacesManager.Spaces.at(spaceIndex).get();
				addModel(space, modelName);

				return CCommand::CmdStatus::OK;
			});

		auto& testscenes = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("testscenes",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				if(!CEngine::GetInstance()->Components.IsComponentPresent<CScenesManager>())
				{
					CEngine::GetInstance()->Components.Add(std::make_unique<CScenesManager>());
					auto _scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();

					_scenesMan->Init();
					_scenesMan->PostInit();
				}

				auto resman = CEngine::GetInstance()->Components.GetComponentTyped<CResourcesManager>();
				auto winman = CEngine::GetInstance()->Components.GetComponentTyped<CWindowManager>();
				auto& renderer = winman->Renderer;

				auto scenesMan = CEngine::GetInstance()->Components.GetComponentTyped<CScenesManager>();
				auto mainScene = std::make_shared<CScene>();
				auto mainCamera = std::make_shared<CCamera>();

				auto mainSpace = std::make_shared<CBaseSpace>();

				scenesMan->AddScene(mainScene);
				mainScene->CamerasManager.AddCamera(mainCamera);

				mainScene->CamerasManager.SetActiveCamera(mainCamera);
				mainScene->SpacesManager.AddSpace(mainSpace);

				scenesMan->SetActiveScene(mainScene);
				//addModel(mainSpace.get(), "tankroad_terrain.emdl");
				//addModel(mainSpace.get(), "crossfire_big_textured.emdl");

				return CCommand::CmdStatus::OK;
			});

		auto& tst = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("tst",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				CEngine::GetInstance()->Components.GetComponentTyped<CCommandProcessor>()->ProcessCommandLine(L"creategame", CCommandSender::Self);
				CEngine::GetInstance()->Components.GetComponentTyped<CCommandProcessor>()->ProcessCommandLine(L"dolua game core.lua", CCommandSender::Self);

				return CCommand::CmdStatus::OK;
			});

		auto& tstr = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("tstr",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				CEngine::GetInstance()->Components.GetComponentTyped<CCommandProcessor>()->ProcessCommandLine(L"creategame", CCommandSender::Self);
				CEngine::GetInstance()->Components.GetComponentTyped<CCommandProcessor>()->ProcessCommandLine(L"dolua game core.lua", CCommandSender::Self);
				CEngine::GetInstance()->Components.GetComponentTyped<CCommandProcessor>()->ProcessCommandLine(L"crcomp testrender", CCommandSender::Self);

				return CCommand::CmdStatus::OK;
			});

		static std::map<std::string, CFunctionalSetter> GetSetFields = 
		{
			{
				"server.rate",
				{
					[](const CCommandArgsWrapper& args) -> bool //setter
					{
						auto server = CEngine::GetInstance()->Components.GetComponentTyped<CServer>();
						if(!server) { return false; }

						auto rate = args.WrapArgument<double>(0);
						server->SetRate(rate);

						return true;
					},
					[](const CCommandArgsWrapper& args) -> std::variant<std::string, std::wstring, void*> //getter
					{
						auto server = CEngine::GetInstance()->Components.GetComponentTyped<CServer>();
						if(!server) { return nullptr; }

						return StringUtils::ToStr(server->GetRate());
					}
				},
			},
			{
				"phys.gravity",
				{
					[](const CCommandArgsWrapper& args) -> bool //setter
					{
						auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
						if(!game || !game->PhysicsEngine) { return false; }

						auto worldid = args.WrapArgument<int>(0);

						auto world = game->PhysicsEngine->GetWorld(worldid);
						if(!world) { return false; }

						world->SetGravity(args.WrapArgument<glm::vec3>(1));
						return true;
					},
					[](const CCommandArgsWrapper& args) -> std::variant<std::string, std::wstring, void*> //getter
					{
						auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();
						if(!game || !game->PhysicsEngine) { return nullptr; }

						auto worldid = args.WrapArgument<int>(0);

						auto world = game->PhysicsEngine->GetWorld(worldid);
						if(!world) { return nullptr; }

						return StringUtils::ToStr(world->GetGravity());
					}
				}
			}
		};

		auto& set = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("set",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto name = args.WrapArgument<std::string>(0);
				CCommandArgsWrapper _args = args;

				_args.RealArgs.erase(_args.RealArgs.begin());
				auto it = GetSetFields.find(name);

				if(it == GetSetFields.end())
				{
					Log::ErrInstance() << "No such field\n";
				}
				else
				{
					it->second.Setter(_args);
					auto var = it->second.Getter(_args);

					Log::Instance() << "Field " << name << " set to ";
					if(std::holds_alternative<std::string>(var))
					{
						auto string = std::get<std::string>(var);
						Log::Instance() << string << Log::Endl;
					}
					else if(std::holds_alternative<std::wstring>(var))
					{
						auto wstring = std::get<std::wstring>(var);
						Log::Instance() << wstring << Log::Endl;
					}
					else
					{
						Log::Instance() << "{unknown}\n";
					}
				}
				return CMD_OK;
			}
		);

		auto& get = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("get",
			CCommand::CType::Server, CCommand::CmdRight::Client,
			[]COMMAND_LAMBDA
			{
				auto name = args.WrapArgument<std::string>(0);
				auto it = GetSetFields.find(name);

				CCommandArgsWrapper _args = args;
				_args.RealArgs.erase(_args.RealArgs.begin());

				if(it == GetSetFields.end())
				{
					Log::ErrInstance() << "No such field\n";
				}
				else
				{
					auto var = it->second.Getter(_args);

					Log::Instance() << "Field " << name << " is ";
					if(std::holds_alternative<std::string>(var))
					{
						auto string = std::get<std::string>(var);
						Log::Instance() << string << Log::Endl;
					}
					else if(std::holds_alternative<std::wstring>(var))
					{
						auto wstring = std::get<std::wstring>(var);
						Log::Instance() << wstring << Log::Endl;
					}
					else
					{
						Log::Instance() << "{unknown}\n";
					}
				}
				return CMD_OK;
			}
		);

		auto& entity = Components.GetComponentTyped<CCommandProcessor>("commandprocessor")->Commands.CreateCommand("world",
				CCommand::CType::Server, CCommand::CmdRight::Client,
				[]COMMAND_LAMBDA
				{
					bool gameexist = CEngine::GetInstance()->Components.IsComponentPresent<CGame>();
					if(!gameexist) { COMPONENT_CALL(CLogger, Err(L"Game doesn't exist\n")); return CMD_OK; }

					auto game = CEngine::GetInstance()->Components.GetComponentTyped<CGame>();

					if(!args.empty())
					{
						if(StringUtils::isNumber(args[0]))
						{
							int num = StringUtils::FromStr<int, std::wstring>(args[0]);

							if(!game->Worlds.IsWorldExist(num)) { COMPONENT_CALL(CLogger, Err(L"World doesn't exist\n")); return CMD_OK; }
							auto world = game->Worlds.GetWorld(num);

							if(args.size() > 1)
							{
								if(args[1] == L"entity" && args.size() > 2)
								{
									if(args[2] == L"create" && args.size() > 3)
									{
										auto handle = world->Entities.CreateEntity(StringUtils::WstrToStr(args[3]));
										
										if(handle.IsValid())
										{
											COMPONENT_CALL(CLogger, Out(L"Entity created\n"));
										}
										else
										{
											COMPONENT_CALL(CLogger, Err(L"Entity is not created (invalid handle)\n"));
										}
									}
									else if(args[2] == L"param")
									{
										size_t ind = 3;
										for(;;)
										{
											std::string type = args.WrapArgument<std::string>(ind);

											ind++;
											if(ind >= args.size()) { break; }

											std::string name = args.WrapArgument<std::string>(ind);

											ind++;
											if(ind >= args.size()) { break; }

											if(type == "string")
											{
												world->Entities.SetNextEntityInitParam(name, new CWrapable<std::string>(args.WrapArgument<std::string>(ind)));
												Log::Instance() << "name " << name << " value " << args.WrapArgument<std::string>(ind) << Log::Endl;
											}
											else if(type == "wstring") { world->Entities.SetNextEntityInitParam(name, new CWrapable<std::wstring>(args.WrapArgument<std::wstring>(ind))); }
											else if(type == "float") { world->Entities.SetNextEntityInitParam(name, new CWrapable<float>(args.WrapArgument<float>(ind))); }
											else if(type == "vec3") { world->Entities.SetNextEntityInitParam(name, new CWrapable<glm::vec3>(args.WrapArgument<glm::vec3>(ind))); }
											else if(type == "int") { world->Entities.SetNextEntityInitParam(name, new CWrapable<int>(args.WrapArgument<int>(ind))); }
											else { Log::ErrInstance() << "Unknown type\n"; break; }

											ind++;
											if(ind >= args.size()) { break; }
										}

										//4
										//5
										//6 - 3 = 3 / 3 = 1
										//7
										//8
										//9 - 3 = 6 / 3 = 2

										Log::Instance() << "Added " << ((ind - 3) / 3) << " params\n";
									}
									else if(args[2] == L"vel")
									{
										auto entid = args.WrapArgument<int>(3);
										
										glm::vec3 newvel;
										newvel.x = args.WrapArgument<float>(4);
										newvel.y = args.WrapArgument<float>(5);
										newvel.z = args.WrapArgument<float>(6);

										Log::Instance() << "Setting velocity " << newvel << Log::Endl;

										auto it = std::find_if(world->Entities.Items.begin(), world->Entities.Items.end(), [entid](auto& kv) -> bool
										{
											return kv.second->GetUUID() == entid;
										});

										if(it != world->Entities.Items.end())
										{
											auto& entity = it->second;
											auto physical = entity->Components.GetComponentTyped<CPhysicalEntity>();
											if(physical && physical->WorldUnit)
											{
												auto rigidBody = dynamic_cast<CRigidBody*>(physical->WorldUnit);
												if(rigidBody)
												{
													rigidBody->Activate();
													rigidBody->SetLinearVelocity
													(
														{
															newvel.x,
															newvel.y,
															newvel.z
														}
													);
													rigidBody->Activate();
													Log::Instance() << "Set!\n";
												}
											}
										}
									}
									else if(args[2] == L"physreset")
									{
										auto entid = args.WrapArgument<int>(3);
										Log::Instance() << "Correcting...\n";

										auto it = std::find_if(world->Entities.Items.begin(), world->Entities.Items.end(), [entid](auto& kv) -> bool
										{
											return kv.second->GetUUID() == entid;
										});

										if(it != world->Entities.Items.end())
										{
											auto& entity = it->second;
											auto physical = entity->Components.GetComponentTyped<CPhysicalEntity>();
											auto transformable = entity->Components.GetComponentTyped<CTransformable>();

											if(transformable)
											{
												CTransform trans;
												trans.ResetTransform();

												transformable->Transform.SetPRS(trans.GetPRS());
											}

											if(physical && physical->WorldUnit)
											{
												auto rigidBody = dynamic_cast<CRigidBody*>(physical->WorldUnit);
												if(rigidBody)
												{
													rigidBody->SetLinearVelocity({ 0.0f, 0.0f, 0.0f });
													rigidBody->Activate();
													Log::Instance() << "Done!\n";
												}
											}
										}
									}
									else if(args[2] == L"transdump")
									{
										auto entid = args.WrapArgument<int>(3);
										
										auto it = std::find_if(world->Entities.Items.begin(), world->Entities.Items.end(), [entid](auto& kv) -> bool
										{
											return kv.second->GetUUID() == entid;
										});

										if(it != world->Entities.Items.end())
										{
											auto& entity = it->second;

											auto trans = entity->Components.GetComponentTyped<CTransformable>();
											if(trans)
											{
												std::ofstream file("transdump_ent" + std::to_string(entid) + (IsConnectedClient() ? "_cl" : "_sv") + ".csv");
												if(file.is_open())
												{
													file << "time;x;y;z\n";
													auto base = trans->DumpQueue.front().first;

													for(auto& kv : trans->DumpQueue)
													{
														auto dt = kv.first - base;
														double t = std::chrono::duration<double>(dt).count();

														file << std::fixed << t << ';' << kv.second.Position.x << ';' << kv.second.Position.y << ';' << kv.second.Position.z << std::endl;
													}
													file.close();
													Log::Instance() << "Dumped!\n";
												}
											}
										}
									}
									else if(args[2] == L"trans")
									{
										auto entid = args.WrapArgument<int>(3);
										
										auto it = std::find_if(world->Entities.Items.begin(), world->Entities.Items.end(), [entid](auto& kv) -> bool
										{
											return kv.second->GetUUID() == entid;
										});

										if(it != world->Entities.Items.end())
										{
											auto& entity = it->second;

											auto trans = entity->Components.GetComponentTyped<CTransformable>();
											if(trans && args.size() > 4)
											{
												CTransformBase::CMeasurePack mp = trans->Transform.GetPRS();

												std::string format = args.WrapArgument<std::string>(4);

												size_t argIndex = 5;
												for(size_t i = 0; i < format.size(); i++)
												{
													auto arg = args.WrapArgument<std::string>(argIndex);
													if(format[i] == 'p')
													{
														mp.Position = StringUtils::FromStr<glm::vec3>(arg);
														Log::Instance() << "Set trans position " << StringUtils::ToStr(mp.Position) << Log::Endl;
													}
													else if(format[i] == 's')
													{
														mp.Scale = StringUtils::FromStr<glm::vec3>(arg);
														Log::Instance() << "Set trans scale " << StringUtils::ToStr(mp.Scale) << Log::Endl;
													}
													else if(format[i] == 'r')
													{
														auto euler = mp.GetEulerRotation();
														euler.SetRotation(AngleUtils::FromStrDegrees(arg));

														Log::Instance() << "Set trans euler " << AngleUtils::ToStrDegrees(euler.GetRotation());
														Log::Instance() << " as quaternion " << StringUtils::ToStr(mp.Rotation) << Log::Endl;
													}
													else if(format[i] == 'q')
													{
														mp.Rotation = StringUtils::FromStr<glm::quat>(arg);
														auto euler = mp.GetEulerRotation();

														Log::Instance() << "Set trans quaternion " << StringUtils::ToStr(mp.Rotation);
														Log::Instance() << " as euler " << AngleUtils::ToStrDegrees(euler.GetRotation()) << Log::Endl;
													}
													else if(format[i] == 'z')
													{
														mp = CTransformBase::CMeasurePack();
														Log::Instance() << "Zeroed trans transform\n";
													}
													else
													{
														Log::ErrInstance() << "Unknown char in format" << Log::Endl;
													}
													argIndex++;
												}

												trans->Transform.SetPRS(mp);
											}
											else if(trans && args.size() <= 4)
											{
												auto pos = StringUtils::ToStr(trans->Transform.GetPosition());
												auto qua = StringUtils::ToStr(trans->Transform.GetRotation());
												auto eul = trans->Transform.GetEulerRotation().GetRotation();
												auto scl = StringUtils::ToStr(trans->Transform.GetScale());

												Log::Instance() << "Entity transform\n";
												Log::Instance() << "Position: " << pos << Log::Endl;
												Log::Instance() << "Euler: " << StringUtils::ToStr(eul.x.asDegrees()) << " " << StringUtils::ToStr(eul.y.asDegrees()) << " " << StringUtils::ToStr(eul.z.asDegrees()) << Log::Endl;
												Log::Instance() << "Rotation: " << qua << Log::Endl;
												Log::Instance() << "Scale: " << scl << Log::Endl;
											}
										}
									}
									else if(args[2] == L"list")
									{
										std::wstringstream ws;
										ws << L"There is " << world->Entities.Count() << L" entities:\n";

										for(auto& kv : world->Entities.Items)
										{
											ws << kv.first << L": " << StringUtils::StrToWstr(kv.second->GetType()) << '\n';
										}

										COMPONENT_CALL(CLogger, Out(ws.str()));
									}
								}
							}
						}
						else
						{
							if(args[0] == L"list")
							{
								std::wstringstream ws;
								ws << L"There is " << game->Worlds.Count() << L" worlds:\n";
								
								for(auto& kv : game->Worlds.Items)
								{
									ws << kv.first << L": " << (kv.second ? L"valid" : L"invalid") << '\n';
								}

								COMPONENT_CALL(CLogger, Out(ws.str()));
							}
							else if(args[0] == L"create")
							{
								auto id = game->Worlds.CreateWorld();

								std::wstringstream ws;
								ws << L"Created world with id " << id << '\n';

								COMPONENT_CALL(CLogger, Out(ws.str()));
							}
						}
					}
					return CMD_OK;
				});
	}

	if (Components.IsComponentPresent("convarmanager"))
	{
		auto mgr = CEngine::GetInstance()->Components.GetComponentTyped<CConVarManager>();
		
		mgr->AddConVar("sv.cheats", new CWrapable<bool>(false));
		mgr->AddConVar("fps.max", new CWrapable<unsigned int>(0));
		mgr->AddConVar("fps.dump", new CWrapable<bool>(false));
		mgr->AddConVar("con.echo", new CWrapable<bool>(true));
		mgr->AddConVar("cam.sensitivity", new CWrapable<float>(0.005f));
		mgr->AddConVar("cam.speed", new CWrapable<float>(1.0f));
		mgr->AddConVar("cam.accel", new CWrapable<float>(50.0f));
		mgr->AddConVar("cam.damping", new CWrapable<float>(8.0f));
		mgr->AddConVar("cam.maxspeed", new CWrapable<float>(10.0f));

		mgr->AddConVar("dump.trans.timestep", new CWrapable<int>(100));
		mgr->AddConVar("dump.trans.maxmem", new CWrapable<std::uint64_t>(1000000));

		mgr->AddConVar("phys.gravity", new CWrapable<glm::vec3>(DefaultGravity));
		mgr->AddConVar("phys.substeps", new CWrapable<int>(10));
		mgr->AddConVar("phys.fixedtimestep", new CWrapable<int>(60));

		mgr->AddConVar("time.limit.yield", new CWrapable<bool>(true));
   		mgr->AddConVar("time.limit.adaptive_sleep", new CWrapable<bool>(true));
    	mgr->AddConVar("time.limit.strict_sleep", new CWrapable<bool>(false));
		
    	mgr->AddConVar("lag.threshold", new CWrapable<float>(0.4f));
    	mgr->AddConVar("lag.move", new CWrapable<double>(0.001));

		mgr->GetRealConVar("sv.cheats").Description = L"Enable or disable cheats";
		mgr->GetRealConVar("fps.max").Description = L"Limit FPS. 0 for unlimited";
		mgr->GetRealConVar("fps.dump").Description = L"Debug engine timings";
		mgr->GetRealConVar("con.echo").Description = L"Enable or disable printing user commands";
		mgr->GetRealConVar("cam.sensitivity").Description = L"Camera sensitivity";
		mgr->GetRealConVar("cam.speed").Description = L"Camera speed";

		mgr->AddConVar("net.clientport", new CWrapable<unsigned short>(Networking::ClientPort));
		mgr->AddConVar("net.hostport", new CWrapable<unsigned short>(Networking::HostPort));
		mgr->AddConVar("net.retries", new CWrapable<std::uint32_t>(3));
		mgr->AddConVar("net.connection_timeout", new CWrapable<std::uint32_t>(10000));
		mgr->AddConVar("net.timeout", new CWrapable<std::uint32_t>(20000));
		mgr->AddConVar("net.server_timeout", new CWrapable<std::uint32_t>(20000));
		mgr->AddConVar("cl.nickname", new CWrapable<std::wstring>(L"player"));

		mgr->AddConVar("cl.interp", new CWrapable<bool>(true));
		mgr->AddConVar("cl.interp.frames", new CWrapable<std::uint8_t>(1));

		mgr->GetRealConVar("net.clientport").Description = L"Client port";
		mgr->GetRealConVar("net.hostport").Description = L"Host (server) port";
		mgr->GetRealConVar("net.retries").Description = L"Attempts for connecting to server before dropping the connection";
		mgr->GetRealConVar("net.connection_timeout").Description = L"Connection timeout in milliseconds";
		mgr->GetRealConVar("net.timeout").Description = L"Client timeout in milliseconds";
		mgr->GetRealConVar("net.server_timeout").Description = L"Server timeout in milliseconds";
	}

	Components.Trace([](CComponent* comp) { comp->PostInit(); });
	Time.Init();

	if(engine)
	{
		engine->ScriptDirectory(".\\resources\\lua\\main");
	}
	COMPONENT_CALL(CScriptingManager, CallHook("mainInit"));
	//Components.GetComponentTyped<CLogger>()->OnUpdate.Add(std::make_unique<CFunction<void, CComponent*>>([](CComponent* comp) { std::wcout << L"Updated!\n"; }));

	CAngelScriptEngine angel;
	angel.Init();

	angel.LoadDirectory("main", ".\\resources\\angel");
	angel.CallInitFunctions();
}

void CEngine::Run(const std::string& cmdline)
{
	ProcessEarlyInitters();
	
	std::unique_ptr<CConversionRegistry> ConversionRegistry = std::make_unique<CConversionRegistry>();
	std::unique_ptr<CEngine> Engine = std::make_unique<CEngine>(cmdline);

	auto start_init = Engine->Time.GetCurrent();
	Engine->Init();
	auto stop_init = Engine->Time.GetCurrent();

	auto init_diff = std::chrono::duration_cast<std::chrono::duration<float>>(stop_init - start_init);
	Log::Instance() << "Engine init took " << init_diff.count() << "s\n";
	
	while (!Engine->GetStopFlag())
	{
		Engine->Update();
	}
	Engine->DeInit();
	COMPONENT_CALL(CLogger, Out(L"Quitting engine...\n"));

	std::cout << "Quitting engine...\n";
}

void CEngine::Update()
{
	Time.NewTick();
	
	float lagThreshold = 0.4f;
	COMPONENT_CALL_GET(lagThreshold, CConVarManager, GetConVarValue<float>("lag.threshold", 0.4f));

	double lagMove = 0.0;
	COMPONENT_CALL_GET(lagMove, CConVarManager, GetConVarValue<double>("lag.move", 0.0));

	if(Time.GetDeltaTime() >= lagThreshold)
	{
		Log::ErrInstance() << "Lag!\n";
		Time.ResetDeltaTime(lagMove);
	}

	std::vector<std::string> ComponentsOrder;
	ComponentsOrder.reserve(128);

	Components.Trace([&ComponentsOrder](CComponent* comp)
	{
		auto start_update = CEngine::GetInstance()->Time.GetCurrent();
		comp->Update();
		auto stop_update = CEngine::GetInstance()->Time.GetCurrent();

		auto update_diff = std::chrono::duration_cast<std::chrono::duration<float>>(stop_update - start_update);
		if(update_diff.count() >= 0.1f)
		{
			Log::ErrInstance() << "Component " << comp->GetType() << " update took " << update_diff.count() << "s\n";
		}

		ComponentsOrder.push_back(comp->GetType());
	});

	auto man = CEngine::GetInstance()->Components.GetComponentTyped<CScriptingManager>();
	if(man)
	{
		man->CallHook("update");
	}

	Time.EndTick();

	if(GetStopFlag())
	{
		size_t compIndex = 0;

		Log::Instance() << "Components order:\n";
		for(auto& s : ComponentsOrder)
		{
			Log::Instance() << "[" << compIndex << "] " << s << Log::Endl;
			compIndex++;
		}
	}
}

bool CEngine::GetStopFlag() const
{
	return m_stopFlag;
}

void CEngine::Quit()
{
	m_stopFlag = true;
}

void CEngine::DeInit()
{
	Components.ReverseTrace([](CComponent* comp) { comp->DeInit(); });
}
