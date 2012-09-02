-- Minetest: builtin/item.lua

--
-- Item definition helpers
--

function minetest.inventorycube(img1, img2, img3)
	img2 = img2 or img1
	img3 = img3 or img1
	return "[inventorycube"
			.. "{" .. img1:gsub("%^", "&")
			.. "{" .. img2:gsub("%^", "&")
			.. "{" .. img3:gsub("%^", "&")
end

function minetest.get_pointed_thing_position(pointed_thing, above)
	if pointed_thing.type == "node" then
		if above then
			-- The position where a node would be placed
			return pointed_thing.above
		else
			-- The position where a node would be dug
			return pointed_thing.under
		end
	elseif pointed_thing.type == "object" then
		obj = pointed_thing.ref
		if obj ~= nil then
			return obj:getpos()
		else
			return nil
		end
	else
		return nil
	end
end

function minetest.dir_to_facedir(dir)
	if math.abs(dir.x) > math.abs(dir.z) then
		if dir.x < 0 then
			return 3
		else
			return 1
		end
	else
		if dir.z < 0 then
			return 2
		else
			return 0
		end
	end
end

function minetest.dir_to_wallmounted(dir)
	if math.abs(dir.y) > math.max(math.abs(dir.x), math.abs(dir.z)) then
		if dir.y < 0 then
			return 1
		else
			return 0
		end
	elseif math.abs(dir.x) > math.abs(dir.z) then
		if dir.x < 0 then
			return 3
		else
			return 2
		end
	else
		if dir.z < 0 then
			return 5
		else
			return 4
		end
	end
end

