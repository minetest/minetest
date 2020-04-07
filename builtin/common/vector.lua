--[[
Vector helpers

Note: The vector.*-functions must be able to accept old vectors that had no
      metatables and looked like this: {x = 42, y = 13, z = 0}
]]

vector = {}

-- operations that are only accessed via v:func(...). the first argument is
-- always a vector with metatable
local vector_new = {}

local metatable = {}
vector.metatable = metatable

local xyz = {x = 1, y = 2, z = 3}

-- only called when rawget(v, key) returns nil
function metatable.__index(v, key)
	return rawget(v, xyz[key]) or vector_new[key] or vector[key]
end

-- only called when rawget(v, key) returns nil
function metatable.__newindex(v, key, value)
	rawset(v, xyz[key] or key, value)
end

-- constructors

local function fast_new(x, y, z)
	local v = setmetatable({x, y, z}, metatable)
	return v
end

function vector.new(a, b, c)
	if type(a) == "table" then
		assert(a.x and a.y and a.z, "Invalid vector passed to vector.new()")
		return fast_new(a.x, a.y, a.z)
	elseif a then
		assert(b and c, "Invalid arguments for vector.new()")
		return fast_new(a, b, c)
	end
	return fast_new(0, 0, 0)
end

function vector.equals(a, b)
	return a.x == b.x and
	       a.y == b.y and
	       a.z == b.z
end
function vector_new.equals(a, b)
	return a[1] == b.x and
	       a[2] == b.y and
	       a[3] == b.z
end
function metatable.__eq(a, b)
	return a[1] == b[1] and
	       a[2] == b[2] and
	       a[3] == b[3]
end

-- unary operations

function vector.length(v)
	return math.hypot(v.x, math.hypot(v.y, v.z))
end
function vector_new.length(v)
	return math.hypot(v[1], math.hypot(v[2], v[3]))
end
-- Note: we can not use __len because it is already used for primitive table length

function vector.normalize(v)
	local len = vector.length(v)
	if len == 0 then
		return fast_new(0, 0, 0)
	else
		return vector.divide(v, len)
	end
end
function vector_new.normalize(v)
	local len = v:length()
	if len == 0 then
		return fast_new(0, 0, 0)
	else
		return v / len
	end
end

function vector.floor(v)
	return vector.apply(v, math.floor)
end
function vector_new.floor(v)
	return v:apply(math.floor)
end

function vector.round(v)
	return fast_new(
		math.floor(v.x + 0.5),
		math.floor(v.y + 0.5),
		math.floor(v.z + 0.5)
	)
end
function vector_new.round(v)
	return fast_new(
		math.floor(v[1] + 0.5),
		math.floor(v[2] + 0.5),
		math.floor(v[3] + 0.5)
	)
end

function vector.apply(v, func)
	return fast_new(
		func(v.x),
		func(v.y),
		func(v.z)
	)
end
function vector_new.apply(v, func)
	return fast_new(
		func(v[1]),
		func(v[2]),
		func(v[3])
	)
end

function vector.distance(a, b)
	local x = a.x - b.x
	local y = a.y - b.y
	local z = a.z - b.z
	return math.hypot(x, math.hypot(y, z))
end
function vector_new.distance(a, b)
	local x = a[1] - b.x
	local y = a[2] - b.y
	local z = a[3] - b.z
	return math.hypot(x, math.hypot(y, z))
end

function vector.direction(pos1, pos2)
	return vector.subtract(pos2, pos1):normalize()
end

function vector.angle(a, b)
	local dotp = vector.dot(a, b)
	local cp = vector.cross(a, b)
	local crossplen = cp:length()
	return math.atan2(crossplen, dotp)
end
function vector_new.angle(a, b)
	local dotp = a:dot(b)
	local cp = a:cross(b)
	local crossplen = cp:length()
	return math.atan2(crossplen, dotp)
end

function vector.dot(a, b)
	return a.x * b.x + a.y * b.y + a.z * b.z
end
function vector_new.dot(a, b)
	return a[1] * b.x + a[2] * b.y + a[3] * b.z
end

function vector.cross(a, b)
	return fast_new(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	)
