-- Generate binding code
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- Last update: Apr 2003
-- $Id: doit.lua,v 1.3 2009/11/24 16:45:13 fabraham Exp $


-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications. 

function doit ()
	-- define package name, if not provided
	if not flags.n then
		if flags.f then
			flags.n = gsub(flags.f,"%..*","")
		else
			error("#no package name nor input file provided")
		end
	end

	-- proccess package
	local p  = Package(flags.n,flags.f)

	if flags.p then
		return        -- only parse
	end

	if flags.o then
		local st,msg = writeto(flags.o)
		if not st then
			error('#'..msg)
		end
	end

	p:decltype()
	if flags.P then
		p:print()
	else
		p:preamble()
		p:supcode()
		p:register()
	end

	if flags.o then
		writeto()
	end

	-- write header file
	if not flags.P then
		if flags.H then
			local st,msg = writeto(flags.H)
			if not st then
				error('#'..msg)
			end
			p:header()
			writeto()
		end
	end
end

