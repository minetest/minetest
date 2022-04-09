core.log("info", "Initializing asynchronous environment (game)")

local function pack2(...)
	return {n=select('#', ...), ...}
end

function core.job_processor(func, params)
	local retval = pack2(func(unpack(params, 1, params.n)))

	return retval
end

-- Import a bunch of individual files from builtin/game/
local gamepath = core.get_builtin_path() .. "game" .. DIR_DELIM

dofile(gamepath .. "constants.lua")
-- TODO item.lua has some
-- TODO misc.lua has some
dofile(gamepath .. "voxelarea.lua")
