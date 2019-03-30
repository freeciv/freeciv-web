/* tolua: functions to push C values.
 ** Support code for Lua bindings.
 ** Written by Waldemar Celes
 ** TeCGraf/PUC-Rio
 ** Apr 2003
 ** $Id: tolua_push.c,v 1.7 2010/01/22 15:39:29 fabraham Exp $
 */

/* This code is free software; you can redistribute it and/or modify it.
 ** The software provided hereunder is on an "as is" basis, and
 ** the author has no obligation to provide maintenance, support, updates,
 ** enhancements, or modifications.
 */

#include "tolua.h"
#include "lauxlib.h"

#include <stdlib.h>

TOLUA_API void tolua_pushvalue (lua_State* L, int lo)
{
  lua_pushvalue(L,lo);
}

TOLUA_API void tolua_pushboolean (lua_State* L, int value)
{
  lua_pushboolean(L,value);
}

TOLUA_API void tolua_pushnumber (lua_State* L, double value)
{
  lua_pushnumber(L,value);
}

TOLUA_API void tolua_pushstring (lua_State* L, const char* value)
{
  if (value == NULL)
    lua_pushnil(L);
  else
    lua_pushstring(L,value);
}

TOLUA_API void tolua_pushuserdata (lua_State* L, void* value)
{
  if (value == NULL)
    lua_pushnil(L);
  else
    lua_pushlightuserdata(L,value);
}

TOLUA_API void tolua_pushusertype (lua_State* L, void* value, const char* type)
{
  if (value == NULL)
    lua_pushnil(L);
  else
  {
    lua_pushstring(L,"tolua_ubox");
    lua_rawget(L,LUA_REGISTRYINDEX);        /* stack: ubox */
    lua_pushlightuserdata(L,value);
    lua_rawget(L,-2);                       /* stack: ubox ubox[u] */
    if (lua_isnil(L,-1))
    {
      /* creating ubox of value */
      lua_pop(L,1);                          /* stack: ubox */
      lua_pushlightuserdata(L,value);
      *(void**)lua_newuserdata(L,sizeof(void *)) = value;   /* stack: ubox u newud */
      lua_pushvalue(L,-1);                   /* stack: ubox u newud newud */
      lua_insert(L,-4);                      /* stack: newud ubox u newud */
      lua_rawset(L,-3);                      /* stack: newud ubox */
      lua_pop(L,1);                          /* stack: newud */
      tolua_getmetatable(L,type);             /* stack: newud mt */
      lua_setmetatable(L,-2);                /* stack: newud */
    }
    else
    {
      /* reusing ubox of value */
      /* check the need of updating the metatable to a more specialized class */
      lua_insert(L,-2);                       /* stack: ubox[u] ubox */
      lua_pop(L,1);                           /* stack: ubox[u] */
      lua_pushstring(L,"tolua_super");
      lua_rawget(L,LUA_REGISTRYINDEX);        /* stack: ubox[u] super */
      lua_getmetatable(L,-2);                 /* stack: ubox[u] super mt */
      lua_rawget(L,-2);                       /* stack: ubox[u] super super[mt] */
      if (lua_istable(L,-1))
      {
        lua_pushstring(L,type);                 /* stack: ubox[u] super super[mt] type */
        lua_rawget(L,-2);                       /* stack: ubox[u] super super[mt] flag */
        if (lua_toboolean(L,-1) == 1)           /* if true */
        {
          lua_pop(L,3);                         /* stack: ubox[u] */
          return;
        }
      }
      /* type represents a more specilized type */
      tolua_getmetatable(L,type);             /* stack: ubox[u] super super[mt] flag mt */
      lua_setmetatable(L,-5);                /* stack: ubox[u] super super[mt] flag */
      lua_pop(L,3);                          /* stack: ubox[u] */
    }
  }
}

TOLUA_API void tolua_pushfieldvalue (lua_State* L, int lo, int index, int v)
{
  lua_pushnumber(L,index);
  lua_pushvalue(L,v);
  lua_settable(L,lo);
}

TOLUA_API void tolua_pushfieldboolean (lua_State* L, int lo, int index, int v)
{
  lua_pushnumber(L,index);
  lua_pushboolean(L,v);
  lua_settable(L,lo);
}


TOLUA_API void tolua_pushfieldnumber (lua_State* L, int lo, int index, double v)
{
  lua_pushnumber(L,index);
  tolua_pushnumber(L,v);
  lua_settable(L,lo);
}

TOLUA_API void tolua_pushfieldstring (lua_State* L, int lo, int index, const char* v)
{
  lua_pushnumber(L,index);
  tolua_pushstring(L,v);
  lua_settable(L,lo);
}

TOLUA_API void tolua_pushfielduserdata (lua_State* L, int lo, int index, void* v)
{
  lua_pushnumber(L,index);
  tolua_pushuserdata(L,v);
  lua_settable(L,lo);
}

TOLUA_API void tolua_pushfieldusertype (lua_State* L, int lo, int index, void* v, const char* type)
{
  lua_pushnumber(L,index);
  tolua_pushusertype(L,v,type);
  lua_settable(L,lo);
}

