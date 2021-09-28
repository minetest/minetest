--- Lua module to serialize values as Lua code.
-- From: https://github.com/fab13n/metalua/blob/no-dll/src/lib/serialize.lua
-- License: MIT
-- @copyright 2006-2997 Fabien Fleutot <metalua@gmail.com>
-- @author Fabien Fleutot <metalua@gmail.com>
-- @author ShadowNinja <shadowninja@minetest.net>
--------------------------------------------------------------------------------

--- Serialize an object into a source code string. This string, when passed as
-- an argument to deserialize(), returns an object structurally identical to
-- the original one.  The following are currently supported:
--   * Booleans, numbers, strings, and nil.
--   * Functions; uses interpreter-dependent (and sometimes platform-dependent) bytecode!
--   * Tables; they can cantain multiple references and can be recursive, but metatables aren't saved.
-- This works in two phases:
--   1. Recursively find and record multiple references and recursion.
--   2. Recursively dump the value into a string.
-- @param x Value to serialize (nil is allowed).
-- @return load()able string containing the value.
function core.serialize(x)
	local local_index  = 1  -- Top index of the "_" local table in the dump
	-- table->nil/1/2 set of tables seen.
	-- nil = not seen, 1 = seen once, 2 = seen multiple times.
	local seen = {}

	-- nest_points are places where a table appears within itself, directly
	-- or not.  For instance, all of these chunks create nest points in
	-- table x: "x = {}; x[x] = 1", "x = {}; x[1] = x",
	-- "x = {}; x[1] = {y = {x}}".
	-- To handle those, two tables are used by mark_nest_point:
	-- * nested - Transient set of tables being currently traversed.
	--   Used for detecting nested tables.
	-- * nest_points - parent->{key=value, ...} table cantaining the nested
	--   keys and values in the parent.  They're all dumped after all the
	--   other table operations have been performed.
	--
	-- mark_nest_point(p, k, v) fills nest_points with information required
	-- to remember that key/value (k, v) creates a nest point  in table
	-- parent. It also marks "parent" and the nested item(s) as occuring
	-- multiple times, since several references to it will be required in
	-- order to patch the nest points.
	local nest_points  = {}
	local nested = {}
	local function mark_nest_point(parent, k, v)
		local nk, nv = nested[k], nested[v]
		local np = nest_points[parent]
		if not np then
			np = {}
			nest_points[parent] = np
		end
		np[k] = v
		seen[parent] = 2
		if nk then seen[k] = 2 end
		if nv then seen[v] = 2 end
	end

	-- First phase, list the tables and functions which appear more than
	-- once in x.
	local function mark_multiple_occurences(x)
		local tp = type(x)
		if tp ~= "table" and tp ~= "function" then
			-- No identity (comparison is done by value, not by instance)
			return
		end
		if seen[x] == 1 then
			seen[x] = 2
		elseif seen[x] ~= 2 then
			seen[x] = 1
		end

		if tp == "table" then
			nested[x] = true
			for k, v in pairs(x) do
				if nested[k] or nested[v] then
					mark_nest_point(x, k, v)
				else
					mark_multiple_occurences(k)
					mark_multiple_occurences(v)
				end
			end
			nested[x] = nil
		end
	end

	local dumped     = {}  -- object->varname set
	local local_defs = {}  -- Dumped local definitions as source code lines

	-- Mutually recursive local functions:
	local dump_val, dump_or_ref_val

	-- If x occurs multiple times, dump the local variable rather than
	-- the value. If it's the first time it's dumped, also dump the
	-- content in local_defs.
	function dump_or_ref_val(x)
		if seen[x] ~= 2 then
			return dump_val(x)
		end
		local var = dumped[x]
		if var then  -- Already referenced
			return var
		end
		-- First occurence, create and register reference
		local val = dump_val(x)
		local i = local_index
		local_index = local_index + 1
		var = "_["..i.."]"
		local_defs[#local_defs + 1] = var.." = "..val
		dumped[x] = var
		return var
	end

	-- Second phase.  Dump the object; subparts occuring multiple times
	-- are dumped in local variables which can be referenced multiple
	-- times.  Care is taken to dump local vars in a sensible order.
	function dump_val(x)
		local  tp = type(x)
		if     x  == nil        then return "nil"
		elseif tp == "string"   then return string.format("%q", x)
		elseif tp == "boolean"  then return x and "true" or "false"
		elseif tp == "function" then
			return string.format("loadstring(%q)", string.dump(x))
		elseif tp == "number"   then
			-- Serialize numbers reversibly with string.format
			return string.format("%.17g", x)
		elseif tp == "table" then
			local vals = {}
			local idx_dumped = {}
			local np = nest_points[x]
			for i, v in ipairs(x) do
				if not np or not np[i] then
					vals[#vals + 1] = dump_or_ref_val(v)
				end
				idx_dumped[i] = true
			end
			for k, v in pairs(x) do
				if (not np or not np[k]) and
						not idx_dumped[k] then
					vals[#vals + 1] = "["..dump_or_ref_val(k).."] = "
						..dump_or_ref_val(v)
				end
			end
			return "{"..table.concat(vals, ", ").."}"
		else
			error("Can't serialize data of type "..tp)
		end
	end

	local function dump_nest_points()
		for parent, vals in pairs(nest_points) do
			for k, v in pairs(vals) do
				local_defs[#local_defs + 1] = dump_or_ref_val(parent)
					.."["..dump_or_ref_val(k).."] = "
					..dump_or_ref_val(v)
			end
		end
	end

	mark_multiple_occurences(x)
	local top_level = dump_or_ref_val(x)
	dump_nest_points()

	if next(local_defs) then
		return "local _ = {}\n"
			..table.concat(local_defs, "\n")
			.."\nreturn "..top_level
	else
		return "return "..top_level
	end
end

-- Deserialization

local function safe_loadstring(...)
	local func, err = loadstring(...)
	if func then
		setfenv(func, {})
		return func
	end
	return nil, err
end

local function dummy_func() end

function core.deserialize(str, safe)
	if type(str) ~= "string" then
		return nil, "Cannot deserialize type '"..type(str)
			.."'. Argument must be a string."
	end
	if str:byte(1) == 0x1B then
		return nil, "Bytecode prohibited"
	end
	local f, err = loadstring(str)
	if not f then return nil, err end

	-- The environment is recreated every time so deseralized code cannot
	-- pollute it with permanent references.
	setfenv(f, {loadstring = safe and dummy_func or safe_loadstring})

	local good, data = pcall(f)
	if good then
		return data
	else
		return nil, data
	end
end
