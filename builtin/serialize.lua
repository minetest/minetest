-- Minetest: builtin/serialize.lua

-- https://github.com/fab13n/metalua/blob/no-dll/src/lib/serialize.lua
-- Copyright (c) 2006-2997 Fabien Fleutot <metalua@gmail.com>
-- License: MIT
--------------------------------------------------------------------------------
-- Serialize an object into a source code string. This string, when passed as
-- an argument to deserialize(), returns an object structurally identical
-- to the original one. The following are currently supported:
-- * strings, numbers, booleans, nil
-- * tables thereof. Tables can have shared part, but can't be recursive yet.
-- Caveat: metatables and environments aren't saved.
--------------------------------------------------------------------------------

local no_identity = { number=1, boolean=1, string=1, ['nil']=1 }

function minetest.serialize(x)

	local gensym_max   =  0  -- index of the gensym() symbol generator
	local seen_once    = { } -- element->true set of elements seen exactly once in the table
	local multiple     = { } -- element->varname set of elements seen more than once
	local nested       = { } -- transient, set of elements currently being traversed
	local nest_points  = { }
	local nest_patches = { }
	
	local function gensym()
		gensym_max = gensym_max + 1 ;  return gensym_max
	end

	-----------------------------------------------------------------------------
	-- nest_points are places where a table appears within itself, directly or not.
	-- for instance, all of these chunks create nest points in table x:
	-- "x = { }; x[x] = 1", "x = { }; x[1] = x", "x = { }; x[1] = { y = { x } }".
	-- To handle those, two tables are created by mark_nest_point:
	-- * nest_points [parent] associates all keys and values in table parent which
	--   create a nest_point with boolean `true'
	-- * nest_patches contain a list of { parent, key, value } tuples creating
	--   a nest point. They're all dumped after all the other table operations
	--   have been performed.
	--
	-- mark_nest_point (p, k, v) fills tables nest_points and nest_patches with
	-- informations required to remember that key/value (k,v) create a nest point
	-- in table parent. It also marks `parent' as occuring multiple times, since
	-- several references to it will be required in order to patch the nest
	-- points.
	-----------------------------------------------------------------------------
	local function mark_nest_point (parent, k, v)
		local nk, nv = nested[k], nested[v]
		assert (not nk or seen_once[k] or multiple[k])
		assert (not nv or seen_once[v] or multiple[v])
		local mode = (nk and nv and "kv") or (nk and "k") or ("v")
		local parent_np = nest_points [parent]
		local pair = { k, v }
		if not parent_np then parent_np = { }; nest_points [parent] = parent_np end
		parent_np [k], parent_np [v] = nk, nv
		table.insert (nest_patches, { parent, k, v })
		seen_once [parent], multiple [parent]  = nil, true
	end

	-----------------------------------------------------------------------------
	-- First pass, list the tables and functions which appear more than once in x
	-----------------------------------------------------------------------------
	local function mark_multiple_occurences (x)
		if no_identity [type(x)] then return end
		if     seen_once [x]     then seen_once [x], multiple [x] = nil, true
		elseif multiple  [x]     then -- pass
		else   seen_once [x] = true end
		
		if type (x) == 'table' then
			nested [x] = true
			for k, v in pairs (x) do
				if nested[k] or nested[v] then mark_nest_point (x, k, v) else
					mark_multiple_occurences (k)
					mark_multiple_occurences (v)
				end
			end
			nested [x] = nil
		end
	end

	local dumped    = { } -- multiply occuring values already dumped in localdefs
	local localdefs = { } -- already dumped local definitions as source code lines

	-- mutually recursive functions:
	local dump_val, dump_or_ref_val

	--------------------------------------------------------------------
	-- if x occurs multiple times, dump the local var rather than the
	-- value. If it's the first time it's dumped, also dump the content
	-- in localdefs.
	--------------------------------------------------------------------
	function dump_or_ref_val (x)
		if nested[x] then return 'false' end -- placeholder for recursive reference
		if not multiple[x] then return dump_val (x) end
		local var = dumped [x]
		if var then return "_[" .. var .. "]" end -- already referenced
		local val = dump_val(x) -- first occurence, create and register reference
		var = gensym()
		table.insert(localdefs, "_["..var.."]="..val)
		dumped [x] = var
		return "_[" .. var .. "]"
	end

	-----------------------------------------------------------------------------
	-- Second pass, dump the object; subparts occuring multiple times are dumped
	-- in local variables which can be referenced multiple times;
	-- care is taken to dump locla vars in asensible order.
	-----------------------------------------------------------------------------
	function dump_val(x)
		local  t = type(x)
		if     x==nil        then return 'nil'
		elseif t=="number"   then return tostring(x)
		elseif t=="string"   then return string.format("%q", x)
		elseif t=="boolean"  then return x and "true" or "false"
		elseif t=="function" then
			return "loadstring("..string.format("%q", string.dump(x))..")"
		elseif t=="table" then
			local acc        = { }
			local idx_dumped = { }
			local np         = nest_points [x]
			for i, v in ipairs(x) do
				if np and np[v] then
					table.insert (acc, 'false') -- placeholder
				else
					table.insert (acc, dump_or_ref_val(v))
				end
				idx_dumped[i] = true
			end
			for k, v in pairs(x) do
				if np and (np[k] or np[v]) then
					--check_multiple(k); check_multiple(v) -- force dumps in localdefs
				elseif not idx_dumped[k] then
					table.insert (acc, "[" .. dump_or_ref_val(k) .. "] = " .. dump_or_ref_val(v))
				end
			end
			return "{ "..table.concat(acc,", ").." }"
		else
			error ("Can't serialize data of type "..t)
		end
	end
	
	local function dump_nest_patches()
		for _, entry in ipairs(nest_patches) do
			local p, k, v = unpack (entry)
			assert (multiple[p])
			local set = dump_or_ref_val (p) .. "[" .. dump_or_ref_val (k) .. "] = " .. 
				dump_or_ref_val (v) .. " -- rec "
			table.insert (localdefs, set)
		end
	end

	mark_multiple_occurences (x)
	local toplevel = dump_or_ref_val (x)
	dump_nest_patches()

	if next (localdefs) then
		return "local _={ }\n" ..
			table.concat (localdefs, "\n") .. 
			"\nreturn " .. toplevel
	else
		return "return " .. toplevel
	end
