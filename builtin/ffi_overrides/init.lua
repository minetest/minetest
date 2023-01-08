-- Global access is done explicitly with _G to avoid mistakes leading to security issues.

if not _G.core.settings:get_bool("secure.use_ffi", true) then
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

local ie = _G.core.request_insecure_environment()

local has_ffi, ffi = ie.pcall(ie.require, "ffi")
if not has_ffi then
	_G.core.log("warning",
		"Since the FFI library is absent, " ..
		"optimized FFI implementions are unavailable.")
	return
end

_G.core.log("info", "Initializing LuaJIT FFI overrides")

local ffi_shared = {}

local rawequal, error = ie.rawequal, ie.error
local tobit, band, rshift = ie.bit.tobit, ie.bit.band, ie.bit.rshift
local get_name_from_content_id = _G.core.get_name_from_content_id
local get_real_metatable = ie.debug.getmetatable
local registry = ie.debug.getregistry()

if registry.INDIRECT_SCRIPTAPI_RIDX then
	ffi_shared.script = ffi.cast("void**", registry[registry.CUSTOM_RIDX_SCRIPTAPI])[0]
else
	ffi_shared.script = ffi.cast("void*", registry[registry.CUSTOM_RIDX_SCRIPTAPI])
end

function ffi_shared.method_checker(name, metatable)
	local error_message = "Invalid object passed to " .. name .. " method"
	return function(o)
		if not rawequal(get_real_metatable(o), metatable) then
			error(error_message, 3)
		end
	end
end

function ffi_shared.bits_to_node(bits)
	bits = tobit(bits)
	local content = band(bits, 0xFFFF)
	local param1 = band(rshift(bits, 16), 0xFF)
	local param2 = rshift(bits, 24)
	return {name = get_name_from_content_id(content), param1 = param1, param2 = param2}
end

local ffipath = _G.core.get_builtin_path() .. "ffi_overrides" .. _G.DIR_DELIM

if _G.core.get_node then
	_G.assert(_G.loadfile(ffipath .. "map.lua"))(ffi_shared)
end
