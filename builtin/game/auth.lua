-- Minetest: builtin/auth.lua

--
-- Builtin authentication handler
--

local auth_file_path = core.get_worldpath().."/auth.txt"
local auth_table = {}

local function read_auth_file()
	local newtable = {}
	local file, errmsg = io.open(auth_file_path, 'rb')
	if not file then
		core.log("info", auth_file_path.." could not be opened for reading ("..errmsg.."); assuming new world")
		return
	end
	for line in file:lines() do
		if line ~= "" then
			local fields = line:split(":", true)
			local name, password, privilege_string, last_login = unpack(fields)
			last_login = tonumber(last_login)
			if not (name and password and privilege_string) then
				error("Invalid line in auth.txt: "..dump(line))
			end
			local privileges = core.string_to_privs(privilege_string)
			newtable[name] = {password=password, privileges=privileges, last_login=last_login}
		end
	end
	io.close(file)
	auth_table = newtable
	core.notify_authentication_modified()
end

local function save_auth_file()
	local newtable = {}
	-- Check table for validness before attempting to save
	for name, stuff in pairs(auth_table) do
		assert(type(name) == "string")
		assert(name ~= "")
		assert(type(stuff) == "table")
		assert(type(stuff.password) == "string")
		assert(type(stuff.privileges) == "table")
		assert(stuff.last_login == nil or type(stuff.last_login) == "number")
	end
	local content = {}
	for name, stuff in pairs(auth_table) do
		local priv_string = core.privs_to_string(stuff.privileges)
		local parts = {name, stuff.password, priv_string, stuff.last_login or ""}
		content[#content + 1] = table.concat(parts, ":")
	end
	if not core.safe_file_write(auth_file_path, table.concat(content, "\n")) then
		error(auth_file_path.." could not be written to")
	end
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
		if not auth_table[name] then
			return nil
		end
		-- Figure out what privileges the player should have.
		-- Take a copy of the privilege table
		local privileges = {}
		for priv, _ in pairs(auth_table[name].privileges) do
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
		elseif name == core.settings:get("name") then
			for priv, def in pairs(core.registered_privileges) do
				if def.give_to_admin then
					privileges[priv] = true
				end
			end
		end
		-- All done
		return {
			password = auth_table[name].password,
			privileges = privileges,
			-- Is set to nil if unknown
			last_login = auth_table[name].last_login,
		}
	end,
	create_auth = function(name, password)
		assert(type(name) == "string")
		assert(type(password) == "string")
		core.log('info', "Built-in authentication handler adding player '"..name.."'")
		auth_table[name] = {
			password = password,
			privileges = core.string_to_privs(core.settings:get("default_privs")),
			last_login = os.time(),
		}
		save_auth_file()
	end,
	delete_auth = function(name)
		assert(type(name) == "string")
		if not auth_table[name] then
			return false
		end
		core.log('info', "Built-in authentication handler deleting player '"..name.."'")
		auth_table[name] = nil
		save_auth_file()
		return true
	end,
	set_password = function(name, password)
		assert(type(name) == "string")
		assert(type(password) == "string")
		if not auth_table[name] then
			core.builtin_auth_handler.create_auth(name, password)
		else
			core.log('info', "Built-in authentication handler setting password of player '"..name.."'")
			auth_table[name].password = password
			save_auth_file()
		end
		return true
	end,
	set_privileges = function(name, privileges)
		assert(type(name) == "string")
		assert(type(privileges) == "table")
		if not auth_table[name] then
			core.builtin_auth_handler.create_auth(name,
				core.get_password_hash(name,
					core.settings:get("default_password")))
		end

		-- Run grant callbacks
		for priv, _ in pairs(privileges) do
			if not auth_table[name].privileges[priv] then
				core.run_priv_callbacks(name, priv, nil, "grant")
			end
		end

		-- Run revoke callbacks
		for priv, _ in pairs(auth_table[name].privileges) do
			if not privileges[priv] then
				core.run_priv_callbacks(name, priv, nil, "revoke")
			end
		end

		auth_table[name].privileges = privileges
		core.notify_authentication_modified(name)
		save_auth_file()
	end,
	reload = function()
		read_auth_file()
		return true
	end,
	record_login = function(name)
		assert(type(name) == "string")
		assert(auth_table[name]).last_login = os.time()
		save_auth_file()
	end,
	iterate = function()
		local names = {}
		for k in pairs(auth_table) do
			names[k] = true
		end
		return pairs(names)
	end,
}

core.register_on_prejoinplayer(function(name, ip)
	if core.registered_auth_handler ~= nil then
		return -- Don't do anything if custom auth handler registered
	end
	if auth_table[name] ~= nil then
		return
	end

	local name_lower = name:lower()
	for k in pairs(auth_table) do
		if k:lower() == name_lower then
			return string.format("\nCannot create new player called '%s'. "..
					"Another account called '%s' is already registered. "..
					"Please check the spelling if it's your account "..
					"or use a different nickname.", name, k)
		end
	end
end)

--
-- Authentication API
--

function core.register_authentication_handler(handler)
	if core.registered_auth_handler then
		error("Add-on authentication handler already registered by "..core.registered_auth_handler_modname)
	end
	core.registered_auth_handler = handler
	core.registered_auth_handler_modname = core.get_current_modname()
	handler.mod_origin = core.registered_auth_handler_modname
end

function core.get_auth_handler()
	return core.registered_auth_handler or core.builtin_auth_handler
end

local function auth_pass(name)
	return function(...)
		local auth_handler = core.get_auth_handler()
		if auth_handler[name] then
			return auth_handler[name](...)
		end
		return false
	end
end

core.set_player_password = auth_pass("set_password")
core.set_player_privs    = auth_pass("set_privileges")
core.remove_player_auth  = auth_pass("delete_auth")
core.auth_reload         = auth_pass("reload")

local record_login = auth_pass("record_login")
core.register_on_joinplayer(function(player)
	record_login(player:get_player_name())
end)
