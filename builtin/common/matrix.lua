
matrix = {}

function matrix.new(a, ...)
	if not a then
		return {0, 0, 0, 0, 0, 0, 0, 0, 0}
	elseif type(a) ~= "table" then
		return {a, ...}
	else
		return {a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]}
	end
end

matrix.identity = {1, 0, 0, 0, 1, 0, 0, 0, 1}

function matrix.equals(m1, m2)
	for i = 1, 9 do
		if m1[i] ~= m2[i] then
			return false
		end
	end
	return true
end

function matrix.index(m, l, c)
	return m[(l - 1) * 3 + c]
end

function matrix.add(m1, m2)
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

local function vector_matrix_multiply(m1, v)
	local m2 = matrix.new()
	m2[1] = v.x
	m2[2] = v.y
	m2[3] = v.z
	local m3 = matrix.multiply(m1, m2)
	return vector.new(m3[1], m3[4], m3[7])
end

function matrix.multiply(m1, m2)
	if type(m2) ~= "table" then
		return multiply_scalar(m1, m2)
	elseif m2.x then
		return vector_matrix_multiply(m1, m2)
	end
	local m3 = {}
	for l = 1, 3 do
		for c = 1, 3 do
			local i = (l - 1) * 3 + c
			m3[i] = 0
			for k = 1, 3 do
				m3[i] = m3[i] + m1[(l - 1) * 3 + k] + m2[(k - 1) * 3 + c]
			end
		end
	end
	return m3
end

function matrix.tensor_multiply(a, b)
	local m1 = matrix.new()
	m1[1] = a.x
	m1[4] = a.y
	m1[7] = a.z
	local m2 = matrix.new()
	m2[1] = a.x
	m2[2] = a.y
	m2[3] = a.z
	return matrix.multiply(m1, m2)
end

function matrix.transpose(m)
	return {m[1], m[4], m[7],
			m[2], m[5], m[8],
			m[3], m[6], m[9]}
end

-- note that minetest has a left-handed coordinate system.
-- ergo the rotation will be clockwise
-- (see wikipedia for details)

function matrix.rotation_around_x(angle)
	local s = math.sin(angle)
	local c = math.cos(angle)
	return {1, 0,  0,
            0, c, -s,
	        0, s,  c}
end

function matrix.rotation_around_y(angle)
	local s = math.sin(angle)
	local c = math.cos(angle)
	return { c, 0, s,
             0, 1, 0,
	        -s, 0, c}
end

function matrix.rotation_around_z(angle)
	local s = math.sin(angle)
	local c = math.cos(angle)
	return {c, -s, 0,
            s,  c, 0,
	        0,  0, 1}
end

function matrix.rotation_around_vector(v, angle)
	--todo
	return matrix.new(matrix.identity)
end

function matrix.to_yaw_pitch_roll(m)
	--todo
	return vector.new()
end

function matrix.from_yaw_pitch_roll(v)
	--todo
	return matrix.new(matrix.identity)
end
