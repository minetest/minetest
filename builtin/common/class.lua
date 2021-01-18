class = {}

function class.new(name, ...)
	local cls = {}

	-- Copy inherited classes in reverse order to make the first ones overwrite later ones
	local bases = {...}
	for i = #bases, 1, -1 do
		for k, v in pairs(bases[i]) do
			cls[class.copy(k)] = class.copy(v)
		end
	end

	cls.__name = name
	cls.__index = cls

	-- Add instance information
	cls.__is = {[cls] = true}
	for _, base in ipairs(bases) do
		for base in pairs(base.__is) do
			cls.__is[base] = true
		end
	end

	-- Create pretty class constructor
	setmetatable(cls, {
		__call = function(cls, ...)
			local args = {...}
			if #args == 1 and class.is(args[1], cls) then
				return class.copy(args[1])
			else
				return class.init(cls, ...)
			end
		end
	})

	return cls
end

function class.instantiate(cls)
	return setmetatable({}, cls)
end

function class.init(cls, ...)
	local self = class.instantiate(cls)

	local init = self.__init
	if init then
		init(self, ...)
	end

	return self
end

local function class_deepcopy(obj, seen, bypass_ctor, into)
	if type(obj) == "table" then
		local already_seen = seen[obj]
		if already_seen then
			return already_seen
		end

		local mt = getmetatable(obj)
		local copier = mt and mt.__copy
		if copier == "ref" then
			return obj
		elseif not bypass_ctor and type(copier) == "function" then
			local copy = class.instantiate(mt)
			seen[obj] = copy
			copier(copy, obj)
			return copy
		end

		local copy = into or {}
		seen[obj] = copy

		for k, v in pairs(obj) do
			copy[class_deepcopy(k, seen)] = class_deepcopy(v, seen)
		end
		if mt and not into then
			setmetatable(copy, mt)
		end

		return copy
	end
	return obj
end

function class.copy(obj, bypass_ctor)
	return class_deepcopy(obj, {}, bypass_ctor)
end

-- Note: `into` will not be copied if it is inside itself
function class.copy_into(obj, into, bypass_ctro)
	class_deepcopy(obj, {}, bypass_ctor, into)
end

function class.name(cls_or_obj)
	local prototype = class.prototype(cls_or_obj)
	return prototype and prototype.__name
end

function class.prototype(cls_or_obj)
	local mt = getmetatable(cls_or_obj) or {}
	if mt.__is then
		return mt -- An object was passed
	elseif cls_or_obj.__is then
		return cls_or_obj -- A prototype was passed
	end
	return nil -- Invalid
end

function class.is(cls_or_obj, cls)
	local prototype = class.prototype(cls_or_obj)
	return prototype and prototype.__is and prototype.__is[cls]
end

function class.cast(obj, cls)
	setmetatable(obj, cls)
end
