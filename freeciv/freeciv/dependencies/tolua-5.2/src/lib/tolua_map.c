/* tolua: functions to map features
 ** Support code for Lua bindings.
 ** Written by Waldemar Celes
 ** TeCGraf/PUC-Rio
 ** Apr 2003
 ** $Id: tolua_map.c,v 1.10 2011/01/13 13:43:46 fabraham Exp $
 */

/* This code is free software; you can redistribute it and/or modify it.
 ** The software provided hereunder is on an "as is" basis, and
 ** the author has no obligation to provide maintenance, support, updates,
 ** enhancements, or modifications.
 */

#include "tolua.h"
#include "tolua_event.h"
#include "lauxlib.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int tolua_bnd_takeownership(lua_State *L);

static char toluaname[128] = "tolua.";
static const char* TOLUANAME (const char* n)  
{
  sprintf(&toluaname[6],"%.120s",n);
  return toluaname;
}

/* Create metatable
 * Create and register new metatable
 */
void tolua_newmetatable (lua_State* L, const char* name)
{
  if (luaL_newmetatable(L,TOLUANAME(name)))
  {
    lua_pushvalue(L,-1);
    lua_pushstring(L,name);
    lua_rawset(L,LUA_REGISTRYINDEX);
  }

  /* set meta events */
  tolua_classevents(L);
  lua_pop(L,1);
}

/* Get metatable
 * Access already created metatable
 */
void tolua_getmetatable (lua_State* L, const char* name)
{
  luaL_getmetatable(L,TOLUANAME(name));
}

/* Map super classes
 * It sets 'name' as being also a 'base', mapping all super classes of 'base' in 'name'
 */
static void mapsuper (lua_State* L, const char* name, const char* base)
{
  /* push registry.super */
  lua_pushstring(L,"tolua_super");
  lua_rawget(L,LUA_REGISTRYINDEX);    /* stack: super */
  tolua_getmetatable(L,name);          /* stack: super mt */
  lua_rawget(L,-2);                   /* stack: super table */
  if (lua_isnil(L,-1))
  {
    /* create table */
    lua_pop(L,1);
    lua_newtable(L);                    /* stack: super table */
    tolua_getmetatable(L,name);          /* stack: super table mt */
    lua_pushvalue(L,-2);                /* stack: super table mt table */
    lua_rawset(L,-4);                   /* stack: super table */
  }

  /* set base as super class */
  lua_pushstring(L,base);
  lua_pushboolean(L,1);
  lua_rawset(L,-3);                    /* stack: super table */

  /* set all super class of base as super class of name */
  tolua_getmetatable(L,base);          /* stack: super table base_mt */
  lua_rawget(L,-3);                   /* stack: super table base_table */
  if (lua_istable(L,-1))
  {
    /* traverse base table */
    lua_pushnil(L);  /* first key */
    while (lua_next(L,-2) != 0)
    {
      /* stack: ... base_table key value */
      lua_pushvalue(L,-2);    /* stack: ... base_table key value key */
      lua_insert(L,-2);       /* stack: ... base_table key key value */
      lua_rawset(L,-5);       /* stack: ... base_table key */
    }
  }
  lua_pop(L,3);                       /* stack: <empty> */
}


/* Map inheritance
 * It sets 'name' as derived from 'base' by setting 'base' as metatable of 'name'
 */
static void mapinheritance (lua_State* L, const char* name, const char* base)
{
  /* set metatable inheritance */
  tolua_getmetatable(L,name);
  if (base && *base)
    tolua_getmetatable(L,base);
  else
    tolua_getmetatable(L,"tolua_commonclass");
  lua_setmetatable(L,-2);
  lua_pop(L,1);
}

/* Object type
 */
static int tolua_bnd_type (lua_State* L)
{
  tolua_typename(L,lua_gettop(L));
  return 1;
}

/* Take ownership
 */
int tolua_bnd_takeownership (lua_State* L)
{
  lua_CFunction func = 0;
  if (lua_isuserdata(L,1))
  {
    if (lua_getmetatable(L,1))        /* if metatable? */
    {
      void* u;
      lua_pushstring(L,".collector");
      lua_rawget(L,-2);
      func = lua_iscfunction(L,-1) ? lua_tocfunction(L,-1) : NULL; 
      lua_pop(L,2);
      u = *((void**)lua_touserdata(L,1));
      tolua_clone(L,u,func);
    }
  }
  lua_pushboolean(L,func!=0);
  return 1;
}

