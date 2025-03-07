-- Modify core.get_mod_storage to return the storage for the current mod.

local get_current_modname = core.get_current_modname

local old_get_mod_storage = core.get_mod_storage

local storages = setmetatable({}, {
	__mode = "v", -- values are weak references (can be garbage-collected)
	__index = function(self, modname)
		local storage = old_get_mod_storage(modname)
		self[modname] = storage
		return storage
	end,
})

function core.get_mod_storage()
	local modname = get_current_modname()
	return modname and storages[modname]
end
