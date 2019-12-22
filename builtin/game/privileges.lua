-- Minetest: builtin/privileges.lua

--
-- Privileges
--

minetest.registered_privileges = {}

function minetest.register_privilege(name, param)
	local function fill_defaults(def)
		if def.give_to_singleplayer == nil then
			def.give_to_singleplayer = true
		end
		if def.give_to_admin == nil then
			def.give_to_admin = def.give_to_singleplayer
		end
		if def.description == nil then
			def.description = "(no description)"
		end
	end
	local def
	if type(param) == "table" then
		def = param
	else
		def = {description = param}
	end
	fill_defaults(def)
	minetest.registered_privileges[name] = def
end

minetest.register_privilege("interact", "Can interact with things and modify the world")
minetest.register_privilege("shout", "Can speak in chat")
minetest.register_privilege("basic_privs", "Can modify 'shout' and 'interact' privileges")
minetest.register_privilege("privs", "Can modify privileges")

minetest.register_privilege("teleport", {
	description = "Can teleport self",
	give_to_singleplayer = false,
})
minetest.register_privilege("bring", {
	description = "Can teleport other players",
	give_to_singleplayer = false,
})
minetest.register_privilege("settime", {
	description = "Can set the time of day using /time",
	give_to_singleplayer = false,
})
minetest.register_privilege("server", {
	description = "Can do server maintenance stuff",
	give_to_singleplayer = false,
	give_to_admin = true,
})
minetest.register_privilege("protection_bypass", {
	description = "Can bypass node protection in the world",
	give_to_singleplayer = false,
})
minetest.register_privilege("ban", {
	description = "Can ban and unban players",
	give_to_singleplayer = false,
	give_to_admin = true,
})
minetest.register_privilege("kick", {
	description = "Can kick players",
	give_to_singleplayer = false,
	give_to_admin = true,
})
minetest.register_privilege("give", {
	description = "Can use /give and /giveme",
	give_to_singleplayer = false,
})
minetest.register_privilege("password", {
	description = "Can use /setpassword and /clearpassword",
	give_to_singleplayer = false,
	give_to_admin = true,
})
minetest.register_privilege("fly", {
	description = "Can use fly mode",
	give_to_singleplayer = false,
})
minetest.register_privilege("fast", {
	description = "Can use fast mode",
	give_to_singleplayer = false,
})
minetest.register_privilege("noclip", {
	description = "Can fly through solid nodes using noclip mode",
	give_to_singleplayer = false,
})
minetest.register_privilege("rollback", {
	description = "Can use the rollback functionality",
	give_to_singleplayer = false,
})
minetest.register_privilege("debug", {
	description = "Allows enabling various debug options that may affect gameplay",
	give_to_singleplayer = false,
	give_to_admin = true,
})

minetest.register_can_bypass_userlimit(function(name, ip)
	local privs = minetest.get_player_privs(name)
	return privs["server"] or privs["ban"] or privs["privs"] or privs["password"]
end)
