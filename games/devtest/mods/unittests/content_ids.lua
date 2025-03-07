core.register_alias("unittests:test_content_ids_alias1", "air")
core.register_alias("unittests:test_content_ids_alias2", "~")

local function test_content_ids()
	assert(core.get_content_id("air") == core.CONTENT_AIR)
	assert(core.get_content_id("unittests:test_content_ids_alias1") == core.CONTENT_AIR)
	assert(core.get_content_id("unknown") == core.CONTENT_UNKNOWN)
	assert(core.get_content_id("ignore") == core.CONTENT_IGNORE)

	assert(core.get_name_from_content_id(core.CONTENT_AIR) == "air")
	assert(core.get_name_from_content_id(core.CONTENT_UNKNOWN) == "unknown")
	assert(core.get_name_from_content_id(core.CONTENT_IGNORE) == "ignore")

	assert(pcall(core.get_content_id, "~") == false)
	assert(pcall(core.get_content_id, "unittests:test_content_ids_alias2") == false)
	assert(pcall(core.get_content_id) == false)
	assert(core.get_name_from_content_id(0xFFFF) == "unknown")
	assert(pcall(core.get_name_from_content_id) == false)
end

-- Run while mod is loading.
test_content_ids()

-- Run after mods have loaded.
unittests.register("test_content_ids", test_content_ids)

-- Run in async environment.
local function test_content_ids_async(cb)
	local function func(test_func)
		local ok, err = pcall(test_func)
		if not ok then
			return err
		end
	end
	core.handle_async(func, cb, test_content_ids)
end
unittests.register("test_content_ids_async", test_content_ids_async, {async=true})
