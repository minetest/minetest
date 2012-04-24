--register stoppers for movestones/pistons

mesecon.mvps_stoppers={}

function mesecon:is_mvps_stopper(nodename)
	local i=1
	repeat
		i=i+1
		if mesecon.mvps_stoppers[i]==nodename then return true end
	until mesecon.mvps_stoppers[i]==nil
	return false
end

function mesecon:register_mvps_stopper(nodename)
	local i=1
	repeat
		i=i+1
		if mesecon.mvps_stoppers[i]==nil then break end
	until false
	mesecon.mvps_stoppers[i]=nodename
end

mesecon:register_mvps_stopper("default:chest")
mesecon:register_mvps_stopper("default:chest_locked")
mesecon:register_mvps_stopper("default:furnace")
