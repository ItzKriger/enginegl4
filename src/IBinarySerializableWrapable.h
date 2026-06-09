#pragma once
#include "IBinarySerializableWrapableBase.h"
#include "CTypedWrapableBase.h"

template<typename T>
class IBinarySerializableWrapable : public IBinarySerializableWrapableBase, public virtual CTypedWrapableBase<T>
{
public:
    void Write(IBinaryStream& stream) override
    {
        stream.Write<T>(CTypedWrapableBase<T>::GetValue());
    }

    void Read(IBinaryStream& stream) override
    {
        T toset = stream.Read<T>();
        CTypedWrapableBase<T>::SetValue(toset);
    }
};
