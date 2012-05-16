/* tolua
** Support code for Lua bindings.
** Written by Phoenix IV
** RareSky/Community
** Sep 2006
** $Id: help.cxx,v 1.1.1.2 2006/10/25 10:55:39 phoenix11 Exp $
*/

/* This code is free software; you can redistribute it and/or modify it.
** The software provided hereunder is on an "as is" basis, and
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications.
*/

#include"help.hxx"
#include"toluaxx.h"

string usage(string locale){
  return string(
		TOLUA_VERSION
		": no input files\n"
		"use -h|--help for more information....\n\n"
		);
}
string version(string locale){
  return string(
		TOLUA_VERSION
		"(written by W. Celes, A. Manzur, Phoenix IV)\n\n"
		);
}
string help(string locale){
  return string(
		"usage: tolua++ [options] input_file\n\n"
		"Command line options are:\n"
		"  -v|--version             : print version information.\n"
		"  -o|--output   file       : set output file; default is stdout.\n"
		"  -H|--header   file       : create include file.\n"
		"  -n|--pkgname  name       : set package name; default is input file root name.\n"
		"  -p|--parseonly           : parse only.\n"
		"  -P|--parseinfo           : parse and print structure information (for debug).\n"
		"  -S|--nostdstring         : disable support for c++ strings.\n"
		"  -1|--subst1index         : substract 1 to operator[] index (for compatibility with tolua5).\n"
		"  -L|--luafile  file       : run lua file (with dofile()) before doing anything.\n"
		"  -D|--noautodest          : disable automatic exporting of destructors for classes that\n"
		"                             have constructors (for compatibility with tolua5)\n"
		"  -W|--nowarnings          : disable warnings for unsupported features\n"
		"                             (for compatibility with tolua5)\n"
		"  -C|--nocleanup           : disable cleanup of included lua code (for easier debugging)\n"
		"  -E|--extra value[=value] : add extra values to the luastate\n"
		"  -t|--typeidlist          : export a list of types asociates with the C++ typeid name\n"
		"  -h|--help                : print this message.\n\n"
		"Should the input file be omitted, stdin is assumed;\n"
		"in that case, the package name must be explicitly set.\n\n"
		);
}
