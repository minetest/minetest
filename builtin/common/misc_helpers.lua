-- Minetest: builtin/misc_helpers.lua

--------------------------------------------------------------------------------
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

--------------------------------------------------------------------------------
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

--------------------------------------------------------------------------------
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
			t[#t+1] = "[" .. dump(k, dumped) .. "] = " .. dump(v, dumped)
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

--------------------------------------------------------------------------------
function string:split(sep)
	local sep, fields = sep or ",", {}
	local pattern = string.format("([^%s]+)", sep)
	self:gsub(pattern, function(c) fields[#fields+1] = c end)
	return fields
end

--------------------------------------------------------------------------------
function file_exists(filename)
	local f = io.open(filename, "r")
	if f==nil then
		return false
	else
		f:close()
		return true
	end
end

--------------------------------------------------------------------------------
function string:trim()
	return (self:gsub("^%s*(.-)%s*$", "%1"))
end

assert(string.trim("\n \t\tfoo bar\t ") == "foo bar")

--------------------------------------------------------------------------------
function math.hypot(x, y)
	local t
	x = math.abs(x)
	y = math.abs(y)
	t = math.min(x, y)
	x = math.max(x, y)
	if x == 0 then return 0 end
	t = t / x
	return x * math.sqrt(1 + t * t)
end

--------------------------------------------------------------------------------
function get_last_folder(text,count)
	local parts = text:split(DIR_DELIM)
	
	if count == nil then
		return parts[#parts]
	end
	
	local retval = ""
	for i=1,count,1 do
		retval = retval .. parts[#parts - (count-i)] .. DIR_DELIM
	end
	
	return retval
end

--------------------------------------------------------------------------------
function cleanup_path(temppath)
	
	local parts = temppath:split("-")
	temppath = ""	
	for i=1,#parts,1 do
		if temppath ~= "" then
			temppath = temppath .. "_"
		end
		temppath = temppath .. parts[i]
	end
	
	parts = temppath:split(".")
	temppath = ""	
	for i=1,#parts,1 do
		if temppath ~= "" then
			temppath = temppath .. "_"
		end
		temppath = temppath .. parts[i]
	end
	
	parts = temppath:split("'")
	temppath = ""	
	for i=1,#parts,1 do
		if temppath ~= "" then
			temppath = temppath .. ""
		end
		temppath = temppath .. parts[i]
	end
	
	parts = temppath:split(" ")
	temppath = ""	
	for i=1,#parts,1 do
		if temppath ~= "" then
			temppath = temppath
		end
		temppath = temppath .. parts[i]
	end
	
	return temppath
end

local tbl = engine or minetest
function tbl.formspec_escape(text)
	if text ~= nil then
		text = string.gsub(text,"\\","\\\\")
		text = string.gsub(text,"%]","\\]")
		text = string.gsub(text,"%[","\\[")
		text = string.gsub(text,";","\\;")
		text = string.gsub(text,",","\\,")
	end
	return text
end


function tbl.splittext(text,charlimit)
	local retval = {}

	local current_idx = 1
	
	local start,stop = string.find(text," ",current_idx)
	local nl_start,nl_stop = string.find(text,"\n",current_idx)
	local gotnewline = false
	if nl_start ~= nil and (start == nil or nl_start < start) then
		start = nl_start
		stop = nl_stop
		gotnewline = true
	end
	local last_line = ""
	while start ~= nil do
		if string.len(last_line) + (stop-start) > charlimit then
			table.insert(retval,last_line)
			last_line = ""
		end
		
		if last_line ~= "" then
			last_line = last_line .. " "
		end
		
		last_line = last_line .. string.sub(text,current_idx,stop -1)
		
		if gotnewline then
			table.insert(retval,last_line)
			last_line = ""
			gotnewline = false
		end
		current_idx = stop+1
		
		start,stop = string.find(text," ",current_idx)
		nl_start,nl_stop = string.find(text,"\n",current_idx)
	
		if nl_start ~= nil and (start == nil or nl_start < start) then
			start = nl_start
			stop = nl_stop
			gotnewline = true
		end
	end
	
	--add last part of text
	if string.len(last_line) + (string.len(text) - current_idx) > charlimit then
			table.insert(retval,last_line)
			table.insert(retval,string.sub(text,current_idx))
	else
		last_line = last_line .. " " .. string.sub(text,current_idx)
		table.insert(retval,last_line)
	end
	
	return retval
end

--------------------------------------------------------------------------------

if minetest then
	local dirs1 = {9, 18, 7, 12}
	local dirs2 = {20, 23, 22, 21}

	function minetest.rotate_and_place(itemstack, placer, pointed_thing,
				infinitestacks, orient_flags)
		orient_flags = orient_flags or {}

		local unode = minetest.get_node_or_nil(pointed_thing.under)
		if not unode then
			return
		end
		local undef = minetest.registered_nodes[unode.name]
		if undef and undef.on_rightclick then
			undef.on_rightclick(pointed_thing.under, unode, placer,
					itemstack, pointed_thing)
			return
		end
		local pitch = placer:get_look_pitch()
		local fdir = minetest.dir_to_facedir(placer:get_look_dir())
		local wield_name = itemstack:get_name()

		local above = pointed_thing.above
		local under = pointed_thing.under
		local iswall = (above.y == under.y)
		local isceiling = not iswall and (above.y < under.y)
		local anode = minetest.get_node_or_nil(above)
		if not anode then
			return
		end
		local pos = pointed_thing.above
		local node = anode

		if undef and undef.buildable_to then
			pos = pointed_thing.under
			node = unode
			iswall = false
		end

		if minetest.is_protected(pos, placer:get_player_name()) then
			minetest.record_protection_violation(pos,
					placer:get_player_name())
			return
		end

		local ndef = minetest.registered_nodes[node.name]
		if not ndef or not ndef.buildable_to then
			return
		end

		if orient_flags.force_floor then
			iswall = false
			isceiling = false
		elseif orient_flags.force_ceiling then
			iswall = false
			isceiling = true
		elseif orient_flags.force_wall then
			iswall = true
			isceiling = false
		elseif orient_flags.invert_wall then
			iswall = not iswall
		end

		if iswall then
			minetest.set_node(pos, {name = wield_name,
					param2 = dirs1[fdir+1]})
		elseif isceiling then
			if orient_flags.force_facedir then
				minetest.set_node(pos, {name = wield_name,
						param2 = 20})
			else
				minetest.set_node(pos, {name = wield_name,
						param2 = dirs2[fdir+1]})
			end
		else -- place right side up
			if orient_flags.force_facedir then
				minetest.set_node(pos, {name = wield_name,
						param2 = 0})
			else
				minetest.set_node(pos, {name = wield_name,
						param2 = fdir})
			end
		end

		if not infinitestacks then
			itemstack:take_item()
			return itemstack
		end
	end


--------------------------------------------------------------------------------
--Wrapper for rotate_and_place() to check for sneak and assume Creative mode
--implies infinite stacks when performing a 6d rotation.
--------------------------------------------------------------------------------


	minetest.rotate_node = function(itemstack, placer, pointed_thing)
		minetest.rotate_and_place(itemstack, placer, pointed_thing,
				minetest.setting_getbool("creative_mode"),
				{invert_wall = placer:get_player_control().sneak})
		return itemstack
	end
end

--------------------------------------------------------------------------------
function tbl.explode_table_event(evt)
	if evt ~= nil then
		local parts = evt:split(":")
		if #parts == 3 then
			local t = parts[1]:trim()
			local r = tonumber(parts[2]:trim())
			local c = tonumber(parts[3]:trim())
			if type(r) == "number" and type(c) == "number" and t ~= "INV" then
				return {type=t, row=r, column=c}
			end
		end
	end
	return {type="INV", row=0, column=0}
end

--------------------------------------------------------------------------------
function tbl.explode_textlist_event(evt)
	if evt ~= nil then
		local parts = evt:split(":")
		if #parts == 2 then
			local t = parts[1]:trim()
			local r = tonumber(parts[2]:trim())
			if type(r) == "number" and t ~= "INV" then
				return {type=t, index=r}
			end
		end
	end
	return {type="INV", index=0}
end

--------------------------------------------------------------------------------
-- mainmenu only functions
--------------------------------------------------------------------------------
if engine ~= nil then
	engine.get_game = function(index)
		local games = game.get_games()
		
		if index > 0 and index <= #games then
			return games[index]
		end
		
		return nil
	end
	
	function fgettext(text, ...)
		text = engine.gettext(text)
		local arg = {n=select('#', ...), ...}
		if arg.n >= 1 then
			-- Insert positional parameters ($1, $2, ...)
			result = ''
			pos = 1
			while pos <= text:len() do
				newpos = text:find('[$]', pos)
				if newpos == nil then
					result = result .. text:sub(pos)
					pos = text:len() + 1
				else
					paramindex = tonumber(text:sub(newpos+1, newpos+1))
					result = result .. text:sub(pos, newpos-1) .. tostring(arg[paramindex])
					pos = newpos + 2
				end
			end
			text = result
		end
		return engine.formspec_escape(text)
	end
end
--------------------------------------------------------------------------------
-- core only fct
--------------------------------------------------------------------------------
if minetest ~= nil then
	--------------------------------------------------------------------------------
	function minetest.pos_to_string(pos)
		return "(" .. pos.x .. "," .. pos.y .. "," .. pos.z .. ")"
	end
end

