-- Minetest: builtin/deprecated.lua

--
-- Default material types
--
local function digprop_err()
	core.log("deprecated", "The core.digprop_* functions are obsolete and need to be replaced by item groups.")
end

core.digprop_constanttime = digprop_err
core.digprop_stonelike = digprop_err
core.digprop_dirtlike = digprop_err
core.digprop_gravellike = digprop_err
core.digprop_woodlike = digprop_err
core.digprop_leaveslike = digprop_err
core.digprop_glasslike = digprop_err

function core.node_metadata_inventory_move_allow_all()
	core.log("deprecated", "core.node_metadata_inventory_move_allow_all is obsolete and does nothing.")
end

function core.add_to_creative_inventory(itemstring)
	core.log("deprecated", "core.add_to_creative_inventory: This function is deprecated and does nothing.")
end

--
-- EnvRef
--
core.env = {}
local envref_deprecation_message_printed = false
setmetatable(core.env, {
	__index = function(table, key)
		if not envref_deprecation_message_printed then
			core.log("deprecated", "core.env:[...] is deprecated and should be replaced with core.[...]")
			envref_deprecation_message_printed = true
		end
		local func = core[key]
		if type(func) == "function" then
			rawset(table, key, function(self, ...)
				return func(...)
			end)
		else
			rawset(table, key, nil)
		end
		return rawget(table, key)
	end
})

function core.rollback_get_last_node_actor(pos, range, seconds)
	return core.rollback_get_node_actions(pos, range, seconds, 1)[1]
end

local function create_alias(old_name,new_name, warn)
	core[old_name] = function(...)
		if warn then
			core.log("deprecated", old_name .. " is deprecated and has been replaced by " .. new_name)
		end
		return core[new_name](...)
	end
end

create_alias("add_particlespawner", "add_particle_spawner")
create_alias("delete_particlespawner", "delete_particle_spawner")
create_alias("forceload_block", "force_load_block")
create_alias("forceload_free_block", "force_load_free_block")
create_alias("get_current_modname", "get_current_mod_name")
create_alias("get_worldpath", "get_world_path")
create_alias("get_modpath", "get_mod_path")
create_alias("set_timeofday", "set_time_of_day")
create_alias("get_timeofday", "get_time_of_day")
create_alias("get_gametime", "get_game_time")
create_alias("set_noiseparams", "set_noise_params")
create_alias("get_noiseparams", "get_noise_params")
create_alias("get_modnames", "get_mod_names")
create_alias("setting_setbool", "setting_set_bool")
create_alias("setting_getbool", "setting_get_bool")