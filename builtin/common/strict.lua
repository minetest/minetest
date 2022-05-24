local getinfo = debug.getinfo

function core.global_exists(name)
	if type(name) ~= "string" then
		error("core.global_exists: " .. tostring(name) .. " is not a string")
	end
	return rawget(_G, name) ~= nil
end


local meta = {}
local declared = {}
-- Key is source file, line, and variable name; seperated by NULs
local warned = {}

function meta:__newindex(name, value)
	local info = getinfo(2, "Sl")
	local desc = ("%s:%d"):format(info.short_src, info.currentline)
	if not declared[name] then
		local warn_key = ("%s\0%d\0%s"):format(info.source,
				info.currentline, name)
		if not warned[warn_key] and info.what ~= "main" and
				info.what ~= "C" then
			core.log("warning", ("Assignment to undeclared "..
					"global %q inside a function at %s.")
				:format(name, desc))
			warned[warn_key] = true
		end
		declared[name] = true
	end
	rawset(self, name, value)
end


function meta:__index(name)
	local info = getinfo(2, "Sl")
	local warn_key = ("%s\0%d\0%s"):format(info.source, info.currentline, name)
	if not declared[name] and not warned[warn_key] and info.what ~= "C" then
		core.log("warning", ("Undeclared global variable %q accessed at %s:%s")
				:format(name, info.short_src, info.currentline))
		warned[warn_key] = true
	end
	return rawget(self, name)
end

setmetatable(_G, meta)
