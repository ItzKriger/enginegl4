#include "CWindowManager.h"
#include "CEngine.h"
#include "CLogger.h"
#include "CWrapable.h"
#include "CStartupArgsManager.h"
#include "U_Macros.h"

#include "boost/bimap.hpp"
#include "boost/assign/list_of.hpp"

#include "CTestRender.h"

CWindowManager::CWindowManager()
{
}

CWindowManager::~CWindowManager()
{
	DestructWindow();
	SDL_Quit();
}

bool CWindowManager::V_Init()
{
	Log::Instance() << "CWindowManager::V_Init\n";
	constexpr int init = SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_SENSOR;

	int status = SDL_Init(init); //TODO maybe choose what subsystems we need to use?
	if (status < 0)
	{
		COMPONENT_CALL(CLogger, Err(L"Can't initialize SDL. Not using window manager.\n"));
		return false;
	}

	unsigned int width = 1024, height = 768;
	bool windowed = true, noborder = false, fullscreen = false;

	COMPONENT_CALL(CStartupArgsManager, GetIfExist<unsigned int>("-width", width));
	COMPONENT_CALL(CStartupArgsManager, GetIfExist<unsigned int>("-height", height));
	COMPONENT_CALL_GET(windowed, CStartupArgsManager, IsArgumentSet("-windowed"));
	COMPONENT_CALL_GET(noborder, CStartupArgsManager, IsArgumentSet("-noborder"));
	COMPONENT_CALL_GET(fullscreen, CStartupArgsManager, IsArgumentSet("-fullscreen"));

	if(fullscreen)
	{
		windowed = noborder = false;
	}

	width = std::clamp<unsigned int>(width, 1, 0x00FFFFFF);
	height = std::clamp<unsigned int>(height, 1, 0x00FFFFFF);

	std::string RendererType = "gl430";
	COMPONENT_CALL_GET(RendererType, CStartupArgsManager, GetArgumentValue("-renderer"));

	Renderer = CEngine::GetInstance()->RenderersFactory.create<std::unique_ptr<CRendererBase>>(RendererType);
	if(!Renderer)
	{
		COMPONENT_CALL(CLogger, Err(L"Can't create renderer.\n"));
		return false;
	}

	size_t flags = GetWindowFlags(windowed, noborder, Renderer->GetGenericFlag());
	Window = SDL_CreateWindow("EngineGL 4.0", width, height, flags);

	if(!Window)
	{
		COMPONENT_CALL(CLogger, Err(L"Can't create window!\n"));
		return false;
	}

	m_UpdateAspect();

	Renderer->Init(Window);
	Log::Instance() << "CWindowManager::V_Init END\n";

	SDL_SetWindowRelativeMouseMode(Window, true); //TODO temporary
	return true;
}

void CWindowManager::m_UpdateAspect()
{
	auto size = GetWindowSize();
	m_aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
}

float CWindowManager::GetWindowAspect() const
{
	return m_aspect;
}

void CWindowManager::HandleEvent(const SDL_Event& event)
{
	if (event.type == SDL_EVENT_QUIT)
	{
		CEngine::GetInstance()->Quit();
	}
	else if (event.type == SDL_EVENT_WINDOW_RESIZED)
	{
		m_UpdateAspect();
	}
	else if (event.type == SDL_EVENT_KEY_DOWN)
	{
		SDL_Keycode keycode = event.key.key;
		if(keycode == SDLK_T)
		{
			bool toggled = !SDL_GetWindowRelativeMouseMode(Window);
			SDL_SetWindowRelativeMouseMode(Window, toggled);

			Log::Instance() << "Cursor toggled\n";
		}
	}
}

void CWindowManager::V_Update()
{
	while (SDL_PollEvent(&m_Event) != 0)
	{
		HandleEvent(m_Event);
		OnEventHandle(m_Event);
	}

	Renderer->Update();
	Renderer->ClearWindow();
	COMPONENT_CALL(CTestRender, Draw());
	OnDraw();
	Renderer->Display();
}

