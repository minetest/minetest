
local S = minetest.get_translator("testtools")

minetest.register_tool("testtools:lighttool", {
	description = S("Light tool"),
	inventory_image = "testtools_lighttool.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		local pos = pointed_thing.above
		if pointed_thing.type ~= "node" or not pos then
			return
		end

		local node = minetest.get_node(pos)
		local time = minetest.get_timeofday()
		local sunlight = minetest.get_natural_light(pos)
		local artificial = minetest.get_artificial_light(node.param1)
		local message = ("param1 0x%02x | time %.5f | sunlight %d | artificial %d")
				:format(node.param1, time, sunlight, artificial)
		minetest.chat_send_player(user:get_player_name(), message)
	end
})
