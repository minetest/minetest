
-- this is minimal try to test sending and receiving of server messages

local worldpath = minetest.get_worldpath()

local conf = worldpath.."/network.conf"

local f=io.open(conf,"r")
if f then
	f:close()

	conf = Settings(conf)

	local index = 0;
	local server = {}
	local servers = {}

	while true do
		if conf:has("server"..index.."_name") then
			server.name = conf:get("server"..index.."_name")
			server.address = conf:get("server"..index.."_address") or "localhost"
			server.port = tonumber(conf:get("server"..index.."_port") or "30000")
			server.auth_send = conf:get("server"..index.."_auth_send") or "NO_AUTH"
			server.auth_receive = conf:get("server"..index.."_auth_receive") or "NO_AUTH"

			minetest.set_another_server(server.name, server)
			local check = minetest.get_another_server(server.name)
			if check then
				table.insert(servers, server.name)
			else
				error("[devtest] Another server "..server.name.." setting failed: "..dump(server))
			end
		else
			break
		end
    index = index + 1
	end

	minetest.register_chatcommand("send_msg_to_server", {
			params = "server_name message",
			description = "Tune lighting parameters",
			privs = {server = true},
			func = function(player_name, param)
				local params = string.split(param, " ", false, 2)
				if #params < 2 then
					return false, "Expected server name and message body as argument."
				end
				if minetest.send_msg_another_server(params[1], params[2]) then
					return true, "Message has been sent."
				else
					return false, "Message sending failed."
				end
			end
		})
	minetest.register_on_server_receive_message(
			function (name, message)
				minetest.chat_send_all("Received server message from server "..name.."with body: "..message)
				minetest.log("action", "[devtest] Received server message from server "..name.."with body: "..message)
			end
		)
end
