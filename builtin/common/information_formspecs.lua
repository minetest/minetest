local COLOR_BLUE = "#7AF"
local COLOR_GREEN = "#7F7"
local COLOR_GRAY = "#BBB"

local LIST_FORMSPEC = [[
		size[13,6.5]
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,5.5;list;%s;0]
		button_exit[5,6;3,1;quit;%s]
	]]

local LIST_FORMSPEC_DESCRIPTION = [[
		size[13,7.5]
		label[0,-0.1;%s]
		tablecolumns[color;tree;text;text]
		table[0,0.5;12.8,4.8;list;%s;%i]
		box[0,5.5;12.8,1.5;#000]
		textarea[0.3,5.5;13.05,1.9;;;%s]
		button_exit[5,7;3,1;quit;%s]
	]]

local LIST_FORMSPEC_MODSLIST = [[
        size[6,8]
        label[0,-0.1;%s]
        tablecolumns[color;tree;text]
        table[0,0.5;6,7;modslist;%s;0]
        button_exit[1.5,7.5;3,1;quit;%s]
    ]]

local formspec_escape = core.formspec_escape
local check_player_privs = core.check_player_privs


-- CHAT COMMANDS FORMSPEC

local mod_cmds = {}

local function load_mod_command_tree()
	mod_cmds = {}

	for name, def in pairs(core.registered_chatcommands) do
		mod_cmds[def.mod_origin] = mod_cmds[def.mod_origin] or {}
		local cmds = mod_cmds[def.mod_origin]

		-- Could be simplified, but avoid the priv checks whenever possible
		cmds[#cmds + 1] = { name, def }
	end
	local sorted_mod_cmds = {}
	for modname, cmds in pairs(mod_cmds) do
		table.sort(cmds, function(a, b) return a[1] < b[1] end)
		sorted_mod_cmds[#sorted_mod_cmds + 1] = { modname, cmds }
	end
	table.sort(sorted_mod_cmds, function(a, b) return a[1] < b[1] end)
	mod_cmds = sorted_mod_cmds
end

core.after(0, load_mod_command_tree)

local function build_chatcommands_formspec(name, sel, copy)
	local rows = {}
	rows[1] = "#FFF,0,Command,Parameters"

	local description = "For more information, click on any entry in the list.\n" ..
		"Double-click to copy the entry to the chat history."

	for i, data in ipairs(mod_cmds) do
		rows[#rows + 1] = COLOR_BLUE .. ",0," .. formspec_escape(data[1]) .. ","
		for j, cmds in ipairs(data[2]) do
			local has_priv = check_player_privs(name, cmds[2].privs)
			rows[#rows + 1] = ("%s,1,%s,%s"):format(
				has_priv and COLOR_GREEN or COLOR_GRAY,
				cmds[1], formspec_escape(cmds[2].params))
			if sel == #rows then
				description = cmds[2].description
				if copy then
					core.chat_send_player(name, ("Command: %s %s"):format(
						core.colorize("#0FF", "/" .. cmds[1]), cmds[2].params))
				end
			end
		end
	end

	return LIST_FORMSPEC_DESCRIPTION:format(
			"Available commands: (see also: /help <cmd>)",
			table.concat(rows, ","), sel or 0,
			description, "Close"
		)
end


-- PRIVILEGES FORMSPEC

local function build_privs_formspec(name)
	local privs = {}
	for priv_name, def in pairs(core.registered_privileges) do
		privs[#privs + 1] = { priv_name, def }
	end
	table.sort(privs, function(a, b) return a[1] < b[1] end)

	local rows = {}
	rows[1] = "#FFF,0,Privilege,Description"

	local player_privs = core.get_player_privs(name)
	for i, data in ipairs(privs) do
		rows[#rows + 1] = ("%s,0,%s,%s"):format(
			player_privs[data[1]] and COLOR_GREEN or COLOR_GRAY,
				data[1], formspec_escape(data[2].description))
	end

	return LIST_FORMSPEC:format(
			"Available privileges:",
			table.concat(rows, ","),
			"Close"
		)
end


-- Checks whether the mod with the given name is a separate mod or a part of a modpack
local function is_mod_sep_or_part_of_mp(modname)
    local modpath = core.get_modpath(modname)
    local find_ptn = "[^" .. DIR_DELIM .. "]*.$"
    local modpart = modpath:match(find_ptn)
    local mp_part = modpath:sub(1, modpath:len()-modpart:len()-1):match(find_ptn)
    
    if mp_part ~= "mods" then   -- this is inside of the modpack!
        return mp_part, modpart
    else                        -- this is a just separate mod!
        return modpart
    end
    
end


-- Returns a list with the modpack`s mods
local function get_modpack_mods(mp_name)
    local modnames = core.get_modnames()
    
    local modlist = {}
    for i, mname in ipairs(modnames) do
        local mpath = core.get_modpath(mname) 
        if mpath:match(DIR_DELIM .. mp_name .. DIR_DELIM) then
            table.insert(modlist, mname)
        end
    end
    
    return modlist
end
      

-- MODS FORMSPEC

local function build_mods_formspec()
    local modslist = core.get_modnames()
    local mps_and_smods = {}
    
    local rows = {}
    rows[1] = "#FFF,0,Mod or Modpack Name"
    for i = 1, #modslist do
        local mp, mod = is_mod_sep_or_part_of_mp(modslist[i])
        
        if mod then
            if #table.find(mps_and_smods, {mp, "modpack"}) == 0 then
                mps_and_smods[#mps_and_smods+1] = {mp, "modpack"}
            end
        else
            mps_and_smods[#mps_and_smods+1] = {mp, "separate_mod"}
        end
    end
    
    table.sort(mps_and_smods, function(e1, e2) return e1[1] < e2[1] end)
    for j = 1, #mps_and_smods do
        rows[#rows+1] = ("%s, 0, %s"):format(COLOR_BLUE, mps_and_smods[j][1])
        
        if mps_and_smods[j][2] == "modpack" then
            for _, mname in ipairs(get_modpack_mods(mps_and_smods[j][1])) do
                rows[#rows+1] = ("%s, 1, %s"):format(COLOR_GREEN, mname)
            end
        end
    end
        
        
                
                
    --[[for i, modname in ipairs(modslist) do
        local mp, mod = is_mod_sep_or_part_of_mp(modname)
        core.debug("Modpack Or Mod: " .. mp .. ", Mod: " .. (mod or ""))
        
        if mod then
            if not mps_and_smods[mp] then 
                mps_and_smods[mp] = {mod}
                rows[#rows+1] = ("%s, 0, %s"):format(COLOR_BLUE, mp)
            else
                mps_and_smods[mp][#mps_and_smods[mp]+1] = mod
                rows[#rows+1] = ("%s, 1, %s"):format(COLOR_GREEN, mod)
            end
        else
            mps_and_smods[mp] = mp
            rows[#rows+1] = ("%s, 0, %s"):format(COLOR_BLUE, mp)
        end
    end]]
    
    --[[for mname_i, mname in pairs(mps_and_smods) do
        rows[#rows+1] = ("%s, 0, %s"):format(COLOR_BLUE, mname_i)
        
        if mname_i ~= mname then
            for j, modname2 in ipairs(mname) do
                rows[#rows+1] = ("%s, 1, %s"):format(COLOR_GREEN, modname2)
            end
        end
    end]]
    
    return LIST_FORMSPEC_MODSLIST:format(
            "List of installed mods:",
            table.concat(rows, ","),
            "Close"
        )
        
end

        
--[[local function build_mods_formspec()
    local modslist = core.get_modnames()
    
    local rows = {}
    rows[1] = "#FFF,0,Mod"
    local find_ptn = "[^" .. DIR_DELIM .. "]*.$"
    for i, modname in ipairs(modslist) do
        local mp, mod = is_mod_sep_or_part_of_mp(modname)
        
        if mod then
            rows[#rows+1] = ("%s, 0, %s"):format(COLOR_BLUE, mp)
            
            for j, modname2 in ipairs(modslist) do
                local mp2, mod2 = is_mod_sep_or_part_of_mp(modname2)
                if mp2 == mp then
                    rows[#rows+1] = ("%s, 1, %s"):format(COLOR_GREEN, mod2)
                    table.remove(modslist, modname2)
                end
            end
        else
             rows[#rows+1] = ("%s, 0, %s"):format(COLOR_BLUE, mp)
        end
    end
    
    return LIST_FORMSPEC_MODSLIST:format(
            "List of installed mods:",
            table.concat(rows, ","),
            "Close"
        )
end
        local modpath = core.get_modpath(modname)
        
        core.debug("MODPATH: " .. modpath)
        local m1 = modpath:match(find_ptn)
        core.debug("mod1: " .. m1)
        local m2 = modpath:sub(1, modpath:len()-m1:len()-1):match(find_ptn)
        core.debug("mod2: " .. m2)
        
        if m2 ~= "mods" then      --  if true, this is a modpack
            rows[#rows+1] = ("%s, 0, %s"):format(COLOR_BLUE, m2)
            
            --  Then searching out its components
            for j=i+1, #modslist do
                local modname2 = modslist[j]
                local modpath2 = core.get_modpath(modname2)
                local m3 = modpath2:match(find_ptn)
                local m4 = modpath:sub(1, modpath2:len()-m3:len()-1):match(find_ptn)
                
                if m4 == m2 then
                    rows[#rows+1] = ("%s, 1, %s"):format(COLOR_GREEN, m3)
                end
            end
        else        -- Otherwise, this is a mod
            rows[#rows+1] = ("%s, 0, %s"):format(COLOR_BLUE, m1)
        end
    end
    
    return LIST_FORMSPEC_MODSLIST:format(
            "List of installed mods:",
            table.concat(rows, ","),
            "Close"
        )
end]]


-- DETAILED CHAT COMMAND INFORMATION

core.register_on_player_receive_fields(function(player, formname, fields)
	if formname ~= "__builtin:help_cmds" or fields.quit then
		return
	end

	local event = minetest.explode_table_event(fields.list)
	if event.type ~= "INV" then
		local name = player:get_player_name()
		core.show_formspec(name, "__builtin:help_cmds",
			build_chatcommands_formspec(name, event.row, event.type == "DCL"))
	end
end)


local help_command = core.registered_chatcommands["help"]
local old_help_func = help_command.func

help_command.func = function(name, param)
	local admin = core.settings:get("name")

	-- If the admin ran help, put the output in the chat buffer as well to
	-- work with the server terminal
	if param == "privs" then
		core.show_formspec(name, "__builtin:help_privs",
			build_privs_formspec(name))
		if name ~= admin then
			return true
		end
	end
    
    if param == "mods" then
        core.show_formspec(name, "__builtin:help_mods",
            build_mods_formspec())
        if name ~= admin then
            return true
        end
    end
    
	if param == "" or param == "all" then
		core.show_formspec(name, "__builtin:help_cmds",
			build_chatcommands_formspec(name))
		if name ~= admin then
			return true
		end
	end

	return old_help_func(name, param)
end

