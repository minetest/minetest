-- Minetest: builtin/detached_inventory.lua

core.detached_inventories = {}

function core.create_detached_inventory(name, callbacks, player_name)
	local stuff = {}
	stuff.name = name
	if callbacks then
		stuff.allow_move = callbacks.allow_move
		stuff.allow_put = callbacks.allow_put
		stuff.allow_take = callbacks.allow_take
		stuff.on_move = callbacks.on_move
		stuff.on_put = callbacks.on_put
		stuff.on_take = callbacks.on_take
	end
	stuff.mod_origin = core.get_current_modname() or "??"
	core.detached_inventories[name] = stuff
	return core.create_detached_inventory_raw(name, player_name)
end

function core.remove_detached_inventory(name)
	core.detached_inventories[name] = nil
	return core.remove_detached_inventory_raw(name)
end
