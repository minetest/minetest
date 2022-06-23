
local function rename_favorite_formspec(dialogdata)
	local n_val = ""
	if dialogdata.nick then n_val = dialogdata.nick:trim() end

	local retval = "size[11.5,4.5,true]"
		.. "field[2.5,2;7,0.5;te_fav_nick;" .. fgettext("Set server nick:") .. ";" .. n_val .. "]"
		.. "button[3.25,3;2.5,0.5;btn_accept;" .. fgettext("Accept") .. "]"
		.. "button[5.75,3;2.5,0.5;btn_cancel;" .. fgettext("Cancel") .. "]"

	return retval
end

local function rename_favorite_buttonhandler(this, fields)
	if fields.btn_cancel then
		this:delete()
		return true
	elseif fields.btn_accept and fields.te_fav_nick then
		local nick = fields.te_fav_nick:trim()
		if nick ~= "" then
			this.data.nick = nick
			serverlistmgr.add_favorite(this.data)

			this:delete()
			return true
		end
	end

	return false
end

function create_rename_favorite_dlg(data)
	local retval = dialog_create(
		"dlg_rename_fav",
		rename_favorite_formspec,
		rename_favorite_buttonhandler,
		nil,
		data)

	return retval
end
