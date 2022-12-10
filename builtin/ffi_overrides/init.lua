local ie = ...

if not _G.core.settings:get_bool("use_ffi", true) then
	return
end

if not _G.core.global_exists("jit") then
	-- Not LuaJIT, nothing to do.
	return
end

if _G.jit.version_num < 20100 then
	_G.core.log("warning",
		"FFI implementations are disabled on LuaJIT versions before 2.1.")
	return
end

local has_ffi, ffi = _G.pcall(ie.require, "ffi")
if not has_ffi then
	_G.core.log("warning",
		"Since the FFI library is absent, " ..
		"optimized FFI implementions are unavailable.")
	return
end

local ffi_shared = {
	ffi = ffi,
	insecure_environment = ie,
}

local rawequal, error = _G.rawequal, _G.error
local get_real_metatable = ie.debug.getmetatable

function ffi_shared.method_checker(name, metatable)
	local error_message = "Invalid object passed to " .. name .. " method"
	return function(o)
		if not rawequal(get_real_metatable(o), metatable) then
			error(error_message, 3)
		end
	end
end

local ffipath = _G.core.get_builtin_path() .. "ffi_overrides" .. _G.DIR_DELIM
