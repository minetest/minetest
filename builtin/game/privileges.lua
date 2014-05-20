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
core.register_privilege("teleport", "Can use /teleport command")
core.register_privilege("bring", "Can teleport other players")
core.register_privilege("settime", "Can use /time")
core.register_privilege("privs", "Can modify privileges")
core.register_privilege("basic_privs", "Can modify 'shout' and 'interact' privileges")
core.register_privilege("server", "Can do server maintenance stuff")
core.register_privilege("shout", "Can speak in chat")
core.register_privilege("ban", "Can ban and unban players")
core.register_privilege("kick", "Can kick players")
core.register_privilege("give", "Can use /give and /giveme")
core.register_privilege("password", "Can use /setpassword and /clearpassword")
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
core.register_privilege("rollback", "Can use the rollback functionality")

