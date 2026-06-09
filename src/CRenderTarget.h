#pragma once

class CRenderTarget
{
public:
    virtual void Bind() = 0;
    virtual void Unbind() = 0;
};
