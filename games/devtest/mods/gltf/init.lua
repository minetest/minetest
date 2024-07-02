local function register_entity(name, textures)
	minetest.register_entity("gltf:" .. name, {
		initial_properties = {
			visual = "mesh",
			mesh = "gltf_" .. name .. ".gltf",
			textures = textures,
		},
	})
end

-- These do not have texture coordinates; they simple render as black surfaces.
register_entity("minimal_triangle", {})
register_entity("triangle_with_vertex_stride", {})
register_entity("triangle_without_indices", {})
do
	local cube_textures = {"no_texture.png"} -- TODO provide proper textures
	register_entity("blender_cube", cube_textures)
	register_entity("blender_cube_scaled", cube_textures)
	register_entity("blender_cube_matrix_transform", cube_textures)
end

register_entity("snow_man", {"gltf_snow_man.png"})
register_entity("spider", {"gltf_spider.png"})

minetest.register_entity("gltf:spider_animated", {
	initial_properties = {
		visual = "mesh",
		mesh = "gltf_spider_animated.gltf",
		textures = {"gltf_spider.png"},
	},
	on_activate = function(self)
		self.object:set_animation({x = 0, y = 140}, 1)
	end
})

minetest.register_entity("gltf:simple_skin", {
	initial_properties = {
		visual = "mesh",
		visual_size = vector.new(5, 5, 5),
		mesh = "gltf_simple_skin.gltf",
		textures = {},
		backface_culling = false
	},
	on_activate = function(self)
		self.object:set_animation({x = 0, y = 5.5}, 1)
	end
})
