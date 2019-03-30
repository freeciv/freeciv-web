-- tolua: basic utility functions
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- Last update: Apr 2003
-- $Id: basic.lua,v 1.4 2009/11/24 16:45:13 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Basic C types and their corresponding Lua types
-- All occurrences of "char*" will be replaced by "_cstring",
-- and all occurrences of "void*" will be replaced by "_userdata" 
_basic = {
 ['void'] = '',
 ['char'] = 'number',
 ['tolua_index'] = 'number',
 ['tolua_len'] = 'number',
 ['tolua_byte'] = 'number',
 ['tolua_ubyte'] = 'number',
 ['tolua_multret'] = 'number',
 ['int'] = 'number',
 ['short'] = 'number',
 ['long'] = 'number',
 ['unsigned'] = 'number',
 ['float'] = 'number',
 ['double'] = 'number',
 ['_cstring'] = 'string',
 ['_userdata'] = 'userdata',
 ['char*'] = 'string',
 ['void*'] = 'userdata',
 ['bool'] = 'boolean',
 ['lua_State*'] = 'state',
 ['_lstate'] = 'state',
 ['lua_Object'] = 'value',
 ['lua_Function'] = 'function',
 ['LUA_VALUE'] = 'value',    -- for compatibility with tolua 4.0
}

_basic_ctype = {
 number = "lua_Number",
 string = "const char*",
 userdata = "void*",
 boolean = "bool",
 value = "int",
}

-- List of user defined types
-- Each type corresponds to a variable name that stores its tag value.
_usertype = {}

-- List of types that have to be collected
_collect = {}


-- List of auto renaming
_renaming = {}
function appendrenaming (s)
 local b,e,old,new = strfind(s,"%s*(.-)%s*@%s*(.-)%s*$")
	if not b then
	 error("#Invalid renaming syntax; it should be of the form: pattern@pattern")
	end
	tinsert(_renaming,{old=old, new=new})
end

function applyrenaming (s)
	for i=1,getn(_renaming) do
	 local m,n = gsub(s,_renaming[i].old,_renaming[i].new)
		if n ~= 0 then
		 return m
		end
	end
	return nil
end

-- Error handler
function tolua_error (s,f)
 local out = _OUTPUT
 _OUTPUT = _STDERR
 if strsub(s,1,1) == '#' then
  write("\n** tolua: "..strsub(s,2)..".\n\n")
  if _curr_code then
   local _,_,s = strfind(_curr_code,"^%s*(.-\n)") -- extract first line
   if s==nil then s = _curr_code end
   s = gsub(s,"_userdata","void*") -- return with 'void*'
   s = gsub(s,"_cstring","char*")  -- return with 'char*'
   s = gsub(s,"_lstate","lua_State*")  -- return with 'lua_State*'
   write("Code being processed:\n"..s.."\n")
  end
 else
  print(debug.traceback("\n** tolua internal error: "..f..s..".\n\n"))
  return
 end
 _OUTPUT = out
end

function warning (msg)
 local out = _OUTPUT
 _OUTPUT = _STDERR
 write("\n** tolua warning: "..msg..".\n\n")
 _OUTPUT = out
end

-- register an user defined type: returns full type
function regtype (t)
 local ft = findtype(t)
	if isbasic(t) then
	 return t
	end
 if not ft then
		return appendusertype(t)
 end
end

-- return type name: returns full type
function typevar(type)
 if type == '' or type == 'void' or type == "..." then
  return type
 else
		local ft = findtype(type)
  if ft then
   return ft
  end
		_usertype[type] = type
		return type
	end
end

-- check if basic type
function isbasic (type)
 local t = gsub(type,'const ','')
 local m,t = applytypedef(t)
 local b = _basic[t]
 if b then
  return b,_basic_ctype[b]
 end
 return nil
end

-- split string using a token
function split (s,t)
 local l = {n=0}
 local f = function (s)
  l.n = l.n + 1
  l[l.n] = s
  return ""
 end
 local p = "%s*(.-)%s*"..t.."%s*"
 s = gsub(s,"^%s+","")
 s = gsub(s,"%s+$","")
 s = gsub(s,p,f)
 l.n = l.n + 1
 l[l.n] = gsub(s,"(%s%s*)$","")
 return l
end


-- concatenate strings of a table
function concat (t,f,l)
 local s = ''
 local i=f
 while i<=l do
  s = s..t[i]
  i = i+1
  if i <= l then s = s..' ' end
 end
 return s
end

-- concatenate all parameters, following output rules
function concatparam (line, ...)
 local arg = {...}
 local i=1
 while i<=#arg do
  if _cont and not strfind(_cont,'[%(,"]') and 
     strfind(arg[i],"^[%a_~]") then 
	    line = line .. ' ' 
  end
  line = line .. arg[i]
  if arg[i] ~= '' then
   _cont = strsub(arg[i],-1,-1)
  end
  i = i+1
 end
 if strfind(arg[#arg],"[%/%)%;%{%}]$") then 
  _cont=nil line = line .. '\n'
 end
	return line
end

-- output line
function output (...)
 local arg = {...}
 local i=1
 while i<=#arg do
  if _cont and not strfind(_cont,'[%(,"]') and 
     strfind(arg[i],"^[%a_~]") then 
	    write(' ') 
  end
  write(arg[i])
  if arg[i] ~= '' then
   _cont = strsub(arg[i],-1,-1)
  end
  i = i+1
 end
 if strfind(arg[#arg],"[%/%)%;%{%}]$") then 
  _cont=nil write('\n')
 end
end


