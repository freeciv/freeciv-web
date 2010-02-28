/*
** Lua binding: api
** Generated automatically by toluaxx-1.1.0 on Thu Oct  8 11:26:28 2009.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "toluaxx.h"

#ifdef __cplusplus
#include<string>
 inline const char* tolua_tocppstring(lua_State* L, int narg, std::string def){return tolua_tocppstring(L,narg,def.c_str());}
 inline const char* tolua_tofieldcppstring(lua_State* L, int lo, int index, std::string def){return tolua_tofieldcppstring(L,lo,index,def.c_str());}
#endif
/* Exported function */
TOLUA_API int tolua_api_open (lua_State* tolua_S);

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "api_types.h"
#include "api_actions.h"
#include "api_find.h"
#include "api_intl.h"
#include "api_methods.h"
#include "api_notify.h"
#include "api_utilities.h"
#include "script.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"Government");
 tolua_usertype(tolua_S,"Tile");
 tolua_usertype(tolua_S,"Terrain");
 tolua_usertype(tolua_S,"Player_ai");
 tolua_usertype(tolua_S,"Nation_Type");
 tolua_usertype(tolua_S,"City");
 tolua_usertype(tolua_S,"Tech_Type");
 tolua_usertype(tolua_S,"Unit_Type");
 tolua_usertype(tolua_S,"Building_Type");
 tolua_usertype(tolua_S,"Player");
 tolua_usertype(tolua_S,"Unit");
}

