minetest.register_craftitem("flint_and_steel:flint_and_steel", {
	inventory_image = "flint_and_steel.png",
	description = 'Flint and Steel',
	stack_max = 1,
	liquids_pointable = false,
	on_use = function(itemstack, user, pointed_thing)
		n = minetest.env:get_node(pointed_thing)
		if pointed_thing.type == "node" then
		  soundfile = "strike"
		  minetest.sound_play(soundfile, {gain=0.5})
		  itemstack:take_item()
			minetest.env:add_node(pointed_thing.above, {name="fire:basic_flame"})
		end
		return itemstack
	end
})

minetest.register_craftitem("flint_and_steel:flint", {
	inventory_image = "flint.png",
	description = 'Flint',
	stack_max = 99,
	liquids_pointable = false,
})
	
minetest.register_craft({
	output = 'flint_and_steel:flint_and_steel 16',
	recipe = {
		{'','default:steel_ingot'},
		{'flint_and_steel:flint',''},
	}
})