--
-- exception handler test module
--
--
-- To avoid this from crashing the module will startup in inactive mode.
-- to make specific errors happen you need to cause them by following
-- chat command:
--
-- exceptiontest <location> <errortype>
--
-- location has to be one of:
--   * mapgen:          cause in next on_generate call
--   * entity_step:     spawn a entity and make it do error in on_step
--   * globalstep:      do error in next globalstep
--   * immediate:       cause right in chat handler
--
-- errortypes defined are:
--   * segv:            make sigsegv happen
--   * zerodivision:    cause a division by zero to happen
--   * exception:       throw an exception

if core.cause_error == nil or
	type(core.cause_error) ~= "function" then
	return
end
	

core.log("action", "WARNING: loading exception handler test module!")

local exceptiondata = {
	tocause = "none",
	mapgen = false,
	entity_step = false,
	globalstep = false,
}

local exception_entity =
{
	on_step = function(self, dtime)
		if exceptiondata.entity_step then
			core.cause_error(exceptiondata.tocause)
		end
	end,
}
local exception_entity_name = "errorhandler_test:error_entity"

local function exception_chat_handler(playername, param)
	local parameters = param:split(" ")
	
	if #parameters ~= 2 then
		core.chat_send_player(playername, "Invalid argument count for exceptiontest")
	end
	
	core.log("error", "Causing error at:" .. parameters[1])
	
	if parameters[1] == "mapgen" then
		exceptiondata.tocause = parameters[2]
		exceptiondata.mapgen = true
	elseif parameters[1] == "entity_step" then
		--spawn entity at player location
		local player = core.get_player_by_name(playername)
		
		if player:is_player() then
			local pos = player:getpos()
			
			core.add_entity(pos, exception_entity_name)
		end
		
		exceptiondata.tocause = parameters[2]
		exceptiondata.entity_step = true
		
	elseif parameters[1] == "globalstep" then
		exceptiondata.tocause = parameters[2]
		exceptiondata.globalstep = true
		
	elseif parameters[1] == "immediate" then
		core.cause_error(parameters[2])
		
	else
		core.chat_send_player(playername, "Invalid error location: " .. dump(parameters[1]))
	end
end

core.register_chatcommand("exceptiontest",
	{
		params      = "<location> <errortype>",
		description = "cause a given error to happen.\n" ..
				" location=(mapgen,entity_step,globalstep,immediate)\n" ..
				" errortype=(segv,zerodivision,exception)",
		func        = exception_chat_handler,
		privs       = { server=true }
	})
	
core.register_globalstep(function(dtime)
	if exceptiondata.globalstep then
		core.cause_error(exceptiondata.tocause)
	end
end)

core.register_on_generated(function(minp, maxp, blockseed)
	if exceptiondata.mapgen then
		core.cause_error(exceptiondata.tocause)
	end
end)

core.register_entity(exception_entity_name, exception_entity)
