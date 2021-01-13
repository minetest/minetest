-- Minetest: builtin/client/render.lua
local next_draws = {}
local next_predraws = {}

function core.on_next_draw(func)
	next_draws[#next_draws + 1] = func
end

function core.on_next_predraw(func)
	next_predraws[#next_predraws + 1] = func
end

core.register_on_draw(function(dtime)
	for _, func in ipairs(next_draws) do
		func(dtime)
	end
	next_draws = {}
end)

core.register_on_predraw(function(dtime)
	for _, func in ipairs(next_predraws) do
		func(dtime)
	end
	next_predraws = {}
end)
