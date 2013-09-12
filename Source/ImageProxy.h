////////////////////////////////////////////////////////////
//
// DAGON - An Adventure Game Engine
// Copyright (c) 2011-2013 Senscape s.r.l.
// All rights reserved.
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
//
////////////////////////////////////////////////////////////

#ifndef DG_IMAGEPROXY_H
#define DG_IMAGEPROXY_H

////////////////////////////////////////////////////////////
// NOTE: This header file should never be included directly.
// It's auto-included by DGProxy.h
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "Image.h"

////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////

class ImageProxy : public ObjectProxy {
public:
  static const char className[];
  static Luna<ImageProxy>::RegType methods[];
  
  // Constructor
  ImageProxy(lua_State *L) {
    i = new Image(luaL_checkstring(L, 1));
    i->setName(luaL_checkstring(L, 1));
    
    // Init the base class
    this->setObject(i);
  }
  
  // Destructor
  ~ImageProxy() { delete i; }
  
  // Move the image
  int move(lua_State *L) {
    i->move(static_cast<int>(luaL_checknumber(L, 1)),
            static_cast<int>(luaL_checknumber(L, 2)));
    return 0;
  }
  
  // Return the position
  int position(lua_State *L) {
    Point pos = i->position();
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    return 2;
  }
  
  // Scale the image
  int scale(lua_State *L) {
    i->scale(static_cast<float>(luaL_checknumber(L, 1)));
    return 0;
  }
  
  // Set a new position
  int setPosition(lua_State *L) {
    i->setPosition(static_cast<int>(luaL_checknumber(L, 1)),
                   static_cast<int>(luaL_checknumber(L, 2)));
    return 0;
  }
  
  // Set a new size
  int setSize(lua_State *L) {
    i->setSize(static_cast<int>(luaL_checknumber(L, 1)),
               static_cast<int>(luaL_checknumber(L, 2)));
    return 0;
  }
  
  // Return the size
  int size(lua_State *L) {
    Size newSize = i->size();
    lua_pushnumber(L, newSize.width);
    lua_pushnumber(L, newSize.height);
    return 2;
  }
  
  Image* ptr() { return i; }
  
private:
  Image* i;
  
};

////////////////////////////////////////////////////////////
// Static definitions
////////////////////////////////////////////////////////////

const char ImageProxy::className[] = ImageProxyName;

Luna<ImageProxy>::RegType ImageProxy::methods[] = {
  ObjectMethods(ImageProxy),
  method(ImageProxy, move),
  method(ImageProxy, position),
  method(ImageProxy, scale),
  method(ImageProxy, setPosition),
  method(ImageProxy, setSize),
  method(ImageProxy, size),
  {0,0}
};

#endif // DG_IMAGEPROXY_H