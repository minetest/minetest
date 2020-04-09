
vector = {}

function vector.new(a, b, c)
	if type(a) == "table" then
		assert(a.x and a.y and a.z, "Invalid vector passed to vector.new()")
		return {x=a.x, y=a.y, z=a.z}
	elseif a then
		assert(b and c, "Invalid arguments for vector.new()")
		return {x=a, y=b, z=c}
	end
	return {x=0, y=0, z=0}
end

function vector.equals(a, b)
	return a.x == b.x and
	       a.y == b.y and
	       a.z == b.z
end

function vector.length(v)
	return math.hypot(v.x, math.hypot(v.y, v.z))
end

function vector.normalize(v)
	local len = vector.length(v)
	if len == 0 then
		return {x=0, y=0, z=0}
	else
		return vector.divide(v, len)
	end
end

function vector.floor(v)
	return {
		x = math.floor(v.x),
		y = math.floor(v.y),
		z = math.floor(v.z)
	}
end

function vector.round(v)
	return {
		x = math.floor(v.x + 0.5),
		y = math.floor(v.y + 0.5),
		z = math.floor(v.z + 0.5)
	}
end

function vector.apply(v, func)
	return {
		x = func(v.x),
		y = func(v.y),
		z = func(v.z)
	}
end

function vector.distance(a, b)
	local x = a.x - b.x
	local y = a.y - b.y
	local z = a.z - b.z
	return math.hypot(x, math.hypot(y, z))
end

function vector.direction(pos1, pos2)
	return vector.normalize({
		x = pos2.x - pos1.x,
		y = pos2.y - pos1.y,
		z = pos2.z - pos1.z
	})
end

function vector.angle(a, b)
	local dotp = vector.dot(a, b)
	local cp = vector.cross(a, b)
	local crossplen = vector.length(cp)
	return math.atan2(crossplen, dotp)
end

function vector.dot(a, b)
	return a.x * b.x + a.y * b.y + a.z * b.z
end

function vector.cross(a, b)
	return {
		x = a.y * b.z - a.z * b.y,
		y = a.z * b.x - a.x * b.z,
		z = a.x * b.y - a.y * b.x
	}
end

function vector.add(a, b)
	if type(b) == "table" then
		return {x = a.x + b.x,
			y = a.y + b.y,
			z = a.z + b.z}
	else
		return {x = a.x + b,
			y = a.y + b,
			z = a.z + b}
	end
end

function vector.subtract(a, b)
	if type(b) == "table" then
		return {x = a.x - b.x,
			y = a.y - b.y,
			z = a.z - b.z}
	else
		return {x = a.x - b,
			y = a.y - b,
			z = a.z - b}
	end
end

function vector.multiply(a, b)
	if type(b) == "table" then
		return {x = a.x * b.x,
			y = a.y * b.y,
			z = a.z * b.z}
	else
		return {x = a.x * b,
			y = a.y * b,
			z = a.z * b}
	end
end

function vector.divide(a, b)
	if type(b) == "table" then
		return {x = a.x / b.x,
			y = a.y / b.y,
			z = a.z / b.z}
	else
		return {x = a.x / b,
			y = a.y / b,
			z = a.z / b}
	end
end

function vector.sort(a, b)
	return {x = math.min(a.x, b.x), y = math.min(a.y, b.y), z = math.min(a.z, b.z)},
		{x = math.max(a.x, b.x), y = math.max(a.y, b.y), z = math.max(a.z, b.z)}
end

local function sin(x)
	if x % math.pi == 0 then
		return 0
	else
		return math.sin(x)
	end
end

local function cos(x)
	if x % math.pi == math.pi / 2 then
		return 0
	else
		return math.cos(x)
	end
end

function vector.rotate_around_axis(v, axis, angle)
	--flip the angle because the formula is for the right hand rule
	angle = -angle
	local cosangle = cos(angle)
	local sinangle = sin(angle)
	axis = vector.normalize(axis)
	--https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
	local dot_axis = vector.multiply(axis, vector.dot(axis, v))
	return vector.add(
		vector.multiply(vector.cross(axis, v), sinangle),
		vector.add(
		vector.multiply(vector.subtract(v, dot_axis), cosangle),
		dot_axis
	))
end

function vector.rotate(v, rot)
	local sinpitch = sin(-rot.x)
	local sinyaw = sin(-rot.y)
	local sinroll = sin(-rot.z)
	local cospitch = cos(rot.x)
	local cosyaw = cos(rot.y)
	local cosroll = math.cos(rot.z)
	--rotation matrix that applies yaw, pitch and roll
	--result of matrix multiplying the rotation matrices for yaw, pitch and roll
	local matrix = {
		{
			cosyaw * cospitch,
			cosyaw * sinpitch * sinroll - sinyaw * cosroll,
			cosyaw * sinpitch * cosroll + sinyaw * sinroll
		},
		{
			sinyaw * cospitch,
			sinyaw * sinpitch * sinroll + cosyaw * cosroll,
			sinyaw * sinpitch * cosroll - cosyaw * sinroll
		},
		{
			-sinpitch,
			cospitch * sinroll,
			cospitch * cosroll
		},
	}
	local keys = {"z", "x", "y"}
	--compute matrix multiplication: `matrix` * `v`
	local ret = vector.new(0, 0, 0)
	for i = 1, 3 do
		local row = matrix[i]
		for ii = 1, 3 do
			ret[keys[i]] = ret[keys[i]] + row[ii] * v[keys[ii]]
		end
	end
	return ret
end


function vector.forward_at_rotation(rot)
	return vector.rotate({x = 0, y = 0, z = 1}, rot)
end

function vector.up_at_rotation(rot)
	return vector.rotate({x = 0, y = 1, z = 0}, rot)
end

function vector.directions_to_rotation(forward, up)
	local rot = {x = math.asin(forward.y), y = -math.atan2(forward.x, forward.z), z = 0}
	--calculate vector pointing uwith roll = 0, just based on forward vector
	local forwup = vector.up_at_rotation(rot)
	--'forwup' and 'up' are now in a plane with 'forward' as normal.
	--the angle between them is equal to math.abs(roll)
	rot.z = vector.angle(forwup, up)
	--if 'up' rotated by rot.z is equal to forwup, then roll = -rot.z, else roll = rot.z
	--we don't use vector.equals for this comparison because of floating point rounding errors
	if vector.distance(vector.rotate_around_axis(up, forward, rot.z), forwup) < 0.0000000000001 then
		rot.z = -rot.z
	end
	return rot
end
