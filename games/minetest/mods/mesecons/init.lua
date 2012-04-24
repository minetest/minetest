-- |\    /| ____ ____  ____ _____   ____         _____
-- | \  / | |    |     |    |      |    | |\   | |
-- |  \/  | |___ ____  |___ |      |    | | \  | |____
-- |      | |        | |    |      |    | |  \ |     |
-- |      | |___ ____| |___ |____  |____| |   \| ____|
-- by Jeija and Minerd247
--
--
--
-- This mod adds mesecons[=minecraft redstone] and different receptors/effectors to minetest.
--
-- See the documentation on the forum for additional information, especially about crafting
--
--Quick Developer documentation for the mesecon API
--=================================================
--
--RECEPTORS
--
--A receptor is a node that emits power, e.g. a solar panel, a switch or a power plant.
--Usually you create two blocks per receptor that have to be switched when switching the on/off state: 
--	# An off-state node (e.g. mesecons:mesecon_switch_off"
--	# An on-state node (e.g. mesecons:mesecon_switch_on"
--The on-state and off-state nodes should be registered in the mesecon api, 
--so that the Mesecon circuit can be recalculated. This can be done using
--
--mesecon:add_receptor_node(nodename) -- for on-state node
--mesecon:add_receptor_node_off(nodename) -- for off-state node
--example: mesecon:add_receptor_node("mesecons:mesecon_switch_on")
--
--Turning receptors on and off
--Usually the receptor has to turn on and off. For this, you have to
--	# Remove the node and replace it with the node in the other state (e.g. replace on by off)
--	# Send the event to the mesecon circuit by using the api functions
--		mesecon:receptor_on (pos, rules) } These functions take the position of your receptor
--		mesecon:receptor_off(pos, rules) } as their parameter.
--
--You can specify the rules using the rules parameter. If you don't want special rules, just leave it out
--e.g. if you want to use the "pressureplate" rules, you use this command:
--mesecon:receptor_on (pos, mesecon:get_rules("pressureplate"))
--The rules can be manipulated by several rotate functions:
--rules=mesecon:rotate_rules_right/left/up/down(rules)
--
--
--!! If a receptor node is removed, the circuit should be recalculated. This means you have to
--send an mesecon:receptor_off signal to the api when the function in minetest.register_on_dignode
--is called.
--
--EFFECTORS
--
--A receptor is a node that uses power and transfers the signal to a mechanical, optical whatever
--event. e.g. the meselamp, the movestone or the removestone.
--
--There are two callback functions for receptors.
--	# function mesecon:register_on_signal_on (action)
--	# function mesecon:register_on_signal_off(action)
--
--These functions will be called for each block next to a mesecon conductor.
--
--Example: The removestone
--The removestone only uses one callback: The mesecon:register_on_signal_on function
--
--mesecon:register_on_signal_on(function(pos, node) -- As the action prameter you have to use a function
--	if node.name=="mesecons:removestone" then -- Check if it really is removestone. If you wouldn't use this, every node next to mesecons would be removed
--		minetest.env:remove_node(pos) -- The action: The removestone is removed
--	end -- end of if
--end) -- end of the function, )=end of the parameters of mesecon:register_on_signal_on
--
--CONDUCTORS: (new feature!! yay)
--You can specify your custom conductors using
--# mesecon:register_conductor(onstate, offstate)
--	onstate=the conductor's nodename when it is turned on
--	offstate=the conductor's nodename when it is turned off
--
--As you can see, conductors need an offstate and an onstate node, just like receptors
--mesecons:mesecon_on / mesecons:mesecon_off are the default conductors
--Other conductors connect to other conductors. It's always "the same energy"
--! As there is no special drawtype, conductors don't connect to others visually,
--but it works in fact.
--
--The function # mesecon:register_conductor(onstate, offstate) is the only thing you need to do,
--the mod does everything else for you (turn the conductor on and off...)

-- INCLUDE SETTINGS
dofile(minetest.get_modpath("mesecons").."/settings.lua")

-- PUBLIC VARIABLES
mesecon={} -- contains all functions and all global variables
mesecon.actions_on={} -- Saves registered function callbacks for mesecon on
mesecon.actions_off={} -- Saves registered function callbacks for mesecon off
mesecon.actions_change={} -- Saves registered function callbacks for mesecon change
mesecon.pwr_srcs={}
mesecon.pwr_srcs_off={}
mesecon.rules={}
mesecon.conductors={}

