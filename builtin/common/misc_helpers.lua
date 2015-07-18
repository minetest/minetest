-- Minetest: builtin/misc_helpers.lua

--------------------------------------------------------------------------------
-- Localize functions to avoid table lookups (better performance).
local table_insert = table.insert
local string_sub, string_find = string.sub, string.find

--------------------------------------------------------------------------------
function basic_dump(o)
	local tp = type(o)
	if tp == "number" then
		return tostring(o)
	elseif tp == "string" then
		return string.format("%q", o)
	elseif tp == "boolean" then
		return tostring(o)
	elseif tp == "nil" then
		return "nil"
	-- Uncomment for full function dumping support.
	-- Not currently enabled because bytecode isn't very human-readable and
	-- dump's output is intended for humans.
	--elseif tp == "function" then
	--	return string.format("loadstring(%q)", string.dump(o))
	else
		return string.format("<%s>", tp)
	end
end

local keywords = {
	["and"] = true,
	["break"] = true,
	["do"] = true,
	["else"] = true,
	["elseif"] = true,
	["end"] = true,
	["false"] = true,
	["for"] = true,
	["function"] = true,
	["goto"] = true,  -- Lua 5.2
	["if"] = true,
	["in"] = true,
	["local"] = true,
	["nil"] = true,
	["not"] = true,
	["or"] = true,
	["repeat"] = true,
	["return"] = true,
	["then"] = true,
	["true"] = true,
	["until"] = true,
	["while"] = true,
}
local function is_valid_identifier(str)
	if not str:find("^[a-zA-Z_][a-zA-Z0-9_]*$") or keywords[str] then
		return false
	end
	return true
end

--------------------------------------------------------------------------------
-- Dumps values in a line-per-value format.
-- For example, {test = {"Testing..."}} becomes:
--   _["test"] = {}
--   _["test"][1] = "Testing..."
-- This handles tables as keys and circular references properly.
-- It also handles multiple references well, writing the table only once.
-- The dumped argument is internal-only.

function dump2(o, name, dumped)
	name = name or "_"
	-- "dumped" is used to keep track of serialized tables to handle
	-- multiple references and circular tables properly.
	-- It only contains tables as keys.  The value is the name that
	-- the table has in the dump, eg:
	-- {x = {"y"}} -> dumped[{"y"}] = '_["x"]'
	dumped = dumped or {}
	if type(o) ~= "table" then
		return string.format("%s = %s\n", name, basic_dump(o))
	end
	if dumped[o] then
		return string.format("%s = %s\n", name, dumped[o])
	end
	dumped[o] = name
	-- This contains a list of strings to be concatenated later (because
	-- Lua is slow at individual concatenation).
	local t = {}
	for k, v in pairs(o) do
		local keyStr
		if type(k) == "table" then
			if dumped[k] then
				keyStr = dumped[k]
			else
				-- Key tables don't have a name, so use one of
				-- the form _G["table: 0xFFFFFFF"]
				keyStr = string.format("_G[%q]", tostring(k))
				-- Dump key table
				table_insert(t, dump2(k, keyStr, dumped))
			end
		else
			keyStr = basic_dump(k)
		end
		local vname = string.format("%s[%s]", name, keyStr)
		table_insert(t, dump2(v, vname, dumped))
	end
	return string.format("%s = {}\n%s", name, table.concat(t))
end

--------------------------------------------------------------------------------
-- This dumps values in a one-statement format.
-- For example, {test = {"Testing..."}} becomes:
-- [[{
-- 	test = {
-- 		"Testing..."
-- 	}
-- }]]
-- This supports tables as keys, but not circular references.
-- It performs poorly with multiple references as it writes out the full
-- table each time.
-- The indent field specifies a indentation string, it defaults to a tab.
-- Use the empty string to disable indentation.
-- The dumped and level arguments are internal-only.

function dump(o, indent, nested, level)
	if type(o) ~= "table" then
		return basic_dump(o)
	end
	-- Contains table -> true/nil of currently nested tables
	nested = nested or {}
	if nested[o] then
		return "<circular reference>"
	end
	nested[o] = true
	indent = indent or "\t"
	level = level or 1
	local t = {}
	local dumped_indexes = {}
	for i, v in ipairs(o) do
		table_insert(t, dump(v, indent, nested, level + 1))
		dumped_indexes[i] = true
	end
	for k, v in pairs(o) do
		if not dumped_indexes[k] then
			if type(k) ~= "string" or not is_valid_identifier(k) then
				k = "["..dump(k, indent, nested, level + 1).."]"
			end
			v = dump(v, indent, nested, level + 1)
			table_insert(t, k.." = "..v)
		end
	end
	nested[o] = nil
	if indent ~= "" then
		local indent_str = "\n"..string.rep(indent, level)
		local end_indent_str = "\n"..string.rep(indent, level - 1)
		return string.format("{%s%s%s}",
				indent_str,
				table.concat(t, ","..indent_str),
				end_indent_str)
	end
	return "{"..table.concat(t, ", ").."}"
