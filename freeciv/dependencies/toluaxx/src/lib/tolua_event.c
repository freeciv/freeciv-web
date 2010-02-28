/* tolua: event functions
** Support code for Lua bindings.
** Written by Waldemar Celes
** TeCGraf/PUC-Rio
** Apr 2003
** $Id: tolua_event.c 14589 2008-04-15 21:40:53Z cazfi $
*/

/* This code is free software; you can redistribute it and/or modify it.
** The software provided hereunder is on an "as is" basis, and
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications.
*/

#include <stdio.h>

#include "toluaxx.h"
#include "tolua_event.h"

/* Store at ubox
 * It stores, creating the corresponding table if needed,
 * the pair key/value in the corresponding ubox table
 */
static void storeatubox (lua_State* L, int lo){
#ifdef LUA_VERSION_NUM                     /* stack: obj key val */
  lua_getfenv(L, lo);                      /* stack: obj key val ?peer? */
  if (lua_rawequal(L, -1, TOLUA_NOPEER)) { /* check if ?peer? is no peer really */ /* create peer table */
    lua_pop(L, 1);                         /* stack: obj key val */
    lua_newtable(L);                       /* stack: obj key val peer */
    lua_pushvalue(L, -1);                  /* stack: obj key val peer peer */
    lua_setfenv(L, lo);	                   /* stack: obj key val peer */
  }
  lua_insert(L, -3);                       /* stack: obj peer key val */
  lua_settable(L, -3);                     /* stack: obj peer */
  lua_pop(L, 1);                           /* stack: obj */
  /* on lua 5.1, we trade the "tolua_peers" lookup for a settable call */
#else
  /* stack: key value (to be stored) */
  lua_pushstring(L,"tolua_peers");
  lua_rawget(L,LUA_REGISTRYINDEX);        /* stack: k v ubox */
  lua_pushvalue(L,lo);
  lua_rawget(L,-2);                       /* stack: k v ubox ubox[u] */
  if (!lua_istable(L,-1)) {
    lua_pop(L,1);                         /* stack: k v ubox */
    lua_newtable(L);                      /* stack: k v ubox table */
    lua_pushvalue(L,1);
    lua_pushvalue(L,-2);                  /* stack: k v ubox table u table */
    lua_rawset(L,-4);                     /* stack: k v ubox ubox[u]=table */
  }
  lua_insert(L,-4);                       /* put table before k */
  lua_pop(L,1);                           /* pop ubox */
  lua_rawset(L,-3);                       /* store at table */
  lua_pop(L,1);                           /* pop ubox[u] */
#endif
}

/* Module index function
 */
