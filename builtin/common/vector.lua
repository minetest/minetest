
vector = {}

function vector.new(a, b, c)
	if type(a) == "table" then
		assert(a.x and a.y and a.z, "Invalid vector passed to vector.new()")
		return {x = a.x, y = a.y, z = a.z}
	elseif a and b and c then
		return {x = a, y = b, z = c}
	elseif a then
		return {x = a, y = a, z = a}
	end
	return {x = 0, y = 0, z = 0}
end

function vector.equals(a, b)
	return a.x == b.x and
	       a.y == b.y and
	       a.z == b.z
end

function vector.is_zero(a)
	return a.x == 0 and
	       a.y == 0 and
	       a.z == 0
end

function vector.length(v)
	return math.hypot(v.x, math.hypot(v.y, v.z))
end

function vector.normalize(v)
	if vector.is_nil(v) then return v end
	local inv_len = 1 / vector.length(v)
	return {
		x = v.x * inv_len,
		y = v.y * inv_len,
		z = v.z * inv_len,
	}
end

function vector.ceil(v)
	return {
		x = math.ceil(v.x),
		y = math.ceil(v.y),
		z = math.ceil(v.z),
	}
end

function vector.floor(v)
	return {
		x = math.floor(v.x),
		y = math.floor(v.y),
		z = math.floor(v.z),
	}
end

function vector.round(v)
	return {
		x = math.floor(v.x + 0.5),
		y = math.floor(v.y + 0.5),
		z = math.floor(v.z + 0.5),
	}
end

function vector.apply(v, func)
	return {
		x = func(v.x),
		y = func(v.y),
		z = func(v.z),
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
		z = pos2.z - pos1.z,
	})
end


function vector.add(a, b)
	if type(b) == "table" then
		return {
			x = a.x + b.x,
			y = a.y + b.y,
			z = a.z + b.z,
		}
	else
		return {
			x = a.x + b,
			y = a.y + b,
			z = a.z + b,
		}
	end
end

function vector.subtract(a, b)
	if type(b) == "table" then
		return {
			x = a.x - b.x,
			y = a.y - b.y,
			z = a.z - b.z,
		}
	else
		return {
			x = a.x - b,
			y = a.y - b,
			z = a.z - b,
		}
	end
end

function vector.multiply(a, b)
	if type(b) == "table" then
		return {
			x = a.x * b.x,
			y = a.y * b.y,
			z = a.z * b.z,
		}
	else
		return {
			x = a.x * b,
			y = a.y * b,
			z = a.z * b,
		}
	end
end

function vector.divide(a, b)
	if type(b) == "table" then
		return {
			x = a.x / b.x,
			y = a.y / b.y,
			z = a.z / b.z,
		}
	else
		return {
			x = a.x / b,
			y = a.y / b,
			z = a.z / b,
		}
	end
end

function vector.sum(vs)
	local V = {x = 0, y = 0, z = 0}
	for _, v in ipairs(vs) do
		V.x = V.x + v.x
		V.y = V.y + v.y
		V.z = V.z + v.z
	end
	return V
end

function vector.sort(a, b)
	return {
		x = math.min(a.x, b.x),
		y = math.min(a.y, b.y),
		z = math.min(a.z, b.z),
	}, {
		x = math.max(a.x, b.x),
		y = math.max(a.y, b.y),
		z = math.max(a.z, b.z),
	}
end
