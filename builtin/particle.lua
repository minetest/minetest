-- This file defines some generic particle stuff

function minetest.register_particle(name, intab)
-- tab = {texture, size, timers, on_end, is_upright, fall, fallspeed}
-- tab.timers = {[func(self)]=duration}
	local tab = {}
	for x,y in pairs(intab) do tab[x] = y end

	if not tab.size then tab.size = {x=1, y=1} end
	if not tab.timers then tab.timers = {} end
	if not tab.on_end then tab.on_end = function(self) end end
	if not tab.is_upright then tab.is_upright = false end
	if tab.fall == nil then tab.fall = false end
	if not tab.fallspeed then tab.fallspeed = 0.01 end

	print(dump(tab.on_end == intab.on_end))

	local xx = ''
	if tab.is_upright then xx = 'upright_' end
	local ts = {}
	for f,i in pairs(tab.timers) do
		ts[f] = 0
	end

	minetest.register_entity(name, {
		hp_max = 1,
		groups = {immortal=1},
		physical = false,
		collisionbox = {0,0,0,0,0,0},
		visual = xx.."sprite",
		visual_size = tab.size,
		textures = {tab.texture},
		spritediv = {x=1, y=1},
		initial_sprite_basepos = {x=0, y=0},
		is_visible = true,
		timer = 0,
		endtime = 0,
		timers = tab.timers,
		timestat = ts,
		fall = tab.fall,
		fallspeed = tab.fallspeed,
		on_step = function(self, dtime)
			self.timer = self.timer + dtime
			if self.timer > self.endtime then
				self.on_end(self)
				self.object:remove()
				return
			end
			for f,dur in pairs(self.timers) do
				self.timestat[f] = self.timestat[f] + dtime
				if self.timestat[f] > self.timers[f] then
					f(self)
					self.timestat[f] = 0
				end
			end
			if self.fall then
				local vel = self.object:getvelocity()
				self.object:setvelocity({x=vel.x,y=vel.y-self.fallspeed,z=vel.z})
			end
		end,
		on_end = tab.on_end,
	})
end

function minetest.spawn_particle(name, pos, vel, time)
	if pos == nil then return end
	local obj = minetest.env:add_entity(pos, name)
	local le = obj:get_luaentity()
	obj:setvelocity(vel)
	le.endtime = time
end
