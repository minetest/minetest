-- Server-side code of the test lua object

--
-- Some helper functions and classes
--

function vector_subtract(a, b)
	return {X=a.X-b.X, Y=a.Y-b.Y, Z=a.Z-b.Z}
end

function vector_add(a, b)
	return {X=a.X+b.X, Y=a.Y+b.Y, Z=a.Z+b.Z}
end

function vector_multiply(a, d)
	return {X=a.X*d, Y=a.Y*d, Z=a.Z*d}
end

--
-- Actual code
--

counter = 0
counter2 = 0
counter3 = 0
counter4 = 0
death_counter = 0
-- This is got in initialization from object_get_base_position(self)
position = {X=0,Y=0,Z=0}
rotation = {X=0, Y=math.random(0,360), Z=0}
dir = 1
temp1 = 0

function on_step(self, dtime)
	--[[if position.Y > 9.5 then
		position.Y = 6
	end
	if position.Y < 5.5 then
		position.Y = 9]]
		
	-- Limit step to a sane value; it jumps a lot while the map generator
	-- is in action
	if dtime > 0.5 then
		dtime = 0.5
	end

	-- Returned value has these fields:
	-- * int content
	-- * int param1
	-- * int param2
	p = {X=position.X, Y=position.Y-0.35, Z=position.Z}
	n = object_get_node(self, p)
	f = get_content_features(n.content)
	if f.walkable then
		dir = 1
	else
		dir = -1
	end
	-- Keep the object approximately at ground level
	position.Y = position.Y + dtime * 2.0 * dir

	-- Move the object around
	position.X = position.X + math.cos(math.pi+rotation.Y/180*math.pi)
			* dtime * 2.0
	position.Z = position.Z + math.sin(math.pi+rotation.Y/180*math.pi)
			* dtime * 2.0

	-- This value has to be set; it determines to which player the
	-- object is near to and such
	object_set_base_position(self, position)

	counter4 = counter4 - dtime
	if counter4 < 0 then
		counter4 = counter4 + math.random(0.5,8)
		-- Mess around with the map
		np = vector_add(position, {X=0,Y=0,Z=0})
		object_place_node(self, np, {content=0})
		-- A node could be digged out with this:
		-- object_dig_node(self, np)
	end

	counter3 = counter3 - dtime
	if counter3 < 0 then
		counter3 = counter3 + math.random(1,4)
		rotation.Y = rotation.Y + math.random(-180, 180)
	end
	
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

	-- Mess around with the map
	--[[counter2 = counter2 - dtime
	if counter2 < 0 then
		counter2 = counter2 + 3
		if temp1 == 0 then
			temp1 = 1
			object_dig_node(self, {X=0,Y=1,Z=0})
		else
			temp1 = 0
			n = {content=1}
			object_place_node(self, {X=0,Y=5,Z=0}, n)
		end
	end]]

	-- Remove the object after some time
	death_counter = death_counter + dtime
	if death_counter > 30 then
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
end

