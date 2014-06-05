--
-- This file contains built-in stuff in Minetest implemented in Lua.
--
-- It is always loaded and executed after registration of the C API,
-- before loading and running any mods.
--

-- Initialize some very basic things
print = core.debug
math.randomseed(os.time())
os.setlocale("C", "numeric")
minetest = core

-- Load other files
local scriptdir = core.get_builtin_path()..DIR_DELIM
local gamepath = scriptdir.."game"..DIR_DELIM
local commonpath = scriptdir.."common"..DIR_DELIM
local asyncpath = scriptdir.."async"..DIR_DELIM

dofile(commonpath.."strict.lua")
dofile(commonpath.."serialize.lua")
dofile(commonpath.."misc_helpers.lua")

if INIT == "game" then
	dofile(gamepath.."init.lua")
elseif INIT == "mainmenu" then
	local mainmenuscript = core.setting_get("main_menu_script")
	if mainmenuscript ~= nil and mainmenuscript ~= "" then
		dofile(mainmenuscript)
	else
		dofile(core.get_mainmenu_path()..DIR_DELIM.."init.lua")
	end
elseif INIT == "async" then
	dofile(asyncpath.."init.lua")
else
	error(("Unrecognized builtin initialization type %s!"):format(tostring(INIT)))
end

