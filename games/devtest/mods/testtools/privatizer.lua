minetest.register_tool("testtools:privatizer", {
	description = "Node Meta Privatizer".."\n"..
		"Punch: Marks 'infotext' and 'formspec' meta fields of chest as private",
	inventory_image = "testtools_privatizer.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		if pointed_thing.type == "node" then
			local node = minetest.get_node(pointed_thing.under)
			if node.name == "chest:chest" then
				local p = pointed_thing.under
				minetest.log("action", "[testtools] Privatizer used at "..minetest.pos_to_string(p))
				minetest.get_meta(p):mark_as_private({"infotext", "formspec"})
				if user and user:is_player() then
					minetest.chat_send_player(user:get_player_name(), "Chest metadata (infotext, formspec) set private!")
				end
				return
			end
		end
		if user and user:is_player() then
			minetest.chat_send_player(user:get_player_name(), "Privatizer can only be used on chest!")
		end
	end,
})

