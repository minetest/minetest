local ffi_shared = ...
local ie = _G.core.request_insecure_environment()
local ffi = ie.require("ffi")
local script = ffi_shared.script
local bits_to_node = ffi_shared.bits_to_node

local C = ffi.C
local tonumber = ie.tonumber

ffi.cdef[[
int32_t mtffi_get_node(void *script, double x, double y, double z);

int64_t mtffi_get_node_or_neg(void *script, double x, double y, double z);
]]

function _G.core.get_node(pos)
	return (bits_to_node(C.mtffi_get_node(script,
			tonumber(pos.x) or 0, tonumber(pos.y) or 0, tonumber(pos.z) or 0)))
end

function _G.core.get_node_or_nil(pos)
	local bits = C.mtffi_get_node_or_neg(script,
			tonumber(pos.x) or 0, tonumber(pos.y) or 0, tonumber(pos.z) or 0)
	return bits >= 0 and bits_to_node(bits) or nil
end
