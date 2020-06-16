--Minetest
--Copyright (C) 2013 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

--------------------------------------------------------------------------------

function get_client_mods()
    local path = core.get_clientmodpath()
    local client_mods = core.get_dir_list(path, true)
    local retval = {}
    for _, name in ipairs(client_mods) do
        if name:sub(1, 1) ~= "." then
            table.insert(retval,name)
        end
    end
    return(retval)
end

local function get_mods_conf()
    local client_modpath = core.get_clientmodpath()

    local f = io.open(client_modpath..DIR_DELIM.."mods.conf", "r")
    local temp_settings = {}

    if f then
        for line in io.lines(client_modpath..DIR_DELIM.."mods.conf") do
            local name = string.gsub(string.match(line, "(.*) ="), "load_mod_", "")
            local value = string.match(line, "= (.*)")
            temp_settings[name] = value
        end
    end

    return(temp_settings)
end


local function build_menu()
    local mods = get_client_mods()

    local menu = "label[0,-0.25;" .. fgettext("Enabled Client Mods:") .. "]" ..
    "box[0,0.25;3.75,5;#999999]" ..
    "scrollbaroptions[min=6;max="..table.getn(mods).."]"..
    "scrollbar[3.75, 0.25; 0.3, 5;vertical;client_mods;]"..
    "scroll_container[ -0.25,0.7; 5.25, 5.7;client_mods;vertical]"

    local settings = get_mods_conf()
    local count = 0
    for _,name in pairs(mods) do
        local check = settings[name]
        if check == nil then
            check = "false"
        end
        menu = menu .. "checkbox[0.25,"..(count*0.3)..";"..name..";" .. fgettext(name) .. ";"
        .. check .. "]"
        count = count + 1
    end
    menu = menu .."scroll_container_end[]"..
    --this has to contain an unsupported character in the name
    "checkbox[4.25,0;!enable_client_modding;" .. fgettext("Enable Client Mods") .. ";"..dump(core.settings:get_bool("enable_client_modding")).. "]"..
    "label[4.25,-0.2;Warning! Client Modding is still unsupported and experimental.]"
    return menu
end

local function write_data(fields)

    if fields["!enable_client_modding"] then
        core.settings:set("enable_client_modding", fields["!enable_client_modding"])
        return
    elseif not fields.client_mods then
        return
    else
        fields.client_mods = nil
    end

    local path = core.get_clientmodpath()

    local new_name
    local new_data

    for name,data in pairs(fields) do
        new_name = name
        new_data = data
    end

    local settings = get_mods_conf()
    local mods = get_client_mods()
    local data = ""
    for _,name in pairs(mods) do
        local check = settings[name]
        if name == new_name then
            check = new_data
        end
        if check == nil then
            check = "false"
        end
        data = data .."load_mod_"..name.." = "..check.."\n"
    end

    local f = io.open(path..DIR_DELIM.."mods.conf", "w")
    f:write(data)
    f:close()
end

local function cbf_button_handler (this, fields, name, tabdata)
    write_data(fields)
end

return {
	name = "client mods",
	caption = fgettext("Client Mods"),
    cbf_formspec = function(tabview, name, tabdata)
		return build_menu()
	end,
	cbf_button_handler = cbf_button_handler,
}