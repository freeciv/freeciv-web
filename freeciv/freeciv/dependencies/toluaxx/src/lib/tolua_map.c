/* tolua: functions to map features
** Support code for Lua bindings.
** Written by Waldemar Celes
** TeCGraf/PUC-Rio
** Apr 2003
** $Id: tolua_map.c 14589 2008-04-15 21:40:53Z cazfi $
*/

/* This code is free software; you can redistribute it and/or modify it.
** The software provided hereunder is on an "as is" basis, and
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications.
*/

#include "toluaxx.h"
#include "tolua_event.h"
#include "lauxlib.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static lua_State* __tolua_current_state_=NULL;

/* Call method to self object from C
 */
/*
  TOLUA_API int tolua_callmethod(lua_State* L,void* self,const char* type,const char* func,int nargs,int nresults){
  tolua_pushusertype(L,self,type);
  lua_pushstring(L,func);
  lua_gettable(L,-2);
  if(!lua_isfunction(L,-1)){lua_pop(L,1);return 0;} * method is nil *
  lua_insert(L,-2-nargs);
  lua_insert(L,-1-nargs);
  lua_call(L,nargs+1,nresults);
  return 1;
  }*/

TOLUA_API lua_State* tolua_state(){ /* Get current state  */
  return __tolua_current_state_;
}
TOLUA_API void tolua_setstate(lua_State* L){ /* Set current state */
  __tolua_current_state_=L;
}

/* Create metatable
 * Create and register new metatable
 */
static int tolua_newmetatable (lua_State* L, char* name){
  int r = luaL_newmetatable(L,name);
  
#ifdef LUA_VERSION_NUM /* only lua 5.1 */
  if (r) {
    lua_pushvalue(L, -1);
    lua_pushstring(L, name);
    lua_settable(L, LUA_REGISTRYINDEX); /* reg[mt] = type_name */
  };
#endif
  
  if (r) tolua_classevents(L); /* set meta events */
  lua_pop(L,1);
  return r;
}

/* Map super classes
 * It sets 'name' as being also a 'base', mapping all super classes of 'base' in 'name'
 */
static void mapsuper (lua_State* L, const char* name, const char* base) {
  /* push registry.super */
  lua_pushstring(L,"tolua_super");
  lua_rawget(L,LUA_REGISTRYINDEX);    /* stack: super */
  luaL_getmetatable(L,name);          /* stack: super mt */
  lua_rawget(L,-2);                   /* stack: super table */
  if (lua_isnil(L,-1)) {
    /* create table */
    lua_pop(L,1);
    lua_newtable(L);                    /* stack: super table */
    luaL_getmetatable(L,name);          /* stack: super table mt */
    lua_pushvalue(L,-2);                /* stack: super table mt table */
    lua_rawset(L,-4);                   /* stack: super table */
  }
  
  /* set base as super class */
  lua_pushstring(L,base);
  lua_pushboolean(L,1);
  lua_rawset(L,-3);                    /* stack: super table */
  
  /* set all super class of base as super class of name */
  luaL_getmetatable(L,base);          /* stack: super table base_mt */
  lua_rawget(L,-3);                   /* stack: super table base_table */
  if (lua_istable(L,-1)) {
    /* traverse base table */
    lua_pushnil(L);  /* first key */
    while (lua_next(L,-2) != 0) {
      /* stack: ... base_table key value */
      lua_pushvalue(L,-2);    /* stack: ... base_table key value key */
      lua_insert(L,-2);       /* stack: ... base_table key key value */
      lua_rawset(L,-5);       /* stack: ... base_table key */
    }
  }
  lua_pop(L,3);                       /* stack: <empty> */
}

