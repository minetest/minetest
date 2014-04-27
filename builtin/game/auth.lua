-- Minetest: builtin/auth.lua

--
-- Authentication handler
--

function minetest.string_to_privs(str, delim)
	assert(type(str) == "string")
	delim = delim or ','
	privs = {}
	for _, priv in pairs(string.split(str, delim)) do
		privs[priv:trim()] = true
	end
	return privs
end

function minetest.privs_to_string(privs, delim)
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

assert(minetest.string_to_privs("a,b").b == true)
assert(minetest.privs_to_string({a=true,b=true}) == "a,b")

minetest.auth_file_path = minetest.get_worldpath().."/auth.txt"
minetest.auth_table = {}

local function read_auth_file()
	local newtable = {}
	local file, errmsg = io.open(minetest.auth_file_path, 'rb')
	if not file then
		minetest.log("info", minetest.auth_file_path.." could not be opened for reading ("..errmsg.."); assuming new world")
		return
	end
	for line in file:lines() do
		if line ~= "" then
			local name, password, privilegestring = string.match(line, "([^:]*):([^:]*):([^:]*)")
			if not name or not password or not privilegestring then
				error("Invalid line in auth.txt: "..dump(line))
			end
			local privileges = minetest.string_to_privs(privilegestring)
			newtable[name] = {password=password, privileges=privileges}
		end
	end
	io.close(file)
	minetest.auth_table = newtable
	minetest.notify_authentication_modified()
end

local function save_auth_file()
	local newtable = {}
	-- Check table for validness before attempting to save
	for name, stuff in pairs(minetest.auth_table) do
		assert(type(name) == "string")
		assert(name ~= "")
		assert(type(stuff) == "table")
		assert(type(stuff.password) == "string")
		assert(type(stuff.privileges) == "table")
	end
	local file, errmsg = io.open(minetest.auth_file_path, 'w+b')
	if not file then
		error(minetest.auth_file_path.." could not be opened for writing: "..errmsg)
	end
	for name, stuff in pairs(minetest.auth_table) do
		local privstring = minetest.privs_to_string(stuff.privileges)
		file:write(name..":"..stuff.password..":"..privstring..'\n')
	end
	io.close(file)
end

read_auth_file()

minetest.builtin_auth_handler = {
	get_auth = function(name)
		assert(type(name) == "string")
		-- Figure out what password to use for a new player (singleplayer
		-- always has an empty password, otherwise use default, which is
		-- usually empty too)
		local new_password_hash = ""
		-- If not in authentication table, return nil
		if not minetest.auth_table[name] then
			return nil
		end
		-- Figure out what privileges the player should have.
		-- Take a copy of the privilege table
		local privileges = {}
		for priv, _ in pairs(minetest.auth_table[name].privileges) do
			privileges[priv] = true
		end
		-- If singleplayer, give all privileges except those marked as give_to_singleplayer = false
		if minetest.is_singleplayer() then
			for priv, def in pairs(minetest.registered_privileges) do
				if def.give_to_singleplayer then
					privileges[priv] = true
				end
			end
		-- For the admin, give everything
		elseif name == minetest.setting_get("name") then
			for priv, def in pairs(minetest.registered_privileges) do
				privileges[priv] = true
			end
		end
		-- All done
		return {
			password = minetest.auth_table[name].password,
			privileges = privileges,
		}
	end,
	create_auth = function(name, password)
		assert(type(name) == "string")
		assert(type(password) == "string")
		minetest.log('info', "Built-in authentication handler adding player '"..name.."'")
		minetest.auth_table[name] = {
			password = password,
			privileges = minetest.string_to_privs(minetest.setting_get("default_privs")),
		}
		save_auth_file()
	end,
	set_password = function(name, password)
		assert(type(name) == "string")
		assert(type(password) == "string")
		if not minetest.auth_table[name] then
			minetest.builtin_auth_handler.create_auth(name, password)
		else
			minetest.log('info', "Built-in authentication handler setting password of player '"..name.."'")
			minetest.auth_table[name].password = password
			save_auth_file()
		end
		return true
	end,
	set_privileges = function(name, privileges)
		assert(type(name) == "string")
		assert(type(privileges) == "table")
		if not minetest.auth_table[name] then
			minetest.builtin_auth_handler.create_auth(name, minetest.get_password_hash(name, minetest.setting_get("default_password")))
		end
		minetest.auth_table[name].privileges = privileges
		minetest.notify_authentication_modified(name)
		save_auth_file()
	end,
	reload = function()
		read_auth_file()
		return true
	end,
}

function minetest.register_authentication_handler(handler)
	if minetest.registered_auth_handler then
		error("Add-on authentication handler already registered by "..minetest.registered_auth_handler_modname)
	end
	minetest.registered_auth_handler = handler
	minetest.registered_auth_handler_modname = minetest.get_current_modname()
end

function minetest.get_auth_handler()
	if minetest.registered_auth_handler then
		return minetest.registered_auth_handler
	end
	return minetest.builtin_auth_handler
end

function minetest.set_player_password(name, password)
	if minetest.get_auth_handler().set_password then
		minetest.get_auth_handler().set_password(name, password)
	end
end

function minetest.set_player_privs(name, privs)
	if minetest.get_auth_handler().set_privileges then
		minetest.get_auth_handler().set_privileges(name, privs)
	end
end

function minetest.auth_reload()
	if minetest.get_auth_handler().reload then
		return minetest.get_auth_handler().reload()
	end
	return false
end