end
function vector_new.cross(a, b)
	return fast_new(
		a[2] * b.z - a[3] * b.y,
		a[3] * b.x - a[1] * b.z,
		a[1] * b.y - a[2] * b.x
	)
end

function metatable.__unm(v)
	return fast_new(-v[1], -v[2], -v[3])
end

-- add, sub, mul, div operations

function vector.add(a, b)
	if type(b) == "table" then
		return fast_new(
			a.x + b.x,
			a.y + b.y,
			a.z + b.z
		)
	else
		return fast_new(
			a.x + b,
			a.y + b,
			a.z + b
		)
	end
end
function vector_new.add(a, b)
	if type(b) == "table" then
		return fast_new(
			a[1] + b.x,
			a[2] + b.y,
			a[3] + b.z
		)
	else
		return fast_new(
			a[1] + b,
			a[2] + b,
			a[3] + b
		)
	end
end
function metatable.__add(a, b)
	return fast_new(
		a[1] + b[1],
		a[2] + b[2],
		a[3] + b[3]
	)
end

-- the useless v-s where s is a scalar needs to be supported by vector.subtract
function vector.subtract(a, b)
	if type(b) == "table" then
		return fast_new(
			a.x - b.x,
			a.y - b.y,
			a.z - b.z
		)
	else
		return fast_new(
			a.x - b,
			a.y - b,
			a.z - b
		)
	end
end
function vector_new.subtract(a, b)
	if type(b) == "table" then
		return fast_new(
			a[1] - b.x,
			a[2] - b.y,
			a[3] - b.z
		)
	else
		return fast_new(
			a[1] - b,
			a[2] - b,
			a[3] - b
		)
	end
end
function metatable.__sub(a, b)
	return fast_new(
		a[1] - b[1],
		a[2] - b[2],
		a[3] - b[3]
	)
end

-- deprecated schur product needs to be supported by vector.multiply
function vector.multiply(a, b)
	if type(b) == "table" then
		return fast_new(
			a.x * b.x,
			a.y * b.y,
			a.z * b.z
		)
	else
		return fast_new(
			a.x * b,
			a.y * b,
			a.z * b
		)
	end
end
function vector_new.multiply(a, b)
	return fast_new(
		a[1] * b,
		a[2] * b,
		a[3] * b
	)
end
function metatable.__mul(a, b)
	if type(a) == "table" then
		return fast_new(
			a[1] * b,
			a[2] * b,
			a[3] * b
		)
	else
		return fast_new(
			a * b[1],
			a * b[2],
			a * b[3]
		)
	end
end

-- deprecated schur quotient needs to be supported by vector.divide
function vector.divide(a, b)
	if type(b) == "table" then
		return fast_new(
			a.x / b.x,
			a.y / b.y,
			a.z / b.z
		)
	else
		return fast_new(
			a.x / b,
			a.y / b,
			a.z / b
		)
	end
end
function vector_new.divide(a, b)
	return fast_new(
		a[1] / b,
		a[2] / b,
		a[3] / b
	)
end
metatable.__div = vector_new.divide -- scalar/vector makes no sense

-- misc stuff

function vector.offset(v, x, y, z)
	return fast_new(
		v.x + x,
		v.y + y,
		v.z + z
	)
end
function vector_new.offset(v, x, y, z)
	return fast_new(
		v[1] + x,
		v[2] + y,
		v[3] + z
	)
end

function vector.sort(a, b)
	return fast_new(math.min(a.x, b.x), math.min(a.y, b.y), math.min(a.z, b.z)),
		fast_new(math.max(a.x, b.x), math.max(a.y, b.y), math.max(a.z, b.z))
end
function vector_new.sort(a, b)
	return fast_new(math.min(a[1], b.x), math.min(a[2], b.y), math.min(a[3], b.z)),
		fast_new(math.max(a[1], b.x), math.max(a[2], b.y), math.max(a[3], b.z))
end

function vector.is_vector(v)
	return getmetatable(v) == metatable
end

function vector.ipairs(v)
	return ipairs(v)
end

local function vec_next(v, last_index)
	if last_index == nil then
		return "x", v[1]
	elseif last_index == "x" then
		return "y", v[2]
	elseif last_index == "y" then
		return "z", v[3]
	else
		return nil
	end