/* creates a 'tolua_ubox' table for base clases, and
// expects the metatable and base metatable on the stack */
static void set_ubox(lua_State* L) {
  /* mt basemt */
  if (!lua_isnil(L, -1)) {
    lua_pushstring(L, "tolua_ubox");
    lua_rawget(L,-2);
  } else {
    lua_pushnil(L);
  }
  /* mt basemt base_ubox */
  if (!lua_isnil(L,-1)) {
    lua_pushstring(L, "tolua_ubox");
    lua_insert(L, -2);
    /* mt basemt key ubox */
    lua_rawset(L,-4);
    /* (mt with ubox) basemt */
  } else {
    /* mt basemt nil */
    lua_pop(L, 1);
    lua_pushstring(L,"tolua_ubox"); lua_newtable(L);
    /* make weak value metatable for ubox table to allow userdata to be
       garbage-collected */
    lua_newtable(L); lua_pushliteral(L, "__mode"); lua_pushliteral(L, "v"); lua_rawset(L, -3);               /* stack: string ubox mt */
    lua_setmetatable(L, -2);  /* stack:mt basemt string ubox */
    lua_rawset(L,-4);
  }
}

/* Map inheritance
 * It sets 'name' as derived from 'base' by setting 'base' as metatable of 'name'
 */
static void mapinheritance (lua_State* L, const char* name, const char* base){
  /* set metatable inheritance */
  luaL_getmetatable(L,name);
  
  if (base && *base) luaL_getmetatable(L,base); else {
    
    if (lua_getmetatable(L, -1)) { /* already has a mt, we don't overwrite it */
      lua_pop(L, 2);
      return;
    }
    luaL_getmetatable(L,"tolua_commonclass");
  }
  
  set_ubox(L);
  
  lua_setmetatable(L,-2);
  lua_pop(L,1);
}

/* Object type
 */
static int tolua_bnd_type (lua_State* L){
  tolua_typename(L,lua_gettop(L));
  return 1;
}

/* Take ownership
 */
static int tolua_bnd_takeownership (lua_State* L){
  int success = 0;
  if (lua_isuserdata(L,1)) {
    if (lua_getmetatable(L,1)) {       /* if metatable? */
      lua_pop(L,1);             /* clear metatable off stack */
      /* force garbage collection to avoid C to reuse a to-be-collected address */
#ifdef LUA_VERSION_NUM
      lua_gc(L, LUA_GCCOLLECT, 0);
#else
      lua_setgcthreshold(L,0);
#endif
      success = tolua_register_gc(L,1);
    }
  }
  lua_pushboolean(L,success!=0);
  return 1;
}

/* Release ownership
 */
static int tolua_bnd_releaseownership (lua_State* L){
  int done = 0;
  if (lua_isuserdata(L,1)) {
    void* u = *((void**)lua_touserdata(L,1));
    /* force garbage collection to avoid releasing a to-be-collected address */
#ifdef LUA_VERSION_NUM
    lua_gc(L, LUA_GCCOLLECT, 0);
#else
    lua_setgcthreshold(L,0);
#endif
    lua_pushstring(L,"tolua_gc");
    lua_rawget(L,LUA_REGISTRYINDEX);
    lua_pushlightuserdata(L,u);
    lua_rawget(L,-2);
    lua_getmetatable(L,1);
    if (lua_rawequal(L,-1,-2)) {  /* check that we are releasing the correct type */
      lua_pushlightuserdata(L,u);
      lua_pushnil(L);
      lua_rawset(L,-5);
      done = 1;
    }
  }
  lua_pushboolean(L,done!=0);
  return 1;
}

/* Type casting
 */
static int tolua_bnd_cast (lua_State* L){
  
  /* // old code
     void* v = tolua_tousertype(L,1,NULL);
     const char* s = tolua_tostring(L,2,NULL);
     if (v && s)
     tolua_pushusertype(L,v,s);
     else
     lua_pushnil(L);
     return 1;
  */
  
  void* v;
  const char* s;
  if (lua_islightuserdata(L, 1)) {
    v = tolua_touserdata(L, 1, NULL);
  } else {
    v = tolua_tousertype(L, 1, 0);
  }
  
  s = tolua_tostring(L,2,NULL);
  if (v && s) tolua_pushusertype(L,v,s);
  else lua_pushnil(L);
  return 1;
}

