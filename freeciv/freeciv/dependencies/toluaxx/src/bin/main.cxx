/* tolua
** Support code for Lua bindings.
** Written by Phoenix IV
** RareSky/Community
** Sep 2006
** $Id: main.cxx 14589 2008-04-15 21:40:53Z cazfi $
*/

/* This code is free software; you can redistribute it and/or modify it.
** The software provided hereunder is on an "as is" basis, and
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications.
*/

#include"main.hxx"

void setfield(lua_State* L,int t,string f,string v){
  lua_pushstring(L,f.data());
  lua_pushstring(L,v.data());
  lua_settable(L,t);
}

void add_extra(lua_State* L,string v){
  lua_getglobal(L,"_extra_parameters");
  int l=luaL_getn(L,-1);
  lua_pushstring(L,v.data());
  lua_rawseti(L,-2,l+1);
  lua_pop(L,1);
};

int main(int argc, char* argv[]){

  PARSECMD c(argc,argv);
  c("version",     "-v|--version"         );
  c("output",      "-o|--output",     true);
  c("header",      "-H|--header",     true);
  c("pkgname",     "-n|--pkgname",    true);
  c("parseonly",   "-p|--parseonly"       );
  c("parseinfo",   "-P|--parseinfo"       );
  c("nostdtring",  "-S|--nostdstring"     );
  c("subst1index", "-1|--subst1index"     );
  c("luafile",     "-L|--luafile",    true);
  c("noautodest",  "-D|--noautodest"      );
  c("nowarnings",  "-W|--nowarnings"      );
  c("nocleanup",   "-C|--nocleanup"       );
  c("extraval",    "-E|--extra",      true);
  c("typeidlist",  "-t|--typeidlist"      );
  c("help",        "-h|--help"            );
  c(                                      );
  
  //cout<<c.cnstr();
  
  for(unsigned int i=0;i<(*c).size();i++)cout<<"Warning: unrecognized option `"<<(*c)[i]<<"`.."<<endl;

  if(c("version")){cout<<version();return 0;}
  if(c("help")){cout<<help();return 0;}
  
  for(unsigned int i=0;i<c["line"].size();c["input"].push_back(c["line"][i]),i++);
  
  if(!c("input")){cout<<usage();return 0;}
  
  {
#  if LUA_VERSION_NUM >= 501 /* lua 5.1 */
    lua_State* L=luaL_newstate();
    luaL_openlibs(L);
#  else
    lua_State* L=lua_open();
    luaopen_base(L);
    luaopen_io(L);
    luaopen_string(L);
    luaopen_table(L);
    luaopen_math(L);
    luaopen_debug(L);
#  endif
    
    lua_pushstring(L,TOLUA_VERSION); lua_setglobal(L,"TOLUA_VERSION");
    lua_pushstring(L,LUA_VERSION); lua_setglobal(L,"TOLUA_LUA_VERSION");
    {
      lua_newtable(L);
      lua_setglobal(L,"_extra_parameters");
      lua_newtable(L);
      lua_pushvalue(L,-1);
      lua_setglobal(L,"flags");
      int t=lua_gettop(L);
      {
	//for(--c;!c;string n=++c){
	//  if(n=="exec"||n="line")continue;
	//  if(c[n]);
	//}
	
	if(c("parseonly"  )) setfield(L,t,"p","");
	if(c("parseinfo"  )) setfield(L,t,"P","");
	if(c("output"     )){
	  if(c["output"].size()>0)setfield(L,t,"o",c["output"][0]);
	  else return 1;
	}
	if(c("pkgname"    )){
	  if(c["pkgname"].size()>0)setfield(L,t,"n",c["pkgname"][0]);
	  else return 1;
	}
	if(c("header"     )){
	  if(c["header"].size()>0)setfield(L,t,"H",c["header"][0]);
	  else return 1;
	}
	if(c("nostdstring")) setfield(L,t,"S","");
	if(c("subst1index")) setfield(L,t,"1","");
	if(c("luafile"    )){
	  if(c["luafile"].size()>0)setfield(L,t,"L",c["luafile"][0]);
	  else return 1;
	}
	if(c("noautodest" )) setfield(L,t,"D","");
	if(c("nowarnings" )) setfield(L,t,"W","");
	if(c("nocleanup"  )) setfield(L,t,"C","");
	if(c("typeidlist" )) setfield(L,t,"t","");
	if(c("input"      )){
	  if(c["input"].size()>0)setfield(L,t,"f",c["input"][0]);
	  else return 1;
	}
      }
      lua_pop(L,1);
      if(c("extraval"))for(unsigned int i=0;i<c["extraval"].size();i++)add_extra(L,c["extraval"][i]);
    }
#  ifndef DEBUG_MODE
    tolua_toluaxx_open(L);
#  else
    {
      string path=c["exec"][1]+"/../src/bin/lua/?.lua";
      string doit="package.path=package.path..\";"+path+"\" require\"tlx_all\"";
      luaL_loadstring(L,doit.data());
      lua_call(L,0,0);
    }
#  endif
  }
  return 0;
}