/* Release ownership
 */
static int tolua_bnd_releaseownership (lua_State* L)
{
  int done = 0;
  if (lua_isuserdata(L,1))
  {
    void* u = *((void**)lua_touserdata(L,1));
    lua_pushstring(L,"tolua_gc");
    lua_rawget(L,LUA_REGISTRYINDEX);
    lua_pushlightuserdata(L,u);
    lua_rawget(L,-2);
    lua_pushlightuserdata(L,u);
    lua_pushnil(L);
    lua_rawset(L,-4);
    done = 1;
  }
  lua_pushboolean(L,done!=0);
  return 1;
}

/* Type casting
 */
static int tolua_bnd_cast (lua_State* L)
{
  void* v = tolua_tousertype(L,1,NULL);
  const char* s = tolua_tostring(L,2,NULL);
  if (!v)
    lua_pushnil(L);
  else if (v && s) {
    tolua_getmetatable(L,s);             /* stack: ubox[u] super super[mt] flag mt */
    if (lua_isnil(L,-1)) {
      tolua_error(L,"Unknown 'type' for 'tolua.cast' function",NULL);
    }
    tolua_pushusertype(L,v,s);
  }
  else {
    tolua_error(L,"Invalid arguments for 'tolua.cast' function",NULL);
  }
  return 1;
}

/* Release
** Function to be called by a Lua code that uses a function to explicitly
** release a mapped object. This function is automatically called by all
** destructors bound by tolua and by all collected objects.
*/
static int tolua_bnd_release (lua_State* L)
{
  void* value = tolua_tousertype(L,1,NULL);
  if (value)
    tolua_release(L,value);
  return 1;
}

/*
** Function to return peer table associated to a given object
*/
static int tolua_bnd_getpeertable (lua_State* L)
{
  lua_pushstring(L,"tolua_peer");
  lua_rawget(L,LUA_REGISTRYINDEX);        /* stack: k v peer */
  lua_pushvalue(L,1);
  lua_rawget(L,-2);                       /* stack: k v peer peer[u] */
  if (!lua_istable(L,-1))
  {
    lua_pop(L,1);                          /* stack: k v peer */
    lua_newtable(L);                       /* stack: k v peer table */
    lua_pushvalue(L,1);
    lua_pushvalue(L,-2);                   /* stack: k v peer table u table */
    lua_rawset(L,-4);                      /* stack: k v peer peer[u]=table */
  }
  return 1;
}

static void tolua_push_globals_table (lua_State* L)
{
#if defined (LUA_VERSION_NUM) && LUA_VERSION_NUM >= 502 /* after lua 5.2 */
  lua_pushvalue(L,LUA_REGISTRYINDEX); /* registry */
  lua_pushnumber(L,LUA_RIDX_GLOBALS); /* registry globalsindex */
  lua_rawget(L, -2);                  /* registry registry[globalsindex] */
  lua_remove(L, -2);                  /* registry[globalsindex] */
#else
  lua_pushvalue(L,LUA_GLOBALSINDEX);
#endif
}

TOLUA_API void tolua_open (lua_State* L)
{
  int top = lua_gettop(L);
  lua_pushstring(L,"tolua_opened");
  lua_rawget(L,LUA_REGISTRYINDEX);
  if (!lua_isboolean(L,-1))
  {
    lua_pushstring(L,"tolua_opened"); lua_pushboolean(L,1); lua_rawset(L,LUA_REGISTRYINDEX);
    lua_pushstring(L,"tolua_ubox"); 
    lua_newtable(L); 
    lua_pushvalue(L, -1); /* metatable: for weak table */
    lua_pushstring(L, "__mode");
    lua_pushstring(L, "v");
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
    lua_rawset(L,LUA_REGISTRYINDEX);
    lua_pushstring(L,"tolua_peer"); 
    lua_newtable(L); 
    lua_pushvalue(L, -1); /* metatable: for weak table */
    lua_pushstring(L, "__mode");
    lua_pushstring(L, "k");
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
    lua_rawset(L,LUA_REGISTRYINDEX);
    lua_pushstring(L,"tolua_super"); lua_newtable(L); lua_rawset(L,LUA_REGISTRYINDEX);
    lua_pushstring(L,"tolua_gc"); lua_newtable(L); lua_rawset(L,LUA_REGISTRYINDEX);

    tolua_newmetatable(L,"tolua_commonclass");

    tolua_module(L,NULL,0);
    tolua_beginmodule(L,NULL);
    tolua_module(L,"tolua",0);
    tolua_beginmodule(L,"tolua");
    tolua_function(L,"type",tolua_bnd_type);
    tolua_function(L,"takeownership",tolua_bnd_takeownership);
    tolua_function(L,"releaseownership",tolua_bnd_releaseownership);
    tolua_function(L,"cast",tolua_bnd_cast);
    tolua_function(L,"release",tolua_bnd_release);
    tolua_function(L,"getpeertable",tolua_bnd_getpeertable);
    tolua_endmodule(L);
    tolua_endmodule(L);
  }
  lua_settop(L,top);
}

