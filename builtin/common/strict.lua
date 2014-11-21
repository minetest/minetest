
-- Always warn when creating a global variable, even outside of a function.
-- This ignores mod namespaces (variables with the same name as the current mod).
local WARN_INIT = false


function core.global_exists(name)
	return rawget(_G, name) ~= nil
end


local function warn(message)
	print(os.date("%H:%M:%S: WARNING: ")..message)
end


local meta = {}
local declared = {}
-- Key is source file, line, and variable name; seperated by NULs
local warned = {}

function meta:__newindex(name, value)
	local info = debug.getinfo(2, "Sl")
	local desc = ("%s:%d"):format(info.short_src, info.currentline)
	if not declared[name] then
		local warn_key = ("%s\0%d\0%s"):format(info.source,
				info.currentline, name)
		if not warned[warn_key] and info.what ~= "main" and
				info.what ~= "C" then
			warn(("Assignment to undeclared "..
					"global %q inside a function at %s.")
				:format(name, desc))
			warned[warn_key] = true
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
	local warn_key = ("%s\0%d\0%s"):format(info.source, info.currentline, name)
	if not declared[name] and not warned[warn_key] and info.what ~= "C" then
		warn(("Undeclared global variable %q accessed at %s:%s")
				:format(name, info.short_src, info.currentline))
		warned[warn_key] = true
	end
	return rawget(self, name)
end

setmetatable(_G, meta)

