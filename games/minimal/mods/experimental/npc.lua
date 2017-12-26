--
-- NPC stuff
--

minetest.register_npc("experimental:npc", {
	initial_properties = {
		hp_max = 20,
		physical = false,
		collisionbox = {-0.4,-0.4,-0.4, 0.4,0.4,0.4},
		visual = "sprite",
		visual_size = {x=1, y=1},
		textures = {"experimental_dummyball.png"},
		spritediv = {x=1, y=3},
		initial_sprite_basepos = {x=0, y=0},
	},

	phase = 0,
	phasetimer = 0,

	on_activate = function(self, staticdata)
		minetest.log("Dummyball npc!")
	end,

	on_step = function(self, dtime)
		self.phasetimer = self.phasetimer + dtime
		if self.phasetimer > 2.0 then
			self.phasetimer = self.phasetimer - 2.0
			self.phase = self.phase + 1
			if self.phase >= 3 then
				self.phase = 0
			end
			self.object:setsprite({x=0, y=self.phase})
			phasearmor = {
				[0]={cracky=3},
				[1]={crumbly=3},
				[2]={fleshy=3}
			}
			self.object:set_armor_groups(phasearmor[self.phase])
		end
	end,

	on_punch = function(self, hitter)
	end,
})