/* Copy a C object
 */
TOLUA_API void* tolua_copy (lua_State* L, void* value, unsigned int size)
{
  void* clone = (void*)malloc(size);
  if (clone)
    memcpy(clone,value,size);
  else
    tolua_error(L,"insuficient memory",NULL);
  return clone;
}


/* Release userdata from tolua
 */
TOLUA_API void tolua_release (lua_State* L, void* value)
{
  void** p;
  lua_pushstring(L,"tolua_ubox");
  lua_rawget(L,LUA_REGISTRYINDEX); /* stack: ubox */
  lua_pushlightuserdata(L,value);  /* stack: ubox u */
  /* set userdata pointer to NULL: this pointer might be reused by C/C++ */
  lua_rawget(L,-2);                /* stack: ubox ud */
  p = (void**)lua_touserdata(L,-1);
  if (p) *p = NULL;
      /* fixed bug: thanks to Ulrik Sverdrup -- it was 1 instead of -1 */
  lua_pop(L,1);                    /* stack: ubox */
  /* remove value from ubox */
  lua_pushlightuserdata(L,value);  /* stack: ubox u */
  lua_pushnil(L);
  lua_rawset(L,-3);
  lua_pop(L,1);
}

/* Do clone
 */
TOLUA_API void* tolua_clone (lua_State* L, void* value, lua_CFunction func)
{
  lua_pushstring(L,"tolua_gc");
  lua_rawget(L,LUA_REGISTRYINDEX);
  lua_pushlightuserdata(L,value);
  lua_pushcfunction(L,func);
  lua_rawset(L,-3);
  lua_pop(L,1);
  return value;
}

/* Register a usertype
 * It creates the correspoding metatable in the registry, for both 'type' and 'const type'.
 * It maps 'const type' as being also a 'type'
 */
TOLUA_API void tolua_usertype (lua_State* L, const char* type)
{
  char ctype[128] = "const ";
  strncat(ctype,type,120);

  tolua_newmetatable(L,ctype);         /* create both metatables */
  tolua_newmetatable(L,type);

  mapsuper(L,type,ctype);             /* 'type' is also a 'const type' */
}


/* Begin module
 * It pushes the module (or class) table on the stack
 */
TOLUA_API void tolua_beginmodule (lua_State* L, const char* name)
{
  if (name)
  {
    lua_pushstring(L,name);
    lua_rawget(L,-2);
  }
  else
    tolua_push_globals_table(L);
}

/* End module
 * It pops the module (or class) from the stack
 */
TOLUA_API void tolua_endmodule (lua_State* L)
{
  lua_pop(L,1);
}

/* Map module
 * It creates a new module
 */
#if 1
TOLUA_API void tolua_module (lua_State* L, const char* name, int hasvar)
{
  if (name)
  {
    /* tolua module */
    lua_pushstring(L,name);
    lua_rawget(L,-2);
    if (!lua_istable(L,-1))  /* check if module already exists */
    {
      lua_pop(L,1);
      lua_newtable(L);
      lua_pushstring(L,name);
      lua_pushvalue(L,-2);
      lua_rawset(L,-4);       /* assing module into module */
    }
  }
  else
    tolua_push_globals_table(L);
  if (hasvar)
  {
    if (!tolua_ismodulemetatable(L))  /* check if it already has a module metatable */
    {
      /* create metatable to get/set C/C++ variable */
      lua_newtable(L);
      tolua_moduleevents(L);
      if (lua_getmetatable(L,-2))
        lua_setmetatable(L,-2);  /* set old metatable as metatable of metatable */
      lua_setmetatable(L,-2);
    }
  }
  lua_pop(L,1);               /* pop module */
}
#else
TOLUA_API void tolua_module (lua_State* L, const char* name, int hasvar)
{
  if (name)
  {
    /* tolua module */
    lua_pushstring(L,name);
    lua_newtable(L);
  }
  else
    tolua_push_globals_table(L);
  if (hasvar)
  {
    /* create metatable to get/set C/C++ variable */
    lua_newtable(L);
    tolua_moduleevents(L);
    if (lua_getmetatable(L,-2))
      lua_setmetatable(L,-2);  /* set old metatable as metatable of metatable */
    lua_setmetatable(L,-2);
  }
  if (name)
    lua_rawset(L,-3);       /* assing module into module */
  else
    lua_pop(L,1);           /* pop global table */
}
#endif

