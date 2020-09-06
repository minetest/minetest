-- Armorball: Test entity for testing armor groups
-- Rightclick to change armor group

local phasearmor = {
	[0]={icy=100},
	[1]={fiery=100},
	[2]={fleshy=100},
	[3]={immortal=1},
	[4]={punch_operable=1},
}

minetest.register_entity("testentities:armorball", {
	initial_properties = {
		hp_max = 20,
		physical = false,
		collisionbox = {-0.4,-0.4,-0.4, 0.4,0.4,0.4},
		visual = "sprite",
		visual_size = {x=1, y=1},
		textures = {"testentities_armorball.png"},
		spritediv = {x=1, y=5},
		initial_sprite_basepos = {x=0, y=0},
	},

	_phase = 2,

	on_activate = function(self, staticdata)
		minetest.log("action", "[testentities] armorball.on_activate")
		self.object:set_armor_groups(phasearmor[self._phase])
		self.object:set_sprite({x=0, y=self._phase})
	end,

	on_rightclick = function(self, clicker)
		-- Change armor group and sprite
		self._phase = self._phase + 1
		if self._phase >= 5 then
			self._phase = 0
		end
		self.object:set_sprite({x=0, y=self._phase})
		self.object:set_armor_groups(phasearmor[self._phase])
	end,
})
