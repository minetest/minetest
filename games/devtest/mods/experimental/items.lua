minetest.register_node("experimental:callback_node", {
	description = "Callback Test Node (construct/destruct/timer)",
	tiles = {"experimental_callback_node.png"},
	groups = {dig_immediate=3},
	-- This was known to cause a bug in minetest.item_place_node() when used
	-- via minetest.place_node(), causing a placer with no position
	paramtype2 = "facedir",
	drop = "",

	on_construct = function(pos)
		experimental.print_to_everything("experimental:callback_node:on_construct("..minetest.pos_to_string(pos)..")")
		local meta = minetest.get_meta(pos)
		meta:set_string("mine", "test")
		local timer = minetest.get_node_timer(pos)
		timer:start(4, 3)
	end,

	after_place_node = function(pos, placer)
		experimental.print_to_everything("experimental:callback_node:after_place_node("..minetest.pos_to_string(pos)..")")
		local meta = minetest.get_meta(pos)
		if meta:get_string("mine") == "test" then
			experimental.print_to_everything("correct metadata found")
		else
			experimental.print_to_everything("incorrect metadata found")
		end
	end,

	on_destruct = function(pos)
		experimental.print_to_everything("experimental:callback_node:on_destruct("..minetest.pos_to_string(pos)..")")
	end,

	after_destruct = function(pos)
		experimental.print_to_everything("experimental:callback_node:after_destruct("..minetest.pos_to_string(pos)..")")
	end,

	after_dig_node = function(pos, oldnode, oldmetadata, digger)
		experimental.print_to_everything("experimental:callback_node:after_dig_node("..minetest.pos_to_string(pos)..")")
	end,

	on_timer = function(pos, elapsed)
		experimental.print_to_everything("on_timer(): elapsed="..dump(elapsed))
		return true
	end,
})

minetest.register_tool("experimental:privatizer", {
	description = "Node Meta Privatizer".."\n"..
		"Punch: Marks 'infotext' and 'formspec' meta fields of chest as private",
	inventory_image = "experimental_tester_tool_1.png",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		if pointed_thing.type == "node" then
			local node = minetest.get_node(pointed_thing.under)
			if node.name == "chest:chest" then
				local p = pointed_thing.under
				minetest.log("action", "Privatizer used at "..minetest.pos_to_string(p))
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

minetest.register_tool("experimental:particle_spawner", {
	description = "Particle Spawner".."\n"..
		"Punch: Spawn random test particle",
	inventory_image = "experimental_tester_tool_1.png^[invert:g",
	groups = { testtool = 1, disable_repair = 1 },
	on_use = function(itemstack, user, pointed_thing)
		local pos = minetest.get_pointed_thing_position(pointed_thing, true)
		if pos == nil then
			if user then
				pos = user:get_pos()
			end
		end
		pos = vector.add(pos, {x=0, y=0.5, z=0})
		local tex, anim
		if math.random(0, 1) == 0 then
			tex = "experimental_particle_sheet.png"
			anim = {type="sheet_2d", frames_w=3, frames_h=2, frame_length=0.5}
		else
			tex = "experimental_particle_vertical.png"
			anim = {type="vertical_frames", aspect_w=16, aspect_h=16, length=3.3}
		end

		minetest.add_particle({
			pos = pos,
			velocity = {x=0, y=0, z=0},
			acceleration = {x=0, y=0.04, z=0},
			expirationtime = 6,
			collisiondetection = true,
			texture = tex,
			animation = anim,
			size = 4,
			glow = math.random(0, 5),
		})
	end,
})

