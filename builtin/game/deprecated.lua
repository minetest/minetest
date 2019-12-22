-- Minetest: builtin/deprecated.lua

--
-- Default material types
--
local function digprop_err()
	minetest.log("deprecated", "The minetest.digprop_* functions are obsolete and need to be replaced by item groups.")
end

minetest.digprop_constanttime = digprop_err
minetest.digprop_stonelike = digprop_err
minetest.digprop_dirtlike = digprop_err
minetest.digprop_gravellike = digprop_err
minetest.digprop_woodlike = digprop_err
minetest.digprop_leaveslike = digprop_err
minetest.digprop_glasslike = digprop_err

function minetest.node_metadata_inventory_move_allow_all()
	minetest.log("deprecated", "minetest.node_metadata_inventory_move_allow_all is obsolete and does nothing.")
end

function minetest.add_to_creative_inventory(itemstring)
	minetest.log("deprecated", "minetest.add_to_creative_inventory is obsolete and does nothing.")
end

--
-- EnvRef
--
minetest.env = {}
local envref_deprecation_message_printed = false
setmetatable(minetest.env, {
	__index = function(table, key)
		if not envref_deprecation_message_printed then
			minetest.log("deprecated", "minetest.env:[...] is deprecated and should be replaced with minetest.[...]")
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

function minetest.rollback_get_last_node_actor(pos, range, seconds)
	return minetest.rollback_get_node_actions(pos, range, seconds, 1)[1]
end

--
-- minetest.setting_*
--

local settings = minetest.settings

local function setting_proxy(name)
	return function(...)
		minetest.log("deprecated", "WARNING: minetest.setting_* "..
			"functions are deprecated.  "..
			"Use methods on the minetest.settings object.")
		return settings[name](settings, ...)
	end
end

minetest.setting_set = setting_proxy("set")
minetest.setting_get = setting_proxy("get")
minetest.setting_setbool = setting_proxy("set_bool")
minetest.setting_getbool = setting_proxy("get_bool")
minetest.setting_save = setting_proxy("write")
