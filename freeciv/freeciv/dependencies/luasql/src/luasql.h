/*
** $Id: luasql.h,v 1.12 2009/02/07 23:16:23 tomas Exp $
** See Copyright Notice in license.html
*/

#ifndef _LUASQL_
#define _LUASQL_

#ifndef LUASQL_API
#define LUASQL_API
#endif

#if !defined LUA_VERSION_NUM
/* Lua 5.0 */
#define luaL_Reg luaL_reg

#define lua_pushinteger(L, n) \
	lua_pushnumber(L, (lua_Number)n)
#endif

#define LUASQL_PREFIX "LuaSQL: "
#define LUASQL_TABLENAME "luasql"
#define LUASQL_ENVIRONMENT "Each driver must have an environment metatable"
#define LUASQL_CONNECTION "Each driver must have a connection metatable"
#define LUASQL_CURSOR "Each driver must have a cursor metatable"

LUASQL_API int luasql_faildirect (lua_State *L, const char *err);
LUASQL_API int luasql_failmsg (lua_State *L, const char *err, const char *m);
LUASQL_API int luasql_createmeta (lua_State *L, const char *name, const luaL_Reg *methods);
LUASQL_API void luasql_setmeta (lua_State *L, const char *name);
LUASQL_API void luasql_set_info (lua_State *L);

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup);
#endif

#endif
