
local scriptpath = core.get_builtin_path()
local commonpath = scriptpath .. "common" .. DIR_DELIM
local mypath     = scriptpath .. "sscsm_client".. DIR_DELIM

-- Shared between builtin files, but
-- not exposed to outer context
local builtin_shared = {}

-- Workaround for bug https://www.lua.org/bugs.html#5.2.3-1
-- (fixed in our bundled lua, but users might use system wide lua with bug)
local actual_unpack = unpack
function unpack(t, a, b)
	assert(not b or b < 2^30)
	return actual_unpack(t, a, b)
end
table.unpack = unpack

-- placeholders
-- FIXME: send actual content defs to sscsm env
function core.get_content_id(name)
	return tonumber(name)
end
function core.get_name_from_content_id(id)
	return tostring(id)
end

assert(loadfile(commonpath .. "item_s.lua"))(builtin_shared)
assert(loadfile(commonpath .. "register.lua"))(builtin_shared)
assert(loadfile(mypath .. "register.lua"))(builtin_shared)

dofile(commonpath .. "after.lua")


-- TODO: tmp

local function dings()
	print(dump(core.get_node_or_nil(vector.zero())))
	core.after(1, dings)
end
--~ core.after(0, dings)

print(core.get_current_modname())
