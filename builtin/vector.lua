
vector = {}

function vector.new(a, b, c)
	assert(a)
	if type(a) == "table" then
		return {x=a.x, y=a.y, z=a.z}
	else
		assert(b and c)
		return {x=a, y=b, z=c}
	end
	return {x=0, y=0, z=0}
end

function vector.equals(a, b)
	assert(a and b)
	return a.x == b.x and
	       a.y == b.y and
	       a.z == b.z
end

function vector.length(v)
	assert(v)
	return math.hypot(v.x, math.hypot(v.y, v.z))
end

function vector.normalize(v)
	assert(v)
	local len = vector.length(v)
	if len == 0 then
		return vector.new()
	else
		return vector.divide(v, len)
	end
end

function vector.round(v)
	assert(v)
	return {
		x = math.floor(v.x + 0.5),
		y = math.floor(v.y + 0.5),
		z = math.floor(v.z + 0.5)
	}
end

function vector.distance(a, b)
	assert(a and b)
	local x = a.x - b.x
	local y = a.y - b.y
	local z = a.z - b.z
	return math.hypot(x, math.hypot(y, z))
end

function vector.direction(pos1, pos2)
	assert(pos1 and pos2)
	local x_raw = pos2.x - pos1.x
	local y_raw = pos2.y - pos1.y
	local z_raw = pos2.z - pos1.z
	local x_abs = math.abs(x_raw)
	local y_abs = math.abs(y_raw)
	local z_abs = math.abs(z_raw)
	if x_abs >= y_abs and
	   x_abs >= z_abs then
		y_raw = y_raw * (1 / x_abs)
		z_raw = z_raw * (1 / x_abs)
		x_raw = x_raw / x_abs
	end
	if y_abs >= x_abs and
	   y_abs >= z_abs then
		x_raw = x_raw * (1 / y_abs)
		z_raw = z_raw * (1 / y_abs)
		y_raw = y_raw / y_abs
	end
	if z_abs >= y_abs and
	   z_abs >= x_abs then
		x_raw = x_raw * (1 / z_abs)
		y_raw = y_raw * (1 / z_abs)
		z_raw = z_raw / z_abs
	end
	return {x=x_raw, y=y_raw, z=z_raw}
end


function vector.add(a, b)
	assert(a and b)
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
	assert(a and b)
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
	assert(a and b)
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
	assert(a and b)
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

