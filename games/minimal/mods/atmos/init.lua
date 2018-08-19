-- atmos, WTFPL for minimal_game

atmos_enabled = true

atmos = {}

local atmos_clear_weather = {}

local storage = minetest.get_modpath("atmos").."/skybox/"
local val = 0

for line in io.lines(storage.."skybox_clear_gradient"..".atm") do

	atmos_clear_weather[val] = minetest.deserialize(line) 

	val = val + 1

end

local function atmos_ratio(current, next, ctime2)

	if current < next then -- check if we're darker than the next skybox frame

		local ratio = (next - current) * ctime2
		return (current + ratio)
	
	else -- we darken instead, this repeats for the next two if, else statements

		local ratio = (current - next) * ctime2
		return (current - ratio)
	
	end	

end

function atmos.set_skybox(player)

	--local skybox = atmos.get_weather_skybox()
	local ctime = minetest.get_timeofday() * 100

	-- figure out our multiplier since get_timeofday returns 0->1, we can use the sig figs as a 0-100 percentage multiplier

	-- contributed by @rubenwardy

	local ctime2 = math.floor((ctime - math.floor(ctime)) * 100) / 100

	if ctime2 == 0 then ctime2 = 0.01 end -- anti sudden skybox change syndrome

	local fade_factor =  math.floor(255 * ctime2)
	
	ctime = math.floor(ctime) -- remove the sig figs, since we're accessing table points

	-- assemble the skyboxes to fade neatly

	local side_string = "(atmos_sky.png^[multiply:".. atmos_clear_weather[ctime].bottom .. ")^" .. "(atmos_sky_top.png^[multiply:" .. atmos_clear_weather[ctime].top .. ")"
	local side_string_new = "(atmos_sky.png^[multiply:".. atmos_clear_weather[ctime+1].bottom .. ")^" .. "(atmos_sky_top.png^[multiply:" .. atmos_clear_weather[ctime+1].top .. ")"

	local sky_top = "(atmos_sky.png^[multiply:".. atmos_clear_weather[ctime].bottom .. ")^(atmos_sky_top_radial.png^[multiply:".. atmos_clear_weather[ctime].top .. ")"
	local sky_top_new = "(atmos_sky.png^[multiply:".. atmos_clear_weather[ctime+1].bottom .. ")^(atmos_sky_top_radial.png^[multiply:".. atmos_clear_weather[ctime+1].top .. ")"

	local sky_bottom = "(atmos_sky.png^[multiply:".. atmos_clear_weather[ctime].bottom .. ")"
	local sky_bottom_new = "(atmos_sky.png^[multiply:".. atmos_clear_weather[ctime+1].bottom .. ")"

	-- let's convert the base colour to convert it into our transitioning fog colour:

	local fog = {}

	fog.current = {} -- we need two tables for comparing, as any matching pairs of hex will be skipped.
	fog.next = {}
	fog.result = {}

	fog.current.red = 0
	fog.current.grn = 0
	fog.current.blu = 0

	fog.next.red = 0
	fog.next.grn = 0
	fog.next.blu = 0

	fog.result.red = 0
	fog.result.grn = 0
	fog.result.blu = 0

	-- convert our hex into compatible minetest.rgba components:
	-- we need these to make our lives easier when it comes to uh, things.

	fog.current.red = tonumber("0x" .. atmos_clear_weather[ctime].base:sub(2,3))
	fog.current.grn = tonumber("0x" .. atmos_clear_weather[ctime].base:sub(4,5))
	fog.current.blu = tonumber("0x" .. atmos_clear_weather[ctime].base:sub(6,7))

	fog.next.red = tonumber("0x" .. atmos_clear_weather[ctime+1].base:sub(2,3))
	fog.next.grn = tonumber("0x" .. atmos_clear_weather[ctime+1].base:sub(4,5))
	fog.next.blu = tonumber("0x" .. atmos_clear_weather[ctime+1].base:sub(6,7))

	if atmos_clear_weather[ctime].base ~= atmos_clear_weather[ctime+1].base then

		-- we compare colours the same way we do it for the light level

		fog.result.red = atmos_ratio(fog.current.red, fog.next.red, ctime2)
		fog.result.grn = atmos_ratio(fog.current.grn, fog.next.grn, ctime2)
		fog.result.blu = atmos_ratio(fog.current.blu, fog.next.blu, ctime2)

	else

		fog.result.red = fog.current.red
		fog.result.grn = fog.current.grn
		fog.result.blu = fog.current.blu

	end

	if atmos_clear_weather[ctime].bottom == atmos_clear_weather[ctime+1].bottom then -- prevent more leakage
		if atmos_clear_weather[ctime].top == atmos_clear_weather[ctime+1].top then
			fade_factor = 0
		end
	end
	
	player:set_sky({
        sky_color = minetest.rgba(fog.result.red, fog.result.grn, fog.result.blu), 
		type = "dynamic",
		textures = {
			sky_top .. "^(" .. sky_top_new .. "^[opacity:" .. fade_factor .. ")",
			sky_bottom .. "^(" .. sky_bottom_new .. "^[opacity:" .. fade_factor .. ")",
	
			side_string .. "^(" .. side_string_new .. "^[opacity:" .. fade_factor .. ")",
			side_string .. "^(" .. side_string_new .. "^[opacity:" .. fade_factor .. ")",
			side_string .. "^(" .. side_string_new .. "^[opacity:" .. fade_factor .. ")",
			side_string .. "^(" .. side_string_new .. "^[opacity:" .. fade_factor .. ")"
        },
        clouds = true,
        custom_fog = true,
    	sun = {
            visible = true,
            yaw = 90,
            tilt = -5,
			texture = "sun.png",
			sunrise_glow = true,
		},
		
		moon = {
			visible = true,
			yaw = -90,
			tilt = -5,
			texture = "moon.png",	
		},

		stars = {
			visible = true,
			yaw = 0,
			tilt = 0,
			count = 1000,
			
		}
	})
		
	player:set_clouds({
		
		density = 0.4,
		color = "#fff0f0e5",
		thickness = 16,
		height = 210,
	
	})
	
	local light_ratio = 0
	local light_level = 0

	if atmos_clear_weather[ctime].light == atmos_clear_weather[ctime+1].light then -- we do nothing, because there's nothing worth doing
		
		light_level = atmos_clear_weather[ctime].light

	else -- we do the light to dark fade 

		light_level = atmos_ratio(atmos_clear_weather[ctime].light, atmos_clear_weather[ctime+1].light, ctime2)
		
	end

	if light_level > 1 then light_level = 1 end -- sanity checks, going over 1 makes it dark again
	if light_level < 0 then light_level = 0 end -- going under 0 makes it bright again

	player:override_day_night_ratio(light_level)
	
end


function atmos.sync_skybox()

	-- sync skyboxes to all players connected to the server.

	for _, player in ipairs(minetest.get_connected_players()) do
	
		-- do not sync the current weather to players under -32 depth.
	
		if player:get_pos().y <= -32 then
		
			player:set_sky("#000000", "plain", false)
		
			player:override_day_night_ratio(0)
		
		elseif player:get_pos().y > 10000 then

			-- change to low orbit skybox here

		else
		
			-- sync weather to players that are above -32, lightning effects (and flashes) only affects players above -16
		
			atmos.set_skybox(player)
		
		end
	
	end
	
	minetest.after(0.1, atmos.sync_skybox)
end

if atmos_enabled then

	minetest.after(2, atmos.sync_skybox)

end