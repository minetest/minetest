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

function basic_dump2(o)
	if type(o) == "number" then
		return tostring(o)
	elseif type(o) == "string" then
		return string.format("%q", o)
	elseif type(o) == "boolean" then
		return tostring(o)
	elseif type(o) == "function" then
		return "<function>"
	elseif type(o) == "userdata" then
		return "<userdata>"
	elseif type(o) == "nil" then
		return "nil"
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

function dump2(o, name, dumped)
	name = name or "_"
	dumped = dumped or {}
	io.write(name, " = ")
	if type(o) == "number" or type(o) == "string" or type(o) == "boolean"
			or type(o) == "function" or type(o) == "nil"
			or type(o) == "userdata" then
		io.write(basic_dump2(o), "\n")
	elseif type(o) == "table" then
		if dumped[o] then
			io.write(dumped[o], "\n")
		else
			dumped[o] = name
			io.write("{}\n") -- new table
			for k,v in pairs(o) do
				local fieldname = string.format("%s[%s]", name, basic_dump2(k))
				dump2(v, fieldname, dumped)
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
	elseif type(o) == "userdata" then
		return "<userdata>"
	elseif type(o) == "nil" then
		return "nil"
	else
		error("cannot dump a " .. type(o))
		return nil
	end
end

print("omg lol")
print("minetest dump: "..dump(minetest))

--local TNT = minetest.new_entity {
local TNT = {
	-- Maybe handle gravity and collision this way? dunno
	physical = true,
	weight = 5,
	collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "cube",
	textures = {"tnt_top.png","tnt_bottom.png","tnt_side.png","tnt_side.png","tnt_side.png","tnt_side.png"},
	--visual = "single_sprite",
	--textures = {"mese.png^[forcesingle"},
	-- Initial value for our timer
	timer = 0,
	-- List names of state variables, for serializing object state
	state_variables = {"timer"},
}

-- Called periodically
function TNT:on_step(dtime)
	--print("TNT:on_step()")
end

-- Called when object is punched
function TNT:on_punch(hitter)
	print("TNT:on_punch()")
end

-- Called when object is right-clicked
function TNT:on_rightclick(clicker)
	pos = self.object:getpos()
	pos = {x=pos.x, y=pos.y+0.1, z=pos.z}
	self.object:moveto(pos, false)
end
--[[
function TNT:on_rightclick(clicker)
	print("TNT:on_rightclick()")
	print("self: "..dump(self))
	print("getmetatable(self): "..dump(getmetatable(self)))
	print("getmetatable(getmetatable(self)): "..dump(getmetatable(getmetatable(self))))
	pos = self.object:getpos()
	print("TNT:on_rightclick(): object position: "..dump(pos))
	pos = {x=pos.x+0.5+1, y=pos.y+0.5, z=pos.z+0.5}
	--minetest.env:add_node(pos, 0)
end
--]]

print("TNT dump: "..dump(TNT))

print("Registering TNT");
minetest.register_entity("TNT", TNT)

--print("minetest.registered_entities: "..dump(minetest.registered_entities))
print("minetest.registered_entities:")
dump2(minetest.registered_entities)