/* Inheritance
 */
static int tolua_bnd_inherit (lua_State* L) {

  /* stack: lua object, c object */
  lua_pushstring(L, ".c_instance");
  lua_pushvalue(L, -2);
  lua_rawset(L, -4);
  /* l_obj[".c_instance"] = c_obj */

  return 0;
}

#ifdef LUA_VERSION_NUM /* lua 5.1 */
static int tolua_bnd_setpeer(lua_State* L) {
  /* stack: userdata, table */
  if (!lua_isuserdata(L, -2)) {
    lua_pushstring(L, "Invalid argument #1 to setpeer: userdata expected.");
    lua_error(L);
  }
  
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    lua_pushvalue(L, TOLUA_NOPEER);
  }
  lua_setfenv(L, -2);
  
  return 0;
}

static int tolua_bnd_getpeer(lua_State* L) {
  /* stack: userdata */
  lua_getfenv(L, -1);
  if (lua_rawequal(L, -1, TOLUA_NOPEER)) {
    lua_pop(L, 1);
    lua_pushnil(L);
  }
  return 1;
}
#endif

/*
 *  toluaxx proxy technique
 *  
 *
 */

TOLUA_API void tolua_proxystack(lua_State* L){
  /* stack: */
  lua_pushstring(L,"tolua_proxy");  /* stack: "tolua_proxy" */
  lua_rawget(L,LUA_REGISTRYINDEX);  /* stack: ?proxy_stack_table? */
  if(!lua_istable(L,-1)){  /* check if ?proxy_stack_table? is not table */
    lua_remove(L,-1);               /* stack: */
    lua_pushnil(L);                 /* stack: nil */
    return;
  }
  /* stack: proxy_stack_table */
}

TOLUA_API int tolua_proxytop(lua_State* L){
  int __tolua_proxy_top_;
  int __top_;
  __top_=lua_gettop(L);              /* stack: */
  lua_pushstring(L,"tolua_proxy");   /* stack: "tolua_proxy" */
  lua_rawget(L,LUA_REGISTRYINDEX);   /* stack: ?proxy_stack_table? */
  if(lua_istable(L,-1)){    /* check if ?proxy_stack_table? is table */
    lua_pushnumber(L,0);             /* stack: proxy_stack_table 0 */
    lua_rawget(L,-2);                /* stack: proxy_stack_table ?proxy_stack_top? */
    if(lua_isnumber(L,-1)){ /* check if ?proxy_stack_top? is number */
      __tolua_proxy_top_=lua_tonumber(L,-1);
    }else __tolua_proxy_top_=-2;
  }else __tolua_proxy_top_=-1;
  lua_settop(L,__top_);
  return __tolua_proxy_top_;
}

