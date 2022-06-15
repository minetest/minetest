-- Minetest: builtin/map.lua

-- Internal interfaces
local get_node_raw = core.get_node_raw
core.get_node_raw = nil

function core.get_node(pos)
	return core.get_node_or_nil(pos) or { name = "ignore", param1 = 0, param2 = 0 }
end

function core.get_node_or_nil(pos)
	local param0, param1, param2, pos_ok = get_node_raw(pos.x, pos.y, pos.z)
	if not pos_ok then
		return nil
	end
	local name = core.get_name_from_content_id(param0)
	return { name = name, param1 = param1, param2 = param2 }
end
