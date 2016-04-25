
-- Rat by PilzAdam

mobs:register_mob("mobs_animal:rat", {
	type = "animal",
	passive = true,
	hp_min = 1,
	hp_max = 4,
	armor = 200,
	collisionbox = {-0.2, -1, -0.2, 0.2, -0.8, 0.2},
	visual = "mesh",
	mesh = "mobs_rat.b3d",
	textures = {
		{"mobs_rat.png"},
		{"mobs_rat2.png"},
	},
	makes_footstep_sound = false,
	sounds = {
		random = "mobs_rat",
	},
	walk_velocity = 1,
	run_velocity = 2,
	runaway = true,
	jump = true,
	water_damage = 0,
	lava_damage = 4,
	light_damage = 0,
	fear_height = 2,
	on_rightclick = function(self, clicker)
		mobs:capture_mob(self, clicker, 25, 80, 0, true, nil)
	end,
--[[
	do_custom = function(self)
		local pos = self.object:getpos()
		print("rat pos", pos.x, pos.y, pos.z)
	end,
--]]
})

mobs:register_spawn("mobs_animal:rat", {"default:stone"}, 20, 5, 15000, 2, 0)

mobs:register_egg("mobs_animal:rat", "Rat", "mobs_rat_inventory.png", 0)

-- compatibility
mobs:alias_mob("mobs:rat", "mobs_animal:rat")

-- cooked rat, yummy!
minetest.register_craftitem(":mobs:rat_cooked", {
	description = "Cooked Rat",
	inventory_image = "mobs_cooked_rat.png",
	on_use = minetest.item_eat(3),
})

minetest.register_craft({
	type = "cooking",
	output = "mobs:rat_cooked",
	recipe = "mobs:rat",
	cooktime = 5,
})
