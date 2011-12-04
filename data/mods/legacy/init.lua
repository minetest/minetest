-- legacy (Minetest 0.4 mod)
-- Provides as much backwards-compatibility as feasible

--
-- Aliases to support loading 0.3 and old 0.4 worlds and inventories
--

minetest.alias_node("stone", "default:stone")
minetest.alias_node("dirt_with_grass", "default:dirt_with_grass")
minetest.alias_node("dirt_with_grass_footsteps", "default:dirt_with_grass_footsteps")
minetest.alias_node("dirt", "default:dirt")
minetest.alias_node("sand", "default:sand")
minetest.alias_node("gravel", "default:gravel")
minetest.alias_node("sandstone", "default:sandstone")
minetest.alias_node("clay", "default:clay")
minetest.alias_node("brick", "default:brick")
minetest.alias_node("tree", "default:tree")
minetest.alias_node("jungletree", "default:jungletree")
minetest.alias_node("junglegrass", "default:junglegrass")
minetest.alias_node("leaves", "default:leaves")
minetest.alias_node("cactus", "default:cactus")
minetest.alias_node("papyrus", "default:papyrus")
minetest.alias_node("bookshelf", "default:bookshelf")
minetest.alias_node("glass", "default:glass")
minetest.alias_node("wooden_fence", "default:fence_wood")
minetest.alias_node("rail", "default:rail")
minetest.alias_node("ladder", "default:ladder")
minetest.alias_node("wood", "default:wood")
minetest.alias_node("mese", "default:mese")
minetest.alias_node("cloud", "default:cloud")
minetest.alias_node("water_flowing", "default:water_flowing")
minetest.alias_node("water_source", "default:water_source")
minetest.alias_node("lava_flowing", "default:lava_flowing")
minetest.alias_node("lava_source", "default:lava_source")
minetest.alias_node("torch", "default:torch")
minetest.alias_node("sign_wall", "default:sign_wall")
minetest.alias_node("furnace", "default:furnace")
minetest.alias_node("chest", "default:chest")
minetest.alias_node("locked_chest", "default:chest_locked")
minetest.alias_node("cobble", "default:cobble")
minetest.alias_node("mossycobble", "default:mossycobble")
minetest.alias_node("steelblock", "default:steelblock")
minetest.alias_node("nyancat", "default:nyancat")
minetest.alias_node("nyancat_rainbow", "default:nyancat_rainbow")
minetest.alias_node("sapling", "default:sapling")
minetest.alias_node("apple", "default:apple")

minetest.alias_tool("WPick", "default:pick_wood")
minetest.alias_tool("STPick", "default:pick_stone")
minetest.alias_tool("SteelPick", "default:pick_steel")
minetest.alias_tool("MesePick", "default:pick_mese")
minetest.alias_tool("WShovel", "default:shovel_wood")
minetest.alias_tool("STShovel", "default:shovel_stone")
minetest.alias_tool("SteelShovel", "default:shovel_steel")
minetest.alias_tool("WAxe", "default:axe_wood")
minetest.alias_tool("STAxe", "default:axe_stone")
minetest.alias_tool("SteelAxe", "default:axe_steel")
minetest.alias_tool("WSword", "default:sword_wood")
minetest.alias_tool("STSword", "default:sword_stone")
minetest.alias_tool("SteelSword", "default:sword_steel")

minetest.alias_craftitem("Stick", "default:stick")
minetest.alias_craftitem("paper", "default:paper")
minetest.alias_craftitem("book", "default:book")
minetest.alias_craftitem("lump_of_coal", "default:coal_lump")
minetest.alias_craftitem("lump_of_iron", "default:iron_lump")
minetest.alias_craftitem("lump_of_clay", "default:clay_lump")
minetest.alias_craftitem("steel_ingot", "default:steel_ingot")
minetest.alias_craftitem("clay_brick", "default:clay_brick")
minetest.alias_craftitem("scorched_stuff", "default:scorched_stuff")
minetest.alias_craftitem("apple", "default:apple")

--
-- Old items
--

minetest.register_craftitem(":rat", {
	image = "rat.png",
	cookresult_itemstring = 'craft "cooked_rat" 1',
	on_drop = function(item, dropper, pos)
		minetest.env:add_rat(pos)
		return true
	end,
})

minetest.register_craftitem(":cooked_rat", {
	image = "cooked_rat.png",
	cookresult_itemstring = 'craft "scorched_stuff" 1',
	on_place_on_ground = minetest.craftitem_place_item,
	on_use = minetest.craftitem_eat(6),
})

minetest.register_craftitem(":firefly", {
	image = "firefly.png",
	on_drop = function(item, dropper, pos)
		minetest.env:add_firefly(pos)
		return true
	end,
})

-- END
