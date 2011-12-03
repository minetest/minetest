--
-- This file contains built-in stuff in Minetest implemented in Lua.
--
-- It is always loaded and executed after registration of the C API,
-- before loading and running any mods.
--

function basic_dump2(o)
	if type(o) == "number" then
		return tostring(o)
	elseif type(o) == "string" then
		return string.format("%q", o)
	elseif type(o) == "boolean" then
		return tostring(o)
	elseif type(o) == "function" then
		return "<function>"
	elseif type(o) == "userdata" then
		return "<userdata>"
	elseif type(o) == "nil" then
		return "nil"
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

function dump2(o, name, dumped)
	name = name or "_"
	dumped = dumped or {}
	io.write(name, " = ")
	if type(o) == "number" or type(o) == "string" or type(o) == "boolean"
			or type(o) == "function" or type(o) == "nil"
			or type(o) == "userdata" then
		io.write(basic_dump2(o), "\n")
	elseif type(o) == "table" then
		if dumped[o] then
			io.write(dumped[o], "\n")
		else
			dumped[o] = name
			io.write("{}\n") -- new table
			for k,v in pairs(o) do
				local fieldname = string.format("%s[%s]", name, basic_dump2(k))
				dump2(v, fieldname, dumped)
			end
		end
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

function dump(o, dumped)
	dumped = dumped or {}
	if type(o) == "number" then
		return tostring(o)
	elseif type(o) == "string" then
		return string.format("%q", o)
	elseif type(o) == "table" then
		if dumped[o] then
			return "<circular reference>"
		end
		dumped[o] = true
		local t = {}
		for k,v in pairs(o) do
			t[#t+1] = "" .. k .. " = " .. dump(v, dumped)
		end
		return "{" .. table.concat(t, ", ") .. "}"
	elseif type(o) == "boolean" then
		return tostring(o)
	elseif type(o) == "function" then
		return "<function>"
	elseif type(o) == "userdata" then
		return "<userdata>"
	elseif type(o) == "nil" then
		return "nil"
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

--
-- Built-in node definitions. Also defined in C.
--

minetest.register_nodedef_defaults({
	-- name intentionally not defined here
	drawtype = "normal",
	visual_scale = 1.0,
	tile_images = {"unknown_block.png"},
	inventory_image = "unknown_block.png",
	special_materials = {
		{image="", backface_culling=true},
		{image="", backface_culling=true},
	},
	alpha = 255,
	post_effect_color = {a=0, r=0, g=0, b=0},
	paramtype = "none",
	is_ground_content = false,
	light_propagates = false,
	sunlight_propagates = false,
	walkable = true,
	pointable = true,
	diggable = true,
	climbable = false,
	buildable_to = false,
	wall_mounted = false,
	often_contains_mineral = false,
	dug_item = "",
	extra_dug_item = "",
	extra_dug_item_rarity = 2,
	metadata_name = "",
	liquidtype = "none",
	liquid_alternative_flowing = "",
	liquid_alternative_source = "",
	liquid_viscosity = 0,
	light_source = 0,
	damage_per_second = 0,
	selection_box = {type="regular"},
	material = {
		diggablity = "normal",
		weight = 0,
		crackiness = 0,
		crumbliness = 0,
		cuttability = 0,
		flammability = 0,
	},
	cookresult_item = "", -- Cannot be cooked
	furnace_cooktime = 3.0,
	furnace_burntime = -1, -- Cannot be used as fuel
})

minetest.register_node("air", {
	drawtype = "airlike",
	paramtype = "light",
	light_propagates = true,
	sunlight_propagates = true,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	air_equivalent = true,
})

minetest.register_node("ignore", {
	drawtype = "airlike",
	paramtype = "none",
	light_propagates = false,
	sunlight_propagates = false,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true, -- A way to remove accidentally placed ignores
	air_equivalent = true,
})

--
-- stackstring manipulation functions
-- example stackstring: 'craft "apple" 4'
-- example item: {type="craft", name="apple"}
-- example item: {type="tool", name="SteelPick", wear="23272"}
--

function stackstring_take_item(stackstring)
	if stackstring == nil then
		return '', nil
	end
	local stacktype = nil
	stacktype = string.match(stackstring,
			'([%a%d]+)')
	if stacktype == "node" or stacktype == "craft" then
		local itemtype = nil
		local itemname = nil
		local itemcount = nil
		itemtype, itemname, itemcount = string.match(stackstring,
				'([%a%d]+) "([^"]*)" (%d+)')
		itemcount = tonumber(itemcount)
		if itemcount == 0 then
			return '', nil
		elseif itemcount == 1 then
			return '', {type=itemtype, name=itemname}
		else
			return itemtype.." \""..itemname.."\" "..(itemcount-1),
					{type=itemtype, name=itemname}
		end
	elseif stacktype == "tool" then
		local itemtype = nil
		local itemname = nil
		local itemwear = nil
		itemtype, itemname, itemwear = string.match(stackstring,
				'([%a%d]+) "([^"]*)" (%d+)')
		itemwear = tonumber(itemwear)
		return '', {type=itemtype, name=itemname, wear=itemwear}
	end
end

function stackstring_put_item(stackstring, item)
	if item == nil then
		return stackstring, false
	end
	stackstring = stackstring or ''
	local stacktype = nil
	stacktype = string.match(stackstring,
			'([%a%d]+)')
	stacktype = stacktype or ''
	if stacktype ~= '' and stacktype ~= item.type then
		return stackstring, false
	end
	if item.type == "node" or item.type == "craft" then
		local itemtype = nil
		local itemname = nil
		local itemcount = nil
		itemtype, itemname, itemcount = string.match(stackstring,
				'([%a%d]+) "([^"]*)" (%d+)')
		itemtype = itemtype or item.type
		itemname = itemname or item.name
		if itemcount == nil then
			itemcount = 0
		end
		itemcount = itemcount + 1
		return itemtype.." \""..itemname.."\" "..itemcount, true
	elseif item.type == "tool" then
		if stacktype ~= nil then
			return stackstring, false
		end
		local itemtype = nil
		local itemname = nil
		local itemwear = nil
		itemtype, itemname, itemwear = string.match(stackstring,
				'([%a%d]+) "([^"]*)" (%d+)')
		itemwear = tonumber(itemwear)
		return itemtype.." \""..itemname.."\" "..itemwear, true
	end
	return stackstring, false
end

function stackstring_put_stackstring(stackstring, src)
	while src ~= '' do
		--print("src="..dump(src))
		src, item = stackstring_take_item(src)
		--print("src="..dump(src).." item="..dump(item))
		local success
		stackstring, success = stackstring_put_item(stackstring, item)
		if not success then
			return stackstring, false
		end
	end
	return stackstring, true
end

function test_stackstring()
	local stack
	local item
	local success

	stack, item = stackstring_take_item('node "TNT" 3')
	assert(stack == 'node "TNT" 2')
	assert(item.type == 'node')
	assert(item.name == 'TNT')

	stack, item = stackstring_take_item('craft "with spaces" 2')
	assert(stack == 'craft "with spaces" 1')
	assert(item.type == 'craft')
	assert(item.name == 'with spaces')

	stack, item = stackstring_take_item('craft "with spaces" 1')
	assert(stack == '')
	assert(item.type == 'craft')
	assert(item.name == 'with spaces')

	stack, item = stackstring_take_item('craft "s8df2kj3" 0')
	assert(stack == '')
	assert(item == nil)

	stack, item = stackstring_take_item('tool "With Spaces" 32487')
	assert(stack == '')
	assert(item.type == 'tool')
	assert(item.name == 'With Spaces')
	assert(item.wear == 32487)

	stack, success = stackstring_put_item('node "With Spaces" 40',
			{type='node', name='With Spaces'})
	assert(stack == 'node "With Spaces" 41')
	assert(success == true)

	stack, success = stackstring_put_item('craft "With Spaces" 40',
			{type='craft', name='With Spaces'})
	assert(stack == 'craft "With Spaces" 41')
	assert(success == true)

	stack, success = stackstring_put_item('tool "With Spaces" 32487',
			{type='tool', name='With Spaces'})
	assert(stack == 'tool "With Spaces" 32487')
	assert(success == false)

	stack, success = stackstring_put_item('node "With Spaces" 40',
			{type='tool', name='With Spaces'})
	assert(stack == 'node "With Spaces" 40')
	assert(success == false)
	
	assert(stackstring_put_stackstring('node "With Spaces" 2',
			'node "With Spaces" 1') == 'node "With Spaces" 3')
end
test_stackstring()

--
-- nodeitem helpers
--

minetest.inventorycube = function(img1, img2, img3)
	img2 = img2 or img1
	img3 = img3 or img1
	return "[inventorycube"
			.. "{" .. img1:gsub("%^", "&")
			.. "{" .. img2:gsub("%^", "&")
			.. "{" .. img3:gsub("%^", "&")
end

--
-- craftitem helpers
--

minetest.craftitem_place_item = function(item, placer, pos)
	--print("craftitem_place_item")
	--print("item: " .. dump(item))
	--print("placer: " .. dump(placer))
	--print("pos: " .. dump(pos))
	minetest.env:add_item(pos, 'craft "' .. item .. '" 1')
	return true
end

minetest.craftitem_eat = function(hp_change)
	return function(item, user, pointed_thing)  -- closure
		--print("craftitem_eat(" .. hp_change .. ")")
		--print("item: " .. dump(item))
		--print("user: " .. dump(user))
		--print("pointed_thing: " .. dump(pointed_thing))
		user:set_hp(user:get_hp() + hp_change)
		return true
	end
end

--
-- Default material types
--

function minetest.digprop_constanttime(time)
	return {
		diggability = "constant",
		constant_time = time,
	}
end

function minetest.digprop_stonelike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 5,
		crackiness = 1,
		crumbliness = -0.1,
		cuttability = -0.2,
	}
