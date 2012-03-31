--
-- This file contains built-in stuff in Minetest implemented in Lua.
--
-- It is always loaded and executed after registration of the C API,
-- before loading and running any mods.
--

--
-- Override some Lua library functions
--

print = minetest.debug

--
-- Define some random basic things
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

function string:split(sep)
	local sep, fields = sep or ",", {}
	local pattern = string.format("([^%s]+)", sep)
	self:gsub(pattern, function(c) fields[#fields+1] = c end)
	return fields
end

function string:trim()
	return (self:gsub("^%s*(.-)%s*$", "%1"))
end

assert(string.trim("\n \t\tfoo\t ") == "foo")

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

function minetest.pos_to_string(pos)
	return "(" .. pos.x .. "," .. pos.y .. "," .. pos.z .. ")"
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
	if def.type == "node" and pointed_thing.type == "node" then
		local pos = pointed_thing.above
		local oldnode = minetest.env:get_node(pos)
		local olddef = ItemStack({name=oldnode.name}):get_definition()

		if not olddef.buildable_to then
			minetest.log("info", placer:get_player_name() .. " tried to place"
				.. " node in invalid position " .. minetest.pos_to_string(pos)
				.. ", replacing " .. oldnode.name)
			return
		end

		minetest.log("action", placer:get_player_name() .. " places node "
			.. def.name .. " at " .. minetest.pos_to_string(pos))

		local newnode = {name = def.name, param1 = 0, param2 = 0}

		-- Calculate direction for wall mounted stuff like torches and signs
		if def.paramtype2 == 'wallmounted' then
			local under = pointed_thing.under
			local above = pointed_thing.above
			local dir = {x = under.x - above.x, y = under.y - above.y, z = under.z - above.z}
			newnode.param2 = minetest.dir_to_wallmounted(dir)
		-- Calculate the direction for furnaces and chests and stuff
		elseif def.paramtype2 == 'facedir' then
			local playerpos = placer:getpos()
			local dir = {x = pos.x - playerpos.x, y = pos.y - playerpos.y, z = pos.z - playerpos.z}
			newnode.param2 = minetest.dir_to_facedir(dir)
			minetest.log("action", "facedir: " .. newnode.param2)
		end

		-- Add node and update
		minetest.env:add_node(pos, newnode)

		-- Set metadata owner
		if def.metadata_name ~= "" then
			minetest.env:get_meta(pos):set_owner(placer:get_player_name())
		end

		-- Run script hook
		local _, callback
		for _, callback in ipairs(minetest.registered_on_placenodes) do
			callback(pos, newnode, placer)
		end

		itemstack:take_item()
	end
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
	minetest.env:add_item(pos, itemstack)
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
		callback(pos, node, puncher)
	end

end

function minetest.node_dig(pos, node, digger)
	minetest.debug("node_dig")

	local def = ItemStack({name=node.name}):get_definition()
	if not def.diggable then
		minetest.debug("not diggable")
		minetest.log("info", digger:get_player_name() .. " tried to dig "
			.. node.name .. " which is not diggable "
			.. minetest.pos_to_string(pos))
		return
	end

	local meta = minetest.env:get_meta(pos)
	if meta ~= nil and not meta:get_allow_removal() then
		minetest.debug("dig prevented by metadata")
		minetest.log("info", digger:get_player_name() .. " tried to dig "
			.. node.name .. ", but removal is disabled by metadata "
			.. minetest.pos_to_string(pos))
		return
	end

	minetest.log('action', digger:get_player_name() .. " digs "
		.. node.name .. " at " .. minetest.pos_to_string(pos))

	if not minetest.setting_getbool("creative_mode") then
		local wielded = digger:get_wielded_item()
		local drops = minetest.get_node_drops(node.name, wielded:get_name())

		-- Wear out tool
		tp = wielded:get_tool_capabilities()
		dp = minetest.get_dig_params(def.groups, tp)
		wielded:add_wear(dp.wear)
		digger:set_wielded_item(wielded)

		-- Add dropped items
		local _, dropped_item
		for _, dropped_item in ipairs(drops) do
			digger:get_inventory():add_item("main", dropped_item)
		end
	end

	-- Remove node and update
	minetest.env:remove_node(pos)

	-- Run script hook
	local _, callback
	for _, callback in ipairs(minetest.registered_on_dignodes) do
		callback(pos, node, digger)
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

	-- Interaction callbacks
	on_place = minetest.item_place,
	on_drop = minetest.item_drop,
	on_use = nil,

	on_punch = minetest.node_punch,
	on_dig = minetest.node_dig,

	-- Node properties
	drawtype = "normal",
	visual_scale = 1.0,
	tile_images = {""},
	special_materials = {
		{image="", backface_culling=true},
		{image="", backface_culling=true},
	},
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
	metadata_name = "",
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
	on_place = minetest.item_place,
	on_drop = minetest.item_drop,
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
	on_place = minetest.item_place,
	on_drop = minetest.item_drop,
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

--
-- Make raw registration functions inaccessible to anyone except builtin.lua
--

local register_item_raw = minetest.register_item_raw
minetest.register_item_raw = nil

local register_alias_raw = minetest.register_alias_raw
minetest.register_item_raw = nil

--
-- Item / entity / ABM registration functions
--

minetest.registered_abms = {}
minetest.registered_entities = {}
minetest.registered_items = {}
minetest.registered_nodes = {}
minetest.registered_craftitems = {}
minetest.registered_tools = {}
minetest.registered_aliases = {}

-- For tables that are indexed by item name:
-- If table[X] does not exist, default to table[minetest.registered_aliases[X]]
local function set_alias_metatable(table)
	setmetatable(table, {
		__index = function(name)
			return rawget(table, minetest.registered_aliases[name])
		end
	})
end
set_alias_metatable(minetest.registered_items)
set_alias_metatable(minetest.registered_nodes)
set_alias_metatable(minetest.registered_craftitems)
set_alias_metatable(minetest.registered_tools)

-- These item names may not be used because they would interfere
-- with legacy itemstrings
local forbidden_item_names = {
	MaterialItem = true,
	MaterialItem2 = true,
	MaterialItem3 = true,
	NodeItem = true,
	node = true,
	CraftItem = true,
	craft = true,
	MBOItem = true,
	ToolItem = true,
	tool = true,
}

local function check_modname_prefix(name)
	if name:sub(1,1) == ":" then
		-- Escape the modname prefix enforcement mechanism
		return name:sub(2)
	else
		-- Modname prefix enforcement
		local expected_prefix = minetest.get_current_modname() .. ":"
		if name:sub(1, #expected_prefix) ~= expected_prefix then
			error("Name " .. name .. " does not follow naming conventions: " ..
				"\"modname:\" or \":\" prefix required")
		end
		local subname = name:sub(#expected_prefix+1)
		if subname:find("[^abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_]") then
			error("Name " .. name .. " does not follow naming conventions: " ..
				"contains unallowed characters")
		end
		return name
	end
end

function minetest.register_abm(spec)
	-- Add to minetest.registered_abms
	minetest.registered_abms[#minetest.registered_abms+1] = spec
end

function minetest.register_entity(name, prototype)
	-- Check name
	if name == nil then
		error("Unable to register entity: Name is nil")
	end
	name = check_modname_prefix(tostring(name))

	prototype.name = name
	prototype.__index = prototype  -- so that it can be used as a metatable

	-- Add to minetest.registered_entities
	minetest.registered_entities[name] = prototype
end

function minetest.register_item(name, itemdef)
	-- Check name
	if name == nil then
		error("Unable to register item: Name is nil")
	end
	name = check_modname_prefix(tostring(name))
	if forbidden_item_names[name] then
		error("Unable to register item: Name is forbidden: " .. name)
	end
	itemdef.name = name

	-- Apply defaults and add to registered_* table
	if itemdef.type == "node" then
		setmetatable(itemdef, {__index = minetest.nodedef_default})
		minetest.registered_nodes[itemdef.name] = itemdef
	elseif itemdef.type == "craft" then
		setmetatable(itemdef, {__index = minetest.craftitemdef_default})
		minetest.registered_craftitems[itemdef.name] = itemdef
	elseif itemdef.type == "tool" then
		setmetatable(itemdef, {__index = minetest.tooldef_default})
		minetest.registered_tools[itemdef.name] = itemdef
	elseif itemdef.type == "none" then
		setmetatable(itemdef, {__index = minetest.noneitemdef_default})
	else
		error("Unable to register item: Type is invalid: " .. dump(itemdef))
	end

	-- Flowing liquid uses param2
	if itemdef.type == "node" and itemdef.liquidtype == "flowing" then
		itemdef.paramtype2 = "flowingliquid"
	end

	-- BEGIN Legacy stuff
	if itemdef.cookresult_itemstring ~= nil and itemdef.cookresult_itemstring ~= "" then
		minetest.register_craft({
			type="cooking",
			output=itemdef.cookresult_itemstring,
			recipe=itemdef.name,
			cooktime=itemdef.furnace_cooktime
		})
	end
	if itemdef.furnace_burntime ~= nil and itemdef.furnace_burntime >= 0 then
		minetest.register_craft({
			type="fuel",
			recipe=itemdef.name,
			burntime=itemdef.furnace_burntime
		})
	end
	-- END Legacy stuff

	-- Disable all further modifications
	getmetatable(itemdef).__newindex = {}

	--minetest.log("Registering item: " .. itemdef.name)
	minetest.registered_items[itemdef.name] = itemdef
	minetest.registered_aliases[itemdef.name] = nil
	register_item_raw(itemdef)
end

function minetest.register_node(name, nodedef)
	nodedef.type = "node"
	minetest.register_item(name, nodedef)
end

function minetest.register_craftitem(name, craftitemdef)
	craftitemdef.type = "craft"

	-- BEGIN Legacy stuff
	if craftitemdef.inventory_image == nil and craftitemdef.image ~= nil then
		craftitemdef.inventory_image = craftitemdef.image
	end
	-- END Legacy stuff

	minetest.register_item(name, craftitemdef)
end

function minetest.register_tool(name, tooldef)
	tooldef.type = "tool"
	tooldef.stack_max = 1

	-- BEGIN Legacy stuff
	if tooldef.inventory_image == nil and tooldef.image ~= nil then
		tooldef.inventory_image = tooldef.image
	end
	if tooldef.tool_capabilities == nil and
	   (tooldef.full_punch_interval ~= nil or
	    tooldef.basetime ~= nil or
	    tooldef.dt_weight ~= nil or
	    tooldef.dt_crackiness ~= nil or
	    tooldef.dt_crumbliness ~= nil or
	    tooldef.dt_cuttability ~= nil or
	    tooldef.basedurability ~= nil or
	    tooldef.dd_weight ~= nil or
	    tooldef.dd_crackiness ~= nil or
	    tooldef.dd_crumbliness ~= nil or
	    tooldef.dd_cuttability ~= nil) then
		tooldef.tool_capabilities = {
			full_punch_interval = tooldef.full_punch_interval,
			basetime = tooldef.basetime,
			dt_weight = tooldef.dt_weight,
			dt_crackiness = tooldef.dt_crackiness,
			dt_crumbliness = tooldef.dt_crumbliness,
			dt_cuttability = tooldef.dt_cuttability,
			basedurability = tooldef.basedurability,
			dd_weight = tooldef.dd_weight,
			dd_crackiness = tooldef.dd_crackiness,
			dd_crumbliness = tooldef.dd_crumbliness,
			dd_cuttability = tooldef.dd_cuttability,
		}
	end
	-- END Legacy stuff

	minetest.register_item(name, tooldef)
end

function minetest.register_alias(name, convert_to)
	if forbidden_item_names[name] then
		error("Unable to register alias: Name is forbidden: " .. name)
	end
	if minetest.registered_items[name] ~= nil then
		minetest.log("WARNING: Not registering alias, item with same name" ..
			" is already defined: " .. name .. " -> " .. convert_to)
	else
		--minetest.log("Registering alias: " .. name .. " -> " .. convert_to)
		minetest.registered_aliases[name] = convert_to
		register_alias_raw(name, convert_to)
	end
end

-- Alias the forbidden item names to "" so they can't be
-- created via itemstrings (e.g. /give)
local name
for name in pairs(forbidden_item_names) do
	minetest.registered_aliases[name] = ""
	register_alias_raw(name, "")
end


-- Deprecated:
-- Aliases for minetest.register_alias (how ironic...)
--minetest.alias_node = minetest.register_alias
--minetest.alias_tool = minetest.register_alias
--minetest.alias_craftitem = minetest.register_alias

--
-- Built-in node definitions. Also defined in C.
--

minetest.register_item(":unknown", {
	type = "none",
	description = "Unknown Item",
	inventory_image = "unknown_item.png",
	on_place = minetest.item_place,
	on_drop = minetest.item_drop,
})

minetest.register_node(":air", {
	description = "Air (you hacker you!)",
	inventory_image = "unknown_block.png",
	wield_image = "unknown_block.png",
	drawtype = "airlike",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	air_equivalent = true,
})

minetest.register_node(":ignore", {
	description = "Ignore (you hacker you!)",
	inventory_image = "unknown_block.png",
	wield_image = "unknown_block.png",
	drawtype = "airlike",
	paramtype = "none",
	sunlight_propagates = false,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true, -- A way to remove accidentally placed ignores
	air_equivalent = true,
})

-- The hand (bare definition)
minetest.register_item(":", {
	type = "none",
})

--
-- Default material types
--
function digprop_err()
	minetest.log("info", debug.traceback())
	minetest.log("info", "WARNING: The minetest.digprop_* functions are obsolete and need to be replaced by item groups.")
end

minetest.digprop_constanttime = digprop_err
minetest.digprop_stonelike = digprop_err
minetest.digprop_dirtlike = digprop_err
minetest.digprop_gravellike = digprop_err
minetest.digprop_woodlike = digprop_err
minetest.digprop_leaveslike = digprop_err
minetest.digprop_glasslike = digprop_err

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

local function make_registration()
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
minetest.registered_on_dieplayers, minetest.register_on_dieplayer = make_registration()
minetest.registered_on_respawnplayers, minetest.register_on_respawnplayer = make_registration()
minetest.registered_on_joinplayers, minetest.register_on_joinplayer = make_registration()
minetest.registered_on_leaveplayers, minetest.register_on_leaveplayer = make_registration()

--
-- Misc. API functions
--

minetest.timers_to_add = {}
minetest.timers = {}
minetest.register_globalstep(function(dtime)
	for _, timer in ipairs(minetest.timers_to_add) do
		table.insert(minetest.timers, timer)
	end
	minetest.timers_to_add = {}
	for index, timer in ipairs(minetest.timers) do
		timer.time = timer.time - dtime
		if timer.time <= 0 then
			timer.func(timer.param)
			table.remove(minetest.timers,index)
		end
	end
end)

function minetest.after(time, func, param)
	table.insert(minetest.timers_to_add, {time=time, func=func, param=param})
end

function minetest.check_player_privs(name, privs)
	local player_privs = minetest.get_player_privs(name)
	local missing_privileges = {}
	for priv, val in pairs(privs) do
		if val then
			if not player_privs[priv] then
				table.insert(missing_privileges, priv)
			end
		end
	end
	if #missing_privileges > 0 then
		return false, missing_privileges
	end
	return true, ""
end

function minetest.get_connected_players()
	-- This could be optimized a bit, but leave that for later
	local list = {}
	for _, obj in pairs(minetest.env:get_objects_inside_radius({x=0,y=0,z=0}, 1000000)) do
		if obj:get_player_name() then
			table.insert(list, obj)
		end
	end
	return list
end

function minetest.hash_node_position(pos)
	return (pos.z+32768)*65536*65536 + (pos.y+32768)*65536 + pos.x+32768
end

--
-- Privileges
--

minetest.registered_privileges = {}
function minetest.register_privilege(name, description)
	minetest.registered_privileges[name] = description
end

minetest.register_privilege("interact", "Can interact with things and modify the world")
minetest.register_privilege("teleport", "Can use /teleport command")
minetest.register_privilege("bring", "Can teleport other players")
minetest.register_privilege("settime", "Can use /time")
minetest.register_privilege("privs", "Can modify privileges")
minetest.register_privilege("server", "Can do server maintenance stuff")
minetest.register_privilege("shout", "Can speak in chat")
minetest.register_privilege("ban", "Can ban and unban players")
minetest.register_privilege("give", "Can use /give and /giveme")
minetest.register_privilege("password", "Can use /setpassword and /clearpassword")

--
-- Chat commands
--

minetest.chatcommands = {}
function minetest.register_chatcommand(cmd, def)
	def = def or {}
	def.params = def.params or ""
	def.description = def.description or ""
	def.privs = def.privs or {}
	minetest.chatcommands[cmd] = def
end

-- Register the help command
minetest.register_chatcommand("help", {
	privs = {},
	params = "(nothing)/all/privs/<cmd>",
	description = "Get help for commands or list privileges",
	func = function(name, param)
		local format_help_line = function(cmd, def)
			local msg = "/"..cmd
			if def.params and def.params ~= "" then msg = msg .. " " .. def.params end
			if def.description and def.description ~= "" then msg = msg .. ": " .. def.description end
			return msg
		end
		if param == "" then
			local msg = ""
			cmds = {}
			for cmd, def in pairs(minetest.chatcommands) do
				if minetest.check_player_privs(name, def.privs) then
					table.insert(cmds, cmd)
				end
			end
			minetest.chat_send_player(name, "Available commands: "..table.concat(cmds, " "))
			minetest.chat_send_player(name, "Use '/help <cmd>' to get more information, or '/help all' to list everything.")
		elseif param == "all" then
			minetest.chat_send_player(name, "Available commands:")
			for cmd, def in pairs(minetest.chatcommands) do
				if minetest.check_player_privs(name, def.privs) then
					minetest.chat_send_player(name, format_help_line(cmd, def))
				end
			end
		elseif param == "privs" then
			minetest.chat_send_player(name, "Available privileges:")
			for priv, desc in pairs(minetest.registered_privileges) do
				minetest.chat_send_player(name, priv..": "..desc)
			end
		else
			local cmd = param
			def = minetest.chatcommands[cmd]
			if not def then
				minetest.chat_send_player(name, "Command not available: "..cmd)
			else
				minetest.chat_send_player(name, format_help_line(cmd, def))
			end
		end
	end,
})

-- Register C++ commands without functions
minetest.register_chatcommand("me", {params = nil, description = "chat action (eg. /me orders a pizza)"})
minetest.register_chatcommand("status", {description = "print server status line"})
minetest.register_chatcommand("shutdown", {params = "", description = "shutdown server", privs = {server=true}})
minetest.register_chatcommand("setting", {params = "<name> = <value>", description = "set line in configuration file", privs = {server=true}})
minetest.register_chatcommand("clearobjects", {params = "", description = "clear all objects in world", privs = {server=true}})
minetest.register_chatcommand("time", {params = "<0...24000>", description = "set time of day", privs = {settime=true}})
minetest.register_chatcommand("ban", {params = "<name>", description = "ban IP of player", privs = {ban=true}})
minetest.register_chatcommand("unban", {params = "<name/ip>", description = "remove IP ban", privs = {ban=true}})

-- Register some other commands
minetest.register_chatcommand("privs", {
	params = "<name>",
	description = "print out privileges of player",
	func = function(name, param)
		if param == "" then
			param = name
		else
			if not minetest.check_player_privs(name, {privs=true}) then
				minetest.chat_send_player(name, "Privileges of "..param.." are hidden from you.")
			end
		end
		privs = {}
		for priv, _ in pairs(minetest.get_player_privs(param)) do
			table.insert(privs, priv)
		end
		minetest.chat_send_player(name, "Privileges of "..param..": "..table.concat(privs, " "))
	end,
})
minetest.register_chatcommand("grant", {
	params = "<name> <privilege>",
	description = "Give privilege to player",
	privs = {privs=true},
	func = function(name, param)
		local grantname, grantprivstr = string.match(param, "([^ ]+) (.+)")
		if not grantname or not grantprivstr then
			minetest.chat_send_player(name, "Invalid parameters (see /help grant)")
			return
		end
		local grantprivs = minetest.string_to_privs(grantprivstr)
		local privs = minetest.get_player_privs(grantname)
		for priv, _ in pairs(grantprivs) do
			privs[priv] = true
		end
		minetest.set_player_privs(grantname, privs)
	end,
})
minetest.register_chatcommand("revoke", {
	params = "<name> <privilege>",
	description = "Remove privilege from player",
	privs = {privs=true},
	func = function(name, param)
		local revokename, revokeprivstr = string.match(param, "([^ ]+) (.+)")
		if not revokename or not revokeprivstr then
			minetest.chat_send_player(name, "Invalid parameters (see /help revoke)")
			return
		end
		local revokeprivs = minetest.string_to_privs(revokeprivstr)
		local privs = minetest.get_player_privs(revokename)
		for priv, _ in pairs(revokeprivs) do
			table.remove(privs, priv)
		end
		minetest.set_player_privs(revokename, privs)
	end,
})
minetest.register_chatcommand("setpassword", {
	params = "<name> <password>",
	description = "set given password",
	privs = {password=true},
	func = function(name, param)
		if param == "" then
			minetest.chat_send_player(name, "Password field required")
			return
		end
		minetest.set_player_password(name, param)
		minetest.chat_send_player(name, "Password set")
	end,
})
minetest.register_chatcommand("clearpassword", {
	params = "<name>",
	description = "set empty password",
	privs = {password=true},
	func = function(name, param)
		minetest.set_player_password(name, '')
		minetest.chat_send_player(name, "Password cleared")
	end,
})

minetest.register_chatcommand("teleport", {
	params = "<X>,<Y>,<Z> | <to_name> | <name> <X>,<Y>,<Z> | <name> <to_name>",
	description = "teleport to given position",
	privs = {teleport=true},
	func = function(name, param)
		-- Returns (pos, true) if found, otherwise (pos, false)
		local function find_free_position_near(pos)
			local tries = {
				{x=1,y=0,z=0},
				{x=-1,y=0,z=0},
				{x=0,y=0,z=1},
				{x=0,y=0,z=-1},
			}
			for _, d in ipairs(tries) do
				local p = {x = pos.x+d.x, y = pos.y+d.y, z = pos.z+d.z}
				local n = minetest.env:get_node(p)
				if not minetest.registered_nodes[n.name].walkable then
					return p, true
				end
			end
			return pos, false
		end

		local teleportee = nil
		local p = {}
		p.x, p.y, p.z = string.match(param, "^([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
		teleportee = minetest.env:get_player_by_name(name)
		if teleportee and p.x and p.y and p.z then
			minetest.chat_send_player(name, "Teleporting to ("..p.x..", "..p.y..", "..p.z..")")
			teleportee:setpos(p)
			return
		end
		
		local teleportee = nil
		local p = nil
		local target_name = nil
		target_name = string.match(param, "^([^ ]+)$")
		teleportee = minetest.env:get_player_by_name(name)
		if target_name then
			local target = minetest.env:get_player_by_name(target_name)
			if target then
				p = target:getpos()
			end
		end
		if teleportee and p then
			p = find_free_position_near(p)
			minetest.chat_send_player(name, "Teleporting to "..target_name.." at ("..p.x..", "..p.y..", "..p.z..")")
			teleportee:setpos(p)
			return
		end
		
		if minetest.check_player_privs(name, {bring=true}) then
			local teleportee = nil
			local p = {}
			local teleportee_name = nil
			teleportee_name, p.x, p.y, p.z = string.match(param, "^([^ ]+) +([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
			if teleportee_name then
				teleportee = minetest.env:get_player_by_name(teleportee_name)
			end
			if teleportee and p.x and p.y and p.z then
				minetest.chat_send_player(name, "Teleporting "..teleportee_name.." to ("..p.x..", "..p.y..", "..p.z..")")
				teleportee:setpos(p)
				return
			end
			
			local teleportee = nil
			local p = nil
			local teleportee_name = nil
			local target_name = nil
			teleportee_name, target_name = string.match(param, "^([^ ]+) +([^ ]+)$")
			if teleportee_name then
				teleportee = minetest.env:get_player_by_name(teleportee_name)
			end
			if target_name then
				local target = minetest.env:get_player_by_name(target_name)
				if target then
					p = target:getpos()
				end
			end
			if teleportee and p then
				p = find_free_position_near(p)
				minetest.chat_send_player(name, "Teleporting "..teleportee_name.." to "..target_name.." at ("..p.x..", "..p.y..", "..p.z..")")
				teleportee:setpos(p)
				return
			end
		end

		minetest.chat_send_player(name, "Invalid parameters (\""..param.."\") or player not found (see /help teleport)")
		return
	end,
})

--
-- Builtin chat handler
--

minetest.register_on_chat_message(function(name, message)
	local cmd, param = string.match(message, "/([^ ]+) *(.*)")
	if not param then
		param = ""
	end
	local cmd_def = minetest.chatcommands[cmd]
	if cmd_def then
		if not cmd_def.func then
			-- This is a C++ command
			return false
		else
			local has_privs, missing_privs = minetest.check_player_privs(name, cmd_def.privs)
			if has_privs then
				cmd_def.func(name, param)
			else
				minetest.chat_send_player(name, "You don't have permission to run this command (missing privileges: "..table.concat(missing_privs, ", ")..")")
			end
			return true -- handled chat message
		end
	end
	return false
end)

--
-- Authentication handler
--

function minetest.string_to_privs(str)
	assert(type(str) == "string")
	privs = {}
	for _, priv in pairs(string.split(str, ',')) do
		privs[priv:trim()] = true
	end
	return privs
end

function minetest.privs_to_string(privs)
	assert(type(privs) == "table")
	list = {}
	for priv, bool in pairs(privs) do
		if bool then
			table.insert(list, priv)
		end
	end
	return table.concat(list, ',')
end

assert(minetest.string_to_privs("a,b").b == true)
assert(minetest.privs_to_string({a=true,b=true}) == "a,b")

minetest.auth_file_path = minetest.get_worldpath().."/auth.txt"
minetest.auth_table = {}

local function read_auth_file()
	local newtable = {}
	local file, errmsg = io.open(minetest.auth_file_path, 'rb')
	if not file then
		minetest.log("info", minetest.auth_file_path.." could not be opened for reading ("..errmsg.."); assuming new world")
		return
	end
	for line in file:lines() do
		if line ~= "" then
			local name, password, privilegestring = string.match(line, "([^:]*):([^:]*):([^:]*)")
			if not name or not password or not privilegestring then
				error("Invalid line in auth.txt: "..dump(line))
			end
			local privileges = minetest.string_to_privs(privilegestring)
			newtable[name] = {password=password, privileges=privileges}
		end
	end
	io.close(file)
	minetest.auth_table = newtable
end

local function save_auth_file()
	local newtable = {}
	-- Check table for validness before attempting to save
	for name, stuff in pairs(minetest.auth_table) do
		assert(type(name) == "string")
		assert(name ~= "")
		assert(type(stuff) == "table")
		assert(type(stuff.password) == "string")
		assert(type(stuff.privileges) == "table")
	end
	local file, errmsg = io.open(minetest.auth_file_path, 'w+b')
	if not file then
		error(minetest.auth_file_path.." could not be opened for writing: "..errmsg)
	end
	for name, stuff in pairs(minetest.auth_table) do
		local privstring = minetest.privs_to_string(stuff.privileges)
		file:write(name..":"..stuff.password..":"..privstring..'\n')
	end
	io.close(file)
end

read_auth_file()

minetest.builtin_auth_handler = {
	get_auth = function(name)
		assert(type(name) == "string")
		if not minetest.auth_table[name] then
			minetest.builtin_auth_handler.create_auth(name, minetest.get_password_hash(name, minetest.setting_get("default_password")))
		end
		if minetest.is_singleplayer() then
			return {
				password = "",
				privileges = minetest.registered_privileges
			}
		else
			if minetest.auth_table[name] and name == minetest.setting_get("name") then
				return {
					password = minetest.auth_table[name].password,
					privileges = minetest.registered_privileges
				}
			else
				return minetest.auth_table[name]
			end
		end
	end,
	create_auth = function(name, password)
		assert(type(name) == "string")
		assert(type(password) == "string")
		minetest.log('info', "Built-in authentication handler adding player '"..name.."'")
		minetest.auth_table[name] = {
			password = password,
			privileges = minetest.string_to_privs(minetest.setting_get("default_privs")),
		}
		save_auth_file()
	end,
	set_password = function(name, password)
		assert(type(name) == "string")
		assert(type(password) == "string")
		if not minetest.auth_table[name] then
			minetest.builtin_auth_handler.create_auth(name, password)
		else
			minetest.log('info', "Built-in authentication handler setting password of player '"..name.."'")
			minetest.auth_table[name].password = password
			save_auth_file()
		end
	end,
	set_privileges = function(name, privileges)
		assert(type(name) == "string")
		assert(type(privileges) == "table")
		if not minetest.auth_table[name] then
			minetest.builtin_auth_handler.create_auth(name, minetest.get_password_hash(name, minetest.setting_get("default_password")))
		end
		minetest.auth_table[name].privileges = privileges
		save_auth_file()
	end
}

function minetest.register_authentication_handler(handler)
	if minetest.registered_auth_handler then
		error("Add-on authentication handler already registered by "..minetest.registered_auth_handler_modname)
	end
	minetest.registered_auth_handler = handler
	minetest.registered_auth_handler_modname = minetest.get_current_modname()
end

function minetest.get_auth_handler()
	if minetest.registered_auth_handler then
		return minetest.registered_auth_handler
	end
	return minetest.builtin_auth_handler
end

function minetest.set_player_password(name, password)
	minetest.get_auth_handler().set_password(name, password)
end

function minetest.set_player_privs(name, privs)
	minetest.get_auth_handler().set_privileges(name, privs)
end

--
-- Set random seed
--

math.randomseed(os.time())

-- END