/* Map C class
 * It maps a C class, setting the appropriate inheritance and super classes.
 */
TOLUA_API void tolua_cclass (lua_State* L, const char* lname, const char* name, const char* base, lua_CFunction col)
{
  char cname[128] = "const ";
  char cbase[128] = "const ";
  strncat(cname,name,120);
  strncat(cbase,base,120);

  mapinheritance(L,name,base);
  mapinheritance(L,cname,name);

  mapsuper(L,cname,cbase);
  mapsuper(L,name,base);

  lua_pushstring(L,lname);
  tolua_getmetatable(L,name);
  lua_pushstring(L,".collector");
  lua_pushcfunction(L,col);
  lua_rawset(L,-3);              /* store collector function into metatable */
  lua_rawset(L,-3);              /* assign class metatable to module */
}

/* Map function
 * It assigns a function into the current module (or class)
 */
TOLUA_API void tolua_function (lua_State* L, const char* name, lua_CFunction func)
{
  lua_pushstring(L,name);
  lua_pushcfunction(L,func);
  lua_rawset(L,-3);
}

/* Map constant number
 * It assigns a constant number into the current module (or class)
 */
TOLUA_API void tolua_constant (lua_State* L, const char* name, double value)
{
  lua_pushstring(L,name);
  tolua_pushnumber(L,value);
  lua_rawset(L,-3);
}


/* Map variable
 * It assigns a variable into the current module (or class)
 */
TOLUA_API void tolua_variable (lua_State* L, const char* name, lua_CFunction get, lua_CFunction set)
{
  /* get func */
  lua_pushstring(L,".get");
  lua_rawget(L,-2);
  if (!lua_istable(L,-1))
  {
    /* create .get table, leaving it at the top */
    lua_pop(L,1);
    lua_newtable(L);
    lua_pushstring(L,".get");
    lua_pushvalue(L,-2);
    lua_rawset(L,-4);
  }
  lua_pushstring(L,name);
  lua_pushcfunction(L,get);
  lua_rawset(L,-3);                  /* store variable */
  lua_pop(L,1);                      /* pop .get table */

  /* set func */
  if (set)
  {
    lua_pushstring(L,".set");
    lua_rawget(L,-2);
    if (!lua_istable(L,-1))
    {
      /* create .set table, leaving it at the top */
      lua_pop(L,1);
      lua_newtable(L);
      lua_pushstring(L,".set");
      lua_pushvalue(L,-2);
      lua_rawset(L,-4);
    }
    lua_pushstring(L,name);
    lua_pushcfunction(L,set);
    lua_rawset(L,-3);                  /* store variable */
    lua_pop(L,1);                      /* pop .set table */
  }
}

/* Access const array
 * It reports an error when trying to write into a const array
 */
static int const_array (lua_State* L)
{
  luaL_error(L,"value of const array cannot be changed");
  return 0;
}

/* Map an array
 * It assigns an array into the current module (or class)
 */
TOLUA_API void tolua_array (lua_State* L, const char* name, lua_CFunction get, lua_CFunction set)
{
  lua_pushstring(L,".get");
  lua_rawget(L,-2);
  if (!lua_istable(L,-1))
  {
    /* create .get table, leaving it at the top */
    lua_pop(L,1);
    lua_newtable(L);
    lua_pushstring(L,".get");
    lua_pushvalue(L,-2);
    lua_rawset(L,-4);
  }
  lua_pushstring(L,name);

  lua_newtable(L);           /* create array metatable */
  lua_pushvalue(L,-1);
  lua_setmetatable(L,-2);    /* set the own table as metatable (for modules) */
  lua_pushstring(L,"__index");
  lua_pushcfunction(L,get);
  lua_rawset(L,-3);
  lua_pushstring(L,"__newindex");
  lua_pushcfunction(L,set?set:const_array);
  lua_rawset(L,-3);

  lua_rawset(L,-3);                  /* store variable */
  lua_pop(L,1);                      /* pop .get table */
}

