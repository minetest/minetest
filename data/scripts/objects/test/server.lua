-- Server-side code of the test lua object

--
-- Some helper functions and classes
--

-- For debugging
function dump(o)
    if type(o) == 'table' then
        local s = '{ '
        for k,v in pairs(o) do
                if type(k) ~= 'number' then k = '"'..k..'"' end
                s = s .. '['..k..'] = ' .. dump(v) .. ','
        end
        return s .. '} '
    else
        return tostring(o)
    end
end

function table.copy(t)
	local t2 = {}
	for k,v in pairs(t) do
		t2[k] = v
	end
	return t2
end

function vector_zero()
	return {X=0,Y=0,Z=0}
end

function vector_subtract(a, b)
	return {X=a.X-b.X, Y=a.Y-b.Y, Z=a.Z-b.Z}
end

function vector_add(a, b)
	return {X=a.X+b.X, Y=a.Y+b.Y, Z=a.Z+b.Z}
end

function vector_multiply(a, d)
	return {X=a.X*d, Y=a.Y*d, Z=a.Z*d}
end

function vector_copy(a)
	return {X=a.X, Y=a.Y, Z=a.Z}
end

function vector_length(a)
	return math.sqrt(a.X*a.X + a.Y*a.Y + a.Z*a.Z)
end

function vector_eq(a, b)
	return (a.X==b.X and a.Y==b.Y and a.Z==b.Z)
end

function round(num, idp)
	local mult = 10^(idp or 0)
	return math.floor(num * mult + 0.5) / mult
end

function vector_snap(a)
	return {X=round(a.X, 0), Y=round(a.Y, 0), Z=round(a.Z, 0)}
end

--
-- Actual code
--

CONTENT_STONE = 0

is_digger = false
counter = 0
counter2 = 0
counter3 = 0
counter4 = 0
counter_move = 0
death_counter = 0
-- This is set in on_initialize()
position = {X=0,Y=0,Z=0}
starting_position = {X=0,Y=0,Z=0}
rotation = {X=0, Y=math.random(0,360), Z=0}
y_dir = 1
temp1 = 0
speed = 1.5
main_dir = {X=0,Y=0,Z=0}

function dir_goodness(env, pos, dir)
	if vector_eq(dir, vector_zero()) then
		return -1
	end
	p = vector_add(pos, dir)
	n = env_get_node(env, p)
	f = get_content_features(n.content)
	if f.walkable then
		p.Y = p.Y + 1
		n = env_get_node(env, p)
		f = get_content_features(n.content)
		if f.walkable then
			-- Too high
			return -1
		end
		-- Hill
		return 2
	end
	p.Y = p.Y - 1
	n = env_get_node(env, p)
	f = get_content_features(n.content)
	if f.walkable then
		-- Flat
		return 1
	end
	-- Drop
	return 0
end

