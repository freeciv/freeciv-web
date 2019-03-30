-- tolua: abstract feature class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: feature.lua,v 1.3 2009/11/24 16:45:14 fabraham Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications. 


-- Feature class
-- Represents the base class of all mapped feature.
classFeature = {
}
classFeature.__index = classFeature

-- write support code
function classFeature:supcode ()
end

-- output tag
function classFeature:decltype ()
end

-- register feature
function classFeature:register ()
end

-- translate verbatim
function classFeature:preamble ()
end

-- check if it is a variable
function classFeature:isvariable ()
 return false
end

-- checi if it requires collection
function classFeature:requirecollection (t)
 return false
end

-- build names
function classFeature:buildnames ()
 if self.name and self.name~='' then
  local n = split(self.name,'@')
  self.name = n[1]
		if not n[2] then
		 n[2] = applyrenaming(n[1])
		end
  self.lname = n[2] or gsub(n[1],"%[.-%]","")
 end
 self.name = getonlynamespace() .. self.name
end


-- check if feature is inside a container definition
-- it returns the container class name or nil.
function classFeature:incontainer (which)
 if self.parent then
  local parent = self.parent
  while parent do
   if parent.classtype == which then
    return parent.name
   end
   parent = parent.parent
  end
 end
 return nil
end

function classFeature:inclass ()
 return self:incontainer('class')
end

function classFeature:inmodule ()
 return self:incontainer('module')
end

function classFeature:innamespace ()
 return self:incontainer('namespace')
end

-- return C binding function name based on name
-- the client specifies a prefix
function classFeature:cfuncname (n)
 if self.parent then
  n = self.parent:cfuncname(n)
 end

 if self.lname and 
	   strsub(self.lname,1,1)~="."  -- operator are named as ".add"
	then
  return n..'_'..self.lname
 else
  return n..'_'..self.name
 end
end

