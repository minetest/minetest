-- Minetest: builtin/deprecated.lua

--
-- Default material types
--
function digprop_err()
	core.log("info", debug.traceback())
	core.log("info", "WARNING: The core.digprop_* functions are obsolete and need to be replaced by item groups.")
end

core.digprop_constanttime = digprop_err
core.digprop_stonelike = digprop_err
core.digprop_dirtlike = digprop_err
core.digprop_gravellike = digprop_err
core.digprop_woodlike = digprop_err
core.digprop_leaveslike = digprop_err
core.digprop_glasslike = digprop_err

core.node_metadata_inventory_move_allow_all = function()
	core.log("info", "WARNING: core.node_metadata_inventory_move_allow_all is obsolete and does nothing.")
end

core.add_to_creative_inventory = function(itemstring)
	core.log('info', "WARNING: core.add_to_creative_inventory: This function is deprecated and does nothing.")
end

--
-- EnvRef
--
core.env = {}
local envref_deprecation_message_printed = false
setmetatable(core.env, {
	__index = function(table, key)
		if not envref_deprecation_message_printed then
			core.log("info", "WARNING: core.env:[...] is deprecated and should be replaced with core.[...]")
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