TOLUA_API int tolua_proxypush(lua_State* L){
  int __tolua_proxy_top_;
  
  /* get proxy stack table */           /* stack: */
  lua_pushstring(L,"tolua_proxy");      /* stack: "tolua_proxy" */
  lua_rawget(L,LUA_REGISTRYINDEX);      /* stack: ?proxy_stack_table? */
  if(!lua_istable(L,-1)){  /* check if ?proxy_stack_table? is not table */
    lua_remove(L,-1);                   /* stack: */
    return 0;
  }

  /* get stack level top counter */     /* stack: proxy_stack_table */
  lua_pushnumber(L,0);                  /* stack: proxy_stack_table 0 */
  lua_rawget(L,-2);                     /* stack: proxy_stack_table ?proxy_stack_top? */
  if(!lua_isnumber(L,-1)){ /* check if ?proxy_stack_top? is not number */
    lua_remove(L,-1);                   /* stack: proxy_stack_table */
    lua_remove(L,-1);                   /* stack: */
    return 0;
  }
  __tolua_proxy_top_=lua_tonumber(L,-1);
  lua_remove(L,-1);                     /* stack: proxy_stack_table */

  /* decrement stack level top counter */
  __tolua_proxy_top_=__tolua_proxy_top_+1;
  lua_pushnumber(L,0);                  /* stack: proxy_stack_table 0 */
  lua_pushnumber(L,__tolua_proxy_top_); /* stack: proxy_stack_table 0 new_proxy_stack_top */
  lua_rawset(L,-3);                     /* stack: proxy_stack_table */
  
  /* create a new proxy stack level */
  lua_pushnumber(L,__tolua_proxy_top_); /* stack: proxy_stack_table new_proxy_stack_top */
  lua_newtable(L);                      /* stack: proxy_stack_table new_proxy_stack_level */
  lua_rawset(L,-3);                     /* stack: proxy_stack_table */
  
  /* clear lua stack */
  lua_remove(L,-1);                     /* stack: */
  return 1;
}

TOLUA_API int tolua_proxypop(lua_State* L){
  int __tolua_proxy_top_;
  
  /* get proxy stack table */           /* stack: */
  lua_pushstring(L,"tolua_proxy");      /* stack: "tolua_proxy" */
  lua_rawget(L,LUA_REGISTRYINDEX);      /* stack: ?proxy_stack_table? */
  if(!lua_istable(L,-1)){  /* check if ?proxy_stack_table? is not table */
    lua_remove(L,-1);                   /* stack: */
    return 0;
  }
  
  /* get stack level top counter */     /* stack: proxy_stack_table */
  lua_pushnumber(L,0);                  /* stack: proxy_stack_table 0 */
  lua_rawget(L,-2);                     /* stack: proxy_stack_table ?proxy_stack_top? */
  if(!lua_isnumber(L,-1)){ /* check if ?proxy_stack_top? is not number */
    lua_remove(L,-1);                   /* stack: proxy_stack_table */
    lua_remove(L,-1);                   /* stack: */
    return 0;
  }
  __tolua_proxy_top_=lua_tonumber(L,-1);
  lua_remove(L,-1);                     /* stack: proxy_stack_table */
  if(__tolua_proxy_top_<1){ /* check if proxy top counter is smaller one */
    lua_remove(L,-1);                   /* stack: */
    return 0;
  }
  /* remove proxy stack level */
  lua_pushnumber(L,__tolua_proxy_top_); /* stack: proxy_stack_table proxy_stack_top */
  lua_pushnil(L);                       /* stack: proxy_stack_table  */
  lua_rawset(L,-3);                     /* stack: proxy_stack_table */
  
  /* decrement stack level top counter */
  __tolua_proxy_top_=__tolua_proxy_top_-1;
  lua_pushnumber(L,0);                  /* stack: proxy_stack_table 0 */
  lua_pushnumber(L,__tolua_proxy_top_); /* stack: proxy_stack_table 0 new_proxy_stack_top */
  lua_rawset(L,-3);                     /* stack: proxy_stack_table */
  
  /* clear lua stack */
  lua_remove(L,-1);                     /* stack: */
  return 1;
}

TOLUA_API void tolua_proxylevel(lua_State* L, int level){
  /* stack: */
  int __tolua_proxy_top_=tolua_proxytop(L);
  if( level>+__tolua_proxy_top_ ||
      level<-__tolua_proxy_top_ ||
      level==0 ){ /* check if level exceded */
    lua_pushnil(L);                 /* stack: nil */
    return;
  }                                 /* stack: */
  lua_pushstring(L,"tolua_proxy");  /* stack: "tolua_proxy" */
  lua_rawget(L,LUA_REGISTRYINDEX);  /* stack: ?proxy_stack_table? */
  if(!lua_istable(L,-1)){  /* check if ?proxy_stack_table? is not table */
    lua_remove(L,-1);               /* stack: */
    lua_pushnil(L);                 /* stack: nil */
    return;
  }
  /* stack: proxy_stack_table */
  level=level>0?level:__tolua_proxy_top_+level+1;
  lua_pushnumber(L,level);          /* stack: proxy_stack_table proxy_stack_level */
  lua_rawget(L,-2);                 /* stack: proxy_stack_table ?proxy_level_table? */
  lua_remove(L,-2);                 /* stack: ?proxy_level_table? */
  if(!lua_istable(L,-1)){  /* check if ?proxy_level_table? is not table */
    lua_remove(L,-1);               /* stack: */
    lua_pushnil(L);                 /* stack: nil */
    return;
  }
  return;                           /* stack: proxy_level_table */
}

