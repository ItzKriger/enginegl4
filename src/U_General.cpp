#include "U_General.h"
#include "CEngine.h"
#include "CClient.h"
#include "CServer.h"

std::string GetLuaVectorColorIndex(size_t index)
{
    if(index == 1) { return "r"; }
    else if(index == 2) { return "g"; }
    else if(index == 3) { return "b"; }
    else if(index == 4) { return "a"; }
    return {};
}

std::string GetLuaVectorCharIndex(size_t index)
{
    if(index == 1) { return "x"; }
    else if(index == 2) { return "y"; }
    else if(index == 3) { return "z"; }
    else if(index == 4) { return "w"; }
    return {};
}

std::string GetVectorColorIndex(size_t index)
{
    if(index == 0) { return "r"; }
    else if(index == 1) { return "g"; }
    else if(index == 2) { return "b"; }
    else if(index == 3) { return "a"; }
    return {};
}

std::string GetVectorCharIndex(size_t index)
{
    if(index == 0) { return "x"; }
    else if(index == 1) { return "y"; }
    else if(index == 2) { return "z"; }
    else if(index == 3) { return "w"; }
    return {};
}

bool IsClient()
{
    return CEngine::GetInstance()->Components.GetComponentTyped<CClient>();
}

bool IsServer()
{
    return CEngine::GetInstance()->Components.GetComponentTyped<CServer>();
}

bool IsConnectedClient()
{
    auto client = CEngine::GetInstance()->Components.GetComponentTyped<CClient>();
    return client && client->Transceiver.get(); //TODO also check if authorized or not
}
