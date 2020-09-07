--
-- Tool definitions
--

--[[ TOOLS SUMMARY:

Tool types:

* Hand: basic tool/weapon (just for convenience, not optimized for testing)
* Pickaxe: dig cracky
* Axe: dig choppy
* Shovel: dig crumbly
* Shears: dig snappy
* Sword: deal damage
* Dagger: deal damage, but faster

Tool materials:

* Dirt: dig nodes of rating 3, one use only
* Wood: dig nodes of rating 3
* Stone: dig nodes of rating 3 or 2
* Steel: dig nodes of rating 3, 2 or 1
* Mese: dig "everything" instantly
]]

-- The hand
minetest.register_item(":", {
	type = "none",
	wield_image = "wieldhand.png",
	wield_scale = {x=1,y=1,z=2.5},
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level = 0,
		groupcaps = {
			crumbly = {times={[3]=1.50}, uses=0, maxlevel=0},
			snappy = {times={[3]=1.50}, uses=0, maxlevel=0},
			oddly_breakable_by_hand = {times={[1]=7.00,[2]=4.00,[3]=2.00}, uses=0, maxlevel=0},
		},
		damage_groups = {fleshy=1},
	}
})

-- Mese Pickaxe: special tool that digs "everything" instantly
minetest.register_tool("basetools:pick_mese", {
	description = "Mese Pickaxe".."\n"..
			"Digs diggable nodes instantly",
	inventory_image = "basetools_mesepick.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=3,
		groupcaps={
			cracky={times={[1]=0.0, [2]=0.0, [3]=0.0}, maxlevel=255},
			crumbly={times={[1]=0.0, [2]=0.0, [3]=0.0}, maxlevel=255},
			snappy={times={[1]=0.0, [2]=0.0, [3]=0.0}, maxlevel=255},
			choppy={times={[1]=0.0, [2]=0.0, [3]=0.0}, maxlevel=255},
			dig_immediate={times={[1]=0.0, [2]=0.0, [3]=0.0}, maxlevel=255},
		},
		damage_groups = {fleshy=100},
	},
})


--
-- Pickaxes: Dig cracky
--

-- This should break after only 1 use
minetest.register_tool("basetools:pick_dirt", {
	description = "Dirt Pickaxe".."\n"..
		"Digs cracky=3".."\n"..
		"1 use only",
	inventory_image = "basetools_dirtpick.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			cracky={times={[3]=2.00}, uses=1, maxlevel=0}
		},
	},
})

minetest.register_tool("basetools:pick_wood", {
	description = "Wooden Pickaxe".."\n"..
		"Digs cracky=3",
	inventory_image = "basetools_woodpick.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			cracky={times={[3]=2.00}, uses=30, maxlevel=0}
		},
	},
})
minetest.register_tool("basetools:pick_stone", {
	description = "Stone Pickaxe".."\n"..
		"Digs cracky=2..3",
	inventory_image = "basetools_stonepick.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			cracky={times={[2]=1.20, [3]=0.80}, uses=60, maxlevel=0}
		},
	},
})
minetest.register_tool("basetools:pick_steel", {
	description = "Steel Pickaxe".."\n"..
		"Digs cracky=1..3",
	inventory_image = "basetools_steelpick.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			cracky={times={[1]=4.00, [2]=1.60, [3]=1.00}, uses=90, maxlevel=0}
		},
	},
})
minetest.register_tool("basetools:pick_steel_l1", {
	description = "Steel Pickaxe Level 1".."\n"..
		"Digs cracky=1..3".."\n"..
		"maxlevel=1",
	inventory_image = "basetools_steelpick_l1.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			cracky={times={[1]=4.00, [2]=1.60, [3]=1.00}, uses=90, maxlevel=1}
		},
	},
})
minetest.register_tool("basetools:pick_steel_l2", {
	description = "Steel Pickaxe Level 2".."\n"..
		"Digs cracky=1..3".."\n"..
		"maxlevel=2",
	inventory_image = "basetools_steelpick_l2.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			cracky={times={[1]=4.00, [2]=1.60, [3]=1.00}, uses=90, maxlevel=2}
		},
	},
})

--
-- Shovels (dig crumbly)
--

