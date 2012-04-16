-- Nether Mod (based on Nyanland)
-- lkjoel (nyanland by Jeija)

--== EDITABLE OPTIONS ==--

-- Depth of the nether
NETHER_DEPTH = -100
-- Height of the nether (bottom of the nether is NETHER_DEPTH - NETHER_HEIGHT)
NETHER_HEIGHT = 20
-- Frequency of lava (higher is less frequent)
LAVA_FREQ = 500
-- Maximum height of lava
LAVA_HEIGHT = 3
-- Frequency of nether trees (higher is less frequent)
NETHER_TREE_FREQ = 350
-- Height of nether trees
NETHER_TREESIZE = 2
-- Frequency of apples in a nether tree (higher is less frequent)
NETHER_APPLE_FREQ = 5
-- Frequency of healing apples in a nether tree (higher is less frequent)
NETHER_HEAL_APPLE_FREQ = 10
-- Start position for the throne of Hades (y is relative to the bottom of the nether)
HADES_THRONE_STARTPOS = {x=0, y=1, z=0}
-- Throne of Hades
HADES_THRONE = {
	-- Floor 1
	{pos={x=0,y=0,z=0}, block="nether:netherrack"},
	{pos={x=1,y=0,z=0}, block="nether:netherrack"},
	{pos={x=2,y=0,z=0}, block="nether:netherrack"},
	{pos={x=3,y=0,z=0}, block="nether:netherrack"},
	{pos={x=4,y=0,z=0}, block="nether:netherrack"},
	{pos={x=5,y=0,z=0}, block="nether:netherrack"},
	{pos={x=5,y=0,z=1}, block="nether:netherrack"},
	{pos={x=5,y=0,z=2}, block="nether:netherrack"},
	{pos={x=5,y=0,z=3}, block="nether:netherrack"},
	{pos={x=5,y=0,z=4}, block="nether:netherrack"},
	{pos={x=5,y=0,z=5}, block="nether:netherrack"},
	{pos={x=4,y=0,z=5}, block="nether:netherrack"},
	{pos={x=3,y=0,z=5}, block="nether:netherrack"},
	{pos={x=2,y=0,z=5}, block="nether:netherrack"},
	{pos={x=1,y=0,z=5}, block="nether:netherrack"},
	{pos={x=0,y=0,z=5}, block="nether:netherrack"},
	{pos={x=0,y=0,z=4}, block="nether:netherrack"},
	{pos={x=0,y=0,z=3}, block="nether:netherrack"},
	{pos={x=0,y=0,z=2}, block="nether:netherrack"},
	{pos={x=0,y=0,z=1}, block="nether:netherrack"},
	-- Floor 2
	{pos={x=0,y=1,z=0}, block="nether:netherrack"},
	{pos={x=1,y=1,z=0}, block="nether:netherrack"},
	{pos={x=2,y=1,z=0}, block="nether:netherrack"},
	{pos={x=3,y=1,z=0}, block="nether:netherrack"},
	{pos={x=4,y=1,z=0}, block="nether:netherrack"},
	{pos={x=5,y=1,z=0}, block="nether:netherrack"},
	{pos={x=5,y=1,z=1}, block="nether:netherrack"},
	{pos={x=5,y=1,z=2}, block="nether:netherrack"},
	{pos={x=5,y=1,z=3}, block="nether:netherrack"},
	{pos={x=5,y=1,z=4}, block="nether:netherrack"},
	{pos={x=5,y=1,z=5}, block="nether:netherrack"},
	{pos={x=4,y=1,z=5}, block="nether:netherrack"},
	{pos={x=3,y=1,z=5}, block="nether:netherrack"},
	{pos={x=2,y=1,z=5}, block="nether:netherrack"},
	{pos={x=1,y=1,z=5}, block="nether:netherrack"},
	{pos={x=0,y=1,z=5}, block="nether:netherrack"},
	{pos={x=0,y=1,z=4}, block="nether:netherrack"},
	{pos={x=1,y=1,z=3}, block="nether:netherrack"},
	{pos={x=1,y=1,z=2}, block="nether:netherrack"},
	{pos={x=0,y=1,z=1}, block="nether:netherrack"},
	{pos={x=1,y=1,z=1}, block="nether:netherrack"},
	{pos={x=1,y=1,z=4}, block="nether:netherrack"},
	-- Floor 3
	{pos={x=0,y=2,z=0}, block="nether:netherrack"},
	{pos={x=1,y=2,z=0}, block="nether:netherrack"},
	{pos={x=2,y=2,z=0}, block="nether:netherrack"},
	{pos={x=3,y=2,z=0}, block="nether:netherrack"},
	{pos={x=4,y=2,z=0}, block="nether:netherrack"},
	{pos={x=5,y=2,z=0}, block="nether:netherrack"},
	{pos={x=5,y=2,z=1}, block="nether:netherrack"},
	{pos={x=5,y=2,z=2}, block="nether:netherrack"},
	{pos={x=5,y=2,z=3}, block="nether:netherrack"},
	{pos={x=5,y=2,z=4}, block="nether:netherrack"},
	{pos={x=5,y=2,z=5}, block="nether:netherrack"},
	{pos={x=4,y=2,z=5}, block="nether:netherrack"},
	{pos={x=3,y=2,z=5}, block="nether:netherrack"},
	{pos={x=2,y=2,z=5}, block="nether:netherrack"},
	{pos={x=1,y=2,z=5}, block="nether:netherrack"},
	{pos={x=0,y=2,z=5}, block="nether:netherrack"},
	{pos={x=0,y=2,z=4}, block="nether:netherrack"},
	{pos={x=2,y=2,z=3}, block="nether:netherrack"},
	{pos={x=2,y=2,z=2}, block="nether:netherrack"},
	{pos={x=0,y=2,z=1}, block="nether:netherrack"},
	{pos={x=1,y=2,z=1}, block="nether:netherrack"},
	{pos={x=1,y=2,z=4}, block="nether:netherrack"},
	{pos={x=2,y=2,z=1}, block="nether:netherrack"},
	{pos={x=2,y=2,z=4}, block="nether:netherrack"},
	-- Floor 4
	{pos={x=0,y=3,z=0}, block="nether:netherrack"},
	{pos={x=1,y=3,z=0}, block="nether:netherrack"},
	{pos={x=2,y=3,z=0}, block="nether:netherrack"},
	{pos={x=3,y=3,z=0}, block="nether:netherrack"},
	{pos={x=4,y=3,z=0}, block="nether:netherrack"},
	{pos={x=5,y=3,z=0}, block="nether:netherrack"},
	{pos={x=5,y=3,z=1}, block="nether:netherrack"},
	{pos={x=5,y=3,z=2}, block="nether:netherrack"},
	{pos={x=5,y=3,z=3}, block="nether:netherrack"},
	{pos={x=5,y=3,z=4}, block="nether:netherrack"},
	{pos={x=5,y=3,z=5}, block="nether:netherrack"},
	{pos={x=4,y=3,z=5}, block="nether:netherrack"},
	{pos={x=3,y=3,z=5}, block="nether:netherrack"},
	{pos={x=2,y=3,z=5}, block="nether:netherrack"},
	{pos={x=1,y=3,z=5}, block="nether:netherrack"},
	{pos={x=0,y=3,z=5}, block="nether:netherrack"},
	{pos={x=0,y=3,z=4}, block="nether:netherrack"},
	{pos={x=3,y=3,z=3}, block="nether:netherrack"},
	{pos={x=3,y=3,z=2}, block="nether:netherrack"},
	{pos={x=0,y=3,z=1}, block="nether:netherrack"},
	{pos={x=1,y=3,z=1}, block="nether:netherrack"},
	{pos={x=1,y=3,z=4}, block="nether:netherrack"},
	{pos={x=2,y=3,z=1}, block="nether:netherrack"},
	{pos={x=2,y=3,z=4}, block="nether:netherrack"},
	{pos={x=3,y=3,z=1}, block="nether:netherrack"},
	{pos={x=3,y=3,z=4}, block="nether:netherrack"},
	{pos={x=4,y=3,z=1}, block="nether:netherrack"},
	{pos={x=4,y=3,z=4}, block="nether:netherrack"},
	{pos={x=4,y=3,z=2}, block="nether:netherrack"},
	{pos={x=5,y=3,z=2}, block="nether:netherrack"},
	{pos={x=4,y=3,z=3}, block="nether:netherrack"},
	{pos={x=5,y=3,z=3}, block="nether:netherrack"},
	-- Floor 5
	{pos={x=4,y=4,z=2}, block="nether:netherrack"},
	{pos={x=5,y=4,z=2}, block="nether:netherrack"},
	{pos={x=4,y=4,z=3}, block="nether:netherrack"},
	{pos={x=5,y=4,z=3}, block="nether:netherrack"},
	-- Torches on floor 5
	{pos={x=4,y=4,z=0}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=1}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=0}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=1}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=4}, block="nether:nether_torch_bottom"},
	{pos={x=4,y=4,z=5}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=4}, block="nether:nether_torch_bottom"},
	{pos={x=5,y=4,z=5}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=0}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=1}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=0}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=1}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=4}, block="nether:nether_torch_bottom"},
	{pos={x=0,y=4,z=5}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=4}, block="nether:nether_torch_bottom"},
	{pos={x=1,y=4,z=5}, block="nether:nether_torch_bottom"},
	-- Floor 6
	{pos={x=6,y=5,z=2}, block="nether:netherrack"},
	{pos={x=6,y=5,z=3}, block="nether:netherrack"},
	-- Floor 7
	{pos={x=6,y=6,z=2}, block="nether:netherrack"},
	{pos={x=6,y=6,z=3}, block="nether:netherrack"},
}

