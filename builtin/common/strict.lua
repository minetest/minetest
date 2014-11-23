
-- Always warn when creating a global variable, even outside of a function.
-- This ignores mod namespaces (variables with the same name as the current mod).
local WARN_INIT = false


local function warn(message)
	print(os.date("%H:%M:%S: WARNING: ")..message)
end


local meta = {}
local declared = {}
local alreadywarned = {}

function meta:__newindex(name, value)
	local info = debug.getinfo(2, "Sl")
	local desc = ("%s:%d"):format(info.short_src, info.currentline)
	if not declared[name] then
		if info.what ~= "main" and info.what ~= "C" then
			warn(("Assignment to undeclared global %q inside"
					.." a function at %s.")
				:format(name, desc))
		end
		declared[name] = true
	end
	-- Ignore mod namespaces
	if WARN_INIT and (not core.get_current_modname or
			name ~= core.get_current_modname()) then
		warn(("Global variable %q created at %s.")
			:format(name, desc))
	end
	rawset(self, name, value)
end


function meta:__index(name)
	local info = debug.getinfo(2, "Sl")
	if not declared[name] and info.what ~= "C" and not alreadywarned[name] then
		warn(("Undeclared global variable %q accessed at %s:%s")
				:format(name, info.short_src, info.currentline))
		alreadywarned[name] = true
	end
	return rawget(self, name)
end

setmetatable(_G, meta)

