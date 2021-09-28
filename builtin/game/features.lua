-- Minetest: builtin/features.lua

core.features = {
	glasslike_framed = true,
	nodebox_as_selectionbox = true,
	get_all_craft_recipes_works = true,
	use_texture_alpha = true,
	no_legacy_abms = true,
	texture_names_parens = true,
	area_store_custom_ids = true,
	add_entity_with_staticdata = true,
	no_chat_message_prediction = true,
	object_use_texture_alpha = true,
	object_independent_selectionbox = true,
	httpfetch_binary_data = true,
	formspec_version_element = true,
	area_store_persistent_ids = true,
	pathfinder_works = true,
	object_step_has_moveresult = true,
	direct_velocity_on_players = true,
	use_texture_alpha_string_modes = true,
	degrotate_240_steps = true,
	abm_min_max_y = true,
	dynamic_add_media_table = true,
}

function core.has_feature(arg)
	if type(arg) == "table" then
		local missing_features = {}
		local result = true
		for ftr in pairs(arg) do
			if not core.features[ftr] then
				missing_features[ftr] = true
				result = false
			end
		end
		return result, missing_features
	elseif type(arg) == "string" then
		if not core.features[arg] then
			return false, {[arg]=true}
		end
		return true, {}
	end
end