end

function vector.pairs(v)
	return vec_next, v, nil
end

-- override next() for backwards compatibility reasons
-- pairs on a vector should give "x", "y" and "z" as keys
local old_next = next
function next(t, index, ...)
	if getmetatable(t) ~= metatable then
		return old_next(t, index, ...)
	end

	-- t is a vector
	-- transform x/y/z to internal 1/2/3
	if index == "x" then
		index = 1
	elseif index == "y" then
		index = 2
	elseif index == "z" then
		index = 3
	end

	local k, v = old_next(t, index, ...)

	-- transform internal 1/2/3 to x/y/z
	if k == 1 then
		return "x", v
	elseif k == 2 then
		return "y", v
	elseif k == 3 then
		return "z", v
	end

	-- the vector is abused, there is some other value
	return k, v
end

-- also override pairs as it would otherwise return the old next
function pairs(t)
	return next, t, nil
end

-- rotation helpers

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
	local cosangle = cos(angle)
	local sinangle = sin(angle)
	axis = vector.normalize(axis)
	-- https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
	local dot_axis = vector.multiply(axis, vector.dot(axis, v))
	local cross = vector.cross(v, axis)
	return vector.new(
		cross.x * sinangle + (v.x - dot_axis.x) * cosangle + dot_axis.x,
		cross.y * sinangle + (v.y - dot_axis.y) * cosangle + dot_axis.y,
		cross.z * sinangle + (v.z - dot_axis.z) * cosangle + dot_axis.z
	)
end

function vector.rotate(v, rot)
	local sinpitch = sin(-rot.x)
	local sinyaw = sin(-rot.y)
	local sinroll = sin(-rot.z)
	local cospitch = cos(rot.x)
	local cosyaw = cos(rot.y)
	local cosroll = math.cos(rot.z)
	-- Rotation matrix that applies yaw, pitch and roll
	local matrix = {
		{
			sinyaw * sinpitch * sinroll + cosyaw * cosroll,
			sinyaw * sinpitch * cosroll - cosyaw * sinroll,
			sinyaw * cospitch,
		},
		{
			cospitch * sinroll,
			cospitch * cosroll,
			-sinpitch,
		},
		{
			cosyaw * sinpitch * sinroll - sinyaw * cosroll,
			cosyaw * sinpitch * cosroll + sinyaw * sinroll,
			cosyaw * cospitch,
		},
	}
	-- Compute matrix multiplication: `matrix` * `v`
	return vector.new(
		matrix[1][1] * v.x + matrix[1][2] * v.y + matrix[1][3] * v.z,
		matrix[2][1] * v.x + matrix[2][2] * v.y + matrix[2][3] * v.z,
		matrix[3][1] * v.x + matrix[3][2] * v.y + matrix[3][3] * v.z
	)
end

function vector.dir_to_rotation(forward, up)
	forward = vector.normalize(forward)
	local rot = {x = math.asin(forward.y), y = -math.atan2(forward.x, forward.z), z = 0}
	if not up then
		return rot
	end
	assert(vector.dot(forward, up) < 0.000001,
			"Invalid vectors passed to vector.dir_to_rotation().")
	up = vector.normalize(up)
	-- Calculate vector pointing up with roll = 0, just based on forward vector.
	local forwup = vector.rotate({x = 0, y = 1, z = 0}, rot)
	-- 'forwup' and 'up' are now in a plane with 'forward' as normal.
	-- The angle between them is the absolute of the roll value we're looking for.
	rot.z = vector.angle(forwup, up)

	-- Since vector.angle never returns a negative value or a value greater
	-- than math.pi, rot.z has to be inverted sometimes.
	-- To determine wether this is the case, we rotate the up vector back around
	-- the forward vector and check if it worked out.
	local back = vector.rotate_around_axis(up, forward, -rot.z)

	-- We don't use vector.equals for this because of floating point imprecision.
	if (back.x - forwup.x) * (back.x - forwup.x) +
			(back.y - forwup.y) * (back.y - forwup.y) +
			(back.z - forwup.z) * (back.z - forwup.z) > 0.0000001 then
		rot.z = -rot.z
	end
	return rot
end
