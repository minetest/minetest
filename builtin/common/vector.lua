vector = {}

local floor, hypot = math.floor, math.hypot
local min, max = math.min, math.max

function vector.new(a, b, c)
	if type(a) == "table" then
		assert(a.x and a.y and a.z, "Invalid vector passed to vector.new()")
		return {x = a.x, y = a.y, z = a.z}
	elseif a then
		assert(b and c, "Invalid arguments for vector.new()")
		return {x = a, y = b, z = c}
	end
	return {x = 0, y = 0, z = 0}
end

function vector.equals(a, b)
	return a.x == b.x and
	       a.y == b.y and
	       a.z == b.z
end

function vector.length(v)
	return hypot(v.x, hypot(v.y, v.z))
end

function vector.normalize(v)
	local len = vector.length(v)
	if len == 0 then
		return {x = 0, y = 0, z = 0}
	else
		return vector.divide(v, len)
	end
end

function vector.floor(v)
	return {
		x = floor(v.x),
		y = floor(v.y),
		z = floor(v.z)
	}
end

function vector.round(v)
	return {
		x = floor(v.x + 0.5),
		y = floor(v.y + 0.5),
		z = floor(v.z + 0.5)
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
	return hypot(x, hypot(y, z))
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
		return {
			x = a.x + b.x,
			y = a.y + b.y,
			z = a.z + b.z
		}
	else
		return {
			x = a.x + b,
			y = a.y + b,
			z = a.z + b
		}
	end
end

function vector.subtract(a, b)
	if type(b) == "table" then
		return {
			x = a.x - b.x,
			y = a.y - b.y,
			z = a.z - b.z
		}
	else
		return {
			x = a.x - b,
			y = a.y - b,
			z = a.z - b
		}
	end
end

function vector.multiply(a, b)
	if type(b) == "table" then
		return {
			x = a.x * b.x,
			y = a.y * b.y,
			z = a.z * b.z
		}
	else
		return {
			x = a.x * b,
			y = a.y * b,
			z = a.z * b
		}
	end
end

function vector.divide(a, b)
	if type(b) == "table" then
		return {
			x = a.x / b.x,
			y = a.y / b.y,
			z = a.z / b.z
		}
	else
		return {
			x = a.x / b,
			y = a.y / b,
			z = a.z / b
		}
	end
end

function vector.sort(a, b)
	return {x = min(a.x, b.x), y = min(a.y, b.y), z = min(a.z, b.z)},
	       {x = max(a.x, b.x), y = max(a.y, b.y), z = max(a.z, b.z)}
end
