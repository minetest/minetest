core.register_on_generated(function(vm, blockseed)
	local pos1, pos2 = vm:get_emerged_area()
	print("generating chunk " .. core.pos_to_string(pos1) .. " -> " ..
		core.pos_to_string(pos2))

	local mid = pos1:add(pos2:subtract(pos1):divide(2))
	vm:set_node_at(mid, {name="default:mese"})
end)

--print(dump(_G))
