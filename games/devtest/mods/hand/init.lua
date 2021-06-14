minetest.register_chatcommand("sethand", {
	params = "<itemstring>",
	description = "Set hand item",
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return false, "No player."
		end

		local itemstack = ItemStack(param)
		if not itemstack:is_known() then
			return false, "Item type does not exist."
		end

		player:get_inventory():set_size("hand", 1)
		player:get_inventory():set_list("hand", {itemstack})
		return true
	end,
})