--== END OF EDITABLE OPTIONS ==--

-- Generated variables
NETHER_BOTTOM = (NETHER_DEPTH - NETHER_HEIGHT)
HADES_THRONE_STARTPOS_ABS = {x=HADES_THRONE_STARTPOS.x, y=(NETHER_BOTTOM + HADES_THRONE_STARTPOS.y), z=HADES_THRONE_STARTPOS.z}
LAVA_Y = (NETHER_BOTTOM + LAVA_HEIGHT)
HADES_THRONE_ABS = {}
for i,v in ipairs(HADES_THRONE) do
	v.pos.x = v.pos.x + HADES_THRONE_STARTPOS_ABS.x
	v.pos.y = v.pos.y + HADES_THRONE_STARTPOS_ABS.y
	v.pos.z = v.pos.z + HADES_THRONE_STARTPOS_ABS.z
	HADES_THRONE_ABS[i] = v
end
local nether = {}

-- Netherrack
minetest.register_node("nether:netherrack", {
	description = "Netherrack",
	tile_images = {"nether_netherrack.png"},
	is_ground_content = true,
	groups = {cracky=3},
	drop = "nether_netherrack.png",
	sounds = default.node_sound_stone_defaults(),
})

-- Nether tree
minetest.register_node("nether:nether_tree", {
	description = "Nether Tree",
	tile_images = {"nether_tree_top.png", "nether_tree_top.png", "nether_tree.png"},
	is_ground_content = true,
	groups = {tree=1, snappy=2, choppy=2, oddly_breakable_by_hand=1},
	sounds = default.node_sound_wood_defaults(),
})

