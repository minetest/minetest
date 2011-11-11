--[[function basicSerialize(o)
	if type(o) == "number" then
		return tostring(o)
	else   -- assume it is a string
		return string.format("%q", o)
	end
end

function dump2(name, value, saved)
	saved = saved or {}       -- initial value
	io.write(name, " = ")
	if type(value) == "number" or type(value) == "string" then
		io.write(basicSerialize(value), "\n")
	elseif type(value) == "table" then
		if saved[value] then    -- value already saved?
			io.write(saved[value], "\n")  -- use its previous name
		else
			saved[value] = name   -- save name for next time
			io.write("{}\n")     -- create a new table
			for k,v in pairs(value) do      -- save its fields
				local fieldname = string.format("%s[%s]", name,
												basicSerialize(k))
				save(fieldname, v, saved)
			end
		end
	else
		error("cannot save a " .. type(value))
	end
end]]

--[[function dump(o, name, dumped, s)
	name = name or "_"
	dumped = dumped or {}
	s = s or ""
	s = s .. name .. " = "
	if type(o) == "number" then
		s = s .. tostring(o)
	elseif type(o) == "string" then
		s = s .. string.format("%q", o)
	elseif type(o) == "boolean" then
		s = s .. tostring(o)
	elseif type(o) == "function" then
		s = s .. "<function>"
	elseif type(o) == "nil" then
		s = s .. "nil"
	elseif type(o) == "table" then
		if dumped[o] then
			s = s .. dumped[o]
		else
			dumped[o] = name
			local t = {}
			for k,v in pairs(o) do
				t[#t+1] = dump(v, k, dumped)
			end
			s = s .. "{" .. table.concat(t, ", ") .. "}"
		end
	else
		error("cannot dump a " .. type(o))
		return nil
	end
	return s
end]]

function basic_serialize(o)
	if type(o) == "number" then
		return tostring(o)
	elseif type(o) == "string" then
		return string.format("%q", o)
	elseif type(o) == "boolean" then
		return tostring(o)
	elseif type(o) == "function" then
		return "<function>"
	elseif type(o) == "nil" then
		return "nil"
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

function serialize(o, name, dumped)
	name = name or "_"
	dumped = dumped or {}
	io.write(name, " = ")
	if type(o) == "number" or type(o) == "string" or type(o) == "boolean"
			or type(o) == "function" or type(o) == "nil" then
		io.write(basic_serialize(o), "\n")
	elseif type(o) == "table" then
		if dumped[o] then
			io.write(dumped[o], "\n")
		else
			dumped[o] = name
			io.write("{}\n") -- new table
			for k,v in pairs(o) do
				local fieldname = string.format("%s[%s]", name, basic_serialize(k))
				serialize(v, fieldname, dumped)
			end
		end
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

function dump(o, dumped)
	dumped = dumped or {}
	if type(o) == "number" then
		return tostring(o)
	elseif type(o) == "string" then
		return string.format("%q", o)
	elseif type(o) == "table" then
		if dumped[o] then
			return "<circular reference>"
		end
		dumped[o] = true
		local t = {}
		for k,v in pairs(o) do
			t[#t+1] = "" .. k .. " = " .. dump(v, dumped)
		end
		return "{" .. table.concat(t, ", ") .. "}"
	elseif type(o) == "boolean" then
		return tostring(o)
	elseif type(o) == "function" then
		return "<function>"
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

print("omg lol")
print("minetest dump: "..dump(minetest))

minetest.register_entity("a", "dummy string");

--local TNT = minetest.new_entity {
local TNT = {
	-- Maybe handle gravity and collision this way? dunno
	physical = true,
	weight = 5,
	boundingbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "box",
	textures = {"tnt_top.png","tnt_bottom.png","tnt_side.png","tnt_side.png","tnt_side.png","tnt_side.png"},
	-- Initial value for our timer
	timer = 0,
	-- List names of state variables, for serializing object state
	state_variables = {"timer"},
}

-- Called after object is created
function TNT:on_create(env)
end

-- Called periodically
function TNT:on_step(env, dtime)
	self.timer = self.timer + dtime
	if self.timer > 4.0 then
		self.to_be_deleted = true -- Environment will delete this object at a suitable point of execution
		env:explode(self.pos, 3) -- Uh... well, something like that
	end
end

-- Called when object is punched
function TNT:on_punch(env, hitter)
	-- If tool is bomb defuser, revert back to being a block
	local item = hitter.inventory.get_current()
	if item.itemtype == "tool" and item.param == "bomb_defuser" then
		env:add_node(self.pos, 3072)
		self.to_be_deleted = true
	end
end

-- Called when object is right-clicked
function TNT:on_rightclick(self, env, hitter)
end

print("TNT dump: "..dump(TNT))

print("Registering TNT");
minetest.register_entity("TNT", TNT)

--print("minetest.registered_entities: "..dump(minetest.registered_entities))
print("minetest.registered_entities:")
serialize(minetest.registered_entities)

