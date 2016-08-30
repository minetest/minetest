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
os.setlocale("C", "numeric")
minetest = core

-- Load other files
local script_dir = core.get_builtin_path()
local game_path = script_dir .. "/game"
local common_path = script_dir .. "/common"
local async_path = script_dir .. "/async"

dofile(common_path .. "/strict.lua")
dofile(common_path .. "/serialize.lua")
dofile(common_path .. "/misc_helpers.lua")

if INIT == "game" then
	dofile(game_path .. "/init.lua")
elseif INIT == "mainmenu" then
	local main_menu_script = core.setting_get("main_menu_script")
	if main_menu_script ~= nil and main_menu_script ~= "" then
		dofile(main_menu_script)
	else
		dofile(core.get_mainmenu_path() .. "/init.lua")
	end
elseif INIT == "async" then
	dofile(async_path .. "/init.lua")
else
	error(("Unrecognized builtin initialization type %s!"):format(tostring(INIT)))
end

