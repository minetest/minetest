
minetest.register_chatcommand("transfer_player", {
	params = "<server_ip/url> <port>",
	description = "Test transfer_player by disconnect and reconnect.",
	func = function(player_name, param)
		local player = minetest.get_player_by_name(player_name);
		if not player then return end

		local url, port = string.match(param, "([a-z0-9%.%/%-]+) (%d+)")

		if not url or not port then
			return false, "Bad IP/URL or/and port."
		end

		minetest.chat_send_all("Player "..player_name.." is being transfered to "..url.." port "..port..".")

		minetest.transfer_player(player_name, url, port)
		return true, "Transfering the connection..."
	end
})

