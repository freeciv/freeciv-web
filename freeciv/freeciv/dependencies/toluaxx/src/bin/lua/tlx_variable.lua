-- tolua: variable class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: tlx_variable.lua 14751 2008-06-14 15:24:19Z cazfi $

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
   print(ident.." lname = '"..self.lname.."',")
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
      if self.parent.classtype == 'class' then
	 parent = "_" .. self.parent.type
      else
	 parent = "_" .. p
      end
   end
   
   if strfind(self.mod,"(unsigned)") then
      unsigned = "_unsigned"
   end
   
   if self.ptr == "*" then ptr = "_ptr"
   elseif self.ptr == "&" then ptr = "_ref"
   end
   
   local name = prefix .. parent .. unsigned .. "_" .. gsub(self.lname or self.name,".*::","") .. ptr
   
   name = clean_template(name)
   return name
   
end

-- check if it is a variable
function classVariable:isvariable ()
   return true
end

-- get variable value
function classVariable:getvalue ()
   local prop_get = self.prop_get
   local name
   if prop_get then name = prop_get.."()"
   else name = self.name end
   -- ++ <<
   local p = self:innamespace()
   if p and self.parent.classtype ~= 'class' then
      return p.."::"..gsub(self.lname or self.name,".*::","")
   end
   -- ++ >>
   if self:inclass() and self:isstatic() then
      return self.parent.type..'::'..name
   elseif self:inclass() then
      return 'self->'..name
   else
      return name
   end
end

function classVariable:setvalue (value)
   local name,parent="",""
   if self:innamespace() and not self:inclass() then parent = self:innamespace() end
   --if self.prop_set then name = self.prop_set.."("..value..");"
   --name = self.lname.."="..value..";" end
   if self:inclass() then parent = self.parent.type end
   if self.prop_set then name = self.prop_set.."("..value..");"
   else name = self.lname.."="..value..";" end
   --print("self.name=",self.name,"self.lname=",self.lname)
   if self:inclass() and self:isstatic() or not self:inclass() and self:innamespace() then
      return parent..'::'..name
   elseif self:inclass() and not self:isstatic() then
      return 'self->'..name
   else
      return name
   end
end

-- get variable pointer value
function classVariable:getpointervalue ()
   local class = self:inclass()
   local static = self.static
   if class and static then
      return class..'::p'
   elseif class then
      return 'self->p'
   else
      return 'p'
   end
end

function classVariable:checkproperty()
   if string.find(self.mod, 'tolua_property') then
      local _,_,type = string.find(self.mod, "tolua_property__([^%s]*)")
      type = type or "default"
      self.prop_get,self.prop_set = get_property_methods(type, self.name)
      self.mod = string.gsub(self.mod, "tolua_property[^%s]*", "")
   end
end

function classVariable:checkindex()
   if string.find(self.mod, 'tolua_index') then
      local _,_,type = string.find(self.mod, "tolua_index__([^%s]*)")
      type = type or "default"
      self.prop_get,self.prop_set = get_index_methods(type, self.name)
      self.mod = string.gsub(self.mod, "tolua_index[^%s]*", "")
   end
end

function classVariable:checkstatic()
   local _,_,static = strfind(self.mod,'^%s*(static)')
   if static then
      self.static = true
      _,_,self.mod = strfind(self.mod,'^%s*static%s%s*(.*)')
   else
      self.static = false
   end
end

function classVariable:isstatic()
  return self.static
end

function classVariable:declself()
   if self:inclass() and not self:isstatic() then
      -- declare self, if the case
      return ' '..self.parent.type..'*self = ('..self.parent.type..'*) tolua_tousertype(tolua_S,1,0);\n'..
	 '#ifndef TOLUA_RELEASE\n'..
	 '  if (!self) tolua_error(tolua_S,"invalid \'self\' in accessing variable \''..self.name..'\'",NULL);\n'..
	 '#endif\n'
   else
      return ''
   end
end

