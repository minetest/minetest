-- Minimal test entities to test visuals

core.register_entity("testentities:sprite", {
	initial_properties = {
		visual = "sprite",
		textures = { "testentities_sprite.png" },
	},
})

core.register_entity("testentities:upright_sprite", {
	initial_properties = {
		visual = "upright_sprite",
		textures = {
			"testentities_upright_sprite1.png",
			"testentities_upright_sprite2.png",
		},
	},
})

core.register_entity("testentities:cube", {
	initial_properties = {
		visual = "cube",
		textures = {
			"testentities_cube1.png",
			"testentities_cube2.png",
			"testentities_cube3.png",
			"testentities_cube4.png",
			"testentities_cube5.png",
			"testentities_cube6.png",
		},
	},
})

core.register_entity("testentities:item", {
	initial_properties = {
		visual = "item",
		wield_item = "testnodes:normal",
	},
})

core.register_entity("testentities:wielditem", {
	initial_properties = {
		visual = "wielditem",
		wield_item = "testnodes:normal",
	},
})

core.register_entity("testentities:mesh", {
	initial_properties = {
		visual = "mesh",
		mesh = "testnodes_pyramid.obj",
		textures = {
			"testnodes_mesh_stripes2.png"
		},
	},
})

core.register_entity("testentities:mesh_unshaded", {
	initial_properties = {
		visual = "mesh",
		mesh = "testnodes_pyramid.obj",
		textures = {
			"testnodes_mesh_stripes2.png"
		},
		shaded = false,
	},
})

core.register_entity("testentities:sam", {
	initial_properties = {
		visual = "mesh",
		mesh = "testentities_sam.b3d",
		textures = {
			"testentities_sam.png"
		},
	},
	on_activate = function(self)
		self.object:set_animation({x = 0, y = 219}, 30, 0, true)
	end,
})

core.register_entity("testentities:lava_flan", {
	initial_properties = {
		infotext = "Lava Flan (smoke test .x)",
		visual = "mesh",
		mesh = "testentities_lava_flan.x",
		textures = {
			"testentities_lava_flan.png"
		},
	},
	on_activate = function(self)
		self.object:set_animation({x = 0, y = 28}, 15, 0, true)
	end,
})

-- Advanced visual tests

-- An entity for testing animated and yaw-modulated sprites
core.register_entity("testentities:yawsprite", {
	initial_properties = {
		selectionbox = {-0.3, -0.5, -0.3, 0.3, 0.3, 0.3},
		visual = "sprite",
		visual_size = {x=0.6666, y=1},
		textures = {"testentities_dungeon_master.png^[makealpha:128,0,0^[makealpha:128,128,0"},
		spritediv = {x=6, y=5},
		initial_sprite_basepos = {x=0, y=0},
	},
	on_activate = function(self, staticdata)
		self.object:set_sprite({x=0, y=0}, 3, 0.5, true)
	end,
})

-- An entity for testing animated upright sprites
core.register_entity("testentities:upright_animated", {
	initial_properties = {
		visual = "upright_sprite",
		textures = {"testnodes_anim.png"},
		spritediv = {x = 1, y = 4},
	},
	on_activate = function(self)
		self.object:set_sprite({x=0, y=0}, 4, 1.0, false)
	end,
})

core.register_entity("testentities:nametag", {
	initial_properties = {
		visual = "sprite",
		textures = { "testentities_sprite.png" },
	},

	on_activate = function(self, staticdata)
		if staticdata ~= "" then
			local data = core.deserialize(staticdata)
			self.color = data.color
			self.bgcolor = data.bgcolor
		else
			self.color = {
				r = math.random(0, 255),
				g = math.random(0, 255),
				b = math.random(0, 255),
			}

			if math.random(0, 10) > 5 then
				self.bgcolor = {
					r = math.random(0, 255),
					g = math.random(0, 255),
					b = math.random(0, 255),
					a = math.random(0, 255),
				}
			end
		end

		assert(self.color)
		self.object:set_properties({
			nametag = tostring(math.random(1000, 10000)),
			nametag_color = self.color,
			nametag_bgcolor = self.bgcolor,
		})
	end,

	get_staticdata = function(self)
		return core.serialize({ color = self.color, bgcolor = self.bgcolor })
	end,
})

