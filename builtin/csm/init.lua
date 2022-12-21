local scriptpath = core.get_builtin_path()
local csmpath = scriptpath.."csm"..DIR_DELIM
local commonpath = scriptpath.."common"..DIR_DELIM

dofile(csmpath .. "register.lua")
dofile(commonpath .. "after.lua")
assert(loadfile(commonpath .. "item_s.lua"))({})
