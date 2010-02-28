/* tolua
** Support code for Lua bindings.
** Written by Phoenix IV
** RareSky/Community
** Sep 2006
** $Id: main.hxx,v 1.1.1.2 2006/10/25 10:55:38 phoenix11 Exp $
*/

/* This code is free software; you can redistribute it and/or modify it.
** The software provided hereunder is on an "as is" basis, and
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications.
*/

#pragma once
#include"toluaxx.h"
#include"lua.hpp"

TOLUA_API int tolua_toluaxx_open (lua_State* L);

#include"parsecmd.hxx"
#include"help.hxx"
