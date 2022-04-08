core.log("info", "Initializing asynchronous environment (game)")

function core.job_processor(func, params)
	local retval = {func(unpack(params))}

	return retval
end

-- Import a bunch of individual files from builtin/game/
local gamepath = core.get_builtin_path() .. "game" .. DIR_DELIM

dofile(gamepath .. "constants.lua")
-- TODO item.lua has some
-- TODO misc.lua has some
dofile(gamepath .. "voxelarea.lua")
