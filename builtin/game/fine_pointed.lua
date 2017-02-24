-- Returns the exact coordinate of a pointed surface

local function fine_pointed(pos, camera_pos, pos_off, look_dir)
	local offset, nc
	local oc = {}
	for c, v in pairs(pos_off) do
		if v == 0 then
			oc[#oc + 1] = c
		else
			offset = v
			nc = c
		end
	end

	local fine_pos = {[nc] = pos[nc] + offset}

	camera_pos.y = camera_pos.y + 1.625

	local f = (pos[nc] + offset - camera_pos[nc]) / look_dir[nc]
	for i = 1, #oc do
		fine_pos[oc[i]] = camera_pos[oc[i]] + look_dir[oc[i]] * f
	end

	camera_pos.y = camera_pos.y - 1.625
	return fine_pos
end

function minetest.fine_pointed_position(placer, pointed_thing)
	return fine_pointed(
		pointed_thing.under,
		placer:get_pos(),
		vector.multiply(vector.subtract(pointed_thing.above, pointed_thing.under), 0.5),
		placer:get_look_dir()
	)
end
