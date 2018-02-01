local mesh_name = {
	[0] = "X cross",
	"+ cross",
	"* star",
	"# hash",
	"# hash-outwards",
}

local option_name = {
	"offset",
	"scale",
	"yoffset",
}

local function listoptions(bitfield, names)
	local result = {}
	for _, name in ipairs(names) do
		local val = bitfield % 2
		if val ~= 0 then
			result[#result + 1] = name
		end
		bitfield = (bitfield - val) / 2
	end
	return result
end

local function mutate(itemstack, user, pointed_thing, add)
	if pointed_thing.type ~= "node" then return end
	local pos = pointed_thing.under
	local node = minetest.get_node(pos)
	local ndef = minetest.registered_nodes[node.name]
	if not ndef then return end

	if ndef.paramtype2 == "degrotate" then
		node.param2 = (node.param2 + add + 256) % 256
	elseif ndef.paramtype2 == "leveled" then
		local val = node.param2 + add
		if val < 0 then return end
		if val >= 256 then return end
		node.param2 = val
	elseif ndef.paramtype2 == "meshoptions" then
		local val = node.param2
		local mesh = val % 8
		local options = (val - mesh) / 8
		if add > 0 then
			mesh = (mesh + 1) % 5
			msg = "New plant mesh: " .. mesh_name[mesh]
		else
			options = (options + 1) % 8
			msg = "New plant options: " .. table.concat(listoptions(options, option_name), ", ")
		end
		minetest.chat_send_player(user:get_player_name(), msg)
		node.param2 = options * 8 + mesh
	else
		return
	end
	minetest.swap_node(pos, node)
end

minetest.register_craftitem("tools:mutator", {
	description = "Plant mutator",
	inventory_image = "mutator.png",
	on_use = function(itemstack, user, pointed_thing)
		mutate(itemstack, user, pointed_thing, 1)
		return itemstack
	end,
	on_place = function(itemstack, user, pointed_thing)
		mutate(itemstack, user, pointed_thing, -1)
		return itemstack
	end,
})
