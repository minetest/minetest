
Vector = {}
Vector.__index = Vector
vector = Vector

-- Create a new Vector
--
-- Vector.new(pos)
-- Vector.new(x, y, z)
-- Vector:new(pos)
-- Vector:new(x, y, z)
function Vector:new(a, b, c)
	local obj = { x = 0, y = 0, z = 0 }
	if self == Vector then -- called using : or Vector.new(Vector, ...)
		if type(a) == "table" then
			assert(a.x and a.y and a.z, "Invalid Vector passed to Vector:new()")
			obj = { x = a.x, y = a.y, z = a.z }
		elseif a then
			assert(b and c, "Invalid arguments passed to Vector:new()")
			obj = { x = a, y = b, z = c }
		end
	elseif type(self) == "table" then -- Vector.new(vec)
		assert(self.x and self.y and self.z, "Invalid Vector passed to Vector.new()")
		assert(a == nil and b == nil and c == nil, "Unexpected extra params passed to Vector.new()")
		obj = { x = self.x, y = self.y, z = self.z }
	elseif self then
		assert(a and b, "Invalid arguments for Vector.new()")
		assert(c == nil, "Unexpected extra params passed to Vector.new()")
		obj = { x = self, y = a, z = b }
	end

	setmetatable(obj, Vector)
	return obj
end

function Vector:equals(other)
	return self.x == other.x and
	       self.y == other.y and
	       self.z == other.z
end

function Vector:length()
	return math.hypot(self.x, math.hypot(self.y, self.z))
end

function Vector:normalize()
	local len = Vector.length(self)
	if len == 0 then
		return Vector:new()
	else
		return Vector.divide(self, len)
	end
end

function Vector:floor()
	return Vector:new(math.floor(self.x), math.floor(self.y), math.floor(self.z))
end

function Vector:round()
	return Vector:new(math.floor(self.x + 0.5), math.floor(self.y + 0.5), math.floor(self.z + 0.5))
end

function Vector:apply(func)
	return Vector:new(func(self.x), func(self.y), func(self.z))
end

function Vector:distance(other)
	local x = self.x - other.x
	local y = self.y - other.y
	local z = self.z - other.z
	return math.hypot(x, math.hypot(y, z))
end

function Vector:direction(other)
	return Vector.normalize({
		x = self.x - other.x,
		y = self.y - other.y,
		z = self.z - other.z
	})
end

function Vector:add(other)
	if type(other) == "table" then
		return Vector:new({
			x = self.x + other.x,
			y = self.y + other.y,
			z = self.z + other.z
		})
	else
		return Vector:new({
			x = self.x + other,
			y = self.y + other,
			z = self.z + other
		})
	end
end

function Vector:subtract(other)
	if type(other) == "table" then
		return Vector:new({
			x = self.x - other.x,
			y = self.y - other.y,
			z = self.z - other.z
		})
	else
		return Vector:new({
			x = self.x - other,
			y = self.y - other,
			z = self.z - other
		})
	end
end

function Vector:multiply(other)
	if type(other) == "table" then
		return Vector:new({
			x = self.x * other.x,
			y = self.y * other.y,
			z = self.z * other.z
		})
	else
		return Vector:new({
			x = self.x * other,
			y = self.y * other,
			z = self.z * other
		})
	end
end

function Vector:divide(other)
	if type(other) == "table" then
		return Vector:new({
			x = self.x / other.x,
			y = self.y / other.y,
			z = self.z / other.z
		})
	else
		return Vector:new({
			x = self.x / other,
			y = self.y / other,
			z = self.z / other
		})
	end
end

function Vector.sort(a, b)
	return Vector:new({x = math.min(a.x, b.x), y = math.min(a.y, b.y), z = math.min(a.z, b.z)}),
		Vector:new({x = math.max(a.x, b.x), y = math.max(a.y, b.y), z = math.max(a.z, b.z)})
end



-- tests

local function assertE(a, b, msg)
	assert(Vector.equals(a, b), msg)
end
local function assertNE(a, b, msg)
	assert(not Vector.equals(a, b), msg)
end

assertE({x = 0, y = 0, z = 0}, {x = 0, y = 0, z = 0})
assertE({x = -1, y = 0, z = 1}, {x = -1, y = 0, z = 1})
local a = { x = 2, y = 4, z = -10 }
assertE(a, a)
assertNE({x = -1, y = 0, z = 1}, a)

assertE(Vector:new(), { x = 0, y = 0, z = 0 })
assertE(Vector:new(1, 2, 3), { x = 1, y = 2, z = 3 })
assertE(Vector:new({ x = 1, y = 2, z = 3}), { x = 1, y = 2, z = 3 })
assert(Vector:new().add)
assertE(Vector.new(), { x = 0, y = 0, z = 0 })
assertE(Vector.new(1, 2, 3), { x = 1, y = 2, z = 3 })
assertE(Vector.new({ x = 1, y = 2, z = 3}), { x = 1, y = 2, z = 3 })
assertE(Vector:new(Vector:new()), { x = 0, y = 0, z = 0 })

local vec = Vector:new(1, 2, 3)
assertE(vec, { x = 1, y = 2, z = 3 })
assertE(vec:add({ x = 1, y = 2, z = 3 }), { x = 2, y = 4, z = 6 })
assertE(vec, { x = 1, y = 2, z = 3 })

