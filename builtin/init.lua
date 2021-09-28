--
-- This file contains built-in stuff in Minetest implemented in Lua.
--
-- It is always loaded and executed after registration of the C API,
-- before loading and running any mods.
--

-- Initialize some very basic things
function core.debug(...) core.log(table.concat({...}, "\t")) end
if core.print then
	local core_print = core.print
	-- Override native print and use
	-- terminal if that's turned on
	function print(...)
		local n, t = select("#", ...), {...}
		for i = 1, n do
			t[i] = tostring(t[i])
		end
		core_print(table.concat(t, "\t"))
	end
	core.print = nil -- don't pollute our namespace
end
math.randomseed(os.time())
minetest = core

-- Load other files
local scriptdir = core.get_builtin_path()
local gamepath = scriptdir .. "game" .. DIR_DELIM
local clientpath = scriptdir .. "client" .. DIR_DELIM
local commonpath = scriptdir .. "common" .. DIR_DELIM
local asyncpath = scriptdir .. "async" .. DIR_DELIM

dofile(commonpath .. "vector.lua")
dofile(commonpath .. "strict.lua")
dofile(commonpath .. "serialize.lua")
dofile(commonpath .. "misc_helpers.lua")

if INIT == "game" then
	dofile(gamepath .. "init.lua")
	assert(not core.get_http_api)
elseif INIT == "mainmenu" then
	local mm_script = core.settings:get("main_menu_script")
	local custom_loaded = false
	if mm_script and mm_script ~= "" then
		local testfile = io.open(mm_script, "r")
		if testfile then
			testfile:close()
			dofile(mm_script)
			custom_loaded = true
			core.log("info", "Loaded custom main menu script: "..mm_script)
		else
			core.log("error", "Failed to load custom main menu script: "..mm_script)
			core.log("info", "Falling back to default main menu script")
		end
	end
	if not custom_loaded then
		dofile(core.get_mainmenu_path() .. DIR_DELIM .. "init.lua")
	end
elseif INIT == "async" then
	dofile(asyncpath .. "init.lua")
elseif INIT == "client" then
	dofile(clientpath .. "init.lua")
else
	error(("Unrecognized builtin initialization type %s!"):format(tostring(INIT)))
end