void CWindowManager::DestructWindow()
{
	if(Renderer)
	{
		Renderer.reset();
	}

	if (Window)
	{
		SDL_DestroyWindow(Window);
		Window = nullptr;
	}
}

glm::ivec2 CWindowManager::GetWindowSize()
{
	int w = 0;
	int h = 0;
	
	SDL_GetWindowSizeInPixels(Window, &w, &h);
	return glm::ivec2(static_cast<glm::ivec2::value_type>(w), static_cast<glm::ivec2::value_type>(h));
}

size_t CWindowManager::GetWindowFlags(bool windowed, bool noborder, size_t generic_flag)
{
	size_t flag = generic_flag | SDL_WINDOW_RESIZABLE;

	if (!windowed) { return (flag | SDL_WINDOW_FULLSCREEN); }
	if (windowed && !noborder) { return (flag); }
	if (windowed && noborder) { return (flag | SDL_WINDOW_BORDERLESS); }
	return flag;
}

bool CWindowManager::V_ScriptInit(std::shared_ptr<sol::state> state, sol::table table)
{
	static boost::bimap<std::string, SDL_Scancode> stringToCodes =
    boost::assign::list_of<boost::bimap<std::string, SDL_Scancode>::value_type>
	("a", SDL_SCANCODE_A)
	("b", SDL_SCANCODE_B)
	("c", SDL_SCANCODE_C)
	("d", SDL_SCANCODE_D)
	("e", SDL_SCANCODE_E)
	("f", SDL_SCANCODE_F)
	("g", SDL_SCANCODE_G)
	("h", SDL_SCANCODE_H)
	("i", SDL_SCANCODE_I)
	("j", SDL_SCANCODE_J)
	("k", SDL_SCANCODE_K)
	("l", SDL_SCANCODE_L)
	("m", SDL_SCANCODE_M)
	("n", SDL_SCANCODE_N)
	("o", SDL_SCANCODE_O)
	("p", SDL_SCANCODE_P)
	("q", SDL_SCANCODE_Q)
	("r", SDL_SCANCODE_R)
	("s", SDL_SCANCODE_S)
	("t", SDL_SCANCODE_T)
	("u", SDL_SCANCODE_U)
	("v", SDL_SCANCODE_V)
	("w", SDL_SCANCODE_W)
	("x", SDL_SCANCODE_X)
	("y", SDL_SCANCODE_Y)
	("z", SDL_SCANCODE_Z)
	("lshift", SDL_SCANCODE_LSHIFT)
	("lctrl", SDL_SCANCODE_LCTRL)
	("lalt", SDL_SCANCODE_LALT)
	("rshift", SDL_SCANCODE_RSHIFT)
	("rctrl", SDL_SCANCODE_RCTRL)
	("ralt", SDL_SCANCODE_RALT);

	table["onEventHandle"] = OnEventHandle.GetScriptTable(state);

	table.set_function("getRelativeMouseState", []() -> glm::vec2
	{
		float dx, dy;
    	SDL_GetRelativeMouseState(&dx, &dy);

		return { dx, dy };
	});

	table.set_function("setRelativeMouseMode", [this](bool enable) -> void { SDL_SetWindowRelativeMouseMode(Window, enable); });
	table.set_function("getRelativeMouseMode", [this]() -> bool { return SDL_GetWindowRelativeMouseMode(Window); });

	table.set_function("getKeyState", [](const std::string& name) -> bool
	{
		const bool* keyboard = SDL_GetKeyboardState(NULL);
		auto it = stringToCodes.left.find(name);

		if(it == stringToCodes.left.end()) { return false; }
    	return keyboard[it->second];
	});

	table.set_function("pumpEvents", []() -> void { SDL_PumpEvents(); });
	return true;
}

LINK_SOL_USERTYPE(CWindowManager);
LINK_COMPONENT_TO_CLASS(CWindowManager, windowmanager);