-- Nether leaves
minetest.register_node("nether:nether_leaves", {
	description = "Nether Leaves",
	drawtype = "allfaces_optional",
	visual_scale = 1.3,
	tile_images = {"nether_leaves.png"},
	paramtype = "light",
	groups = {snappy=3, leafdecay=2},
	drop = "nether:nether_leaves",
	sounds = default.node_sound_leaves_defaults(),
})

-- Nether apple
minetest.register_node("nether:nether_apple", {
	description = "Nether Apple",
	drawtype = "plantlike",
	visual_scale = 1.0,
	tile_images = {"nether_apple.png"},
	inventory_image = "nether_apple.png",
	paramtype = "light",
	sunlight_propagates = true,
	walkable = false,
	groups = {fleshy=3, dig_immediate=3},
	on_use = minetest.item_eat(-4),
	sounds = default.node_sound_defaults(),
})

-- Nether torch
minetest.register_node("nether:nether_torch", {
	description = "Nether Torch",
	drawtype = "torchlike",
	tile_images = {"nether_torch_on_floor.png", "nether_torch_on_ceiling.png", "nether_torch.png"},
	inventory_image = "nether_torch_on_floor.png",
	wield_image = "nether_torch_on_floor.png",
	paramtype = "light",
	paramtype2 = "wallmounted",
	sunlight_propagates = true,
	walkable = false,
	light_source = LIGHT_MAX - 1,
	selection_box = {
		type = "wallmounted",
		wall_top = {-0.1, 0.5-0.6, -0.1, 0.1, 0.5, 0.1},
		wall_bottom = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_side = {-0.5, -0.3, -0.1, -0.5+0.3, 0.3, 0.1},
	},
	groups = {choppy=2, dig_immediate=3},
	legacy_wallmounted = true,
	sounds = default.node_sound_defaults(),
})

