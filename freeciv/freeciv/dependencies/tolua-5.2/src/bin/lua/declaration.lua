-- tolua: declaration class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: declaration.lua,v 1.5 2011/01/13 13:43:46 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications. 


-- Declaration class
-- Represents variable, function, or argument declaration.
-- Stores the following fields:
--  mod  = type modifiers
--  type = type
--  ptr  = "*" or "&", if representing a pointer or a reference
--  name = name
--  dim  = dimension, if a vector
--  def  = default value, if any (only for arguments)
--  ret  = "*" or "&", if value is to be returned (only for arguments)
classDeclaration = {
 mod = '',
 type = '',
 ptr = '',
 name = '',
 dim = '',
 ret = '',
 def = ''
}
classDeclaration.__index = classDeclaration
setmetatable(classDeclaration,classFeature)

-- Create an unique variable name
function create_varname ()
 if not _varnumber then _varnumber = 0 end
 _varnumber = _varnumber + 1
 return "tolua_var_".._varnumber
end

-- Check declaration name
-- It also identifies default values
function classDeclaration:checkname ()

 if strsub(self.name,1,1) == '[' and not findtype(self.type) then
  self.name = self.type..self.name
  local m = split(self.mod,'%s%s*')
  self.type = m[m.n]
  self.mod = concat(m,1,m.n-1)
 end

 local t = split(self.name,'=')
 if t.n==2 then
  self.name = t[1]
  self.def = t[t.n]
 end

 local b,e,d = strfind(self.name,"%[(.-)%]")
 if b then
  self.name = strsub(self.name,1,b-1)
  self.dim = d
 end

 if self.type ~= '' and self.type ~= 'void' and self.name == '' then
  self.name = create_varname()
 elseif self.kind=='var' then
  if self.type=='' and self.name~='' then
   self.type = self.type..self.name
   self.name = create_varname()
  elseif findtype(self.name) then
   if self.type=='' then self.type = self.name
   else self.type = self.type..' '..self.name end
   self.name = create_varname()
  end
 end

 -- adjust type of string
-- if self.type == 'char' and self.dim ~= '' then
--	 self.type = 'char*'
--	end
end

-- Check declaration type
-- Substitutes typedef's.
function classDeclaration:checktype ()

 -- check if there is a pointer to basic type
 if isbasic(self.type) and self.ptr~='' then
  self.ret = self.ptr
  self.ptr = nil
 end

 -- check if there is array to be returned
 if self.dim~='' and self.ret~='' then
   error('#invalid parameter: cannot return an array of values')
 end

 -- restore 'void*' and 'string*'
 if self.type == '_userdata' then self.type = 'void*'
 elseif self.type == '_cstring' then self.type = 'char*'
 elseif self.type == '_lstate' then self.type = 'lua_State*'
 end

--
-- -- if returning value, automatically set default value
-- if self.ret ~= '' and self.def == '' then
--  self.def = '0'
-- end
--

end

