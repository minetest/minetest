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

--
-- core.setting_*
--

local settings = core.settings

local function setting_proxy(name)
	return function(...)
		core.log("deprecated", "WARNING: core.setting_* "..
			"functions are deprecated.  "..
			"Use methods on the core.settings object.")
		return settings[name](settings, ...)
	end
end

core.setting_set = setting_proxy("set")
core.setting_get = setting_proxy("get")
core.setting_setbool = setting_proxy("set_bool")
core.setting_getbool = setting_proxy("get_bool")
core.setting_save = setting_proxy("write")

--
-- core.register_on_auth_fail
--

function core.register_on_auth_fail(func)
	core.log("deprecated", "core.register_on_auth_fail " ..
		"is deprecated and should be replaced by " ..
		"core.register_on_authplayer instead.")

	core.register_on_authplayer(function (player_name, ip, is_success)
		if not is_success then
			func(player_name, ip)
		end
	end)
end

--
-- core.get_all_craft_recipes
--

local old_get_all_craft_recipes = core.get_all_craft_recipes
function core.get_all_craft_recipes(...)
	core.log("deprecated", "core.get_all_craft_recipes " ..
		"is deprecated, use core.registered_crafts instead.")
	old_get_all_craft_recipes(...)
end

--
-- core.get_craft_recipe
--

local old_get_craft_recipe = core.get_craft_recipe
function core.get_craft_recipe(...)
	core.log("deprecated", "core.get_craft_recipe " ..
		"is deprecated, use core.registered_crafts instead.")
	old_get_craft_recipe(...)
end

--
-- core.get_craft_result
--

local old_get_craft_result = core.get_craft_result
function core.get_craft_result(...)
	core.log("deprecated", "core.get_craft_result " ..
		"is deprecated, use core.registered_crafts instead.")
	old_get_craft_result(...)
end
