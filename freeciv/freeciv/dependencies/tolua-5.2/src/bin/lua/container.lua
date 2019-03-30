-- tolua: container abstract class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: container.lua,v 1.3 2009/11/24 16:45:13 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications. 

-- table to store namespaced typedefs/enums in global scope
global_typedefs = {}
global_enums = {}

-- Container class
-- Represents a container of features to be bound
-- to lua.
classContainer = 
{
 curr = nil,
}
classContainer.__index = classContainer
setmetatable(classContainer,classFeature)

-- output tags
function classContainer:decltype ()
 push(self)
 local i=1
 while self[i] do
  self[i]:decltype()
  i = i+1
 end
 pop()
end


-- write support code
function classContainer:supcode ()
 push(self)
 local i=1
 while self[i] do
  self[i]:supcode()
  i = i+1
 end
 pop()
end

function classContainer:hasvar ()
 local i=1
 while self[i] do
  if self[i]:isvariable() then
		 return 1
		end
  i = i+1
 end
	return 0
end

-- Internal container constructor
function _Container (self)
 setmetatable(self,classContainer)
 self.n = 0
 self.typedefs = {tolua_n=0}
 self.usertypes = {}
 self.enums = {tolua_n=0}
 self.lnames = {}
 return self
end

-- push container
function push (t)
	t.prox = classContainer.curr
 classContainer.curr = t
end

-- pop container
function pop ()
--print("name",classContainer.curr.name)
--foreach(classContainer.curr.usertypes,print)
--print("______________")
 classContainer.curr = classContainer.curr.prox
end

-- get current namespace
function getcurrnamespace ()
	return getnamespace(classContainer.curr) 
end

-- append to current container
function append (t)
 return classContainer.curr:append(t)
end
 
-- append typedef to current container
function appendtypedef (t)
 return classContainer.curr:appendtypedef(t)
end

-- append usertype to current container
function appendusertype (t)
 return classContainer.curr:appendusertype(t)
end

-- append enum to current container
function appendenum (t)
 return classContainer.curr:appendenum(t)
end
 
-- substitute typedef
function applytypedef (type)
 return classContainer.curr:applytypedef(type)
end

-- check if is type
function findtype (type)
 local t = classContainer.curr:findtype(type)
	return t
end

-- check if is typedef
function istypedef (type)
 return classContainer.curr:istypedef(type)
end

-- get fulltype (with namespace)
function fulltype (t)
 local curr =  classContainer.curr
	while curr do
	 if curr then
		 if curr.typedefs and curr.typedefs[t] then
		  return curr.typedefs[t]
		 elseif curr.usertypes and curr.usertypes[t] then
		  return curr.usertypes[t]
			end
		end
	 curr = curr.prox
	end
	return t
end

-- checks if it requires collection
function classContainer:requirecollection (t)
 push(self)
 local i=1
	local r = false
 while self[i] do
  r = self[i]:requirecollection(t) or r
  i = i+1
 end
	pop()
	return r
end


-- get namesapce
function getnamespace (curr)
	local namespace = ''
	while curr do
	 if curr and 
		   ( curr.classtype == 'class' or curr.classtype == 'namespace')
		then
		 namespace = curr.name .. '::' .. namespace 
		end
	 curr = curr.prox
	end
	return namespace
end

-- get namespace (only namespace)
function getonlynamespace ()
 local curr = classContainer.curr
	local namespace = ''
	while curr do
		if curr.classtype == 'class' then
		 return namespace 
		elseif curr.classtype == 'namespace' then
		 namespace = curr.name .. '::' .. namespace 
		end
	 curr = curr.prox
	end
	return namespace
end

-- check if is enum
function isenum (type)
 return classContainer.curr:isenum(type)
end

-- append feature to container
function classContainer:append (t)
 self.n = self.n + 1
 self[self.n] = t
 t.parent = self
end

-- append typedef 
function classContainer:appendtypedef (t)
 local namespace = getnamespace(classContainer.curr)
 self.typedefs.tolua_n = self.typedefs.tolua_n + 1
 self.typedefs[self.typedefs.tolua_n] = t
 self.typedefs[t.utype] = namespace .. t.utype
 global_typedefs[namespace..t.utype] = t
end

-- append usertype: return full type
function classContainer:appendusertype (t)
 local container
	if t == self.name then
	 container = self.prox
	else
	 container = self
	end
 local ft = getnamespace(container) .. t
	container.usertypes[t] = ft
	_usertype[ft] = ft
	return ft
