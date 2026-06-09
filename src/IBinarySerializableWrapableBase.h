#pragma once
#include "CWrapableBase.h"
#include "IBinaryStream.h"

#include <memory>
#include <functional>

class IBinarySerializableWrapableBase : public virtual CWrapableBase
{
public:
    virtual void Write(IBinaryStream& stream) = 0;
    virtual void Read(IBinaryStream& stream) = 0; //TODO const IBinaryStream&

    //TODO custom functions for read/write (unique_ptr<function>)
};
