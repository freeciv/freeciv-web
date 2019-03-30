-- tolua: operator class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: operator.lua,v 1.3 2009/11/24 16:45:14 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications. 


-- Operator class
-- Represents an operator function or a class operator method.
-- It stores the same fields as functions do plus:
--  kind = set of character representing the operator (as it appers in C++ code)
classOperator = {
 kind = '',
}
classOperator.__index = classOperator
setmetatable(classOperator,classFunction)

-- table to transform operator kind into the appropriate tag method name
_TM = {['+'] = 'add',
       ['-'] = 'sub',
       ['*'] = 'mul',
       ['/'] = 'div',
       ['<'] = 'lt',
       ['<='] = 'le',
       ['=='] = 'eq',
       ['[]'] = 'geti',
       ['&[]'] = 'seti',
      }
       

-- Print method
function classOperator:print (ident,close)
 print(ident.."Operator{")
 print(ident.." kind  = '"..self.kind.."',")
 print(ident.." mod  = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." ptr  = '"..self.ptr.."',")
 print(ident.." name = '"..self.name.."',")
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

-- Internal constructor
function _Operator (t)
 setmetatable(t,classOperator)

 if t.const ~= 'const' and t.const ~= '' then
  error("#invalid 'const' specification")
 end

 append(t)
 if not t:inclass() then
  error("#operator can only be defined as class member")
 end

 t.name = t.name .. "_" .. _TM[t.kind]
 t.cname = t:cfuncname("tolua")..t:overload(t)
 t.name = "operator" .. t.kind  -- set appropriate calling name
 return t
end

-- Constructor
function Operator (d,k,a,c)
	local ref = ''
 local t = split(strsub(a,2,strlen(a)-1),',') -- eliminate braces
 local i=1
 local l = {n=0}
 while t[i] do
  l.n = l.n+1
  l[l.n] = Declaration(t[i],'var')
  i = i+1
 end
 if k == '[]' then
	 local _
	 _, _, ref = strfind(d,'(&)')
  d = gsub(d,'&','')
 elseif k=='&[]' then
  l.n = l.n+1
  l[l.n] = Declaration(d,'var')
  l[l.n].name = 'tolua_value'
 end
 local f = Declaration(d,'func')
 if k == '[]' and (l[1]==nil or isbasic(l[1].type)~='number') then
  error('operator[] can only be defined for numeric index.')
 end
 f.args = l
 f.const = c
 f.kind = gsub(k,"%s","")
 if not _TM[f.kind] then
  error("tolua: no support for operator" .. f.kind)
 end
 f.lname = ".".._TM[f.kind]
 if f.kind == '[]' and ref=='&' and f.const~='const' then
  Operator(d,'&'..k,a,c) 	-- create correspoding set operator
 end
 return _Operator(f)
end


