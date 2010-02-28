-- tolua: operator class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: tlx_operator.lua 13010 2007-06-22 21:18:04Z wsimpson $

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
_TM = {
   ['+'] = 'add',
   ['-'] = 'sub',
   ['*'] = 'mul',
   ['/'] = 'div',
   ['^'] = 'pow',
   ['%'] = 'mod',
   ['<'] = 'lt',
   ['<='] = 'le',
   ['=='] = 'eq',
   ['|'] = 'concat',
   ['[]'] = 'geti',
   ['&[]'] = 'seti',
   ['()'] = 'callself',
   --['->'] = 'flechita',
}
-- unary operator
-- ++ <<
_TM0={
   ['()'] = 'callself',
   ['-'] = 'unm',
   ['~'] = 'len',
}
-- ++ >>

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

function classOperator:supcode_tmp()
   -- -- if not _TM[self.kind] then
   -- ++ <<
   if not _TM[self.kind] and not _TM0[self.kind] then
   -- ++ >>
      return classFunction.supcode(self)
   end
   
   -- no overload, no parameters, always inclass
   output("/* method:",self.name," of class ",self:inclass()," */")
   
   output("#ifndef TOLUA_DISABLE_"..self.cname)
   output("\nstatic int",self.cname,"(lua_State* tolua_S)")
   
   if overload < 0 then
      output('#ifndef TOLUA_RELEASE\n')
   end
   output(' tolua_Error tolua_err;')
   output(' if (\n')
   -- check self
   output('     !'..'tolua_isusertype(tolua_S,1,"'..self.parent.type..'",0,&tolua_err) ||\n')
   output('     !tolua_isnoobj(tolua_S,2,&tolua_err)\n )')
   output('  goto tolua_lerror;')
   
   output(' else\n')
   output('#endif\n') -- tolua_release
   output(' {')
   
   -- declare self
   output(' ',self.const,self.parent.type,'*','self = ')
   output('(',self.const,self.parent.type,'*) ')
   output('tolua_tousertype(tolua_S,1,0);')
   
   -- check self
   output('#ifndef TOLUA_RELEASE\n')
   output('  if (!self) tolua_error(tolua_S,"invalid \'self\' in function \''..self.name..'\'",NULL);');
   output('#endif\n')
   
   -- cast self
   output('  ',self.mod,self.type,self.ptr,'tolua_ret = ')
   output('(',self.mod,self.type,self.ptr,')(*self);')
   
   -- return value
   local t,ct = isbasic(self.type)
   if t then
      output('   tolua_push'..t..'(tolua_S,(',ct,')tolua_ret);')
   else
      t = self.type
      new_t = string.gsub(t, "const%s+", "")
      if self.ptr == '' then
	 output('   {')
	 output('#ifdef __cplusplus\n')
	 output('    void* tolua_obj = new',new_t,'(tolua_ret);')
	 output('    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"',t,'");')
	 output('#else\n')
	 output('    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(',t,'));')
	 output('    tolua_pushusertype_and_takeownership(tolua_S,tolua_obj,"',t,'");')
	 output('#endif\n')
	 output('   }')
      elseif self.ptr == '&' then
	 output('   tolua_pushusertype(tolua_S,(void*)&tolua_ret,"',t,'");')
      else
	 if local_constructor then
	    output('   tolua_pushusertype_and_takeownership(tolua_S,(void *)tolua_ret,"',t,'");')
	 else
	    output('   tolua_pushusertype(tolua_S,(void*)tolua_ret,"',t,'");')
	 end
      end
   end
   
   output('  }')
   output(' return 1;')
   
   output('#ifndef TOLUA_RELEASE\n')
   output('tolua_lerror:\n')
   output(' tolua_error(tolua_S,"#ferror in function \''..self.lname..'\'.",&tolua_err);')
   output(' return 0;')
   output('#endif\n')

   output('}')
   output('#endif //#ifndef TOLUA_DISABLE\n')
   output('\n')
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
   
   --t.name = t.name .. "_" .. (_TM[t.kind] or t.kind)
   t.cname = t:cfuncname("tolua")..t:overload(t)
   t.name = "operator" .. t.kind  -- set appropriate calling name
   return t
end

-- Constructor
function Operator (d,k,a,c,r)
   r=r or ""

   local op_k = string.gsub(k, "^%s*", "")
   op_k = string.gsub(k, "%s*$", "")
   --if string.find(k, "^[%w_:%d<>%*%&]+$") then
   if d == "operator" and k ~= '' then
      d = k.." operator"
   -- --elseif not _TM[op_k] then
   -- ++ <<
   elseif not _TM[op_k] and not _TM0[op_k] then
   -- ++ >>
      if flags['W'] then
	 --error("tolua: no support for operator" .. f.kind)
	 error("tolua: no support for operator" .. op_k)
      else
	 warning("No support for operator "..op_k..", ignoring")
	 return nil
      end
   end
   
   local ref = ''
   local t = split_c_tokens(strsub(a,2,strlen(a)-1),',') -- eliminate braces
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
   -- -- <<
   -- -- if k == '[]' and (l[1]==nil or isbasic(l[1].type)~='number') then
   -- --   error('operator[] can only be defined for numeric index.')
   -- -- end
   -- -- >>
   -- ++ <<
   if k == '[]' and (l[1]==nil or (isbasic(l[1].type)~='number' and isbasic(l[1].type)~='string' and isbasic(l[1].type)~='cppstring')) then
      error('type=='..isbasic(l[1].type)..'; operator[] can only be defined for numeric and string index.')
   end
   -- ++ >>
   f.args = l
   f.const = c
   f.rets = Declaration(f.type..r,'var',true)
   f.kind = op_k
   -- --f.lname = "."..(_TM0[f.kind] or f.kind)
   -- ++ <<
   if #t[1]>0 then
      if f.kind=="[]" or f.kind=="&[]" then
	 local tp="."
	 if f.kind=="&[]" then tp=tp.."set"
	 elseif f.kind=="[]" then tp=tp.."get" end
	 if isbasic(l[1].type)=='number' then
	    f.lname = tp.."i"
	 elseif isbasic(l[1].type)=='string' or isbasic(l[1].type)=='cppstring' then
	    f.lname = tp.."s"
	 end
	 f.last_overload_error=false
      else
	 f.lname = "."..(_TM[f.kind] or f.kind)
      end
   else
      f.lname = "."..(_TM0[f.kind] or f.kind)
   end
   -- ++ >>
   
   -- -- if not _TM[f.kind] then
   -- ++ <<
   if not _TM[f.kind] and not _TM0[f.kind] then
   -- ++ >>
      f.cast_operator = true
   end
   if f.kind == '[]' and ref=='&' and f.const~='const' then
      Operator(d,'&'..k,a,c) 	-- create correspoding set operator
   end
   
   return _Operator(f)
end


