#include "CScopeExit.h"

CScopeExit::CScopeExit(std::function<void()> f) : func(std::move(f)), active(true) {}
CScopeExit::~CScopeExit() { if (active && func) { func(); } }
void CScopeExit::Dismiss() { active = false; }
void CScopeExit::ReEnable() { active = true; }
void CScopeExit::ToggleActive() { active = !active; }
bool CScopeExit::IsActive() const { return active; }

void CScopeExit::SetFunction(std::function<void()> f)
{
    func = std::move(f);
}
