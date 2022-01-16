core.register_chatcommand("set_lighting", {
    params = "<brightness> [color]",
    description = "Set lighting parameters. Brightness is a number from 0 to 1, color is an #RRGGBB color",
    func = function(player_name, param)
        local brightness, color_tint = string.match(param, "([^ ]+)(.*)$");
        if not brightness then
            return false, "Invalid input, see /help set_lighting"
        end
        color_tint = string.gsub(color_tint, " ", "")
        if color_tint == "" then color_tint = nil end
        minetest.get_player_by_name(player_name):set_lighting({brightness=tonumber(brightness), color_tint = color_tint})
    end
})