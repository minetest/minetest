-- Minetest: builtin/detached_inventory.lua

core.detached_inventories = {}

function core.create_detached_inventory(name, callbacks)
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
	core.detached_inventories[name] = stuff
	return core.create_detached_inventory_raw(name)
end

