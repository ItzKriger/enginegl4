#include <iostream>
#include <string>

#include "CEngine.h"

#ifdef _WIN32
#include <windows.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

//TODO lua scripts preprocessor (to advance the syntax)
//TODO base/same class for custom classes (components/entities/entity components) with maps of functions in generic components
//TODO add script classes support to objectfactory
//TODO u16string instead wstring to unify linux & windows (only on networking)
//TODO use boost.locale for string conversions
//TODO wstring commandline
//TODO replacing objects in factories so in case user can modify/replace standard components
//TODO LUA coroutines and threads
//TODO LUA vec, quat and mat through proxies [obj], not the values inside table
//TODO all objects transferred to lua should be cached (maybe map with ptr -> table)

//TODO IMPORTANT make de-init (before destructing the engine)

//entities
//networking
//physics
//lighting

//DONE reliable&resent messages
//only server-side entities (without sync)
//only server-side entity components (without sync)
//DONE map<server_ent_id, client_ent_id>
//DONE net message with server info (tickrate)

//stop all entities
//change client world
//disconnect
//kick on timeout
//server timeout
//keepalive packets

//make_destructor (lua) instead of __gc

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
    std::string args(cmdline);
	
    //timeBeginPeriod(1);
    CEngine::Run(args);
    //timeEndPeriod(1);
    std::cout << "Shutdown!\n";
    return 0;
}

#else

int main(int argc, char* argv[])
{
    std::string args;
    for (int i = 1; i < argc; ++i)
    {
        args += argv[i];
        if (i < argc - 1) { args += " "; }
    }
	
    CEngine::Run(args);

    std::cout << "Shutdown!\n";
    return 0;
}

#endif