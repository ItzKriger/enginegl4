#pragma once
#include "CComponent.h"
#include "CRendererBase.h"
#include "CCallbackHandler.h"

#include "SDL3/SDL.h"
#include "glm/glm.hpp"

#include <cstdint>
#include <memory>

class CWindowManager : public CComponent
{
public:
	CWindowManager();
	~CWindowManager();

	bool V_Init() override;
	void V_Update() override;

	void DestructWindow();
	glm::ivec2 GetWindowSize();
	float GetWindowAspect() const;
	void HandleEvent(const SDL_Event& event);

	DEFINE_SOL_USERTYPE();

	bool V_ScriptInit(std::shared_ptr<sol::state> _state, sol::table table) override;

	static size_t GetWindowFlags(bool windowed, bool noborder, size_t generic_flag);
	SDL_Window* Window = nullptr;

	CCallbackHandler<void> OnDraw;
	CCallbackHandler<void, const SDL_Event&> OnEventHandle;

	std::unique_ptr<CRendererBase> Renderer;
	DEFINE_COMPONENT();
private:
	void m_UpdateAspect();

	SDL_Event m_Event;
	float m_aspect = 1.0f;
};
