local function test_basic()
	local Vector = class.new("Vector")
	-- Constructor
	function Vector:__init(x, y, z)
		self.x = x
		self.y = y
		self.z = z
	end

	-- Copy constructor
	function Vector:__copy(other)
		self.x = other.x
		self.y = other.y
		self.z = other.z
		self.copied = true
	end

	-- Operator overloading
	function Vector:__add(other)
		return Vector(
			self.x + other.x,
			self.y + other.y,
			self.z + other.z
		)
	end

	-- Mutating in-place function
	function Vector:floor()
		self.x = math.floor(self.x)
		self.y = math.floor(self.y)
		self.z = math.floor(self.z)
	end

	-- Returning function
	function Vector:__tostring()
		local copied = ""
		if self.copied then
			copied = "c"
		end
		return copied .. "(" .. self.x .. ", " .. self.y .. ", " .. self.z .. ")"
	end

	-- Default value
	Vector.z = 0

	local v1 = Vector(1, 2, 3)
	local v2 = Vector(5.5, 6.8, 7.1)

	assert(tostring(v1 + v2) == "(6.5, 8.8, 10.1)")

	v2:floor()
	assert(v2.x == 5 and v2.y == 6 and v2.z == 7)

	assert(tostring(Vector(v1)) == "c(1, 2, 3)")

	assert(Vector(1, 2).z == 0)
end

local function test_inheritance()
	-- Base class
	local A = class.new("A")
	function A:a()
		return "A:a"
	end

	function A:ab()
		return "A:ab"
	end

	function A:ax()
		return "A:ax"
	end

	function A:abx()
		return "A:abx"
	end

	-- Since inheritance
	local AX = class.new("AX", A)
	function AX:x()
		return "AX:x"
	end

	function AX:ax()
		return "AX:ax"
	end

	-- Base class
	local B = class.new("B")
	function B:b()
		return "B:b"
	end

	function B:ab()
		return "B:ab"
	end

	function B:abx()
		return "B:abx"
	end

	-- Multiple inheritance
	local ABX = class.new("ABX", B, A)
	function ABX:x()
		return "ABX:x"
	end

	function ABX:abx()
		return "ABX:abx"
	end

	-- Test them
	local a = A()
	assert(a:a() == "A:a")
	assert(a:ab() == "A:ab")
	assert(a:ax() == "A:ax")
	assert(a:abx() == "A:abx")
	assert(class.name(a) == "A" and class.name(A) == "A")
	assert(class.prototype(a) == A and class.prototype(A) == A)
	assert(class.is(a, A) and class.is(A, A))

	class.cast(a, AX)
	assert(a:ax() == "AX:ax")

	local ax = AX()
	assert(ax:a() == "A:a")
	assert(ax:x() == "AX:x")
	assert(ax:ax() == "AX:ax") -- Overwritten
	assert(class.name(ax) == "AX" and class.name(AX) == "AX")
	assert(class.prototype(ax) == AX and class.prototype(AX) == AX)
	assert(class.is(ax, AX) and class.is(AX, AX) and
			class.is(ax, A) and class.is(AX, A))

	class.cast(ax, A)
	assert(class.prototype(ax) == A)
	assert(ax:ax() == "A:ax")

	local b = B()
	assert(b:b() == "B:b")
	assert(b:ab() == "B:ab")
	assert(b:abx() == "B:abx")
	assert(class.name(b) == "B" and class.name(B) == "B")
	assert(class.prototype(b) == B and class.prototype(B) == B)
	assert(class.is(b, B) and class.is(B, B))

	local abx = ABX()
	assert(abx:a() == "A:a")
	assert(abx:b() == "B:b")
	assert(abx:ab() == "B:ab") -- Overwritten
	assert(abx:x() == "ABX:x")
	assert(abx:abx() == "ABX:abx") -- Overwritten
	assert(class.name(abx) == "ABX" and class.name(ABX) == "ABX")
	assert(class.prototype(abx) == ABX and class.prototype(ABX) == ABX)
	assert(class.is(abx, ABX) and class.is(ABX, ABX) and
			class.is(abx, A) and class.is(ABX, A) and
			class.is(abx, B) and class.is(ABX, B))

	class.cast(abx, A)
	assert(class.prototype(abx) == A)
	assert(abx:ab() == "A:ab")
	assert(abx:abx() == "A:abx")

	class.cast(abx, B)
	assert(class.prototype(abx) == B)
	assert(abx:ab() == "B:ab")
	assert(abx:abx() == "B:abx")

	assert(class.prototype({}) == nil)
end

local function test_copying()
	-- Copies with the default deep copy
	local DeepCopy = class.new("DeepCopy")
	function DeepCopy:__init()
		self.param = {1, {2, {3}}}
	end

	-- Copies with a custom copy constructor which does a shallow copy only of itself
	local ShallowCopy = class.new("ShallowCopy")
	function ShallowCopy:__init()
		self.param = {1, {2, {3}}}
	end

	function ShallowCopy:__copy(other)
		self.param = other.param
	end

	-- Does not copy at all
	local Ref = class.new("Ref")
	function Ref:__init()
		self.param = {1, {2, {3}}}
	end
	Ref.__copy = "ref"

	-- Has all sorts of copiable and non-copiable types, copied via the default deep copy
	local Copy = class.new("Copy")
	function Copy:__init(some_value)
		-- Note: For all these tables, values on the same level must be unique.
		self.some_value = some_value

		local reref = {"a", "b", "c"}

		-- A large testing table
		self.some_table = {
			-- Test normal values
			boolean = true,
			number = 7,
			string = "x",

			-- Test nested tables
			table = {
				nest = {
					value = 6,
				},
				array = {1, 2, 5}
			},

			-- Repeated references
			once = reref,
			twice = reref,
			thrice = reref,

			-- Test classes
			deep = DeepCopy(),
			shallow = ShallowCopy(),
			ref = Ref(),

			-- Test non-copiables
			userdata = PseudoRandom(123),
			func = function() end
		}

		-- Another table with cross references
		self.other_table = {
			ref = self.some_table,
			whatever = {
				true,
				3.14159,
				"pi",
			},
		}

		-- Add cross references to the original table to make a circular reference
		self.some_table.ref = self.other_table
	end

	local a1 = Copy()
	local a2 = Copy(a1)

	local function equal(val1, val2, seen)
		if type(val1) == "table" then
			if seen[val1] then
				return true
			end
			seen[val1] = true

			local mt1 = getmetatable(val1)
			assert(mt1 == getmetatable(val2))

			if mt1 then
				if mt1.__copy == "ref" then -- Plain reference
					assert(val1 == val2)
				elseif type(mt1.__copy) == "function" then -- Shallow copy
					for k, v in pairs(val1) do
						assert(v == val2[k])
					end

					return true
				end
			else
				assert(val1 ~= val2)
			end

			for k, v in pairs(val1) do
				assert(equal(v, val2[k], seen))
			end

			return true
		else
			return val1 == val2
		end
	end

	assert(equal(a1, a2, {}))
end

function unittests.test_classes()
	test_basic()
	test_inheritance()
	test_copying()
end