TOLUA_API void tolua_getproxy(lua_State* L, int level){
  /* stack: key */
  tolua_proxylevel(L,level);        /* stack: key ?proxy_level_table? */
  if(!lua_istable(L,-1)){  /* check if ?proxy_level_table? is not table */
    lua_remove(L,-1);               /* stack: key */
    lua_remove(L,-1);               /* stack: */
    lua_pushnil(L);                 /* stack: nil */
    return;
  }                                 /* stack: key proxy_level_table */
  lua_insert(L,-2);                 /* stack: proxy_level_table key */
  lua_rawget(L,-2);                 /* stack: proxy_level_table value */
  lua_remove(L,-2);                 /* stack: value */
  return;
}

TOLUA_API void tolua_setproxy(lua_State* L, int level){
  /* stack: key val */
  tolua_proxylevel(L,level);        /* stack: key val ?proxy_level_table? */
  if(!lua_istable(L,-1)){  /* check if ?proxy_level_table? is not table */
    lua_remove(L,-1);               /* stack: key val */
    lua_remove(L,-1);               /* stack: key */
    lua_remove(L,-1);               /* stack: */
    return;
  }                                 /* stack: key val proxy_level_table */
  lua_insert(L,-3);                 /* stack: proxy_level_table key val */
  lua_rawset(L,-3);                 /* stack: proxy_level_table */
  lua_remove(L,-2);                 /* stack: */
  return;
}

/* local state = tolua.proxy.push() */
static int tolua_bnd_proxy_push(lua_State* L){
  int __ret_code_;
  __ret_code_=tolua_proxypush(L);
  lua_pushboolean(L,__ret_code_);
  return 1;
}

/* local state = tolua.proxy.pop() */
static int tolua_bnd_proxy_pop(lua_State* L){
  int __ret_code_;
  __ret_code_=tolua_proxypop(L);
  lua_pushboolean(L,__ret_code_);
  return 1;
}

/* local level_table = tolua.proxy.level(<level>) */
static int tolua_bnd_proxy_level(lua_State* L){
  /* stack: <level> */
  int __proxy_level_=TOLUA_PROXY_TOP;
  if(lua_isnumber(L,1)){ /* check if level has been retrieved */
    /* stack: level */
    __proxy_level_=lua_tonumber(L,1);
    lua_remove(L,1);                 /* stack: */
  }                                  /* stack: */
  tolua_proxylevel(L,__proxy_level_);
  return 1;
}

/* local val = tolua.proxy.get(key<,level>) */
static int tolua_bnd_proxy_get(lua_State* L){
  /* stack: key <level> */
  int __proxy_level_=TOLUA_PROXY_TOP;
  if(lua_gettop(L)<1 || lua_isnil(L,1)){  /* check if key hasn't been retrieved */
    lua_pushnil(L);
    return 1;
  }
  if(lua_isnumber(L,2)){ /* check if level has been retrieved */
    /* stack: key level */
    __proxy_level_=lua_tonumber(L,2);
    lua_remove(L,2);                 /* stack: key */
  }                                  /* stack: key */
  tolua_getproxy(L,__proxy_level_);  /* stack: val */
  return 1;
}