function on_step(self, dtime)
	-- Limit step to a sane value; it jumps a lot while the map generator
	-- is in action
	if dtime > 0.5 then
		dtime = 0.5
	end

	env = object_get_environment(self)
	
	--[[
	-- Returned value has these fields:
	-- * int content
	-- * int param1
	-- * int param2
	p = {X=position.X, Y=position.Y-0.35, Z=position.Z}
	n = env_get_node(env, p)
	f = get_content_features(n.content)
	if f.walkable then
		y_dir = 1
	else
		y_dir = -1
	end
	-- Keep the object approximately at ground level
	position.Y = position.Y + dtime * 2.0 * y_dir

	-- Move the object around
	position.X = position.X + math.cos(math.pi+rotation.Y/180*math.pi)
			* dtime * speed
	position.Z = position.Z + math.sin(math.pi+rotation.Y/180*math.pi)
			* dtime * speed
	
	-- Rotate the object if it is too far from the starting point
	counter3 = counter3 - dtime
	if counter3 < 0 then
		counter3 = counter3 + 1
		diff = vector_subtract(position, starting_position)
		d = vector_length(diff)
		--print("pos="..dump(position).." starting="..dump(starting_position))
		--print("diff=" .. dump(diff))
		--print("d=" .. d)
		if d > 3 then
			rotation.Y = rotation.Y + 90
			--rotation.Y = rotation.Y + math.random(-180, 180)
		end
	end

	-- This value has to be set; it determines to which player the
	-- object is near to and such
	object_set_base_position(self, position)

	counter4 = counter4 - dtime
	if counter4 < 0 then
		--counter4 = counter4 + math.random(0.5,8)
		counter4 = counter4 + 0.6/speed
		-- Mess around with the map
		if is_digger == true then
			np = vector_add(position, {X=0,Y=-0.6,Z=0})
			env_dig_node(env, np)
		else
			np = vector_add(position, {X=0,Y=0,Z=0})
			env_place_node(env, np, {content=0})
		end
	end
	--]]
	
	counter_move = counter_move - dtime
	if counter_move < 0 then
		counter_move = counter_move + 1/speed
		if counter_move < 0 then counter_move = 0 end

		old_position = vector_copy(position)

		dirs = {
			{X=1, Y=0, Z=0},
			{X=-1, Y=0, Z=0},
			{X=0, Y=0, Z=1},
			{X=0, Y=0, Z=-1}
		}
		
		best_dir = main_dir
		best_goodness = dir_goodness(env, position, main_dir)

		for k,v in ipairs(dirs) do
			-- Don't go directly backwards
			if not vector_eq(vector_subtract(vector_zero(), v), main_dir) then
				goodness = dir_goodness(env, position, v)
				if goodness > best_goodness then
					best_dir = v
					goodness = best_goodness
				end
			end
		end

		-- Place stone block when dir changed
		if not vector_eq(main_dir, best_dir) then
			np = vector_add(position, {X=0,Y=0,Z=0})
			env_place_node(env, np, {content=CONTENT_STONE})
		end

		main_dir = best_dir

		position = vector_add(position, main_dir)
		
		pos_diff = vector_subtract(position, old_position)
		rotation.Y = math.atan2(pos_diff.Z, pos_diff.X)/math.pi*180-180
		
		-- Returned value has these fields:
		-- * int content
		-- * int param1
		-- * int param2
		p = {X=position.X, Y=position.Y, Z=position.Z}
		n = env_get_node(env, p)
		f = get_content_features(n.content)
		if f.walkable then
			position.Y = position.Y + 1
		end
		p = {X=position.X, Y=position.Y-1, Z=position.Z}
		n = env_get_node(env, p)
		f = get_content_features(n.content)
		if not f.walkable then
			position.Y = position.Y - 1
		end
		
		-- Center in the middle of the node
		position = vector_snap(position)
	end

	-- This value has to be set; it determines to which player the
	-- object is near to and such
	object_set_base_position(self, position)
	
	--[[
	counter4 = counter4 - dtime
	if counter4 < 0 then
		--counter4 = counter4 + math.random(0.5,8)
		counter4 = counter4 + 0.6/speed
		-- Mess around with the map
		if is_digger == true then
			np = vector_add(position, {X=0,Y=-0.6,Z=0})
			env_dig_node(env, np)
		else
			np = vector_add(position, {X=0,Y=0,Z=0})
			env_place_node(env, np, {content=0})
		end
	end
	--]]
	
	---[[
	-- Send some custom messages at a custom interval
	
	counter = counter - dtime
	if counter < 0 then
		counter = counter + 0.25
		if counter < 0 then
			counter = 0
		end

		message = "pos " .. position.X .. " " .. position.Y .. " " .. position.Z
		object_add_message(self, message)

		message = "rot " .. rotation.X .. " " .. rotation.Y .. " " .. rotation.Z
		object_add_message(self, message)
	end
	--]]

	-- Remove the object after some time
	death_counter = death_counter + dtime
	if death_counter > 40 then
		object_remove(self)
	end

end

-- This stuff is passed to a newly created client-side counterpart of this object
function on_get_client_init_data(self)
	-- Just return some data for testing
	return "result of get_client_init_data"
end

-- This should return some data that mostly saves the state of this object
-- Not completely implemented yet
function on_get_server_init_data(self)
	-- Just return some data for testing
	return "result of get_server_init_data"
end

-- When the object is loaded from scratch, this is called before the first
-- on_step(). Data is an empty string or the output of an object spawner
-- hook, but such things are not yet implemented.
--
-- At reload time, the last output of get_server_init_data is passed as data.
--
-- This should initialize the position of the object to the value returned
--   by object_get_base_position(self)
--
-- Not completely implemented yet
--
function on_initialize(self, data)
	print("server object got initialization: " .. data)
	position = object_get_base_position(self)
	starting_position = vector_copy(position);
	if math.random() < 0.5 then
		is_digger = true
	end
end

