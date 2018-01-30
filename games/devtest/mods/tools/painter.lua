local color_count = {
	color = 256,
	colorfacedir = 8,
	colorwallmounted = 32,
}

local function paint(itemstack, user, pointed_thing, add)
	if pointed_thing.type ~= "node" then return end
	local pos = pointed_thing.under
	local node = minetest.get_node(pos)
	local ndef = minetest.registered_nodes[node.name]
	if not ndef then return end
	local max = color_count[ndef.paramtype2]
	if not max then return end
	local datasize = 256 / max

	local data = node.param2 % datasize
	local color = (node.param2 - data) / datasize
	color = (color + max + add) % max
	node.param2 = data + color * datasize
	minetest.swap_node(pos, node)
end

minetest.register_craftitem("tools:painter", {
	description = "Node painter",
	inventory_image = "painter.png",
	on_use = function(itemstack, user, pointed_thing)
		paint(itemstack, user, pointed_thing, 1)
		return itemstack
	end,
	on_place = function(itemstack, user, pointed_thing)
		paint(itemstack, user, pointed_thing, -1)
		return itemstack
	end,
})
