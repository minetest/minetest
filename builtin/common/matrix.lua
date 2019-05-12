
matrix3 = {}

function matrix3.new(a, ...)
	if not a then
		return {0, 0, 0, 0, 0, 0, 0, 0, 0}
	elseif type(a) ~= "table" then
		return {a, ...}
	else
		return {a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]}
	end
end

matrix3.identity = {1, 0, 0, 0, 1, 0, 0, 0, 1}

function matrix3.apply(m, func)
	local mr = {}
	for i = 1, 9 do
		mr[i] = func(m[i])
	end
	return mr
end

function matrix3.equals(m1, m2)
	for i = 1, 9 do
		if m1[i] ~= m2[i] then
			return false
		end
	end
	return true
end

function matrix3.index(m, l, c)
	return m[(l - 1) * 3 + c]
end

function matrix3.add(m1, m2)
	local m3 = {}
	for i = 1, 9 do
		m3[i] = m1[i] + m2[i]
	end
	return m3
end

local function multiply_scalar(m, a)
	local mr = {}
	for i = 1, 9 do
		mr[i] = m[i] * a
	end
	return mr
end

local function vector_matrix3_multiply(m1, v)
	return {
		x = m1[1] * v.x + m1[2] * v.y + m1[3] * v.z,
		y = m1[4] * v.x + m1[5] * v.y + m1[6] * v.z,
		z = m1[7] * v.x + m1[8] * v.y + m1[9] * v.z,
	}
end

function matrix3.multiply(m1, m2)
	if type(m2) ~= "table" then
		return multiply_scalar(m1, m2)
	elseif m2.x then
		return vector_matrix3_multiply(m1, m2)
	end
	local m3 = {}
	for l = 1, 3 do
		for c = 1, 3 do
			local i = (l - 1) * 3 + c
			m3[i] = 0
			for k = 1, 3 do
				m3[i] = m3[i] + m1[(l - 1) * 3 + k] * m2[(k - 1) * 3 + c]
			end
		end
	end
	return m3
end

function matrix3.tensor_multiply(a, b)
	local m1 = matrix3.new()
	m1[1] = a.x
	m1[4] = a.y
	m1[7] = a.z
	local m2 = matrix3.new()
	m2[1] = a.x
	m2[2] = a.y
	m2[3] = a.z
	return matrix3.multiply(m1, m2)
end

function matrix3.transpose(m)
	return {m[1], m[4], m[7],
	        m[2], m[5], m[8],
	        m[3], m[6], m[9]}
end

-- note that minetest has a left-handed coordinate system.
-- ergo the rotation will be clockwise
-- (see wikipedia for details)

local rounding_precision = 1000000
local function round(a)
	return math.floor(a * rounding_precision + 0.5) / rounding_precision
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

function matrix3.rotation_around_x(angle)
	local s = sin(angle)
	local c = cos(angle)
	return {1, 0,  0,
	        0, c, -s,
	        0, s,  c}
end

function matrix3.rotation_around_y(angle)
	local s = sin(angle)
	local c = cos(angle)
	return { c, 0, s,
	         0, 1, 0,
	        -s, 0, c}
end

function matrix3.rotation_around_z(angle)
	local s = sin(angle)
	local c = cos(angle)
	return {c, -s, 0,
	        s,  c, 0,
	        0,  0, 1}
end

function matrix3.rotation_around_vector(v, angle)
	local length_v = vector.length(v)
	v = vector.divide(v, length_v)
	angle = angle or length_v

	local s = sin(angle)
	local c = cos(angle)
	local omc = 1 - c
	return {
		v.x * v.x * omc + c,       v.x * v.y * omc - v.z * s, v.x * v.z * omc + v.y * s,
		v.y * v.x * omc + v.z * s, v.y * v.y * omc + c,       v.y * v.z * omc - v.x * s,
		v.z * v.x * omc - v.y * s, v.z * v.y * omc + v.x * s, v.z * v.z * omc + c
	}
end

--~ function matrix3.to_pitch_yaw_roll(m)
	--~ local r = vector.new()
	--~ if round(math.abs(m[6])) ~= 1 then
		--~ r.x = -math.asin(m[6])
		--~ local cosx = math.cos(r.x)
		--~ r.y = math.atan2(m[3] / cosx, m[9] / cosx)
		--~ r.z = math.atan2(m[4] / cosx, m[5] / cosx)
	--~ elseif m[6] < 0 then -- gimbal lock
		--~ r.x = math.pi / 2
		--~ r.z = 0
		--~ r.y = math.atan2(m[2], m[1])
	--~ else
		--~ r.x = -math.pi / 2
		--~ r.z = 0
		--~ r.y = math.atan2(-m[2], m[1])
	--~ end
	--~ r.x = -r.x
	--~ r.y = -r.y
	--~ r.z = -r.z
	--~ return r
--~ end
--~ function matrix3.to_pitch_yaw_roll(m)
	--~ local r = vector.new()
	--~ if round(math.abs(m[6])) ~= 1 then
		--~ r.x = math.asin(m[6])
		--~ local cosx = math.cos(r.x)
		--~ r.y = math.atan2(-m[3] / cosx, m[9] / cosx)
		--~ r.z = math.atan2(-m[4] / cosx, m[5] / cosx)
	--~ elseif m[6] > 0 then -- gimbal lock
		--~ r.x = math.pi / 2
		--~ r.z = 0
		--~ r.y = math.atan2(m[2], m[1])
	--~ else
		--~ r.x = -math.pi / 2
		--~ r.z = 0
		--~ r.y = math.atan2(m[7], m[8])
	--~ end
	--~ return r
--~ end
function matrix3.to_pitch_yaw_roll(m)
	local r = vector.new()
	r.y = math.atan2(-m[3], m[9])
	local c2 = math.sqrt(m[4]^2 + m[5]^2)
	r.x = math.atan2(m[6], c2)
	local s1 = sin(r.y)
	local c1 = cos(r.y)
	r.z = math.atan2(s1 * m[8] + c1 * m[2], s1 * m[7] + c1 * m[1])
	return r
end

function matrix3.from_pitch_yaw_roll(v)
	return matrix3.multiply(matrix3.multiply(matrix3.rotation_around_y(-v.y),
			matrix3.rotation_around_x(-v.x)), matrix3.rotation_around_z(-v.z))
end
