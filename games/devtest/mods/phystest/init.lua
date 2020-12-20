local cmds = {}

minetest.register_chatcommand("physics", {
	func = function(name, param)
		local player = minetest.get_player_by_name(name)
		if not player then
			return
		end

		local cmd = cmds[param:trim()]
		if cmd then
			cmd(player)
		end

		minetest.chat_send_player(name, dump(player:get_effective_physics()))
	end
})

function cmds.moon(player)
	player:set_physics_modifier("phystest:moon", {
		gravity = 0.16,
	})
end

function cmds.nomoon(player)
	player:delete_physics_modifier("phystest:moon")
end

function cmds.speed(player)
	player:set_physics_modifier("phystest:speed", {
		speed = 1.5,
	})
end

function cmds.nospeed(player)
	player:delete_physics_modifier("phystest:speed")
end

function cmds.potion(player)
	player:set_physics_modifier("phystest:potion", {
		type = "add",
		speed = 0.5,
	})
end

function cmds.nopotion(player)
	player:delete_physics_modifier("phystest:potion")
end

function cmds.jupiter(player)
	player:set_physics_modifier("phystest:jupiter", {
		gravity = 2.5,
	})
end

function cmds.nojupiter(player)
	player:delete_physics_modifier("phystest:jupiter")
end

function cmds.override(player)
	player:set_physics_override({
		speed = 2,
	})
end

function cmds.reset(player)
	player:set_physics_override(nil)
end

function cmds.flags(player)
	minetest.chat_send_player(player:get_player_name(), dump(player:get_physics_flags()))
end

function cmds.glitch(player)
	player:set_physics_flags({
		sneak_glitch = true
	})
end

function cmds.noglitch(player)
	player:set_physics_flags({
		sneak_glitch = false
	})
end

function cmds.invalid(player)
	player:set_physics_modifier("phystest:invalid", {
		type = "BLARGH",
		speed = 1.5,
	})
end

function cmds.dep(player)
	player:set_physics_override({
		sneak_glitch = true
	})
end

function cmds.dep2(player)
	player:set_physics_override(2, 3, 0.16)
end
