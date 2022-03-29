minetest.register_on_joinplayer(function (player)
    if player.set_lighting then
        player:set_lighting({shadows={intensity = 0.3}})
    end
end)