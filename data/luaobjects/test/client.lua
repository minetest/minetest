-- Client-side code of the test lua object

--
-- Some helper functions
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

--
-- Actual code
--

function step(self, dtime)
	-- Some smoother animation could be done here
end

function process_message(self, data)
	--print("client got message: " .. data)

	-- Receive our custom messages

	sp = split(data, " ")
	if sp[1] == "pos" then
		object_set_position(self, sp[2], sp[3], sp[4])
	end
	if sp[1] == "rot" then
		object_set_rotation(self, sp[2], sp[3], sp[4])
	end
end

function initialize(self, data)
	print("client object got initialization: " .. data)

	corners = {
		{-1/2,-1/4, 0},
		{ 1/2,-1/4, 0},
		{ 1/2, 1/4, 0},
		{-1/2, 1/4, 0},
	}
	object_add_to_mesh(self, "rat.png", corners, false)

end

