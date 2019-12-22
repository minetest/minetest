-- Minetest: builtin/detached_inventory.lua

minetest.detached_inventories = {}

function minetest.create_detached_inventory(name, callbacks, player_name)
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
	stuff.mod_origin = minetest.get_current_modname() or "??"
	minetest.detached_inventories[name] = stuff
	return minetest.create_detached_inventory_raw(name, player_name)
end

function minetest.remove_detached_inventory(name)
	minetest.detached_inventories[name] = nil
	return minetest.remove_detached_inventory_raw(name)
end
