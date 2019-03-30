-- tolua: function class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: function.lua,v 1.4 2009/11/24 16:45:14 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications. 



-- Function class
-- Represents a function or a class method.
-- The following fields are stored:
--  mod  = type modifiers
--  type = type
--  ptr  = "*" or "&", if representing a pointer or a reference
--  name = name
--  lname = lua name
--  args  = list of argument declarations
--  const = if it is a method receiving a const "this".
classFunction = {
 mod = '',
 type = '',
 ptr = '',
 name = '',
 args = {n=0},
 const = '',
}
classFunction.__index = classFunction
setmetatable(classFunction,classFeature)

-- declare tags
function classFunction:decltype ()
 self.type = typevar(self.type)
 if strfind(self.mod,'const') then
	 self.type = 'const '..self.type
	 self.mod = gsub(self.mod,'const%s*','')
	end
 local i=1
 while self.args[i] do
  self.args[i]:decltype()
  i = i+1
 end
end


-- Write binding function
-- Outputs C/C++ binding function.
function classFunction:supcode ()
 local overload = strsub(self.cname,-2,-1) - 1  -- indicate overloaded func
 local nret = 0      -- number of returned values
 local class = self:inclass()
 local _,_,static = strfind(self.mod,'^%s*(static)')

 if class then
  output("/* method:",self.name," of class ",class," */")
 else
  output("/* function:",self.name," */")
 end
 output("static int",self.cname,"(lua_State* tolua_S)")
 output("{")
  if self.type == 'tolua_multret' then
   output('  int tolua_ret;')
  end

 -- check types
	if overload < 0 then
	 output('#ifndef TOLUA_RELEASE\n')
	end
	output(' tolua_Error tolua_err;')
 output(' if (\n')
 -- check self
 local narg
 if class then narg=2 else narg=1 end
 if class then
	 local func = 'tolua_isusertype'
		local type = self.parent.type
		if self.const ~= '' then
		 type = self.const .. " " .. type
		end
	 if self.name=='new' or static~=nil then
		 func = 'tolua_isusertable'
			type = self.parent.type
		end
		output('     !'..func..'(tolua_S,1,"'..type..'",0,&tolua_err) || \n') 
 end
 -- check args
 local vararg = false
 if self.args[1].type ~= 'void' then
  local i=1
  while self.args[i] and self.args[i].type ~= "..." do
		 local btype = isbasic(self.args[i].type) 
			if btype ~= 'state' then
    output('     !'..self.args[i]:outchecktype(narg,false)..' || \n')
   end
			if btype ~= 'state' then
        narg = narg+1
			end
   i = i+1
  end
  if self.args[i] then
   vararg = true
  end
 end
 -- check end of list 
 if not vararg then
   output('     !tolua_isnoobj(tolua_S,'..narg..',&tolua_err)\n')
 else
   output('     0\n')
 end
 output('    )')
 output('  goto tolua_lerror;')

 output(' else\n')
	if overload < 0 then
	 output('#endif\n')
	end
	output(' {')
 
 -- declare self, if the case
 local narg
 if class then narg=2 else narg=1 end
 if class and self.name~='new' and static==nil then
  output(' ',self.const,self.parent.type,'*','self = ')
  output('(',self.const,self.parent.type,'*) ')
  output('tolua_tousertype(tolua_S,1,0);')
 elseif static then
  _,_,self.mod = strfind(self.mod,'^%s*static%s%s*(.*)')
 end
 -- declare parameters
 if self.args[1].type ~= 'void' then
  local i=1
  while self.args[i] and self.args[i].type ~= "..." do
   self.args[i]:declare(narg)
			if isbasic(self.args[i].type) ~= "state" then
        narg = narg+1
			end
   i = i+1
  end
 end

 -- check self
 if class and self.name~='new' and static==nil then 
	 output('#ifndef TOLUA_RELEASE\n')
  output('  if (!self) tolua_error(tolua_S,"invalid \'self\' in function \''..self.name..'\'",NULL);');
	 output('#endif\n')
 end

 -- get array element values
 if class then narg=2 else narg=1 end
 if self.args[1].type ~= 'void' then
  local i=1
  while self.args[i] and self.args[i].type ~= "..." do
	 if isbasic(self.args[i].type) ~= "state" then
     self.args[i]:getarray(narg)
     narg = narg+1
   end
   i = i+1
  end
 end

 -- call function
 if class and self.name=='delete' then
  output('  tolua_release(tolua_S,self);')
  output('  delete self;')
 elseif class and self.name == 'operator&[]' then
  output('  self->operator[](',self.args[1].name,'-1) = ',self.args[2].name,';')
 else
  output('  {')
  if self.type ~= '' and self.type ~= 'void' then
   local ctype = self.type
   if ctype == 'value' or ctype == 'function' then
    ctype = 'int'
   end
   if self.type == 'tolua_multret' then
     output('  tolua_ret = ')
   else
     output('  ',self.mod,ctype,self.ptr,'tolua_ret = ')
   end
   if isbasic(self.type) or self.ptr ~= '' then
    output('(',self.mod,ctype,self.ptr,') ')
   end
  else
   output('  ')
  end
  if class and self.name=='new' then
   output('new',self.type,'(')
  elseif class and static then
   output(class..'::'..self.name,'(')
  elseif class then
   output('self->'..self.name,'(')
  else
   output(self.name,'(')
  end

  -- write parameters
  local i=1
  while self.args[i] and self.args[i].type ~= "..." do
   self.args[i]:passpar()
   i = i+1
   if self.args[i] and self.args[i].type ~= "..." then
    output(',')
   end
  end
     
  if class and self.name == 'operator[]' then
   output('-1);')
		else
   output(');')
		end

  -- return values
  if self.type ~= '' and self.type ~= 'void' and self.type ~= 'tolua_multret' then
   nret = nret + 1
   local t,ct = isbasic(self.type)
   if t then
     if t=='function' then t='value' end
     if self.type == 'tolua_index' then
      output('   if (tolua_ret < 0) lua_pushnil(tolua_S);')
      output('   else tolua_push'..t..'(tolua_S,(',ct,')tolua_ret+1);')
     else
      output('   tolua_push'..t..'(tolua_S,(',ct,')tolua_ret);')
     end
   else
		t = self.type
    if self.ptr == '' then
     output('   {')
     output('#ifdef __cplusplus\n')
     output('    void* tolua_obj = new',t,'(tolua_ret);') 
	    output('    tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,'.. (_collect[t] or 'NULL') ..'),"',t,'");')
     output('#else\n')
     output('    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(',t,'));')
	    output('    tolua_pushusertype(tolua_S,tolua_clone(tolua_S,tolua_obj,NULL),"',t,'");')
     output('#endif\n')
     output('   }')
    elseif self.ptr == '&' then
     output('   tolua_pushusertype(tolua_S,(void*)&tolua_ret,"',t,'");')
    else
     output('   tolua_pushusertype(tolua_S,(void*)tolua_ret,"',t,'");')
     if self.mod == 'tolua_own' then
       output('   lua_pushcfunction(tolua_S, tolua_bnd_takeownership);')
       output('   lua_pushvalue(tolua_S, -2);')
       output('   lua_call(tolua_S, 1, 0);')
     end
    end
   end
  end
  local i=1
  while self.args[i] do
   nret = nret + self.args[i]:retvalue()
   i = i+1
  end
  output('  }')

  -- set array element values
  if class then narg=2 else narg=1 end
  if self.args[1].type ~= 'void' then
   local i=1
   while self.args[i] do
     if isbasic(self.args[i].type) ~= "state" then
       self.args[i]:setarray(narg)
       narg = narg+1
     end
     i = i+1
   end
  end
 
  -- free dynamically allocated array
  if self.args[1].type ~= 'void' then
   local i=1
   while self.args[i] do
    self.args[i]:freearray()
    i = i+1
   end
  end
 end

 output(' }')
 if self.type == "tolua_multret" then
   output(' return '..nret..' + tolua_ret;')
 else
   output(' return '..nret..';')
 end

 -- call overloaded function or generate error
	if overload < 0 then
	 output('#ifndef TOLUA_RELEASE\n')
  output('tolua_lerror:\n')
  output(' tolua_error(tolua_S,"#ferror in function \''..self.lname..'\'.",&tolua_err);')
  output(' return 0;')
  output('#endif\n')
	else
  output('tolua_lerror:\n')
  output(' return '..strsub(self.cname,1,-3)..format("%02d",overload)..'(tolua_S);')
 end
 output('}')
 output('\n')
