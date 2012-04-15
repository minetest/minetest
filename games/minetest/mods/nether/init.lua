-- Nether Mod (based on Nyanland)
-- lkjoel (nyanland by Jeija)

NETHER_DEPTH = -100
NETHER_HEIGHT = 20
LAVA_FREQ = 500
LAVA_HEIGHT = 2
LAVALAKE_FREQ = 10000
LAVALAKE_HEIGHT = 2
LAVALAKE_WIDTH = 10
LAVALAKE_BREADTH = 10
NETHER_TREESIZE = 2

-- Netherrack
minetest.register_node("nether:netherrack", {
	description = "Netherrack",
	tile_images = {"nether_netherrack.png"},
	is_ground_content = true,
	groups = {cracky=3},
	drop = "nether_netherrack.png",
	sounds = default.node_sound_stone_defaults(),
})

-- Create the Nether
minetest.register_on_generated(function(minp, maxp)
	local addpos = {}
	local llpos = {}
	print("minp:" .. minp.y .. ", maxp:" .. maxp.y)
	if ((maxp.y >= (NETHER_DEPTH - NETHER_HEIGHT)) and (minp.y <= NETHER_DEPTH)) then
		for x=minp.x, maxp.x, 1 do
			for y=minp.y, maxp.y, 1 do
				for z=minp.z, maxp.z, 1 do
					addpos = {x=x, y=y, z=z}
					if y == NETHER_DEPTH or y == (NETHER_DEPTH - NETHER_HEIGHT) then
						minetest.env:add_node(addpos, {name="nether:netherrack"})
					elseif y <= NETHER_DEPTH and y >= (NETHER_DEPTH - NETHER_HEIGHT) then
						minetest.env:add_node(addpos, {name="air"})
						local height = math.random(LAVALAKE_HEIGHT)
						if math.random(LAVALAKE_FREQ) == 1 and y <= (NETHER_DEPTH - NETHER_HEIGHT + height) then
							local width = math.random(LAVALAKE_WIDTH)
							local breadth = math.random(LAVALAKE_BREADTH)
							for h=0, height, 1 do
								for w=0, width, 1 do
									for b=0, breadth, 1 do
										llpos = {x=w,y=h,z=b}
										minetest.env:add_node(llpos, {name="default:lava_source"})
									end
								end
							end
						elseif math.random(LAVA_FREQ) == 1 and y <= (NETHER_DEPTH - NETHER_HEIGHT + LAVA_HEIGHT) then
							minetest.env:add_node(addpos, {name="default:lava_source"})
						end
					end
				end
			end
		end
		print("DONE")
	end
end)

print("Nether mod loaded!")
