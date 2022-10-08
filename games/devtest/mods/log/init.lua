minetest.register_on_chatcommand(function(name, command, params)
	minetest.log("action", "[devtest] Caught command '"..command.."', issued by '"..name.."'. Parameters: '"..params.."'")
end)