end

-- append enum 
function classContainer:appendenum (t)
 local namespace = getnamespace(classContainer.curr)
 self.enums.tolua_n = self.enums.tolua_n + 1
 self.enums[self.enums.tolua_n] = t
 global_enums[namespace..t.name] = t
end

-- determine lua function name overload
function classContainer:overload (lname)
 if not self.lnames[lname] then 
  self.lnames[lname] = 0
 else
  self.lnames[lname] = self.lnames[lname] + 1
 end
 return format("%02d",self.lnames[lname])
end

-- applies typedef: returns the 'the facto' modifier and type
function classContainer:applytypedef (type)
 if global_typedefs[type] then
	local mod1, type1 = global_typedefs[type].mod, global_typedefs[type].type 
	local mod2, type2 = applytypedef(type1)
	return mod2 .. ' ' .. mod1, type2
 end
 local basetype = gsub(type,"^.*::","")
 local env = self
 while env do
  if env.typedefs then
   local i=1
   while env.typedefs[i] do
    if env.typedefs[i].utype == basetype then
	    local mod1,type1 = env.typedefs[i].mod,env.typedefs[i].type
     local mod2,type2 = applytypedef(type1)
     return mod2..' '..mod1,type2
	   end
	  i = i+1
   end
  end
  env = env.parent
 end
 return '',type
end 

-- check if it is a typedef
function classContainer:istypedef (type)
 local env = self
 while env do
  if env.typedefs then
   local i=1
   while env.typedefs[i] do
    if env.typedefs[i].utype == type then
         return type
        end
        i = i+1
   end
  end
  env = env.parent
 end
 return nil 
end

-- check if is a registered type: return full type or nil
function classContainer:findtype (t)
 local curr =  self
	while curr do
		if curr.typedefs and curr.typedefs[t] then
		 return curr.typedefs[t]
		elseif curr.usertypes and curr.usertypes[t] then
		 return curr.usertypes[t]
		end
	 curr = curr.prox
	end
	if _basic[t] then
	 return t
	end
	return nil
end


function classContainer:isenum (type)
 if global_enums[type] then
  return true
 end
 local basetype = gsub(type,"^.*::","")
 local env = self
 while env do
  if env.enums then
   local i=1
   while env.enums[i] do
    if env.enums[i].name == basetype then
         return true
        end
        i = i+1
   end
  end
  env = env.parent
 end
 return false 
end

