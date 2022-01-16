core.register_chatcommand("set_lighting", {
    params = "",
    description = "Set lighting parameters.",
    func = function(player_name, param)
        minetest.get_player_by_name(player_name):set_lighting({})
    end
})