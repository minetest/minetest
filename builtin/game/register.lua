-- Minetest: builtin/misc_register.lua

--
-- Make raw registration functions inaccessible to anyone except this file
--

local register_item_raw = core.register_item_raw
core.register_item_raw = nil

local register_alias_raw = core.register_alias_raw
core.register_alias_raw = nil

--
-- Item / entity / ABM registration functions
--

core.registered_abms = {}
core.registered_entities = {}
core.registered_items = {}
core.registered_nodes = {}
core.registered_craftitems = {}
core.registered_tools = {}
core.registered_aliases = {}

-- For tables that are indexed by item name:
-- If table[X] does not exist, default to table[core.registered_aliases[X]]
local alias_metatable = {
	__index = function(t, name)
		return rawget(t, core.registered_aliases[name])
	end
}
setmetatable(core.registered_items, alias_metatable)
setmetatable(core.registered_nodes, alias_metatable)
setmetatable(core.registered_craftitems, alias_metatable)
setmetatable(core.registered_tools, alias_metatable)

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
		local expected_prefix = core.get_current_modname() .. ":"
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

function core.register_abm(spec)
	-- Add to core.registered_abms
	core.registered_abms[#core.registered_abms+1] = spec
	spec.mod_origin = core.get_current_modname() or "??"
end

function core.register_entity(name, prototype)
	-- Check name
	if name == nil then
		error("Unable to register entity: Name is nil")
	end
	name = check_modname_prefix(tostring(name))

	prototype.name = name
	prototype.__index = prototype  -- so that it can be used as a metatable

	-- Add to core.registered_entities
	core.registered_entities[name] = prototype
	prototype.mod_origin = core.get_current_modname() or "??"
end

function core.register_item(name, itemdef)
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
		-- Use the nodebox as selection box if it's not set manually
		if itemdef.drawtype == "nodebox" and not itemdef.selection_box then
			itemdef.selection_box = itemdef.node_box
		elseif itemdef.drawtype == "fencelike" and not itemdef.selection_box then
			itemdef.selection_box = {
				type = "fixed",
				fixed = {-1/8, -1/2, -1/8, 1/8, 1/2, 1/8},
			}
		end
		setmetatable(itemdef, {__index = core.nodedef_default})
		core.registered_nodes[itemdef.name] = itemdef
	elseif itemdef.type == "craft" then
		setmetatable(itemdef, {__index = core.craftitemdef_default})
		core.registered_craftitems[itemdef.name] = itemdef
	elseif itemdef.type == "tool" then
		setmetatable(itemdef, {__index = core.tooldef_default})
		core.registered_tools[itemdef.name] = itemdef
	elseif itemdef.type == "none" then
		setmetatable(itemdef, {__index = core.noneitemdef_default})
	else
		error("Unable to register item: Type is invalid: " .. dump(itemdef))
	end

	-- Flowing liquid uses param2
	if itemdef.type == "node" and itemdef.liquidtype == "flowing" then
		itemdef.paramtype2 = "flowingliquid"
	end

	-- BEGIN Legacy stuff
	if itemdef.cookresult_itemstring ~= nil and itemdef.cookresult_itemstring ~= "" then
		core.register_craft({
			type="cooking",
			output=itemdef.cookresult_itemstring,
			recipe=itemdef.name,
			cooktime=itemdef.furnace_cooktime
		})
	end
	if itemdef.furnace_burntime ~= nil and itemdef.furnace_burntime >= 0 then
		core.register_craft({
			type="fuel",
			recipe=itemdef.name,
			burntime=itemdef.furnace_burntime
		})
	end
	-- END Legacy stuff

	itemdef.mod_origin = core.get_current_modname() or "??"

	-- Disable all further modifications
	getmetatable(itemdef).__newindex = {}

	--core.log("Registering item: " .. itemdef.name)
	core.registered_items[itemdef.name] = itemdef
	core.registered_aliases[itemdef.name] = nil
	register_item_raw(itemdef)
end

function core.register_node(name, nodedef)
	nodedef.type = "node"
	core.register_item(name, nodedef)
end

function core.register_craftitem(name, craftitemdef)
	craftitemdef.type = "craft"

	-- BEGIN Legacy stuff
	if craftitemdef.inventory_image == nil and craftitemdef.image ~= nil then
		craftitemdef.inventory_image = craftitemdef.image
	end
	-- END Legacy stuff

	core.register_item(name, craftitemdef)
end

function core.register_tool(name, tooldef)
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

	core.register_item(name, tooldef)
end

function core.register_alias(name, convert_to)
	if forbidden_item_names[name] then
		error("Unable to register alias: Name is forbidden: " .. name)
	end
	if core.registered_items[name] ~= nil then
		core.log("WARNING: Not registering alias, item with same name" ..
			" is already defined: " .. name .. " -> " .. convert_to)
	else
		--core.log("Registering alias: " .. name .. " -> " .. convert_to)
		core.registered_aliases[name] = convert_to
		register_alias_raw(name, convert_to)
	end
end

function core.on_craft(itemstack, player, old_craft_list, craft_inv)
	for _, func in ipairs(core.registered_on_crafts) do
		itemstack = func(itemstack, player, old_craft_list, craft_inv) or itemstack
	end
	return itemstack
end

function core.craft_predict(itemstack, player, old_craft_list, craft_inv)
	for _, func in ipairs(core.registered_craft_predicts) do
		itemstack = func(itemstack, player, old_craft_list, craft_inv) or itemstack
	end
	return itemstack
end

-- Alias the forbidden item names to "" so they can't be
-- created via itemstrings (e.g. /give)
local name
for name in pairs(forbidden_item_names) do
	core.registered_aliases[name] = ""
	register_alias_raw(name, "")
end


-- Deprecated:
-- Aliases for core.register_alias (how ironic...)
--core.alias_node = core.register_alias
--core.alias_tool = core.register_alias
--core.alias_craftitem = core.register_alias

--
-- Built-in node definitions. Also defined in C.
--

core.register_item(":unknown", {
	type = "none",
	description = "Unknown Item",
	inventory_image = "unknown_item.png",
	on_place = core.item_place,
	on_drop = core.item_drop,
	groups = {not_in_creative_inventory=1},
	diggable = true,
})

core.register_node(":air", {
	description = "Air (you hacker you!)",
	inventory_image = "unknown_node.png",
	wield_image = "unknown_node.png",
	drawtype = "airlike",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true,
	air_equivalent = true,
	drop = "",
	groups = {not_in_creative_inventory=1},
})

core.register_node(":ignore", {
	description = "Ignore (you hacker you!)",
	inventory_image = "unknown_node.png",
	wield_image = "unknown_node.png",
	drawtype = "airlike",
	paramtype = "none",
	sunlight_propagates = false,
	walkable = false,
	pointable = false,
	diggable = false,
	buildable_to = true, -- A way to remove accidentally placed ignores
	air_equivalent = true,
	drop = "",
	groups = {not_in_creative_inventory=1},
})

-- The hand (bare definition)
core.register_item(":", {
	type = "none",
	groups = {not_in_creative_inventory=1},
})


function core.override_item(name, redefinition)
	if redefinition.name ~= nil then
		error("Attempt to redefine name of "..name.." to "..dump(redefinition.name), 2)
	end
	if redefinition.type ~= nil then
		error("Attempt to redefine type of "..name.." to "..dump(redefinition.type), 2)
	end
	local item = core.registered_items[name]
	if not item then
		error("Attempt to override non-existent item "..name, 2)
	end
	for k, v in pairs(redefinition) do
		rawset(item, k, v)
	end
	register_item_raw(item)
end


core.callback_origins = {}

function core.run_callbacks(callbacks, mode, ...)
	assert(type(callbacks) == "table")
	local cb_len = #callbacks
	if cb_len == 0 then
		if mode == 2 or mode == 3 then
			return true
		elseif mode == 4 or mode == 5 then
			return false
		end
	end
	local ret = nil
	for i = 1, cb_len do
		local origin = core.callback_origins[callbacks[i]]
		if origin then
			core.set_last_run_mod(origin.mod)
			--print("Running " .. tostring(callbacks[i]) ..
			--	" (a " .. origin.name .. " callback in " .. origin.mod .. ")")
		else
			--print("No data associated with callback")
		end
		local cb_ret = callbacks[i](...)

		if mode == 0 and i == 1 then
			ret = cb_ret
		elseif mode == 1 and i == cb_len then
			ret = cb_ret
		elseif mode == 2 then
			if not cb_ret or i == 1 then
				ret = cb_ret
			end
		elseif mode == 3 then
			if cb_ret then
				return cb_ret
			end
			ret = cb_ret
		elseif mode == 4 then
			if (cb_ret and not ret) or i == 1 then
				ret = cb_ret
			end
		elseif mode == 5 and cb_ret then
			return cb_ret
		end
	end
	return ret
end

--
-- Callback registration
--

local function make_registration()
	local t = {}
	local registerfunc = function(func)
		table.insert(t, func)
		core.callback_origins[func] = {
			mod = core.get_current_modname() or "??",
			name = debug.getinfo(1, "n").name or "??"
		}
		--local origin = core.callback_origins[func]
		--print(origin.name .. ": " .. origin.mod .. " registering cbk " .. tostring(func))
	end
	return t, registerfunc
end

local function make_registration_reverse()
	local t = {}
	local registerfunc = function(func)
		table.insert(t, 1, func)
		core.callback_origins[func] = {
			mod = core.get_current_modname() or "??",
			name = debug.getinfo(1, "n").name or "??"
		}
		--local origin = core.callback_origins[func]
		--print(origin.name .. ": " .. origin.mod .. " registering cbk " .. tostring(func))
	end
	return t, registerfunc
end

local function make_registration_wrap(reg_fn_name, clear_fn_name)
	local list = {}

	local orig_reg_fn = core[reg_fn_name]
	core[reg_fn_name] = function(def)
		local retval = orig_reg_fn(def)
		if retval ~= nil then
			if def.name ~= nil then
				list[def.name] = def
			else
				list[retval] = def
			end
		end
		return retval
	end

	local orig_clear_fn = core[clear_fn_name]
	core[clear_fn_name] = function()
		for k in pairs(list) do
			list[k] = nil
		end
		return orig_clear_fn()
	end

	return list
end

core.registered_on_player_hpchanges = { modifiers = { }, loggers = { } }

function core.registered_on_player_hpchange(player, hp_change)
	local last = false
	for i = #core.registered_on_player_hpchanges.modifiers, 1, -1 do
		local func = core.registered_on_player_hpchanges.modifiers[i]
		hp_change, last = func(player, hp_change)
		if type(hp_change) ~= "number" then
			local debuginfo = debug.getinfo(func)
			error("The register_on_hp_changes function has to return a number at " ..
				debuginfo.short_src .. " line " .. debuginfo.linedefined)
		end
		if last then
			break
		end
	end
	for i, func in ipairs(core.registered_on_player_hpchanges.loggers) do
		func(player, hp_change)
	end
	return hp_change
end

function core.register_on_player_hpchange(func, modifier)
	if modifier then
		table.insert(core.registered_on_player_hpchanges.modifiers, func)
	else
		table.insert(core.registered_on_player_hpchanges.loggers, func)
	end
	core.callback_origins[func] = {
		mod = core.get_current_modname() or "??",
		name = debug.getinfo(1, "n").name or "??"
	}
end

core.registered_biomes      = make_registration_wrap("register_biome",      "clear_registered_biomes")
core.registered_ores        = make_registration_wrap("register_ore",        "clear_registered_ores")
core.registered_decorations = make_registration_wrap("register_decoration", "clear_registered_decorations")

core.registered_on_chat_messages, core.register_on_chat_message = make_registration()
core.registered_globalsteps, core.register_globalstep = make_registration()
core.registered_playerevents, core.register_playerevent = make_registration()
core.registered_on_shutdown, core.register_on_shutdown = make_registration()
core.registered_on_punchnodes, core.register_on_punchnode = make_registration()
core.registered_on_placenodes, core.register_on_placenode = make_registration()
core.registered_on_dignodes, core.register_on_dignode = make_registration()
core.registered_on_generateds, core.register_on_generated = make_registration()
core.registered_on_newplayers, core.register_on_newplayer = make_registration()
core.registered_on_dieplayers, core.register_on_dieplayer = make_registration()
core.registered_on_respawnplayers, core.register_on_respawnplayer = make_registration()
core.registered_on_prejoinplayers, core.register_on_prejoinplayer = make_registration()
core.registered_on_joinplayers, core.register_on_joinplayer = make_registration()
core.registered_on_leaveplayers, core.register_on_leaveplayer = make_registration()
core.registered_on_player_receive_fields, core.register_on_player_receive_fields = make_registration_reverse()
core.registered_on_cheats, core.register_on_cheat = make_registration()
core.registered_on_crafts, core.register_on_craft = make_registration()
core.registered_craft_predicts, core.register_craft_predict = make_registration()
core.registered_on_protection_violation, core.register_on_protection_violation = make_registration()
core.registered_on_item_eats, core.register_on_item_eat = make_registration()
core.registered_on_punchplayers, core.register_on_punchplayer = make_registration()

--
-- Compatibility for on_mapgen_init()
--

core.register_on_mapgen_init = function(func) func(core.get_mapgen_params()) end

