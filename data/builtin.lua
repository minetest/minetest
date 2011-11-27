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
-- Chat message processing
--

minetest.registered_on_chat_messages = {}

minetest.on_chat_message = function(name, message)
	for i,func in ipairs(minetest.registered_on_chat_messages) do
		ate = func(name, message)
		if ate then
			return true
		end
	end
	return false
end

minetest.register_on_chat_message = function(func)
	table.insert(minetest.registered_on_chat_messages, func)
end

-- END