end

--------------------------------------------------------------------------------
function string.split(str, delim, include_empty, max_splits, sep_is_pattern)
	delim = delim or ","
	max_splits = max_splits or -1
	local items = {}
	local pos, len, seplen = 1, #str, #delim
	local plain = not sep_is_pattern
	max_splits = max_splits + 1
	repeat
		local np, npe = string_find(str, delim, pos, plain)
		np, npe = (np or (len+1)), (npe or (len+1))
		if (not np) or (max_splits == 1) then
			np = len + 1
			npe = np
		end
		local s = string_sub(str, pos, np - 1)
		if include_empty or (s ~= "") then
			max_splits = max_splits - 1
			table_insert(items, s)
		end
		pos = npe + 1
	until (max_splits == 0) or (pos > (len + 1))
	return items
end

--------------------------------------------------------------------------------
function table.indexof(list, val)
	for i = 1, #list do
		if list[i] == val then
			return i
		end
	end
	return -1
end

assert(table.indexof({"foo", "bar"}, "foo") == 1)
assert(table.indexof({"foo", "bar"}, "baz") == -1)

--------------------------------------------------------------------------------
function file_exists(filename)
	local f = io.open(filename, "r")
	if f == nil then
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
function math.sign(x, tolerance)
	tolerance = tolerance or 0
	if x > tolerance then
		return 1
	elseif x < -tolerance then
		return -1
	end
	return 0
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

function core.formspec_escape(text)
	if text ~= nil then
		text = string.gsub(text,"\\","\\\\")
		text = string.gsub(text,"%]","\\]")
		text = string.gsub(text,"%[","\\[")
		text = string.gsub(text,";","\\;")
		text = string.gsub(text,",","\\,")
	end
	return text
end


function core.splittext(text,charlimit)
	local retval = {}

	local current_idx = 1

	local start,stop = string_find(text, " ", current_idx)
	local nl_start,nl_stop = string_find(text, "\n", current_idx)
	local gotnewline = false
	if nl_start ~= nil and (start == nil or nl_start < start) then
		start = nl_start
		stop = nl_stop
		gotnewline = true
	end
	local last_line = ""
	while start ~= nil do
		if string.len(last_line) + (stop-start) > charlimit then
			table_insert(retval, last_line)
			last_line = ""
		end

		if last_line ~= "" then
			last_line = last_line .. " "
		end

		last_line = last_line .. string_sub(text, current_idx, stop - 1)

		if gotnewline then
			table_insert(retval, last_line)
			last_line = ""
			gotnewline = false
		end
		current_idx = stop+1

		start,stop = string_find(text, " ", current_idx)
		nl_start,nl_stop = string_find(text, "\n", current_idx)

		if nl_start ~= nil and (start == nil or nl_start < start) then
			start = nl_start
			stop = nl_stop
			gotnewline = true
		end
	end

	--add last part of text
	if string.len(last_line) + (string.len(text) - current_idx) > charlimit then
			table_insert(retval, last_line)
			table_insert(retval, string_sub(text, current_idx))
	else
		last_line = last_line .. " " .. string_sub(text, current_idx)
		table_insert(retval, last_line)
	end

	return retval
end

--------------------------------------------------------------------------------