end


-- register function
function classFunction:register ()
 output(' tolua_function(tolua_S,"'..self.lname..'",'..self.cname..');')
end

-- Print method
function classFunction:print (ident,close)
 print(ident.."Function{")
 print(ident.." mod  = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." ptr  = '"..self.ptr.."',")
 print(ident.." name = '"..self.name.."',")
 print(ident.." lname = '"..self.lname.."',")
 print(ident.." const = '"..self.const.."',")
 print(ident.." cname = '"..self.cname.."',")
 print(ident.." lname = '"..self.lname.."',")
 print(ident.." args = {")
 local i=1
 while self.args[i] do
  self.args[i]:print(ident.."  ",",")
  i = i+1
 end
 print(ident.." }")
 print(ident.."}"..close)
end

-- check if it returns a object by value
function classFunction:requirecollection (t)
	local r = false
	if self.type ~= '' and not isbasic(self.type) and self.ptr=='' then
	 local type = gsub(self.type,"%s*const%s*","")
	 t[type] = "tolua_collect_" .. gsub(type,"::","_")
	 r = true
	end
	local i=1
	while self.args[i] do
		r = self.args[i]:requirecollection(t) or r
		i = i+1
	end
	return r
end

-- determine lua function name overload
function classFunction:overload ()
 return self.parent:overload(self.lname)
end



-- Internal constructor
function _Function (t)
 setmetatable(t,classFunction)

 if t.const ~= 'const' and t.const ~= '' then
  error("#invalid 'const' specification")
 end

 append(t)
 if t:inclass() then
  if t.name == t.parent.name then
   t.name = 'new'
   t.lname = 'new'
   if string.find(t.type,"tolua_own") then
     t.mod = "tolua_own"
   end
   t.type = t.parent.name
   t.ptr = '*'
  elseif t.name == '~'..t.parent.name then
   t.name = 'delete'
   t.lname = 'delete'
   t.parent._delete = true
  elseif t.type == 'tolua_len' then
   t.lname = ".len"
  end
 end
 t.cname = t:cfuncname("tolua")..t:overload(t)
 return t
end

-- Constructor
-- Expects three strings: one representing the function declaration,
-- another representing the argument list, and the third representing
-- the "const" or empty string.
function Function (d,a,c)
 local t = split(strsub(a,2,-2),',') -- eliminate braces
 local i=1
 local l = {n=0}
 while t[i] do
  l.n = l.n+1
  l[l.n] = Declaration(t[i],'var')
  i = i+1
 end
 local f = Declaration(d,'func')
 f.args = l
 f.const = c
 return _Function(f)
end


