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
	minetest.registered_privileges[name] = def
end

minetest.register_privilege("interact", "Can interact with things and modify the world")
minetest.register_privilege("teleport", "Can use /teleport command")
minetest.register_privilege("bring", "Can teleport other players")
minetest.register_privilege("settime", "Can use /time")
minetest.register_privilege("privs", "Can modify privileges")
minetest.register_privilege("basic_privs", "Can modify 'shout' and 'interact' privileges")
minetest.register_privilege("server", "Can do server maintenance stuff")
minetest.register_privilege("shout", "Can speak in chat")
minetest.register_privilege("ban", "Can ban and unban players")
minetest.register_privilege("give", "Can use /give and /giveme")
minetest.register_privilege("password", "Can use /setpassword and /clearpassword")
minetest.register_privilege("fly", {
	description = "Can fly using the free_move mode",
	give_to_singleplayer = false,
})
minetest.register_privilege("fast", {
	description = "Can walk fast using the fast_move mode",
	give_to_singleplayer = false,
})
minetest.register_privilege("rollback", "Can use the rollback functionality")

