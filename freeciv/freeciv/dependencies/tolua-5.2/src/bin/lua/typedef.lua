-- tolua: typedef class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: typedef.lua,v 1.3 2009/11/24 16:45:15 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications. 



-- Typedef class
-- Represents a type synonym.
-- The 'de facto' type replaces the typedef before the
-- remaining code is parsed.
-- The following fields are stored:
--   utype = typedef name
--   type = 'the facto' type
--   mod = modifiers to the 'de facto' type
classTypedef = {
 utype = '',
 mod = '',
 type = ''
}
classTypedef.__index = classTypedef

-- Print method
function classTypedef:print (ident,close)
 print(ident.."Typedef{")
 print(ident.." utype = '"..self.utype.."',")
 print(ident.." mod = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.."}"..close)
end

-- Return it's not a variable
function classTypedef:isvariable ()
 return false
end

-- Internal constructor
function _Typedef (t)
 setmetatable(t,classTypedef)
 appendtypedef(t)
 return t
end

-- Constructor
-- Expects one string representing the type definition.
function Typedef (s)
 if strfind(s,'[%*&]') then
  tolua_error("#invalid typedef: pointers (and references) are not supported")
 end
 local t = split(gsub(s,"%s%s*"," ")," ")
	if not isbasic(t[t.n]) then
 return _Typedef {
  utype = t[t.n],
  type = t[t.n-1],
  mod = concat(t,1,t.n-2),
 }
	else
	 return nil
	end
end


