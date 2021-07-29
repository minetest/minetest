-- Node texture tests

local S = minetest.get_translator("testnodes")

minetest.register_node("testnodes:6sides", {
	description = S("Six Textures Test Node"),
	tiles = {
		"testnodes_normal1.png",
		"testnodes_normal2.png",
		"testnodes_normal3.png",
		"testnodes_normal4.png",
		"testnodes_normal5.png",
		"testnodes_normal6.png",
	},

	groups = { dig_immediate = 2 },
})

minetest.register_node("testnodes:anim", {
	description = S("Animated Test Node"),
	tiles = {
		{ name = "testnodes_anim.png",
		animation = {
			type = "vertical_frames",
			aspect_w = 16,
			aspect_h = 16,
			length = 4.0,
		}, },
	},

	groups = { dig_immediate = 2 },
})

-- Node texture transparency test

local alphas = { 64, 128, 191 }

for a=1,#alphas do
	local alpha = alphas[a]

	-- Transparency taken from texture
	minetest.register_node("testnodes:alpha_texture_"..alpha, {
		description = S("Texture Alpha Test Node (@1)", alpha),
		drawtype = "glasslike",
		paramtype = "light",
		tiles = {
			"testnodes_alpha"..alpha..".png",
		},
		use_texture_alpha = "blend",

		groups = { dig_immediate = 3 },
	})

	-- Transparency set via texture modifier
	minetest.register_node("testnodes:alpha_"..alpha, {
		description = S("Alpha Test Node (@1)", alpha),
		drawtype = "glasslike",
		paramtype = "light",
		tiles = {
			"testnodes_alpha.png^[opacity:" .. alpha,
		},
		use_texture_alpha = "blend",

		groups = { dig_immediate = 3 },
	})
end

-- Generate PNG textures

local function mandelbrot(w, h, iterations)
	local r = {}
	for y=0, h-1 do
		for x=0, w-1 do
			local re = (x - w/2) * 4/w
			local im = (y - h/2) * 4/h
			-- zoom in on a nice view
			re = re / 128 - 0.23
			im = im / 128 - 0.82

			local px, py = 0, 0
			local i = 0
			while px*px + py*py <= 4 and i < iterations do
				px, py = px*px - py*py + re, 2 * px * py + im
				i = i + 1
			end
			r[w*y+x+1] = i / iterations
		end
	end
	return r
end

local function gen_checkers(w, h, tile)
	local r = {}
	for y=0, h-1 do
		for x=0, w-1 do
			local hori = math.floor(x / tile) % 2 == 0
			local vert = math.floor(y / tile) % 2 == 0
			r[w*y+x+1] = hori ~= vert and 1 or 0
		end
	end
	return r
end

local fractal = mandelbrot(512, 512, 128)
local frac_emb = mandelbrot(64, 64, 64)
local checker = gen_checkers(512, 512, 32)

local floor = math.floor
local abs = math.abs
local data_emb = {}
local data_mb = {}
local data_ck = {}
for i=1, #frac_emb do
	data_emb[i] = {
		r = floor(abs(frac_emb[i] * 2 - 1) * 255),
		g = floor(abs(1 - frac_emb[i]) * 255),
		b = floor(frac_emb[i] * 255),
		a = frac_emb[i] < 0.95 and 255 or 0,
	}
end
for i=1, #fractal do
	data_mb[i] = {
		r = floor(fractal[i] * 255),
		g = floor(abs(fractal[i] * 2 - 1) * 255),
		b = floor(abs(1 - fractal[i]) * 255),
		a = 255,
	}
	data_ck[i] = checker[i] > 0 and "#F80" or "#000"
end

local textures_path = minetest.get_modpath( minetest.get_current_modname() ) .. "/textures/"
minetest.safe_file_write(
	textures_path .. "testnodes_generated_mb.png",
	minetest.encode_png(512,512,data_mb)
)
minetest.safe_file_write(
	textures_path .. "testnodes_generated_ck.png",
	minetest.encode_png(512,512,data_ck)
)

minetest.register_node("testnodes:generated_png_mb", {
	description = S("Generated Mandelbrot PNG Test Node"),
	tiles = { "testnodes_generated_mb.png" },

	groups = { dig_immediate = 2 },
})
minetest.register_node("testnodes:generated_png_ck", {
	description = S("Generated Checker PNG Test Node"),
	tiles = { "testnodes_generated_ck.png" },

	groups = { dig_immediate = 2 },
})

local png_emb = "[png:" .. minetest.encode_base64(minetest.encode_png(64,64,data_emb))

minetest.register_node("testnodes:generated_png_emb", {
	description = S("Generated In-Band Mandelbrot PNG Test Node"),
	tiles = { png_emb },

	groups = { dig_immediate = 2 },
})
minetest.register_node("testnodes:generated_png_src_emb", {
	description = S("Generated In-Band Source Blit Mandelbrot PNG Test Node"),
	tiles = { png_emb .. "^testnodes_damage_neg.png" },

	groups = { dig_immediate = 2 },
})
minetest.register_node("testnodes:generated_png_dst_emb", {
	description = S("Generated In-Band Dest Blit Mandelbrot PNG Test Node"),
	tiles = { "testnodes_generated_ck.png^" .. png_emb },

	groups = { dig_immediate = 2 },
})
