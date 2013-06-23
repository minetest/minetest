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

--------------------------------------------------------------------------------
function string:split(sep)
	local sep, fields = sep or ",", {}
	local pattern = string.format("([^%s]+)", sep)
	self:gsub(pattern, function(c) fields[#fields+1] = c end)
	return fields
end

--------------------------------------------------------------------------------
function string:trim()
	return (self:gsub("^%s*(.-)%s*$", "%1"))
end

--------------------------------------------------------------------------------
engine.get_game = function(index)
	local games = game.get_games()
	
	if index > 0 and index <= #games then
		return games[index]
	end
	
	return nil
end

--------------------------------------------------------------------------------
function fs_escape_string(text)
	if text ~= nil then
		while (text:find("\r\n") ~= nil) do
			local newtext = text:sub(1,text:find("\r\n")-1)
			newtext = newtext .. " " .. text:sub(text:find("\r\n")+3)
			
			text = newtext
		end
		
		while (text:find("\n") ~= nil) do
			local newtext = text:sub(1,text:find("\n")-1)
			newtext = newtext .. " " .. text:sub(text:find("\n")+1)
			
			text = newtext
		end
		
		while (text:find("\r") ~= nil) do
			local newtext = text:sub(1,text:find("\r")-1)
			newtext = newtext .. " " .. text:sub(text:find("\r")+1)
			
			text = newtext
		end
		
		text = text:gsub("%[","%[%[")
		text = text:gsub("]","]]")
		text = text:gsub(";"," ")
	end
	return text
end

--------------------------------------------------------------------------------
function explode_textlist_event(text)
	
	local retval = {}
	retval.typ = "INV"
	
	local parts = text:split(":")
				
	if #parts == 2 then
		retval.typ = parts[1]:trim()
		retval.index= tonumber(parts[2]:trim())
		
		if type(retval.index) ~= "number" then
			retval.typ = "INV"
		end
	end
	
	return retval
end
