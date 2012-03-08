////////////////////////////////////////////////////////////
//
// DAGON - An Adventure Game Engine
// Copyright (c) 2011 Senscape s.r.l.
// All rights reserved.
//
// NOTICE: Senscape permits you to use, modify, and
// distribute this file in accordance with the terms of the
// license agreement accompanying it.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "DGConfig.h"
#include "DGControl.h"
#include "DGFeedManager.h"
#include "DGLog.h"
#include "DGProxy.h"
#include "DGScript.h"
#include "DGSystem.h"
#include "DGTimerManager.h"
#include "Luna.h"

using namespace std;

// The way the engine is designed, all static Lua functions will
// have to grab a reference to the control singleton and log if necessary
// No performance hit but something to keep in mind

// Spots launch audio and video, then perform a switch in that order
// To avoid doing too many customs

////////////////////////////////////////////////////////////
// Implementation - Constructor
////////////////////////////////////////////////////////////

DGScript::DGScript() {
    log = &DGLog::getInstance();
    config = &DGConfig::getInstance();    
    system = &DGSystem::getInstance();
    
    _isInitialized = false;
}

////////////////////////////////////////////////////////////
// Implementation - Destructor
////////////////////////////////////////////////////////////

DGScript::~DGScript() {
    if (_isInitialized)
        lua_close(_L);
}

////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////

// TODO: Support loading script from parameters
// TODO: Consider seeking paths again if debug mode was enabled
void DGScript::init(int argc, char* argv[]) {
    char script[DGMaxFileLength];
    
    // First thing we do is get the paths to load the script
    // (note that it's not necessary to init the system)
    system->findPaths(argc, argv);
    
    _L = lua_open();
    luaL_openlibs(_L);
    
    // The following code attempts to load a config file, and if it does exist
    // copy the created table to the DGConfig metatable
    if (luaL_loadfile(_L, config->path(DGPathUser, DGDefConfigFile)) == 0) {
		lua_newtable(_L);
		lua_pushvalue(_L, -1);
		int ref = lua_ref(_L, LUA_REGISTRYINDEX);
		lua_setfenv(_L, -2);
		
		lua_pcall(_L, 0, 0, 0);
		
		lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
		lua_pushnil(_L);
		while (lua_next(_L, 1) != 0) {
			DGConfigLibSet(_L);
			
			lua_pop(_L, 1);
		}
	}
    
    lua_getglobal(_L, "_G");
    
    // Push all enum values
    DGLuaEnum(_L, NORTH, DGNorth);
    DGLuaEnum(_L, EAST, DGEast);
    DGLuaEnum(_L, WEST, DGWest);
    DGLuaEnum(_L, SOUTH, DGSouth);
    DGLuaEnum(_L, UP, DGUp);
    DGLuaEnum(_L, DOWN, DGDown);
    DGLuaEnum(_L, NORTHEAST, DGNorthEast);
    DGLuaEnum(_L, SOUTHEAST, DGSouthEast);
    DGLuaEnum(_L, NORTHWEST, DGNorthWest);
    DGLuaEnum(_L, SOUTHWEST, DGSouthWest);

    DGLuaEnum(_L, AUDIO, DGObjectAudio);
    DGLuaEnum(_L, CUSTOM, DGActionCustom);
    DGLuaEnum(_L, IMAGE, DGObjectTexture);
    DGLuaEnum(_L, FEED, DGActionFeed);
    DGLuaEnum(_L, SWITCH, DGActionSwitch);
    DGLuaEnum(_L, VIDEO, DGObjectVideo);
    
    // Register all proxys
    Luna<DGNodeProxy>::Register(_L);
    Luna<DGRoomProxy>::Register(_L);
    Luna<DGSpotProxy>::Register(_L);
    
    // Register all libs
    luaL_register(_L, "system", DGSystemLib);
    
    // The config lib requires a special treatment because
    // it exports properties, not methods
    lua_newuserdata(_L, sizeof(void*));
    
    lua_pushvalue(_L, -1);
	
	luaL_newmetatable(_L, "DGConfigLib");
	luaL_register(_L, NULL, DGConfigLib);
	lua_setmetatable(_L, -2);
	
	lua_newtable(_L);
    lua_setfenv(_L, -2);
	
	lua_setglobal(_L, "config");
    
    // Now we register the global functions that don't belong to any library
    _registerGlobals();
    
    // If autorun is enabled, automatically init the system
    if (config->autorun)
        system->init();
    
    // We're ready to roll, let's attempt to load the script
    
    snprintf(script, DGMaxFileLength, "%s.lua", config->script());
    if (luaL_loadfile(_L, config->path(DGPathApp, script)) == 0)
        _isInitialized = true;
    else
        // Not found!
        log->error(DGModScript, "%s: %s", DGMsg250003, script);
}