if INIT == "game" then
	local dirs1 = {9, 18, 7, 12}
	local dirs2 = {20, 23, 22, 21}

	function core.rotate_and_place(itemstack, placer, pointed_thing,
				infinitestacks, orient_flags)
		orient_flags = orient_flags or {}

		local unode = core.get_node_or_nil(pointed_thing.under)
		if not unode then
			return
		end
		local undef = core.registered_nodes[unode.name]
		if undef and undef.on_rightclick then
			undef.on_rightclick(pointed_thing.under, unode, placer,
					itemstack, pointed_thing)
			return
		end
		local pitch = placer:get_look_pitch()
		local fdir = core.dir_to_facedir(placer:get_look_dir())
		local wield_name = itemstack:get_name()

		local above = pointed_thing.above
		local under = pointed_thing.under
		local iswall = (above.y == under.y)
		local isceiling = not iswall and (above.y < under.y)
		local anode = core.get_node_or_nil(above)
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

		if core.is_protected(pos, placer:get_player_name()) then
			core.record_protection_violation(pos,
					placer:get_player_name())
			return
		end

		local ndef = core.registered_nodes[node.name]
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
			core.set_node(pos, {name = wield_name,
					param2 = dirs1[fdir+1]})
		elseif isceiling then
			if orient_flags.force_facedir then
				core.set_node(pos, {name = wield_name,
						param2 = 20})
			else
				core.set_node(pos, {name = wield_name,
						param2 = dirs2[fdir+1]})
			end
		else -- place right side up
			if orient_flags.force_facedir then
				core.set_node(pos, {name = wield_name,
						param2 = 0})
			else
				core.set_node(pos, {name = wield_name,
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


	core.rotate_node = function(itemstack, placer, pointed_thing)
		core.rotate_and_place(itemstack, placer, pointed_thing,
				core.setting_getbool("creative_mode"),
				{invert_wall = placer:get_player_control().sneak})
		return itemstack
	end
end

--------------------------------------------------------------------------------
function core.explode_table_event(evt)
	if evt ~= nil then
		local parts = evt:split(":")
		if #parts == 3 then
			local t = parts[1]:trim()
			local r = tonumber(parts[2]:trim())
			local c = tonumber(parts[3]:trim())
			if type(r) == "number" and type(c) == "number"
					and t ~= "INV" then
				return {type=t, row=r, column=c}
			end
		end
	end
	return {type="INV", row=0, column=0}
end

--------------------------------------------------------------------------------
function core.explode_textlist_event(evt)
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
function core.explode_scrollbar_event(evt)
	local retval = core.explode_textlist_event(evt)

	retval.value = retval.index
	retval.index = nil

	return retval
end

--------------------------------------------------------------------------------
function core.pos_to_string(pos, decimal_places)
	local x = pos.x
	local y = pos.y
	local z = pos.z
	if decimal_places ~= nil then
		x = string.format("%." .. decimal_places .. "f", x)
		y = string.format("%." .. decimal_places .. "f", y)
		z = string.format("%." .. decimal_places .. "f", z)
	end
	return "(" .. x .. "," .. y .. "," .. z .. ")"
end

--------------------------------------------------------------------------------
function core.string_to_pos(value)
	if value == nil then
		return nil
	end

	local p = {}
	p.x, p.y, p.z = string.match(value, "^([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+)$")
	if p.x and p.y and p.z then
		p.x = tonumber(p.x)
		p.y = tonumber(p.y)
		p.z = tonumber(p.z)
		return p
	end
	local p = {}
	p.x, p.y, p.z = string.match(value, "^%( *([%d.-]+)[, ] *([%d.-]+)[, ] *([%d.-]+) *%)$")
	if p.x and p.y and p.z then
		p.x = tonumber(p.x)
		p.y = tonumber(p.y)
		p.z = tonumber(p.z)
		return p
	end
	return nil
end

assert(core.string_to_pos("10.0, 5, -2").x == 10)
assert(core.string_to_pos("( 10.0, 5, -2)").z == -2)
assert(core.string_to_pos("asd, 5, -2)") == nil)

--------------------------------------------------------------------------------
function table.copy(t, seen)
	local n = {}
	seen = seen or {}
	seen[t] = n
	for k, v in pairs(t) do
		n[(type(k) == "table" and (seen[k] or table.copy(k, seen))) or k] =
			(type(v) == "table" and (seen[v] or table.copy(v, seen))) or v
	end
	return n
end
--------------------------------------------------------------------------------
-- mainmenu only functions
--------------------------------------------------------------------------------
if INIT == "mainmenu" then
	function core.get_game(index)
		local games = game.get_games()

		if index > 0 and index <= #games then
			return games[index]
		end

		return nil
	end

	function fgettext_ne(text, ...)
		text = core.gettext(text)
		local arg = {n=select('#', ...), ...}
		if arg.n >= 1 then
			-- Insert positional parameters ($1, $2, ...)
			local result = ''
			local pos = 1
			while pos <= text:len() do
				local newpos = text:find('[$]', pos)
				if newpos == nil then
					result = result .. text:sub(pos)
					pos = text:len() + 1
				else
					local paramindex =
						tonumber(text:sub(newpos+1, newpos+1))
					result = result .. text:sub(pos, newpos-1)
						.. tostring(arg[paramindex])
					pos = newpos + 2
				end
			end
			text = result
		end
		return text
	end

	function fgettext(text, ...)
		return core.formspec_escape(fgettext_ne(text, ...))
	end
end