function classVariable:declfunc_begin(name)
   local pref=""
   if self:inclass() then
      pref=pref.."/* get function:"..self.name.." of class "..self:inclass().." */\n"
   else
      pref=pref.."/* get function:"..self.name.." */\n"
   end
   return pref.."#ifndef TOLUA_DISABLE_"..name.."\n"..
      "static int "..name.." (lua_State* tolua_S) {\n"..
      " "..self:declself()
end

function classVariable:declfunc_end(name,retn)
   return "  return "..tostring(retn)..";"..
      "}\n"..
      "#endif /* #ifndef TOLUA_DISABLE_"..name.." */\n"..
      "\n"
end

-- Write binding functions
function classVariable:supcode ()
   local class = self:inclass()
   
   self:checkproperty()
   
   self:checkstatic()

   -- get function ------------------------------------------------
   
   self.cgetname = self:cfuncname("tolua_get")
   output(self:declfunc_begin(self.cgetname))
   
   -- return value
   if string.find(self.mod, 'tolua_inherits') then
      output('#ifdef __cplusplus\n')
      output('  tolua_pushusertype(tolua_S,(void*)static_cast<'..self.type..'*>(self), "',self.type,'");')
      output('#else\n')
      output('  tolua_pushusertype(tolua_S,(void*)(('..self.type..'*)self), "',self.type,'");')
      output('#endif\n')
   else
      local t,ct = isbasic(self.type)
      if t then
	 output('  tolua_push'..t..'(tolua_S,(',ct,')'..self:getvalue()..');')
      else
	 t = self.type
	 if self.ptr == '&' or self.ptr == '' then
	    output('  tolua_pushusertype(tolua_S,(void*)&'..self:getvalue()..',"',t,'");')
	 else
	    output('  tolua_pushusertype(tolua_S,(void*)'..self:getvalue()..',"',t,'");')
	 end
      end
   end

   output(self:declfunc_end(self.cgetname,1))
   
   -- set function ------------------------------------------------
   if not (strfind(self.type,'const%s+') or string.find(self.mod, 'tolua_readonly') or string.find(self.mod, 'tolua_inherits')) then
      
      self.csetname = self:cfuncname("tolua_set")
      output(self:declfunc_begin(self.csetname))
      
      -- check variable type
      output('#ifndef TOLUA_RELEASE\n')
      output('  tolua_Error tolua_err;\n')
      output('  if (!'..self:outchecktype(2)..') tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);')
      output('#endif\n')
      
      -- assign value
      local def = 0
      if self.def ~= '' then def = self.def end
      if self.type == 'char*' and self.dim ~= '' then -- is string
	 output(' strncpy(')
	 output(self:getvalue())
	 output(',tolua_tostring(tolua_S,2,',def,'),',self.dim,'-1);')
      else
	 local val = ''
	 local ptr = ''
	 if self.ptr~='' then ptr = '*' end
	 
	 local t = isbasic(self.type)
	 
	 if not t and ptr=='' then val=val..'*' end
	 val=val..'(('..self.mod.." "..self.type
	 if not t then
	    val=val..'*'
	 end
	 val=val..') '
	 if t then
	    if isenum(self.type) then
	       val=val..'(int) '
	    end
	    val=val..'tolua_to'..t..'(tolua_S,2,'..def..'))'
	 else
	    val=val..'tolua_tousertype(tolua_S,2,'..def..'))'
	 end
	 
	 output(self:setvalue(val))
      end
      
      output(self:declfunc_end(self.csetname,0))
   end
end

function classVariable:register (pre)
   if not self:check_public_access() then
      return
   end
   pre = pre or ''
   local parent = self:inmodule() or self:innamespace() or self:inclass()
   if not parent then
      if classVariable._warning==nil then
	 warning("Mapping variable to global may degrade performance")
	 classVariable._warning = 1
      end
   end
   if self.csetname then
      output(pre..'tolua_variable(tolua_S,"'..self.lname..'",'..self.cgetname..','..self.csetname..');')
   else
      output(pre..'tolua_variable(tolua_S,"'..self.lname..'",'..self.cgetname..',NULL);')
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


