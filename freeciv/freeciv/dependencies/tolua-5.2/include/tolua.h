/* tolua
** Support code for Lua bindings.
** Written by Waldemar Celes
** TeCGraf/PUC-Rio
** Apr 2003
** $Id: tolua.h,v 1.12 2011/01/13 13:43:45 fabraham Exp $
*/

/* This code is free software; you can redistribute it and/or modify it. 
** The software provided hereunder is on an "as is" basis, and 
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications. 
*/


#ifndef TOLUA_H
#define TOLUA_H

#ifndef TOLUA_API
# ifdef TOLUA5_DLL
#  define TOLUA_API __declspec(dllexport)
# else
#  define TOLUA_API
# endif
#endif

#define TOLUA_VERSION "tolua 5.2.4"

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"

struct tolua_Error
{
	int index;
	int array;
	const char* type;
};
typedef struct tolua_Error tolua_Error;
typedef char tolua_byte;
typedef unsigned char tolua_ubyte;
typedef int tolua_index;
typedef int tolua_multret;
typedef int tolua_len;
typedef int lua_Object;

#define tolua_own

TOLUA_API const char* tolua_typename (lua_State* L, int lo);
TOLUA_API void tolua_error (lua_State* L, const char* msg, tolua_Error* err);
TOLUA_API int tolua_isnoobj (lua_State* L, int lo, tolua_Error* err);
TOLUA_API int tolua_isvalue (lua_State* L, int lo, int def, tolua_Error* err);
TOLUA_API int tolua_isboolean (lua_State* L, int lo, int def, tolua_Error* err);
TOLUA_API int tolua_isnumber (lua_State* L, int lo, int def, tolua_Error* err);
TOLUA_API int tolua_isstring (lua_State* L, int lo, int def, tolua_Error* err);
TOLUA_API int tolua_istable (lua_State* L, int lo, int def, tolua_Error* err);
TOLUA_API int tolua_isusertable (lua_State* L, int lo, const char* type, int def, tolua_Error* err);
TOLUA_API int tolua_isfunction (lua_State* L, int lo, int def, tolua_Error* err); 
TOLUA_API int tolua_isuserdata (lua_State* L, int lo, int def, tolua_Error* err);
TOLUA_API int tolua_isusertype (lua_State* L, int lo, const char* type, int def, tolua_Error* err);
TOLUA_API int tolua_isvaluearray 
 (lua_State* L, int lo, int dim, int def, tolua_Error* err);
TOLUA_API int tolua_isbooleanarray 
 (lua_State* L, int lo, int dim, int def, tolua_Error* err);
TOLUA_API int tolua_isnumberarray 
 (lua_State* L, int lo, int dim, int def, tolua_Error* err);
TOLUA_API int tolua_isstringarray 
 (lua_State* L, int lo, int dim, int def, tolua_Error* err);
TOLUA_API int tolua_istablearray 
 (lua_State* L, int lo, int dim, int def, tolua_Error* err);
TOLUA_API int tolua_isuserdataarray 
 (lua_State* L, int lo, int dim, int def, tolua_Error* err);
TOLUA_API int tolua_isusertypearray 
 (lua_State* L, int lo, const char* type, int dim, int def, tolua_Error* err);

TOLUA_API void tolua_open (lua_State* L);

TOLUA_API void* tolua_copy (lua_State* L, void* value, unsigned int size);
TOLUA_API void* tolua_clone (lua_State* L, void* value, lua_CFunction func);

TOLUA_API void tolua_usertype (lua_State* L, const char* type);
TOLUA_API void tolua_beginmodule (lua_State* L, const char* name);
TOLUA_API void tolua_endmodule (lua_State* L);
TOLUA_API void tolua_module (lua_State* L, const char* name, int hasvar);
TOLUA_API void tolua_class (lua_State* L, const char* name, const char* base);
TOLUA_API void tolua_cclass (lua_State* L, const char* lname, const char* name, const char* base, lua_CFunction col);
TOLUA_API void tolua_function (lua_State* L, const char* name, lua_CFunction func);
TOLUA_API void tolua_constant (lua_State* L, const char* name, double value);
TOLUA_API void tolua_variable (lua_State* L, const char* name, lua_CFunction get, lua_CFunction set);
TOLUA_API void tolua_array (lua_State* L, const char* name, lua_CFunction get, lua_CFunction set);


TOLUA_API void tolua_pushvalue (lua_State* L, int lo);
TOLUA_API void tolua_pushboolean (lua_State* L, int value);
TOLUA_API void tolua_pushnumber (lua_State* L, double value);
TOLUA_API void tolua_pushstring (lua_State* L, const char* value);
TOLUA_API void tolua_pushuserdata (lua_State* L, void* value);
TOLUA_API void tolua_pushusertype (lua_State* L, void* value, const char* type);
TOLUA_API void tolua_pushfieldvalue (lua_State* L, int lo, int index, int v);
TOLUA_API void tolua_pushfieldboolean (lua_State* L, int lo, int index, int v);
TOLUA_API void tolua_pushfieldnumber (lua_State* L, int lo, int index, double v);
TOLUA_API void tolua_pushfieldstring (lua_State* L, int lo, int index, const char* v);
TOLUA_API void tolua_pushfielduserdata (lua_State* L, int lo, int index, void* v);
TOLUA_API void tolua_pushfieldusertype (lua_State* L, int lo, int index, void* v, const char* type);
TOLUA_API void tolua_release (lua_State* L, void* value);

TOLUA_API double tolua_tonumber (lua_State* L, int narg, double def);
TOLUA_API const char* tolua_tostring (lua_State* L, int narg, const char* def);
TOLUA_API void* tolua_touserdata (lua_State* L, int narg, void* def);
TOLUA_API void* tolua_tousertype (lua_State* L, int narg, void* def);
TOLUA_API int tolua_tovalue (lua_State* L, int narg, int def);
TOLUA_API int tolua_toboolean (lua_State* L, int narg, int def);
TOLUA_API double tolua_tofieldnumber (lua_State* L, int lo, int index, double def);
TOLUA_API int tolua_tofieldboolean (lua_State* L, int lo, int index, int def);
TOLUA_API const char* tolua_tofieldstring (lua_State* L, int lo, int index, const char* def);
TOLUA_API void* tolua_tofielduserdata (lua_State* L, int lo, int index, void* def);
TOLUA_API void* tolua_tofieldusertype (lua_State* L, int lo, int index, void* def);
TOLUA_API int tolua_tofieldvalue (lua_State* L, int lo, int index, int def);
TOLUA_API int tolua_getfieldboolean (lua_State* L, int lo, int index, int def);

TOLUA_API void tolua_newmetatable (lua_State* L, const char* name);
TOLUA_API void tolua_getmetatable (lua_State* L, const char* name);

#ifdef __cplusplus
}
#endif

#endif