/* tolua.proxy.set(key,value<,level>) */
static int tolua_bnd_proxy_set(lua_State* L){
  /* stack: key val <level> */
  int __proxy_level_=TOLUA_PROXY_TOP;
  if(lua_gettop(L)<1 || lua_isnil(L,1)){  /* check if key hasn't been retrieved */
    return 0;
  }
  if(lua_gettop(L)<2){  /* check if val hasn't been retrieved */
    return 0;
  }
  if(lua_isnumber(L,3)){ /* check if level has been retrieved */
    /* stack: key val level */
    __proxy_level_=lua_tonumber(L,3);
    lua_remove(L,3);                 /* stack: key val */
  }                                  /* stack: key val */
  tolua_setproxy(L,__proxy_level_);  /* stack: */
  return 0;
}

/* local top = tolua.proxy.top() */
static int tolua_bnd_proxy_top(lua_State* L){
  int __top_=tolua_proxytop(L);
  lua_pushnumber(L,__top_);
  return 1;
}

/* static int class_gc_event (lua_State* L); */

TOLUA_API void tolua_open (lua_State* L){
  int top = lua_gettop(L);
  lua_pushstring(L,"tolua_opened");
  lua_rawget(L,LUA_REGISTRYINDEX);
  if (!lua_isboolean(L,-1)){
    lua_pushstring(L,"tolua_opened"); lua_pushboolean(L,1); lua_rawset(L,LUA_REGISTRYINDEX);
    
#ifndef LUA_VERSION_NUM /* only prior to lua 5.1 */
    /* create peer object table */
    lua_pushstring(L, "tolua_peers"); lua_newtable(L);
    /* make weak key metatable for peers indexed by userdata object */
    lua_newtable(L); lua_pushliteral(L, "__mode"); lua_pushliteral(L, "k"); lua_rawset(L, -3);                /* stack: string peers mt */
    lua_setmetatable(L, -2);   /* stack: string peers */
    lua_rawset(L,LUA_REGISTRYINDEX);
#endif
    
    /* create object ptr -> udata mapping table */
    lua_pushstring(L,"tolua_ubox"); lua_newtable(L);
    /* make weak value metatable for ubox table to allow userdata to be
       garbage-collected */
    lua_newtable(L); lua_pushliteral(L, "__mode"); lua_pushliteral(L, "v"); lua_rawset(L, -3);
    /* stack: string ubox mt */
    lua_setmetatable(L, -2);  /* stack: string ubox */
    lua_rawset(L,LUA_REGISTRYINDEX);
    
    lua_pushstring(L,"tolua_super"); lua_newtable(L); lua_rawset(L,LUA_REGISTRYINDEX);
    lua_pushstring(L,"tolua_gc");    lua_newtable(L); lua_rawset(L,LUA_REGISTRYINDEX);
    lua_pushstring(L,"tolua_node");  lua_newtable(L); lua_rawset(L,LUA_REGISTRYINDEX);
    
    /* create proxy table */
    lua_pushstring(L,"tolua_proxy");
    lua_newtable(L);
    /* insert counter into proxy table */
    lua_pushnumber(L,0);
    lua_pushnumber(L,0);
    lua_rawset(L,-3);
    /* register proxy table */
    lua_rawset(L,LUA_REGISTRYINDEX);
    
    /* create gc_event closure */
    lua_pushstring(L, "tolua_gc_event");
    lua_pushstring(L, "tolua_gc");
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushstring(L, "tolua_super");
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushcclosure(L, class_gc_event, 2);
    lua_rawset(L, LUA_REGISTRYINDEX);
    
    tolua_newmetatable(L,"tolua_commonclass");
    
    tolua_module(L,NULL,0);
    tolua_beginmodule(L,NULL);
    tolua_module(L,"tolua",0);
    tolua_beginmodule(L,"tolua");
    tolua_function(L,"type",tolua_bnd_type);
    tolua_function(L,"takeownership",tolua_bnd_takeownership);
    tolua_function(L,"releaseownership",tolua_bnd_releaseownership);
    tolua_function(L,"cast",tolua_bnd_cast);
    tolua_function(L,"inherit", tolua_bnd_inherit);
#ifdef LUA_VERSION_NUM /* lua 5.1 */
    tolua_function(L,"setpeer", tolua_bnd_setpeer);
    tolua_function(L,"getpeer", tolua_bnd_getpeer);
#endif
    
    tolua_module(L,"proxy",0);
    tolua_beginmodule(L,"proxy");
    tolua_function(L,"top",  tolua_bnd_proxy_top);
    tolua_function(L,"push", tolua_bnd_proxy_push);
    tolua_function(L,"pop",  tolua_bnd_proxy_pop);
    tolua_function(L,"set",  tolua_bnd_proxy_set);
    tolua_function(L,"get",  tolua_bnd_proxy_get);
    tolua_function(L,"level",tolua_bnd_proxy_level);
    tolua_endmodule(L);
    
    tolua_endmodule(L);
    tolua_endmodule(L);
  }
  lua_settop(L,top);
  tolua_setstate(L);
}

