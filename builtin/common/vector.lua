
vector = {}
vector.__index = vector

-- Create a new vector
--
-- vector.new(pos)
-- vector.new(x, y, z)
-- vector:new(pos)
-- vector:new(x, y, z)
function vector:new(a, b, c)
	local obj = { x = 0, y = 0, z = 0 }
	if self == vector then -- called using : or vector.new(vector, ...)
		if type(a) == "table" then
			assert(a.x and a.y and a.z, "Invalid vector passed to vector:new()")
			obj = { x = a.x, y = a.y, z = a.z }
		elseif a then
			assert(b and c, "Invalid arguments passed to vector:new()")
			obj = { x = a, y = b, z = c }
		end
	elseif type(self) == "table" then -- vector.new(vec)
		assert(self.x and self.y and self.z, "Invalid vector passed to vector.new()")
		assert(a == nil and b == nil and c == nil, "Unexpected extra params passed to vector.new()")
		obj = { x = self.x, y = self.y, z = self.z }
	elseif self then
		assert(a and b, "Invalid arguments for vector.new()")
		assert(c == nil, "Unexpected extra params passed to vector.new()")
		obj = { x = self, y = a, z = b }
	end

	setmetatable(obj, vector)
	return obj
end

function vector:equals(other)
	return self.x == other.x and
	       self.y == other.y and
	       self.z == other.z
end

function vector:length()
	return math.hypot(self.x, math.hypot(self.y, self.z))
end

function vector:normalize()
	local len = vector.length(self)
	if len == 0 then
		return vector:new()
	else
		return vector.divide(self, len)
	end
end

function vector:floor()
	return vector:new(math.floor(self.x), math.floor(self.y), math.floor(self.z))
end

function vector:round()
	return vector:new(math.floor(self.x + 0.5), math.floor(self.y + 0.5), math.floor(self.z + 0.5))
end

function vector:apply(func)
	return vector:new(func(self.x), func(self.y), func(self.z))
end

function vector:distance(other)
	local x = self.x - other.x
	local y = self.y - other.y
	local z = self.z - other.z
	return math.hypot(x, math.hypot(y, z))
end

function vector:direction(other)
	return vector.normalize({
		x = self.x - other.x,
		y = self.y - other.y,
		z = self.z - other.z
	})
end

function vector:add(other)
	if type(other) == "table" then
		return vector:new({
			x = self.x + other.x,
			y = self.y + other.y,
			z = self.z + other.z
		})
	else
		return vector:new({
			x = self.x + other,
			y = self.y + other,
			z = self.z + other
		})
	end
end

function vector:subtract(other)
	if type(other) == "table" then
		return vector:new({
			x = self.x - other.x,
			y = self.y - other.y,
			z = self.z - other.z
		})
	else
		return vector:new({
			x = self.x - other,
			y = self.y - other,
			z = self.z - other
		})
	end
end

function vector:multiply(other)
	if type(other) == "table" then
		return vector:new({
			x = self.x * other.x,
			y = self.y * other.y,
			z = self.z * other.z
		})
	else
		return vector:new({
			x = self.x * other,
			y = self.y * other,
			z = self.z * other
		})
	end
end

function vector:divide(other)
	if type(other) == "table" then
		return vector:new({
			x = self.x / other.x,
			y = self.y / other.y,
			z = self.z / other.z
		})
	else
		return vector:new({
			x = self.x / other,
			y = self.y / other,
			z = self.z / other
		})
	end
end

function vector.sort(a, b)
	return vector:new({x = math.min(a.x, b.x), y = math.min(a.y, b.y), z = math.min(a.z, b.z)}),
		vector:new({x = math.max(a.x, b.x), y = math.max(a.y, b.y), z = math.max(a.z, b.z)})
end



-- tests

local function assertE(a, b, msg)
	assert(vector.equals(a, b), msg)
end
local function assertNE(a, b, msg)
	assert(not vector.equals(a, b), msg)
end

assertE({x = 0, y = 0, z = 0}, {x = 0, y = 0, z = 0})
assertE({x = -1, y = 0, z = 1}, {x = -1, y = 0, z = 1})
local a = { x = 2, y = 4, z = -10 }
assertE(a, a)
assertNE({x = -1, y = 0, z = 1}, a)

assertE(vector:new(), { x = 0, y = 0, z = 0 })
assertE(vector:new(1, 2, 3), { x = 1, y = 2, z = 3 })
assertE(vector:new({ x = 1, y = 2, z = 3}), { x = 1, y = 2, z = 3 })
assert(vector:new().add)
assertE(vector.new(), { x = 0, y = 0, z = 0 })
assertE(vector.new(1, 2, 3), { x = 1, y = 2, z = 3 })
assertE(vector.new({ x = 1, y = 2, z = 3}), { x = 1, y = 2, z = 3 })
assertE(vector:new(vector:new()), { x = 0, y = 0, z = 0 })