-- Print method
function classDeclaration:print (ident,close)
 print(ident.."Declaration{")
 print(ident.." mod  = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." ptr  = '"..self.ptr.."',")
 print(ident.." name = '"..self.name.."',")
 print(ident.." dim  = '"..self.dim.."',")
 print(ident.." def  = '"..self.def.."',")
 print(ident.." ret  = '"..self.ret.."',")
 print(ident.."}"..close)
end

-- check if array of values are returned to Lua
function classDeclaration:requirecollection (t)
 if  self.mod ~= 'const' and
	    self.dim and self.dim ~= '' and
				 not isbasic(self.type) and
				 self.ptr == '' then
		local type = gsub(self.type,"%s*const%s*","")
		t[type] = "tolua_collect_" .. gsub(type,"::","_")
		return true
	end
	return false
end

-- declare tag
function classDeclaration:decltype ()
 self.type = typevar(self.type)
 if strfind(self.mod,'const') then
	 self.type = 'const '..self.type
		self.mod = gsub(self.mod,'const%s*','')
	end
end


-- output type checking
function classDeclaration:outchecktype (narg,var)
 local def
 local t = isbasic(self.type)
 if self.def~='' then
  def = 1
 else
  def = 0
 end
 if self.dim ~= '' then 
	 if var and self.type=='char' then
   return 'tolua_isstring(tolua_S,'..narg..','..def..',&tolua_err)'
		else
   return 'tolua_istable(tolua_S,'..narg..',0,&tolua_err)'
		end
	elseif t then
  return 'tolua_is'..t..'(tolua_S,'..narg..','..def..',&tolua_err)'
	else
  return 'tolua_isusertype(tolua_S,'..narg..',"'..self.type..'",'..def..',&tolua_err)'
	end
end

function classDeclaration:builddeclaration (narg, cplusplus)
 local array = self.dim ~= '' and tonumber(self.dim)==nil
	local line = ""
 local ptr = ''
	local mod
	local type = self.type
 if self.dim ~= '' then
	 type = gsub(self.type,'const%s*','')  -- eliminates const modifier for arrays
	end
 local ctype = type
 if ctype=="lua_Object" or ctype=="lua_Function" then
   ctype = "int"
 end
 if self.ptr~='' then ptr = '*' end
 line = concatparam(line," ",self.mod,ctype,ptr)
 if array then
  line = concatparam(line,'*')
 end 
 line = concatparam(line,self.name)
 if self.dim ~= '' then
  if tonumber(self.dim)~=nil then
   line = concatparam(line,'[',self.dim,'];')
  else
		 if cplusplus then
			 line = concatparam(line,' = new',type,ptr,'['..self.dim..'];')
			else
    line = concatparam(line,' = (',type,ptr,'*)',
           'malloc((',self.dim,')*sizeof(',type,ptr,'));')
			end
  end
 else
  local t = isbasic(type)
  line = concatparam(line,' = ')
		if t == 'state' then
		 line = concatparam(line, 'tolua_S;')
		else
  if not t and ptr=='' then line = concatparam(line,'*') end
			local ct = type
			if t == 'value' or t == 'function' then
				ct = 'int'
			end
			line = concatparam(line,'((',self.mod,ct)
  if not t then
   line = concatparam(line,'*')
  end
  line = concatparam(line,') ')
		if isenum(type) then
			--if not t and isenum(type) then
		 line = concatparam(line,'(int) ')
		end
  local def = 0
  if self.def ~= '' then def = self.def end
  if t then
		  if t=='function' then t='value' end
      if self.type == "tolua_index" then
       line = concatparam(line,'tolua_to'..t,'(tolua_S,',narg,',',def,')-1);')
      else
       line = concatparam(line,'tolua_to'..t,'(tolua_S,',narg,',',def,'));')
      end
  else
   line = concatparam(line,'tolua_tousertype(tolua_S,',narg,',',def,'));')
			end
  end
 end
	return line
end

-- Declare variable
function classDeclaration:declare (narg)
 if self.dim ~= '' and self.type~='char' and tonumber(self.dim)==nil then
	 output('#ifdef __cplusplus\n')
		output(self:builddeclaration(narg,true))
		output('#else\n')
		output(self:builddeclaration(narg,false))
	 output('#endif\n')
	else
		output(self:builddeclaration(narg,false))
	end
end

-- Get parameter value
function classDeclaration:getarray (narg)
 if self.dim ~= '' then
	 local type = gsub(self.type,'const ','')
  output('  {')
	 output('#ifndef TOLUA_RELEASE\n')
  local def; if self.def~='' then def=1 else def=0 end
		local t = isbasic(type)
		if (t) then
   output('   if (!tolua_is'..t..'array(tolua_S,',narg,',',self.dim,',',def,',&tolua_err))')
		else
   output('   if (!tolua_isusertypearray(tolua_S,',narg,',"',type,'",',self.dim,',',def,',&tolua_err))')
		end
  output('    goto tolua_lerror;')
  output('   else\n')
	 output('#endif\n')
  output('   {')
  output('    int i;')
  output('    for(i=0; i<'..self.dim..';i++)')
  local t = isbasic(type)
  local ptr = ''
  if self.ptr~='' then ptr = '*' end
  output('   ',self.name..'[i] = ')
  if not t and ptr=='' then output('*') end
  output('((',type)
  if not t then
   output('*')
  end
  output(') ')
  local def = 0
  if self.def ~= '' then def = self.def end
  if t then
		 if t=='function' then t='value' end
   output('tolua_tofield'..t..'(tolua_S,',narg,',i+1,',def,'));')
  else 
   output('tolua_tofieldusertype(tolua_S,',narg,',i+1,',def,'));')
  end
  output('   }')
  output('  }')
 end
end

-- Get parameter value
function classDeclaration:setarray (narg)
 if not strfind(self.type,'const') and self.dim ~= '' then
	 local type = gsub(self.type,'const ','')
  output('  {')
  output('   int i;')
  output('   for(i=0; i<'..self.dim..';i++)')
  local t,ct = isbasic(type)
  if t then
		 if t=='function' then t='value' end
   output('    tolua_pushfield'..t..'(tolua_S,',narg,',i+1,(',ct,')',self.name,'[i]);')
  else
   if self.ptr == '' then
     output('   {')
     output('#ifdef __cplusplus\n')
     output('    void* tolua_obj = new',type,'(',self.name,'[i]);')
					output('    tolua_pushfieldusertype(tolua_S,',narg,',i+1,tolua_clone(tolua_S,tolua_obj,'.. (_collect[type] or 'NULL') ..'),"',type,'");')
     output('#else\n')
     output('    void* tolua_obj = tolua_copy(tolua_S,(void*)&',self.name,'[i],sizeof(',type,'));')
					output('    tolua_pushfieldusertype(tolua_S,',narg,',i+1,tolua_clone(tolua_S,tolua_obj,NULL),"',type,'");')
     output('#endif\n')
     output('   }')
   else
    output('   tolua_pushfieldusertype(tolua_S,',narg,',i+1,(void*)',self.name,'[i],"',type,'");')
   end
  end
  output('  }')
 end
end

-- Free dynamically allocated array
function classDeclaration:freearray ()
 if self.dim ~= '' and tonumber(self.dim)==nil then
	 output('#ifdef __cplusplus\n')
		output('  delete []',self.name,';')
	 output('#else\n')
  output('  free(',self.name,');')
	 output('#endif\n')
 end
end

-- Pass parameter
function classDeclaration:passpar ()
 local name = self.name
 if self.ptr=='&' then
  output('*'..name)
 elseif self.ret=='*' then
  output('&'..name)
 else
  output(name)
 end
end

-- Return parameter value
function classDeclaration:retvalue ()
 if self.ret ~= '' then
  local t,ct = isbasic(self.type)
  if t then
		 if t=='function' then t='value' end
     if self.type=="tolua_index" then
       output('   tolua_push'..t..'(tolua_S,(',ct,')'..self.name..'+1);')
     else
       output('   tolua_push'..t..'(tolua_S,(',ct,')'..self.name..');')
     end
  else
     output('   tolua_pushusertype(tolua_S,(void*)'..self.name..',"',self.type,'");')
   end
   return 1
 end
 return 0
end

-- Internal constructor
function _Declaration (t)
 setmetatable(t,classDeclaration)
 t:buildnames()
 t:checkname()
 t:checktype()
 return t
end

-- Constructor
-- Expects the string declaration.
-- The kind of declaration can be "var" or "func".
function Declaration (s,kind)
 -- eliminate spaces if default value is provided
 s = gsub(s,"%s*=%s*","=")

 if kind == "var" then
  -- check the form: void
  if s == '' or s == 'void' then
   return _Declaration{type = 'void', kind = kind}
  end
 end

 -- check the form: mod type*& name
 local t = split(s,'%*%s*&')
 if t.n == 2 then
  if kind == 'func' then
   error("#invalid function return type: "..s)
  end
  local m = split(t[1],'%s%s*')
  return _Declaration{
   name = t[2],
   ptr = '*',
   ret = '&',
   type = m[m.n],
   mod = concat(m,1,m.n-1),
   kind = kind
  }
 end

 -- check the form: mod type** name
 t = split(s,'%*%s*%*')
 if t.n == 2 then
  if kind == 'func' then
   error("#invalid function return type: "..s)
  end
  local m = split(t[1],'%s%s*')
  return _Declaration{
   name = t[2],
   ptr = '*',
   ret = '*',
   type = m[m.n],
   mod = concat(m,1,m.n-1),
   kind = kind
  }
 end
 
 -- check the form: mod type& name
 t = split(s,'&')
 if t.n == 2 then
  local m = split(t[1],'%s%s*')
  return _Declaration{
   name = t[2],
   ptr = '&',
   type = m[m.n],
   mod = concat(m,1,m.n-1)   ,
   kind = kind
  }
 end
  
 -- check the form: mod type* name
 local s1 = gsub(s,"(%b%[%])",function (n) return gsub(n,'%*','\1') end)
 t = split(s1,'%*')
 if t.n == 2 then
  t[2] = gsub(t[2],'\1','%*') -- restore * in dimension expression
  local m = split(t[1],'%s%s*')
  return _Declaration{
   name = t[2],
   ptr = '*',
   type = m[m.n],
   mod = concat(m,1,m.n-1)   ,
   kind = kind
  }
 end

 if kind == 'var' then
  -- check the form: mod type name
  t = split(s,'%s%s*')
  local v
  if findtype(t[t.n]) then v = '' else v = t[t.n]; t.n = t.n-1 end
  return _Declaration{
   name = v,
   type = t[t.n],
   mod = concat(t,1,t.n-1),
   kind = kind
  }

 else -- kind == "func"
 
  -- check the form: mod type name
  t = split(s,'%s%s*')
  local v = t[t.n]  -- last word is the function name
  local tp,md
  if t.n>1 then
   tp = t[t.n-1]
   md = concat(t,1,t.n-2)
  end
  return _Declaration{
   name = v,
   type = tp,
   mod = md,
   kind = kind
  }
 end

end

