-- Server-side code of the test lua object

counter = 0
counter2 = 0
death_counter = 0
position = {X=math.random(-2,2),Y=6,Z=math.random(-2,2)}
rotation = {X=0, Y=math.random(0,360), Z=0}
dir = 1

function step(self, dtime)
	--[[if position.Y > 9.5 then
		position.Y = 6
	end
	if position.Y < 5.5 then
		position.Y = 9
		
	counter2 = counter2 - dtime
	if counter2 < 0 then
		counter2 = counter2 + 3
		dir = -dir
	end]]

	-- Limit step to a sane value; it jumps a lot while the map generator
	-- is in action
	if dtime > 0.5 then
		dtime = 0.5
	end

	-- Returned value has these fields:
	-- * bool walkable
	-- * int content
	n = object_get_node(self, position.X,position.Y-0.35,position.Z)
	if n.walkable then
		dir = 1
	else
		dir = -1
	end
	-- Keep the object approximately at ground level
	position.Y = position.Y + dtime * 1.0 * dir

	-- Move the object around
	position.X = position.X + math.cos(math.pi+rotation.Y/180*math.pi)
			* dtime * 2.0
	position.Z = position.Z + math.sin(math.pi+rotation.Y/180*math.pi)
			* dtime * 2.0

	-- This value has to be set; it determines to which player the
	-- object is near to and such
	object_set_base_position(self, position.X,position.Y,position.Z)

	rotation.Y = rotation.Y + dtime * 180
	
	-- Send some custom messages at a custom interval
	
	counter = counter - dtime
	if counter < 0 then
		counter = counter + 0.175

		message = "pos " .. position.X .. " " .. position.Y .. " " .. position.Z
		object_add_message(self, message)

		message = "rot " .. rotation.X .. " " .. rotation.Y .. " " .. rotation.Z
		object_add_message(self, message)
	end

	-- Remove the object after some time
	death_counter = death_counter + dtime
	if death_counter > 30 then
		object_remove(self)
	end

end

-- This stuff is passed to a newly created client-side counterpart of this object
function get_client_init_data(self)
	return "result of get_client_init_data"
end

-- This should return some data that mostly saves the state of this object
-- Not implemented yet
function get_server_init_data(self)
	return "result of get_server_init_data"
end

-- At reload time, the output of get_server_init_data is passed to this
-- Not implemented yet
function initialize(self, data)
	print("server object got initialization: " .. data)
end

