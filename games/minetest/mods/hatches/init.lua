-- Minetest 0.4 mod: hatches
-- See README.txt for licensing and other information.
--------------------------------------------------------------------------------

local WALLMX = 3
local WALLMZ = 5
local WALLPX = 2
local WALLPZ = 4

--------------------------------------------------------------------------------

minetest.register_alias('door', 'hatches:door_wood')
minetest.register_alias('door_wood', 'hatches:door_wood')

minetest.register_craft( {
	output              = 'hatches:door_wood_b_c',
	recipe = {
		{ 'default:wood', 'default:wood', 'default:wood' },
		{ 'default:wood', 'default:wood', 'default:wood' },
	},
})

minetest.register_craft({
	type = 'fuel',
	recipe = 'hatches:door_wood',
	burntime = 30,
})



minetest.register_node( 'hatches:door_wood_b_c', {
	description         = "Hatch",
	drawtype            = 'raillike',
	tile_images         = { 'hatch1.png' },
	inventory_image     = 'hatch1.png',
	paramtype           = 'light',
	paramtype2          = 'wallmounted',
	walkable            = true,
	selection_box       = { type = "fixed", },
	groups              = { choppy=2, dig_immediate=2 },
	legacy_wallmounted  = true,
	drop                = 'hatches:door_wood_b_c',
})


minetest.register_node( 'hatches:door_wood_b_o', {
	description         = 'Hatch',
	drawtype            = 'signlike',
	tile_images         = { 'hatch1.png' },
	inventory_image     = 'hatch1.png',
	paramtype           = 'light',
	paramtype2          = 'wallmounted',
	walkable            = false,
	selection_box       = { type = "wallmounted", },
	groups              = { choppy=2, dig_immediate=2 },
	legacy_wallmounted  = true,
	drop                = 'hatches:door_wood_b_c',
})

--------------------------------------------------------------------------------

local round = function( n )
	if n >= 0 then
		return math.floor( n + 0.5 )
	else
		return math.ceil( n - 0.5 )
	end
end

local on_door_placed = function( pos, node, placer )
	if node.name ~= 'hatches:door_wood_b_c' then return end

	upos = { x = pos.x, y = pos.y - 1, z = pos.z }
	apos = { x = pos.x, y = pos.y + 1, z = pos.z }
	und = minetest.env:get_node( upos )
	abv = minetest.env:get_node( apos )

	dir = placer:get_look_dir()

	if     round( dir.x * -1 ) == 1  then
		newparam = WALLMX
	elseif round( dir.x * -1 ) == -1 then
		newparam = WALLPX
	elseif round( dir.z * -1 ) == 1  then
		newparam = WALLMZ
	elseif round( dir.z * -1 ) == -1 then
		newparam = WALLPZ
	end

end

local on_door_punched = function( pos, node, puncher )
	if string.find( node.name, 'hatches:door_wood' ) == nil then return end

	upos = { x = pos.x, y = pos.y - 1, z = pos.z }
	apos = { x = pos.x, y = pos.y + 1, z = pos.z }

	if string.find( node.name, '_c', -2 ) ~= nil then
		if     node.param2 == WALLPX then
			newparam = WALLMZ
		elseif node.param2 == WALLMZ then
			newparam = WALLMX
		elseif node.param2 == WALLMX then
			newparam = WALLPZ
		elseif node.param2 == WALLPZ then
			newparam = WALLPX
		end
	elseif string.find( node.name, '_o', -2 ) ~= nil then
		if     node.param2 == WALLMZ then
			newparam = WALLPX
		elseif node.param2 == WALLMX then
			newparam = WALLMZ
		elseif node.param2 == WALLPZ then
			newparam = WALLMX
		elseif node.param2 == WALLPX then
			newparam = WALLPZ
		end
	end


	if ( node.name == 'hatches:door_wood_b_c' ) then
		minetest.env:add_node( pos,  { name = 'hatches:door_wood_b_o', param2 = newparam } )

	elseif ( node.name == 'hatches:door_wood_b_o' ) then
		minetest.env:add_node( pos,  { name = 'hatches:door_wood_b_c', param2 = newparam } )

	end
end

local on_door_digged = function( pos, node, digger )
	upos = { x = pos.x, y = pos.y - 1, z = pos.z }
	apos = { x = pos.x, y = pos.y + 1, z = pos.z }

	if ( node.name == 'hatches:door_wood_a_c' ) or ( node.name == 'hatches:door_wood_a_o' ) then
		minetest.env:remove_node( upos )
	elseif ( node.name == 'hatches:door_wood_b_c' ) or ( node.name == 'hatches:door_wood_b_o' ) then
		minetest.env:remove_node( apos )
	end
end

--------------------------------------------------------------------------------

minetest.register_on_placenode( on_door_placed )
minetest.register_on_punchnode( on_door_punched )
minetest.register_on_dignode( on_door_digged )

--------------------------------------------------------------------------------

