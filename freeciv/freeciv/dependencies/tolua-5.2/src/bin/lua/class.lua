-- tolua: class class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: class.lua,v 1.3 2009/11/24 16:45:13 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Class class
-- Represents a class definition.
-- Stores the following fields:
--    name = class name
--    base = class base, if any (only single inheritance is supported)
--    {i}  = list of members
classClass = {
 classtype = 'class',
 name = '',
 base = '',
	type = '',
 btype = '',
	ctype = '',
}
classClass.__index = classClass
setmetatable(classClass,classContainer)


-- register class
function classClass:register ()
 push(self)
 if _collect[self.type] then
 	output('#ifdef __cplusplus\n')
 	output(' tolua_cclass(tolua_S,"'..self.lname..'","'..self.type..'","'..self.btype..'",'.._collect[self.type]..');')
 	output('#else\n')
 	output(' tolua_cclass(tolua_S,"'..self.lname..'","'..self.type..'","'..self.btype..'",NULL);')
 	output('#endif\n')
 else
	output(' tolua_cclass(tolua_S,"'..self.lname..'","'..self.type..'","'..self.btype..'",NULL);')
 end
 output(' tolua_beginmodule(tolua_S,"'..self.lname..'");')
 local i=1
 while self[i] do
  self[i]:register()
  i = i+1
 end
	output(' tolua_endmodule(tolua_S);')
	pop()
end

-- return collection requirement
function classClass:requirecollection (t)
 push(self)
	local r = false
 local i=1
 while self[i] do
  r = self[i]:requirecollection(t) or r
  i = i+1
 end
 pop()
	-- only class that exports destructor can be appropriately collected  
	if self._delete then
  t[self.type] = "tolua_collect_" .. gsub(self.type,"::","_")
		r = true
	end
 return r
end

-- output tags
function classClass:decltype ()
 push(self)
	self.type = regtype(self.name)
	self.btype = typevar(self.base)
	self.ctype = 'const '..self.type
 local i=1
 while self[i] do
  self[i]:decltype()
  i = i+1
 end
	pop()
end


-- Print method
function classClass:print (ident,close)
 print(ident.."Class{")
 print(ident.." name = '"..self.name.."',")
 print(ident.." base = '"..self.base.."';")
 print(ident.." lname = '"..self.lname.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." btype = '"..self.btype.."',")
 print(ident.." ctype = '"..self.ctype.."',")
 local i=1
 while self[i] do
  self[i]:print(ident.." ",",")
  i = i+1
 end
 print(ident.."}"..close)
end

-- Internal constructor
function _Class (t)
 setmetatable(t,classClass)
 t:buildnames()
 append(t)
 return t
end

-- Constructor
-- Expects the name, the base and the body of the class.
function Class (n,p,b)
 local c = _Class(_Container{name=n, base=p})
 push(c)
 c:parse(strsub(b,2,strlen(b)-1)) -- eliminate braces
 pop()
end


