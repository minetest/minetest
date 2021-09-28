
core.callback_origins = {}

local getinfo = debug.getinfo
debug.getinfo = nil

--- Runs given callbacks.
--
-- Note: this function is also called from C++
-- @tparam table  callbacks a table with registered callbacks, like `core.registered_on_*`
-- @tparam number mode      a RunCallbacksMode, as defined in src/script/common/c_internal.h
-- @param         ...       arguments for the callback
-- @return depends on mode
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
	local ret
	for i = 1, cb_len do
		local cb_ret = callbacks[i](...)

		if mode == 0 and i == 1 or mode == 1 and i == cb_len then
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
		t[#t + 1] = func
		core.callback_origins[func] = {
			mod = core.get_current_modname() or "??",
			name = getinfo(1, "n").name or "??"
		}
		--local origin = core.callback_origins[func]
		--print(origin.name .. ": " .. origin.mod .. " registering cbk " .. tostring(func))
	end
	return t, registerfunc
end

core.registered_globalsteps, core.register_globalstep = make_registration()
core.registered_on_mods_loaded, core.register_on_mods_loaded = make_registration()
core.registered_on_shutdown, core.register_on_shutdown = make_registration()
core.registered_on_receiving_chat_message, core.register_on_receiving_chat_message = make_registration()
core.registered_on_sending_chat_message, core.register_on_sending_chat_message = make_registration()
core.registered_on_chatcommand, core.register_on_chatcommand = make_registration()
core.registered_on_death, core.register_on_death = make_registration()
core.registered_on_hp_modification, core.register_on_hp_modification = make_registration()
core.registered_on_damage_taken, core.register_on_damage_taken = make_registration()
core.registered_on_formspec_input, core.register_on_formspec_input = make_registration()
core.registered_on_dignode, core.register_on_dignode = make_registration()
core.registered_on_punchnode, core.register_on_punchnode = make_registration()
core.registered_on_placenode, core.register_on_placenode = make_registration()
core.registered_on_item_use, core.register_on_item_use = make_registration()
core.registered_on_modchannel_message, core.register_on_modchannel_message = make_registration()
core.registered_on_modchannel_signal, core.register_on_modchannel_signal = make_registration()
core.registered_on_inventory_open, core.register_on_inventory_open = make_registration()
