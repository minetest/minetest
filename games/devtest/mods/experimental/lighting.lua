core.register_chatcommand("set_lighting", {
    params = "shadow_intensity",
    description = "Set lighting parameters.",
    func = function(player_name, param)
        local shadow_intensity = tonumber(param)
        minetest.get_player_by_name(player_name):set_lighting({shadows = { intensity = shadow_intensity} })
    end
})