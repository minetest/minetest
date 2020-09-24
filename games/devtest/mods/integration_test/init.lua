
if not minetest.settings:get_bool("enable_integration_test") then
  return
end

minetest.log("warning", "[TEST] integration-test enabled!")

local pos1 = { x=0, y=-50, z=0 }
local pos2 = { x=30, y=50, z=30 }

minetest.register_on_mods_loaded(function()
	minetest.log("warning", "[TEST] starting tests")
	minetest.after(0, function()
		minetest.log("warning", "[TEST] emerging area")
		minetest.emerge_area(pos1, pos2, function(blockpos, _, calls_remaining)
			minetest.log("action", "Emerged: " .. minetest.pos_to_string(blockpos))
			if calls_remaining > 0 then
				return
			end

			local file = io.open(minetest.get_worldpath().."/test_passed", "w" );
			if file then
				file:write("ok")
				file:close()
			end

			minetest.log("warning", "[TEST] integration tests done!")
			minetest.request_shutdown("success")

		end)
	end)
end)