--Internal API
dofile(minetest.get_modpath("mesecons").."/internal_api.lua");



-- MESECONS

minetest.register_node("mesecons:mesecon_off", {
	drawtype = "raillike",
	tile_images = {"jeija_mesecon_off.png", "jeija_mesecon_curved_off.png", "jeija_mesecon_t_junction_off.png", "jeija_mesecon_crossing_off.png"},
	inventory_image = "jeija_mesecon_off.png",
	wield_image = "jeija_mesecon_off.png",
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
	},
	groups = {dig_immediate=3},
    	description="Mesecons",
})

minetest.register_node("mesecons:mesecon_on", {
	drawtype = "raillike",
	tile_images = {"jeija_mesecon_on.png", "jeija_mesecon_curved_on.png", "jeija_mesecon_t_junction_on.png", "jeija_mesecon_crossing_on.png"},
	paramtype = "light",
	is_ground_content = true,
	walkable = false,
	selection_box = {
		type = "fixed",
	},
	groups = {dig_immediate=3},
	drop = '"mesecons:mesecon_off" 1',
	light_source = LIGHT_MAX-11,
})

minetest.register_craft({
	output = '"mesecons:mesecon_off" 16',
	recipe = {
		{'"default:mese"'},
	}
})

mesecon:register_conductor("mesecons:mesecon_on", "mesecons:mesecon_off")

-- API API API API API API API API API API API API API API API API API API

function mesecon:add_receptor_node(nodename, rules, get_rules) --rules table is optional; if rules depend on param2 pass (nodename, nil, function get_rules)
	local i=1
	repeat
		if mesecon.pwr_srcs[i]==nil then break end
		i=i+1
	until false
	if get_rules==nil and rules==nil then
		rules=mesecon:get_rules("default")
	end
	mesecon.pwr_srcs[i]={}
	mesecon.pwr_srcs[i].name=nodename
	mesecon.pwr_srcs[i].rules=rules
	mesecon.pwr_srcs[i].get_rules=get_rules
end

function mesecon:add_receptor_node_off(nodename, rules, get_rules)
	local i=1
	repeat
		if mesecon.pwr_srcs_off[i]==nil then break end
		i=i+1
	until false
	if get_rules==nil and rules==nil then
		rules=mesecon:get_rules("default")
	end
	mesecon.pwr_srcs_off[i]={}
	mesecon.pwr_srcs_off[i].name=nodename
	mesecon.pwr_srcs_off[i].rules=rules
	mesecon.pwr_srcs_off[i].get_rules=get_rules
end

function mesecon:receptor_on(pos, rules)
	mesecon:turnon(pos, 0, 0, 0, true, rules)
end

function mesecon:receptor_off(pos, rules)
	mesecon:turnoff(pos, 0, 0, 0, true, rules)
end

function mesecon:register_on_signal_on(action)
	local i	= 1	
	repeat
		i=i+1
		if mesecon.actions_on[i]==nil then break end
	until false
	mesecon.actions_on[i]=action
end

function mesecon:register_on_signal_off(action)
	local i	= 1	
	repeat
		i=i+1
		if mesecon.actions_off[i]==nil then break end
	until false
	mesecon.actions_off[i]=action
end

function mesecon:register_on_signal_change(action)
	local i	= 1	
	repeat
		i=i+1
		if mesecon.actions_change[i]==nil then break end
	until false
	mesecon.actions_change[i]=action
end

mesecon:add_rules("default", 
{{x=0,  y=0,  z=-1},
{x=1,  y=0,  z=0},
{x=-1, y=0,  z=0},
{x=0,  y=0,  z=1},
{x=1,  y=1,  z=0},
{x=1,  y=-1, z=0},
{x=-1, y=1,  z=0},
{x=-1, y=-1, z=0},
{x=0,  y=1,  z=1},
{x=0,  y=-1, z=1},
{x=0,  y=1,  z=-1},
{x=0,  y=-1, z=-1}})

print("[MESEcons] Main mod Loaded!")

--minetest.register_on_newplayer(function(player)
	--local i=1
	--while mesecon.wireless_receivers[i]~=nil do
	--	pos=mesecon.wireless_receivers[i].pos
	--	request=mesecon.wireless_receivers[i].requested_state
	--	inverting=mesecon.wireless_receivers[i].inverting
	--	if request==inverting then
	--		mesecon:receptor_off(pos)
	--	end
	--	if request~=inverting  then
	--		mesecon:receptor_on(pos)
	--	end
	--end
--end)
