-- WALL LEVER
minetest.register_node("mesecons_walllever:wall_lever_off", {
    drawtype = "signlike",
    tile_images = {"jeija_wall_lever_off.png"},
    inventory_image = "jeija_wall_lever_off.png",
    wield_image = "jeija_wall_lever_off.png",
    paramtype = "light",
    paramtype2 = "wallmounted",
    legacy_wallmounted = true,
    walkable = false,
    selection_box = {
        type = "wallmounted",
    },
    groups = {dig_immediate=2},
    description="Lever",
})
minetest.register_node("mesecons_walllever:wall_lever_on", {
    drawtype = "signlike",
    tile_images = {"jeija_wall_lever_on.png"},
    inventory_image = "jeija_wall_lever_on.png",
    paramtype = "light",
    paramtype2 = "wallmounted",
    legacy_wallmounted = true,
    walkable = false,
    selection_box = {
        type = "wallmounted",
    },
    groups = {dig_immediate=2},
    drop = '"mesecons_walllever:wall_lever_off" 1',
    description="Lever",
})

minetest.register_on_dignode(
    function(pos, oldnode, digger)
        if oldnode.name == "mesecons_walllever:wall_lever_on" then
            mesecon:receptor_off(pos, mesecon.button_get_rules(oldnode.param2))
        end    
    end
)
minetest.register_on_punchnode(function(pos, node, puncher)
	if node.name == "mesecons_walllever:wall_lever_off" then
		minetest.env:add_node(pos, {name="mesecons_walllever:wall_lever_on",param2=node.param2})
		mesecon:receptor_on(pos, mesecon.button_get_rules(node.param2))
	end
	if node.name == "mesecons_walllever:wall_lever_on" then
		minetest.env:add_node(pos, {name="mesecons_walllever:wall_lever_off",param2=node.param2})
		mesecon:receptor_off(pos, mesecon.button_get_rules(node.param2))
	end
end)

minetest.register_craft({
	output = '"mesecons_walllever:wall_lever_off" 2',
	recipe = {
	    {'"mesecons:mesecon_off"'},
		{'"default:stone"'},
		{'"default:stick"'},
	}
})
mesecon:add_receptor_node("mesecons_walllever:wall_lever", nil, mesecon.button_get_rules)
mesecon:add_receptor_node_off("mesecons_walllever:wall_lever_off", nil, mesecon.button_get_rules)
