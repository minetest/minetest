--There are 5 music periods: ~4:30, 9, 12:30, 17, 22, 2 o'clock
music_playing=false

minetest.register_globalstep(function (dtime)
	if math.random(100)==1 then
		local time=minetest.env:get_timeofday()*24000
		local soundfile=""
		if resembles(time, 4500,  100) then
			soundfile = "music_sunrises"
		elseif resembles(time, 9000,  100) then
			soundfile = "music_morning"
		elseif resembles(time, 12500, 100) then
			soundfile = "music_noon"
		elseif resembles(time, 17000, 100) then
			soundfile = "music_sunset"
		elseif resembles(time, 22000, 100) then
			soundfile = "music_evening"
		elseif resembles(time, 2000,  100) then
			soundfile = "music_night"
		else
			music_playing=false
		end
		minetest.sound_play(soundfile, {gain=0.5})
		music_playing=true
	end
end)

function resembles(value1, value2, range)
	if value1+(range/2)>=value2 and value1-(range/2)<=value2 then
		return true
	end
	return false
end
