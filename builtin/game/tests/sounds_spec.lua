local function init(resolveSounds)
	_G.core = {}
	local func

	core.register_on_mods_loaded = function(f)
		func = f
	end

	dofile("builtin/game/sounds.lua")
	if resolveSounds then
		func()
	else
		return func
	end
end

local spec = {
	footstep = {name = "", gain = 1.0},
	dug = {name = "default_dug_node", gain = 0.25},
	place = {name = "default_place_node_hard", gain = 1.0},
}

local spec2 = {
	footstep = {name = "2", gain = 1.0},
	dug = {name = "2", gain = 0.25},
	place = {name = "2", gain = 1.0},
}

describe("sounds", function()
	it("registers", function()
		init(true)

		core.register_node_sounds({ "granite", "stone", "cracky" }, spec)
		assert.is_equal(spec, core.registered_sounds["granite"])
		assert.is_equal(spec, core.registered_sounds["stone"])
		assert.is_equal(spec, core.registered_sounds["cracky"])
	end)

	it("finds", function()
		init(true)

		core.register_node_sounds({ "cracky" }, spec2)
		core.register_node_sounds({ "granite", "stone", "cracky" }, spec)
		assert.is_equal(spec, core.registered_sounds["granite"])
		assert.is_equal(spec, core.registered_sounds["stone"])
		assert.is_equal(spec2, core.registered_sounds["cracky"])

		assert.is_equal(spec, core.find_node_sounds("granite"))
		assert.is_equal(spec, core.find_node_sounds("stone"))
		assert.is_equal(spec2, core.find_node_sounds("cracky"))
		assert.is_equal(spec, core.find_node_sounds({ "stone", "cracky" }))
		assert.is_equal(spec2, core.find_node_sounds({ "cracky", "stone" }))
	end)

	it("postpones resolve", function()
		local resolveSounds = init(false)

		local result = core.find_node_sounds("granite")
		assert.are_same({}, result)

		core.register_node_sounds({ "cracky" }, spec2)
		core.register_node_sounds({ "granite", "stone", "cracky" }, spec)

		resolveSounds()

		assert.are_same(spec, result)
	end)
end)
