#pragma once
#include <string>
#include <atomic>
#include <vector>

#include "CSingleton.h"
#include "CComponentsManager.h"
#include "CConsole.h"
#include "CRendererBase.h"
#include "CResource.h"
#include "CEntity.h"
#include "CNetMessage.h"
#include "CEntityComponent.h"
#include "CBuildInfo.h"
#include "CTime.h"
#include "CPhysicsEngine.h"

#define COMPONENT_CALL(type, call) try { auto comp = CEngine::GetInstance()->Components.GetComponentTyped<type>(); if(comp) { comp->call; } } \
    catch (const std::exception& e) { } \
    catch (...) { }

#define COMPONENT_CALL_GET(_var, type, call) try { auto comp = CEngine::GetInstance()->Components.GetComponentTyped<type>(); if(comp) { _var = comp->call; } } \
    catch (const std::exception& e) { } \
    catch (...) { }

#define COMPONENT_CALL_INTERNAL(type, call, name) try { auto comp = Components.GetComponentTyped<type>(name); if(comp) { comp->call; } } \
    catch (const std::exception& e) { } \
    catch (...) { }

#define COMPONENT_CALL_NAMED(type, name, call) try { auto comp = CEngine::GetInstance()->Components.GetComponentTyped<type>(name); if(comp) {  comp->call; } } \
    catch (const std::exception& e) { } \
    catch (...) { }

class CEngine : public CSingleton<CEngine>
{
public:
	CEngine(const std::string& cmdline);
	~CEngine();

	void Init();
	void Update();
	void DeInit();

	static void Run(const std::string& cmdline);
	
	static void ProcessEarlyInitters();
	static void ProcessInitters();
	
	void Quit();
	bool GetStopFlag() const;

	CObjectFactory<CComponent, std::string> ComponentsFactory;
	CObjectFactory<CConsole, std::string> ConsoleFactory;
	CObjectFactory<CRendererBase, std::string> RenderersFactory;
	CObjectFactory<CResource, std::string> ResourcesFactory;
	CObjectFactory<CEntity, std::string> EntitiesFactory;
	CObjectFactory<CEntityComponent, std::string> EntityComponentsFactory;
	CObjectFactory<CNetMessage, std::string> NetMessagesFactory;
	CObjectFactory<CPhysicsEngine, std::string> PhysicsEnginesFactory;

	CBuildInfo BuildInfo;
	CTime Time;

	CComponentsManager Components;
	std::string CommandLine;
	/*
	CLogger Logger;
    CTimeSystem TimeSystem;
    CResourceManager ResourceManager;
    CInputSystem InputSystem;
    CRenderer Renderer;
    CAudioSystem AudioSystem;
    CPhysicsSystem PhysicsSystem;
    CSceneManager SceneManager;
	*/

	class CHookProxy
	{
	public:
		CEngine& Engine;
	};
private:
	std::atomic_bool m_stopFlag = false;
};
