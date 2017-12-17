
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

-- returns a new normalized 2d vector with random values
function vector.random2d()
	local phi = math.random() * 2 * math.pi
	return {
		x = math.cos(phi),
		y = math.sin(phi),
	}
end

-- returns a new normalized 3d vector with random values
function vector.random3d()
	local phi = math.random() * 2 * math.pi
	local y = math.random() * 2 - 1
	local r = math.sqrt(1 - y * y)
	local x = r * math.cos(phi)
	local z = r * math.sin(phi)
	return {x = x, y = y, z = z}
end

-- returns a vector with a specific length
function vector.set_length(v, l)
	v = vector.normalize(v)
	return vector.multiply(v, l)
end

-- limits the length of a vector
function vector.limit(v, l)
	if vector.length(v) > l then
		return vector.set_length(v, l)
	end
	return v
end

-- returns the dot product of two vectors
function vector.dot(a, b)
	return a.x * b.x + a.y * b.y + a.z * b.z
end

-- returns the cross product of two vectors
function vector.cross(a, b)
	return {x = a.y * b.z - a.z * b.y,
		y = a.z * b.x - a.x * b.z,
		z = a.x * b.y - a.y * b.x}
end

-- returns the scalar triple product of three vectors
function vector.scalar_triple(a, b, c)
	return vector.dot(a, vector.cross(b, c))
end

-- returns the vector triple product of three vectors
function vector.vector_triple(a, b, c)
	return vector.cross(a, vector.cross(b, c))
end

-- returns the angle between two 2d vectors (x,z) in radians
-- y from a 3d vector is ignored
function vector.angle_between_2d(a, b)
	return math.atan2(a.x * b.z - a.z * b.x, a.x * b.x + a.z * b.z)
end


-- returns yaw from a 2d vector (x, z) in radians
-- y from a 3d vector is ignored
function vector.yaw(v)
	local yaw = math.atan2(-v.x, v.z)
	if yaw >= -math.pi and yaw < 0 then
		yaw = math.pi * 2 + yaw
	end
	return yaw
end
