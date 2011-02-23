-- Client-side code of the test lua object

--
-- Some helper functions and classes
--

function split(str, pat)
   local t = {}  -- NOTE: use {n = 0} in Lua-5.0
   local fpat = "(.-)" .. pat
   local last_end = 1
   local s, e, cap = str:find(fpat, 1)
   while s do
      if s ~= 1 or cap ~= "" then
	 table.insert(t,cap)
      end
      last_end = e+1
      s, e, cap = str:find(fpat, last_end)
   end
   if last_end <= #str then
      cap = str:sub(last_end)
      table.insert(t, cap)
   end
   return t
end

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

function vector_subtract(a, b)
	return {X=a.X-b.X, Y=a.Y-b.Y, Z=a.Z-b.Z}
end

function vector_add(a, b)
	return {X=a.X+b.X, Y=a.Y+b.Y, Z=a.Z+b.Z}
end

function vector_multiply(a, d)
	return {X=a.X*d, Y=a.Y*d, Z=a.Z*d}
end

SmoothTranslator = {}
SmoothTranslator.__index = SmoothTranslator

function SmoothTranslator.create()
	local obj = {}
	setmetatable(obj, SmoothTranslator)
	obj.vect_old = {X=0, Y=0, Z=0}
	obj.anim_counter = 0
	obj.anim_time = 0
	obj.anim_time_counter = 0
	obj.vect_show = {X=0, Y=0, Z=0}
	obj.vect_aim = {X=0, Y=0, Z=0}
	return obj
end

function SmoothTranslator:update(vect_new)
	self.vect_old = self.vect_show
	self.vect_aim = vect_new
	if self.anim_time < 0.001 or self.anim_time > 1.0 then
		self.anim_time = self.anim_time_counter
	else
		self.anim_time = self.anim_time * 0.9 + self.anim_time_counter * 0.1
	end
	self.anim_time_counter = 0
	self.anim_counter = 0
end

function SmoothTranslator:translate(dtime)
	self.anim_time_counter = self.anim_time_counter + dtime
	self.anim_counter = self.anim_counter + dtime
	vect_move = vector_subtract(self.vect_aim, self.vect_old)
	moveratio = 1.0
	if self.anim_time > 0.001 then
		moveratio = self.anim_time_counter / self.anim_time
	end
	if moveratio > 1.5 then
		moveratio = 1.5
	end
	self.vect_show = vector_add(self.vect_old, vector_multiply(vect_move, moveratio))
end

--
-- Actual code
--

pos_trans = SmoothTranslator.create()
rot_trans = SmoothTranslator.create()

-- Callback functions

function on_step(self, dtime)
	pos_trans:translate(dtime)
	rot_trans:translate(dtime)
	object_set_position(self, pos_trans.vect_show)
	object_set_rotation(self, rot_trans.vect_show)
end

function on_process_message(self, data)
	--print("client got message: " .. data)

	-- Receive our custom messages

	sp = split(data, " ")
	if sp[1] == "pos" then
		pos_trans:update({X=sp[2], Y=sp[3], Z=sp[4]})
	end
	if sp[1] == "rot" then
		rot_trans:update({X=sp[2], Y=sp[3], Z=sp[4]})
	end
end

function on_initialize(self, data)
	print("client object got initialization: " .. data)

	corners = {
		{-1/2,-1/4, 0},
		{ 1/2,-1/4, 0},
		{ 1/2, 1/4, 0},
		{-1/2, 1/4, 0},
	}
	object_add_to_mesh(self, "rat.png", corners, false)

end