end

-- Deserialization.
-- http://stackoverflow.com/questions/5958818/loading-serialized-data-into-a-table
--

local env = {
	loadstring = loadstring,
}

local function noop() end

local safe_env = {
	loadstring = noop,
}

local function stringtotable(sdata, safe)
	if sdata:byte(1) == 27 then return nil, "binary bytecode prohibited" end
	local f, message = assert(loadstring(sdata))
	if not f then return nil, message end
	if safe then
		setfenv(f, safe_env)
	else
		setfenv(f, env)
	end
	return f()
end

function minetest.deserialize(sdata, safe)
	local table = {}
	local okay, results = pcall(stringtotable, sdata, safe)
	if okay then
		return results
	end
	minetest.log('error', 'minetest.deserialize(): '.. results)
	return nil
end

-- Run some unit tests
local function unit_test()
	function unitTest(name, success)
		if not success then
			error(name .. ': failed')
		end
	end

	unittest_input = {cat={sound="nyan", speed=400}, dog={sound="woof"}}
	unittest_output = minetest.deserialize(minetest.serialize(unittest_input))

	unitTest("test 1a", unittest_input.cat.sound == unittest_output.cat.sound)
	unitTest("test 1b", unittest_input.cat.speed == unittest_output.cat.speed)
	unitTest("test 1c", unittest_input.dog.sound == unittest_output.dog.sound)

	unittest_input = {escapechars="\n\r\t\v\\\"\'", noneuropean="θשׁ٩∂"}
	unittest_output = minetest.deserialize(minetest.serialize(unittest_input))
	unitTest("test 3a", unittest_input.escapechars == unittest_output.escapechars)
	unitTest("test 3b", unittest_input.noneuropean == unittest_output.noneuropean)
end
unit_test() -- Run it
unit_test = nil -- Hide it

