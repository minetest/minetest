-- legacy (Minetest 0.4 mod)
-- Provides as much backwards-compatibility as feasible

--
-- Aliases to support loading 0.3 and old 0.4 worlds and inventories
--

minetest.register_alias("stone", "default:stone")
minetest.register_alias("stone_with_coal", "default:stone_with_coal")
minetest.register_alias("stone_with_iron", "default:stone_with_iron")
minetest.register_alias("dirt_with_grass", "default:dirt_with_grass")
minetest.register_alias("dirt_with_grass_footsteps", "default:dirt_with_grass_footsteps")
minetest.register_alias("dirt", "default:dirt")
minetest.register_alias("sand", "default:sand")
minetest.register_alias("gravel", "default:gravel")
minetest.register_alias("sandstone", "default:sandstone")
minetest.register_alias("clay", "default:clay")
minetest.register_alias("brick", "default:brick")
minetest.register_alias("tree", "default:tree")
minetest.register_alias("jungletree", "default:jungletree")
minetest.register_alias("junglegrass", "default:junglegrass")
minetest.register_alias("leaves", "default:leaves")
minetest.register_alias("cactus", "default:cactus")
minetest.register_alias("papyrus", "default:papyrus")
minetest.register_alias("bookshelf", "default:bookshelf")
minetest.register_alias("glass", "default:glass")
minetest.register_alias("wooden_fence", "default:fence_wood")
minetest.register_alias("rail", "default:rail")
minetest.register_alias("ladder", "default:ladder")
minetest.register_alias("wood", "default:wood")
minetest.register_alias("mese", "default:mese")
minetest.register_alias("cloud", "default:cloud")
minetest.register_alias("water_flowing", "default:water_flowing")
minetest.register_alias("water_source", "default:water_source")
minetest.register_alias("lava_flowing", "default:lava_flowing")
minetest.register_alias("lava_source", "default:lava_source")
minetest.register_alias("torch", "default:torch")
minetest.register_alias("sign_wall", "default:sign_wall")
minetest.register_alias("furnace", "default:furnace")
minetest.register_alias("chest", "default:chest")
minetest.register_alias("locked_chest", "default:chest_locked")
minetest.register_alias("cobble", "default:cobble")
minetest.register_alias("mossycobble", "default:mossycobble")
minetest.register_alias("steelblock", "default:steelblock")
minetest.register_alias("nyancat", "default:nyancat")
minetest.register_alias("nyancat_rainbow", "default:nyancat_rainbow")
minetest.register_alias("sapling", "default:sapling")
minetest.register_alias("apple", "default:apple")

minetest.register_alias("WPick", "default:pick_wood")
minetest.register_alias("STPick", "default:pick_stone")
minetest.register_alias("SteelPick", "default:pick_steel")
minetest.register_alias("MesePick", "default:pick_mese")
minetest.register_alias("WShovel", "default:shovel_wood")
minetest.register_alias("STShovel", "default:shovel_stone")
minetest.register_alias("SteelShovel", "default:shovel_steel")
minetest.register_alias("WAxe", "default:axe_wood")
minetest.register_alias("STAxe", "default:axe_stone")
minetest.register_alias("SteelAxe", "default:axe_steel")
minetest.register_alias("WSword", "default:sword_wood")
minetest.register_alias("STSword", "default:sword_stone")
minetest.register_alias("SteelSword", "default:sword_steel")

minetest.register_alias("Stick", "default:stick")
minetest.register_alias("paper", "default:paper")
minetest.register_alias("book", "default:book")
minetest.register_alias("lump_of_coal", "default:coal_lump")
minetest.register_alias("lump_of_iron", "default:iron_lump")
minetest.register_alias("lump_of_clay", "default:clay_lump")
minetest.register_alias("steel_ingot", "default:steel_ingot")
minetest.register_alias("clay_brick", "default:clay_brick")
minetest.register_alias("scorched_stuff", "default:scorched_stuff")

--
-- Old items
--

minetest.register_craftitem(":rat", {
	description = "Rat",
	inventory_image = "rat.png",
	on_drop = function(item, dropper, pos)
		minetest.env:add_rat(pos)
		item:take_item()
		return item
	end,
	on_place = function(item, dropped, pointed)
		pos = minetest.get_pointed_thing_position(pointed, true)
		if pos ~= nil then
			minetest.env:add_rat(pos)
			item:take_item()
			return item
		end
	end
})

minetest.register_craftitem(":cooked_rat", {
	description = "Cooked rat",
	inventory_image = "cooked_rat.png",
	on_use = minetest.item_eat(6),
})

minetest.register_craftitem(":firefly", {
	description = "Firefly",
	inventory_image = "firefly.png",
	on_drop = function(item, dropper, pos)
		minetest.env:add_firefly(pos)
		item:take_item()
		return item
	end,
	on_place = function(item, dropped, pointed)
		pos = minetest.get_pointed_thing_position(pointed, true)
		if pos ~= nil then
			minetest.env:add_firefly(pos)
			item:take_item()
			return item
		end
	end
})

minetest.register_craft({
	type = "cooking",
	output = "cooked_rat",
	recipe = "rat",
})

minetest.register_craft({
	type = "cooking",
	output = "scorched_stuff",
	recipe = "cooked_rat",
})

-- END
