-- Minetest: builtin/common/sounds.lua

core.registered_sounds = {}

local defaults = {}
local pending = {}

function core.register_node_sounds(names, spec)
	setmetatable(spec, { __index = defaults })

	for i=1, #names do
		local cat = names[i]
		core.registered_sounds[cat] = core.registered_sounds[cat] or spec
	end
end

function core.get_node_sound_defaults()
	return defaults
end

function core.set_node_sound_defaults(spec)
	for key, value in pairs(spec) do
		defaults[key] = value
	end
end

function core.find_node_sounds(names, overrides)
	if type(names) == "string" then
		names = { names }
	end

	if pending then
		pending[#pending + 1] = {
			names = names,
			overrides = overrides,
			spec = {},
		}
		return pending[#pending].spec
	end

	for i=1, #names do
		local sound = core.registered_sounds[names[i]]
		if sound then
			if overrides then
				setmetatable(overrides, { __index = sound })
				return overrides
			else
				return sound
			end
		end
	end

	return nil
end

local function resolve_node_sounds()
	local to_resolve = pending
	pending = nil

	for i=1, #to_resolve do
		local thing = to_resolve[i]
		local result = core.find_node_sounds(thing.names)
		for key, value in pairs(result) do
			thing.spec[key] = value
		end
		for key, value in pairs(thing.overrides or {}) do
			thing.spec[key] = value
		end
	end
end

core.register_on_mods_loaded(function()
	resolve_node_sounds()
end)
