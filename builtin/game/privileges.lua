-- Minetest: builtin/privileges.lua

--
-- Privileges
--

core.registered_privileges = {}

function core.register_privilege(name, param)
	local function fill_defaults(def)
		if def.give_to_singleplayer == nil then
			def.give_to_singleplayer = true
		end
		if def.description == nil then
			def.description = "(no description)"
		end
	end
	local def = {}
	if type(param) == "table" then
		def = param
	else
		def = {description = param}
	end
	fill_defaults(def)
	core.registered_privileges[name] = def
end

core.register_privilege("interact", "Can interact with things and modify the world")
core.register_privilege("shout", "Can speak in chat")
core.register_privilege("basic_privs", "Can modify 'shout' and 'interact' privileges")
core.register_privilege("privs", "Can modify privileges")

core.register_privilege("teleport", {
	description = "Can use /teleport command",
	give_to_singleplayer = false,
})
core.register_privilege("bring", {
	description = "Can teleport other players",
	give_to_singleplayer = false,
})
core.register_privilege("settime", {
	description = "Can use /time",
	give_to_singleplayer = false,
})
core.register_privilege("server", {
	description = "Can do server maintenance stuff",
	give_to_singleplayer = false,
})
core.register_privilege("protection_bypass", {
	description = "Can bypass node protection in the world",
	give_to_singleplayer = false,
})
core.register_privilege("ban", {
	description = "Can ban and unban players",
	give_to_singleplayer = false,
})
core.register_privilege("kick", {
	description = "Can kick players",
	give_to_singleplayer = false,
})
core.register_privilege("give", {
	description = "Can use /give and /giveme",
	give_to_singleplayer = false,
})
core.register_privilege("password", {
	description = "Can use /setpassword and /clearpassword",
	give_to_singleplayer = false,
})
core.register_privilege("fly", {
	description = "Can fly using the free_move mode",
	give_to_singleplayer = false,
})
core.register_privilege("fast", {
	description = "Can walk fast using the fast_move mode",
	give_to_singleplayer = false,
})
core.register_privilege("noclip", {
	description = "Can fly through walls",
	give_to_singleplayer = false,
})
core.register_privilege("rollback", {
	description = "Can use the rollback functionality",
	give_to_singleplayer = false,
})
core.register_privilege("zoom", {
	description = "Can zoom the camera",
	give_to_singleplayer = false,
})
core.register_privilege("debug", {
	description = "Allows enabling various debug options that may affect gameplay",
	give_to_singleplayer = false,
})
