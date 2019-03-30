-- tolua: variable class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: variable.lua,v 1.3 2009/11/24 16:45:15 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications. 


-- Variable class
-- Represents a extern variable or a public member of a class.
-- Stores all fields present in a declaration.
classVariable = {
 _get = {},   -- mapped get functions
 _set = {},   -- mapped set functions
}
classVariable.__index = classVariable
setmetatable(classVariable,classDeclaration)

-- Print method
function classVariable:print (ident,close)
 print(ident.."Variable{")
 print(ident.." mod  = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." ptr  = '"..self.ptr.."',")
 print(ident.." name = '"..self.name.."',")
 if self.dim then print(ident.." dim = '"..self.dim.."',") end
 print(ident.." def  = '"..self.def.."',")
 print(ident.." ret  = '"..self.ret.."',")
 print(ident.."}"..close)
end

-- Generates C function name
function classVariable:cfuncname (prefix)
 local parent = ""
 local unsigned = ""
 local ptr = ""

 local p = self:inmodule() or self:innamespace() or self:inclass()

 if p then
  parent = "_" .. p
 end

 if strfind(self.mod,"(unsigned)") then
  unsigned = "_unsigned"
 end

 if self.ptr == "*" then ptr = "_ptr"
 elseif self.ptr == "&" then ptr = "_ref"
 end

 local name =  prefix .. parent .. unsigned .. "_" .. gsub(self.name,".*::","")  .. ptr 

 return name

end

-- check if it is a variable
function classVariable:isvariable ()
 return true
end

-- get variable value
function classVariable:getvalue (class,static)
 if class and static then
  return class..'::'..self.name
 elseif class then
  return 'self->'..self.name
 else
  return self.name
 end
end

-- get variable pointer value
function classVariable:getpointervalue (class,static)
 if class and static then
  return class..'::p'
 elseif class then
  return 'self->p'
 else
  return 'p'
 end
end

-- Write binding functions
function classVariable:supcode ()
 local class = self:inclass()

 -- get function ------------------------------------------------
 if class then
  output("/* get function:",self.name," of class ",class," */")
  self.cgetname = self:cfuncname("tolua_get_"..class)
 else
  output("/* get function:",self.name," */")
  self.cgetname = self:cfuncname("tolua_get")
 end
 
 output("static int",self.cgetname,"(lua_State* tolua_S)") 
 output("{")

 -- declare self, if the case
 local _,_,static = strfind(self.mod,'^%s*(static)')
 if class and static==nil then
  output(' ',class,'*','self = ')
  output('(',class,'*) ')
  output('tolua_tousertype(tolua_S,1,0);')
 elseif static then
  _,_,self.mod = strfind(self.mod,'^%s*static%s%s*(.*)')
 end


 -- check self value
 if class and static==nil then
	 output('#ifndef TOLUA_RELEASE\n')
  output('  if (!self) tolua_error(tolua_S,"invalid \'self\' in accessing variable \''..self.name..'\'",NULL);');
		output('#endif\n')
 end

 -- return value
	local type = self.type
	if gsub(type,'const ','')=='char' and self.dim~='' then
	 type = 'char*'
	end
 local t,ct = isbasic(type)
 if t then
  output('  tolua_push'..t..'(tolua_S,(',ct,')'..self:getvalue(class,static)..');')
 else
	 t = self.type
  if self.ptr == '&' or self.ptr == '' then
   output('  tolua_pushusertype(tolua_S,(void*)&'..self:getvalue(class,static)..',"',t,'");')
  else
   output('  tolua_pushusertype(tolua_S,(void*)'..self:getvalue(class,static)..',"',t,'");')
  end
 end
 output(' return 1;')
 output('}')
 output('\n')

 -- set function ------------------------------------------------
 if not strfind(self.type,'const') then
  if class then
   output("/* set function:",self.name," of class ",class," */")
   self.csetname = self:cfuncname("tolua_set_"..class)
  else
   output("/* set function:",self.name," */")
   self.csetname = self:cfuncname("tolua_set")
  end
  output("static int",self.csetname,"(lua_State* tolua_S)")
  output("{")

  -- declare self, if the case
  if class and static==nil then
   output(' ',class,'*','self = ')
   output('(',class,'*) ')
   output('tolua_tousertype(tolua_S,1,0);')
   -- check self value
		end
  -- check types
		output('#ifndef TOLUA_RELEASE\n')
		output('  tolua_Error tolua_err;')
  if class and static==nil then
   output('  if (!self) tolua_error(tolua_S,"invalid \'self\' in accessing variable \''..self.name..'\'",NULL);');
  elseif static then
   _,_,self.mod = strfind(self.mod,'^%s*static%s%s*(.*)')
  end

  -- check variable type
  output('  if (!'..self:outchecktype(2,true)..')')
  output('   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);')
		output('#endif\n')
 
  -- assign value
		local def = 0
		if self.def ~= '' then def = self.def end
		if self.type == 'char' and self.dim ~= '' then -- is string
		 output(' strncpy(')
			if class and static then
				output(class..'::'..self.name)
			elseif class then
				output('self->'..self.name)
			else
				output(self.name)
			end
			output(',tolua_tostring(tolua_S,2,',def,'),',self.dim,'-1);')
		else
			local ptr = ''
			if self.ptr~='' then ptr = '*' end
			output(' ')
			if class and static then
				output(class..'::'..self.name)
			elseif class then
				output('self->'..self.name)
			else
				output(self.name)
			end
			local t = isbasic(self.type)
			output(' = ')
			if not t and ptr=='' then output('*') end
			output('((',self.mod,self.type)
			if not t then
				output('*')
			end
			output(') ')
			if t then
			 if isenum(self.type) then
				 output('(int) ')
				end
				if t=='function' then t='value' end 
				output('tolua_to'..t,'(tolua_S,2,',def,'));')
			else
				output('tolua_tousertype(tolua_S,2,',def,'));')
			end
		end
  output(' return 0;')
  output('}')
  output('\n')
 end 

end

function classVariable:register ()
 local parent = self:inmodule() or self:innamespace() or self:inclass()
 if not parent then
  if classVariable._warning==nil then
   warning("Mapping variable to global may degrade performance")
   classVariable._warning = 1 
  end
 end
 if self.csetname then
  output(' tolua_variable(tolua_S,"'..self.lname..'",'..self.cgetname..','..self.csetname..');')
 else
  output(' tolua_variable(tolua_S,"'..self.lname..'",'..self.cgetname..',NULL);')
 end
end

-- Internal constructor
function _Variable (t)
 setmetatable(t,classVariable)
 append(t)
 return t
end

-- Constructor
-- Expects a string representing the variable declaration.
function Variable (s)
 return _Variable (Declaration(s,'var'))
end


