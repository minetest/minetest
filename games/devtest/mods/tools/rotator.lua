local function rotate(itemstack, user, pointed_thing, dir)
	if pointed_thing.type ~= "node" then return end
	local pos = pointed_thing.under
	local node = minetest.get_node(pos)
	local ndef = minetest.registered_nodes[node.name]
	if not ndef then return end
	if ndef.paramtype2 ~= "facedir" and ndef.paramtype2 ~= "colorfacedir" then return end

	local facedir = node.param2 % 32
	local data = node.param2 - facedir
	local rotation = facedir % 4
	local direction = (facedir - rotation) / 4
	if dir then
		direction = (direction + 1) % 6
	else
		rotation = (rotation + 1) % 4
	end
	facedir = 4 * direction + rotation
	node.param2 = data + facedir
	minetest.swap_node(pos, node)
end

minetest.register_craftitem("tools:rotator", {
	description = "Node rotator",
	inventory_image = "rotator.png",
	on_use = function(itemstack, user, pointed_thing)
		rotate(itemstack, user, pointed_thing, false)
		return itemstack
	end,
	on_place = function(itemstack, user, pointed_thing)
		rotate(itemstack, user, pointed_thing, true)
		return itemstack
	end,
})
