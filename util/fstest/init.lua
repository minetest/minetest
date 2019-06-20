core.set_clouds(false)

local tests = {}

local function load_tests(path)
	local list = core.get_dir_list(path, false)
	for _, filename in ipairs(list) do
		if filename:sub(-4) == ".txt" then
			local name = filename:sub(1, -5)

			local f = io.open(path .. DIR_DELIM .. filename)
			local formspec = f:read("*all")
			f:close()

			tests[#tests + 1] = {
				name = name,
				formspec = formspec
			}
		end
	end
end

load_tests("util/fstest/tests")

local co = coroutine.create(function()
	for i=1, #tests do
		core.update_formspec(tests[i].formspec)
		coroutine.yield()
		core.take_screenshot("util/fstest/tests/" .. tests[i].name .. ".out.png")
	end
	core.close()
end)


function core.on_step()
	coroutine.resume(co)
end
