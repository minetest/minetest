local ffi_shared = ...
local ffi = ffi_shared.ffi
local ie = ffi_shared.insecure_environment

local metatable = ie.debug.getregistry().PcgRandom
local C, cast = ffi.C, ffi.cast
local tobit = _G.bit.tobit
local tonumber, error = _G.tonumber, _G.error

ffi.cdef[[
int32_t mtffi_pcg_rng_next(void *ud, int32_t min, int32_t max);

int32_t mtffi_pcg_rng_rand_normal_dist(void *ud, int32_t min, int32_t max, int num_trials);
]]

local ptrdiff_t = ffi.typeof("ptrdiff_t")

local check = ffi_shared.method_checker("PcgRandom", metatable)

local methodtable = metatable.__metatable

local function check_range(min, max)
	-- Replicate the rounding/truncation in src/script/lua_api/l_noise.cpp
	min = tobit(cast(ptrdiff_t, tonumber(min) or -2147483648))
	max = tobit(cast(ptrdiff_t, tonumber(max) or 2147483647))
	if max < min then
		error("Invalid range (max < min)", 3)
	end
	return min, max
end

function methodtable:next(min, max)
	check(self)
	min, max = check_range(min, max)
	return C.mtffi_pcg_rng_next(self, min, max)
end

function methodtable:rand_normal_dist(min, max, num_trials)
	check(self)
	min, max = check_range(min, max)
	num_trials = cast(ptrdiff_t, tonumber(num_trials) or 6)
	return C.mtffi_pcg_rng_rand_normal_dist(self, min, max, num_trials)
end