static int module_index_event (lua_State* L){
  lua_pushstring(L,".get");
  lua_rawget(L,-3);
  if (lua_istable(L,-1)){
    lua_pushvalue(L,2);  /* key */
    lua_rawget(L,-2);
    if (lua_iscfunction(L,-1)){
      lua_call(L,0,1);
      return 1;
    }else if (lua_istable(L,-1))
      return 1;
  }
  /* call old index meta event */
  if (lua_getmetatable(L,1)){
    lua_pushstring(L,"__index");
    lua_rawget(L,-2);
    lua_pushvalue(L,1);
    lua_pushvalue(L,2);
    if (lua_isfunction(L,-1)){
      lua_call(L,2,1);
      return 1;
    }else if (lua_istable(L,-1)){
      lua_gettable(L,-3);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

/* Module newindex function
 */
static int module_newindex_event (lua_State* L){
  lua_pushstring(L,".set");
  lua_rawget(L,-4);
  if (lua_istable(L,-1)){
    lua_pushvalue(L,2);  /* key */
    lua_rawget(L,-2);
    if (lua_iscfunction(L,-1)){
      lua_pushvalue(L,1); /* only to be compatible with non-static vars */
      lua_pushvalue(L,3); /* value */
      lua_call(L,2,0);
      return 0;
    }
  }
  /* call old newindex meta event */
  if (lua_getmetatable(L,1) && lua_getmetatable(L,-1)){
    lua_pushstring(L,"__newindex");
    lua_rawget(L,-2);
    if (lua_isfunction(L,-1)){
      lua_pushvalue(L,1);
      lua_pushvalue(L,2);
      lua_pushvalue(L,3);
      lua_call(L,3,0);
    }
  }
  lua_settop(L,3);
  lua_rawset(L,-3);
  return 0;
}

/* Class index function
 * If the object is a userdata (ie, an object), it searches the field in
 * the alternative table stored in the corresponding "ubox" table.
 */

//#define TOLUA_OLD_INDEX_METHODS
#ifdef TOLUA_OLD_INDEX_METHODS
static int class_index_event (lua_State* L){
  int t = lua_type(L,1);
  if (t == LUA_TUSERDATA) {
    /* Access alternative table */
#ifdef LUA_VERSION_NUM /* new macro on version 5.1 */
    lua_getfenv(L,1);
    if (!lua_rawequal(L, -1, TOLUA_NOPEER)) {
      lua_pushvalue(L, 2); /* key */
      lua_gettable(L, -2); /* on lua 5.1, we trade the "tolua_peers" lookup for a gettable call */
      if (!lua_isnil(L, -1)) return 1;
    };
#else
    lua_pushstring(L,"tolua_peers");
    lua_rawget(L,LUA_REGISTRYINDEX);         /* stack: obj key ubox */
    lua_pushvalue(L,1);
    lua_rawget(L,-2);                        /* stack: obj key ubox ubox[u] */
    if (lua_istable(L,-1)){
      lua_pushvalue(L,2); /* key */
      lua_rawget(L,-2);                      /* stack: obj key ubox ubox[u] value */
      if (!lua_isnil(L,-1)) return 1;
    }
#endif
    lua_settop(L,2);                         /* stack: obj key */
    /* Try metatables */
    lua_pushvalue(L,1);                      /* stack: obj key obj */
    while (lua_getmetatable(L,-1)) {         /* stack: obj key obj mt */
      lua_remove(L,-2);                      /* stack: obj key mt */
      if (lua_isnumber(L,2)) {               /* check if key is a numeric value */
	/* try operator[] */
	lua_pushstring(L,".geti");
	lua_rawget(L,-2);                    /* stack: obj key mt func */
	if (lua_isfunction(L,-1)) {
	  lua_pushvalue(L,1);
	  lua_pushvalue(L,2);
	  lua_call(L,2,1);
	  return 1;
	}
      }else{
	lua_pushvalue(L,2);                    /* stack: obj key mt key */
	lua_rawget(L,-2);                      /* stack: obj key mt value */
	if (!lua_isnil(L,-1))return 1;
	else lua_pop(L,1);
	/* try C/C++ variable */
	lua_pushstring(L,".get");
	lua_rawget(L,-2);                   /* stack: obj key mt tget */
	if (lua_istable(L,-1)){
	  lua_pushvalue(L,2);
	  lua_rawget(L,-2);                 /* stack: obj key mt value */
	  if (lua_iscfunction(L,-1)){
	    lua_pushvalue(L,1);
	    lua_pushvalue(L,2);
	    lua_call(L,2,1);
	    return 1;
	  }else if (lua_istable(L,-1)){
	    /* deal with array: create table to be returned and cache it in ubox */
	    void* u = *((void**)lua_touserdata(L,1));
	    lua_newtable(L);                /* stack: obj key mt value table */
	    lua_pushstring(L,".self");
	    lua_pushlightuserdata(L,u);
	    lua_rawset(L,-3);               /* store usertype in ".self" */
	    lua_insert(L,-2);               /* stack: obj key mt table value */
	    lua_setmetatable(L,-2);         /* set stored value as metatable */
	    lua_pushvalue(L,-1);            /* stack: obj key met table table */
	    lua_pushvalue(L,2);             /* stack: obj key mt table table key */
	    lua_insert(L,-2);               /* stack: obj key mt table key table */
	    storeatubox(L,1);               /* stack: obj key mt table */
	    return 1;
	  }
	}
      }
      lua_settop(L,3);
    }
    lua_pushnil(L);
    return 1;
  }else if (t== LUA_TTABLE){
    module_index_event(L);
    return 1;
  }
  lua_pushnil(L);
  return 1;
}

/* Newindex function
 * It first searches for a C/C++ varaible to be set.
 * Then, it either stores it in the alternative ubox table (in the case it is
 * an object) or in the own table (that represents the class or module).
 */
static int class_newindex_event (lua_State* L){
  int t = lua_type(L,1);
  if (t == LUA_TUSERDATA){
    /* Try accessing a C/C++ variable to be set */
    lua_getmetatable(L,1);
    while (lua_istable(L,-1)) {                /* stack: t k v mt */
      if (lua_isnumber(L,2)) {                 /* check if key is a numeric value */
	/* try operator[] */
	lua_pushstring(L,".seti");
	lua_rawget(L,-2);                      /* stack: obj key mt func */
	if (lua_isfunction(L,-1)) {
	  lua_pushvalue(L,1);
	  lua_pushvalue(L,2);
	  lua_pushvalue(L,3);
	  lua_call(L,3,0);
	  return 0;
	}
      } else {
	lua_pushstring(L,".set");
	lua_rawget(L,-2);                      /* stack: t k v mt tset */
	if (lua_istable(L,-1)) {
	  lua_pushvalue(L,2);
	  lua_rawget(L,-2);                     /* stack: t k v mt tset func */
	  if (lua_iscfunction(L,-1)) {
	    lua_pushvalue(L,1);
	    lua_pushvalue(L,3);
	    lua_call(L,2,0);
	    return 0;
	  }
	  lua_pop(L,1);                          /* stack: t k v mt tset */
	}
	lua_pop(L,1);                           /* stack: t k v mt */
	if (!lua_getmetatable(L,-1))lua_pushnil(L); /* stack: t k v mt mt */
	lua_remove(L,-2);                       /* stack: t k v mt */
      }
    }
    lua_settop(L,3);                          /* stack: t k v */
    /* then, store as a new field */
    storeatubox(L,1);
  } else if (t== LUA_TTABLE) {
    module_newindex_event(L);
  }
  return 0;
}
#else // ifndef TOLUA_OLD_INDEX_METHODS
static int class_index_event (lua_State* L){ // method with operator[](string)
  int t = lua_type(L,1);
  if (t == LUA_TUSERDATA) {
    lua_pushvalue(L,1);                      /* stack: obj key obj */
    /* Try metatables */
    while (lua_getmetatable(L,-1)) {         /* stack: obj key obj mt */
      lua_remove(L,-2);                      /* stack: obj key mt */
      if (lua_isnumber(L,2)) {               /* check if key is a numeric value */
	/* try operator[](number) */
	lua_pushstring(L,".geti");           /* stack: obj key mt ".geti" */
	lua_rawget(L,-2);                    /* stack: obj key mt ?func? */
	if (lua_isfunction(L,-1)) {          /* check if func is function? */
	  lua_pushvalue(L,1);                /* stack: obj key mt func obj */
	  lua_pushvalue(L,2);                /* stack: obj key mt func obj key */
	  lua_call(L,2,1);                   /* stack: obj key mt ret */
	  return 1;                          /* stack: ret */
	}
      }else{
	lua_pushvalue(L,2);                  /* stack: obj key mt key */
	lua_rawget(L,-2);                    /* stack: obj key mt value */
	if (!lua_isnil(L,-1))return 1;       /* value is nil? */
	else lua_pop(L,1);                   /* stack: obj key mt */
	/* try C/C++ variable */
	lua_pushstring(L,".get");            /* stack: obj key mt ".get" */
	lua_rawget(L,-2);                    /* stack: obj key mt tget */
	if (lua_istable(L,-1)){              /* tget is table? */
	  lua_pushvalue(L,2);                /* stack: obj key mt tget key */
	  lua_rawget(L,-2);                  /* stack: obj key mt tget value */
	  if (lua_iscfunction(L,-1)){        /* value is cfunction? */
	    lua_pushvalue(L,1);              /* stack: obj key mt tget value obj */
	    lua_pushvalue(L,2);              /* stack: obj key mt tget value obj key */
	    lua_call(L,2,1);                 /* stack: obj key mt tget value ret */
	    return 1;                        /* stack: ret */
	  }else if (lua_istable(L,-1)){      /* value is table? */
	    /* deal with array: create table to be returned and cache it in ubox */
	    void* u = *((void**)lua_touserdata(L,1));
	    lua_newtable(L);                 /* stack: obj key mt tget value table */
	    lua_pushstring(L,".self");       /* stack: obj key mt tget value table ".self" */
	    lua_pushlightuserdata(L,u);      /* stack: obj key mt tget value table ".self" usertype */
	    /* store usertype in ".self" field table */
	    lua_rawset(L,-3);                /* stack: obj key mt tget value table */
	    lua_insert(L,-2);                /* stack: obj key mt tget table value */
	    /* set stored value as metatable */
	    lua_setmetatable(L,-2);          /* stack: obj key mt tget table */
	    lua_pushvalue(L,-1);             /* stack: obj key mt tget table table */
	    lua_pushvalue(L,2);              /* stack: obj key mt tget table table key */
	    lua_insert(L,-2);                /* stack: obj key mt tget table key table */
	    storeatubox(L,1);                /* stack: obj key mt tget table */
	    return 1;                        /* stack: table */
	  }
	}
      }
      lua_settop(L,3);                       /* stack: obj key mt */
    }
    /* For operator[](string) */
    lua_settop(L,2);                         /* stack: obj key */
    lua_pushvalue(L,1);                      /* stack: obj key obj */
    /* Try metatables */
    while (lua_getmetatable(L,-1)) {         /* stack: obj key obj mt */
      lua_remove(L,-2);                      /* stack: obj key mt */
      if (lua_isstring(L,2)) {               /* check if key is a string value */
	/* try operator[](string) */
	lua_pushstring(L,".gets");           /* stack: obj key mt ".gets" */
	lua_rawget(L,-2);                    /* stack: obj key mt ?func? */
	if (lua_isfunction(L,-1)) {          /* check if func is function? */
	  lua_pushvalue(L,1);                /* stack: obj key mt func obj */
	  lua_pushvalue(L,2);                /* stack: obj key mt func obj key */
	  /*--lua_call(L,2,1);*/             /* stack: obj key mt ret--*/
	  lua_call(L,2,1);                   /* stack: obj key mt ?ret? */
	  if(lua_isnil(L,-1)){               /* check if .gets not passed */
	                                     /* stack: obj key mt nil */
	  }else{                             /* stack: obj key mt ret novalue */
	    return 1;                        /* stack: ret */
	  }
	}
      }
      lua_settop(L,3);                       /* stack: obj key mt */
    }
    lua_settop(L,2);                         /* stack: obj key */
    /* Access alternative table */
#ifdef LUA_VERSION_NUM /* new macro on version 5.1 */
    lua_getfenv(L,1);                        /* stack: obj key ?peer? */
    if (!lua_rawequal(L, -1, TOLUA_NOPEER)) {/* check if ?peer? is peer */
      lua_pushvalue(L, 2);                   /* stack: obj key peer key */
      lua_gettable(L, -2);                   /* stack: obj key val */
      /* on lua 5.1, we trade the "tolua_peers" lookup for a gettable call */
      if (!lua_isnil(L, -1)) return 1;       /* stack: val */
    }
#else
    lua_pushstring(L,"tolua_peers");
    lua_rawget(L,LUA_REGISTRYINDEX);         /* stack: obj key ubox */
    lua_pushvalue(L,1);
    lua_rawget(L,-2);                        /* stack: obj key ubox ubox[u] */
    if (lua_istable(L,-1)) {
      lua_pushvalue(L,2); /* key */
      lua_rawget(L,-2);                      /* stack: obj key ubox ubox[u] value */
      if (!lua_isnil(L,-1)) return 1;
    }
#endif
    lua_settop(L,2);                         /* stack: obj key */
    lua_pushnil(L);                          /* stack: obj key nil */
    return 1;                                /* stack: nil */
  }else if (t == LUA_TTABLE) {
    module_index_event(L);
    return 1;
  }
  lua_pushnil(L);                            /* stack: obj key mt nil */
  return 1;                                  /* stack: nil */
}

/* Newindex function
 * It first searches for a C/C++ varaible to be set.
 * Then, it either stores it in the alternative ubox table (in the case it is
 * an object) or in the own table (that represents the class or module).
 */
static int class_newindex_event (lua_State* L){
  //DEBUG_STACK(class_newindex_event init);
  int t = lua_type(L,1);
  if (t == LUA_TUSERDATA){                   /* stack: obj key val */
    /* Try accessing a C/C++ variable to be set */
    lua_getmetatable(L,1);                   /* stack: obj key val ?mt? */
    while (lua_istable(L,-1)) {              /* check if ?mt? is a table */
      if (lua_isnumber(L,2)) {               /* check if key is a numeric value */
	/* try operator[](number) */
	lua_pushstring(L,".seti");           /* stack: obj key val mt ".seti" */
	lua_rawget(L,-2);                    /* stack: obj key val mt ?func? */
	if (lua_isfunction(L,-1)) {          /* check if ?func? is a function */
	  lua_pushvalue(L,1);                /* stack: obj key val mt func obj */
	  lua_pushvalue(L,2);                /* stack: obj key val mt func obj key */
	  lua_pushvalue(L,3);                /* stack: obj key val mt func obj key val */
	  lua_call(L,3,0);                   /* stack: obj key val mt */
	  lua_settop(L,3);                   /* stack: obj key val */
	  if (lua_isuserdata(L,-1) || lua_isnil(L,-1)) {        /* check if val is userdata or nil */ /* therefor garbage collector not collect userdata store it into peer */
	    storeatubox(L,1);                /* stack: obj */
	  }
	  return 0;                          /* stack: no value */
	}
	lua_pop(L,1);                        /* stack: obj key val mt */
	if (!lua_getmetatable(L,-1))         /* stack: obj key val mt ?next_mt? */
	  lua_pushnil(L);                    /* stack: obj key val mt nil */
	lua_remove(L,-2);                    /* stack: obj key val next_mt */
      } else {                               /* check if key is a not numeric value */
	lua_pushstring(L,".set");            /* stack: obj key val mt ".set" */
	lua_rawget(L,-2);                    /* stack: obj key val mt tset */
	if (lua_istable(L,-1)) {             /* check if tset if table */
	  lua_pushvalue(L,2);                /* stack: obj key val mt tset key */
	  lua_rawget(L,-2);                  /* stack: obj key val mt tset ?func? */
	  if (lua_iscfunction(L,-1)) {       /* check if ?func? is a function */
	    lua_pushvalue(L,1);              /* stack: obj key val mt tset func obj */
	    lua_pushvalue(L,3);              /* stack: obj key val mt tset func obj val */
	    lua_call(L,2,0);                 /* stack: obj key val mt tset */
	    return 0;                        /* stack: */
	  }
	  lua_pop(L,1);                      /* stack: obj key val mt tset */
	}
	lua_pop(L,1);                        /* stack: obj key val mt */
	if (!lua_getmetatable(L,-1))         /* stack: obj key val mt ?next_mt? */
	  lua_pushnil(L);                    /* stack: obj key val mt nil */
	lua_remove(L,-2);                    /* stack: obj key val next_mt */
      }
    }
    lua_settop(L,3);                         /* stack: obj key val */
    if (lua_isstring(L,2)) {                 /* check if key is a string value */
      /* try operator[](string) */
      //DEBUG_STACK(class_newindex_event try operator[](string));
      lua_getmetatable(L,1);                 /* stack: obj key val mt */
      while (lua_istable(L,-1)) {            /* check if mt is table */
	//DEBUG_STACK(class_newindex_event while (lua_istable(L,-1)));
	lua_pushstring(L,".sets");           /* stack: obj key val mt ".sets" */
	lua_rawget(L,-2);                    /* stack: obj key val mt ?func? */
	if (lua_isfunction(L,-1)) {          /* check if ?func? is a function */
	  lua_pushvalue(L,1);                /* stack: obj key val mt func obj */
	  lua_pushvalue(L,2);                /* stack: obj key val mt func obj key */
	  lua_pushvalue(L,3);                /* stack: obj key val mt func obj key val */
	  /*--lua_call(L,3,0);--*/           /*-- stack: obj key val mt --*/
	  int t=lua_gettop(L)-1;
	  //DEBUG_STACK(class_newindex_event call .sets);
	  lua_call(L,3,3);                   /* stack: obj key val mt <?obj? ?key? ?val?> */
	  if (lua_isuserdata(L,t-2) &&
	      lua_isstring(L,t-1)) {         /* check if .sets not passed */
	                                     /* stack: obj key val mt obj key val */
	    lua_remove(L,-1);                /* stack: obj key val mt obj key */
	    lua_remove(L,-1);                /* stack: obj key val mt obj */
	    lua_remove(L,-1);                /* stack: obj key val mt */
	  }else{
	    lua_settop(L,3);                 /* stack: obj key val */
	    if (lua_isuserdata(L,-1) || lua_isnil(L,-1)) {        /* check if val is userdata or nil */ /* therefor garbage collector not collect userdata store it into peer */
	      storeatubox(L,1);              /* stack: obj */
	    }
	    return 0;                        /* stack: no value */
	  }
	}else lua_remove(L,-1);
	if (!lua_getmetatable(L,-1)){        /* stack: obj key val mt ?next_mt? */
	  lua_pushnil(L);                    /* stack: obj key val mt ?next_mt? nil */
	  lua_remove(L,-2);                  /* stack: obj key val nil */
	}
	                                     /* stack: obj key val next_mt */
      }
    }
    lua_settop(L,3);                         /* stack: obj key val */
    /* then, store as a new field */
    storeatubox(L,1);
  } else if (t==LUA_TTABLE) {
    module_newindex_event(L);
  }
  return 0;
}
#endif


/*
 *  .... obj tab
 *    next with tab
 *  .... obj tab key val
 *    if val isn't table
 *    set to obj
 *  .... obj tab 
 *    else
 *    get from obj field with key
 *  .... obj tab key val fld
 *    swap fld and val
 *  .... obj tab key fld val
 *    recursive call this function
 *  .... obj tab key fld val
 *    remove key, fld and val
 *  .... obj tab
 *    repeat it
 */

/* extended table construstor */
static int tolua_class_attribs_from_table(lua_State* L){
  if(!lua_isuserdata(L,-2) || !lua_istable(L,-1))return 0;
                                            /* stack: obj tab */
  lua_pushnil(L);                           /* stack: obj tab nil */
  for(;lua_next(L,-2)!=0;){                 /* stack: obj tab key val */
    //DEBUG_STACK(lua_next(L,-1)!=0);
    if(lua_istable(L,-1)){                  /* check if val is table */
      lua_pushvalue(L,-2);                  /* stack: obj tab key val key */
      lua_gettable(L,-5);                   /* stack: obj tab key val fld */
      lua_insert(L,-2);                     /* stack: obj tab key fld val */
      tolua_class_attribs_from_table(L);    /* stack: obj tab key fld val */
      lua_remove(L,-1);                     /* stack: obj tab key fld */
      lua_remove(L,-1);                     /* stack: obj tab key */
    }else{
                                            /* stack: obj tab key val */
      lua_pushvalue(L,-2);                  /* stack: obj tab key val key */
      lua_insert(L,-2);                     /* stack: obj tab key key val */
      lua_settable(L,-5);                   /* stack: obj tab key */
    }
  }
  //DEBUG_STACK(lua_next(L,-1)!=0 end);
  return 1;                                 /* stack: obj tab */
}

static int class_call_event(lua_State* L) {
  if (lua_istable(L,1)) {                   /* stack: obj <args> */
    lua_pushstring(L,".call");              /* stack: obj <args> ".call" */
    lua_rawget(L,1);                        /* stack: obj <args> ?func? */
    if (lua_isfunction(L,-1)) {             /* chack if ?func? is function */
      lua_insert(L,1);                      /* stack: func obj <args> */
      DEBUG_STACK(lua_isfunction(L,-1) lua_insert(L,1));
      if(lua_gettop(L)>2 && lua_istable(L,-1)){ /* chack if last arg is lua table */
	DEBUG_STACK(lua_gettop(L)>2 && lua_istable(L,-1));
	                                    /* stack: func obj <args> table */
	lua_insert(L,1);                    /* stack: table func obj <args> */
	DEBUG_STACK(lua_insert(L,1));
	lua_call(L,lua_gettop(L)-2,1);      /* stack: table ret */
	DEBUG_STACK(lua_call(L,lua_gettop(L)-2,1));
	lua_insert(L,-2);                   /* stack: ret table */
	DEBUG_STACK(lua_insert(L,-2));
	tolua_class_attribs_from_table(L);  /* stack: ret table */
	DEBUG_STACK(tolua_class_attribs_from_table(L));
	lua_remove(L,-1);                   /* stack: ret */
	DEBUG_STACK(tolua_class_attribs_from_table(L) lua_remove(L,-1));
	return 1;                           /* stack: ret */
      }else{
	                                    /* stack: func obj <args> */
	lua_call(L,lua_gettop(L)-1,1);      /* stack: func ret */
	return 1;                           /* stack: ret */
      }
    }
  }
  if (lua_isuserdata(L,1)) {           /* stack: obj {args} */
    /* for sub object calling: obj{prop1=val1, ...., propN=valN} */
    if(lua_istable(L,2)&&(lua_gettop(L)==2||lua_isnil(L,3))){ /* check if two arg is lua table */
      tolua_class_attribs_from_table(L); /* stack: obj tab */
      return 0;
    }else{
      lua_pushvalue(L,1);                /* stack: obj {args} obj */
      while (lua_getmetatable(L,-1)) {   /* stack: obj {args} obj mt */   /* Try metatables */
	lua_remove(L,-2);                /* stack: obj {args} mt */
	lua_pushstring(L,".callself");   /* stack: obj {args} mt name */
	lua_rawget(L,-2);                /* stack: obj {args} mt func */
	if (lua_isfunction(L,-1)) {      /* func is function? */
	  lua_insert(L,1);               /* stack: func obj {args} mt */
	  lua_settop(L,lua_gettop(L)-1); /* stack: func obj {args} */  /* args ={ nil key } */
	  /*DEBUG_STACK("prepare");*/
	  if(lua_gettop(L)==4&&lua_isnil(L,3))lua_insert(L,3); /* args ={ key nil } */
	  /*DEBUG_STACK("prep call");*/
	  lua_call(L,lua_gettop(L)-1,-1); /* stack: {rets} */   /* rets ={ nil key } */
	  /*if(lua_gettop(L)==2&&lua_isnil(L,2))lua_insert(L,1);*/ /* rets ={ key nil } */
	  /*DEBUG_STACK("post call");*/
	  return lua_gettop(L);          /* stack: {rets} */
	}
	lua_settop(L,3);                 /* stack: obj {args} mt */
      }
    }
  }
  tolua_error(L,"Attempt to call a non-callable object.",NULL);
  return 0;
};

static int do_twin_operator (lua_State* L, const char* op) {
  if (lua_isuserdata(L,1)) {          /* stack: op1 op2 */
    lua_pushvalue(L,1);               /* stack: op1 op2 op1 */
    while (lua_getmetatable(L,-1)) {  /* stack: op1 op2 op1 mt */    /* Try metatables */ 
      lua_remove(L,-2);               /* stack: op1 op2 mt */
      lua_pushstring(L,op);           /* stack: op1 op2 mt key */
      lua_rawget(L,-2);               /* stack: op1 op2 mt ?func? */
      if (lua_isfunction(L,-1)) {     /* func is function? */
	lua_pushvalue(L,1);           /* stack: op1 op2 mt func op1 */
	lua_pushvalue(L,2);           /* stack: op1 op2 mt func op1 op2 */
	lua_call(L,2,1);              /* stack: op1 op2 mt func ret */
	return 1;                     /* stack: ret */
      }
      lua_settop(L,3);                /* stack: op1 op2 mt */
    }
  }
  tolua_error(L,"Attempt to perform operation on an invalid operand",NULL);
  return 0;
}
/// +++ <<<
static int do_unary_operator (lua_State* L, const char* op) {
  if (lua_isuserdata(L,1)) {         /* stack: op */
    lua_pushvalue(L,1);              /* stack: op op */
    /* Try metatables */
    while (lua_getmetatable(L,-1)) { /* stack: op op mt */
      lua_remove(L,-2);              /* stack: op mt */
      lua_pushstring(L,op);          /* stack: op mt key */
      lua_rawget(L,-2);              /* stack: op mt ?func? */
      if (lua_isfunction(L,-1)) {    /* check if ?func? is function */
	lua_pushvalue(L,1);          /* stack: op mt func op */
	lua_call(L,1,1);             /* stack: op mt func ret */
	return 1;                    /* stack: ret */
      }
      lua_settop(L,2);               /* stack: op mt */
    }
  }
  tolua_error(L,"Attempt to perform operation on an invalid operand",NULL);
  return 0;
}
/// +++ >>>

static int class_tostring_event (lua_State* L){
  if (lua_isuserdata(L,1)) {         /* stack: op */
    lua_pushvalue(L,1);              /* stack: op op */
    /* Try metatables */
    while (lua_getmetatable(L,-1)) { /* stack: op op mt */
      lua_remove(L,-2);              /* stack: op mt */
      lua_pushstring(L,".string");   /* stack: op mt key */
      lua_rawget(L,-2);              /* stack: op mt ?func? */
      if (lua_isfunction(L,-1)) {    /* check if ?func? is function */
	lua_pushvalue(L,1);          /* stack: op mt func op */
	lua_call(L,1,1);             /* stack: op mt func ret */
	return 1;                    /* stack: ret */
      }
      lua_settop(L,2);               /* stack: op mt */
    }
  }
  {
    void* p;
    char b[50];
    p=lua_touserdata(L,1);        /* get address pointer */
    sprintf(b,"userdata: 0x%p", p);/* print pointer */
    lua_pushstring(L,b);
    return 1;
  }
  tolua_error(L,"Attempt to perform operation .string on an invalid operand",NULL);
  return 0;
}

static int class_add_event (lua_State* L){
  return do_twin_operator(L,".add");
}

static int class_sub_event (lua_State* L){
  return do_twin_operator(L,".sub");
}

static int class_mul_event (lua_State* L){
  return do_twin_operator(L,".mul");
}

static int class_pow_event (lua_State* L){
  return do_twin_operator(L,".pow");
}

static int class_len_event (lua_State* L){
  return do_unary_operator(L,".len");
}

static int class_unm_event (lua_State* L){
  return do_unary_operator(L,".unm");
}

static int class_mod_event (lua_State* L){
  return do_twin_operator(L,".mod");
}

static int class_div_event (lua_State* L){
  return do_twin_operator(L,".div");
}

static int class_concat_event (lua_State* L){
  return do_twin_operator(L,".concat");
}

static int class_lt_event (lua_State* L){
  return do_twin_operator(L,".lt");
}

static int class_le_event (lua_State* L){
  return do_twin_operator(L,".le");
}

static int class_eq_event (lua_State* L){
  return do_twin_operator(L,".eq");
}

/*
static int class_gc_event (lua_State* L)
{
	void* u = *((void**)lua_touserdata(L,1));
	fprintf(stderr, "collecting: looking at %p\n", u);
	lua_pushstring(L,"tolua_gc");
	lua_rawget(L,LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L,u);
	lua_rawget(L,-2);
	if (lua_isfunction(L,-1))
	{
		lua_pushvalue(L,1);
		lua_call(L,1,0);
 		lua_pushlightuserdata(L,u);
		lua_pushnil(L);
		lua_rawset(L,-3);
	}
	lua_pop(L,2);
	return 0;
}
*/
TOLUA_API int class_gc_event (lua_State* L){
  void* u = *((void**)lua_touserdata(L,1));
  int top;
  /*fprintf(stderr, "collecting: looking at %p\n", u);*/
  /*
    lua_pushstring(L,"tolua_gc");
    lua_rawget(L,LUA_REGISTRYINDEX);
  */
  lua_pushvalue(L, lua_upvalueindex(1));
  lua_pushlightuserdata(L,u);
  lua_rawget(L,-2);            /* stack: gc umt    */
  lua_getmetatable(L,1);       /* stack: gc umt mt */
  /*fprintf(stderr, "checking type\n");*/
  top = lua_gettop(L);
  if (tolua_fast_isa(L,top,top-1, lua_upvalueindex(2))) { /* make sure we collect correct type */
    /*fprintf(stderr, "Found type!\n");*/
    /* get gc function */
    lua_pushliteral(L,".collector");
    lua_rawget(L,-2);           /* stack: gc umt mt collector */
    if (lua_isfunction(L,-1)) {
      /*fprintf(stderr, "Found .collector!\n");*/
    } else {
      lua_pop(L,1);
      /*fprintf(stderr, "Using default cleanup\n");*/
      lua_pushcfunction(L,tolua_default_collect);
    }

    lua_pushvalue(L,1);         /* stack: gc umt mt collector u */
    lua_call(L,1,0);
    
    lua_pushlightuserdata(L,u); /* stack: gc umt mt u */
    lua_pushnil(L);             /* stack: gc umt mt u nil */
    lua_rawset(L,-5);           /* stack: gc umt mt */
  }
  lua_pop(L,3);
  return 0;
}


/* Register module events
	* It expects the metatable on the top of the stack
*/
TOLUA_API void tolua_moduleevents (lua_State* L){
  lua_pushstring(L,"__index");
  lua_pushcfunction(L,module_index_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__newindex");
  lua_pushcfunction(L,module_newindex_event);
  lua_rawset(L,-3);
}

/* Check if the object on the top has a module metatable
*/
TOLUA_API int tolua_ismodulemetatable (lua_State* L){
  int r = 0;
  if (lua_getmetatable(L,-1)) {
    lua_pushstring(L,"__index");
    lua_rawget(L,-2);
    r = (lua_tocfunction(L,-1) == module_index_event);
    lua_pop(L,2);
  }
  return r;
}

/* Register class events
	* It expects the metatable on the top of the stack
*/
TOLUA_API void tolua_classevents (lua_State* L) {
  lua_pushstring(L,"__index");
  lua_pushcfunction(L,class_index_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__newindex");
  lua_pushcfunction(L,class_newindex_event);
  lua_rawset(L,-3);
  
  lua_pushstring(L,"__add");
  lua_pushcfunction(L,class_add_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__sub");
  lua_pushcfunction(L,class_sub_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__mul");
  lua_pushcfunction(L,class_mul_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__div");
  lua_pushcfunction(L,class_div_event);
  lua_rawset(L,-3);
  // toluaxx new operators
  lua_pushstring(L,"__pow");
  lua_pushcfunction(L,class_pow_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__mod");
  lua_pushcfunction(L,class_mod_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__len");
  lua_pushcfunction(L,class_len_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__concat");
  lua_pushcfunction(L,class_concat_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__unm");
  lua_pushcfunction(L,class_unm_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__tostring");
  lua_pushcfunction(L,class_tostring_event);
  lua_rawset(L,-3);
  // end toluaxx operators
  lua_pushstring(L,"__lt");
  lua_pushcfunction(L,class_lt_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__le");
  lua_pushcfunction(L,class_le_event);
  lua_rawset(L,-3);
  lua_pushstring(L,"__eq");
  lua_pushcfunction(L,class_eq_event);
  lua_rawset(L,-3);
  
  lua_pushstring(L,"__call");
  lua_pushcfunction(L,class_call_event);
  lua_rawset(L,-3);
  
  lua_pushstring(L,"__gc");
  lua_pushstring(L, "tolua_gc_event");
  lua_rawget(L, LUA_REGISTRYINDEX);
  /*lua_pushcfunction(L,class_gc_event);*/
  lua_rawset(L,-3);
}