/* Copy a C object
 */
TOLUA_API void* tolua_copy (lua_State* L, void* value, unsigned int size){
  void* clone = (void*)malloc(size);
  if (clone) memcpy(clone,value,size);
  else tolua_error(L,"insuficient memory",NULL);
  return clone;
}

/* Default collect function
 */
TOLUA_API int tolua_default_collect (lua_State* tolua_S){
  void* self = tolua_tousertype(tolua_S,1,0);
  free(self);
  return 0;
}

/* Do clone
 */
TOLUA_API int tolua_register_gc (lua_State* L, int lo){
  int success = 1;
  void *value = *(void **)lua_touserdata(L,lo);
  lua_pushstring(L,"tolua_gc");
  lua_rawget(L,LUA_REGISTRYINDEX);
  lua_pushlightuserdata(L,value);
  lua_rawget(L,-2);
  if (!lua_isnil(L,-1)){ /* make sure that object is not already owned */
    success = 0;
  } else {
    lua_pushlightuserdata(L,value);
    lua_getmetatable(L,lo);
    lua_rawset(L,-4);
  }
  lua_pop(L,2);
  return success;
}

/* Register a usertype
 * It creates the correspoding metatable in the registry, for both 'type' and 'const type'.
 * It maps 'const type' as being also a 'type'
 */
TOLUA_API void tolua_usertype (lua_State* L, char* type)
{
  char ctype[128] = "const ";
  strncat(ctype,type,120);

  /* create both metatables */
  if (tolua_newmetatable(L,ctype) && tolua_newmetatable(L,type))
    mapsuper(L,type,ctype);             /* 'type' is also a 'const type' */
}


/* Begin module
 * It pushes the module (or class) table on the stack
 */
