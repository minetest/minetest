-- Minetest: builtin/features.lua

minetest.features = {
	glasslike_framed = true,
	nodebox_as_selectionbox = true,
	chat_send_player_param3 = true,
	get_all_craft_recipes_works = true,
	use_texture_alpha = true,
	no_legacy_abms = true,
}

function minetest.has_feature(arg)
	if type(arg) == "table" then
		missing_features = {}
		result = true
		for ft, _ in pairs(arg) do
			if not minetest.features[ftr] then
				missing_features[ftr] = true
				result = false
			end
		end
		return result, missing_features
	elseif type(arg) == "string" then
		if not minetest.features[arg] then
			return false, {[arg]=true}
		end
		return true, {}
	end
end