function minetest.get_node_drops(nodename, toolname)
	local drop = ItemStack({name=nodename}):get_definition().drop
	if drop == nil then
		-- default drop
		return {ItemStack({name=nodename})}
	elseif type(drop) == "string" then
		-- itemstring drop
		return {ItemStack(drop)}
	elseif drop.items == nil then
		-- drop = {} to disable default drop
		return {}
	end

	-- Extended drop table
	local got_items = {}
	local got_count = 0
	local _, item, tool
	for _, item in ipairs(drop.items) do
		local good_rarity = true
		local good_tool = true
		if item.rarity ~= nil then
			good_rarity = item.rarity < 1 or math.random(item.rarity) == 1
		end
		if item.tools ~= nil then
			good_tool = false
			for _, tool in ipairs(item.tools) do
				if tool:sub(1, 1) == '~' then
					good_tool = toolname:find(tool:sub(2)) ~= nil
				else
					good_tool = toolname == tool
				end
				if good_tool then
					break
				end
			end
        	end
		if good_rarity and good_tool then
			got_count = got_count + 1
			for _, add_item in ipairs(item.items) do
				got_items[#got_items+1] = add_item
			end
			if drop.max_items ~= nil and got_count == drop.max_items then
				break
			end
		end
	end
	return got_items
end

function minetest.item_place_node(itemstack, placer, pointed_thing)
	local item = itemstack:peek_item()
	local def = itemstack:get_definition()
	if def.type ~= "node" or pointed_thing.type ~= "node" then
		return itemstack
	end

	local under = pointed_thing.under
	local oldnode_under = minetest.env:get_node(under)
	local olddef_under = ItemStack({name=oldnode_under.name}):get_definition()
	olddef_under = olddef_under or minetest.nodedef_default
	local above = pointed_thing.above
	local oldnode_above = minetest.env:get_node(above)
	local olddef_above = ItemStack({name=oldnode_above.name}):get_definition()
	olddef_above = olddef_above or minetest.nodedef_default

	if not olddef_above.buildable_to and not olddef_under.buildable_to then
		minetest.log("info", placer:get_player_name() .. " tried to place"
			.. " node in invalid position " .. minetest.pos_to_string(above)
			.. ", replacing " .. oldnode_above.name)
		return
	end

	-- Place above pointed node
	local place_to = {x = above.x, y = above.y, z = above.z}

	-- If node under is buildable_to, place into it instead (eg. snow)
	if olddef_under.buildable_to then
		minetest.log("info", "node under is buildable to")
		place_to = {x = under.x, y = under.y, z = under.z}
	end

	minetest.log("action", placer:get_player_name() .. " places node "
		.. def.name .. " at " .. minetest.pos_to_string(place_to))
	
	local oldnode = minetest.env:get_node(place_to)
	local newnode = {name = def.name, param1 = 0, param2 = 0}

	-- Calculate direction for wall mounted stuff like torches and signs
	if def.paramtype2 == 'wallmounted' then
		local dir = {
			x = under.x - above.x,
			y = under.y - above.y,
			z = under.z - above.z
		}
		newnode.param2 = minetest.dir_to_wallmounted(dir)
	-- Calculate the direction for furnaces and chests and stuff
	elseif def.paramtype2 == 'facedir' then
		local placer_pos = placer:getpos()
		if placer_pos then
			local dir = {
				x = above.x - placer_pos.x,
				y = above.y - placer_pos.y,
				z = above.z - placer_pos.z
			}
			newnode.param2 = minetest.dir_to_facedir(dir)
			minetest.log("action", "facedir: " .. newnode.param2)
		end
	end

	-- Add node and update
	minetest.env:add_node(place_to, newnode)

	-- Run callback
	if def.after_place_node then
		-- Copy place_to because callback can modify it
		local place_to_copy = {x=place_to.x, y=place_to.y, z=place_to.z}
		def.after_place_node(place_to_copy, placer)
	end

	-- Run script hook
	local _, callback
	for _, callback in ipairs(minetest.registered_on_placenodes) do
		-- Copy pos and node because callback can modify them
		local place_to_copy = {x=place_to.x, y=place_to.y, z=place_to.z}
		local newnode_copy = {name=newnode.name, param1=newnode.param1, param2=newnode.param2}
		local oldnode_copy = {name=oldnode.name, param1=oldnode.param1, param2=oldnode.param2}
		callback(place_to_copy, newnode_copy, placer, oldnode_copy)
	end

	itemstack:take_item()
	return itemstack
end

function minetest.item_place_object(itemstack, placer, pointed_thing)
	local pos = minetest.get_pointed_thing_position(pointed_thing, true)
	if pos ~= nil then
		local item = itemstack:take_item()
		minetest.env:add_item(pos, item)
	end
	return itemstack
end

function minetest.item_place(itemstack, placer, pointed_thing)
	if itemstack:get_definition().type == "node" then
		return minetest.item_place_node(itemstack, placer, pointed_thing)
	else
		return minetest.item_place_object(itemstack, placer, pointed_thing)
	end
end

function minetest.item_drop(itemstack, dropper, pos)
	if dropper.get_player_name then
		local v = dropper:get_look_dir()
		local p = {x=pos.x+v.x, y=pos.y+1.5+v.y, z=pos.z+v.z}
		local obj = minetest.env:add_item(p, itemstack)
		if obj then
			v.x = v.x*2
			v.y = v.y*2 + 1
			v.z = v.z*2
			obj:setvelocity(v)
		end
	else
		minetest.env:add_item(pos, itemstack)
	end
	return ""
end

function minetest.item_eat(hp_change, replace_with_item)
	return function(itemstack, user, pointed_thing)  -- closure
		if itemstack:take_item() ~= nil then
			user:set_hp(user:get_hp() + hp_change)
			itemstack:add_item(replace_with_item) -- note: replace_with_item is optional
		end
		return itemstack
	end
end

function minetest.node_punch(pos, node, puncher)
	-- Run script hook
	local _, callback
	for _, callback in ipairs(minetest.registered_on_punchnodes) do
		-- Copy pos and node because callback can modify them
		local pos_copy = {x=pos.x, y=pos.y, z=pos.z}
		local node_copy = {name=node.name, param1=node.param1, param2=node.param2}
		callback(pos_copy, node_copy, puncher)
	end
end

function minetest.handle_node_drops(pos, drops, digger)
	-- Add dropped items to object's inventory
	if digger:get_inventory() then
		local _, dropped_item
		for _, dropped_item in ipairs(drops) do
			digger:get_inventory():add_item("main", dropped_item)
		end
	end
end

function minetest.node_dig(pos, node, digger)
	minetest.debug("node_dig")

	local def = ItemStack({name=node.name}):get_definition()
	-- Check if def ~= 0 because we always want to be able to remove unknown nodes
	if #def ~= 0 and not def.diggable or (def.can_dig and not def.can_dig(pos,digger)) then
		minetest.debug("not diggable")
		minetest.log("info", digger:get_player_name() .. " tried to dig "
			.. node.name .. " which is not diggable "
			.. minetest.pos_to_string(pos))
		return
	end

	minetest.log('action', digger:get_player_name() .. " digs "
		.. node.name .. " at " .. minetest.pos_to_string(pos))

	local wielded = digger:get_wielded_item()
	local drops = minetest.get_node_drops(node.name, wielded:get_name())

	-- Wear out tool
	local tp = wielded:get_tool_capabilities()
	local dp = minetest.get_dig_params(def.groups, tp)
	wielded:add_wear(dp.wear)
	digger:set_wielded_item(wielded)
	
	-- Handle drops
	minetest.handle_node_drops(pos, drops, digger)

	local oldmetadata = nil
	if def.after_dig_node then
		oldmetadata = minetest.env:get_meta(pos):to_table()
	end

	-- Remove node and update
	minetest.env:remove_node(pos)
	
	-- Run callback
	if def.after_dig_node then
		-- Copy pos and node because callback can modify them
		local pos_copy = {x=pos.x, y=pos.y, z=pos.z}
		local node_copy = {name=node.name, param1=node.param1, param2=node.param2}
		def.after_dig_node(pos_copy, node_copy, oldmetadata, digger)
	end

	-- Run script hook
	local _, callback
	for _, callback in ipairs(minetest.registered_on_dignodes) do
		-- Copy pos and node because callback can modify them
		local pos_copy = {x=pos.x, y=pos.y, z=pos.z}
		local node_copy = {name=node.name, param1=node.param1, param2=node.param2}
		callback(pos_copy, node_copy, digger)
	end
end

-- This is used to allow mods to redefine minetest.item_place and so on
-- NOTE: This is not the preferred way. Preferred way is to provide enough
--       callbacks to not require redefining global functions. -celeron55
local function redef_wrapper(table, name)
	return function(...)
		return table[name](...)
	end
end

--
-- Item definition defaults
--

minetest.nodedef_default = {
	-- Item properties
	type="node",
	-- name intentionally not defined here
	description = "",
	groups = {},
	inventory_image = "",
	wield_image = "",
	wield_scale = {x=1,y=1,z=1},
	stack_max = 99,
	usable = false,
	liquids_pointable = false,
	tool_capabilities = nil,
	node_placement_prediction = nil,

	-- Interaction callbacks
	on_place = redef_wrapper(minetest, 'item_place'), -- minetest.item_place
	on_drop = redef_wrapper(minetest, 'item_drop'), -- minetest.item_drop
	on_use = nil,
	can_dig = nil,

	on_punch = redef_wrapper(minetest, 'node_punch'), -- minetest.node_punch
	on_dig = redef_wrapper(minetest, 'node_dig'), -- minetest.node_dig

	on_receive_fields = nil,
	
	on_metadata_inventory_move = minetest.node_metadata_inventory_move_allow_all,
	on_metadata_inventory_offer = minetest.node_metadata_inventory_offer_allow_all,
	on_metadata_inventory_take = minetest.node_metadata_inventory_take_allow_all,

	-- Node properties
	drawtype = "normal",
	visual_scale = 1.0,
	-- Don't define these because otherwise the old tile_images and
	-- special_materials wouldn't be read
	--tiles ={""},
	--special_tiles = {
	--	{name="", backface_culling=true},
	--	{name="", backface_culling=true},
	--},
	alpha = 255,
	post_effect_color = {a=0, r=0, g=0, b=0},
	paramtype = "none",
	paramtype2 = "none",
	is_ground_content = false,
	sunlight_propagates = false,
	walkable = true,
	pointable = true,
	diggable = true,
	climbable = false,
	buildable_to = false,
	liquidtype = "none",
	liquid_alternative_flowing = "",
	liquid_alternative_source = "",
	liquid_viscosity = 0,
	light_source = 0,
	damage_per_second = 0,
	selection_box = {type="regular"},
	legacy_facedir_simple = false,
	legacy_wallmounted = false,
}

minetest.craftitemdef_default = {
	type="craft",
	-- name intentionally not defined here
	description = "",
	groups = {},
	inventory_image = "",
	wield_image = "",
	wield_scale = {x=1,y=1,z=1},
	stack_max = 99,
	liquids_pointable = false,
	tool_capabilities = nil,

	-- Interaction callbacks
	on_place = redef_wrapper(minetest, 'item_place'), -- minetest.item_place
	on_drop = redef_wrapper(minetest, 'item_drop'), -- minetest.item_drop
	on_use = nil,
}

minetest.tooldef_default = {
	type="tool",
	-- name intentionally not defined here
	description = "",
	groups = {},
	inventory_image = "",
	wield_image = "",
	wield_scale = {x=1,y=1,z=1},
	stack_max = 1,
	liquids_pointable = false,
	tool_capabilities = nil,

	-- Interaction callbacks
	on_place = redef_wrapper(minetest, 'item_place'), -- minetest.item_place
	on_drop = redef_wrapper(minetest, 'item_drop'), -- minetest.item_drop
	on_use = nil,
}

minetest.noneitemdef_default = {  -- This is used for the hand and unknown items
	type="none",
	-- name intentionally not defined here
	description = "",
	groups = {},
	inventory_image = "",
	wield_image = "",
	wield_scale = {x=1,y=1,z=1},
	stack_max = 99,
	liquids_pointable = false,
	tool_capabilities = nil,

	-- Interaction callbacks
	on_place = nil,
	on_drop = nil,
	on_use = nil,
}

