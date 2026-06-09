#pragma once

class CWrapableBindingBase
{
public:
	virtual void Update() = 0;
    virtual void* GetPointer() = 0;

    bool ReturnCheck = false;
};
