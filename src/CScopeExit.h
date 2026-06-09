#pragma once
#include <functional>

class CScopeExit
{
public:
    CScopeExit() = default;
    explicit CScopeExit(std::function<void()> f);
    ~CScopeExit();

    void SetFunction(std::function<void()> f);

    void Dismiss();
    void ReEnable();
    void ToggleActive();
    bool IsActive() const;
private:
    std::function<void()> func;
    bool active = true;
};
