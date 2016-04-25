if minetest.get_modpath("technic") then
	local stats = {
		brass = { name="Brass", armor=1.8, heal=0, use=650 },
		cast = { name="Cast Iron", armor=2.5, heal=8, use=200 },
		carbon = { name="Carbon Steel", armor=2.7, heal=10, use=100 },
		stainless = { name="Stainless Steel", armor=2.7, heal=10, use=75 },
	}
	local mats = {
		brass="technic:brass_ingot",
		cast="technic:cast_iron_ingot",
		carbon="technic:carbon_steel_ingot",
		stainless="technic:stainless_steel_ingot",
	}
	if minetest.get_modpath("moreores") then
		stats.tin = { name="Tin", armor=1.6, heal=0, use=750 }
		stats.silver = { name="Silver", armor=1.8, heal=6, use=650 }
		mats.tin = "moreores:tin_ingot"
		mats.silver = "moreores:silver_ingot"
	end

	for k, v in pairs(stats) do
		minetest.register_tool("technic_armor:helmet_"..k, {
			description = v.name.." Helmet",
			inventory_image = "technic_armor_inv_helmet_"..k..".png",
			groups = {armor_head=math.floor(5*v.armor), armor_heal=v.heal, armor_use=v.use},
			wear = 0,
		})
		minetest.register_tool("technic_armor:chestplate_"..k, {
			description = v.name.." Chestplate",
			inventory_image = "technic_armor_inv_chestplate_"..k..".png",
			groups = {armor_torso=math.floor(8*v.armor), armor_heal=v.heal, armor_use=v.use},
			wear = 0,
		})
		minetest.register_tool("technic_armor:leggings_"..k, {
			description = v.name.." Leggings",
			inventory_image = "technic_armor_inv_leggings_"..k..".png",
			groups = {armor_legs=math.floor(7*v.armor), armor_heal=v.heal, armor_use=v.use},
			wear = 0,
		})
		minetest.register_tool("technic_armor:boots_"..k, {
			description = v.name.." Boots",
			inventory_image = "technic_armor_inv_boots_"..k..".png",
			groups = {armor_feet=math.floor(4*v.armor), armor_heal=v.heal, armor_use=v.use},
			wear = 0,
		})
	end
	for k, v in pairs(mats) do
		minetest.register_craft({
			output = "technic_armor:helmet_"..k,
			recipe = {
				{v, v, v},
				{v, "", v},
				{"", "", ""},
			},
		})
		minetest.register_craft({
			output = "technic_armor:chestplate_"..k,
			recipe = {
				{v, "", v},
				{v, v, v},
				{v, v, v},
			},
		})
		minetest.register_craft({
			output = "technic_armor:leggings_"..k,
			recipe = {
				{v, v, v},
				{v, "", v},
				{v, "", v},
			},
		})
		minetest.register_craft({
			output = "technic_armor:boots_"..k,
			recipe = {
				{v, "", v},
				{v, "", v},
			},
		})
	end

	if minetest.get_modpath("shields") then
		for k, v in pairs(stats) do
			minetest.register_tool("technic_armor:shield_"..k, {
				description = v.name.." Shield",
				inventory_image = "technic_armor_inv_shield_"..k..".png",
				groups = {armor_shield=math.floor(5*v.armor), armor_heal=v.heal, armor_use=v.use},
				wear = 0,
			})
			local m = mats[k]
			minetest.register_craft({
				output = "technic_armor:shield_"..k,
				recipe = {
					{m, m, m},
					{m, m, m},
					{"", m, ""},
				},
			})
		end
	end
end

