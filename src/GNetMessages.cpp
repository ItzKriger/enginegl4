#include "GNetMessages.h"
#include "CEngine.h"

void CNetChangeWorld::V_Write(CBufferWrapper& wrapper)
{
    wrapper.Write<std::uint16_t>(WorldID);
}

void CNetChangeWorld::V_Read(CBufferWrapper& wrapper)
{
    WorldID = wrapper.Read<std::uint16_t>();
}

void CNetChangeWorld::Process()
{

}

void CNetServerDisconnect::V_Write(CBufferWrapper& wrapper)
{

}

void CNetServerDisconnect::V_Read(CBufferWrapper& wrapper)
{

}

void CNetServerDisconnect::Process()
{

}

void CNetClientDisconnect::V_Write(CBufferWrapper& wrapper)
{
    
}

void CNetClientDisconnect::V_Read(CBufferWrapper& wrapper)
{

}

void CNetClientDisconnect::Process()
{

}

void CNetKeepAlive::V_Write(CBufferWrapper& wrapper)
{
    
}

void CNetKeepAlive::V_Read(CBufferWrapper& wrapper)
{

}

void CNetKeepAlive::Process()
{

}


LINK_NET_MESSAGE_TO_CLASS(CNetChangeWorld, changeworld);
LINK_NET_MESSAGE_TO_CLASS(CNetServerDisconnect, serverdisconnect);
LINK_NET_MESSAGE_TO_CLASS(CNetClientDisconnect, clientdisconnect);
LINK_NET_MESSAGE_TO_CLASS(CNetKeepAlive, keepalive);