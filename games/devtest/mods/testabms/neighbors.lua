-- test ABMs with neighbor and without_neighbor

local S = minetest.get_translator("testabms")

-- ABM required neighboor
minetest.register_node("testabms:required_neighbor", {
	description = S("Node for test ABM required_neighbor."),
	drawtype = "normal",
	tiles = { "testabms_wait_node.png" },

	groups = { dig_immediate = 3 },
	
	on_construct = function (pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("infotext", "Waiting for ABM testabms:required_neighbor (normal drawtype testnode sensitive)")
	end,
})

minetest.register_abm({
		label = "testabms:required_neighbor",
		nodenames = "testabms:required_neighbor",
		neighbors = {"testnodes:normal"},
		interval = 1,
		chance = 1,
		action = function (pos)
			minetest.swap_node(pos, {name="testabms:after_abm"})
			local meta = minetest.get_meta(pos)
			meta:set_string("infotext", "ABM testabsm:required_neighbor changed this node.")
		end
	})