-- Nether torch (only shows the bottom torch. This is a hack)
minetest.register_node("nether:nether_torch_bottom", {
	description = "Nether Torch Bottom Side (you hacker!)",
	drawtype = "torchlike",
	tile_images = {"nether_torch_on_floor.png", "nether_torch_on_floor.png", "nether_torch_floor.png"},
	inventory_image = "nether_torch_on_floor.png",
	wield_image = "nether_torch_on_floor.png",
	paramtype = "light",
	paramtype2 = "facedir",
	sunlight_propagates = true,
	walkable = false,
	light_source = LIGHT_MAX - 1,
	drop = "nether:nether_torch",
	selection_box = {
		type = "wallmounted",
		wall_top = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_bottom = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
		wall_side = {-0.1, -0.5, -0.1, 0.1, -0.5+0.6, 0.1},
	},
	groups = {choppy=2, dig_immediate=3},
	legacy_wallmounted = true,
	sounds = default.node_sound_defaults(),
})

-- Create the Nether
minetest.register_on_generated(function(minp, maxp)
	local addpos = {}
	hadesthronecounter = 1
	print("minp:" .. minp.y .. ", maxp:" .. maxp.y)
	if ((maxp.y >= NETHER_BOTTOM) and (minp.y <= NETHER_DEPTH)) then
		-- Pass 1: Terrain generation
		for x=minp.x, maxp.x, 1 do
			for y=minp.y, maxp.y, 1 do
				for z=minp.z, maxp.z, 1 do
					addpos = {x=x, y=y, z=z}
					if y == NETHER_DEPTH or y == NETHER_BOTTOM then
						minetest.env:add_node(addpos, {name="nether:netherrack"})
					elseif y <= NETHER_DEPTH and y >= NETHER_BOTTOM then
						minetest.env:add_node(addpos, {name="air"})
					end
				end
			end
		end
		-- Pass 2: Details
		for x=minp.x, maxp.x, 1 do
			for y=minp.y, maxp.y, 1 do
				for z=minp.z, maxp.z, 1 do
					addpos = {x=x, y=y, z=z}
					if y < NETHER_DEPTH and y > NETHER_BOTTOM then
						if math.random(NETHER_TREE_FREQ) == 1 and y == (NETHER_BOTTOM + 1) then
							nether:grow_nethertree(addpos)
						elseif math.random(LAVA_FREQ) == 1 and y <= LAVA_Y then
							minetest.env:add_node(addpos, {name="default:lava_source"})
						end
					end
				end
			end
		end
		-- Pass 3: Throne of Hades
		for i,v in ipairs(HADES_THRONE_ABS) do
			minetest.env:add_node(v.pos, {name=v.block})
		end
		print("DONE")
	end
end)

-- Create a nether tree
function nether:grow_nethertree(pos)
	--TRUNK
	pos.y=pos.y+1
	local trunkpos={x=pos.x, z=pos.z}
	for y=pos.y, pos.y+4+math.random(2) do
		trunkpos.y=y
		minetest.env:add_node(trunkpos, {name="nether:nether_tree"})
	end
	--LEAVES
	local leafpos={}
	for x=(trunkpos.x-NETHER_TREESIZE), (trunkpos.x+NETHER_TREESIZE), 1 do
       		for y=(trunkpos.y-NETHER_TREESIZE), (trunkpos.y+NETHER_TREESIZE), 1 do
       			for z=(trunkpos.z-NETHER_TREESIZE), (trunkpos.z+NETHER_TREESIZE), 1 do
		       		if (x-trunkpos.x)*(x-trunkpos.x)
				+(y-trunkpos.y)*(y-trunkpos.y)
				+(z-trunkpos.z)*(z-trunkpos.z)
				<= NETHER_TREESIZE*NETHER_TREESIZE + NETHER_TREESIZE then
					leafpos={x=x, y=y, z=z}
					if minetest.env:get_node(leafpos).name=="air" then
						if math.random(NETHER_APPLE_FREQ) == 1 then
							if math.random(NETHER_HEAL_APPLE_FREQ) == 1 then
								minetest.env:add_node(leafpos, {name="default:apple"})
							else
								minetest.env:add_node(leafpos, {name="nether:nether_apple"})
							end
						else
							minetest.env:add_node(leafpos, {name="nether:nether_leaves"})
						end				
					end				
				end
			end
		end
	end
end

print("Nether mod loaded!")
