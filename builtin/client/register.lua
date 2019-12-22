
minetest.callback_origins = {}

local getinfo = debug.getinfo
debug.getinfo = nil

function minetest.run_callbacks(callbacks, mode, ...)
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
		minetest.callback_origins[func] = {
			mod = minetest.get_current_modname() or "??",
			name = getinfo(1, "n").name or "??"
		}
		--local origin = minetest.callback_origins[func]
		--print(origin.name .. ": " .. origin.mod .. " registering cbk " .. tostring(func))
	end
	return t, registerfunc
end

minetest.registered_globalsteps, minetest.register_globalstep = make_registration()
minetest.registered_on_mods_loaded, minetest.register_on_mods_loaded = make_registration()
minetest.registered_on_shutdown, minetest.register_on_shutdown = make_registration()
minetest.registered_on_receiving_chat_message, minetest.register_on_receiving_chat_message = make_registration()
minetest.registered_on_sending_chat_message, minetest.register_on_sending_chat_message = make_registration()
minetest.registered_on_death, minetest.register_on_death = make_registration()
minetest.registered_on_hp_modification, minetest.register_on_hp_modification = make_registration()
minetest.registered_on_damage_taken, minetest.register_on_damage_taken = make_registration()
minetest.registered_on_formspec_input, minetest.register_on_formspec_input = make_registration()
minetest.registered_on_dignode, minetest.register_on_dignode = make_registration()
minetest.registered_on_punchnode, minetest.register_on_punchnode = make_registration()
minetest.registered_on_placenode, minetest.register_on_placenode = make_registration()
minetest.registered_on_item_use, minetest.register_on_item_use = make_registration()
minetest.registered_on_modchannel_message, minetest.register_on_modchannel_message = make_registration()
minetest.registered_on_modchannel_signal, minetest.register_on_modchannel_signal = make_registration()
minetest.registered_on_inventory_open, minetest.register_on_inventory_open = make_registration()
