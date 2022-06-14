local log = {}

local function insert_log(...)
	log[#log+1] = string.format(...)
end

local function objref_str(self, ref)
	if ref and ref:is_player() then
		return "player"
	end
	return self.object == ref and "self" or tostring(ref)
end

core.register_entity("unittests:callbacks", {
	initial_properties = {
		hp_max = 5,
		visual = "upright_sprite",
		textures = { "unittests_stick.png" },
		static_save = false,
	},

	on_activate = function(self, staticdata, dtime_s)
		self.object:set_armor_groups({test = 100})
		assert(self.object:get_hp() == self.initial_properties.hp_max)
		insert_log("on_activate(%d)", #staticdata)
	end,
	on_deactivate = function(self, removal)
		insert_log("on_deactivate(%s)", tostring(removal))
	end,
	on_punch = function(self, puncher, time_from_last_punch, tool_capabilities, dir, damage)
		insert_log("on_punch(%s, %.1f, %d)", objref_str(self, puncher),
			time_from_last_punch, damage)
	end,
	on_death = function(self, killer)
		assert(self.object:get_hp() == 0)
		insert_log("on_death(%s)", objref_str(self, killer))
	end,
	on_rightclick = function(self, clicker)
		insert_log("on_rightclick(%s)", objref_str(self, clicker))
	end,
	on_attach_child = function(self, child)
		insert_log("on_attach_child(%s)", objref_str(self, child))
	end,
	on_detach_child = function(self, child)
		insert_log("on_detach_child(%s)", objref_str(self, child))
	end,
	on_detach = function(self, parent)
		insert_log("on_detach(%s)", objref_str(self, parent))
	end,
	get_staticdata = function(self)
		assert(false)
	end,
})

--

local function check_log(expect)
	if #expect ~= #log then
		error("Log mismatch: " .. core.write_json(log))
	end
	for i, s in ipairs(expect) do
		if log[i] ~= s then
			error("Log mismatch at " .. i .. ": " .. core.write_json(log))
		end
	end
	log = {} -- clear it for next time
end

local function test_entity_lifecycle(_, pos)
	log = {}

	-- with binary in staticdata
	local obj = core.add_entity(pos, "unittests:callbacks", "abc\000def")
	check_log({"on_activate(7)"})

	obj:set_hp(0)
	check_log({"on_death(nil)", "on_deactivate(true)"})

	-- objectref must be invalid now
	assert(obj:get_velocity() == nil)
end
unittests.register("test_entity_lifecycle", test_entity_lifecycle, {map=true})

local function test_entity_interact(_, pos)
	log = {}

	local obj = core.add_entity(pos, "unittests:callbacks")
	check_log({"on_activate(0)"})

	-- rightclick
	obj:right_click(obj)
	check_log({"on_rightclick(self)"})

	-- useless punch
	obj:punch(obj, 0.5, {})
	check_log({"on_punch(self, 0.5, 0)"})

	-- fatal punch
	obj:punch(obj, 1.9, {
		full_punch_interval = 1.0,
		damage_groups = { test = 10 },
	})
	check_log({
		-- does 10 damage even though we only have 5 hp
		"on_punch(self, 1.9, 10)",
		"on_death(self)",
		"on_deactivate(true)"
	})
end
unittests.register("test_entity_interact", test_entity_interact, {map=true})

local function test_entity_attach(player, pos)
	log = {}

	local obj = core.add_entity(pos, "unittests:callbacks")
	check_log({"on_activate(0)"})

	-- attach player to entity
	player:set_attach(obj)
	check_log({"on_attach_child(player)"})
	player:set_detach()
	check_log({"on_detach_child(player)"})

	-- attach entity to player
	obj:set_attach(player)
	check_log({})
	obj:set_detach()
	check_log({"on_detach(player)"})

	obj:remove()
end
unittests.register("test_entity_attach", test_entity_attach, {player=true, map=true})
