--
-- HP Change Reasons
--
local expect = nil
minetest.register_on_player_hpchange(function(player, hp, reason)
	if expect == nil then
		return
	end

	for key, value in pairs(reason) do
		assert(expect[key] == value)
	end
	for key, value in pairs(expect) do
		assert(reason[key] == value)
	end

	expect = nil
end)

local function run_hpchangereason_tests(player)
	local old_hp = player:get_hp()

	player:set_hp(20)
	expect = { type = "set_hp", from = "mod" }
	player:set_hp(3)
	assert(expect == nil)

	expect = { a = 234, type = "set_hp", from = "mod" }
	player:set_hp(7, { a= 234 })
	assert(expect == nil)

	expect = { df = 3458973454, type = "fall", from = "mod" }
	player:set_hp(10, { type = "fall", df = 3458973454 })
	assert(expect == nil)

	player:set_hp(old_hp)
end
unittests.register("test_hpchangereason", run_hpchangereason_tests, {player=true})

--
-- Player meta
--
local function run_player_meta_tests(player)
	local meta = player:get_meta()
	meta:set_string("foo", "bar")
	assert(meta:contains("foo"))
	assert(meta:get_string("foo") == "bar")
	assert(meta:get("foo") == "bar")

	local meta2 = player:get_meta()
	assert(meta2:get_string("foo") == "bar")
	assert(meta2:get("foo") == "bar")
	assert(meta:equals(meta2))

	meta:set_string("bob", "dillan")
	assert(meta:get_string("foo") == "bar")
	assert(meta:get_string("bob") == "dillan")
	assert(meta:get("bob") == "dillan")
	assert(meta2:get_string("foo") == "bar")
	assert(meta2:get_string("bob") == "dillan")
	assert(meta2:get("bob") == "dillan")
	assert(meta:equals(meta2))

	meta:set_string("foo", "")
	assert(not meta:contains("foo"))
	assert(meta:get("foo") == nil)
	assert(meta:get_string("foo") == "")
	assert(meta:equals(meta2))
end
unittests.register("test_player_meta", run_player_meta_tests, {player=true})

--
-- Player add pos
--
local function run_player_add_pos_tests(player)
	local pos = player:get_pos()
	player:add_pos(vector.new(0, 1000, 0))
	local newpos = player:get_pos()
	player:add_pos(vector.new(0, -1000, 0))
	local backpos = player:get_pos()
	local newdist = vector.distance(pos, newpos)
	assert(math.abs(newdist - 1000) <= 1)
	assert(vector.distance(pos, backpos) <= 1)
end
unittests.register("test_player_add_pos", run_player_add_pos_tests, {player=true})

--
-- Hotbar selection clamp
--
local function run_player_hotbar_clamp_tests(player)
	local inv = player:get_inventory()
	local old_inv_size = inv:get_size("main")
	local old_inv_list = inv:get_list("main") -- Avoid accidentally removing item
	local old_bar_size = player:hud_get_hotbar_itemcount()

	inv:set_size("main", 5)

	player:hud_set_hotbar_itemcount(2)
	assert(player:hud_get_hotbar_itemcount() == 2)

	player:hud_set_hotbar_itemcount(6)
	assert(player:hud_get_hotbar_itemcount() == 5)

	inv:set_size("main", old_inv_size)
	inv:set_list("main", old_inv_list)
	player:hud_set_hotbar_itemcount(old_bar_size)
end
unittests.register("test_player_hotbar_clamp", run_player_hotbar_clamp_tests, {player=true})
