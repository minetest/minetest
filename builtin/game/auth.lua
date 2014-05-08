-- Minetest: builtin/auth.lua

--
-- Authentication handler
--

function core.string_to_privs(str, delim)
	assert(type(str) == "string")
	delim = delim or ','
	privs = {}
	for _, priv in pairs(string.split(str, delim)) do
		privs[priv:trim()] = true
	end
	return privs
end

function core.privs_to_string(privs, delim)
	assert(type(privs) == "table")
	delim = delim or ','
	list = {}
	for priv, bool in pairs(privs) do
		if bool then
			table.insert(list, priv)
		end
	end
	return table.concat(list, delim)
end

assert(core.string_to_privs("a,b").b == true)
assert(core.privs_to_string({a=true,b=true}) == "a,b")

core.auth_file_path = core.get_worldpath().."/auth.txt"
core.auth_table = {}

local function read_auth_file()
	local newtable = {}
	local file, errmsg = io.open(core.auth_file_path, 'rb')
	if not file then
		core.log("info", core.auth_file_path.." could not be opened for reading ("..errmsg.."); assuming new world")
		return
	end
	for line in file:lines() do
		if line ~= "" then
			local name, password, privilegestring = string.match(line, "([^:]*):([^:]*):([^:]*)")
			if not name or not password or not privilegestring then
				error("Invalid line in auth.txt: "..dump(line))
			end
			local privileges = core.string_to_privs(privilegestring)
			newtable[name] = {password=password, privileges=privileges}
		end
	end
	io.close(file)
	core.auth_table = newtable
	core.notify_authentication_modified()
end

local function save_auth_file()
	local newtable = {}
	-- Check table for validness before attempting to save
	for name, stuff in pairs(core.auth_table) do
		assert(type(name) == "string")
		assert(name ~= "")
		assert(type(stuff) == "table")
		assert(type(stuff.password) == "string")
		assert(type(stuff.privileges) == "table")
	end
	local file, errmsg = io.open(core.auth_file_path, 'w+b')
	if not file then
		error(core.auth_file_path.." could not be opened for writing: "..errmsg)
	end
	for name, stuff in pairs(core.auth_table) do
		local privstring = core.privs_to_string(stuff.privileges)
		file:write(name..":"..stuff.password..":"..privstring..'\n')
	end
	io.close(file)
end

read_auth_file()

core.builtin_auth_handler = {
	get_auth = function(name)
		assert(type(name) == "string")
		-- Figure out what password to use for a new player (singleplayer
		-- always has an empty password, otherwise use default, which is
		-- usually empty too)
		local new_password_hash = ""
		-- If not in authentication table, return nil
		if not core.auth_table[name] then
			return nil
		end
		-- Figure out what privileges the player should have.
		-- Take a copy of the privilege table
		local privileges = {}
		for priv, _ in pairs(core.auth_table[name].privileges) do
			privileges[priv] = true
		end
		-- If singleplayer, give all privileges except those marked as give_to_singleplayer = false
		if core.is_singleplayer() then
			for priv, def in pairs(core.registered_privileges) do
				if def.give_to_singleplayer then
					privileges[priv] = true
				end
			end
		-- For the admin, give everything
		elseif name == core.setting_get("name") then
			for priv, def in pairs(core.registered_privileges) do
				privileges[priv] = true
			end
		end
		-- All done
		return {
			password = core.auth_table[name].password,
			privileges = privileges,
		}
	end,
	create_auth = function(name, password)
		assert(type(name) == "string")
		assert(type(password) == "string")
		core.log('info', "Built-in authentication handler adding player '"..name.."'")
		core.auth_table[name] = {
			password = password,
			privileges = core.string_to_privs(core.setting_get("default_privs")),
		}
		save_auth_file()
	end,
	set_password = function(name, password)
		assert(type(name) == "string")
		assert(type(password) == "string")
		if not core.auth_table[name] then
			core.builtin_auth_handler.create_auth(name, password)
		else
			core.log('info', "Built-in authentication handler setting password of player '"..name.."'")
			core.auth_table[name].password = password
			save_auth_file()
		end
		return true
	end,
	set_privileges = function(name, privileges)
		assert(type(name) == "string")
		assert(type(privileges) == "table")
		if not core.auth_table[name] then
			core.builtin_auth_handler.create_auth(name, core.get_password_hash(name, core.setting_get("default_password")))
		end
		core.auth_table[name].privileges = privileges
		core.notify_authentication_modified(name)
		save_auth_file()
	end,
	reload = function()
		read_auth_file()
		return true
	end,
}

function core.register_authentication_handler(handler)
	if core.registered_auth_handler then
		error("Add-on authentication handler already registered by "..core.registered_auth_handler_modname)
	end
	core.registered_auth_handler = handler
	core.registered_auth_handler_modname = core.get_current_modname()
end

function core.get_auth_handler()
	if core.registered_auth_handler then
		return core.registered_auth_handler
	end
	return core.builtin_auth_handler
end

function core.set_player_password(name, password)
	if core.get_auth_handler().set_password then
		core.get_auth_handler().set_password(name, password)
	end
end

function core.set_player_privs(name, privs)
	if core.get_auth_handler().set_privileges then
		core.get_auth_handler().set_privileges(name, privs)
	end
end

function core.auth_reload()
	if core.get_auth_handler().reload then
		return core.get_auth_handler().reload()
	end
	return false
end


