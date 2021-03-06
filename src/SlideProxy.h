////////////////////////////////////////////////////////////
//
// DAGON - An Adventure Game Engine
// Copyright (c) 2011-2014 Senscape s.r.l.
// All rights reserved.
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
//
////////////////////////////////////////////////////////////

#ifndef DAGON_SLIDEPROXY_H_
#define DAGON_SLIDEPROXY_H_

////////////////////////////////////////////////////////////
// NOTE: This header file should never be included directly.
// It's auto-included by Proxy.h
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "Control.h"
#include "Node.h"
#include "Script.h"
#include "Spot.h"
#include "Texture.h"

namespace dagon {

////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////

class SlideProxy : public ObjectProxy {
public:
  static const char className[];
  static Luna<SlideProxy>::RegType methods[];
  
  // Constructor
  SlideProxy(lua_State *L) {
    n = new Node;
    n->setName(luaL_checkstring(L, 1));
    
    n->setType(kObjectSlide);
    n->setSlide(true);
    
    if (lua_gettop(L) == 2)
      n->setDescription(luaL_checkstring(L, 2));
    
    // Image
    std::vector<int> arrayOfCoordinates;
    int coordsImage[] = {64, 424};
    arrayOfCoordinates.assign(coordsImage, coordsImage + 2);
    
    Spot* spot = new Spot(arrayOfCoordinates, kNorth, kSpotClass);
    
    Texture* texture = new Texture;
    texture->setResource(Config::instance().path(kPathResources, luaL_checkstring(L, 1), kObjectImage).c_str());
    spot->setTexture(texture);
    
    n->addSpot(spot);
    
    // Back
    if (lua_toboolean(L, 3) != true) {
      int coordsBack[] = {0, 1280, 2048, 1280, 2048, 1624, 0, 1624};
      arrayOfCoordinates.assign(coordsBack, coordsBack + 8);
      spot = new Spot(arrayOfCoordinates, kNorth, kSpotClass);
      
      Action action;
      action.type = kActionSwitch;
      action.cursor = kCursorBackward;
      action.target = NULL; // Current node
      
      spot->setAction(&action);
      spot->setColor(0);
      
      n->addSpot(spot);
    }
    
    // Register the node in the controller (generates the bundle)
    Control::instance().registerObject(n);
    
    if (Script::instance().isExecutingModule()) {
      // A module is being executed, so we know a room
      // object of the same name exists
      
      lua_getglobal(L, Script::instance().module());
      Room* r = ProxyToRoom(L, -1);
      
      // We add the node automatically
      r->addNode(n);
    }
    
    // Init the base class
    this->setObject(n);
  }
  
  // Destructor
  ~SlideProxy() { delete n; }
  
  // Add a spot to the node
  int addSpot(lua_State *L) {
    if (DGCheckProxy(L, 1) == kObjectSpot) {
      Spot* spot = ProxyToSpot(L, 1);
      spot->setOrigin(64, 424);
      n->addSpot(spot);
      
      // Now we get the metatable of the added spot and set it
      // as a return value
      lua_getfield(L, LUA_REGISTRYINDEX, SpotProxyName);
      lua_setmetatable(L, -2);
      
      return 1;
    }
    
    return 0;
  }
  
  // Process a custom action
  int onReturn(lua_State *L) {
    if (lua_isfunction(L, 1)) {
      n->setSlideReturn(luaL_ref(L, LUA_REGISTRYINDEX));
    }
    
    return 0;
  }
  
  Node* ptr() { return n; }
  
private:
  Node* n;
};

////////////////////////////////////////////////////////////
// Static definitions
////////////////////////////////////////////////////////////

const char SlideProxy::className[] = SlideProxyName;

Luna<SlideProxy>::RegType SlideProxy::methods[] = {
  ObjectMethods(SlideProxy),
  method(SlideProxy, addSpot),
  method(SlideProxy, onReturn),
  {0,0}
};
  
}

#endif // DAGON_SLIDEPROXY_H_
