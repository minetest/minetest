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
-- Note: Model has an animation, but we can use it as a static test nevertheless
register_entity("frog", {"gltf_frog.png"})

minetest.register_node("gltf:frog", {
	description = "glTF frog, but it's a node",
	tiles = {"gltf_frog.png"},
	drawtype = "mesh",
	mesh = "gltf_frog.gltf",
})

minetest.register_chatcommand("show_model", {
	params = "<model> [textures]",
	description = "Show a model (defaults to gltf models, for example '/show_model frog').",
	func = function(name, param)
		local model, textures = param:match"^(.-)%s+(.+)$"
		if not model then
			model = "gltf_" .. param .. ".gltf"
			textures = "gltf_" .. param .. ".png"
		end
		minetest.show_formspec(name, "gltf:model", table.concat{
			"formspec_version[7]",
			"size[10,10]",
			"model[0,0;10,10;model;", model, ";", textures, ";0,0;true;true;0,0;0]",
		})
	end,
})