-- Enriched nametag strings
do

local bold = minetest.get_font_escape_sequence("bold")
local unbold = minetest.get_font_escape_sequence("unbold")
local italic = minetest.get_font_escape_sequence("italic")
local unitalic = minetest.get_font_escape_sequence("unitalic")
local mono = minetest.get_font_escape_sequence("mono")
local unmono = minetest.get_font_escape_sequence("unmono")
local all_off = unbold..unitalic..unmono

local nametags = {
	"normal "..bold.."bold "..unbold..italic.."italic "..unitalic..mono.."mono",
	bold..italic.."bold+italic"..unitalic..mono.." bold+mono"..unbold..italic.." mono+italic",
	bold..italic..mono.."bold+mono+italic",
	bold.."bold colored"..minetest.colorize("red", " red").." text",
	"colored"..minetest.colorize("red", " red"..bold.." bold"..unbold).." text",
	minetest.strip_font(bold.."striped"..italic.." font fomats"),
	minetest.get_background_escape_sequence("green").."background is not supported",
	"colored"..minetest.colorize("gray", " gray"..bold.." bold multi\nline").." text",
	bold..unbold.."\nempty lines"..italic.." before and after\n"..unitalic,
	"m"..mono.."m"..unmono.."m"..mono.."m"..unmono.."m"..mono.."m"..unmono.."m"..mono.."m",
	"i"..italic.."i"..unitalic.."i"..italic.."i"..unitalic.."i"..italic.."i"..unitalic.."i"..
		italic.."i",
	"b"..bold.."b"..unbold.."b"..bold.."b"..unbold.."b"..bold.."b"..unbold.."b"..bold.."b",
}

-- Remove this if you want to look them separately
local all_lines = "Enriched nametag text:"
for _, nametag in ipairs(nametags) do
	all_lines = all_lines.."\n"..all_off..nametag
end
nametags = {all_lines}

local nametag_iterator = 0
minetest.register_entity("testentities:nametag_enriched_text", {
	initial_properties = {
		visual = "sprite",
		textures = { "testentities_sprite.png" },
	},

	on_activate = function(self, staticdata)
		local color
		local bgcolor
		if (nametag_iterator/#nametags) % 2 < 1 then
			color = {r = 0, g = 0, b = 255}
			bgcolor = {r = 200, g = 200, b = 10,a = 150}
		end

		local nametag = nametags[(nametag_iterator % #nametags)+1]
		nametag_iterator = nametag_iterator + 1

		self.object:set_properties({
			nametag = nametag,
			nametag_color = color,
			nametag_bgcolor = bgcolor,
		})
	end,

})

-- Who doesn't like ascii art
minetest.register_entity("testentities:nametag_ascii_art", {
	initial_properties = {
		visual = "sprite",
		textures = { "testentities_sprite.png" },
	},
	on_activate = function(self, staticdata)
		local nametag = mono..
			"         __.               __.                 __.  \n" ..
			"  _____ |__| ____   _____ /  |_  _____  _____ /  |_ \n" ..
			" /     \\|  |/    \\ /  __ \\    _\\/  __ \\/   __>    _\\\n" ..
			"|  Y Y  \\  |   |  \\   ___/|  | |   ___/\\___  \\|  |  \n" ..
			"|__|_|  /  |___|  /\\______>  |  \\______>_____/|  |  \n" ..
			"      \\/ \\/     \\/         \\/                  \\/   "
		self.object:set_properties({
			nametag = nametag,
			nametag_color = "green",
			nametag_bgcolor = "black",
		})
	end,

})

end
