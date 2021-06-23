-- Cache Functions To Local Register
local string_format = string.format
local string_dump = string.dump
local table_concat = table.concat
local pairs = pairs
local type = type

function core.serialize(input)
	local saved = {}
	local num_saved_locals = 0

	local function stringify(name, value)
		local tp = type(value)

		if tp == "table" then
			if not saved[value] then
				saved[value] = name

				local buff = {}
				for k, v in pairs(value) do
					-- Inlined Name-Conversion Logic For Speed Reasons
					local n_key
					local n_tp = type(k)

					if n_tp == "table" then
						if saved[k] then
							n_key = saved[k]
						else
							local n_name = "_" .. num_saved_locals
							buff[#buff + 1] = "local " .. n_name .. "=" .. stringify(n_name, k)
							saved[k] = n_name
							n_key = n_name
						end
					elseif n_tp == "function" then
						n_key = string_format("loadstring(%q)", string_dump(k, true))
					elseif n_tp == "boolean" then
						n_key = k and "true" or "false"
					elseif n_tp == "nil" then
						n_key = "nil"
					elseif n_tp == "number" then
						n_key = string_format("%.17g", k)
					elseif n_tp == "string" then
						n_key = string_format("%q", k)
					else
						error("Invalid Datatype Used As Key")
					end

					local n = string_format("%s[%s]", name, n_key)
					buff[#buff + 1] = n .. "=" .. stringify(n, v)
				end

				return "{}\n" .. table_concat(buff, "\n")
			else
				return saved[value]
			end
		elseif tp == "function" then
			return string_format("loadstring(%q)", string_dump(value, true))
		elseif tp == "boolean" then
			return value and "true" or "false"
		elseif tp == "nil" then
			return "nil"
		elseif tp == "number" then
			return string_format("%.17g", value)
		elseif tp == "string" then
			return string_format("%q", value)
		else
			error("Invalid Datatype Passed To minetest.serialize()")
		end
	end

	return "local _=" .. stringify("_", input) .. "\nreturn _"
end

local function dummy_func() end

local function safe_loadstring(s)
	local func, err = loadstring(s)
	if err then return nil, err end
	setfenv(func, {})
	return func
end

function core.deserialize(input, do_load_functions)
	if type(input) ~= "string" then
		return nil, "Deserialization Input Must Be A String"
	elseif input:byte(1) == 0x1B then
		return nil, "Deserialization Input Must Not Be Bytecode"
	end

	local func, err = loadstring(input)
	if err then return nil, err end

	setfenv(func, {
		loadstring = do_load_functions and dummy_func or safe_loadstring
	})

	local is_good, data = pcall(func)
	if not is_good then return nil, data end

	return data
end