TOLUA_API void tolua_beginmodule (lua_State* L, char* name)
{
  if (name)
    {
      lua_pushstring(L,name);
      lua_rawget(L,-2);
    }
  else
    lua_pushvalue(L,LUA_GLOBALSINDEX);
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
TOLUA_API void tolua_module (lua_State* L, char* name, int hasvar)
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
    {
      /* global table */
      lua_pushvalue(L,LUA_GLOBALSINDEX);
    }
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
TOLUA_API void tolua_module (lua_State* L, char* name, int hasvar)
{
  if (name)
    {
      /* tolua module */
      lua_pushstring(L,name);
      lua_newtable(L);
    }
  else
    {
      /* global table */
      lua_pushvalue(L,LUA_GLOBALSINDEX);
    }
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

static void push_collector(lua_State* L, const char* type, lua_CFunction col) {

  /* push collector function, but only if it's not NULL, or if there's no
     collector already */
  if (!col) return;
  luaL_getmetatable(L,type);
  lua_pushstring(L,".collector");
  /*
    if (!col) {
    lua_pushvalue(L, -1);
    lua_rawget(L, -3);
    if (!lua_isnil(L, -1)) {
    lua_pop(L, 3);
    return;
    };
    lua_pop(L, 1);
    };
    //	*/
  lua_pushcfunction(L,col);

  lua_rawset(L,-3);
  lua_pop(L, 1);
};

/* Map C class
 * It maps a C class, setting the appropriate inheritance and super classes.
 */
TOLUA_API void tolua_cclass (lua_State* L, char* lname, char* name, char* base, lua_CFunction col)
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
	
  push_collector(L, name, col);
  /*
    luaL_getmetatable(L,name);
    lua_pushstring(L,".collector");
    lua_pushcfunction(L,col);

    lua_rawset(L,-3);
  */
	
  luaL_getmetatable(L,name);
  lua_rawset(L,-3);              /* assign class metatable to module */

  /* now we also need to store the collector table for the const
     instances of the class */
  push_collector(L, cname, col);
  /*
    luaL_getmetatable(L,cname);
    lua_pushstring(L,".collector");
    lua_pushcfunction(L,col);
    lua_rawset(L,-3);
    lua_pop(L,1);
  */
	

}

/* Add base
 * It adds additional base classes to a class (for multiple inheritance)
 * (not for now)
 TOLUA_API void tolua_addbase(lua_State* L, char* name, char* base) {

 char cname[128] = "const ";
 char cbase[128] = "const ";
 strncat(cname,name,120);
 strncat(cbase,base,120);

 mapsuper(L,cname,cbase);
 mapsuper(L,name,base);
 };
*/

/* Map function
 * It assigns a function into the current module (or class)
 */
TOLUA_API void tolua_function (lua_State* L, char* name, lua_CFunction func){
  lua_pushstring(L,name);
  lua_pushcfunction(L,func);
  lua_rawset(L,-3);
}

/* sets the __call event for the class (expects the class' main table on top) */
/*	never really worked :(
	TOLUA_API void tolua_set_call_event(lua_State* L, lua_CFunction func, char* type) {

	lua_getmetatable(L, -1);
	//luaL_getmetatable(L, type);
	lua_pushstring(L,"__call");
	lua_pushcfunction(L,func);
	lua_rawset(L,-3);
	lua_pop(L, 1);
	};
*/

/* Map constant number
 * It assigns a constant number into the current module (or class)
 */
TOLUA_API void tolua_constant (lua_State* L, char* name, double value){
  lua_pushstring(L,name);
  tolua_pushnumber(L,value);
  lua_rawset(L,-3);
}


/* Map variable
 * It assigns a variable into the current module (or class)
 */
TOLUA_API void tolua_variable (lua_State* L, char* name, lua_CFunction get, lua_CFunction set){
  /* get func */
  lua_pushstring(L,".get");
  lua_rawget(L,-2);
  if (!lua_istable(L,-1)) {
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
  if (set) {
    lua_pushstring(L,".set");
    lua_rawget(L,-2);
    if (!lua_istable(L,-1)) {
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
static int const_array (lua_State* L){
  luaL_error(L,"value of const array cannot be changed");
  return 0;
}

/* Map an array
 * It assigns an array into the current module (or class)
 */
TOLUA_API void tolua_array (lua_State* L, char* name, lua_CFunction get, lua_CFunction set){
  lua_pushstring(L,".get");
  lua_rawget(L,-2);
  if (!lua_istable(L,-1)) {
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


TOLUA_API void tolua_dobuffer(lua_State* L, char* B, unsigned int size, const char* name) {
#ifdef LUA_VERSION_NUM /* lua 5.1 */
  if (! luaL_loadbuffer(L, B, size, name)) {
    lua_pcall(L, 0, 0, 0);
  }
#else
  lua_dobuffer(L, B, size, name);
#endif
};

