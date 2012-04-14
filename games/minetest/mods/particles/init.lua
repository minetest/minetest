------------- Settings --------------
BLOCK_BREAK_PARTICLES = 8
-------------------------------------
SMOKE = {
    physical = true,
    collisionbox = {-0.1,-0.1,-0.1,0,0,0},
    visual = "sprite",
    textures = {"smoke.png"},
    on_step = function(self, dtime)
        self.object:setacceleration({x=0, y=0.5, z=0})
        self.timer = self.timer + dtime
        if self.timer > 3 then
            self.object:remove()
        end
    end,
    timer = 0,
}

minetest.register_entity("particles:smoke", SMOKE)
minetest.register_abm({
	nodenames = {"default:torch"},
	interval = 10,
	chance = 10,
	action = function(pos)
		minetest.env:add_entity({x=pos.x+math.random()*0.5,y=pos.y-0.25,z=pos.z+math.random()*0.5}, "particles:smoke")
	end,
})

if minetest.get_modpath("jeija") ~= nil then -- Mesecons is installed
    MESECONDUST = {
        physical = true,
        collisionbox = {-0.1,-0.1,-0.1,0,0,0},
        visual = "sprite",
        textures = {"mesecondust.png"},
        on_step = function(self, dtime)
            self.timer = self.timer + dtime
            if self.timer > 2.5 then
                self.object:remove()
            end
        end,
        timer = 0,
    }
    minetest.register_entity("particles:mesecondust", MESECONDUST)
    minetest.register_abm({
        nodenames = {"jeija:mesecon_on","jeija:wall_lever_on","jeija:mesecon_torch_on"},
        interval = 1,
        chance = 5,
        action = function(pos)
            --minetest.env:add_entity({x=pos.x+math.random()*0.5,y=pos.y,z=pos.z+math.random()*0.5}, "particles:mesecondust")
        end,
    })
end

nodename2color = {
--Brown
{"default:dirt","brown"},
{"default:chest","brown"},
{"default:chest_locked","brown"},
{"default:wood","brown"},
{"default:tree","brown"},
{"default:jungletree","brown"},
{"default:bookshelf","brown"},
{"default:sign_wall","brown"},
{"default:ladder","brown"},
{"default:fence_wood","brown"},
--Red
{"default:apple","red"},
{"default:brick","red"},
--Green
{"default:cactus","green"},
{"default:junglegrass","green"},
{"default:dirt_with_grass","green"},
{"default:sapling","green"},
{"default:papyrus","green"},
{"default:leaves","green"},
--Gray
{"default:cobble","gray"},
{"default:furnace","gray"},
{"default:stone","gray"},
{"default:stone_with_iron","gray"},
{"default:rail","gray"},
{"default:mossycobble","gray"},
--Lightgray
{"default:steelblock","lightgray"},
{"default:clay","lightgray"},
--Yellow
{"default:mese","yellow"},
{"default:torch","yellow"},
--Sandcolor
{"default:sand","sandcolor"},
{"default:sandstone","sandcolor"},
--Black
{"default:gravel","black"},
{"default:stone_with_coal","black"},
--White
{"default:cloud","white"},
{"default:glass","white"},
--=== Mesecons ===--
{"mesecons_powerplant:power_plant", "yellow"},
{"mesecons_random:removestone", "gray"},
{"mesecons_lamp:lamp_off", "yellow"},
{"mesecons_lamp:lamp_on", "yellow"},
{"mesecons:mesecon_off", "yellow"},
{"mesecons:mesecon_on", "yellow"},
{"mesecons_detector:object_detector_off", "lightgray"},
{"mesecons_detector:object_detector_on", "lightgray"},
{"mesecons_wireless:wireless_inverter_on", "brown"},
{"mesecons_wireless:wireless_inverter_off", "brown"},
{"mesecons_wireless:wireless_receiver_on", "brown"},
{"mesecons_wireless:wireless_receiver_off", "brown"},
{"mesecons_wireless:wireless_transmitter_on", "brown"},
{"mesecons_wireless:wireless_transmitter_off", "brown"},
{"mesecons_switch:mesecon_switch_off", "gray"},
{"mesecons_switch:mesecon_switch_off", "gray"},
{"mesecons_button:button_on", "yellow"},
{"mesecons_button:button_off", "yellow"},
{"mesecons_pistons:piston_normal", "brown"},
{"mesecons_pistons:piston_sticky", "brown"},
{"mesecons_blinkyplant:blinky_plant_off", "yellow"},
{"mesecons_blinkyplant:blinky_plant_on", "yellow"},
{"mesecons_torch:mesecon_torch_off", "yellow"},
{"mesecons_torch:mesecon_torch_on", "yellow"},
{"mesecons_hydroturbine:hydro_turbine_off", "gray"},
{"mesecons_hydroturbine:hydro_turbine_on", "gray"},
{"mesecons_pressureplates:pressure_plate_stone_off", "gray"},
{"mesecons_pressureplates:pressure_plate_stone_on", "gray"},
{"mesecons_pressureplates:pressure_plate_wood_off", "brown"},
{"mesecons_pressureplates:pressure_plate_wood_on", "brown"},
{"mesecons_temperest:mesecon_socket_off", "gray"},
{"mesecons_temperest:mesecon_socket_on", "red"},
{"mesecons_temperest:mesecon_inverter_off", "gray"},
{"mesecons_temperest:mesecon_inverter_on", "red"},
{"mesecons_temperest:mesecon_plug", "black"},
{"mesecons_movestones:movestone", "gray"},
{"mesecons_movestones:sticky_movestone", "gray"},
}

reg_colors = {}

for idx, tbl in pairs(nodename2color) do
    nn = tbl[1]
    color = tbl[2]
    if reg_colors[color] == nil then
        local TEMP = {
            physical = true,
            collisionbox = {-0.1,-0.1,-0.1,0,0,0},
            visual = "sprite",
            textures = {"p_"..color..".png"},
            timer = 0,
            on_step = function(self, dtime)
                self.timer = self.timer + dtime
                if self.timer > 1 then
                    self.object:remove()
                end
            end,
            on_activate = function(self, staticdata)
                self.object:setacceleration({x=0, y=-7.5, z=0})
            end,
        }
        minetest.register_entity("particles:p_"..color, TEMP)
        reg_colors[color] = true
    end
end

minetest.register_on_dignode(function(pos, oldnode, digger)
    for idx, tbl in pairs(nodename2color) do
        if oldnode.name == tbl[1] then
            for x = 1,BLOCK_BREAK_PARTICLES,1 do
                e = minetest.env:add_entity({x=pos.x+1-(math.random()*1.5),y=pos.y+0.25,z=pos.z+1-(math.random()*1.5)}, "particles:p_"..tbl[2])
                e:setvelocity({x=math.random(),y=1,z=math.random()})
            end
        end
    end
end)
