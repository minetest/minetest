minetest.register_alias("adminboots","3d_armor:boots_admin")
minetest.register_alias("adminhelmet","3d_armor:helmet_admin")
minetest.register_alias("adminchestplate","3d_armor:chestplate_admin")
minetest.register_alias("adminlegginss","3d_armor:leggings_admin")

minetest.register_tool("3d_armor:helmet_admin", {
	description = "Admin Helmet",
	inventory_image = "3d_armor_inv_helmet_admin.png",
	groups = {armor_head=1000, armor_heal=1000, armor_use=0, armor_water=1, not_in_creative_inventory=1},
	wear = 0,
	on_drop = function(itemstack, dropper, pos)
		return
	end,
})

minetest.register_tool("3d_armor:chestplate_admin", {
	description = "Admin Chestplate",
	inventory_image = "3d_armor_inv_chestplate_admin.png",
	groups = {armor_torso=1000, armor_heal=1000, armor_use=0, not_in_creative_inventory=1},
	wear = 0,
	on_drop = function(itemstack, dropper, pos)
		return
	end,
})

minetest.register_tool("3d_armor:leggings_admin", {
	description = "Admin Leggings",
	inventory_image = "3d_armor_inv_leggings_admin.png",
	groups = {armor_legs=1000, armor_heal=1000, armor_use=0, not_in_creative_inventory=1},
	wear = 0,
	on_drop = function(itemstack, dropper, pos)
		return
	end,
})

minetest.register_tool("3d_armor:boots_admin", {
	description = "Admin Boots",
	inventory_image = "3d_armor_inv_boots_admin.png",
	groups = {armor_feet=1000, armor_heal=1000, armor_use=0, not_in_creative_inventory=1},
	wear = 0,
	on_drop = function(itemstack, dropper, pos)
		return
	end,
})

