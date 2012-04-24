-- MESECON_SWITCH

minetest.register_node("mesecons_switch:mesecon_switch_off", {
	tile_images = {"jeija_mesecon_switch_side.png", "jeija_mesecon_switch_side.png", "jeija_mesecon_switch_side.png", "jeija_mesecon_switch_side.png", "jeija_mesecon_switch_side.png", "jeija_mesecon_switch_off.png"},
	paramtype2="facedir",
	groups = {dig_immediate=2},
    	description="Switch",
})

minetest.register_node("mesecons_switch:mesecon_switch_on", {
	tile_images = {"jeija_mesecon_switch_side.png", "jeija_mesecon_switch_side.png", "jeija_mesecon_switch_side.png", "jeija_mesecon_switch_side.png", "jeija_mesecon_switch_side.png", "jeija_mesecon_switch_on.png"},
	paramtype2="facedir",
	groups = {dig_immediate=2},
	drop='"mesecons_switch:mesecon_switch_off" 1',
    	description="Switch",
})

mesecon:add_receptor_node("mesecons_switch:mesecon_switch_on")
mesecon:add_receptor_node_off("mesecons_switch:mesecon_switch_off")

minetest.register_on_punchnode(function(pos, node, puncher)
	if node.name == "mesecons_switch:mesecon_switch_on" then
		minetest.env:add_node(pos, {name="mesecons_switch:mesecon_switch_off", param2=node.param2})
		nodeupdate(pos)
		mesecon:receptor_off(pos)
	end
	if node.name == "mesecons_switch:mesecon_switch_off" then
		minetest.env:add_node(pos, {name="mesecons_switch:mesecon_switch_on", param2=node.param2})
		nodeupdate(pos)
		mesecon:receptor_on(pos)
	end
end)

minetest.register_on_dignode(
	function(pos, oldnode, digger)
		if oldnode.name == "mesecons_switch:mesecon_switch_on" then
			mesecon:receptor_off(pos)
		end
	end
)

minetest.register_craft({
	output = '"mesecons_switch:mesecon_switch_off" 2',
	recipe = {
		{'"default:steel_ingot"', '"default:cobble"', '"default:steel_ingot"'},
		{'"mesecons:mesecon_off"','', '"mesecons:mesecon_off"'},
	}
})
