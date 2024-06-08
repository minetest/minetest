local function register_entity(name, textures)
	minetest.register_entity("gltf:" .. name, {
		initial_properties = {
			visual = "mesh",
			mesh = name .. ".gltf",
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
register_entity("snow_man", {"snow_man.png"})
register_entity("spider", {"spider.png"})
