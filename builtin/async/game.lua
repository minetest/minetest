core.log("info", "Initializing asynchronous environment (game)")

local function pack2(...)
	return {n=select('#', ...), ...}
end

-- Entrypoint to run async jobs, called by C++
function core.job_processor(func, params)
	local retval = pack2(func(unpack(params, 1, params.n)))

	return retval
end

-- Import a bunch of individual files from builtin/game/
local gamepath = core.get_builtin_path() .. "game" .. DIR_DELIM

dofile(gamepath .. "constants.lua")
dofile(gamepath .. "item_s.lua")
dofile(gamepath .. "misc_s.lua")
dofile(gamepath .. "features.lua")
dofile(gamepath .. "voxelarea.lua")

-- Transfer of globals
do
	local all = assert(core.transferred_globals)
	core.transferred_globals = nil

	-- reassemble other tables
	all.registered_nodes = {}
	all.registered_craftitems = {}
	all.registered_tools = {}
	for k, v in pairs(all.registered_items) do
		if v.type == "node" then
			all.registered_nodes[k] = v
		elseif v.type == "craftitem" then
			all.registered_craftitems[k] = v
		elseif v.type == "tool" then
			all.registered_tools[k] = v
		end
	end

	for k, v in pairs(all) do
		core[k] = v
	end
end
