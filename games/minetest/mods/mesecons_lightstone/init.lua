function mesecon:lightstone_add(name, base_item, texture_off, texture_on)
    minetest.register_node("mesecons_lightstone:lightstone_" .. name .. "_off", {
	    tile_images = {texture_off},
	    inventory_image = minetest.inventorycube(texture_off),
	    groups = {cracky=2},
    	    description=name.." Lightstone",
    })
    minetest.register_node("mesecons_lightstone:lightstone_" .. name .. "_on", {
	    tile_images = {texture_on},
	    inventory_image = minetest.inventorycube(texture_on),
	    groups = {cracky=2},
	    drop = "node mesecons_lightstone:lightstone_" .. name .. "_off 1",
	    light_source = LIGHT_MAX-2,
    	    description=name.." Lightstone",
    })
    assert(loadstring('mesecon:register_on_signal_on(function(pos, node) \n \
                    if node.name == "mesecons_lightstone:lightstone_' .. name .. '_off" then \n \
                        minetest.env:add_node(pos, {name="mesecons_lightstone:lightstone_' .. name .. '_on"}) \n \
                        nodeupdate(pos) \n \
                    end \n \
                end)'))()
    assert(loadstring('mesecon:register_on_signal_off(function(pos, node) \n \
                    if node.name == "mesecons_lightstone:lightstone_' .. name .. '_on" then \n \
                        minetest.env:add_node(pos, {name="mesecons_lightstone:lightstone_' .. name .. '_off"}) \n \
                        nodeupdate(pos) \n \
                    end \n \
                end)'))()
    minetest.register_craft({
	    output = "node mesecons_lightstone:lightstone_" .. name .. "_off 1",
	    recipe = {
		    {'',base_item,''},
		    {base_item,'node default:torch 1',base_item},
		    {'','node mesecons:mesecon_off 1',''},
	    }
    })
end


mesecon:lightstone_add("red", "craft default:clay_brick 1", "jeija_lightstone_red_off.png", "jeija_lightstone_red_on.png")
mesecon:lightstone_add("green", "node default:cactus 1", "jeija_lightstone_green_off.png", "jeija_lightstone_green_on.png")
mesecon:lightstone_add("gray", "node default:cobble 1", "jeija_lightstone_gray_off.png", "jeija_lightstone_gray_on.png")
mesecon:lightstone_add("darkgray", "node default:gravel 1", "jeija_lightstone_darkgray_off.png", "jeija_lightstone_darkgray_on.png")
