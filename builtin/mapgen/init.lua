local gamepath = core.get_builtin_path() .. "game" .. DIR_DELIM
local commonpath = core.get_builtin_path() .. "common" .. DIR_DELIM
local mgpath = core.get_builtin_path() .. "mapgen" .. DIR_DELIM

local builtin_shared = {}

-- Import parts shared with "game" environment
dofile(gamepath .. "constants.lua")
assert(loadfile(commonpath .. "item_s.lua"))(builtin_shared)
dofile(gamepath .. "misc_s.lua")
dofile(gamepath .. "features.lua")
dofile(gamepath .. "voxelarea.lua")

-- Now for our own stuff
assert(loadfile(commonpath .. "register.lua"))(builtin_shared)
assert(loadfile(mgpath .. "register.lua"))(builtin_shared)
dofile(mgpath .. "example.lua")

builtin_shared.cache_content_ids()

core.log("info", "Initialized mapgen Lua environment")
