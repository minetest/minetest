--[[
-- Lua 5.1 -> 5.2 compatibility
-- This is run with LuaJIT too
--]]

-- unpack was moved into the table library.
-- But LuaJIT without LUAJIT_ENABLE_LUA52COMPAT doesn't have unpack moved.
table.unpack = table.unpack or unpack
function unpack(...)
	minetest.log("deprecated", "unpack has been moved into the table library")
	return table.unpack(...)
end


-- loadstring() was deprecated (load() has all of it's features now)
function loadstring(str, chunk_name)
	minetest.log("deprecated", "loadstring is deprecated, use load instead")
	return load(str, chunk_name)
end


-- package.loaders was renamed to package.searchers.
-- But not in LuaJIT.
package.searchers = package.searchers or package.loaders
package.loaders = {}
local loader_meta = {}
function loader_meta:__index(k)
	minetest.log("deprecated", "package.loaders has been renamed to package.searchers")
	return package.searchers[k]
end
function loader_meta:__newindex(k, v)
	minetest.log("deprecated", "package.loaders has been renamed to package.searchers")
	package.searchers[k] = v
end
setmetatable(package.loaders, loader_meta)


-- table.maxn was removed
function table.maxn(t)
	minetest.log("deprecated", "table.maxn is deprecated")
	local max = 0
	for i, _ in pairs(t) do
		if type(i) == "number" and i > max then
			max = i
		end
	end
	return max
end


-- getfenv/setfenv were removed
local function fenvwarn()
	-- LuaJIT doesn't support _ENV yet
	minetest.log("deprecated",
		"getfenv/setfenv are deprecated.  Use load()'s env argument.")
end

if rawget(_G, "setfenv") and rawget(_G, "getfenv") then  -- LuaJIT
	local raw_setfenv = setfenv
	function setfenv(f, env)
		fenvwarn()
		return raw_setfenv(f, env)
	end
	local raw_getfenv = getfenv
	function getfenv(f)
		fenvwarn()
		return raw_getfenv(f)
	end
else
	local function envlookup(f)
		local name, val
		local up = 0
		local unknown
		repeat
			up = up + 1
			name, val = debug.getupvalue(f, up)
			if name == "" then
				unknown = true
			end
		until name == "_ENV" or name == nil
		if name ~= "_ENV" then
			up = nil
			if unknown then
				error("Upvalues are not readable in Lua 5.2 when debug info is missing", 3)
			end
		end
		return (name == "_ENV") and up, val, unknown
	end

	local function envhelper(f, name)
		local tp = type(f)
		if tp == "number" then
			if f < 0 then
				error(("Bad argument #1 to '%s' (level must be non-negative)")
					:format(name), 3)
			elseif f < 1 then
				error("Thread environments are not supported in Lua 5.2", 3)
			end
			f = debug.getinfo(f + 2, "f").func
		elseif tp ~= "function" then
			error(("Bad argument #1 to '%s' (number expected, got %s)")
				:format(name, tp), 2)
		end
		return f
	end

	function setfenv(f, t)
		fenvwarn()
		local f = envhelper(f, "setfenv")
		local up, val, unknown = envlookup(f)
		if up then
			debug.upvaluejoin(f, up, function() return up end, 1)
			debug.setupvalue(f, up, t)
		else
			local what = debug.getinfo(f, "S").what
			if what ~= "Lua" and what ~= "main" then
				error("Can't set function environment of "..what, 2)
			end
		end
		return f
	end

	function getfenv(f)
		fenvwarn()
		if f == 0 or f == nil then
			return _G
		end
		local f = envhelper(f, "getfenv")
		local up, val = envlookup(f)
		if not up then
			return _G
		end
		return val
	end
end