-- parse chunk
function classContainer:doparse (s)

 -- try Lua code
 do
  local b,e,code = strfind(s,"^%s*(%b\1\2)")
  if b then
   Code(strsub(code,2,-2))
   return strsub(s,e+1)
  end 
 end 

 -- try C code
 do
  local b,e,code = strfind(s,"^%s*(%b\3\4)")
  if b then
		 code = '{'..strsub(code,2,-2)..'\n}\n'
   Verbatim(code,'r')        -- verbatim code for 'r'egister fragment
   return strsub(s,e+1)
  end 
 end 

 -- try verbatim
 do
  local b,e,line = strfind(s,"^%s*%$(.-\n)")
  if b then
   Verbatim(line)
   return strsub(s,e+1)
  end 
 end 


 -- try module
 do
  local b,e,name,body = strfind(s,"^%s*module%s%s*([_%w][_%w]*)%s*(%b{})%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Module(name,body)
   return strsub(s,e+1)
  end
 end

 -- try namesapce
 do
  local b,e,name,body = strfind(s,"^%s*namespace%s%s*([_%w][_%w]*)%s*(%b{})%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Namespace(name,body)
   return strsub(s,e+1)
  end
 end

 -- try define
 do
  local b,e,name = strfind(s,"^%s*#define%s%s*([^%s]*)[^\n]*\n%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Define(name)
   return strsub(s,e+1)
  end
 end

 -- try enumerates
 do
  local b,e,name,body = strfind(s,"^%s*enum%s+(%S*)%s*(%b{})%s*;?%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Enumerate(name,body)
   return strsub(s,e+1)
  end
 end

 do
  local b,e,body,name = strfind(s,"^%s*typedef%s+enum[^{]*(%b{})%s*([%w_][^%s]*)%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Enumerate(name,body)
   return strsub(s,e+1)
  end
 end

 -- try operator 
 do
  local b,e,decl,kind,arg,const = strfind(s,"^%s*([_%w][_%w%s%*&:]*operator)%s*([^%s][^%s]*)%s*(%b())%s*(c?o?n?s?t?)%s*;%s*")
  if not b then
		 -- try inline
   b,e,decl,kind,arg,const = strfind(s,"^%s*([_%w][_%w%s%*&:]*operator)%s*([^%s][^%s]*)%s*(%b())%s*(c?o?n?s?t?)%s*%b{}%s*;?%s*")
  end 
		if b then
   _curr_code = strsub(s,b,e)
   Operator(decl,kind,arg,const)
   return strsub(s,e+1)
  end
 end

 -- try function
 do
  local b,e,decl,arg,const = strfind(s,"^%s*([~_%w][_@%w%s%*&:]*[_%w])%s*(%b())%s*(c?o?n?s?t?)%s*=?%s*0?%s*;%s*")
  if not b then
   -- try a single letter function name
   b,e,decl,arg,const = strfind(s,"^%s*([_%w])%s*(%b())%s*(c?o?n?s?t?)%s*;%s*")
  end
  if b then
   _curr_code = strsub(s,b,e)
   Function(decl,arg,const)
   return strsub(s,e+1)
  end
 end

 -- try inline function
 do
  local b,e,decl,arg,const = strfind(s,"^%s*([~_%w][_@%w%s%*&:]*[_%w])%s*(%b())%s*(c?o?n?s?t?).-%b{}%s*;?%s*")
  if not b then
   -- try a single letter function name
   b,e,decl,arg,const = strfind(s,"^%s*([_%w])%s*(%b())%s*(c?o?n?s?t?).-%b{}%s*;?%s*")
  end
  if b then
   _curr_code = strsub(s,b,e)
   Function(decl,arg,const)
   return strsub(s,e+1)
  end
 end

 -- try class
 do
	 local b,e,name,base,body
		base = '' body = ''
		b,e,name = strfind(s,"^%s*class%s*([_%w][_%w@]*)%s*;")  -- dummy class
		if not b then
			b,e,name = strfind(s,"^%s*struct%s*([_%w][_%w@]*)%s*;")    -- dummy struct
			if not b then
				b,e,name,base,body = strfind(s,"^%s*class%s*([_%w][_%w@]*)%s*(.-)%s*(%b{})%s*;%s*")
				if not b then
					b,e,name,base,body = strfind(s,"^%s*struct%s*([_%w][_%w@]*)%s*(.-)%s*(%b{})%s*;%s*")
					if not b then
						b,e,name,base,body = strfind(s,"^%s*union%s*([_%w][_%w@]*)%s*(.-)%s*(%b{})%s*;%s*")
						if not b then
							base = ''
							b,e,body,name = strfind(s,"^%s*typedef%s%s*struct%s*[_%w]*%s*(%b{})%s*([_%w][_%w@]*)%s*;%s*")
						if not b then
							base = ''
								b,e,body,name = strfind(s,"^%s*typedef%s%s*union%s*[_%w]*%s*(%b{})%s*([_%w][_%w@]*)%s*;%s*")
							end
						end
					end
				end
			end
		end
		if b then
			if base ~= '' then 
				local b,e
				b,e,base = strfind(base,".-([_%w][_%w]*)$") 
			end
			_curr_code = strsub(s,b,e)
			Class(name,base,body)
			return strsub(s,e+1)
		end
	end

 -- try typedef
 do
  local b,e,types = strfind(s,"^%s*typedef%s%s*(.-)%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Typedef(types)
   return strsub(s,e+1)
  end
 end

 -- try variable
 do
  local b,e,decl = strfind(s,"^%s*([_%w][_@%s%w%d%*&:]*[_%w%d])%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Variable(decl)
   return strsub(s,e+1)
  end
 end

	-- try string
 do
  local b,e,decl = strfind(s,"^%s*([_%w]?[_%s%w%d]-char%s+[_@%w%d]*%s*%[%s*%S+%s*%])%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Variable(decl)
   return strsub(s,e+1)
  end
 end

 -- try array
 do
  local b,e,decl = strfind(s,"^%s*([_%w][][_@%s%w%d%*&:]*[]_%w%d])%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Array(decl)
   return strsub(s,e+1)
  end
 end

 -- no matching
 if gsub(s,"%s%s*","") ~= "" then
  _curr_code = s
  error("#parse error")
 else
  return ""
 end

end

function classContainer:parse (s)
 while s ~= '' do
  s = self:doparse(s)
 end
end