end

function minetest.digprop_dirtlike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 1.2,
		crackiness = 0,
		crumbliness = 1.2,
		cuttability = -0.4,
	}
end

function minetest.digprop_gravellike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 2,
		crackiness = 0.2,
		crumbliness = 1.5,
		cuttability = -1.0,
	}
end

function minetest.digprop_woodlike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 1.0,
		crackiness = 0.75,
		crumbliness = -1.0,
		cuttability = 1.5,
	}
end

function minetest.digprop_leaveslike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * (-0.5),
		crackiness = 0,
		crumbliness = 0,
		cuttability = 2.0,
	}
end

function minetest.digprop_glasslike(toughness)
	return {
		diggablity = "normal",
		weight = toughness * 0.1,
		crackiness = 2.0,
		crumbliness = -1.0,
		cuttability = -1.0,
	}
end

--
-- Creative inventory
--

minetest.creative_inventory = {}

minetest.add_to_creative_inventory = function(itemstring)
	table.insert(minetest.creative_inventory, itemstring)
end

--
-- Callback registration
--

function make_registration()
	local t = {}
	local registerfunc = function(func) table.insert(t, func) end
	return t, registerfunc
end

minetest.registered_on_chat_messages, minetest.register_on_chat_message = make_registration()
minetest.registered_globalsteps, minetest.register_globalstep = make_registration()
minetest.registered_on_placenodes, minetest.register_on_placenode = make_registration()
minetest.registered_on_dignodes, minetest.register_on_dignode = make_registration()
minetest.registered_on_punchnodes, minetest.register_on_punchnode = make_registration()
minetest.registered_on_generateds, minetest.register_on_generated = make_registration()
minetest.registered_on_newplayers, minetest.register_on_newplayer = make_registration()
minetest.registered_on_respawnplayers, minetest.register_on_respawnplayer = make_registration()

-- END