void DGScript::callback(int handler, int object) {
    if (object) {
        // Grab the reference to the Lua object and set it as 'self'
        lua_rawgeti(_L, LUA_REGISTRYINDEX, object);
        lua_setglobal(_L, "self");
    }
    
    lua_rawgeti(_L, LUA_REGISTRYINDEX, handler);
    
    if (int result = lua_pcall(_L, 0, 0, 0))
        _error(result);
}

const char* DGScript::module() {
    return _arrayOfModuleNames.back().c_str();
}

bool DGScript::isExecutingModule() {
    // We simply test if the vector is empty. If it is,
    // it means no modules are stacked.
    return !_arrayOfModuleNames.empty();
}

void DGScript::run() {
    if (_isInitialized) {
        if (int result = lua_pcall(_L, 0, 0, 0))
            _error(result);

        // Check if we must start the main loop ourselves
        if (config->autorun)
            system->run();
    }
}

void DGScript::setModule(const char* module) {
    _arrayOfModuleNames.push_back(module);
}

void DGScript::unsetModule() {
    _arrayOfModuleNames.pop_back();
}

////////////////////////////////////////////////////////////
// Implementation - Private methods
////////////////////////////////////////////////////////////

void DGScript::_error(int result) {
    switch (result) {
        case LUA_ERRRUN:
            log->error(DGModScript, "%s", DGMsg250007);
            break;
        case LUA_ERRMEM:
            log->error(DGModScript, "%s", DGMsg250008);
            break;
        case LUA_ERRERR:
            log->error(DGModScript, "%s", DGMsg250009);
            break;
    }
    
    // Now print the last Lua string in the stack, which should indicate the error
    log->error(DGModScript, "%s", lua_tostring(_L, -1));
}

int DGScript::_globalFeed(lua_State *L) {
    DGFeedManager::getInstance().parse(luaL_checkstring(L, 1));
	
	return 0;
}

int DGScript::_globalRoom(lua_State *L) {
    // NOTE: This is a convenience Lua hack, which in theory is 100% safe.
    // Must do losts of testing. Basically, this helper creates a room instance,
    // loads the corresponding Lua file and makes sure it's never loaded twice.
    
    char module[DGMaxObjectName];
    strncpy(module, luaL_checkstring(L, 1), DGMaxObjectName);
    
    // We first check if the object already exists
    lua_getglobal(L, module);
    if (!lua_isuserdata(L, -1)) {
        char line[DGMaxLogLength], script[DGMaxFileLength];
        
        // Parse line to automatically create the room
        snprintf(line, DGMaxLogLength, "%s = Room(\"%s\")", module, module);
        luaL_dostring(L, line);
        
        // Load the corresponding Lua file
        // TODO: Read rooms from path
        snprintf(script, DGMaxFileLength, "%s.lua", module);
        
        if (luaL_loadfile(L, DGConfig::getInstance().path(DGPathApp, script)) == 0) {
            DGScript::getInstance().setModule(module);
            lua_pcall(L, 0, 0, 0);
            DGScript::getInstance().unsetModule();
        }
    }
    
    // Nothing to do...
    
    return 0;
}

int DGScript::_globalSetFont(lua_State *L) {
    switch ((int)luaL_checknumber(L, 1)) {
        case FEED:
            DGFeedManager::getInstance().setFont(luaL_checkstring(L, 2), luaL_checknumber(L, 3));
            break;
    }
    
    return 0;
}

int DGScript::_globalSwitch(lua_State *L) {
    switch (DGCheckProxy(L, 1)) {
        case DGObjectNode:
            DGControl::getInstance().switchTo(DGProxyToNode(L, 1));
            break;
        case DGObjectRoom:
            DGControl::getInstance().switchTo(DGProxyToRoom(L, 1));
            break;
        case DGObjectGeneric:
            DGLog::getInstance().error(DGModScript, "%s", DGMsg250000);
            break;
            
        case DGObjectNone:
            DGLog::getInstance().error(DGModScript, "%s", DGMsg250001);
            break;
    }
    
	return 0;
}

int DGScript::_globalTimer(lua_State *L) {
	if (!lua_isfunction(L, -1)) {
		DGLog::getInstance().trace(DGModScript, "%s", DGMsg250006);
        
		return 0;
	}
	
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);  // Pop and return a reference to the table.
	int handle = DGTimerManager::getInstance().create(luaL_checknumber(L, 1), ref);
    
    lua_pushnumber(L, handle);
	
	return 1;
}

void DGScript::_registerGlobals() {
    static const struct luaL_reg globalLibs [] = {
        {"feed", _globalFeed},
        {"room", _globalRoom},
        {"setFont", _globalSetFont},        
        {"switch", _globalSwitch},
        {"timer", _globalTimer},
        {NULL, NULL}
    };
    
    lua_getglobal(_L, "_G");
	luaL_register(_L, NULL, globalLibs);
	lua_pop(_L, 1);
}