/* get function:control of class Player_ai */
#ifndef TOLUA_DISABLE_tolua_get_Player_ai_control
static int tolua_get_Player_ai_control (lua_State* tolua_S) {
  Player_ai*self = (Player_ai*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'control'",NULL);
#endif
 tolua_pushboolean(tolua_S,(bool)self->control);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Player_ai_control */

/* get function:control of class Player_ai */
#ifndef TOLUA_DISABLE_tolua_set_Player_ai_control
static int tolua_set_Player_ai_control (lua_State* tolua_S) {
  Player_ai*self = (Player_ai*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'control'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isboolean(tolua_S,2,0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->control=(( bool) tolua_toboolean(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Player_ai_control */

/* get function:name of class Player */
#ifndef TOLUA_DISABLE_tolua_get_Player_name
static int tolua_get_Player_name (lua_State* tolua_S) {
  Player*self = (Player*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'name'",NULL);
#endif
 tolua_pushstring(tolua_S,(const char*)self->name);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Player_name */

/* get function:nation of class Player */
#ifndef TOLUA_DISABLE_tolua_get_Player_nation_ptr
static int tolua_get_Player_nation_ptr (lua_State* tolua_S) {
  Player*self = (Player*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'nation'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)self->nation,"Nation_Type");
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Player_nation_ptr */

/* get function:nation of class Player */
#ifndef TOLUA_DISABLE_tolua_set_Player_nation_ptr
static int tolua_set_Player_nation_ptr (lua_State* tolua_S) {
  Player*self = (Player*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'nation'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"Nation_Type",0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->nation=(( Nation_Type*) tolua_tousertype(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Player_nation_ptr */

/* get function:ai_data of class Player */
#ifndef TOLUA_DISABLE_tolua_get_Player_ai_data
static int tolua_get_Player_ai_data (lua_State* tolua_S) {
  Player*self = (Player*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'ai_data'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)&self->ai_data,"Player_ai");
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Player_ai_data */

/* get function:ai_data of class Player */
#ifndef TOLUA_DISABLE_tolua_set_Player_ai_data
static int tolua_set_Player_ai_data (lua_State* tolua_S) {
  Player*self = (Player*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'ai_data'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"Player_ai",0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->ai_data=*(( Player_ai*) tolua_tousertype(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Player_ai_data */

/* get function:name of class City */
#ifndef TOLUA_DISABLE_tolua_get_City_name
static int tolua_get_City_name (lua_State* tolua_S) {
  City*self = (City*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'name'",NULL);
#endif
 tolua_pushstring(tolua_S,(const char*)self->name);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_City_name */

/* get function:owner of class City */
#ifndef TOLUA_DISABLE_tolua_get_City_owner_ptr
static int tolua_get_City_owner_ptr (lua_State* tolua_S) {
  City*self = (City*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'owner'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)self->owner,"Player");
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_City_owner_ptr */

/* get function:owner of class City */
#ifndef TOLUA_DISABLE_tolua_set_City_owner_ptr
static int tolua_set_City_owner_ptr (lua_State* tolua_S) {
  City*self = (City*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'owner'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"Player",0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->owner=(( Player*) tolua_tousertype(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_City_owner_ptr */

/* get function:tile of class City */
#ifndef TOLUA_DISABLE_tolua_get_City_tile_ptr
static int tolua_get_City_tile_ptr (lua_State* tolua_S) {
  City*self = (City*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'tile'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)self->tile,"Tile");
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_City_tile_ptr */

/* get function:tile of class City */
#ifndef TOLUA_DISABLE_tolua_set_City_tile_ptr
static int tolua_set_City_tile_ptr (lua_State* tolua_S) {
  City*self = (City*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'tile'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"Tile",0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->tile=(( Tile*) tolua_tousertype(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_City_tile_ptr */

/* get function:id of class City */
#ifndef TOLUA_DISABLE_tolua_get_City_id
static int tolua_get_City_id (lua_State* tolua_S) {
  City*self = (City*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'id'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->id);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_City_id */

/* get function:utype of class Unit */
#ifndef TOLUA_DISABLE_tolua_get_Unit_utype_ptr
static int tolua_get_Unit_utype_ptr (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'utype'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)self->utype,"Unit_Type");
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Unit_utype_ptr */

/* get function:utype of class Unit */
#ifndef TOLUA_DISABLE_tolua_set_Unit_utype_ptr
static int tolua_set_Unit_utype_ptr (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'utype'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"Unit_Type",0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->utype=(( Unit_Type*) tolua_tousertype(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Unit_utype_ptr */

/* get function:owner of class Unit */
#ifndef TOLUA_DISABLE_tolua_get_Unit_owner_ptr
static int tolua_get_Unit_owner_ptr (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'owner'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)self->owner,"Player");
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Unit_owner_ptr */

/* get function:owner of class Unit */
#ifndef TOLUA_DISABLE_tolua_set_Unit_owner_ptr
static int tolua_set_Unit_owner_ptr (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'owner'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"Player",0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->owner=(( Player*) tolua_tousertype(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Unit_owner_ptr */

/* get function:homecity of class Unit */
#ifndef TOLUA_DISABLE_tolua_get_Unit_homecity
static int tolua_get_Unit_homecity (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'homecity'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->homecity);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Unit_homecity */

/* get function:homecity of class Unit */
#ifndef TOLUA_DISABLE_tolua_set_Unit_homecity
static int tolua_set_Unit_homecity (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'homecity'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->homecity=(( int) tolua_tonumber(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Unit_homecity */

/* get function:tile of class Unit */
#ifndef TOLUA_DISABLE_tolua_get_Unit_tile_ptr
static int tolua_get_Unit_tile_ptr (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'tile'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)self->tile,"Tile");
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Unit_tile_ptr */

/* get function:tile of class Unit */
#ifndef TOLUA_DISABLE_tolua_set_Unit_tile_ptr
static int tolua_set_Unit_tile_ptr (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'tile'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"Tile",0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->tile=(( Tile*) tolua_tousertype(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Unit_tile_ptr */

/* get function:id of class Unit */
#ifndef TOLUA_DISABLE_tolua_get_Unit_id
static int tolua_get_Unit_id (lua_State* tolua_S) {
  Unit*self = (Unit*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'id'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->id);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Unit_id */

/* get function:nat_x of class Tile */
#ifndef TOLUA_DISABLE_tolua_get_Tile_nat_x
static int tolua_get_Tile_nat_x (lua_State* tolua_S) {
  Tile*self = (Tile*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'nat_x'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->nat_x);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Tile_nat_x */

/* get function:nat_y of class Tile */
#ifndef TOLUA_DISABLE_tolua_get_Tile_nat_y
static int tolua_get_Tile_nat_y (lua_State* tolua_S) {
  Tile*self = (Tile*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'nat_y'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->nat_y);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Tile_nat_y */

/* get function:terrain of class Tile */
#ifndef TOLUA_DISABLE_tolua_get_Tile_terrain_ptr
static int tolua_get_Tile_terrain_ptr (lua_State* tolua_S) {
  Tile*self = (Tile*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'terrain'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)self->terrain,"Terrain");
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Tile_terrain_ptr */

/* get function:terrain of class Tile */
#ifndef TOLUA_DISABLE_tolua_set_Tile_terrain_ptr
static int tolua_set_Tile_terrain_ptr (lua_State* tolua_S) {
  Tile*self = (Tile*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'terrain'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"Terrain",0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->terrain=(( Terrain*) tolua_tousertype(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Tile_terrain_ptr */

/* get function:index of class Tile */
#ifndef TOLUA_DISABLE_tolua_get_Tile_id
static int tolua_get_Tile_id (lua_State* tolua_S) {
  Tile*self = (Tile*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'index'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->index);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Tile_id */

/* get function:item_number of class Government */
#ifndef TOLUA_DISABLE_tolua_get_Government_id
static int tolua_get_Government_id (lua_State* tolua_S) {
  Government*self = (Government*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'item_number'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->item_number);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Government_id */

/* get function:item_number of class Nation_Type */
#ifndef TOLUA_DISABLE_tolua_get_Nation_Type_id
static int tolua_get_Nation_Type_id (lua_State* tolua_S) {
  Nation_Type*self = (Nation_Type*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'item_number'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->item_number);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Nation_Type_id */

/* get function:build_cost of class Building_Type */
#ifndef TOLUA_DISABLE_tolua_get_Building_Type_build_cost
static int tolua_get_Building_Type_build_cost (lua_State* tolua_S) {
  Building_Type*self = (Building_Type*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'build_cost'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->build_cost);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Building_Type_build_cost */

/* get function:build_cost of class Building_Type */
#ifndef TOLUA_DISABLE_tolua_set_Building_Type_build_cost
static int tolua_set_Building_Type_build_cost (lua_State* tolua_S) {
  Building_Type*self = (Building_Type*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'build_cost'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->build_cost=(( int) tolua_tonumber(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Building_Type_build_cost */

/* get function:item_number of class Building_Type */
#ifndef TOLUA_DISABLE_tolua_get_Building_Type_id
static int tolua_get_Building_Type_id (lua_State* tolua_S) {
  Building_Type*self = (Building_Type*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'item_number'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->item_number);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Building_Type_id */

/* get function:build_cost of class Unit_Type */
#ifndef TOLUA_DISABLE_tolua_get_Unit_Type_build_cost
static int tolua_get_Unit_Type_build_cost (lua_State* tolua_S) {
  Unit_Type*self = (Unit_Type*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'build_cost'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->build_cost);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Unit_Type_build_cost */

/* get function:build_cost of class Unit_Type */
#ifndef TOLUA_DISABLE_tolua_set_Unit_Type_build_cost
static int tolua_set_Unit_Type_build_cost (lua_State* tolua_S) {
  Unit_Type*self = (Unit_Type*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'build_cost'",NULL);
#endif
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err)) tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 self->build_cost=(( int) tolua_tonumber(tolua_S,2,0));
 return 0;}
#endif /* #ifndef TOLUA_DISABLE_tolua_set_Unit_Type_build_cost */

/* get function:item_number of class Unit_Type */
#ifndef TOLUA_DISABLE_tolua_get_Unit_Type_id
static int tolua_get_Unit_Type_id (lua_State* tolua_S) {
  Unit_Type*self = (Unit_Type*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'item_number'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->item_number);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Unit_Type_id */

/* get function:item_number of class Tech_Type */
#ifndef TOLUA_DISABLE_tolua_get_Tech_Type_id
static int tolua_get_Tech_Type_id (lua_State* tolua_S) {
  Tech_Type*self = (Tech_Type*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'item_number'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->item_number);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Tech_Type_id */

/* get function:item_number of class Terrain */
#ifndef TOLUA_DISABLE_tolua_get_Terrain_id
static int tolua_get_Terrain_id (lua_State* tolua_S) {
  Terrain*self = (Terrain*) tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'item_number'",NULL);
#endif
 tolua_pushnumber(tolua_S,(lua_Number)self->item_number);
 return 1;}
#endif /* #ifndef TOLUA_DISABLE_tolua_get_Terrain_id */

/* function: api_methods_player_num_cities */
#ifndef TOLUA_DISABLE_tolua_api_methods_player_num_cities00
static int tolua_api_methods_player_num_cities00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
 {
  int tolua_ret = (int)  api_methods_player_num_cities(pplayer);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_player_num_cities'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_player_num_units */
#ifndef TOLUA_DISABLE_tolua_api_methods_player_num_units00
static int tolua_api_methods_player_num_units00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
 {
  int tolua_ret = (int)  api_methods_player_num_units(pplayer);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_player_num_units'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_player_has_wonder */
#ifndef TOLUA_DISABLE_tolua_api_methods_player_has_wonder00
static int tolua_api_methods_player_has_wonder00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"Building_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  Building_Type* building = ((Building_Type*)  tolua_tousertype(tolua_S,2,0));
 {
  bool tolua_ret = (bool)  api_methods_player_has_wonder(pplayer,building);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_player_has_wonder'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_player_victory */
#ifndef TOLUA_DISABLE_tolua_api_methods_player_victory00
static int tolua_api_methods_player_victory00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
 {
  api_methods_player_victory(pplayer);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_player_victory'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_unit_city_can_be_built_here */
#ifndef TOLUA_DISABLE_tolua_api_methods_unit_city_can_be_built_here00
static int tolua_api_methods_unit_city_can_be_built_here00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Unit",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Unit* punit = ((Unit*)  tolua_tousertype(tolua_S,1,0));
 {
  bool tolua_ret = (bool)  api_methods_unit_city_can_be_built_here(punit);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_unit_city_can_be_built_here'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_tile_city_exists_within_city_radius */
#ifndef TOLUA_DISABLE_tolua_api_methods_tile_city_exists_within_city_radius00
static int tolua_api_methods_tile_city_exists_within_city_radius00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Tile",0,&tolua_err) ||
 !tolua_isboolean(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Tile* ptile = ((Tile*)  tolua_tousertype(tolua_S,1,0));
  bool center = ((bool)  tolua_toboolean(tolua_S,2,0));
 {
  bool tolua_ret = (bool)  api_methods_tile_city_exists_within_city_radius(ptile,center);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_tile_city_exists_within_city_radius'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_government_rule_name */
#ifndef TOLUA_DISABLE_tolua_api_methods_government_rule_name00
static int tolua_api_methods_government_rule_name00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Government",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Government* pgovernment = ((Government*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_government_rule_name(pgovernment);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_government_rule_name'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_government_name_translation */
#ifndef TOLUA_DISABLE_tolua_api_methods_government_name_translation00
static int tolua_api_methods_government_name_translation00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Government",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Government* pgovernment = ((Government*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_government_name_translation(pgovernment);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_government_name_translation'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_nation_type_rule_name */
#ifndef TOLUA_DISABLE_tolua_api_methods_nation_type_rule_name00
static int tolua_api_methods_nation_type_rule_name00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Nation_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Nation_Type* pnation = ((Nation_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_nation_type_rule_name(pnation);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_nation_type_rule_name'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_nation_type_name_translation */
#ifndef TOLUA_DISABLE_tolua_api_methods_nation_type_name_translation00
static int tolua_api_methods_nation_type_name_translation00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Nation_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Nation_Type* pnation = ((Nation_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_nation_type_name_translation(pnation);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_nation_type_name_translation'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_nation_type_plural_translation */
#ifndef TOLUA_DISABLE_tolua_api_methods_nation_type_plural_translation00
static int tolua_api_methods_nation_type_plural_translation00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Nation_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Nation_Type* pnation = ((Nation_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_nation_type_plural_translation(pnation);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_nation_type_plural_translation'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_building_type_is_wonder */
#ifndef TOLUA_DISABLE_tolua_api_methods_building_type_is_wonder00
static int tolua_api_methods_building_type_is_wonder00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Building_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Building_Type* pbuilding = ((Building_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  bool tolua_ret = (bool)  api_methods_building_type_is_wonder(pbuilding);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_building_type_is_wonder'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_building_type_is_great_wonder */
#ifndef TOLUA_DISABLE_tolua_api_methods_building_type_is_great_wonder00
static int tolua_api_methods_building_type_is_great_wonder00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Building_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Building_Type* pbuilding = ((Building_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  bool tolua_ret = (bool)  api_methods_building_type_is_great_wonder(pbuilding);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_building_type_is_great_wonder'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_building_type_is_small_wonder */
#ifndef TOLUA_DISABLE_tolua_api_methods_building_type_is_small_wonder00
static int tolua_api_methods_building_type_is_small_wonder00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Building_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Building_Type* pbuilding = ((Building_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  bool tolua_ret = (bool)  api_methods_building_type_is_small_wonder(pbuilding);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_building_type_is_small_wonder'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_building_type_is_improvement */
#ifndef TOLUA_DISABLE_tolua_api_methods_building_type_is_improvement00
static int tolua_api_methods_building_type_is_improvement00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Building_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Building_Type* pbuilding = ((Building_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  bool tolua_ret = (bool)  api_methods_building_type_is_improvement(pbuilding);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_building_type_is_improvement'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_building_type_rule_name */
#ifndef TOLUA_DISABLE_tolua_api_methods_building_type_rule_name00
static int tolua_api_methods_building_type_rule_name00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Building_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Building_Type* pbuilding = ((Building_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_building_type_rule_name(pbuilding);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_building_type_rule_name'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_building_type_name_translation */
#ifndef TOLUA_DISABLE_tolua_api_methods_building_type_name_translation00
static int tolua_api_methods_building_type_name_translation00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Building_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Building_Type* pbuilding = ((Building_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_building_type_name_translation(pbuilding);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_building_type_name_translation'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_unit_type_has_flag */
#ifndef TOLUA_DISABLE_tolua_api_methods_unit_type_has_flag00
static int tolua_api_methods_unit_type_has_flag00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Unit_Type",0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Unit_Type* punit_type = ((Unit_Type*)  tolua_tousertype(tolua_S,1,0));
  const char* flag = ((const char*)  tolua_tostring(tolua_S,2,0));
 {
  bool tolua_ret = (bool)  api_methods_unit_type_has_flag(punit_type,flag);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_unit_type_has_flag'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_unit_type_has_role */
#ifndef TOLUA_DISABLE_tolua_api_methods_unit_type_has_role00
static int tolua_api_methods_unit_type_has_role00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Unit_Type",0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Unit_Type* punit_type = ((Unit_Type*)  tolua_tousertype(tolua_S,1,0));
  const char* role = ((const char*)  tolua_tostring(tolua_S,2,0));
 {
  bool tolua_ret = (bool)  api_methods_unit_type_has_role(punit_type,role);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_unit_type_has_role'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_unit_type_rule_name */
#ifndef TOLUA_DISABLE_tolua_api_methods_unit_type_rule_name00
static int tolua_api_methods_unit_type_rule_name00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Unit_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Unit_Type* punit_type = ((Unit_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_unit_type_rule_name(punit_type);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_unit_type_rule_name'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_unit_type_name_translation */
#ifndef TOLUA_DISABLE_tolua_api_methods_unit_type_name_translation00
static int tolua_api_methods_unit_type_name_translation00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Unit_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Unit_Type* punit_type = ((Unit_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_unit_type_name_translation(punit_type);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_unit_type_name_translation'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_tech_type_rule_name */
#ifndef TOLUA_DISABLE_tolua_api_methods_tech_type_rule_name00
static int tolua_api_methods_tech_type_rule_name00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Tech_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Tech_Type* ptech = ((Tech_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_tech_type_rule_name(ptech);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_tech_type_rule_name'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_tech_type_name_translation */
#ifndef TOLUA_DISABLE_tolua_api_methods_tech_type_name_translation00
static int tolua_api_methods_tech_type_name_translation00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Tech_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Tech_Type* ptech = ((Tech_Type*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_tech_type_name_translation(ptech);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_tech_type_name_translation'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_terrain_rule_name */
#ifndef TOLUA_DISABLE_tolua_api_methods_terrain_rule_name00
static int tolua_api_methods_terrain_rule_name00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Terrain",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Terrain* pterrain = ((Terrain*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_terrain_rule_name(pterrain);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_terrain_rule_name'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_terrain_name_translation */
#ifndef TOLUA_DISABLE_tolua_api_methods_terrain_name_translation00
static int tolua_api_methods_terrain_name_translation00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Terrain",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Terrain* pterrain = ((Terrain*)  tolua_tousertype(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_methods_terrain_name_translation(pterrain);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_terrain_name_translation'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_methods_city_has_building */
#ifndef TOLUA_DISABLE_tolua_api_methods_city_has_building00
static int tolua_api_methods_city_has_building00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"City",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"Building_Type",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  City* pcity = ((City*)  tolua_tousertype(tolua_S,1,0));
  Building_Type* building = ((Building_Type*)  tolua_tousertype(tolua_S,2,0));
 {
  bool tolua_ret = (bool)  api_methods_city_has_building(pcity,building);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'methods_city_has_building'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_player */
#ifndef TOLUA_DISABLE_tolua_api_find_player00
static int tolua_api_find_player00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int player_id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  Player* tolua_ret = (Player*)  api_find_player(player_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Player");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'player'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_city */
#ifndef TOLUA_DISABLE_tolua_api_find_city00
static int tolua_api_find_city00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  int city_id = ((int)  tolua_tonumber(tolua_S,2,0));
 {
  City* tolua_ret = (City*)  api_find_city(pplayer,city_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"City");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'city'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_unit */
#ifndef TOLUA_DISABLE_tolua_api_find_unit00
static int tolua_api_find_unit00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  int unit_id = ((int)  tolua_tonumber(tolua_S,2,0));
 {
  Unit* tolua_ret = (Unit*)  api_find_unit(pplayer,unit_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Unit");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'unit'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_tile */
#ifndef TOLUA_DISABLE_tolua_api_find_tile00
static int tolua_api_find_tile00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int nat_x = ((int)  tolua_tonumber(tolua_S,1,0));
  int nat_y = ((int)  tolua_tonumber(tolua_S,2,0));
 {
  Tile* tolua_ret = (Tile*)  api_find_tile(nat_x,nat_y);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Tile");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'tile'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_government */
#ifndef TOLUA_DISABLE_tolua_api_find_government00
static int tolua_api_find_government00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int government_id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  Government* tolua_ret = (Government*)  api_find_government(government_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Government");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'government'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_government_by_name */
#ifndef TOLUA_DISABLE_tolua_api_find_government01
static int tolua_api_find_government01(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* name_orig = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  Government* tolua_ret = (Government*)  api_find_government_by_name(name_orig);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Government");
 }
 }
 return 1;
tolua_lerror:
 return tolua_api_find_government00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_nation_type_by_name */
#ifndef TOLUA_DISABLE_tolua_api_find_nation_type00
static int tolua_api_find_nation_type00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* name_orig = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  Nation_Type* tolua_ret = (Nation_Type*)  api_find_nation_type_by_name(name_orig);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Nation_Type");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'nation_type'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_nation_type */
#ifndef TOLUA_DISABLE_tolua_api_find_nation_type01
static int tolua_api_find_nation_type01(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int nation_type_id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  Nation_Type* tolua_ret = (Nation_Type*)  api_find_nation_type(nation_type_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Nation_Type");
 }
 }
 return 1;
tolua_lerror:
 return tolua_api_find_nation_type00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_building_type_by_name */
#ifndef TOLUA_DISABLE_tolua_api_find_building_type00
static int tolua_api_find_building_type00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* name_orig = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  Building_Type* tolua_ret = (Building_Type*)  api_find_building_type_by_name(name_orig);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Building_Type");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'building_type'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_building_type */
#ifndef TOLUA_DISABLE_tolua_api_find_building_type01
static int tolua_api_find_building_type01(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int building_type_id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  Building_Type* tolua_ret = (Building_Type*)  api_find_building_type(building_type_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Building_Type");
 }
 }
 return 1;
tolua_lerror:
 return tolua_api_find_building_type00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_unit_type_by_name */
#ifndef TOLUA_DISABLE_tolua_api_find_unit_type00
static int tolua_api_find_unit_type00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* name_orig = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  Unit_Type* tolua_ret = (Unit_Type*)  api_find_unit_type_by_name(name_orig);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Unit_Type");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'unit_type'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_unit_type */
#ifndef TOLUA_DISABLE_tolua_api_find_unit_type01
static int tolua_api_find_unit_type01(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int unit_type_id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  Unit_Type* tolua_ret = (Unit_Type*)  api_find_unit_type(unit_type_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Unit_Type");
 }
 }
 return 1;
tolua_lerror:
 return tolua_api_find_unit_type00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_role_unit_type */
#ifndef TOLUA_DISABLE_tolua_api_find_role_unit_type00
static int tolua_api_find_role_unit_type00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"Player",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* role_name = ((const char*)  tolua_tostring(tolua_S,1,0));
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,2,0));
 {
  Unit_Type* tolua_ret = (Unit_Type*)  api_find_role_unit_type(role_name,pplayer);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Unit_Type");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'role_unit_type'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_tech_type_by_name */
#ifndef TOLUA_DISABLE_tolua_api_find_tech_type00
static int tolua_api_find_tech_type00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* name_orig = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  Tech_Type* tolua_ret = (Tech_Type*)  api_find_tech_type_by_name(name_orig);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Tech_Type");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'tech_type'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_tech_type */
#ifndef TOLUA_DISABLE_tolua_api_find_tech_type01
static int tolua_api_find_tech_type01(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int tech_type_id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  Tech_Type* tolua_ret = (Tech_Type*)  api_find_tech_type(tech_type_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Tech_Type");
 }
 }
 return 1;
tolua_lerror:
 return tolua_api_find_tech_type00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_terrain_by_name */
#ifndef TOLUA_DISABLE_tolua_api_find_terrain00
static int tolua_api_find_terrain00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* name_orig = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  Terrain* tolua_ret = (Terrain*)  api_find_terrain_by_name(name_orig);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Terrain");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'terrain'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_find_terrain */
#ifndef TOLUA_DISABLE_tolua_api_find_terrain01
static int tolua_api_find_terrain01(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int terrain_id = ((int)  tolua_tonumber(tolua_S,1,0));
 {
  Terrain* tolua_ret = (Terrain*)  api_find_terrain(terrain_id);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Terrain");
 }
 }
 return 1;
tolua_lerror:
 return tolua_api_find_terrain00(tolua_S);
}
#endif //#ifndef TOLUA_DISABLE

/* function: script_signal_connect */
#ifndef TOLUA_DISABLE_tolua_api_signal_connect00
static int tolua_api_signal_connect00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* signal_name = ((const char*)  tolua_tostring(tolua_S,1,0));
  const char* callback_name = ((const char*)  tolua_tostring(tolua_S,2,0));
 {
  script_signal_connect(signal_name,callback_name);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'connect'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_intl__ */
#ifndef TOLUA_DISABLE_tolua_api__00
static int tolua_api__00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* untranslated = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_intl__(untranslated);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function '_'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_intl_N_ */
#ifndef TOLUA_DISABLE_tolua_api_N_00
static int tolua_api_N_00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* untranslated = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_intl_N_(untranslated);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'N_'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_intl_Q_ */
#ifndef TOLUA_DISABLE_tolua_api_Q_00
static int tolua_api_Q_00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* untranslated = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  const char* tolua_ret = (const char*)  api_intl_Q_(untranslated);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'Q_'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_intl_PL_ */
#ifndef TOLUA_DISABLE_tolua_api_PL_00
static int tolua_api_PL_00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* singular = ((const char*)  tolua_tostring(tolua_S,1,0));
  const char* plural = ((const char*)  tolua_tostring(tolua_S,2,0));
  int n = ((int)  tolua_tonumber(tolua_S,3,0));
 {
  const char* tolua_ret = (const char*)  api_intl_PL_(singular,plural,n);
 tolua_pushstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'PL_'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_notify_embassies_msg */
#ifndef TOLUA_DISABLE_tolua_api_notify_embassies_msg00
static int tolua_api_notify_embassies_msg00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"Tile",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
 !tolua_isstring(tolua_S,4,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  Tile* ptile = ((Tile*)  tolua_tousertype(tolua_S,2,0));
  int event = ((int)  tolua_tonumber(tolua_S,3,0));
  const char* message = ((const char*)  tolua_tostring(tolua_S,4,0));
 {
  api_notify_embassies_msg(pplayer,ptile,event,message);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'embassies_msg'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_notify_event_msg */
#ifndef TOLUA_DISABLE_tolua_api_notify_event_msg00
static int tolua_api_notify_event_msg00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"Tile",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,3,0,&tolua_err) ||
 !tolua_isstring(tolua_S,4,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,5,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  Tile* ptile = ((Tile*)  tolua_tousertype(tolua_S,2,0));
  int event = ((int)  tolua_tonumber(tolua_S,3,0));
  const char* message = ((const char*)  tolua_tostring(tolua_S,4,0));
 {
  api_notify_event_msg(pplayer,ptile,event,message);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'event_msg'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_utilities_random */
#ifndef TOLUA_DISABLE_tolua_api_random00
static int tolua_api_random00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isnumber(tolua_S,1,0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  int min = ((int)  tolua_tonumber(tolua_S,1,0));
  int max = ((int)  tolua_tonumber(tolua_S,2,0));
 {
  int tolua_ret = (int)  api_utilities_random(min,max);
 tolua_pushnumber(tolua_S,(lua_Number)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'random'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_utilities_error_log */
#ifndef TOLUA_DISABLE_tolua_api_error_log00
static int tolua_api_error_log00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* msg = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  api_utilities_error_log(msg);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'error_log'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_utilities_debug_log */
#ifndef TOLUA_DISABLE_tolua_api_debug_log00
static int tolua_api_debug_log00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isstring(tolua_S,1,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  const char* msg = ((const char*)  tolua_tostring(tolua_S,1,0));
 {
  api_utilities_debug_log(msg);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'debug_log'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_actions_create_unit */
#ifndef TOLUA_DISABLE_tolua_api_create_unit00
static int tolua_api_create_unit00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"Tile",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,3,"Unit_Type",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,4,0,&tolua_err) ||
 !tolua_isusertype(tolua_S,5,"City",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,6,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,7,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  Tile* ptile = ((Tile*)  tolua_tousertype(tolua_S,2,0));
  Unit_Type* ptype = ((Unit_Type*)  tolua_tousertype(tolua_S,3,0));
  int veteran_level = ((int)  tolua_tonumber(tolua_S,4,0));
  City* homecity = ((City*)  tolua_tousertype(tolua_S,5,0));
  int moves_left = ((int)  tolua_tonumber(tolua_S,6,0));
 {
  Unit* tolua_ret = (Unit*)  api_actions_create_unit(pplayer,ptile,ptype,veteran_level,homecity,moves_left);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Unit");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'create_unit'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_actions_create_city */
#ifndef TOLUA_DISABLE_tolua_api_create_city00
static int tolua_api_create_city00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"Tile",0,&tolua_err) ||
 !tolua_isstring(tolua_S,3,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  Tile* ptile = ((Tile*)  tolua_tousertype(tolua_S,2,0));
  const char* name = ((const char*)  tolua_tostring(tolua_S,3,0));
 {
  api_actions_create_city(pplayer,ptile,name);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'create_city'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_actions_create_base */
#ifndef TOLUA_DISABLE_tolua_api_create_base00
static int tolua_api_create_base00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Tile",0,&tolua_err) ||
 !tolua_isstring(tolua_S,2,0,&tolua_err) ||
 !tolua_isusertype(tolua_S,3,"Player",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Tile* ptile = ((Tile*)  tolua_tousertype(tolua_S,1,0));
  const char* name = ((const char*)  tolua_tostring(tolua_S,2,0));
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,3,0));
 {
  api_actions_create_base(ptile,name,pplayer);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'create_base'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_actions_change_gold */
#ifndef TOLUA_DISABLE_tolua_api_change_gold00
static int tolua_api_change_gold00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isnumber(tolua_S,2,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,3,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  int amount = ((int)  tolua_tonumber(tolua_S,2,0));
 {
  api_actions_change_gold(pplayer,amount);
 }
 }
 return 0;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'change_gold'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_actions_give_technology */
#ifndef TOLUA_DISABLE_tolua_api_give_technology00
static int tolua_api_give_technology00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Player",0,&tolua_err) ||
 !tolua_isusertype(tolua_S,2,"Tech_Type",0,&tolua_err) ||
 !tolua_isstring(tolua_S,3,0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,4,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Player* pplayer = ((Player*)  tolua_tousertype(tolua_S,1,0));
  Tech_Type* ptech = ((Tech_Type*)  tolua_tousertype(tolua_S,2,0));
  const char* reason = ((const char*)  tolua_tostring(tolua_S,3,0));
 {
  Tech_Type* tolua_ret = (Tech_Type*)  api_actions_give_technology(pplayer,ptech,reason);
 tolua_pushusertype(tolua_S,(void*)tolua_ret,"Tech_Type");
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'give_technology'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* function: api_actions_unleash_barbarians */
#ifndef TOLUA_DISABLE_tolua_api_unleash_barbarians00
static int tolua_api_unleash_barbarians00(lua_State* tolua_S)
{
 tolua_Error tolua_err;  if (
 !tolua_isusertype(tolua_S,1,"Tile",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
 {
  Tile* ptile = ((Tile*)  tolua_tousertype(tolua_S,1,0));
 {
  bool tolua_ret = (bool)  api_actions_unleash_barbarians(ptile);
 tolua_pushboolean(tolua_S,(bool)tolua_ret);
 }
 }
 return 1;
tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'unleash_barbarians'.",&tolua_err);
 return 0;
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_api_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
 tolua_cclass(tolua_S,"Player_ai","Player_ai","",NULL);
 tolua_beginmodule(tolua_S,"Player_ai");
  tolua_variable(tolua_S,"control",tolua_get_Player_ai_control,tolua_set_Player_ai_control);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Player","Player","",NULL);
 tolua_beginmodule(tolua_S,"Player");
  tolua_variable(tolua_S,"name",tolua_get_Player_name,NULL);
  tolua_variable(tolua_S,"nation",tolua_get_Player_nation_ptr,tolua_set_Player_nation_ptr);
  tolua_variable(tolua_S,"ai_data",tolua_get_Player_ai_data,tolua_set_Player_ai_data);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"City","City","",NULL);
 tolua_beginmodule(tolua_S,"City");
  tolua_variable(tolua_S,"name",tolua_get_City_name,NULL);
  tolua_variable(tolua_S,"owner",tolua_get_City_owner_ptr,tolua_set_City_owner_ptr);
  tolua_variable(tolua_S,"tile",tolua_get_City_tile_ptr,tolua_set_City_tile_ptr);
  tolua_variable(tolua_S,"id",tolua_get_City_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Unit","Unit","",NULL);
 tolua_beginmodule(tolua_S,"Unit");
  tolua_variable(tolua_S,"utype",tolua_get_Unit_utype_ptr,tolua_set_Unit_utype_ptr);
  tolua_variable(tolua_S,"owner",tolua_get_Unit_owner_ptr,tolua_set_Unit_owner_ptr);
  tolua_variable(tolua_S,"homecity",tolua_get_Unit_homecity,tolua_set_Unit_homecity);
  tolua_variable(tolua_S,"tile",tolua_get_Unit_tile_ptr,tolua_set_Unit_tile_ptr);
  tolua_variable(tolua_S,"id",tolua_get_Unit_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Tile","Tile","",NULL);
 tolua_beginmodule(tolua_S,"Tile");
  tolua_variable(tolua_S,"nat_x",tolua_get_Tile_nat_x,NULL);
  tolua_variable(tolua_S,"nat_y",tolua_get_Tile_nat_y,NULL);
  tolua_variable(tolua_S,"terrain",tolua_get_Tile_terrain_ptr,tolua_set_Tile_terrain_ptr);
  tolua_variable(tolua_S,"id",tolua_get_Tile_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Government","Government","",NULL);
 tolua_beginmodule(tolua_S,"Government");
  tolua_variable(tolua_S,"id",tolua_get_Government_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Nation_Type","Nation_Type","",NULL);
 tolua_beginmodule(tolua_S,"Nation_Type");
  tolua_variable(tolua_S,"id",tolua_get_Nation_Type_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Building_Type","Building_Type","",NULL);
 tolua_beginmodule(tolua_S,"Building_Type");
  tolua_variable(tolua_S,"build_cost",tolua_get_Building_Type_build_cost,tolua_set_Building_Type_build_cost);
  tolua_variable(tolua_S,"id",tolua_get_Building_Type_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Unit_Type","Unit_Type","",NULL);
 tolua_beginmodule(tolua_S,"Unit_Type");
  tolua_variable(tolua_S,"build_cost",tolua_get_Unit_Type_build_cost,tolua_set_Unit_Type_build_cost);
  tolua_variable(tolua_S,"id",tolua_get_Unit_Type_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Tech_Type","Tech_Type","",NULL);
 tolua_beginmodule(tolua_S,"Tech_Type");
  tolua_variable(tolua_S,"id",tolua_get_Tech_Type_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"Terrain","Terrain","",NULL);
 tolua_beginmodule(tolua_S,"Terrain");
  tolua_variable(tolua_S,"id",tolua_get_Terrain_id,NULL);
 tolua_endmodule(tolua_S);
 tolua_function(tolua_S,"methods_player_num_cities",tolua_api_methods_player_num_cities00);
 tolua_function(tolua_S,"methods_player_num_units",tolua_api_methods_player_num_units00);
 tolua_function(tolua_S,"methods_player_has_wonder",tolua_api_methods_player_has_wonder00);
 tolua_function(tolua_S,"methods_player_victory",tolua_api_methods_player_victory00);
 tolua_function(tolua_S,"methods_unit_city_can_be_built_here",tolua_api_methods_unit_city_can_be_built_here00);
 tolua_function(tolua_S,"methods_tile_city_exists_within_city_radius",tolua_api_methods_tile_city_exists_within_city_radius00);
 tolua_function(tolua_S,"methods_government_rule_name",tolua_api_methods_government_rule_name00);
 tolua_function(tolua_S,"methods_government_name_translation",tolua_api_methods_government_name_translation00);
 tolua_function(tolua_S,"methods_nation_type_rule_name",tolua_api_methods_nation_type_rule_name00);
 tolua_function(tolua_S,"methods_nation_type_name_translation",tolua_api_methods_nation_type_name_translation00);
 tolua_function(tolua_S,"methods_nation_type_plural_translation",tolua_api_methods_nation_type_plural_translation00);
 tolua_function(tolua_S,"methods_building_type_is_wonder",tolua_api_methods_building_type_is_wonder00);
 tolua_function(tolua_S,"methods_building_type_is_great_wonder",tolua_api_methods_building_type_is_great_wonder00);
 tolua_function(tolua_S,"methods_building_type_is_small_wonder",tolua_api_methods_building_type_is_small_wonder00);
 tolua_function(tolua_S,"methods_building_type_is_improvement",tolua_api_methods_building_type_is_improvement00);
 tolua_function(tolua_S,"methods_building_type_rule_name",tolua_api_methods_building_type_rule_name00);
 tolua_function(tolua_S,"methods_building_type_name_translation",tolua_api_methods_building_type_name_translation00);
 tolua_function(tolua_S,"methods_unit_type_has_flag",tolua_api_methods_unit_type_has_flag00);
 tolua_function(tolua_S,"methods_unit_type_has_role",tolua_api_methods_unit_type_has_role00);
 tolua_function(tolua_S,"methods_unit_type_rule_name",tolua_api_methods_unit_type_rule_name00);
 tolua_function(tolua_S,"methods_unit_type_name_translation",tolua_api_methods_unit_type_name_translation00);
 tolua_function(tolua_S,"methods_tech_type_rule_name",tolua_api_methods_tech_type_rule_name00);
 tolua_function(tolua_S,"methods_tech_type_name_translation",tolua_api_methods_tech_type_name_translation00);
 tolua_function(tolua_S,"methods_terrain_rule_name",tolua_api_methods_terrain_rule_name00);
 tolua_function(tolua_S,"methods_terrain_name_translation",tolua_api_methods_terrain_name_translation00);
 tolua_function(tolua_S,"methods_city_has_building",tolua_api_methods_city_has_building00);

 { /* begin embedded lua code */
  int top = lua_gettop(tolua_S);
  static unsigned char B[] = {
  10,102,117,110, 99,116,105,111,110, 32, 80,108, 97,121,101,
  114, 58,105,115, 95,104,117,109, 97,110, 40, 41, 10,114,101,
  116,117,114,110, 32,110,111,116, 32,115,101,108,102, 46, 97,
  105, 95,100, 97,116, 97, 46, 99,111,110,116,114,111,108, 10,
  101,110,100, 10,102,117,110, 99,116,105,111,110, 32, 80,108,
   97,121,101,114, 58,110,117,109, 95, 99,105,116,105,101,115,
   40, 41, 10,114,101,116,117,114,110, 32,109,101,116,104,111,
  100,115, 95,112,108, 97,121,101,114, 95,110,117,109, 95, 99,
  105,116,105,101,115, 40,115,101,108,102, 41, 10,101,110,100,
   10,102,117,110, 99,116,105,111,110, 32, 80,108, 97,121,101,
  114, 58,110,117,109, 95,117,110,105,116,115, 40, 41, 10,114,
  101,116,117,114,110, 32,109,101,116,104,111,100,115, 95,112,
  108, 97,121,101,114, 95,110,117,109, 95,117,110,105,116,115,
   40,115,101,108,102, 41, 10,101,110,100, 10,102,117,110, 99,
  116,105,111,110, 32, 80,108, 97,121,101,114, 58,104, 97,115,
   95,119,111,110,100,101,114, 40, 98,117,105,108,100,105,110,
  103, 41, 10,114,101,116,117,114,110, 32,109,101,116,104,111,
  100,115, 95,112,108, 97,121,101,114, 95,104, 97,115, 95,119,
  111,110,100,101,114, 40,115,101,108,102, 44, 32, 98,117,105,
  108,100,105,110,103, 41, 10,101,110,100, 10,102,117,110, 99,
  116,105,111,110, 32, 80,108, 97,121,101,114, 58,118,105, 99,
  116,111,114,121, 40, 41, 10,109,101,116,104,111,100,115, 95,
  112,108, 97,121,101,114, 95,118,105, 99,116,111,114,121, 40,
  115,101,108,102, 41, 10,101,110,100, 10,102,117,110, 99,116,
  105,111,110, 32, 85,110,105,116, 58,103,101,116, 95,104,111,
  109,101, 99,105,116,121, 40, 41, 10,114,101,116,117,114,110,
   32,102,105,110,100, 46, 99,105,116,121, 40,115,101,108,102,
   46,111,119,110,101,114, 44, 32,115,101,108,102, 46,104,111,
  109,101, 99,105,116,121, 41, 10,101,110,100, 10,102,117,110,
   99,116,105,111,110, 32, 85,110,105,116, 58,105,115, 95,111,
  110, 95,112,111,115,115,105, 98,108,101, 95, 99,105,116,121,
   95,116,105,108,101, 40, 41, 10,114,101,116,117,114,110, 32,
  109,101,116,104,111,100,115, 95,117,110,105,116, 95, 99,105,
  116,121, 95, 99, 97,110, 95, 98,101, 95, 98,117,105,108,116,
   95,104,101,114,101, 40,115,101,108,102, 41, 10,101,110,100,
   10,102,117,110, 99,116,105,111,110, 32, 84,105,108,101, 58,
   99,105,116,121, 95,101,120,105,115,116,115, 95,119,105,116,
  104,105,110, 95, 99,105,116,121, 95,114, 97,100,105,117,115,
   40, 99,101,110,116,101,114, 41, 10,114,101,116,117,114,110,
   32,109,101,116,104,111,100,115, 95,116,105,108,101, 95, 99,
  105,116,121, 95,101,120,105,115,116,115, 95,119,105,116,104,
  105,110, 95, 99,105,116,121, 95,114, 97,100,105,117,115, 40,
  115,101,108,102, 44, 32, 99,101,110,116,101,114, 41, 10,101,
  110,100, 10,102,117,110, 99,116,105,111,110, 32, 71,111,118,
  101,114,110,109,101,110,116, 58,114,117,108,101, 95,110, 97,
  109,101, 40, 41, 10,114,101,116,117,114,110, 32,109,101,116,
  104,111,100,115, 95,103,111,118,101,114,110,109,101,110,116,
   95,114,117,108,101, 95,110, 97,109,101, 40,115,101,108,102,
   41, 10,101,110,100, 10,102,117,110, 99,116,105,111,110, 32,
   71,111,118,101,114,110,109,101,110,116, 58,110, 97,109,101,
   95,116,114, 97,110,115,108, 97,116,105,111,110, 40, 41, 10,
  114,101,116,117,114,110, 32,109,101,116,104,111,100,115, 95,
  103,111,118,101,114,110,109,101,110,116, 95,110, 97,109,101,
   95,116,114, 97,110,115,108, 97,116,105,111,110, 40,115,101,
  108,102, 41, 10,101,110,100, 10,102,117,110, 99,116,105,111,
  110, 32, 78, 97,116,105,111,110, 95, 84,121,112,101, 58,114,
  117,108,101, 95,110, 97,109,101, 40, 41, 10,114,101,116,117,
  114,110, 32,109,101,116,104,111,100,115, 95,110, 97,116,105,
  111,110, 95,116,121,112,101, 95,114,117,108,101, 95,110, 97,
  109,101, 40,115,101,108,102, 41, 10,101,110,100, 10,102,117,
  110, 99,116,105,111,110, 32, 78, 97,116,105,111,110, 95, 84,
  121,112,101, 58,110, 97,109,101, 95,116,114, 97,110,115,108,
   97,116,105,111,110, 40, 41, 10,114,101,116,117,114,110, 32,
  109,101,116,104,111,100,115, 95,110, 97,116,105,111,110, 95,
  116,121,112,101, 95,110, 97,109,101, 95,116,114, 97,110,115,
  108, 97,116,105,111,110, 40,115,101,108,102, 41, 10,101,110,
  100, 10,102,117,110, 99,116,105,111,110, 32, 78, 97,116,105,
  111,110, 95, 84,121,112,101, 58,112,108,117,114, 97,108, 95,
  116,114, 97,110,115,108, 97,116,105,111,110, 40, 41, 10,114,
  101,116,117,114,110, 32,109,101,116,104,111,100,115, 95,110,
   97,116,105,111,110, 95,116,121,112,101, 95,112,108,117,114,
   97,108, 95,116,114, 97,110,115,108, 97,116,105,111,110, 40,
  115,101,108,102, 41, 10,101,110,100, 10,102,117,110, 99,116,
  105,111,110, 32, 66,117,105,108,100,105,110,103, 95, 84,121,
  112,101, 58, 98,117,105,108,100, 95,115,104,105,101,108,100,
   95, 99,111,115,116, 40, 41, 10,114,101,116,117,114,110, 32,
  115,101,108,102, 46, 98,117,105,108,100, 95, 99,111,115,116,
   10,101,110,100, 10,102,117,110, 99,116,105,111,110, 32, 66,
  117,105,108,100,105,110,103, 95, 84,121,112,101, 58,105,115,
   95,119,111,110,100,101,114, 40, 41, 10,114,101,116,117,114,
  110, 32,109,101,116,104,111,100,115, 95, 98,117,105,108,100,
  105,110,103, 95,116,121,112,101, 95,105,115, 95,119,111,110,
  100,101,114, 40,115,101,108,102, 41, 10,101,110,100, 10,102,
  117,110, 99,116,105,111,110, 32, 66,117,105,108,100,105,110,
  103, 95, 84,121,112,101, 58,105,115, 95,103,114,101, 97,116,
   95,119,111,110,100,101,114, 40, 41, 10,114,101,116,117,114,
  110, 32,109,101,116,104,111,100,115, 95, 98,117,105,108,100,
  105,110,103, 95,116,121,112,101, 95,105,115, 95,103,114,101,
   97,116, 95,119,111,110,100,101,114, 40,115,101,108,102, 41,
   10,101,110,100, 10,102,117,110, 99,116,105,111,110, 32, 66,
  117,105,108,100,105,110,103, 95, 84,121,112,101, 58,105,115,
   95,115,109, 97,108,108, 95,119,111,110,100,101,114, 40, 41,
   10,114,101,116,117,114,110, 32,109,101,116,104,111,100,115,
   95, 98,117,105,108,100,105,110,103, 95,116,121,112,101, 95,
  105,115, 95,115,109, 97,108,108, 95,119,111,110,100,101,114,
   40,115,101,108,102, 41, 10,101,110,100, 10,102,117,110, 99,
  116,105,111,110, 32, 66,117,105,108,100,105,110,103, 95, 84,
  121,112,101, 58,105,115, 95,105,109,112,114,111,118,101,109,
  101,110,116, 40, 41, 10,114,101,116,117,114,110, 32,109,101,
  116,104,111,100,115, 95, 98,117,105,108,100,105,110,103, 95,
  116,121,112,101, 95,105,115, 95,105,109,112,114,111,118,101,
  109,101,110,116, 40,115,101,108,102, 41, 10,101,110,100, 10,
  102,117,110, 99,116,105,111,110, 32, 66,117,105,108,100,105,
  110,103, 95, 84,121,112,101, 58,114,117,108,101, 95,110, 97,
  109,101, 40, 41, 10,114,101,116,117,114,110, 32,109,101,116,
  104,111,100,115, 95, 98,117,105,108,100,105,110,103, 95,116,
  121,112,101, 95,114,117,108,101, 95,110, 97,109,101, 40,115,
  101,108,102, 41, 10,101,110,100, 10,102,117,110, 99,116,105,
  111,110, 32, 66,117,105,108,100,105,110,103, 95, 84,121,112,
  101, 58,110, 97,109,101, 95,116,114, 97,110,115,108, 97,116,
  105,111,110, 40, 41, 10,114,101,116,117,114,110, 32,109,101,
  116,104,111,100,115, 95, 98,117,105,108,100,105,110,103, 95,
  116,121,112,101, 95,110, 97,109,101, 95,116,114, 97,110,115,
  108, 97,116,105,111,110, 40,115,101,108,102, 41, 10,101,110,
  100, 10,102,117,110, 99,116,105,111,110, 32, 85,110,105,116,
   95, 84,121,112,101, 58, 98,117,105,108,100, 95,115,104,105,
  101,108,100, 95, 99,111,115,116, 40, 41, 10,114,101,116,117,
  114,110, 32,115,101,108,102, 46, 98,117,105,108,100, 95, 99,
  111,115,116, 10,101,110,100, 10,102,117,110, 99,116,105,111,
  110, 32, 85,110,105,116, 95, 84,121,112,101, 58,104, 97,115,
   95,102,108, 97,103, 40,102,108, 97,103, 41, 10,114,101,116,
  117,114,110, 32,109,101,116,104,111,100,115, 95,117,110,105,
  116, 95,116,121,112,101, 95,104, 97,115, 95,102,108, 97,103,
   40,115,101,108,102, 44, 32,102,108, 97,103, 41, 10,101,110,
  100, 10,102,117,110, 99,116,105,111,110, 32, 85,110,105,116,
   95, 84,121,112,101, 58,104, 97,115, 95,114,111,108,101, 40,
  114,111,108,101, 41, 10,114,101,116,117,114,110, 32,109,101,
  116,104,111,100,115, 95,117,110,105,116, 95,116,121,112,101,
   95,104, 97,115, 95,114,111,108,101, 40,115,101,108,102, 44,
   32,114,111,108,101, 41, 10,101,110,100, 10,102,117,110, 99,
  116,105,111,110, 32, 85,110,105,116, 95, 84,121,112,101, 58,
  114,117,108,101, 95,110, 97,109,101, 40, 41, 10,114,101,116,
  117,114,110, 32,109,101,116,104,111,100,115, 95,117,110,105,
  116, 95,116,121,112,101, 95,114,117,108,101, 95,110, 97,109,
  101, 40,115,101,108,102, 41, 10,101,110,100, 10,102,117,110,
   99,116,105,111,110, 32, 85,110,105,116, 95, 84,121,112,101,
   58,110, 97,109,101, 95,116,114, 97,110,115,108, 97,116,105,
  111,110, 40, 41, 10,114,101,116,117,114,110, 32,109,101,116,
  104,111,100,115, 95,117,110,105,116, 95,116,121,112,101, 95,
  110, 97,109,101, 95,116,114, 97,110,115,108, 97,116,105,111,
  110, 40,115,101,108,102, 41, 10,101,110,100, 10,102,117,110,
   99,116,105,111,110, 32, 84,101, 99,104, 95, 84,121,112,101,
   58,114,117,108,101, 95,110, 97,109,101, 40, 41, 10,114,101,
  116,117,114,110, 32,109,101,116,104,111,100,115, 95,116,101,
   99,104, 95,116,121,112,101, 95,114,117,108,101, 95,110, 97,
  109,101, 40,115,101,108,102, 41, 10,101,110,100, 10,102,117,
  110, 99,116,105,111,110, 32, 84,101, 99,104, 95, 84,121,112,
  101, 58,110, 97,109,101, 95,116,114, 97,110,115,108, 97,116,
  105,111,110, 40, 41, 10,114,101,116,117,114,110, 32,109,101,
  116,104,111,100,115, 95,116,101, 99,104, 95,116,121,112,101,
   95,110, 97,109,101, 95,116,114, 97,110,115,108, 97,116,105,
  111,110, 40,115,101,108,102, 41, 10,101,110,100, 10,102,117,
  110, 99,116,105,111,110, 32, 84,101,114,114, 97,105,110, 58,
  114,117,108,101, 95,110, 97,109,101, 40, 41, 10,114,101,116,
  117,114,110, 32,109,101,116,104,111,100,115, 95,116,101,114,
  114, 97,105,110, 95,114,117,108,101, 95,110, 97,109,101, 40,
  115,101,108,102, 41, 10,101,110,100, 10,102,117,110, 99,116,
  105,111,110, 32, 84,101,114,114, 97,105,110, 58,110, 97,109,
  101, 95,116,114, 97,110,115,108, 97,116,105,111,110, 40, 41,
   10,114,101,116,117,114,110, 32,109,101,116,104,111,100,115,
   95,116,101,114,114, 97,105,110, 95,110, 97,109,101, 95,116,
  114, 97,110,115,108, 97,116,105,111,110, 40,115,101,108,102,
   41, 10,101,110,100, 10,102,117,110, 99,116,105,111,110, 32,
   67,105,116,121, 58,104, 97,115, 95, 98,117,105,108,100,105,
  110,103, 40, 98,117,105,108,100,105,110,103, 41, 10,114,101,
  116,117,114,110, 32,109,101,116,104,111,100,115, 95, 99,105,
  116,121, 95,104, 97,115, 95, 98,117,105,108,100,105,110,103,
   40,115,101,108,102, 44, 32, 98,117,105,108,100,105,110,103,
   41, 10,101,110,100,32
  };
  tolua_dobuffer(tolua_S,(char*)B,sizeof(B),"tolua: embedded Lua code 1");
  lua_settop(tolua_S, top);
 } /* end of embedded lua code */

 tolua_module(tolua_S,"find",0);
 tolua_beginmodule(tolua_S,"find");
  tolua_function(tolua_S,"player",tolua_api_find_player00);
  tolua_function(tolua_S,"city",tolua_api_find_city00);
  tolua_function(tolua_S,"unit",tolua_api_find_unit00);
  tolua_function(tolua_S,"tile",tolua_api_find_tile00);
  tolua_function(tolua_S,"government",tolua_api_find_government00);
  tolua_function(tolua_S,"government",tolua_api_find_government01);
  tolua_function(tolua_S,"nation_type",tolua_api_find_nation_type00);
  tolua_function(tolua_S,"nation_type",tolua_api_find_nation_type01);
  tolua_function(tolua_S,"building_type",tolua_api_find_building_type00);
  tolua_function(tolua_S,"building_type",tolua_api_find_building_type01);
  tolua_function(tolua_S,"unit_type",tolua_api_find_unit_type00);
  tolua_function(tolua_S,"unit_type",tolua_api_find_unit_type01);
  tolua_function(tolua_S,"role_unit_type",tolua_api_find_role_unit_type00);
  tolua_function(tolua_S,"tech_type",tolua_api_find_tech_type00);
  tolua_function(tolua_S,"tech_type",tolua_api_find_tech_type01);
  tolua_function(tolua_S,"terrain",tolua_api_find_terrain00);
  tolua_function(tolua_S,"terrain",tolua_api_find_terrain01);
 tolua_endmodule(tolua_S);

 { /* begin embedded lua code */
  int top = lua_gettop(tolua_S);
  static unsigned char B[] = {
  10,102,117,110, 99,116,105,111,110, 32, 95,102,114,101,101,
   99,105,118, 95,115,116, 97,116,101, 95,100,117,109,112, 40,
   41, 10,108,111, 99, 97,108, 32,114,101,115, 32, 61, 32, 39,
   39, 10,102,111,114, 32,107, 44, 32,118, 32,105,110, 32,112,
   97,105,114,115, 40, 95, 71, 41, 32,100,111, 10,105,102, 32,
  107, 32, 61, 61, 32, 39, 95, 86, 69, 82, 83, 73, 79, 78, 39,
   32,116,104,101,110, 10,101,108,115,101,105,102, 32,116,121,
  112,101, 40,118, 41, 32, 61, 61, 32, 39, 98,111,111,108,101,
   97,110, 39, 10,111,114, 32,116,121,112,101, 40,118, 41, 32,
   61, 61, 32, 39,110,117,109, 98,101,114, 39, 32,116,104,101,
  110, 10,108,111, 99, 97,108, 32,114,118, 97,108,117,101, 32,
   61, 32,116,111,115,116,114,105,110,103, 40,118, 41, 10,114,
  101,115, 32, 61, 32,114,101,115, 32, 46, 46, 32,107, 32, 46,
   46, 32, 39, 61, 39, 32, 46, 46, 32,114,118, 97,108,117,101,
   32, 46, 46, 32, 39, 92,110, 39, 10,101,108,115,101,105,102,
   32,116,121,112,101, 40,118, 41, 32, 61, 61, 32, 39,115,116,
  114,105,110,103, 39, 32,116,104,101,110, 10,108,111, 99, 97,
  108, 32,114,118, 97,108,117,101, 32, 61, 32,115,116,114,105,
  110,103, 46,102,111,114,109, 97,116, 40, 39, 37,113, 39, 44,
   32,118, 41, 10,114,101,115, 32, 61, 32,114,101,115, 32, 46,
   46, 32,107, 32, 46, 46, 32, 39, 61, 39, 32, 46, 46, 32,114,
  118, 97,108,117,101, 32, 46, 46, 32, 39, 92,110, 39, 10,101,
  108,115,101,105,102, 32,116,121,112,101, 40,118, 41, 32, 61,
   61, 32, 39,117,115,101,114,100, 97,116, 97, 39, 32,116,104,
  101,110, 10,108,111, 99, 97,108, 32,109,101,116,104,111,100,
   32, 61, 32,115,116,114,105,110,103, 46,108,111,119,101,114,
   40,116,111,108,117, 97, 46,116,121,112,101, 40,118, 41, 41,
   10,114,101,115, 32, 61, 32,114,101,115, 32, 46, 46, 32,107,
   32, 46, 46, 32, 39, 61,102,105,110,100, 46, 39, 32, 46, 46,
   32,109,101,116,104,111,100, 10,105,102, 32,109,101,116,104,
  111,100, 32, 61, 61, 32, 39, 99,105,116,121, 39, 32,111,114,
   32,109,101,116,104,111,100, 32, 61, 61, 32, 39,117,110,105,
  116, 39, 32,116,104,101,110, 10,114,101,115, 32, 61, 32,114,
  101,115, 32, 46, 46, 32, 39, 40,110,105,108, 44, 39, 32, 46,
   46, 32,118, 46,105,100, 32, 46, 46, 32, 39, 41, 39, 10,101,
  108,115,101, 10,114,101,115, 32, 61, 32,114,101,115, 32, 46,
   46, 32, 39, 40, 39, 32, 46, 46, 32,118, 46,105,100, 32, 46,
   46, 32, 39, 41, 39, 10,101,110,100, 10,114,101,115, 32, 61,
   32,114,101,115, 32, 46, 46, 32, 39, 92,110, 39, 10,101,110,
  100, 10,101,110,100, 10,114,101,116,117,114,110, 32,114,101,
  115, 10,101,110,100,32
  };
  tolua_dobuffer(tolua_S,(char*)B,sizeof(B),"tolua: embedded Lua code 2");
  lua_settop(tolua_S, top);
 } /* end of embedded lua code */

 tolua_module(tolua_S,"signal",0);
 tolua_beginmodule(tolua_S,"signal");
  tolua_function(tolua_S,"connect",tolua_api_signal_connect00);
 tolua_endmodule(tolua_S);
 tolua_function(tolua_S,"_",tolua_api__00);
 tolua_function(tolua_S,"N_",tolua_api_N_00);
 tolua_function(tolua_S,"Q_",tolua_api_Q_00);
 tolua_function(tolua_S,"PL_",tolua_api_PL_00);
 tolua_module(tolua_S,"notify",0);
 tolua_beginmodule(tolua_S,"notify");
  tolua_function(tolua_S,"embassies_msg",tolua_api_notify_embassies_msg00);
  tolua_function(tolua_S,"event_msg",tolua_api_notify_event_msg00);
 tolua_endmodule(tolua_S);
 tolua_module(tolua_S,"E",0);
 tolua_beginmodule(tolua_S,"E");
  tolua_constant(tolua_S,"CITY_CANTBUILD",E_CITY_CANTBUILD);
  tolua_constant(tolua_S,"CITY_LOST",E_CITY_LOST);
  tolua_constant(tolua_S,"CITY_LOVE",E_CITY_LOVE);
  tolua_constant(tolua_S,"CITY_DISORDER",E_CITY_DISORDER);
  tolua_constant(tolua_S,"CITY_FAMINE",E_CITY_FAMINE);
  tolua_constant(tolua_S,"CITY_FAMINE_FEARED",E_CITY_FAMINE_FEARED);
  tolua_constant(tolua_S,"CITY_GROWTH",E_CITY_GROWTH);
  tolua_constant(tolua_S,"CITY_MAY_SOON_GROW",E_CITY_MAY_SOON_GROW);
  tolua_constant(tolua_S,"CITY_AQUEDUCT",E_CITY_AQUEDUCT);
  tolua_constant(tolua_S,"CITY_AQ_BUILDING",E_CITY_AQ_BUILDING);
  tolua_constant(tolua_S,"CITY_NORMAL",E_CITY_NORMAL);
  tolua_constant(tolua_S,"CITY_NUKED",E_CITY_NUKED);
  tolua_constant(tolua_S,"CITY_CMA_RELEASE",E_CITY_CMA_RELEASE);
  tolua_constant(tolua_S,"CITY_GRAN_THROTTLE",E_CITY_GRAN_THROTTLE);
  tolua_constant(tolua_S,"CITY_TRANSFER",E_CITY_TRANSFER);
  tolua_constant(tolua_S,"CITY_BUILD",E_CITY_BUILD);
  tolua_constant(tolua_S,"CITY_PRODUCTION_CHANGED",E_CITY_PRODUCTION_CHANGED);
  tolua_constant(tolua_S,"CITY_PLAGUE",E_CITY_PLAGUE);
  tolua_constant(tolua_S,"WORKLIST",E_WORKLIST);
  tolua_constant(tolua_S,"UPRISING",E_UPRISING);
  tolua_constant(tolua_S,"CIVIL_WAR",E_CIVIL_WAR);
  tolua_constant(tolua_S,"ANARCHY",E_ANARCHY);
  tolua_constant(tolua_S,"FIRST_CONTACT",E_FIRST_CONTACT);
  tolua_constant(tolua_S,"NEW_GOVERNMENT",E_NEW_GOVERNMENT);
  tolua_constant(tolua_S,"LOW_ON_FUNDS",E_LOW_ON_FUNDS);
  tolua_constant(tolua_S,"POLLUTION",E_POLLUTION);
  tolua_constant(tolua_S,"REVOLT_DONE",E_REVOLT_DONE);
  tolua_constant(tolua_S,"REVOLT_START",E_REVOLT_START);
  tolua_constant(tolua_S,"SPACESHIP",E_SPACESHIP);
  tolua_constant(tolua_S,"MY_DIPLOMAT_BRIBE",E_MY_DIPLOMAT_BRIBE);
  tolua_constant(tolua_S,"DIPLOMATIC_INCIDENT",E_DIPLOMATIC_INCIDENT);
  tolua_constant(tolua_S,"MY_DIPLOMAT_ESCAPE",E_MY_DIPLOMAT_ESCAPE);
  tolua_constant(tolua_S,"MY_DIPLOMAT_EMBASSY",E_MY_DIPLOMAT_EMBASSY);
  tolua_constant(tolua_S,"MY_DIPLOMAT_FAILED",E_MY_DIPLOMAT_FAILED);
  tolua_constant(tolua_S,"MY_DIPLOMAT_INCITE",E_MY_DIPLOMAT_INCITE);
  tolua_constant(tolua_S,"MY_DIPLOMAT_POISON",E_MY_DIPLOMAT_POISON);
  tolua_constant(tolua_S,"MY_DIPLOMAT_SABOTAGE",E_MY_DIPLOMAT_SABOTAGE);
  tolua_constant(tolua_S,"MY_DIPLOMAT_THEFT",E_MY_DIPLOMAT_THEFT);
  tolua_constant(tolua_S,"ENEMY_DIPLOMAT_BRIBE",E_ENEMY_DIPLOMAT_BRIBE);
  tolua_constant(tolua_S,"ENEMY_DIPLOMAT_EMBASSY",E_ENEMY_DIPLOMAT_EMBASSY);
  tolua_constant(tolua_S,"ENEMY_DIPLOMAT_FAILED",E_ENEMY_DIPLOMAT_FAILED);
  tolua_constant(tolua_S,"ENEMY_DIPLOMAT_INCITE",E_ENEMY_DIPLOMAT_INCITE);
  tolua_constant(tolua_S,"ENEMY_DIPLOMAT_POISON",E_ENEMY_DIPLOMAT_POISON);
  tolua_constant(tolua_S,"ENEMY_DIPLOMAT_SABOTAGE",E_ENEMY_DIPLOMAT_SABOTAGE);
  tolua_constant(tolua_S,"ENEMY_DIPLOMAT_THEFT",E_ENEMY_DIPLOMAT_THEFT);
  tolua_constant(tolua_S,"CARAVAN_ACTION",E_CARAVAN_ACTION);
  tolua_constant(tolua_S,"SCRIPT",E_SCRIPT);
  tolua_constant(tolua_S,"BROADCAST_REPORT",E_BROADCAST_REPORT);
  tolua_constant(tolua_S,"GAME_END",E_GAME_END);
  tolua_constant(tolua_S,"GAME_START",E_GAME_START);
  tolua_constant(tolua_S,"E_LOG_ERROR",E_LOG_ERROR);
  tolua_constant(tolua_S,"MESSAGE_WALL",E_MESSAGE_WALL);
  tolua_constant(tolua_S,"NATION_SELECTED",E_NATION_SELECTED);
  tolua_constant(tolua_S,"DESTROYED",E_DESTROYED);
  tolua_constant(tolua_S,"REPORT",E_REPORT);
  tolua_constant(tolua_S,"TURN_BELL",E_TURN_BELL);
  tolua_constant(tolua_S,"NEXT_YEAR",E_NEXT_YEAR);
  tolua_constant(tolua_S,"GLOBAL_ECO",E_GLOBAL_ECO);
  tolua_constant(tolua_S,"NUKE",E_NUKE);
  tolua_constant(tolua_S,"HUT_BARB",E_HUT_BARB);
  tolua_constant(tolua_S,"HUT_CITY",E_HUT_CITY);
  tolua_constant(tolua_S,"HUT_GOLD",E_HUT_GOLD);
  tolua_constant(tolua_S,"HUT_BARB_KILLED",E_HUT_BARB_KILLED);
  tolua_constant(tolua_S,"HUT_MERC",E_HUT_MERC);
  tolua_constant(tolua_S,"HUT_SETTLER",E_HUT_SETTLER);
  tolua_constant(tolua_S,"HUT_TECH",E_HUT_TECH);
  tolua_constant(tolua_S,"HUT_BARB_CITY_NEAR",E_HUT_BARB_CITY_NEAR);
  tolua_constant(tolua_S,"IMP_BUY",E_IMP_BUY);
  tolua_constant(tolua_S,"IMP_BUILD",E_IMP_BUILD);
  tolua_constant(tolua_S,"IMP_AUCTIONED",E_IMP_AUCTIONED);
  tolua_constant(tolua_S,"IMP_AUTO",E_IMP_AUTO);
  tolua_constant(tolua_S,"IMP_SOLD",E_IMP_SOLD);
  tolua_constant(tolua_S,"TECH_GAIN",E_TECH_GAIN);
  tolua_constant(tolua_S,"TECH_LEARNED",E_TECH_LEARNED);
  tolua_constant(tolua_S,"TREATY_ALLIANCE",E_TREATY_ALLIANCE);
  tolua_constant(tolua_S,"TREATY_BROKEN",E_TREATY_BROKEN);
  tolua_constant(tolua_S,"TREATY_CEASEFIRE",E_TREATY_CEASEFIRE);
  tolua_constant(tolua_S,"TREATY_PEACE",E_TREATY_PEACE);
  tolua_constant(tolua_S,"TREATY_SHARED_VISION",E_TREATY_SHARED_VISION);
  tolua_constant(tolua_S,"UNIT_LOST_ATT",E_UNIT_LOST_ATT);
  tolua_constant(tolua_S,"UNIT_WIN_ATT",E_UNIT_WIN_ATT);
  tolua_constant(tolua_S,"UNIT_BUY",E_UNIT_BUY);
  tolua_constant(tolua_S,"UNIT_BUILT",E_UNIT_BUILT);
  tolua_constant(tolua_S,"UNIT_LOST_DEF",E_UNIT_LOST_DEF);
  tolua_constant(tolua_S,"UNIT_WIN",E_UNIT_WIN);
  tolua_constant(tolua_S,"UNIT_LOST_MISC",E_UNIT_LOST_MISC);
  tolua_constant(tolua_S,"UNIT_BECAME_VET",E_UNIT_BECAME_VET);
  tolua_constant(tolua_S,"UNIT_UPGRADED",E_UNIT_UPGRADED);
  tolua_constant(tolua_S,"UNIT_RELOCATED",E_UNIT_RELOCATED);
  tolua_constant(tolua_S,"UNIT_ORDERS",E_UNIT_ORDERS);
  tolua_constant(tolua_S,"WONDER_BUILD",E_WONDER_BUILD);
  tolua_constant(tolua_S,"WONDER_OBSOLETE",E_WONDER_OBSOLETE);
  tolua_constant(tolua_S,"WONDER_STARTED",E_WONDER_STARTED);
  tolua_constant(tolua_S,"WONDER_STOPPED",E_WONDER_STOPPED);
  tolua_constant(tolua_S,"WONDER_WILL_BE_BUILT",E_WONDER_WILL_BE_BUILT);
  tolua_constant(tolua_S,"DIPLOMACY",E_DIPLOMACY);
  tolua_constant(tolua_S,"TREATY_EMBASSY",E_TREATY_EMBASSY);
  tolua_constant(tolua_S,"BAD_COMMAND",E_BAD_COMMAND);
  tolua_constant(tolua_S,"SETTING",E_SETTING);
  tolua_constant(tolua_S,"CHAT_MSG",E_CHAT_MSG);
  tolua_constant(tolua_S,"CHAT_ERROR",E_CHAT_ERROR);
  tolua_constant(tolua_S,"CONNECTION",E_CONNECTION);
  tolua_constant(tolua_S,"AI_DEBUG",E_AI_DEBUG);
  tolua_constant(tolua_S,"LOG_FATAL",E_LOG_FATAL);
  tolua_constant(tolua_S,"TECH_GOAL",E_TECH_GOAL);
  tolua_constant(tolua_S,"LAST",E_LAST);
 tolua_endmodule(tolua_S);

 { /* begin embedded lua code */
  int top = lua_gettop(tolua_S);
  static unsigned char B[] = {
  10,102,117,110, 99,116,105,111,110, 32,110,111,116,105,102,
  121, 46, 97,108,108, 40, 46, 46, 46, 41, 10,110,111,116,105,
  102,121, 46,101,118,101,110,116, 95,109,115,103, 40,110,105,
  108, 44, 32,110,105,108, 44, 32, 69, 46, 83, 67, 82, 73, 80,
   84, 44, 32,115,116,114,105,110,103, 46,102,111,114,109, 97,
  116, 40,117,110,112, 97, 99,107, 40, 97,114,103, 41, 41, 41,
   10,101,110,100, 10,102,117,110, 99,116,105,111,110, 32,110,
  111,116,105,102,121, 46,112,108, 97,121,101,114, 40,112,108,
   97,121,101,114, 44, 32, 46, 46, 46, 41, 10,110,111,116,105,
  102,121, 46,101,118,101,110,116, 95,109,115,103, 40,112,108,
   97,121,101,114, 44, 32,110,105,108, 44, 32, 69, 46, 83, 67,
   82, 73, 80, 84, 44, 32,115,116,114,105,110,103, 46,102,111,
  114,109, 97,116, 40,117,110,112, 97, 99,107, 40, 97,114,103,
   41, 41, 41, 10,101,110,100, 10,102,117,110, 99,116,105,111,
  110, 32,110,111,116,105,102,121, 46,101,118,101,110,116, 40,
  112,108, 97,121,101,114, 44, 32,116,105,108,101, 44, 32,101,
  118,101,110,116, 44, 32, 46, 46, 46, 41, 10,110,111,116,105,
  102,121, 46,101,118,101,110,116, 95,109,115,103, 40,112,108,
   97,121,101,114, 44, 32,116,105,108,101, 44, 32,101,118,101,
  110,116, 44, 32,115,116,114,105,110,103, 46,102,111,114,109,
   97,116, 40,117,110,112, 97, 99,107, 40, 97,114,103, 41, 41,
   41, 10,101,110,100, 10,102,117,110, 99,116,105,111,110, 32,
  110,111,116,105,102,121, 46,101,109, 98, 97,115,115,105,101,
  115, 40,112,108, 97,121,101,114, 44, 32,112,116,105,108,101,
   44, 32,101,118,101,110,116, 44, 32, 46, 46, 46, 41, 10,110,
  111,116,105,102,121, 46,101,109, 98, 97,115,115,105,101,115,
   95,109,115,103, 40,112,108, 97,121,101,114, 44, 32,112,116,
  105,108,101, 44, 32,101,118,101,110,116, 44, 32,115,116,114,
  105,110,103, 46,102,111,114,109, 97,116, 40,117,110,112, 97,
   99,107, 40, 97,114,103, 41, 41, 41, 10,101,110,100,32
  };
  tolua_dobuffer(tolua_S,(char*)B,sizeof(B),"tolua: embedded Lua code 3");
  lua_settop(tolua_S, top);
 } /* end of embedded lua code */

 tolua_function(tolua_S,"random",tolua_api_random00);
 tolua_function(tolua_S,"error_log",tolua_api_error_log00);
 tolua_function(tolua_S,"debug_log",tolua_api_debug_log00);
 tolua_function(tolua_S,"create_unit",tolua_api_create_unit00);
 tolua_function(tolua_S,"create_city",tolua_api_create_city00);
 tolua_function(tolua_S,"create_base",tolua_api_create_base00);
 tolua_function(tolua_S,"change_gold",tolua_api_change_gold00);
 tolua_function(tolua_S,"give_technology",tolua_api_give_technology00);
 tolua_function(tolua_S,"unleash_barbarians",tolua_api_unleash_barbarians00);
 tolua_endmodule(tolua_S);
 return 1;
}


/* This is never used in Freeciv.
 * Fix toluaxx to provide prototype before restoring this.
 * Note that this comes all the way from lua bindings of toluaxx
 * itself. */
/*
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_api (lua_State* tolua_S) {
 return tolua_api_open(tolua_S);
};
#endif
*/


