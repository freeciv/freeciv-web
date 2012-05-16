/* tolua
** Support code for Lua bindings.
** Written by Phoenix IV
** RareSky/Community
** Sep 2006
** $Id: parsecmd.hxx,v 1.2 2007/04/07 09:52:56 phoenix11 Exp $
*/

/* This code is free software; you can redistribute it and/or modify it.
** The software provided hereunder is on an "as is" basis, and
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications.
*/

#pragma once
#include"platform.hxx"
#include<map>

class PARSECMD{
 public:
  typedef vector<string> CORT;
  typedef map<string,CORT> POOL;
  typedef map<string,CORT>::iterator ITER;
 protected:
  POOL arg;
  CORT&exec;
  CORT&line;
  CORT unrec;
  ITER iter;
 public:
  
  void parse(int argc,char **argv); // Parse cmd line
  void cast(string name, string argum, bool value=false); // cast arg
  void final(); // finalize casting
  bool exist(string name); // arg existing
  string cnstr();
  string next();
  bool begin();
  bool end();
  
  PARSECMD(int argc,char **argv):exec(arg["exec"]),line(arg["line"]){parse(argc,argv);}

  inline void operator()(int argc,char **argv){parse(argc,argv);}
  inline bool operator()(string name){return exist(name);}
  inline void operator()(string name, string argum, bool value=false)
  {return cast(name,argum,value);}
  inline CORT&operator[](string name){return arg[name];} // arg value
  inline CORT&operator*(){return unrec;} // unrecognized options
  inline void operator--(){begin();}
  inline string operator++(){return next();}
  inline bool operator!(){return !end();}
  inline void operator ()(){final();} // finalize
};

