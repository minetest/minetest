local function register_entity(name, textures, backface_culling)
	minetest.register_entity("gltf:" .. name, {
		initial_properties = {
			visual = "mesh",
			mesh = "gltf_" .. name .. ".gltf",
			textures = textures,
			backface_culling = backface_culling,
		},
	})
end

-- These do not have texture coordinates; they simple render as black surfaces.
register_entity("minimal_triangle", {}, false)
register_entity("triangle_with_vertex_stride", {}, false)
register_entity("triangle_without_indices", {}, false)
do
	local cube_textures = {"gltf_cube.png"}
	register_entity("blender_cube", cube_textures)
	register_entity("blender_cube_scaled", cube_textures)
	register_entity("blender_cube_matrix_transform", cube_textures)
end
register_entity("snow_man", {"gltf_snow_man.png"})
register_entity("spider", {"gltf_spider.png"})
-- Note: Model has an animation, but we can use it as a static test nevertheless
-- The claws rendering incorrectly from one side is expected behavior:
-- They use an unsupported double-sided material.
register_entity("frog", {"gltf_frog.png"}, false)

minetest.register_node("gltf:frog", {
	description = "glTF frog, but it's a node",
	tiles = {{name = "gltf_frog.png", backface_culling = false}},
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