minetest.register_tool("basetools:shovel_wood", {
	description = "Wooden Shovel".."\n"..
		"Digs crumbly=3",
	inventory_image = "basetools_woodshovel.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			crumbly={times={[3]=0.50}, uses=30, maxlevel=0}
		},
	},
})
minetest.register_tool("basetools:shovel_stone", {
	description = "Stone Shovel".."\n"..
		"Digs crumbly=2..3",
	inventory_image = "basetools_stoneshovel.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			crumbly={times={[2]=0.50, [3]=0.30}, uses=60, maxlevel=0}
		},
	},
})
minetest.register_tool("basetools:shovel_steel", {
	description = "Steel Shovel".."\n"..
		"Digs crumbly=1..3",
	inventory_image = "basetools_steelshovel.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			crumbly={times={[1]=1.00, [2]=0.70, [3]=0.60}, uses=90, maxlevel=0}
		},
	},
})

--
-- Axes (dig choppy)
--

minetest.register_tool("basetools:axe_wood", {
	description = "Wooden Axe".."\n"..
		"Digs choppy=3",
	inventory_image = "basetools_woodaxe.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			choppy={times={[3]=0.80}, uses=30, maxlevel=0},
		},
	},
})
minetest.register_tool("basetools:axe_stone", {
	description = "Stone Axe".."\n"..
		"Digs choppy=2..3",
	inventory_image = "basetools_stoneaxe.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			choppy={times={[2]=1.00, [3]=0.60}, uses=60, maxlevel=0},
		},
	},
})
minetest.register_tool("basetools:axe_steel", {
	description = "Steel Axe".."\n"..
		"Digs choppy=1..3",
	inventory_image = "basetools_steelaxe.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			choppy={times={[1]=2.00, [2]=0.80, [3]=0.40}, uses=90, maxlevel=0},
		},
	},
})

--
-- Shears (dig snappy)
--

minetest.register_tool("basetools:shears_wood", {
	description = "Wooden Shears".."\n"..
		"Digs snappy=3",
	inventory_image = "basetools_woodshears.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			snappy={times={[3]=1.00}, uses=30, maxlevel=0},
		},
	},
})
minetest.register_tool("basetools:shears_stone", {
	description = "Stone Shears".."\n"..
		"Digs snappy=2..3",
	inventory_image = "basetools_stoneshears.png",
	tool_capabilities = {
		max_drop_level=0,
		groupcaps={
			snappy={times={[2]=1.00, [3]=0.50}, uses=60, maxlevel=0},
		},
	},
})
minetest.register_tool("basetools:shears_steel", {
	description = "Steel Shears".."\n"..
		"Digs snappy=1..3",
	inventory_image = "basetools_steelshears.png",
	tool_capabilities = {
		max_drop_level=1,
		groupcaps={
			snappy={times={[1]=1.00, [2]=0.50, [3]=0.25}, uses=90, maxlevel=0},
		},
	},
})

--
-- Swords (deal damage)
--

minetest.register_tool("basetools:sword_wood", {
	description = "Wooden Sword".."\n"..
		"Damage: fleshy=2",
	inventory_image = "basetools_woodsword.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		damage_groups = {fleshy=2},
	}
})
minetest.register_tool("basetools:sword_stone", {
	description = "Stone Sword".."\n"..
		"Damage: fleshy=4",
	inventory_image = "basetools_stonesword.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=0,
		damage_groups = {fleshy=4},
	}
})
minetest.register_tool("basetools:sword_steel", {
	description = "Steel Sword".."\n"..
		"Damage: fleshy=6",
	inventory_image = "basetools_steelsword.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=1,
		damage_groups = {fleshy=6},
	}
})

-- Fire/Ice sword: Deal damage to non-fleshy damage groups
minetest.register_tool("basetools:sword_fire", {
	description = "Fire Sword".."\n"..
		"Damage: icy=6",
	inventory_image = "basetools_firesword.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=0,
		damage_groups = {icy=6},
	}
})
minetest.register_tool("basetools:sword_ice", {
	description = "Ice Sword".."\n"..
		"Damage: fiery=6",
	inventory_image = "basetools_icesword.png",
	tool_capabilities = {
		full_punch_interval = 1.0,
		max_drop_level=0,
		damage_groups = {fiery=6},
	}
})

--
-- Dagger: Low damage, fast punch interval
--
minetest.register_tool("basetools:dagger_steel", {
	description = "Steel Dagger".."\n"..
		"Damage: fleshy=2".."\n"..
		"Full Punch Interval: 0.5s",
	inventory_image = "basetools_steeldagger.png",
	tool_capabilities = {
		full_punch_interval = 0.5,
		max_drop_level=0,
		damage_groups = {fleshy=2},
	}
})
